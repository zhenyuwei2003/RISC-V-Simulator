#ifndef RISC_V_SIMULATOR_REGISTER
#define RISC_V_SIMULATOR_REGISTER

#include <iostream>
#include <cstring>

using u32 = unsigned int;

namespace REGISTER
{
    class Register
    {
    private:
        u32 Data[32];

    public:
        Register() { memset(Data, 0, sizeof(Data)); }

        u32 Load(u32 index) { return Data[index]; }
        void Store(u32 index, u32 data)
        {
            if(!index) return;
            Data[index] = data;
        }
    };
} // namespace REGISTER

#endif // RISC_V_SIMULATOR_REGISTER