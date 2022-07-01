#include "CPU.hpp"

int main()
{
#ifdef RISC_V_SIMULATOR_LOCAL_TEST
    for (int i = 0; i < 18; ++i)
    {
        printf("[%s]\n", FileTable[i].c_str());
        ifstream IN("testcases/" + FileTable[i] + ".data");
        CPU::CPU cpu(IN);
        u32 Ans = cpu.run();
        printf("Ans: %u\n", Ans);
        printf("%s\n\n", Ans == AnsTable[i] ? "Correct!" : "Wrong!");
    }
    printf("Average Correct Rate: %lf%%\n", ToTalCorrectRate / 17);
    return 0;
#endif

#ifndef RISC_V_SIMULATOR_LOCAL_TEST
    CPU::CPU cpu(std::cin);
    u32 ans = cpu.run();
    printf("%u\n", ans);
    return 0;
#endif
}
