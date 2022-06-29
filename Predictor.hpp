#ifndef RISC_V_SIMULATOR_PREDICTOR
#define RISC_V_SIMULATOR_PREDICTOR

#include <iostream>

namespace PREDICTOR
{
    class Predictor
    {
    private:
        int ToTalNum, CorrectNum;

    public:
        Predictor() : ToTalNum(0), CorrectNum(0) {}

        int NextPredict(int pc)
        {
            ++ToTalNum;
            return pc + 4;
        }

        void Update(int pc, int pcPredict)
        {
            if(pc == pcPredict) ++CorrectNum;
        }

        void PrintResult() const
        {
            printf("Total: %d, Correct: %d\n", ToTalNum, CorrectNum);
            printf("Prediction Correct Rate: %lf\n", (double)CorrectNum / ToTalNum);
        }
    };
} // namespace PREDICTOR

#endif // RISC_V_SIMULATOR_PREDICTOR