#ifndef RISC_V_SIMULATOR_REGISTER
#define RISC_V_SIMULATOR_REGISTER

#include <iostream>
#include <cstring>

using u32 = unsigned int;

namespace REGISTER
{
    const string RegTable[] =
    {
        "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
        "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
        "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
        "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
    };

    class Register
    {
    private:
        u32 Data[32];

    public:
        Register() { memset(Data, 0, sizeof(Data)); }

        u32 Load(u32 index) { return Data[index]; }
        void Store(u32 index, u32 val) { if (index) Data[index] = val; }
    };
} // namespace REGISTER

#endif // RISC_V_SIMULATOR_REGISTER