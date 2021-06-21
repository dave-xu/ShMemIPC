#pragma once

#include "ShMemUtils.h"
#include <map>
#include <vector>
#include <memory>

#define GOOD                0
#define MAX_IDLE_COUNT      5000000

namespace smi
{
    enum ProcState
    {
        NONE,
        GETVAL,
        SETVAL,
        ERRVAL
    };

    class ShMemServer
    {
    public:
        ShMemServer(const char* aSharedSlotName, unsigned char MaxConnNum, int aDataSize);
        virtual ~ShMemServer();

        bool            CreateShMemSlotData();
        void            Loop();
        
    protected:
        bool            AllocateDataShMem(unsigned char SlotIndex);
        ProcState       ProcessShMemData(std::shared_ptr<ShMemHolder> DataShMemHolderPtr);
        void            Clean();

    private:
        char*           SharedSlotName;
        unsigned char   MaxSlotNum;
        int             MaxDataSize;

        std::shared_ptr<ShMemHolder>                             SlotMemPtr;
        std::map<unsigned char, std::shared_ptr<ShMemHolder>>    SlotToShMemData;
        std::map<unsigned char, int>                             SlotTimeStamp;
        std::map<KeyType, std::vector<char>>                     DataMap;
    };
}