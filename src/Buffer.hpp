#ifndef RISC_V_SIMULATOR_BUFFER
#define RISC_V_SIMULATOR_BUFFER

#include "Instruction.hpp"

using namespace INSTRUCTION;

namespace BUFFER
{
    struct IF_Buffer
    {
        u32 pc = 0;
        u32 pcPredict = 0;
        u32 Ins = 0;

        void Clear() { pc = Ins = pcPredict = 0; }
    };

    struct ID_Buffer
    {
        INS_TYPE InsType = NOP;
        u32 pc = 0;
        u32 pcPredict = 0;
        u32 rd = 0;
        u32 imm = 0;
        u32 rv1 = 0;
        u32 rv2 = 0;

        void Clear()
        {
            pc = rd = imm = rv1 = rv2 = pcPredict = 0;
            InsType = NOP;
        }
    };

    struct EX_Buffer
    {
        INS_TYPE InsType = NOP;
        u32 pc = 0;
        u32 rd = 0;
        u32 exr = 0;
        u32 ad = 0;

        void Clear()
        {
            pc = rd = exr = ad = 0;
            InsType = NOP;
        }
    };

    struct MEM_Buffer
    {
        INS_TYPE InsType = NOP;
        u32 pc = 0;
        u32 rd = 0;
        u32 exr = 0;

        void Clear()
        {
            pc = rd = exr = 0;
            InsType = NOP;
        }
    };

    struct WB_Buffer
    {
        INS_TYPE InsType = NOP;
        u32 pc = 0;
        u32 rd = 0;
        u32 exr = 0;

        void Clear()
        {
            pc = rd = exr = 0;
            InsType = NOP;
        }
    };
} // namespace BUFFER

#endif // RISC_V_SIMULATOR_BUFFER