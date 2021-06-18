#include "ShMemUtils.h"
#include <iostream>
#include <memory>

#if ON_WINDOWS
#define SERVER_PROC_NAME "ShMemServer.exe"
#else
#include <unistd.h>
#include <spawn.h>
#include <sys/wait.h>
#define SERVER_PROC_NAME "ShMemServer"
#endif

bool PrepareShMemSlotData(const char* SharedSlotName, int MaxSlotNum)
{
    std::shared_ptr<smi::ShMemHolder> SlotMemPtr = std::make_shared<smi::ShMemHolder>(SharedSlotName, MaxSlotNum);
    DebugPrint("SlotMemHolder:|%s|\n", SlotMemPtr->GetName());
    if (SlotMemPtr->IsValid())
    {
        DebugPrint("SlotData create succ.\n");
        smiAddressType SlotDataAddress = SlotMemPtr->GetAddress();
        DebugPrint("CreateSlotDataAddress:|%s|%lld|\n", SlotMemPtr->GetName(), (int64_t)SlotDataAddress);
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


int main()
{
    if(smi::CheckProcAliveByName(SERVER_PROC_NAME))
    {
        std::cout << "ShMemServer exists!" << std::endl;
        return 0;
    }

    if(!PrepareShMemSlotData(SH_MEM_SLOTDATA_NAME, MAX_CONN_NUM))
    {
        std::cout << "ServerMain PrepareSlotData Failed!" << std::endl;
        return 0;
    }
#if ON_WINDOWS
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Start the child process. 
    if (!CreateProcess(NULL,   // No module name (use command line)
        SERVER_PROC_NAME,        // Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // No creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory 
        &si,            // Pointer to STARTUPINFO structure
        &pi)           // Pointer to PROCESS_INFORMATION structure
        )
    {
        printf("CreateProcess failed (%d).\n", GetLastError());
        return 0;
    }
    // Wait until child process exits.
    WaitForSingleObject(pi.hProcess, INFINITE);

    // Close process and thread handles. 
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
#else
    int status;
    int pid = 0;
    pid = fork();
    switch (pid) {
        case -1:
            perror("fork");
            break;
        case 0:
            execl("./" SERVER_PROC_NAME, "ShMemServer", (char*)0);
            perror("exec");
            break;
        default:
            if (waitpid(pid, &status, 0) != -1) 
            {
                printf("Child exited with status %i\n", status);
            }
            else
            {
                perror("waitpid");
            }
        break;
    }
    return 0;
#endif
}
