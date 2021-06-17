// ShMemClient.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "ShMemClient.h"
#include "ShMemUtils.h"
#include <iostream>
#include <cassert>
#include <atomic>
#include <chrono>
#if ON_WINDOWS
#define SERVER_PROC_NAME "ShMemServer.exe"
#else
#include <time.h>
#define SERVER_PROC_NAME "ShMemServer"
#endif

double GetTickCountA()
{
    #if ON_WINDOWS
    __int64 Freq = 0;

    __int64 Count = 0;

    if (QueryPerformanceFrequency((LARGE_INTEGER*)&Freq)

        && Freq > 0

        && QueryPerformanceCounter((LARGE_INTEGER*)&Count))

    {
        return (double)Count / (double)Freq * 1000.0;

    }
    #else
        using namespace std::chrono;
        return duration_cast< milliseconds >(
            system_clock::now().time_since_epoch()
        ).count();
    #endif

    return 0.0;

}

int main()
{
    std::cout << "Hello World!\n";

    if(!smi::CheckProcAliveByName(SERVER_PROC_NAME))
    {
        std::cout << "ShMemServer not exists!" << std::endl;
        return 0;
    }


    smi::ShMemClient smc(SH_MEM_SLOTDATA_NAME, MAX_CONN_NUM, MAX_DATA_SIZE);
    if (smc.IsValid())
    {
        std::cout << "open -data succ" << std::endl;
        int IncNum = 0;
        int tmp = 100000;
        double start = GetTickCountA();
        while (--tmp)
        {
            char buff[128] = { 0 };
            char buff_data[256] = { 0 };
            int datalen = 0;
            #if ON_WINDOWS
            sprintf_s(buff, 128, "test--%d", ++IncNum);
            #else
            sprintf(buff, "test--%d", ++IncNum);
            #endif
            KeyType key(10 + IncNum);
            //std::cout << "set-v:" << buff << std::endl;
            smc.SetValue(key, buff, 128);
            //std::cout << "get-v:" << buff << std::endl;
            if(!smc.GetValue(key, buff_data, 256, datalen))
            {
                std::cout << "GetValue Failed" << std::endl;
                continue;
            }
            if (strcmp(buff, buff_data) != 0)
            {
                char* p = 0;
                *p = 100;
            }
            else
            {
                //std::cout << "match!!!" << buff << "|" << buff_data << std::endl;
                
            }
        }
        double end = GetTickCountA();
        double val = (end - start);
        printf("100000 times: %lf", val);
    }
}
