#pragma once

// platform-specific headers.
#if defined(_MSC_VER)
# define ON_WINDOWS		1
# include <windows.h>
#undef UNICODE
#include <tlhelp32.h>
#define UNICODE
#else
# define ON_WINDOWS		0
# include <unistd.h>
# include <fcntl.h>
# include <sys/mman.h>
# include <sys/stat.h>
# include <string.h>
# include <linux/kernel.h>
# include <dirent.h>
#endif

#include <cstdint>
#include <atomic>
#include <string>

#define DEBUG						1
#define DebugPrint(fmt, ...)		do { if (DEBUG) fprintf(stderr, "File:%s, Line:%d\n" fmt, __FILE__, __LINE__, ##__VA_ARGS__ ); } while (0)


#if ON_WINDOWS
#define smiSizeType                  DWORD
#define smiAddressType               LPVOID
#define smiFileHandle                HANDLE
#define smiInvalidFileHandle         INVALID_HANDLE_VALUE
#define MAX_PATH_LEN                 128
#else
#define smiSizeType                  unsigned int
#define smiAddressType               uint8_t*
#define smiFileHandle                int
#define smiInvalidFileHandle         -1
#define MAX_PATH_LEN                 128
#define FILE_PERMISSION              S_IRWXU
#endif

#define MAX_CONN_NUM                        255
#define MAX_DATA_SIZE                       24576
#define SH_MEM_SLOTDATA_NAME                "SHMEMIPCSLOTS"

#define KEY_CLEAN                           0
#define KEY_GET                             1
#define KEY_SET                             2

#define MAX_TRY_NUM                         50000
#define MAX_FAIL_NUM	                    5


namespace smi
{
    typedef volatile char          SlotType;
    typedef volatile uint64_t      KeyType;
    typedef volatile char          KeyStatType;
    typedef volatile uint32_t      KeyLenType;

    SlotType 			TestAndSwapSlotVal(SlotType*    ptr, SlotType       expected, SlotType      newval);
    KeyStatType 		TestAndSwapKeyStat(KeyStatType* ptr, KeyStatType    expected, KeyStatType   newval);

    bool				CAllocAndClone(char*& Dest, const char* Source);
    void                MemoryFence();
    bool                CheckProcAliveByName(const char* ProcName);

    struct ShMemDataHead
    {
        KeyStatType     KeyStat;
        KeyType         Key;
        KeyLenType      DataLen;
    };



    class ShMemHolder
    {
    public:
        explicit ShMemHolder(const char* name, smiSizeType size);
        ~ShMemHolder();

    private:
        void				CreateOrOpen();
        void				Close();

    public:
        bool 				IsValid();
        const char*			GetName();
        smiSizeType			GetSize();
        smiAddressType		GetAddress();

    private:
        ShMemHolder(const ShMemHolder&);
        ShMemHolder(const ShMemHolder&&);

    private:
        char*				Name;
        char* 				FullShMemName;
        smiSizeType			Size;
        smiFileHandle		Handle;
        smiAddressType		MappedAddress;
        bool				Initialized;
    };
}
