#include "ShMemUtils.h"
#include <iostream>
#include <fstream>
#include <cassert>

namespace smi
{
	bool CAllocAndClone(char*& Dest, const char* Source)
	{
	    size_t len = strlen(Source);
	    Dest = new char[len + 1];
	    if (!Dest)
	    {
	        return false;
	    }
	    #if ON_WINDOWS
	    strncpy_s(Dest, len + 1, Source, len);
	    #else
	    strncpy(Dest, Source, len);
	    #endif
	    Dest[len] = 0;
	    return true;
	}

	void MemoryFence()
	{
#if ON_WINDOWS
		MemoryBarrier();
#else
		asm volatile("" ::: "memory");
#endif
	}

	bool CheckProcAliveByName(const char* ProcName)
	{
#if ON_WINDOWS
#pragma warning(disable : 4302)
#undef UNICODE
		PROCESSENTRY32 entry;
		entry.dwSize = sizeof(PROCESSENTRY32);

		HANDLE hProcess = NULL;

		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
		if (Process32First(snapshot, &entry) == TRUE)
		{
			while (Process32Next(snapshot, &entry) == TRUE)
			{
				if (stricmp(entry.szExeFile, ProcName) == 0)
				{
					hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);
					CloseHandle(hProcess);
					break;
				}
			}
		}
#define UNICODE
#pragma warning(error : 4302)
		return (hProcess != NULL);
#else
		int pid = -1;
		// Open the /proc directory
		DIR* dp = opendir("/proc");
		if (dp != NULL)
		{
			// Enumerate all entries in directory until process found
			struct dirent* dirp;
			while (pid < 0 && (dirp = readdir(dp)))
			{
				// Skip non-numeric entries
				int id = atoi(dirp->d_name);
				if (id > 0)
				{
					// Read contents of virtual /proc/{pid}/cmdline file
					std::string cmdPath = std::string("/proc/") + dirp->d_name + "/cmdline";
					std::ifstream cmdFile(cmdPath.c_str());
					std::string cmdLine;
					getline(cmdFile, cmdLine);
					if (!cmdLine.empty())
					{
						// Keep first cmdline item which contains the program path
						size_t pos = cmdLine.find('\0');
						if (pos != std::string::npos)
							cmdLine = cmdLine.substr(0, pos);
						// Keep program name only, removing the path
						pos = cmdLine.rfind('/');
						if (pos != std::string::npos)
							cmdLine = cmdLine.substr(pos + 1);
						// Compare against requested process name
						if (ProcName == cmdLine)
							pid = id;
					}
				}
			}
		}

		closedir(dp);

		return pid != -1;
#endif
	}




	ShMemHolder::ShMemHolder(const char* name, smiSizeType size)
		: Size(size)
#if ON_WINDOWS
		, Handle(NULL)
#else
		, Handle(-1)
#endif
		, MappedAddress(0)
		, Initialized(false)
	{
		CAllocAndClone(Name, name);
		FullShMemName = new char[MAX_PATH_LEN];
		memset(FullShMemName, 0, MAX_PATH_LEN);
		#if ON_WINDOWS
		snprintf(FullShMemName, MAX_PATH_LEN, "%s\\%s", "Local", Name);
		#else
		snprintf(FullShMemName, MAX_PATH_LEN, "/%s", Name);
		#endif
		CreateOrOpen();
	}

	ShMemHolder::~ShMemHolder()
	{
		Close();
		delete[] Name;
		delete[] FullShMemName;
	}

	void ShMemHolder::CreateOrOpen()
	{
		if (!Initialized)
		{
#if ON_WINDOWS
			Handle = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, Size, FullShMemName);
			if (Handle != NULL)
			{
				MappedAddress = MapViewOfFile(Handle, FILE_MAP_ALL_ACCESS, 0, 0, Size);
			}
			if (Handle && MappedAddress)
			{
				Initialized = true;
			}
#else
			Handle = shm_open(FullShMemName, (O_CREAT | O_RDWR), FILE_PERMISSION);
			if (Handle != -1)
			{
				if (ftruncate(Handle, Size) != -1)
				{
					MappedAddress = (uint8_t*)mmap(0, Size, (PROT_READ | PROT_WRITE), MAP_SHARED, Handle, 0);
					if (MappedAddress != MAP_FAILED)
					{
						Initialized = true;
					}
				}
			}
			else
			{
				DebugPrint("shm_open failed.");
			}
#endif
		}
	}

	void ShMemHolder::Close()
	{
		if(Initialized)
		{
#if ON_WINDOWS
			if (MappedAddress)
			{
				UnmapViewOfFile(MappedAddress);
				MappedAddress = NULL;
			}
			if (Handle)
			{
				CloseHandle(Handle);
				Handle = NULL;
			}
			Initialized = false;
#else

			if (MappedAddress != MAP_FAILED)
			{
				munmap(MappedAddress, Size);
				MappedAddress = NULL;
			}
			if (Handle != -1)
			{
				close(Handle);
				Handle = -1;
				//shm_unlink(FullShMemName);
			}
#endif
		}
	}

	bool ShMemHolder::IsValid()
	{
		return Initialized;
	}

	const char* ShMemHolder::GetName()
	{
		return Name;
	}

	smiSizeType ShMemHolder::GetSize()
	{
		return Size;
	}

	smiAddressType ShMemHolder::GetAddress()
	{
		return MappedAddress;
	}

}
