#include "CPU.hpp"

int main()
{
    //std::ifstream IN("testcases/superloop.data");
    CPU::CPU cpu(std::cin);
    u32 ans = cpu.run();
    printf("%d\n", ans);
    return 0;
}
