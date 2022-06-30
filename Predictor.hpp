#ifndef RISC_V_SIMULATOR_PREDICTOR
#define RISC_V_SIMULATOR_PREDICTOR

#include <iostream>
#include "Instruction.hpp"

using namespace INSTRUCTION;

#define PREDICTOR_SIZE 65535

namespace PREDICTOR
{
    class Predictor
    {
    private:
        u32 ToTalNum, CorrectNum;
        u32 pcAddress[PREDICTOR_SIZE];
        char TwoBitCounter[PREDICTOR_SIZE];

    public:
        Predictor() : ToTalNum(0), CorrectNum(0)
        {
            memset(TwoBitCounter, 1, sizeof(TwoBitCounter)); // weakly not taken
            for (u32 i = 0; i < PREDICTOR_SIZE; ++i)
                pcAddress[i] = i + 4;
        }

        void NextPredict(u32 pc, u32 Ins, u32 &pcNext, u32 &pcPredict)
        {
            u32 opcode = (Ins & 0b00000000'00000000'00000000'01111111u);
            if((opcode & 0b1000000u) == 0)
            {
                pcNext = pc + 4;
                pcPredict = 0;
                return;
            }

            ++ToTalNum;
            if(opcode == 0b1101111u) // JAL
            {
                pcNext = pcPredict = pcAddress[pc];
            }
            if(opcode == 0b1100111u) // JALR
            {
                pcNext = pcPredict = pcAddress[pc];
            }
            if(opcode == 0b1100011u) // Branch
            {
                if (TwoBitCounter[pc] & 0b10u)
                    pcNext = pcPredict = pcAddress[pc];
                else
                    pcNext = pcPredict = pc + 4;
            }
        }

        void Update(u32 pc, u32 pcNext, u32 pcPredict)
        {
            if (pcNext == pcPredict) ++CorrectNum;
            if (pcNext == pc + 4) // not taken
            {
                if (TwoBitCounter[pc]) --TwoBitCounter[pc];
            }
            else // taken
            {
                if (TwoBitCounter[pc] != 0b11u) ++TwoBitCounter[pc];
                if (pcAddress[pc] == pc + 4) pcAddress[pc] = pcNext;
            }
        }

        void PrintResult() const
        {
            printf("Total: %d, Correct: %d\n", ToTalNum, CorrectNum);
            printf("Prediction Correct Rate: %lf\n", (double)CorrectNum / ToTalNum);
        }
    };
} // namespace PREDICTOR

#endif // RISC_V_SIMULATOR_PREDICTOR