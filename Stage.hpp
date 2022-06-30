#ifndef RISC_V_SIMULATOR_STAGE
#define RISC_V_SIMULATOR_STAGE

#include "Memory.hpp"
#include "Register.hpp"
#include "Buffer.hpp"
#include "Predictor.hpp"

using namespace MEMORY;
using namespace REGISTER;
using namespace BUFFER;
using namespace PREDICTOR;
using namespace INSTRUCTION;

namespace STAGE
{
    class stageIF
    {
    public:
        IF_Buffer Buffer;
        Memory &Mem;
        Predictor &Pred;

        u32 &pc, &pcNext;
        u32 &Stall, &StopFlag;
        bool NOPFlag;

        explicit stageIF(Memory &Mem_, Predictor &Pred_, u32 &pc_, u32 &pcNext_, u32 &Stall_, u32 &StopFlag_)
            : Mem(Mem_), Pred(Pred_), pc(pc_), pcNext(pcNext_), Stall(Stall_), StopFlag(StopFlag_), NOPFlag(false) {}

        void execute()
        {
            NOPFlag = true;
            if (StopFlag >= 2) return;
            if (Stall) { --Stall; return; }
            NOPFlag = false;

            Buffer.pc = pc;
            Buffer.Ins = Mem.Load(pc, 4);
            if (Buffer.Ins == 0x0FF00513) StopFlag = 1;

            Pred.NextPredict(Buffer.pc, Buffer.Ins, pcNext, Buffer.pcPredict);
        }
    };

    class stageID
    {
    public:
        IF_Buffer &preBuffer;
        ID_Buffer Buffer;
        Register &Reg;

        u32 &Stall, &StopFlag;
        bool NOPFlag;

        explicit stageID(IF_Buffer &preBuffer_, Register &Reg_, u32 &Stall_, u32 &StopFlag_)
            : preBuffer(preBuffer_), Reg(Reg_), Stall(Stall_), StopFlag(StopFlag_), NOPFlag(false) {}

        void execute()
        {
            NOPFlag = true;
            if (StopFlag >= 3) return;
            if (Stall) { --Stall; return; }
            NOPFlag = false;

            u32 RegNum = 0;
            Buffer.pc = preBuffer.pc;
            Buffer.pcPredict = preBuffer.pcPredict;
            InsDecode(preBuffer.Ins, Buffer.InsType, Buffer.rd, Buffer.imm, Buffer.rs1, Buffer.rs2, RegNum);

            switch (RegNum)
            {
                case 0:
                    Buffer.rv1 = 0;
                    Buffer.rv2 = 0;
                    break;
                case 1:
                    Buffer.rv1 = Reg.Load(Buffer.rs1);
                    Buffer.rv2 = 0;
                    break;
                case 2:
                    Buffer.rv1 = Reg.Load(Buffer.rs1);
                    Buffer.rv2 = Reg.Load(Buffer.rs2);
                    break;
                default:
                    printf("RegNum ERROR!\n");
                    break;
            }
        }
    };

    class stageEX
    {
    public:
        ID_Buffer &preBuffer;
        EX_Buffer Buffer;

        u32 &Stall, &StopFlag;
        bool NOPFlag;

        explicit stageEX(ID_Buffer &preBuffer_, u32 &Stall_, u32 &StopFlag_)
            : preBuffer(preBuffer_), Stall(Stall_), StopFlag(StopFlag_), NOPFlag(false) {};

        void execute()
        {
            NOPFlag = true;
            if (StopFlag >= 4) return;
            if (preBuffer.InsType == INSTRUCTION::NOP) return;
            if (Stall) { --Stall; return; }
            NOPFlag = false;

            Buffer.pc = preBuffer.pc;
            Buffer.pcNext = Buffer.pc;
            Buffer.pcPredict = preBuffer.pcPredict;
            Buffer.InsType = preBuffer.InsType;
            Buffer.rd = preBuffer.rd;

            u32 pc = Buffer.pc, rv1 = preBuffer.rv1, rv2 = preBuffer.rv2, imm = preBuffer.imm;
            switch(Buffer.InsType)
            {
                case ADD:
                    Buffer.exr = rv1 + rv2;
                    Buffer.ad = 0;
                    break;
                case ADDI:
                    Buffer.exr = rv1 + imm;
                    Buffer.ad = 0;
                    break;
                case AND:
                    Buffer.exr = rv1 & rv2;
                    Buffer.ad = 0;
                    break;
                case ANDI:
                    Buffer.exr = rv1 & imm;
                    Buffer.ad = 0;
                    break;
                case AUIPC:
                    Buffer.exr = pc + imm;
                    Buffer.ad = 0;
                    break;
                case BEQ:
                    if (rv1 == rv2) Buffer.pcNext += imm;
                    else Buffer.pcNext += 4;
                    Buffer.exr = 0;
                    Buffer.ad = 0;
                    break;
                case BGE:
                    if ((int)rv1 >= (int)rv2) Buffer.pcNext += imm;
                    else Buffer.pcNext += 4;
                    Buffer.exr = 0;
                    Buffer.ad = 0;
                    break;
                case BGEU:
                    if(rv1 >= rv2) Buffer.pcNext += imm;
                    else Buffer.pcNext += 4;
                    Buffer.exr = 0;
                    Buffer.ad = 0;
                    break;
                case BLT:
                    if ((int)rv1 < (int)rv2) Buffer.pcNext += imm;
                    else Buffer.pcNext += 4;
                    Buffer.exr = 0;
                    Buffer.ad = 0;
                    break;
                case BLTU:
                    if (rv1 < rv2) Buffer.pcNext += imm;
                    else Buffer.pcNext += 4;
                    Buffer.exr = 0;
                    Buffer.ad = 0;
                    break;
                case BNE:
                    if (rv1 != rv2) Buffer.pcNext += imm;
                    else Buffer.pcNext += 4;
                    Buffer.exr = 0;
                    Buffer.ad = 0;
                    break;
                case JAL:
                    Buffer.exr = pc + 4;
                    Buffer.ad = 0;
                    Buffer.pcNext += imm;
                    break;
                case JALR:
                    Buffer.exr = pc + 4;
                    Buffer.ad = 0;
                    Buffer.pcNext = (rv1 + imm) & ~1;
                    break;
                case LB:
                case LBU:
                case LH:
                case LHU:
                case LW:
                    Buffer.exr = 0;
                    Buffer.ad = rv1 + imm;
                    break;
                case LUI:
                    Buffer.exr = imm;
                    Buffer.ad = 0;
                    break;
                case OR:
                    Buffer.exr = rv1 | rv2;
                    Buffer.ad = 0;
                    break;
                case ORI:
                    Buffer.exr = rv1 | imm;
                    Buffer.ad = 0;
                    break;
                case SB:
                    Buffer.exr = rv2 & 0xFFu;
                    Buffer.ad = rv1 + imm;
                    break;
                case SH:
                    Buffer.exr = rv2 & 0xFFFFu;
                    Buffer.ad = rv1 + imm;
                    break;
                case SLL:
                    Buffer.exr = rv1 << (rv2 & 0b11111u);
                    Buffer.ad = 0;
                    break;
                case SLLI:
                    Buffer.exr = rv1 << imm;
                    Buffer.ad = 0;
                    break;
                case SLT:
                    Buffer.exr = ((int)rv1 < (int)rv2);
                    Buffer.ad = 0;
                    break;
                case SLTI:
                    Buffer.exr = ((int)rv1 < (int)imm);
                    Buffer.ad = 0;
                    break;
                case SLTU:
                    Buffer.exr = (rv1 < rv2);
                    Buffer.ad = 0;
                    break;
                case SLTIU:
                    Buffer.exr = (rv1 < imm);
                    Buffer.ad = 0;
                    break;
                case SRA:
                    Buffer.exr = rv1 >> (rv2 & 0b11111u);
                    SignExtend(Buffer.exr, 31u - (rv2 & 0b11111u));
                    Buffer.ad = 0;
                    break;
                case SRAI:
                    Buffer.exr = rv1 >> (imm & 0b11111u);
                    SignExtend(Buffer.exr, 31u - (imm & 0b11111u));
                    Buffer.ad = 0;
                    break;
                case SRL:
                    Buffer.exr = rv1 >> (rv2 & 0b11111u);
                    Buffer.ad = 0;
                    break;
                case SRLI:
                    Buffer.exr = rv1 >> imm;
                    Buffer.ad = 0;
                    break;
                case SUB:
                    Buffer.exr = rv1 - rv2;
                    Buffer.ad = 0;
                    break;
                case SW:
                    Buffer.exr = rv2 & 0xFFFFFFFFu;
                    Buffer.ad = rv1 + imm;
                    break;
                case XOR:
                    Buffer.exr = rv1 ^ rv2;
                    Buffer.ad = 0;
                    break;
                case XORI:
                    Buffer.exr = rv1 ^ imm;
                    Buffer.ad = 0;
                    break;
                default:
                    printf("[EX] InsType ERROR!\n");
            }
        }
    };

    class stageMEM
    {
    public:
        EX_Buffer &preBuffer;
        MEM_Buffer Buffer;
        Memory &Mem;
        Predictor &Pred;

        u32 &pcNext, &Stall, &StopFlag;
        bool &IF_ID_EX_ClearFlag, NOPFlag;

        explicit stageMEM(EX_Buffer &preBuffer_, Memory &Mem_, Predictor &Pred_, u32 &pcNext_, u32 &Stall_, u32 &StopFlag_, bool &IF_ID_EX_ClearFlag_)
            : preBuffer(preBuffer_), Mem(Mem_), Pred(Pred_), pcNext(pcNext_), Stall(Stall_), StopFlag(StopFlag_), IF_ID_EX_ClearFlag(IF_ID_EX_ClearFlag_), NOPFlag(false) {}

        void execute()
        {
            NOPFlag = true;
            if(StopFlag >= 5) return;
            if(preBuffer.InsType == NOP) return;
            if(Stall) { --Stall; return; }
            NOPFlag = false;

            Buffer.pc = preBuffer.pc;
            Buffer.InsType = preBuffer.InsType;
            Buffer.rd = preBuffer.rd;
            Buffer.exr = preBuffer.exr;

            switch(Buffer.InsType)
            {
                case LB:
                    Buffer.exr = Mem.Load(preBuffer.ad, 1);
                    SignExtend(Buffer.exr, 7);
                    break;
                case LBU:
                    Buffer.exr = Mem.Load(preBuffer.ad, 1);
                    break;
                case LH:
                    Buffer.exr = Mem.Load(preBuffer.ad, 2);
                    SignExtend(Buffer.exr, 15);
                    break;
                case LHU:
                    Buffer.exr = Mem.Load(preBuffer.ad, 2);
                    break;
                case LW:
                    Buffer.exr = Mem.Load(preBuffer.ad, 4);
                    SignExtend(Buffer.exr, 31);
                    break;
                case SB:
                    Mem.Store(preBuffer.ad, 1, Buffer.exr);
                    break;
                case SH:
                    Mem.Store(preBuffer.ad, 2, Buffer.exr);
                    break;
                case SW:
                    Mem.Store(preBuffer.ad, 4, Buffer.exr);
                    break;
                case BEQ:
                case BGE:
                case BGEU:
                case BLT:
                case BLTU:
                case BNE:
                case JAL:
                case JALR:
                    Pred.Update(Buffer.pc, preBuffer.pcNext, preBuffer.pcPredict);
                    if(preBuffer.pcPredict != preBuffer.pcNext)
                    {
                        pcNext = preBuffer.pcNext;
                        IF_ID_EX_ClearFlag = true;
                        StopFlag = 0;
                    }
                    break;
                default: break;
            }
        }
    };

    class stageWB
    {
    public:
        MEM_Buffer &preBuffer;
        WB_Buffer Buffer;
        Register &Reg;
        u32 &Stall, &StopFlag;
        bool NOPFlag;

        explicit stageWB(MEM_Buffer &preBuffer_, Register &Reg_, u32 &Stall_, u32 &StopFlag_)
            : preBuffer(preBuffer_), Reg(Reg_), Stall(Stall_), StopFlag(StopFlag_), NOPFlag(false) {}

        void execute()
        {
            NOPFlag = true;
            if(preBuffer.InsType == NOP) return;
            if(Stall) { --Stall; return; }
            NOPFlag = false;

            Buffer.pc = preBuffer.pc;
            Buffer.InsType = preBuffer.InsType;
            Buffer.rd = preBuffer.rd;
            Buffer.exr = preBuffer.exr;

            switch(preBuffer.InsType)
            {
                case ADD:
                case ADDI:
                case AND:
                case ANDI:
                case AUIPC:
                case LB:
                case LBU:
                case LH:
                case LHU:
                case LW:
                case LUI:
                case OR:
                case ORI:
                case SLL:
                case SLLI:
                case SLT:
                case SLTI:
                case SLTU:
                case SLTIU:
                case SRA:
                case SRAI:
                case SRL:
                case SRLI:
                case SUB:
                case XOR:
                case XORI:
                case JAL:
                case JALR:
                    Reg.Store(preBuffer.rd, preBuffer.exr);
                    break;
                default: break;
            }
        }
    };
} // namespace STAGE

#endif // RISC_V_SIMULATOR_STAGE