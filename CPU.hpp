#ifndef RISC_V_SIMULATOR_CPU
#define RISC_V_SIMULATOR_CPU

#include "Stage.hpp"

#ifdef RISC_V_DEBUG

string InsTable[100] =
{
    "NOP", "ADD", "ADDI", "AND", "ANDI", "AUIPC", "BEQ", "BGE",
    "BGEU", "BLT", "BLTU", "BNE", "JAL", "JALR", "LB", "LBU",
    "LH", "LHU", "LUI", "LW", "OR", "ORI", "SB", "SH",
    "SLL", "SLLI", "SLT", "SLTI", "SLTIU", "SLTU", "SRA", "SRAI",
    "SRL", "SRLI", "SUB", "SW", "XOR", "XORI"
};

string RegTable[100] =
{
    "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
    "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
    "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
    "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

#endif

namespace CPU
{
    class CPU
    {
    private:
        MEMORY::Memory Mem;
        REGISTER::Register Reg;
        int pc, pcNext;
        u32 StopFlag;
        u32 Stall[6];
        bool IF_ID_EX_ClearFlag;
        bool StallFlag;

        STAGE::stageIF IF;
        STAGE::stageID ID;
        STAGE::stageEX EX;
        STAGE::stageMEM MEM;
        STAGE::stageWB WB;

        BUFFER::IF_Buffer IF_Buffer_Pre;
        BUFFER::ID_Buffer ID_Buffer_Pre;
        BUFFER::EX_Buffer EX_Buffer_Pre;
        BUFFER::MEM_Buffer MEM_Buffer_Pre;
        BUFFER::WB_Buffer WB_Buffer_Pre;

    public:
        explicit CPU(istream &InputStream)
            : Mem(InputStream), pc(0), pcNext(-1), StopFlag(false), IF_ID_EX_ClearFlag(false), StallFlag(false),
              IF(Mem, pc, pcNext, Stall[1], StopFlag),
              ID(Reg, IF_Buffer_Pre, Stall[2], StopFlag),
              EX(ID_Buffer_Pre, Stall[3], StopFlag),
              MEM(Mem, EX_Buffer_Pre, pcNext, Stall[4], StopFlag, IF_ID_EX_ClearFlag),
              WB(Reg, MEM_Buffer_Pre, Stall[5], StopFlag)
              { memset(Stall, 0, sizeof(Stall)); }

        u32 run()
        {
#ifdef RISC_V_DEBUG
            int DebugCnt = 0;
#endif
            while(true)
            {
#ifdef RISC_V_DEBUG
                printf("Loop: %d\n", ++DebugCnt);
                printf("NowPC: %x\n", pc);
                printf("StopFlag: %d", StopFlag);
#endif

                WB.execute();
                EX.execute();
                MEM.execute();
                IF.execute();
                ID.execute();
                if(StopFlag) ++StopFlag;

#ifdef RISC_V_DEBUG
                printf("\n--------------------\n");
                printf("\n[Register]");
                bool DEBUG_FLAG = false;
                for(int i = 0; i < 32; ++i) if(Reg.Load(i)) DEBUG_FLAG = true, printf("\n%s: %d", RegTable[i].c_str(), Reg.Load(i));
                if(!DEBUG_FLAG) printf("\nAll Register is 0!");
                printf("\n");
#endif

                if(StopFlag >= 5) break;

                if(pcNext != -1) pc = pcNext, pcNext = -1;
                if(StallFlag)
                {
                    StallFlag = false;
                    EX_Buffer_Pre.Clear();
                    MEM_Buffer_Pre = MEM.Buffer;
                    WB_Buffer_Pre = WB.Buffer;
                }
                else
                {
                    if(IF.NOPFlag || IF_ID_EX_ClearFlag) IF_Buffer_Pre.Clear();
                    else IF_Buffer_Pre = IF.Buffer;
                    if(ID.NOPFlag || IF_ID_EX_ClearFlag) ID_Buffer_Pre.Clear();
                    else ID_Buffer_Pre = ID.Buffer;
                    if(EX.NOPFlag || IF_ID_EX_ClearFlag) EX_Buffer_Pre.Clear();
                    else EX_Buffer_Pre = EX.Buffer;
                    if(MEM.NOPFlag) MEM_Buffer_Pre.Clear();
                    else MEM_Buffer_Pre = MEM.Buffer;
                    if(WB.NOPFlag) WB_Buffer_Pre.Clear();
                    else WB_Buffer_Pre = WB.Buffer;

                    IF_ID_EX_ClearFlag = false;
                }

                if(INSTRUCTION::IsLoad(EX_Buffer_Pre.InsType) && (ID_Buffer_Pre.rs1 == EX_Buffer_Pre.rd || ID_Buffer_Pre.rs2 == EX_Buffer_Pre.rd))
                    ++Stall[1], ++Stall[2], ++Stall[3], StallFlag = true;

                // forwarding (B5->B2)
                if(INSTRUCTION::IsRegEdit(WB_Buffer_Pre.InsType) && ID_Buffer_Pre.rs1 && ID_Buffer_Pre.rs1 == WB_Buffer_Pre.rd)
                    ID_Buffer_Pre.rv1 = WB_Buffer_Pre.exr;
                if(INSTRUCTION::IsRegEdit(WB_Buffer_Pre.InsType) && ID_Buffer_Pre.rs2 && ID_Buffer_Pre.rs2 == WB_Buffer_Pre.rd)
                    ID_Buffer_Pre.rv2 = WB_Buffer_Pre.exr;

                // forwarding (B4->B2)
                if(INSTRUCTION::IsRegEdit(MEM_Buffer_Pre.InsType) && ID_Buffer_Pre.rs1 && ID_Buffer_Pre.rs1 == MEM_Buffer_Pre.rd)
                    ID_Buffer_Pre.rv1 = MEM_Buffer_Pre.exr;
                if(INSTRUCTION::IsRegEdit(MEM_Buffer_Pre.InsType) && ID_Buffer_Pre.rs2 && ID_Buffer_Pre.rs2 == MEM_Buffer_Pre.rd)
                    ID_Buffer_Pre.rv2 = MEM_Buffer_Pre.exr;

                // forwarding (B3->B2)
                if(INSTRUCTION::IsRegEdit(EX_Buffer_Pre.InsType) && ID_Buffer_Pre.rs1 && ID_Buffer_Pre.rs1 == EX_Buffer_Pre.rd)
                    ID_Buffer_Pre.rv1 = EX_Buffer_Pre.exr;
                if(INSTRUCTION::IsRegEdit(EX_Buffer_Pre.InsType) && ID_Buffer_Pre.rs2 && ID_Buffer_Pre.rs2 == EX_Buffer_Pre.rd)
                    ID_Buffer_Pre.rv2 = EX_Buffer_Pre.exr;

#ifdef RISC_V_DEBUG
                printf("\n[IF_Buffer]   %s\n", InsTable[IF_Buffer_Pre.InsType].c_str());
                if(IF_Buffer_Pre.InsType) printf("pc: %x\nrd: %s\nimm: %d\nrs1: %s\nrs2: %s\nRegNum: %d\n", IF_Buffer_Pre.pc, RegTable[IF_Buffer_Pre.rd].c_str(), IF_Buffer_Pre.imm, RegTable[IF_Buffer_Pre.rs1].c_str(), RegTable[IF_Buffer_Pre.rs2].c_str(), IF_Buffer_Pre.RegNum);
                printf("\n[ID_Buffer]   %s\n", InsTable[ID_Buffer_Pre.InsType].c_str());
                if(ID_Buffer_Pre.InsType) printf("pc: %x\nrd: %s\nimm: %d\nrs1: %s\nrs2: %s\nrv1: %d\nrv2: %d\n", ID_Buffer_Pre.pc, RegTable[ID_Buffer_Pre.rd].c_str(), ID_Buffer_Pre.imm, RegTable[ID_Buffer_Pre.rs1].c_str(), RegTable[ID_Buffer_Pre.rs2].c_str(), ID_Buffer_Pre.rv1, ID_Buffer_Pre.rv2);
                printf("\n[EX_Buffer]   %s\n", InsTable[EX_Buffer_Pre.InsType].c_str());
                if(EX_Buffer_Pre.InsType) printf("pc: %x\nrd: %s\nexr: %d\nad: %d\n", EX_Buffer_Pre.pc, RegTable[EX_Buffer_Pre.rd].c_str(), EX_Buffer_Pre.exr, EX_Buffer_Pre.ad);
                printf("\n[MEM_Buffer]  %s\n", InsTable[MEM_Buffer_Pre.InsType].c_str());
                if(MEM_Buffer_Pre.InsType) printf("pc: %x\nrd: %s\nexr: %d\n", MEM_Buffer_Pre.pc, RegTable[MEM_Buffer_Pre.rd].c_str(), MEM_Buffer_Pre.exr);
                printf("\n[WB_Buffer]   %s\n", InsTable[WB_Buffer_Pre.InsType].c_str());
                if(WB_Buffer_Pre.InsType) printf("pc: %x\nrd: %s\nexr: %d\n", WB_Buffer_Pre.pc, RegTable[WB_Buffer_Pre.rd].c_str(), WB_Buffer_Pre.exr);
                printf("\n--------------------\n");
#endif
            }

#ifdef RISC_V_DEBUG
            u32 ans = Reg.Load(10) & 0xFFu;
            printf("\n\nANS: %d\n", ans);
#endif
            return Reg.Load(10) & 0xFFu;
        }
    };
} // namespace CPU

#endif // RISC_V_SIMULATOR_CPU