#include <stdio.h>
#include <stdint.h>
#include "processor.h"
#include "memory.h"

int Register[256];
int VectorRegister[32][8];

int PC = 0;
int opcode = 0;
int dest = 0;
int src1 = 0;
int src2 = 0;

int Z = 0;
int N = 0;
int C = 0;
int V = 0;

int end_of_simulation = 0;

uint32_t readData(int address)
{
    uint32_t value = 0;
    if(address < 0 || address + 3 >= 4096)
        return 0;

    value |= (uint32_t)Data[address];
    value |= (uint32_t)Data[address + 1] << 8;
    value |= (uint32_t)Data[address + 2] << 16;
    value |= (uint32_t)Data[address + 3] << 24;
    return value;
}

void writeData(int address, uint32_t value)
{
    if(address < 0 || address + 3 >= 4096)
        return;

    Data[address]     = value & 0xFF;
    Data[address + 1] = (value >> 8) & 0xFF;
    Data[address + 2] = (value >> 16) & 0xFF;
    Data[address + 3] = (value >> 24) & 0xFF;
}

void setFlagsAdd(int a, int b, int result)
{
    unsigned int ua = (unsigned int)a;
    unsigned int ub = (unsigned int)b;
    unsigned int ur = (unsigned int)result;

    Z = (result == 0);
    N = ((unsigned int)result >> 31) & 1;
    C = (ur < ua || ur < ub);
    V = ((a >= 0 && b >= 0 && result < 0) ||
         (a < 0 && b < 0 && result >= 0));
}

void setFlagsSub(int a, int b, int result)
{
    Z = (result == 0);
    N = ((unsigned int)result >> 31) & 1;
    C = (a > b);
    V = ((a >= 0 && b < 0 && result < 0) ||
         (a < 0 && b >= 0 && result >= 0));
}

void reset(void)
{
    int i, j;

    for(i = 0; i < 256; i++)
        Register[i] = 0;

    for(i = 0; i < 32; i++)
        for(j = 0; j < 8; j++)
            VectorRegister[i][j] = 0;

    PC = 0;
    Z = 0;
    N = 0;
    C = 0;
    V = 0;
    end_of_simulation = 0;
}

void fetch(void)
{
    opcode = Instruction[PC];
    dest   = Instruction[PC + 1];
    src1   = Instruction[PC + 2];
    src2   = Instruction[PC + 3];
}

void decode(void)
{
}

void execute(void)
{
    int a, b, result;
    int take;
    int i;
    int address;
    int scalar;

    if(opcode == 0)
    {
        end_of_simulation = 1;
        return;
    }

    /* Integer operations */
    if(opcode == 0x01)
    {
        a = Register[src1]; b = Register[src2]; result = a + b;
        Register[dest] = result; setFlagsAdd(a, b, result);
    }
    else if(opcode == 0x02)
    {
        a = Register[src1]; b = Register[src2]; result = a - b;
        Register[dest] = result; setFlagsSub(a, b, result);
    }
    else if(opcode == 0x03)
        Register[dest] = Register[src1] * Register[src2];
    else if(opcode == 0x04)
    {
        if(Register[src2] != 0) Register[dest] = Register[src1] / Register[src2];
    }
    else if(opcode == 0x05)
        Register[dest] = (int32_t)readData(Register[src1]);
    else if(opcode == 0x06)
        writeData(Register[dest], (uint32_t)Register[src1]);
    else if(opcode == 0x07)
        Register[dest] = Register[src1];
    else if(opcode == 0x09)
    {
        a = Register[src1]; b = src2; result = a + b;
        Register[dest] = result; setFlagsAdd(a, b, result);
    }
    else if(opcode == 0x0A)
    {
        a = Register[src1]; b = src2; result = a - b;
        Register[dest] = result; setFlagsSub(a, b, result);
    }
    else if(opcode == 0x0B)
        Register[dest] = Register[src1] * src2;
    else if(opcode == 0x0C)
    {
        if(src2 != 0) Register[dest] = Register[src1] / src2;
    }
    else if(opcode == 0x0D)
        Register[dest] = (int32_t)readData(src2);
    else if(opcode == 0x0E)
        writeData(dest, (uint32_t)Register[src1]);
    else if(opcode == 0x0F)
        Register[dest] = src2;

    /* Vector arithmetic with variable operands.
       src2 0..31  -> vector register.
       src2 0x80..0xFF -> integer register 0..127, broadcast to 8 lanes.
       This internal encoding removes ambiguity between v4 and x4 in the
       four-byte instruction format while keeping the specified opcodes. */
    else if(opcode >= 0x21 && opcode <= 0x23)
    {
        for(i = 0; i < 8; i++)
        {
            if(src2 & 0x80)
                scalar = Register[src2 & 0x7F];
            else
                scalar = VectorRegister[src2][i];

            if(opcode == 0x21)
                VectorRegister[dest][i] = VectorRegister[src1][i] + scalar;
            else if(opcode == 0x22)
                VectorRegister[dest][i] = VectorRegister[src1][i] - scalar;
            else
                VectorRegister[dest][i] = VectorRegister[src1][i] * scalar;
        }
    }

    /* Vector arithmetic with constant */
    else if(opcode >= 0x29 && opcode <= 0x2B)
    {
        for(i = 0; i < 8; i++)
        {
            if(opcode == 0x29)
                VectorRegister[dest][i] = VectorRegister[src1][i] + src2;
            else if(opcode == 0x2A)
                VectorRegister[dest][i] = VectorRegister[src1][i] - src2;
            else
                VectorRegister[dest][i] = VectorRegister[src1][i] * src2;
        }
    }

    /* Vector memory read: vD = [xA] */
    else if(opcode == 0x25)
    {
        address = Register[src2];
        for(i = 0; i < 8; i++)
        {
            VectorRegister[dest][i] = (int32_t)readData(address);
            address += 4;
        }
    }

    /* Vector memory read: vD = [constant] */
    else if(opcode == 0x2C)
    {
        address = src2;
        for(i = 0; i < 8; i++)
        {
            VectorRegister[dest][i] = (int32_t)readData(address);
            address += 4;
        }
    }

    /* Vector memory write: [xA] = vD */
    else if(opcode == 0x26)
    {
        address = Register[dest];
        for(i = 0; i < 8; i++)
        {
            writeData(address, (uint32_t)VectorRegister[src1][i]);
            address += 4;
        }
    }

    /* Vector memory write: [constant] = vD */
    else if(opcode == 0x2E)
    {
        address = dest;
        for(i = 0; i < 8; i++)
        {
            writeData(address, (uint32_t)VectorRegister[src1][i]);
            address += 4;
        }
    }

    /* Branches */
    else if(opcode >= 0x10 && opcode <= 0x1E)
    {
        int code = opcode - 0x10;
        take = 0;

        if(code == 0) take = Z;
        else if(code == 1) take = !Z;
        else if(code == 2) take = C;
        else if(code == 3) take = !C;
        else if(code == 4) take = N;
        else if(code == 5) take = !N;
        else if(code == 6) take = V;
        else if(code == 7) take = !V;
        else if(code == 8) take = C && !Z;
        else if(code == 9) take = !C || Z;
        else if(code == 10) take = (N == V);
        else if(code == 11) take = (N != V);
        else if(code == 12) take = (!Z && (N == V));
        else if(code == 13) take = (Z || (N != V));
        else if(code == 14) take = 1;

        if(take)
        {
            PC = PC + ((int8_t)src2 * 4);
            return;
        }
    }

    PC = PC + 4;
}
