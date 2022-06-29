#ifndef RISC_V_SIMULATOR_BUFFER
#define RISC_V_SIMULATOR_BUFFER

#include "Instruction.hpp"

namespace BUFFER
{
    struct IF_Buffer
    {
        u32 pc = 0;
        INSTRUCTION::INS_TYPE InsType = INSTRUCTION::NOP;
        u32 rd = 0;
        u32 imm = 0;
        u32 rs1 = 0;
        u32 rs2 = 0;
        u32 RegNum = 0;
        int pcPredict = 0;

        void Clear()
        {
            pc = rd = imm = rs1 = rs2 = RegNum = pcPredict = 0;
            InsType = INSTRUCTION::NOP;
        }
    };

    struct ID_Buffer
    {
        u32 pc = 0;
        INSTRUCTION::INS_TYPE InsType = INSTRUCTION::NOP;
        u32 rd = 0;
        u32 imm = 0;
        u32 rs1 = 0;
        u32 rs2 = 0;
        u32 rv1 = 0;
        u32 rv2 = 0;
        int pcPredict = 0;

        void Clear()
        {
            pc = rd = imm = rs1 = rs2 = rv1 = rv2 = pcPredict = 0;
            InsType = INSTRUCTION::NOP;
        }
    };

    struct EX_Buffer
    {
        u32 pc = 0;
        INSTRUCTION::INS_TYPE InsType = INSTRUCTION::NOP;
        u32 rd = 0;
        u32 exr = 0; // Execute Result
        u32 ad = 0;  // Address Destination
        int pcPredict = 0;

        void Clear()
        {
            pc = rd = exr = ad = pcPredict = 0;
            InsType = INSTRUCTION::NOP;
        }
    };

    struct MEM_Buffer
    {
        u32 pc = 0;
        INSTRUCTION::INS_TYPE InsType = INSTRUCTION::NOP;
        u32 rd = 0;
        u32 exr = 0;

        void Clear()
        {
            pc = rd = exr = 0;
            InsType = INSTRUCTION::NOP;
        }
    };

    struct WB_Buffer
    {
        u32 pc = 0;
        INSTRUCTION::INS_TYPE InsType = INSTRUCTION::NOP;
        u32 rd = 0;
        u32 exr = 0;

        void Clear()
        {
            pc = rd = exr = 0;
            InsType = INSTRUCTION::NOP;
        }
    };
} // namespace BUFFER

#endif // RISC_V_SIMULATOR_BUFFER