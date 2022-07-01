#ifndef RISC_V_SIMULATOR_INSTRUCTION
#define RISC_V_SIMULATOR_INSTRUCTION

#include <iostream>

using std::string;
using u8 = unsigned char;
using u32 = unsigned int;

namespace INSTRUCTION
{
    const string InsTable[] =
    {
        "NOP", "ADD", "ADDI", "AND", "ANDI", "AUIPC", "BEQ", "BGE",
        "BGEU", "BLT", "BLTU", "BNE", "JAL", "JALR", "LB", "LBU",
        "LH", "LHU", "LUI", "LW", "OR", "ORI", "SB", "SH",
        "SLL", "SLLI", "SLT", "SLTI", "SLTIU", "SLTU", "SRA", "SRAI",
        "SRL", "SRLI", "SUB", "SW", "XOR", "XORI"
    };

    enum INS_TYPE
    {
        NOP,
        ADD,   // R   ADD
        ADDI,  // I   ADD Immediate
        AND,   // R   AND
        ANDI,  // I   AND Immediate
        AUIPC, // U   Add Upper Immediate to PC
        BEQ,   // SB  Branch EQual
        BGE,   // SB  Branch Greater than or Equal
        BGEU,  // SB  Branch Greater than or Equal Unsigned
        BLT,   // SB  Branch Less Than
        BLTU,  // SB  Branch Less Than Unsigned
        BNE,   // SB  Branch Not Equal
        JAL,   // UJ  Jump And Link
        JALR,  // I   Jump And Link Register
        LB,    // I   Load Byte
        LBU,   // I   Load Byte Unsigned
        LH,    // I   Load Halfword
        LHU,   // I   Load Halfword Unsigned
        LUI,   // U   Load Upper Immediate
        LW,    // I   Load Word
        OR,    // R   OR
        ORI,   // I   OR Immediate
        SB,    // S   Store Byte
        SH,    // S   Store Halfword
        SLL,   // R   Shift Left
        SLLI,  // I   Shift Left Immediate
        SLT,   // R   Set Less Than
        SLTI,  // I   Set Less Than Immediate
        SLTIU, // I   Set Less Than Immediate Unsigned
        SLTU,  // R   Set Less Than Unsigned
        SRA,   // R   Shift Right Arithmetic
        SRAI,  // I   Shift Right Arithmetic Immediate
        SRL,   // R   Shift Right
        SRLI,  // I   Shift Right Immediate
        SUB,   // R   SUBtract
        SW,    // S   Store Word
        XOR,   // R   XOR
        XORI   // I   XOR Immediate
    };

    bool IsJump(INS_TYPE x)
    { return (x == JAL) || (x == JALR); }

    bool IsBranch(INS_TYPE x)
    { return (x == BEQ) || (x == BGE) || (x == BGEU) || (x == BLT) || (x == BLTU) || (x == BNE); }

    bool IsLoad(INS_TYPE x)
    { return (x == LB) || (x == LBU) || (x == LH) || (x == LHU) || (x == LW); }

    bool IsStore(INS_TYPE x)
    { return (x == SB) || (x == SH) || (x == SW); }

    bool IsRegEdit(INS_TYPE x)
    { return !IsBranch(x) && !IsStore(x) && x != NOP; }

    void SignExtend(u32 &Ins, u32 HighBit)
    {
        if(Ins & (1u << HighBit))
            Ins |= 0xFFFFFFFFu << (HighBit + 1);
    }

    void InsDecode(const u32 Ins, INSTRUCTION::INS_TYPE &InsType, u32 &rd, u32 &imm, u32 &rs1, u32 &rs2, u32 &RegNum)
    {
        u32 opcode, funct3, funct7;
        opcode = (Ins & 0b00000000'00000000'00000000'01111111u);

        switch (opcode)
        {
            // R
            case 0b0110011u:
                funct3  = (Ins & 0b00000000'00000000'01110000'00000000u) >> 12u;
                funct7  = (Ins & 0b11111110'00000000'00000000'00000000u) >> 25u;
                rd      = (Ins & 0b00000000'00000000'00001111'10000000u) >> 7u;
                rs1     = (Ins & 0b00000000'00001111'10000000'00000000u) >> 15u;
                rs2     = (Ins & 0b00000001'11110000'00000000'00000000u) >> 20u;
                imm     = 0;
                RegNum = 2;
                switch (funct3)
                {
                    case 0b000u:
                        if (funct7 == 0b0000000u) InsType = ADD;
                        if (funct7 == 0b0100000u) InsType = SUB;
                        break;
                    case 0b001u: InsType = SLL; break;
                    case 0b010u: InsType = SLT; break;
                    case 0b011u: InsType = SLTU; break;
                    case 0b100u: InsType = XOR; break;
                    case 0b101u:
                        if (funct7 == 0b0000000u) InsType = SRL;
                        if (funct7 == 0b0100000u) InsType = SRA;
                        break;
                    case 0b110u: InsType = OR; break;
                    case 0b111u: InsType = AND; break;
                    default: printf("InsType ERROR!\n");
                }
                break;

            // I
            case 0b0000011u:
                funct3  = (Ins & 0b00000000'00000000'01110000'00000000u) >> 12u;
                rd      = (Ins & 0b00000000'00000000'00001111'10000000u) >> 7u;
                rs1     = (Ins & 0b00000000'00001111'10000000'00000000u) >> 15u;
                rs2     = 0;
                imm     = (Ins & 0b11111111'11110000'00000000'00000000u) >> 20u; // 31:20 -> 11:0
                SignExtend(imm, 11);
                RegNum = 1;
                switch (funct3)
                {
                    case 0b000u: InsType = LB; break;
                    case 0b001u: InsType = LH; break;
                    case 0b010u: InsType = LW; break;
                    case 0b100u: InsType = LBU; break;
                    case 0b101u: InsType = LHU; break;
                    default: printf("InsType ERROR!\n");
                }
                break;

            case 0b0010011u:
                funct3  = (Ins & 0b00000000'00000000'01110000'00000000u) >> 12u;
                funct7  = (Ins & 0b11111110'00000000'00000000'00000000u) >> 25u;
                rd      = (Ins & 0b00000000'00000000'00001111'10000000u) >> 7u;
                rs1     = (Ins & 0b00000000'00001111'10000000'00000000u) >> 15u;
                rs2     = 0;
                imm     = (Ins & 0b11111111'11110000'00000000'00000000u) >> 20u; // 31:20 -> 11:0
                SignExtend(imm, 11);
                RegNum = 1;
                switch (funct3)
                {
                    case 0b000u: InsType = ADDI; break;
                    case 0b001u: InsType = SLLI; break;
                    case 0b010u: InsType = SLTI; break;
                    case 0b011u: InsType = SLTIU; break;
                    case 0b100u: InsType = XORI; break;
                    case 0b101u:
                        if(funct7 == 0b0000000u) InsType = SRLI;
                        if(funct7 == 0b0100000u) InsType = SRAI;
                        break;
                    case 0b110u: InsType = ORI; break;
                    case 0b111u: InsType = ANDI; break;
                    default: printf("InsType ERROR!\n");
                }
                break;

            case 0b1100111u:
                rd  = (Ins & 0b00000000'00000000'00001111'10000000u) >> 7u;
                rs1 = (Ins & 0b00000000'00001111'10000000'00000000u) >> 15u;
                rs2 = 0;
                imm = (Ins & 0b11111111'11110000'00000000'00000000u) >> 20u; // 31:20 -> 11:0
                SignExtend(imm, 11);
                RegNum = 1;
                InsType = JALR;
                break;

            // S
            case 0b0100011u:
                funct3  = (Ins & 0b00000000'00000000'01110000'00000000u) >> 12u;
                rd      = 0;
                rs1     = (Ins & 0b00000000'00001111'10000000'00000000u) >> 15u;
                rs2     = (Ins & 0b00000001'11110000'00000000'00000000u) >> 20u;
                imm     = (Ins & 0b11111110'00000000'00000000'00000000u) >> 20u; // 31:25 -> 11:5
                imm    |= (Ins & 0b00000000'00000000'00001111'10000000u) >> 7u;  // 11:7  -> 4:0
                SignExtend(imm, 11);
                RegNum = 2;
                switch (funct3)
                {
                    case 0b000u: InsType = SB; break;
                    case 0b001u: InsType = SH; break;
                    case 0b010u: InsType = SW; break;
                    default: printf("InsType ERROR!\n");
                }
                break;

            // SB
            case 0b1100011u:
                funct3  = (Ins & 0b00000000'00000000'01110000'00000000u) >> 12u;
                rd      = 0;
                rs1     = (Ins & 0b00000000'00001111'10000000'00000000u) >> 15u;
                rs2     = (Ins & 0b00000001'11110000'00000000'00000000u) >> 20u;
                imm     = (Ins & 0b10000000'00000000'00000000'00000000u) >> 19u; // 31    -> 12
                imm    |= (Ins & 0b01111110'00000000'00000000'00000000u) >> 20u; // 30:25 -> 10:5
                imm    |= (Ins & 0b00000000'00000000'00001111'00000000u) >> 7u;  // 11:8  -> 4:1
                imm    |= (Ins & 0b00000000'00000000'00000000'10000000u) << 4u;  // 7     -> 11
                SignExtend(imm, 12);
                RegNum = 2;
                switch (funct3)
                {
                    case 0b000u: InsType = BEQ; break;
                    case 0b001u: InsType = BNE; break;
                    case 0b100u: InsType = BLT; break;
                    case 0b101u: InsType = BGE; break;
                    case 0b110u: InsType = BLTU; break;
                    case 0b111u: InsType = BGEU; break;
                    default: printf("InsType ERROR!\n");
                }
                break;

            // U
            case 0b0010111u:
                rd  = (Ins & 0b00000000'00000000'00001111'10000000u) >> 7u;
                rs1 = 0;
                rs2 = 0;
                imm = (Ins & 0b11111111'11111111'11110000'00000000u); // 31:12 -> 31:12
                RegNum = 0;
                InsType = AUIPC;
                break;

            case 0b0110111u:
                rd  = (Ins & 0b00000000'00000000'00001111'10000000u) >> 7u;
                rs1 = 0;
                rs2 = 0;
                imm = (Ins & 0b11111111'11111111'11110000'00000000u); // 31:12 -> 31:12
                RegNum = 0;
                InsType = LUI;
                break;

            // UJ
            case 0b1101111u:
                rd   = (Ins & 0b00000000'00000000'00001111'10000000u) >> 7u;
                rs1 = 0;
                rs2 = 0;
                imm  = (Ins & 0b10000000'00000000'00000000'00000000u) >> 11u; // 31    -> 20
                imm |= (Ins & 0b01111111'11100000'00000000'00000000u) >> 20u; // 30:21 -> 10:1
                imm |= (Ins & 0b00000000'00010000'00000000'00000000u) >> 9u;  // 20    -> 11
                imm |= (Ins & 0b00000000'00001111'11110000'00000000u);        // 19:12 -> 19:12
                SignExtend(imm, 20);
                RegNum = 0;
                InsType = JAL;
                break;

            default:
                InsType = NOP;
                break;
        }
    }
} // namespace INSTRUCTION

#endif // RISC_V_SIMULATOR_INSTRUCTION