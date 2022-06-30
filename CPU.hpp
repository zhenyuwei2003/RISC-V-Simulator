#ifndef RISC_V_SIMULATOR_CPU
#define RISC_V_SIMULATOR_CPU

#include "Stage.hpp"

using namespace MEMORY;
using namespace REGISTER;
using namespace STAGE;
using namespace BUFFER;
using namespace PREDICTOR;

//#define RISC_V_DEBUG
//#define RISC_V_PRINT

namespace CPU
{
    class CPU
    {
    private:
        Memory Mem;
        Register Reg;
        Predictor Pred;

        u32 pc, pcNext;
        u32 StopCnt, BubbleCnt, Stall[6];
        bool IF_ID_EX_DiscardFlag, HazardStallFlag;

        stageIF IF;
        stageID ID;
        stageEX EX;
        stageMEM MEM;
        stageWB WB;

        IF_Buffer IF_Buffer_Pre;
        ID_Buffer ID_Buffer_Pre;
        EX_Buffer EX_Buffer_Pre;
        MEM_Buffer MEM_Buffer_Pre;
        WB_Buffer WB_Buffer_Pre;

    public:
        explicit CPU(istream &InputStream)
            : Mem(InputStream), pc(0), pcNext(0), StopCnt(0), BubbleCnt(0), IF_ID_EX_DiscardFlag(false), HazardStallFlag(false),
              IF(Mem, Pred, pc, pcNext, Stall[1], StopCnt),
              ID(IF_Buffer_Pre, Reg, Stall[2], StopCnt),
              EX(ID_Buffer_Pre, Stall[3], StopCnt),
              MEM(EX_Buffer_Pre, Mem, Pred, pcNext, Stall[4], StopCnt, IF_ID_EX_DiscardFlag),
              WB(MEM_Buffer_Pre, Reg, Stall[5])
              { memset(Stall, 0, sizeof(Stall)); }

        int run()
        {
            u32 CLK = 0;
            while (true)
            {
                ++CLK;
                if (BubbleCnt)
                { --BubbleCnt; continue; }

                IF.execute();
                ID.execute();
                EX.execute();
                MEM.execute();
                WB.execute();

                if (StopCnt) ++StopCnt;
                if (StopCnt == 5) break;

                // update pc
                if (pcNext) pc = pcNext, pcNext = 0;

                // update Buffer
                if (HazardStallFlag)
                {
                    HazardStallFlag = false;
                    EX_Buffer_Pre.Clear();
                    MEM_Buffer_Pre = MEM.Buffer;
                    WB_Buffer_Pre = WB.Buffer;
                }
                else
                {
                    if (IF.NOPFlag || IF_ID_EX_DiscardFlag)
                        IF_Buffer_Pre.Clear();
                    else
                        IF_Buffer_Pre = IF.Buffer;
                    if (ID.NOPFlag || IF_ID_EX_DiscardFlag)
                        ID_Buffer_Pre.Clear();
                    else
                        ID_Buffer_Pre = ID.Buffer;
                    if (EX.NOPFlag || IF_ID_EX_DiscardFlag)
                        EX_Buffer_Pre.Clear();
                    else
                        EX_Buffer_Pre = EX.Buffer;
                    if (MEM.NOPFlag)
                        MEM_Buffer_Pre.Clear();
                    else
                        MEM_Buffer_Pre = MEM.Buffer;
                    if (WB.NOPFlag)
                        WB_Buffer_Pre.Clear();
                    else
                        WB_Buffer_Pre = WB.Buffer;

                    IF_ID_EX_DiscardFlag = false;
                }

                if (MEM.BubbleFlag) BubbleCnt = 2;
                if (IsLoad(EX_Buffer_Pre.InsType) && (ID_Buffer_Pre.rs1 == EX_Buffer_Pre.rd || ID_Buffer_Pre.rs2 == EX_Buffer_Pre.rd))
                    ++Stall[1], ++Stall[2], ++Stall[3], HazardStallFlag = true;

                // forwarding (B5->B2)
                if (IsRegEdit(WB_Buffer_Pre.InsType) && ID_Buffer_Pre.rs1 && ID_Buffer_Pre.rs1 == WB_Buffer_Pre.rd)
                    ID_Buffer_Pre.rv1 = WB_Buffer_Pre.exr;
                if (IsRegEdit(WB_Buffer_Pre.InsType) && ID_Buffer_Pre.rs2 && ID_Buffer_Pre.rs2 == WB_Buffer_Pre.rd)
                    ID_Buffer_Pre.rv2 = WB_Buffer_Pre.exr;

                // forwarding (B4->B2)
                if (IsRegEdit(MEM_Buffer_Pre.InsType) && ID_Buffer_Pre.rs1 && ID_Buffer_Pre.rs1 == MEM_Buffer_Pre.rd)
                    ID_Buffer_Pre.rv1 = MEM_Buffer_Pre.exr;
                if (IsRegEdit(MEM_Buffer_Pre.InsType) && ID_Buffer_Pre.rs2 && ID_Buffer_Pre.rs2 == MEM_Buffer_Pre.rd)
                    ID_Buffer_Pre.rv2 = MEM_Buffer_Pre.exr;

                // forwarding (B3->B2)
                if (IsRegEdit(EX_Buffer_Pre.InsType) && ID_Buffer_Pre.rs1 && ID_Buffer_Pre.rs1 == EX_Buffer_Pre.rd)
                    ID_Buffer_Pre.rv1 = EX_Buffer_Pre.exr;
                if (IsRegEdit(EX_Buffer_Pre.InsType) && ID_Buffer_Pre.rs2 && ID_Buffer_Pre.rs2 == EX_Buffer_Pre.rd)
                    ID_Buffer_Pre.rv2 = EX_Buffer_Pre.exr;

#ifdef RISC_V_DEBUG
                printf("\n[Register]");
                bool DEBUG_REG_FLAG = false;
                for(u32 i = 0; i < 32; ++i)
                    if(Reg.Load(i))
                        DEBUG_REG_FLAG = true,
                                printf("\n%s: %d", RegTable[i].c_str(), Reg.Load(i));
                if(!DEBUG_REG_FLAG)
                    printf("\nAll Register is 0!");
                printf("\n");
                printf("\n[IF_Buffer]\n");
                printf("pc: %x\npcPredict: %x\nIns: 0x%08x\n", IF_Buffer_Pre.pc, IF_Buffer_Pre.pcPredict, IF_Buffer_Pre.Ins);
                printf("\n[ID_Buffer]\n");
                if(ID_Buffer_Pre.InsType)printf("InsType: %s\npc: %x\nrd: %s\nimm: %d\nrs1: %s\nrs2: %s\nrv1: %d\nrv2: %d\n", InsTable[ID_Buffer_Pre.InsType].c_str(), ID_Buffer_Pre.pc, RegTable[ID_Buffer_Pre.rd].c_str(), ID_Buffer_Pre.imm, RegTable[ID_Buffer_Pre.rs1].c_str(), RegTable[ID_Buffer_Pre.rs2].c_str(), ID_Buffer_Pre.rv1, ID_Buffer_Pre.rv2);
                else printf("Empty!\n");
                printf("\n[EX_Buffer]\n");
                if(EX_Buffer_Pre.InsType) printf("InsType: %s\npc: %x\nrd: %s\nexr: %d\nad: %d\n", InsTable[EX_Buffer_Pre.InsType].c_str(), EX_Buffer_Pre.pc, RegTable[EX_Buffer_Pre.rd].c_str(), EX_Buffer_Pre.exr, EX_Buffer_Pre.ad);
                else printf("Empty!\n");
                printf("\n[MEM_Buffer]\n");
                if(MEM_Buffer_Pre.InsType) printf("InsType: %s\npc: %x\nrd: %s\nexr: %d\n", InsTable[MEM_Buffer_Pre.InsType].c_str(), MEM_Buffer_Pre.pc, RegTable[MEM_Buffer_Pre.rd].c_str(), MEM_Buffer_Pre.exr);
                else printf("Empty!\n");
                printf("\n[WB_Buffer]\n");
                if(WB_Buffer_Pre.InsType) printf("InsType: %s\npc: %x\nrd: %s\nexr: %d\n", InsTable[WB_Buffer_Pre.InsType].c_str(), WB_Buffer_Pre.pc, RegTable[WB_Buffer_Pre.rd].c_str(), WB_Buffer_Pre.exr);
                else printf("Empty!\n");
                printf("\n----------\n");
#endif
            }

#ifdef RISC_V_PRINT
            printf("CLK: %u\n", CLK);
            Pred.PrintResult();
#endif
            return Reg.Load(10) & 0xFFu;
        }
    };
} // namespace CPU

#endif // RISC_V_SIMULATOR_CPU