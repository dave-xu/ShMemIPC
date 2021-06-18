#pragma once

#include "ShMemUtils.h"
#include <memory>

namespace smi
{
    class ShMemClient
    {
    public:
        explicit ShMemClient(const char* aSharedSlotName, int MaxConnNum, int aDataSize);
        virtual ~ShMemClient();

    public:
        bool            IsValid();
        bool            GetValue(KeyType key, char* Data, int BuffLen, int& DataLen);
        void            SetValue(KeyType key, char* Data, int DataLen);

    private:
        bool            AllocateShMemSlotIndex();
        void            OpenData();
        void            FreeShMemSlotIndex();
        bool            WaitForClean(volatile ShMemDataHead* Head);

        ShMemClient(const ShMemClient&);
        ShMemClient(const ShMemClient&&);

    private:
        char*                           SharedSlotName;
        int                             MaxSlotNum;
        int                             MaxDataSize;
        int                             AllocatedSlotIndex;
        int                             FailedCount;

        std::shared_ptr<ShMemHolder>    ShMemSlotPtr;
        std::shared_ptr<ShMemHolder>    ShMemDataPtr;
        bool                            Initialized;
    };
}
