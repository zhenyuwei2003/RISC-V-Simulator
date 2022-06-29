#ifndef RISC_V_SIMULATOR_STAGE
#define RISC_V_SIMULATOR_STAGE

#include "Buffer.hpp"
#include "Memory.hpp"
#include "Register.hpp"

namespace STAGE
{
    class stageIF
    {
    public:
        BUFFER::IF_Buffer Buffer;
        MEMORY::Memory &Mem;
        u32 &pc;
        u32 &pcNew;
        u32 &Stall;
        u32 &StopFlag;
        bool NOPFlag;

        explicit stageIF(MEMORY::Memory &Mem_param, u32 &pc_param, u32 &pcNew_param, u32 &Stall_param, u32 &StopFlag_param)
            : Mem(Mem_param), pc(pc_param), pcNew(pcNew_param), Stall(Stall_param), StopFlag(StopFlag_param), NOPFlag(false) {}

        void execute()
        {
            if(StopFlag >= 2) { NOPFlag = true; MEMORY::DEBUGprintf("\n[IF] Stop"); return; }
            if(Stall) { --Stall; NOPFlag = true; MEMORY::DEBUGprintf("\n[IF] Stall"); return; }

            u32 Ins = Mem.Load(pc, 4);
            if(Ins == 0x0FF00513) StopFlag = 1;

            Buffer.pc = pc;
            INSTRUCTION::InsDecode(Ins, Buffer.InsType, Buffer.rd, Buffer.imm, Buffer.rs1, Buffer.rs2, Buffer.RegNum);

            if(Buffer.InsType == INSTRUCTION::NOP) { NOPFlag = true; MEMORY::DEBUGprintf("\n[IF] NOP"); return; }

            NOPFlag = false;
            if(!INSTRUCTION::IsJump(Buffer.InsType) && !INSTRUCTION::IsBranch(Buffer.InsType))
                pcNew = pc + 4;
            else
                pcNew = pc + 4; // predictPC TODO
        }
    };

    class stageID
    {
    public:
        BUFFER::IF_Buffer &preBuffer;
        BUFFER::ID_Buffer Buffer;
        REGISTER::Register &Reg;
        u32 &Stall;
        u32 &StopFlag;
        bool NOPFlag;

        explicit stageID(REGISTER::Register &Reg_param, BUFFER::IF_Buffer &preBuffer_param, u32 &Stall_param, u32 &StopFlag_param)
            : preBuffer(preBuffer_param), Reg(Reg_param), Stall(Stall_param), StopFlag(StopFlag_param), NOPFlag(false) {}

        void execute()
        {
            if(StopFlag >= 3) { NOPFlag = true; MEMORY::DEBUGprintf("\n[ID] Stop"); return; }
            if(preBuffer.InsType == INSTRUCTION::NOP) { NOPFlag = true; MEMORY::DEBUGprintf("\n[ID] NOP"); return; }
            if(Stall) { --Stall; NOPFlag = true; MEMORY::DEBUGprintf("\n[ID] Stall"); return; }

            NOPFlag = false;
            Buffer.pc = preBuffer.pc;
            Buffer.InsType = preBuffer.InsType;
            Buffer.rd = preBuffer.rd;
            Buffer.imm = preBuffer.imm;
            Buffer.rs1 = preBuffer.rs1;
            Buffer.rs2 = preBuffer.rs2;

            switch(preBuffer.RegNum)
            {
                case 0:
                    Buffer.rv1 = 0;
                    Buffer.rv2 = 0;
                    break;
                case 1:
                    Buffer.rv1 = Reg.Load(preBuffer.rs1);
                    Buffer.rv2 = 0;
                    break;
                case 2:
                    Buffer.rv1 = Reg.Load(preBuffer.rs1);
                    Buffer.rv2 = Reg.Load(preBuffer.rs2);
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
        BUFFER::ID_Buffer &preBuffer;
        BUFFER::EX_Buffer Buffer;
        u32 &Stall;
        u32 &StopFlag;
        bool NOPFlag;

        explicit stageEX(BUFFER::ID_Buffer &preBuffer_param, u32 &Stall_param, u32 &StopFlag_param)
            : preBuffer(preBuffer_param), Stall(Stall_param), StopFlag(StopFlag_param), NOPFlag(false) {};

        void execute()
        {
            if(StopFlag >= 4) { NOPFlag = true; MEMORY::DEBUGprintf("\n[EX] Stop"); return; }
            if(preBuffer.InsType == INSTRUCTION::NOP) { NOPFlag = true; MEMORY::DEBUGprintf("\n[EX] NOP"); return; }
            if(Stall) { --Stall; NOPFlag = true; MEMORY::DEBUGprintf("\n[EX] Stall"); return; }

            NOPFlag = false;
            Buffer.pc = preBuffer.pc;
            Buffer.InsType = preBuffer.InsType;
            Buffer.rd = preBuffer.rd;

            u32 rv1 = preBuffer.rv1, rv2 = preBuffer.rv2, imm = preBuffer.imm, pc = Buffer.pc;
            switch(Buffer.InsType)
            {
                case INSTRUCTION::ADD:
                    Buffer.exr = rv1 + rv2;
                    Buffer.ad = 0;
                    break;
                case INSTRUCTION::ADDI:
                    Buffer.exr = rv1 + imm;
                    Buffer.ad = 0;
                    break;
                case INSTRUCTION::AND:
                    Buffer.exr = rv1 & rv2;
                    Buffer.ad = 0;
                    break;
                case INSTRUCTION::ANDI:
                    Buffer.exr = rv1 & imm;
                    Buffer.ad = 0;
                    break;
                case INSTRUCTION::AUIPC:
                    Buffer.exr = pc + imm;
                    Buffer.ad = 0;
                    break;
                case INSTRUCTION::BEQ:
                    if(rv1 == rv2) Buffer.pc += imm;
                    else Buffer.pc += 4;
                    Buffer.exr = 0;
                    Buffer.ad = 0;
                    break;
                case INSTRUCTION::BGE:
                    if((int)rv1 >= (int)rv2) Buffer.pc += imm;
                    else Buffer.pc += 4;
                    Buffer.exr = 0;
                    Buffer.ad = 0;
                    break;
                case INSTRUCTION::BGEU:
                    if(rv1 >= rv2) Buffer.pc += imm;
                    else Buffer.pc += 4;
                    Buffer.exr = 0;
                    Buffer.ad = 0;
                    break;
                case INSTRUCTION::BLT:
                    if((int)rv1 < (int)rv2) Buffer.pc += imm;
                    else Buffer.pc += 4;
                    Buffer.exr = 0;
                    Buffer.ad = 0;
                    break;
                case INSTRUCTION::BLTU:
                    if(rv1 < rv2) Buffer.pc += imm;
                    else Buffer.pc += 4;
                    Buffer.exr = 0;
                    Buffer.ad = 0;
                    break;
                case INSTRUCTION::BNE:
                    if(rv1 != rv2) Buffer.pc += imm;
                    else Buffer.pc += 4;
                    Buffer.exr = 0;
                    Buffer.ad = 0;
                    break;
                case INSTRUCTION::JAL:
                    Buffer.exr = pc + 4;
                    Buffer.ad = 0;
                    Buffer.pc += imm;
                    break;
                case INSTRUCTION::JALR:
                    Buffer.exr = pc + 4;
                    Buffer.ad = 0;
                    Buffer.pc = (rv1 + imm) & ~1;
                    break;
                case INSTRUCTION::LB:
                case INSTRUCTION::LBU:
                case INSTRUCTION::LH:
                case INSTRUCTION::LHU:
                case INSTRUCTION::LW:
                    Buffer.exr = 0;
                    Buffer.ad = rv1 + imm;
                    break;
                case INSTRUCTION::LUI:
                    Buffer.exr = imm;
                    Buffer.ad = 0;
                    break;
                case INSTRUCTION::OR:
                    Buffer.exr = rv1 | rv2;
                    Buffer.ad = 0;
                    break;
                case INSTRUCTION::ORI:
                    Buffer.exr = rv1 | imm;
                    Buffer.ad = 0;
                    break;
                case INSTRUCTION::SB:
                    Buffer.exr = rv2 & 0xFFu;
                    Buffer.ad = rv1 + imm;
                    break;
                case INSTRUCTION::SH:
                    Buffer.exr = rv2 & 0xFFFFu;
                    Buffer.ad = rv1 + imm;
                    break;
                case INSTRUCTION::SLL:
                    Buffer.exr = rv1 << (rv2 & 0b11111u);
                    Buffer.ad = 0;
                    break;
                case INSTRUCTION::SLLI:
                    Buffer.exr = rv1 << imm;
                    Buffer.ad = 0;
                    break;
                case INSTRUCTION::SLT:
                    Buffer.exr = ((int)rv1 < (int)rv2);
                    Buffer.ad = 0;
                    break;
                case INSTRUCTION::SLTI:
                    Buffer.exr = ((int)rv1 < (int)imm);
                    Buffer.ad = 0;
                    break;
                case INSTRUCTION::SLTU:
                    Buffer.exr = (rv1 < rv2);
                    Buffer.ad = 0;
                    break;
                case INSTRUCTION::SLTIU:
                    Buffer.exr = (rv1 < imm);
                    Buffer.ad = 0;
                    break;
                case INSTRUCTION::SRA:
                    Buffer.exr = rv1 >> (rv2 & 0b11111u);
                    INSTRUCTION::SignExtend(Buffer.exr, 31u - (rv2 & 0b11111u));
                    Buffer.ad = 0;
                    break;
                case INSTRUCTION::SRAI:
                    Buffer.exr = rv1 >> (imm & 0b11111u);
                    INSTRUCTION::SignExtend(Buffer.exr, 31u - (imm & 0b11111u));
                    Buffer.ad = 0;
                    break;
                case INSTRUCTION::SRL:
                    Buffer.exr = rv1 >> (rv2 & 0b11111u);
                    Buffer.ad = 0;
                    break;
                case INSTRUCTION::SRLI:
                    Buffer.exr = rv1 >> imm;
                    Buffer.ad = 0;
                    break;
                case INSTRUCTION::SUB:
                    Buffer.exr = rv1 - rv2;
                    Buffer.ad = 0;
                    break;
                case INSTRUCTION::SW:
                    Buffer.exr = rv2 & 0xFFFFFFFFu;
                    Buffer.ad = rv1 + imm;
                    break;
                case INSTRUCTION::XOR:
                    Buffer.exr = rv1 ^ rv2;
                    Buffer.ad = 0;
                    break;
                case INSTRUCTION::XORI:
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
        BUFFER::EX_Buffer &preBuffer;
        BUFFER::MEM_Buffer Buffer;
        MEMORY::Memory &Mem;
        u32 &pcNew;
        u32 &Stall;
        u32 &StopFlag;
        bool &IF_ID_EX_ClearFlag;
        bool NOPFlag;

        explicit stageMEM(MEMORY::Memory &Mem_param, BUFFER::EX_Buffer &preBuffer_param, u32 &pcNew_param, u32 &Stall_param, u32 &StopFlag_param, bool &IF_ID_EX_ClearFlag_param)
            : preBuffer(preBuffer_param), Mem(Mem_param), pcNew(pcNew_param), Stall(Stall_param), StopFlag(StopFlag_param), IF_ID_EX_ClearFlag(IF_ID_EX_ClearFlag_param), NOPFlag(false) {}

        void execute()
        {
            if(StopFlag >= 5) { NOPFlag = true; MEMORY::DEBUGprintf("\n[MEM] Stop"); return; }
            if(preBuffer.InsType == INSTRUCTION::NOP) { NOPFlag = true; MEMORY::DEBUGprintf("\n[MEM] NOP"); return; }
            if(Stall) { --Stall, NOPFlag = true; MEMORY::DEBUGprintf("\n[MEM] Stall"); return; }

            NOPFlag = false;
            Buffer.pc = preBuffer.pc;
            Buffer.InsType = preBuffer.InsType;
            Buffer.rd = preBuffer.rd;
            Buffer.exr = preBuffer.exr;

            switch(Buffer.InsType)
            {
                case INSTRUCTION::LB:
                    Buffer.exr = Mem.Load(preBuffer.ad, 1);
                    INSTRUCTION::SignExtend(Buffer.exr, 7);
                    break;
                case INSTRUCTION::LBU:
                    Buffer.exr = Mem.Load(preBuffer.ad, 1);
                    break;
                case INSTRUCTION::LH:
                    Buffer.exr = Mem.Load(preBuffer.ad, 2);
                    INSTRUCTION::SignExtend(Buffer.exr, 15);
                    break;
                case INSTRUCTION::LHU:
                    Buffer.exr = Mem.Load(preBuffer.ad, 2);
                    break;
                case INSTRUCTION::LW:
                    Buffer.exr = Mem.Load(preBuffer.ad, 4);
                    INSTRUCTION::SignExtend(Buffer.exr, 31);
                    break;
                case INSTRUCTION::SB:
                    Mem.Store(preBuffer.ad, 1, Buffer.exr);
                    break;
                case INSTRUCTION::SH:
                    Mem.Store(preBuffer.ad, 2, Buffer.exr);
                    break;
                case INSTRUCTION::SW:
                    Mem.Store(preBuffer.ad, 4, Buffer.exr);
                    break;
                case INSTRUCTION::BEQ:
                case INSTRUCTION::BGE:
                case INSTRUCTION::BGEU:
                case INSTRUCTION::BLT:
                case INSTRUCTION::BLTU:
                case INSTRUCTION::BNE:
                case INSTRUCTION::JAL:
                case INSTRUCTION::JALR:
                    if(pcNew - 12 != Buffer.pc)
                    {
                        pcNew = Buffer.pc;
                        IF_ID_EX_ClearFlag = true;
                        StopFlag = 0;
                    }
                    break;
            }
        }
    };

    class stageWB
    {
    public:
        BUFFER::MEM_Buffer &preBuffer;
        BUFFER::WB_Buffer Buffer;
        REGISTER::Register &Reg;
        u32 &Stall;
        u32 &StopFlag;
        bool NOPFlag;

        explicit stageWB(REGISTER::Register &Reg_param, BUFFER::MEM_Buffer &preBuffer_param, u32 &Stall_param, u32 &StopFlag_param)
            : preBuffer(preBuffer_param), Reg(Reg_param), Stall(Stall_param), StopFlag(StopFlag_param), NOPFlag(false) {}

        void execute()
        {
            if(StopFlag) ++StopFlag;
            if(preBuffer.InsType == INSTRUCTION::NOP) { NOPFlag = true; MEMORY::DEBUGprintf("\n[WB] NOP"); return; }
            if(Stall) { --Stall; NOPFlag = true; MEMORY::DEBUGprintf("\n[WB] Stall"); return; }

            NOPFlag = false;
            Buffer.pc = preBuffer.pc;
            Buffer.InsType = preBuffer.InsType;
            Buffer.rd = preBuffer.rd;
            Buffer.exr = preBuffer.exr;

            switch(preBuffer.InsType)
            {
                case INSTRUCTION::ADD:
                case INSTRUCTION::ADDI:
                case INSTRUCTION::AND:
                case INSTRUCTION::ANDI:
                case INSTRUCTION::AUIPC:
                case INSTRUCTION::LB:
                case INSTRUCTION::LBU:
                case INSTRUCTION::LH:
                case INSTRUCTION::LHU:
                case INSTRUCTION::LW:
                case INSTRUCTION::LUI:
                case INSTRUCTION::OR:
                case INSTRUCTION::ORI:
                case INSTRUCTION::SLL:
                case INSTRUCTION::SLLI:
                case INSTRUCTION::SLT:
                case INSTRUCTION::SLTI:
                case INSTRUCTION::SLTU:
                case INSTRUCTION::SLTIU:
                case INSTRUCTION::SRA:
                case INSTRUCTION::SRAI:
                case INSTRUCTION::SRL:
                case INSTRUCTION::SRLI:
                case INSTRUCTION::SUB:
                case INSTRUCTION::XOR:
                case INSTRUCTION::XORI:
                case INSTRUCTION::JAL:
                case INSTRUCTION::JALR:
                    Reg.Store(preBuffer.rd, preBuffer.exr);
                    break;
            }
        }
    };
} // namespace STAGE

#endif // RISC_V_SIMULATOR_STAGE