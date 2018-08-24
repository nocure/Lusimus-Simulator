/*
 * =====================================================================================
 *
 *       Filename:  i_smul.c
 *
 *    Description:  Implementation of the UDIV instruction
 *
 *        Version:  1.0
 *        Created:  16/04/08 18:09:22
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Aitor Viana Sanchez (avs), aitor.viana.sanchez@esa.int
 *        Company:  European Space Agency (ESA-ESTEC)
 *
 * =====================================================================================
 */

/*  Format (3)

31-30  29-25   24-19     18-14    13        12-5          4-0
+----+-------+--------+------------------------------------------+
| op |   rd  |  op3   |   rs1   | i=0 |   non-used   |    rs2    |
+----+-------+--------+------------------------------------------+

31-30  29-25   24-19     18-14    13            12-0
+----+-------+--------+------------------------------------------+
| op |   rd  |  op3   |   rs1   | i=1 |         simm13           |
+----+-------+--------+------------------------------------------+

op              = 10
rd              = 00000
op3             = 001110

31-30  29-25   24-19     18-14    13        12-5          4-0
+----+-------+--------+------------------------------------------+
| 10 | 00000 | 001110 |   rs1   | 1/0 |      simm13/rs2          |
+----+-------+--------+------------------------------------------+

*/
#include "../common/bits.h"
#include "../common/sparc.h"
#include "../common/traps.h"
#include "../common/iu.h"
#include "i_utils.h"

#include <stdio.h>

static int execute(void *);
static int disassemble(uint32 instr, void *state);
static int rd, rs1, rs2, imm;

#define SMUL_CYCLES    1
#define SMUL_CODE_MASK 0x80580000
#define PRIVILEDGE  0

#define OP_OFF_first     30
#define OP_OFF_last      31
#define OP         0x2

#define OP3_OFF_first     19
#define OP3_OFF_last      24
#define OP3         0x0b

#define RD_OFF_first      25
#define RD_OFF_last       29

#define RS1_OFF_first     14
#define RS1_OFF_last      18

#define RS2_OFF_first     0
#define RS2_OFF_last      4

#define I_OFF       13

#define SIMM13_OFF_first  0
#define SIMM13_OFF_last   12

sparc_instruction_t i_smul = {
    execute,
    disassemble,
    SMUL_CODE_MASK,
    PRIVILEDGE,
    OP,
};



static int execute(void *state)
{
    int64 result = 0;

    if( imm < 0 )
    {
        result = (int64)(REG(rs1) * REG(rs2));
//        DBG("smul reg[%d], reg[%d], reg[%d]\n", rs1, rs2, rd);
        print_inst_RS_RS_RD("smul",  rs1, rs2, rd);
    }
    else
    {
        result = (int64)(REG(rs1) * (int)sign_ext13(imm));
//        DBG("smul reg[%d], 0x%x, reg[%d]\n", rs1, imm, rd);
        print_inst_RS_IM13_RD("smul", rs1, sign_ext13(imm), rd);
    }

    REG(rd) = (int)result;
    YREG = (int)(result >> 32);
//    REG(rd) = (int32)bits(result, 31, 0);
//    YREG = (int32)bits(result, 63, 32);
    PCREG = NPCREG;
    NPCREG += 4;

    // Everyting takes some time
    return SMUL_CYCLES;

}

static int disassemble(uint32 instr, void *state)
{
    int op3 = 0, op;

    op = bits(instr, OP_OFF_last, OP_OFF_first);
    op3 = bits(instr, OP3_OFF_last, OP3_OFF_first);

    if( (instr & SMUL_CODE_MASK) && (op == OP) && (op3 == OP3) )
    {
        rd = bits(instr, RD_OFF_last, RD_OFF_first);
        rs1 = bits(instr, RS1_OFF_last, RS1_OFF_first);
        int i = bit(instr, I_OFF);

        if( i )
        {
            imm = bits(instr, SIMM13_OFF_last, SIMM13_OFF_first);
            rs2 = -1;
        }
        else
        {
            rs2 = bits(instr, RS2_OFF_last, RS2_OFF_first);
            imm = -1;
        }

        return 1;
    }
    return 0;
}

