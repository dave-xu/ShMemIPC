#include "ShMemServer.h"
#include <iostream>
#include <inttypes.h>
#if ON_WINDOWS
#else
#include <sched.h>
#include <time.h>
#endif

namespace smi
{
    ShMemServer::ShMemServer(const char* aSharedSlotName, int MaxConnNum, int aDataSize)
        : MaxSlotNum(MaxConnNum)
        , MaxDataSize(aDataSize)
    {
        CAllocAndClone(SharedSlotName, aSharedSlotName);
    }

    // ShMemServer would never try to destroy shared memory or semaphore.
    // It always tries to reuse them first, and create them if not exists.
    ShMemServer::~ShMemServer()
    {
        delete[] SharedSlotName;
        SharedSlotName = 0;
    }

    bool ShMemServer::CreateShMemSlotData()
    {
        SlotMemPtr = std::make_shared<ShMemHolder>(SharedSlotName, MaxSlotNum);
        DebugPrint("SlotMemHolder:|%s|\n", SlotMemPtr->GetName());
        if (SlotMemPtr->IsValid())
        {
            DebugPrint("SlotData create succ.\n");
            smiAddressType SlotDataAddress = SlotMemPtr->GetAddress();
            DebugPrint("CreateSlotDataAddress:|%s|%" PRId64 "|\n", SlotMemPtr->GetName(), (int64_t)SlotDataAddress);
            memset(SlotDataAddress, 0, MaxSlotNum);
            DebugPrint("SlotData memset ok.\n");
            return true;
        }
        else
        {
            DebugPrint("SlotData create failed, release Semaphore and exit.\n");
            return false;
        }
    }

    bool ShMemServer::AllocateDataShMem(int SlotIndex)
    {
        char ConcatName[MAX_PATH_LEN] = { 0 };
        #if ON_WINDOWS
        sprintf_s(ConcatName, MAX_PATH_LEN, "%s-%d", SharedSlotName, SlotIndex);
        #else
        sprintf(ConcatName, "%s-%d", SharedSlotName, SlotIndex);
        #endif
        std::shared_ptr<ShMemHolder> sh(new ShMemHolder(ConcatName, MaxDataSize));
        DebugPrint("");
        if(sh->IsValid())
        {
            SlotToShMemData[SlotIndex] = sh;
            return true;
        }
        return false;
    }

    bool ShMemServer::ProcessShMemData(std::shared_ptr<ShMemHolder> DataShMemHolderPtr)
    {
        smiAddressType DataAddress = DataShMemHolderPtr->GetAddress();
        ShMemDataHead* Head = (ShMemDataHead*)DataAddress;
        if (std::atomic_load(&(Head->KeyStat)) == KEY_DIRTY)
        {
            auto itr = DataMap.find(Head->Key);
            if (itr != DataMap.end())
            {
                std::vector<char>& vec = itr->second;
                char* p = (char*)DataAddress + sizeof(ShMemDataHead);
                if(MAX_DATA_SIZE - sizeof(ShMemDataHead) <= vec.size())
                {
                    printf("Data exceed max size.\n");
                    return false;
                }
                memmove(p, vec.data(), vec.size());
                std::atomic_store(&Head->DataLen, (uint32_t)vec.size());
            }
            else
            {
                std::atomic_store(&Head->DataLen, 0);
            }
            MemoryFence();
            std::atomic_store(&Head->KeyStat, KEY_CLEAN);
        }
        else if (std::atomic_load(&(Head->KeyStat)) == KEY_SET)
        {
            char* p = (char*)DataAddress + sizeof(ShMemDataHead);
            uint32_t DataLen = std::atomic_load(&Head->DataLen);
            DataMap[Head->Key] = std::vector<char>(p, p + DataLen);
            MemoryFence();
            std::atomic_store(&Head->KeyStat, KEY_CLEAN);
        }
        return true;
    }


    void ShMemServer::Loop()
    {
        printf("Server Begin Loop.\n");
        int state = GOOD;
        int count = 0;
        while (state == GOOD)
        {
            smiAddressType SlotDataAddress = SlotMemPtr->GetAddress();
            char* p = (char*)SlotDataAddress;
            if (p[0])
            {
                for (int i = 1; i < MaxSlotNum; ++i)
                {
                    if (p[i] != 0)
                    {
                        if (SlotToShMemData.find(i) == SlotToShMemData.end())
                        {
                            DebugPrint("AllocateNewDataIndex:|%d|\n", i);
                            if(AllocateDataShMem(i))
                            {
                                DebugPrint("AllocateNewDataShMem_index:%d succ\n", i);
                            }
                            else
                            {
                                state = SHMEM_ERROR;
                                DebugPrint("AllocateNewDataShMem failed\n");
                                Clean();
                                return;
                            }
                        }
                    }
                }
                --p[0];
            }

            for (auto itr = SlotToShMemData.begin(); itr != SlotToShMemData.end(); ++itr)
            {
                ++count;
                //DebugPrint("ProcessIndex:|%d|%s|%d|\n", itr->first, itr->second->GetName(), count);
                if(!ProcessShMemData(itr->second))
                {
                    DebugPrint("ProcessShMemData failed!\n");
                    Clean();
                    return;
                }
            }
            #if ON_WINDOWS
            Sleep(0);
            #else
            sched_yield();
            #endif
        }
    }

    void ShMemServer::Clean()
    {

    }
}

int main()
{
    smi::ShMemServer sms(SH_MEM_SLOTDATA_NAME, MAX_CONN_NUM, MAX_DATA_SIZE);
    if (sms.CreateShMemSlotData())
    {
        sms.Loop();
    }
    return 0;
}
