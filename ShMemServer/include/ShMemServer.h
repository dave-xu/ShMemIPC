#pragma once

#include "ShMemUtils.h"
#include <map>
#include <vector>
#include <memory>

#define GOOD        0
#define SEMA_ERROR  1
#define SHMEM_ERROR 2

namespace smi
{
    class ShMemServer
    {
    public:
        ShMemServer(const char* aSharedSlotName, int MaxConnNum, int aDataSize);
        virtual ~ShMemServer();

        bool            CreateShMemSlotData();
        void            Loop();
        
    protected:
        bool            AllocateDataShMem(int SlotIndex);
        bool            ProcessShMemData(std::shared_ptr<ShMemHolder> DataShMemHolderPtr);
        void            Clean();

    private:
        char*           SharedSlotName;
        int             MaxSlotNum;
        int             MaxDataSize;

        std::shared_ptr<ShMemHolder>                    SlotMemPtr;
        std::map<char, std::shared_ptr<ShMemHolder>>    SlotToShMemData;
        std::map<KeyType, std::vector<char>>            DataMap;
    };
}