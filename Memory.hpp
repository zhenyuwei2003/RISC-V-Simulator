#ifndef RISC_V_SIMULATOR_MEMORY
#define RISC_V_SIMULATOR_MEMORY

#include <iostream>
#include <cstring>
#include <fstream>

using std::string;
using std::istream;
using u32 = unsigned int;

const u32 SIZE = 200000;

#define RISC_V_DEBUG

namespace MEMORY
{
    class Memory
    {
    public:
        unsigned char Data[SIZE];

        explicit Memory(istream &Input)
        {
            memset(Data, 0, sizeof(Data));
            string InputString;
            u32 pos = 0;
            while(!(Input >> InputString).eof())
            {
                char* ptr;
                if(InputString[0] == '@')
                    pos = strtoul(InputString.substr(1, 8).c_str(), &ptr, 16);
                else
                    Data[pos++] = strtoul(InputString.c_str(), &ptr, 16);
            }
        }

        u32 Load(u32 pos, int len)
        {
            u32 ret = 0;
            for(int i = len - 1; i >= 0; --i)
            {
                ret <<= 8u;
                ret += Data[pos + i];
            }
#ifdef RISC_V_DEBUG
            printf("\nLoad Data[%x]: %d 0x%08x", pos, ret, ret);
#endif
            return ret;
        }

        void Store(u32 pos, int len, u32 val)
        {
#ifdef RISC_V_DEBUG
            printf("\nStore Data[%x]: %d 0x%08x", pos, val, val);
#endif
            for(int i = 0; i < len; ++i)
            {
                Data[pos + i] = val & 0b11111111u;
                val >>= 8u;
            }
        }
    };

    void DEBUGprintf(const char* s)
    {
#ifdef RISC_V_DEBUG
        printf("%s", s);
#endif
    }
}

#endif // RISC_V_SIMULATOR_MEMORY