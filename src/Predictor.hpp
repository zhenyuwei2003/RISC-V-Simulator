#ifndef RISC_V_SIMULATOR_PREDICTOR
#define RISC_V_SIMULATOR_PREDICTOR

#include <iostream>
#include "Instruction.hpp"

using namespace INSTRUCTION;

//#define STATIC_PREDICTION
//#define FOUR_BIT_COUNTER_PREDICTION
//#define TWO_LEVEL_PREDICTION
#define HYBRID_BRANCH_PREDICTION

double ToTalCorrectRate = 0;

#ifdef STATIC_PREDICTION
namespace PREDICTOR
{
    class Predictor
    {
    private:
        u32 ToTalNum, CorrectNum;

    public:
        Predictor() : ToTalNum(0), CorrectNum(0) {}
        ~Predictor() { if (ToTalNum) ToTalCorrectRate += 100 * (double)CorrectNum / ToTalNum; }

        void NextPredict(u32 pc, u32 Ins, u32 &pcNext, u32 &pcPredict) { pcNext = pcPredict = pc + 4; }

        void Update(u32 pc, u32 pcNext, u32 pcPredict, INS_TYPE InsType)
        {
            if (IsBranch(InsType))
            {
                ++ToTalNum;
                if (pcNext == pcPredict) ++CorrectNum;
            }
        }

        void PrintResult() const
        {
            printf("PredictTotal: %d\nPredictCorrect: %d\n", ToTalNum, CorrectNum);
            if (ToTalNum) printf("Predict Correct Rate: %lf%%\n", 100 * (double)CorrectNum / ToTalNum);
            else printf("Predict Correct Rate: /\n");
        }
    };
} // namespace PREDICTOR
#endif // STATIC_PREDICTION

#ifdef FOUR_BIT_COUNTER_PREDICTION
#define PREDICTOR_SIZE 1500

namespace PREDICTOR
{
    class Predictor
    {
    private:
        u32 ToTalNum, CorrectNum;
        u32 BTB[PREDICTOR_SIZE];
        u8 FourBitCounter[PREDICTOR_SIZE];

    public:
        Predictor() : ToTalNum(0), CorrectNum(0)
        {
            memset(FourBitCounter, 0b0111u, sizeof(FourBitCounter));
            for (u32 i = 0; i < PREDICTOR_SIZE << 2; i += 4) BTB[i >> 2] = i + 4;
        }
        ~Predictor() { if (ToTalNum) ToTalCorrectRate += 100 * (double)CorrectNum / ToTalNum; }

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
                if (FourBitCounter[pc >> 2] & 0b1000u) pcNext = pcPredict = BTB[pc >> 2];
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
            if (pcNext == pc + 4 && FourBitCounter[pc >> 2]) // not taken
                --FourBitCounter[pc >> 2];
            else // taken
            {
                if (FourBitCounter[pc >> 2] != 0b1111u) ++FourBitCounter[pc >> 2];
                if (BTB[pc >> 2] == pc + 4) BTB[pc >> 2] = pcNext;
            }
        }

        void PrintResult() const
        {
            printf("PredictTotal: %d\nPredictCorrect: %d\n", ToTalNum, CorrectNum);
            if (ToTalNum) printf("Predict Correct Rate: %lf%%\n", 100 * (double)CorrectNum / ToTalNum);
            else printf("Predict Correct Rate: /\n");
        }
    };
} // namespace PREDICTOR
#endif // FOUR_BIT_COUNTER_PREDICTION

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
            memset(PHT, 0b0111u, sizeof(PHT));
            for (u32 i = 0; i < PREDICTOR_SIZE << 2; i += 4) BTB[i >> 2] = i + 4;
        }
        ~Predictor() { if (ToTalNum) ToTalCorrectRate += 100 * (double)CorrectNum / ToTalNum; }

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
                if (PHT[pcIndex][BHT[pcIndex]] & 0b1000u) pcNext = pcPredict = BTB[pcIndex];
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
                if (PHT[pcIndex][BHT[pcIndex]] != 0b1111u) ++PHT[pcIndex][BHT[pcIndex]];
                BHT[pcIndex] = BHT[pcIndex] << 1 | 1;
                if (BTB[pcIndex] == pc + 4) BTB[pcIndex] = pcNext;
            }
        }

        void PrintResult() const
        {
            printf("PredictTotal: %d\nPredictCorrect: %d\n", ToTalNum, CorrectNum);
            if (ToTalNum) printf("Predict Correct Rate: %lf%%\n", 100 * (double)CorrectNum / ToTalNum);
            else printf("Predict Correct Rate: /\n");
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
        u32 BTB[PREDICTOR_SIZE];
        u8 BHT[PREDICTOR_SIZE];
        u8 PHT[PREDICTOR_SIZE][256];
        u8 FourBitCounter[PREDICTOR_SIZE];

    public:
        Predictor() : ToTalNum(0), CorrectNum(0)
        {
            memset(BHT, 0, sizeof(BHT));
            memset(PHT, 0b0111u, sizeof(PHT));
            memset(FourBitCounter, 0b0111u, sizeof(FourBitCounter));
            for (u32 i = 0; i < PREDICTOR_SIZE << 2; i += 4) BTB[i >> 2] = i + 4;
        }
        ~Predictor() { if (ToTalNum) ToTalCorrectRate += 100 * (double)CorrectNum / ToTalNum; }

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
                    if (PHT[pcIndex][BHT[pcIndex]] & 0b1000u) pcNext = pcPredict = BTB[pcIndex];
                    else pcNext = pcPredict = pc + 4;
                }
                else // four bit counter
                {
                    if (FourBitCounter[pcIndex] & 0b1000u) pcNext = pcPredict = BTB[pcIndex];
                    else pcNext = pcPredict = pc + 4;
                }
            }
        }

        void Update(u32 pc, u32 pcNext, u32 pcPredict, INS_TYPE InsType)
        {
            ++Counter[pcIndex];
            if (IsBranch(InsType))
            {
                ++ToTalNum;
                if (pcNext == pcPredict) ++CorrectNum;
            }
            if (pcNext == pc + 4) // not taken
            {
                if (FourBitCounter[pcIndex]) --FourBitCounter[pcIndex];
                if (PHT[pcIndex][BHT[pcIndex]]) --PHT[pcIndex][BHT[pcIndex]];
                BHT[pcIndex] <<= 1;
            }
            else // taken
            {
                if (FourBitCounter[pcIndex] != 0b1111u) ++FourBitCounter[pcIndex];
                if (PHT[pcIndex][BHT[pcIndex]] != 0b1111u) ++PHT[pcIndex][BHT[pcIndex]];
                BHT[pcIndex] = BHT[pcIndex] << 1 | 1;
                if (BTB[pcIndex] == pc + 4) BTB[pcIndex] = pcNext;
            }
        }

        void PrintResult() const
        {
            printf("PredictTotal: %d\nPredictCorrect: %d\n", ToTalNum, CorrectNum);
            if (ToTalNum) printf("Predict Correct Rate: %lf%%\n", 100 * (double)CorrectNum / ToTalNum);
            else printf("Predict Correct Rate: /\n");
        }
    };
} // namespace PREDICTOR
#endif // HYBRID_BRANCH_PREDICTION

#endif // RISC_V_SIMULATOR_PREDICTOR