#include "CPU.hpp"
//#include "LocalTest.h"

int main()
{
#ifdef RISC_V_SIMULATOR_LOCAL_TEST
    for (int i = 0; i < 18; ++i)
    {
        printf("[%s]\n", FileTable[i].c_str());
        ifstream IN("testcases/" + FileTable[i] + ".data");
        CPU::CPU cpu(IN);
        int Ans = cpu.run();
        printf("Ans: %d\n", Ans);
        printf("%s\n\n", Ans == AnsTable[i] ? "Correct!" : "Wrong!");
    }
    return 0;
#endif

#ifndef RISC_V_SIMULATOR_LOCAL_TEST
    CPU::CPU cpu(std::cin);
    u32 ans = cpu.run();
    printf("%d\n", ans);
    return 0;
#endif
}
