#ifndef RISC_V_SIMULATOR_PREDICTOR
#define RISC_V_SIMULATOR_PREDICTOR

#include <iostream>
#include "Instruction.hpp"

using namespace INSTRUCTION;

//#define TWO_BIT_COUNTER_PREDICTION
//#define TWO_LEVEL_PREDICTION
#define HYBRID_BRANCH_PREDICTION

double ToTalCorrectRate = 0;

#ifdef TWO_BIT_COUNTER_PREDICTION

#define PREDICTOR_SIZE 1500

namespace PREDICTOR
{
    class Predictor
    {
    private:
        u32 ToTalNum, CorrectNum;
        u32 BTB[PREDICTOR_SIZE];
        u8 TwoBitCounter[PREDICTOR_SIZE];

    public:
        Predictor() : ToTalNum(0), CorrectNum(0)
        {
            memset(TwoBitCounter, 1, sizeof(TwoBitCounter)); // weakly not taken
            for (u32 i = 0; i < PREDICTOR_SIZE << 2; i += 4) BTB[i >> 2] = i + 4;
        }

        void NextPredict(u32 pc, u32 Ins, u32 &pcNext, u32 &pcPredict)
        {
            u32 opcode = (Ins & 0b00000000'00000000'00000000'01111111u);
            if ((opcode & 0b1000000u) == 0)
            {
                pcNext = pc + 4;
                pcPredict = 0;
                return;
            }

            if (opcode == 0b1101111u || opcode == 0b1100111u) // JAL or JALR
                pcNext = pcPredict = BTB[pc >> 2];
            if (opcode == 0b1100011u) // Branch
            {
                if (TwoBitCounter[pc >> 2] & 0b10u) pcNext = pcPredict = BTB[pc >> 2];
                else pcNext = pcPredict = pc + 4;
            }
        }

        void Update(u32 pc, u32 pcNext, u32 pcPredict, INS_TYPE InsType)
        {
            if (IsBranch(InsType))
            {
                //printf("pc: %x\npcNext: %x\npcPredict: %x\n%s\n\n", pc, pcNext, pcPredict, pcNext == pcPredict ? "Correct!" : "Wrong!");
                ++ToTalNum;
                if (pcNext == pcPredict) ++CorrectNum;
            }
            if (pcNext == pc + 4 && TwoBitCounter[pc >> 2]) // not taken
                --TwoBitCounter[pc >> 2];
            else // taken
            {
                if (TwoBitCounter[pc >> 2] != 0b11u) ++TwoBitCounter[pc >> 2];
                if (BTB[pc >> 2] == pc + 4) BTB[pc >> 2] = pcNext;
            }
        }

        void PrintResult() const
        {
            printf("PredictTotal: %d\nPredictCorrect: %d\n", ToTalNum, CorrectNum);
            if (ToTalNum) printf("Predict Correct Rate: %lf%%\n", 100 * (double)CorrectNum / ToTalNum);
            else printf("Predict Correct Rate: /\n");
            if (ToTalNum) ToTalCorrectRate += 100 * (double)CorrectNum / ToTalNum;
        }
    };
} // namespace PREDICTOR

#endif // TWO_BIT_COUNTER_PREDICTION


#ifdef TWO_LEVEL_PREDICTION

#define PREDICTOR_SIZE 1500
#define BRANCH_HISTORY_SIZE 7

namespace PREDICTOR
{
    class Predictor
    {
    private:
        u32 ToTalNum, CorrectNum;
        u32 BTB[PREDICTOR_SIZE]; // Branch Target Buffer
        u8 BHT[PREDICTOR_SIZE];  // Branch History Table
        u8 PHT[PREDICTOR_SIZE][1 << BRANCH_HISTORY_SIZE]; // Pattern History Table

    public:
        Predictor() : ToTalNum(0), CorrectNum(0)
        {
            memset(BHT, 0, sizeof(BHT));
            memset(PHT, 1, sizeof(PHT)); // weakly not taken
            for (u32 i = 0; i < PREDICTOR_SIZE << 2; i += 4) BTB[i >> 2] = i + 4;
        }

        #define pcIndex pc >> 2

        void NextPredict(u32 pc, u32 Ins, u32 &pcNext, u32 &pcPredict)
        {
            u32 opcode = (Ins & 0b1111111u);
            if ((opcode & 0b1000000u) == 0) // not Jump or Branch
            {
                pcNext = pc + 4;
                pcPredict = 0;
                return;
            }

            if (opcode == 0b1101111u || opcode == 0b1100111u) // Jump
                pcNext = pcPredict = BTB[pcIndex];
            if (opcode == 0b1100011u) // Branch
            {
                if (PHT[pcIndex][BHT[pcIndex]] & 0b10u) pcNext = pcPredict = BTB[pcIndex];
                else pcNext = pcPredict = pc + 4;
            }
        }

        void Update(u32 pc, u32 pcNext, u32 pcPredict, INS_TYPE InsType)
        {
            if (IsBranch(InsType))
            {
                ++ToTalNum;
                if (pcNext == pcPredict) ++CorrectNum;
            }
            if (pcNext == pc + 4) // not taken
            {
                if (PHT[pcIndex][BHT[pcIndex]]) --PHT[pcIndex][BHT[pcIndex]];
                BHT[pcIndex] <<= 1;
            }
            else // taken
            {
                if (PHT[pcIndex][BHT[pcIndex]] != 0b11u) ++PHT[pcIndex][BHT[pcIndex]];
                BHT[pcIndex] = BHT[pcIndex] << 1 | 1;
                if (BTB[pcIndex] == pc + 4) BTB[pcIndex] = pcNext;
            }
        }

        void PrintResult() const
        {
            printf("PredictTotal: %d\nPredictCorrect: %d\n", ToTalNum, CorrectNum);
            if (ToTalNum) printf("Predict Correct Rate: %lf%%\n", 100 * (double)CorrectNum / ToTalNum);
            else printf("Predict Correct Rate: /\n");
            if (ToTalNum) ToTalCorrectRate += 100 * (double)CorrectNum / ToTalNum;
        }
    };
} // namespace PREDICTOR

#endif // TWO_LEVEL_PREDICTION


#ifdef HYBRID_BRANCH_PREDICTION

#define PREDICTOR_SIZE 1500
#define SWITCH_THRESHOLD 50

namespace PREDICTOR
{
    class Predictor
    {
    private:
        u32 ToTalNum, CorrectNum;
        u32 Counter[PREDICTOR_SIZE];
        u32 BTB[PREDICTOR_SIZE]; // Branch Target Buffer
        u8 BHT[PREDICTOR_SIZE];  // Branch History Table
        u8 PHT[PREDICTOR_SIZE][256]; // Pattern History Table
        u8 TwoBitCounter[PREDICTOR_SIZE];

    public:
        Predictor() : ToTalNum(0), CorrectNum(0)
        {
            memset(BHT, 0, sizeof(BHT));
            memset(PHT, 1, sizeof(PHT)); // weakly not taken
            memset(TwoBitCounter, 1, sizeof(TwoBitCounter)); // weakly not taken
            for (u32 i = 0; i < PREDICTOR_SIZE << 2; i += 4) BTB[i >> 2] = i + 4;
        }

#define pcIndex pc >> 2

        void NextPredict(u32 pc, u32 Ins, u32 &pcNext, u32 &pcPredict)
        {
            u32 opcode = (Ins & 0b1111111u);
            if ((opcode & 0b1000000u) == 0) // not Jump or Branch
            {
                pcNext = pc + 4;
                pcPredict = 0;
                return;
            }

            if (opcode == 0b1101111u || opcode == 0b1100111u) // Jump
                pcNext = pcPredict = BTB[pcIndex];
            if (opcode == 0b1100011u) // Branch
            {
                if (Counter[pcIndex] > SWITCH_THRESHOLD) // two level prediction
                {
                    if (PHT[pcIndex][BHT[pcIndex]] & 0b10u) pcNext = pcPredict = BTB[pcIndex];
                    else pcNext = pcPredict = pc + 4;
                }
                else // two bit counter
                {
                    if (TwoBitCounter[pcIndex] & 0b10u) pcNext = pcPredict = BTB[pcIndex];
                    else pcNext = pcPredict = pc + 4;
                }
            }
        }

        void Update(u32 pc, u32 pcNext, u32 pcPredict, INS_TYPE InsType)
        {
            ++Counter[pcIndex];
            if (IsBranch(InsType))
            {
                //printf("pc: %x\npcNext: %x\npcPredict: %x\n%s\n\n", pc, pcNext, pcPredict, pcNext == pcPredict ? "Correct!" : "Wrong!");
                ++ToTalNum;
                if (pcNext == pcPredict) ++CorrectNum;
            }
            if (pcNext == pc + 4) // not taken
            {
                if (TwoBitCounter[pcIndex]) --TwoBitCounter[pcIndex];
                if (PHT[pcIndex][BHT[pcIndex]]) --PHT[pcIndex][BHT[pcIndex]];
                BHT[pcIndex] <<= 1;
            }
            else // taken
            {
                if (TwoBitCounter[pcIndex] != 0b11u) ++TwoBitCounter[pcIndex];
                if (PHT[pcIndex][BHT[pcIndex]] != 0b11u) ++PHT[pcIndex][BHT[pcIndex]];
                BHT[pcIndex] = BHT[pcIndex] << 1 | 1;
                if (BTB[pcIndex] == pc + 4) BTB[pcIndex] = pcNext;
            }
        }

        void PrintResult() const
        {
            printf("PredictTotal: %d\nPredictCorrect: %d\n", ToTalNum, CorrectNum);
            if (ToTalNum) printf("Predict Correct Rate: %lf%%\n", 100 * (double)CorrectNum / ToTalNum);
            else printf("Predict Correct Rate: /\n");
            if (ToTalNum) ToTalCorrectRate += 100 * (double)CorrectNum / ToTalNum;
        }
    };
} // namespace PREDICTOR

#endif // HYBRID_BRANCH_PREDICTION

#endif // RISC_V_SIMULATOR_PREDICTOR