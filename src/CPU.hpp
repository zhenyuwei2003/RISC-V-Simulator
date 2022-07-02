#ifndef RISC_V_SIMULATOR_CPU
#define RISC_V_SIMULATOR_CPU

#include "Stage.hpp"
//#include "LocalTest.h"

using namespace MEMORY;
using namespace REGISTER;
using namespace STAGE;
using namespace BUFFER;
using namespace PREDICTOR;

namespace CPU
{
    class CPU
    {
    private:
        Memory Mem;
        Register Reg;
        Predictor Pred;

        u32 pc, pcNext;
        u32 StopCnt, BubbleCnt;
        bool HazardStallFlag, StallFlag_IF, StallFlag_ID, StallFlag_EX, IF_ID_DiscardFlag;

        StageIF IF;
        StageID ID;
        StageEX EX;
        StageMEM MEM;
        StageWB WB;

        IF_Buffer IF_Buffer_Pre;
        ID_Buffer ID_Buffer_Pre;
        EX_Buffer EX_Buffer_Pre;
        MEM_Buffer MEM_Buffer_Pre;
        WB_Buffer WB_Buffer_Pre;

    public:
        explicit CPU(istream &InputStream)
            : Mem(InputStream), pc(0), pcNext(0), StopCnt(0), BubbleCnt(0),
              HazardStallFlag(false), StallFlag_IF(false), StallFlag_ID(false), StallFlag_EX(false), IF_ID_DiscardFlag(false),
              IF(Mem, Pred, pc, pcNext, StopCnt, StallFlag_IF),
              ID(IF_Buffer_Pre, Reg, StopCnt, StallFlag_ID),
              EX(ID_Buffer_Pre, Pred, pcNext, StopCnt, StallFlag_EX, IF_ID_DiscardFlag),
              MEM(EX_Buffer_Pre, Mem, StopCnt),
              WB(MEM_Buffer_Pre, Reg)
              {}

        u32 run()
        {
            u32 CLK = 0;
            while (true)
            {
                ++CLK;
                if (BubbleCnt) { --BubbleCnt; continue; }

                IF.execute();
                ID.execute();
                WB.execute();
                EX.execute();
                MEM.execute();

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
                    if (IF.NOPFlag || IF_ID_DiscardFlag)
                        IF_Buffer_Pre.Clear();
                    else
                        IF_Buffer_Pre = IF.Buffer;
                    if (ID.NOPFlag || IF_ID_DiscardFlag)
                        ID_Buffer_Pre.Clear();
                    else
                        ID_Buffer_Pre = ID.Buffer;
                    if (EX.NOPFlag)
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

                    IF_ID_DiscardFlag = false;
                }

                // Bubble/Stall
                if (MEM.BubbleFlag) BubbleCnt = 2;
                if (IsLoad(EX_Buffer_Pre.InsType) && (ID.rs1 == EX_Buffer_Pre.rd || ID.rs2 == EX_Buffer_Pre.rd))
                    StallFlag_IF = StallFlag_ID = StallFlag_EX = true, HazardStallFlag = true;

                // forwarding (WB.Buffer -> ID.Buffer)
                if (ID.rs1 && ID.rs1 == WB_Buffer_Pre.rd && IsRegEdit(WB_Buffer_Pre.InsType))
                    ID_Buffer_Pre.rv1 = WB_Buffer_Pre.exr;
                if (ID.rs2 && ID.rs2 == WB_Buffer_Pre.rd && IsRegEdit(WB_Buffer_Pre.InsType))
                    ID_Buffer_Pre.rv2 = WB_Buffer_Pre.exr;

                // forwarding (MEM.Buffer -> ID.Buffer)
                if (ID.rs1 && ID.rs1 == MEM_Buffer_Pre.rd && IsRegEdit(MEM_Buffer_Pre.InsType))
                    ID_Buffer_Pre.rv1 = MEM_Buffer_Pre.exr;
                if (ID.rs2 && ID.rs2 == MEM_Buffer_Pre.rd && IsRegEdit(MEM_Buffer_Pre.InsType))
                    ID_Buffer_Pre.rv2 = MEM_Buffer_Pre.exr;

                // forwarding (EX.Buffer -> ID.Buffer)
                if (ID.rs1 && ID.rs1 == EX_Buffer_Pre.rd && IsRegEdit(EX_Buffer_Pre.InsType))
                    ID_Buffer_Pre.rv1 = EX_Buffer_Pre.exr;
                if (ID.rs2 && ID.rs2 == EX_Buffer_Pre.rd && IsRegEdit(EX_Buffer_Pre.InsType))
                    ID_Buffer_Pre.rv2 = EX_Buffer_Pre.exr;

#ifdef RISC_V_DEBUG
                printf("\n[Register]");
                bool DEBUG_REG_FLAG = false;
                for (u32 i = 0; i < 32; ++i)
                    if (Reg.Load(i))
                        DEBUG_REG_FLAG = true,
                                printf("\n%s: %d", RegTable[i].c_str(), Reg.Load(i));
                if (!DEBUG_REG_FLAG)
                    printf("\nAll Register is 0!");
                printf("\n");

                printf("\n[IF_Buffer]\n");
                if (IF_Buffer_Pre.Ins) printf("pc: %x\npcPredict: %x\nIns: 0x%08x\n", IF_Buffer_Pre.pc, IF_Buffer_Pre.pcPredict, IF_Buffer_Pre.Ins);
                else printf("Empty!\n");

                printf("\n[ID_Buffer]\n");
                if (ID_Buffer_Pre.InsType)printf("InsType: %s\npc: %x\nrd: %s\nimm: %d\nrs1: %s\nrs2: %s\nrv1: %d\nrv2: %d\n", InsTable[ID_Buffer_Pre.InsType].c_str(), ID_Buffer_Pre.pc, RegTable[ID_Buffer_Pre.rd].c_str(), ID_Buffer_Pre.imm, RegTable[ID_Buffer_Pre.rs1].c_str(), RegTable[ID_Buffer_Pre.rs2].c_str(), ID_Buffer_Pre.rv1, ID_Buffer_Pre.rv2);
                else printf("Empty!\n");

                printf("\n[EX_Buffer]\n");
                if (EX_Buffer_Pre.InsType) printf("InsType: %s\npc: %x\nrd: %s\nexr: %d\nad: %d\n", InsTable[EX_Buffer_Pre.InsType].c_str(), EX_Buffer_Pre.pc, RegTable[EX_Buffer_Pre.rd].c_str(), EX_Buffer_Pre.exr, EX_Buffer_Pre.ad);
                else printf("Empty!\n");

                printf("\n[MEM_Buffer]\n");
                if (MEM_Buffer_Pre.InsType) printf("InsType: %s\npc: %x\nrd: %s\nexr: %d\n", InsTable[MEM_Buffer_Pre.InsType].c_str(), MEM_Buffer_Pre.pc, RegTable[MEM_Buffer_Pre.rd].c_str(), MEM_Buffer_Pre.exr);
                else printf("Empty!\n");

                printf("\n[WB_Buffer]\n");
                if (WB_Buffer_Pre.InsType) printf("InsType: %s\npc: %x\nrd: %s\nexr: %d\n", InsTable[WB_Buffer_Pre.InsType].c_str(), WB_Buffer_Pre.pc, RegTable[WB_Buffer_Pre.rd].c_str(), WB_Buffer_Pre.exr);
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