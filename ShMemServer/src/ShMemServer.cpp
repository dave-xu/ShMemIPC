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
    ShMemServer::ShMemServer(const char* aSharedSlotName, unsigned char MaxConnNum, int aDataSize)
        : MaxSlotNum(MaxConnNum)
        , MaxDataSize(aDataSize)
    {
        CAllocAndClone(SharedSlotName, aSharedSlotName);
    }

    // ShMemServer would never try to destroy shared memory.
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
            DebugPrint("SlotData create failed.\n");
            return false;
        }
    }

    bool ShMemServer::AllocateDataShMem(unsigned char SlotIndex)
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
            SlotTimeStamp[SlotIndex] = 0;
            return true;
        }
        return false;
    }

    ProcState ShMemServer::ProcessShMemData(std::shared_ptr<ShMemHolder> DataShMemHolderPtr)
    {
        smiAddressType DataAddress = DataShMemHolderPtr->GetAddress();
        ShMemDataHead* Head = (ShMemDataHead*)DataAddress;
        if (Head->KeyStat == KEY_GET)
        {
            auto itr = DataMap.find(Head->Key);
            if (itr != DataMap.end())
            {
                std::vector<char>& vec = itr->second;
                char* p = (char*)DataAddress + sizeof(ShMemDataHead);
                if(MAX_DATA_SIZE - sizeof(ShMemDataHead) <= vec.size())
                {
                    printf("Data exceed max size.\n");
                    return ERRVAL;
                }
                memmove(p, vec.data(), vec.size());
                Head->DataLen = (uint32_t)vec.size();
            }
            else
            {
                Head->DataLen = 0;
            }
            MemoryFence();
            KeyStatType expected = KEY_GET;
            if(expected != TestAndSwapKeyStat(&Head->KeyStat, expected, KEY_CLEAN))
            {
                return ERRVAL;
            }
            return GETVAL;
        }
        else if (Head->KeyStat == KEY_SET)
        {
            char* p = (char*)DataAddress + sizeof(ShMemDataHead);
            uint32_t DataLen = Head->DataLen;
            DataMap[Head->Key] = std::vector<char>(p, p + DataLen);
            MemoryFence();
            KeyStatType expected = KEY_SET;
            if(expected != TestAndSwapKeyStat(&Head->KeyStat, expected, KEY_CLEAN))
            {
                return ERRVAL;
            }
            return SETVAL;
        }
        return NONE;
    }


    void ShMemServer::Loop()
    {
        printf("Server Begin Loop.\n");
        int state = GOOD;
        while (state == GOOD)
        {
            smiAddressType SlotDataAddress = SlotMemPtr->GetAddress();
            SlotType* p = (SlotType*)SlotDataAddress;
            if (p[0])
            {
                for (unsigned char i = 1; i < MaxSlotNum; ++i)
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
                //DebugPrint("ProcessIndex:|%d|%s|%d|\n", itr->first, itr->second->GetName(), count);
                ProcState ps = ProcessShMemData(itr->second);
                if(NONE == ps)
                {
                    ++SlotTimeStamp[itr->first];
                }
                else if(ERRVAL == ps)
                {
                    DebugPrint("ProcessShMemData failed!\n");
                    Clean();
                    return;
                }
                else
                {
                    SlotTimeStamp[itr->first] = 0;
                }
            }

            static std::vector<unsigned char> tmpdel;
            tmpdel.clear();           

            for(auto itr = SlotTimeStamp.begin(); itr != SlotTimeStamp.end(); ++itr)
            {
                if(itr->second > MAX_IDLE_COUNT)
                {
                    tmpdel.emplace_back(itr->first);
                }
            }

            for(unsigned char idx : tmpdel)
            {
                //DebugPrint("recycle-idx:%d,count:%d\n", idx, SlotTimeStamp[idx]);
                SlotType expected = 1;
                TestAndSwapSlotVal(&p[idx], expected, 0);                
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
