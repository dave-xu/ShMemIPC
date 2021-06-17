// ShMemClient.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "ShMemClient.h"
#include <iostream>
#include <cassert>
#include <atomic>
#include <chrono>
#if ON_WINDOWS
#else
#include <sched.h>
#include <time.h>
#endif

namespace smi
{
    ShMemClient::ShMemClient(const char* aSharedSlotName, int MaxConnNum, int aDataSize)
        : MaxSlotNum(MaxConnNum)
        , MaxDataSize(aDataSize)
        , AllocatedSlotIndex(-1)
        , Initialized(false)
    {
        CAllocAndClone(SharedSlotName, aSharedSlotName);
        if(AllocateShMemSlotIndex())
        {
            OpenData();
        }
    }

    ShMemClient::~ShMemClient()
    {
        delete[] SharedSlotName;
        FreeShMemSlotIndex();
    }

    bool ShMemClient::IsValid()
    {
        return Initialized;
    }

    bool ShMemClient::AllocateShMemSlotIndex()
    {
        if(AllocatedSlotIndex == -1)
        {
            ShMemSlotPtr = std::make_shared<ShMemHolder>(SharedSlotName, MaxSlotNum);
            DebugPrint("SlotMemHolder:|%s|\n", ShMemSlotPtr->GetName());
            if (!ShMemSlotPtr->IsValid())
            {
                printf("share memory create failed.\n");
                AllocatedSlotIndex = -1;
                return false;
            }
            else
            {
                smiAddressType SlotDataAddress = ShMemSlotPtr->GetAddress();
                char* p = (char*)SlotDataAddress;
                for (int i = 1; i < MAX_CONN_NUM; ++i)
                {
                    if (p[i] == 0)
                    {                    
                        p[i] = 1;
                        AllocatedSlotIndex = i;
                        MemoryFence();
                        ++p[0];
                        printf("AllocateShMemSlot:|%d|\n", AllocatedSlotIndex);
                        return true;
                    }
                }
                AllocatedSlotIndex = -1;
                return false;
            }
        }
        else
        {
            return true;
        }
    }

    void ShMemClient::OpenData()
    {
        if (AllocatedSlotIndex == -1)
        {
            return;
        }
        if(!Initialized)
        {
            char ConcatName[MAX_PATH_LEN] = { 0 };
            #if ON_WINDOWS
            sprintf_s(ConcatName, MAX_PATH_LEN, "%s-%d", SharedSlotName, AllocatedSlotIndex);
            #else
            sprintf(ConcatName, "%s-%d", SharedSlotName, AllocatedSlotIndex);
            #endif
            ShMemDataPtr = std::make_shared<ShMemHolder>(ConcatName, MaxDataSize);
            DebugPrint("ClientOpenData:%s", ShMemDataPtr->GetName());
            if(ShMemDataPtr->IsValid())
            {
                Initialized = true;
            }
        }
    }

    bool ShMemClient::WaitForClean(volatile ShMemDataHead* Head)
    {
        for(int i = 0; i < MAX_TRY_NUM; ++i)
        {
            if(std::atomic_load(&(Head->KeyStat)) == KEY_CLEAN)
            {
                return true;
            }
            else
            {
                #if ON_WINDOWS
                Sleep(0);
                #else
                sched_yield();
                #endif
            }
        }
        return false;
    }

    bool ShMemClient::GetValue(KeyType key, char* Data, int BuffLen, int& DataLen)
    {
        smiAddressType DataAddress = ShMemDataPtr->GetAddress();
        volatile ShMemDataHead* Head = (volatile ShMemDataHead*)DataAddress;
        if(!WaitForClean(Head))
        {
            return false;
        }
        MemoryFence();
        Head->Key = key;
        MemoryFence();
        Head->KeyStat = KEY_DIRTY;
        MemoryFence();
        if(!WaitForClean(Head))
        {
            return false;
        }
        DataLen = std::atomic_load(&Head->DataLen);
        if (DataLen == 0)
        {
            return false;
        }
        char* p = (char*)DataAddress + sizeof(ShMemDataHead);
        #if ON_WINDOWS
        memcpy_s(Data, BuffLen, p, DataLen);
        #else
        memcpy(Data, p, DataLen);
        #endif
        return true;
    }

    void ShMemClient::SetValue(KeyType key, char* Data, int DataLen)
    {
        smiAddressType DataAddress = ShMemDataPtr->GetAddress();
        volatile ShMemDataHead* Head = (volatile ShMemDataHead*)DataAddress;
        if(!WaitForClean(Head))
        {
            return;
        }
        // std::cout << "set-wait-key-clean" << std::endl;
        MemoryFence();
        Head->Key = key;
        char* p = (char*)DataAddress + sizeof(ShMemDataHead);
        #if ON_WINDOWS
        memcpy_s(p, MAX_DATA_SIZE - sizeof(ShMemDataHead), Data, DataLen);
        #else
        memcpy(p, Data, DataLen);
        #endif
        Head->DataLen = DataLen;
        MemoryFence();
        Head->KeyStat = KEY_SET;
    }

    void ShMemClient::FreeShMemSlotIndex()
    {
        if (ShMemSlotPtr->IsValid())
        {
            smiAddressType SlotDataAddress = ShMemSlotPtr->GetAddress();
            char* p = (char*)SlotDataAddress;
            p[AllocatedSlotIndex] = 0;
            AllocatedSlotIndex = -1;
        }
    }
}


