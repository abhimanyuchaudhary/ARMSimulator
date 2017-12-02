
/* 

The project is developed as part of Computer Architecture class
Project Name: Functional Simulator for subset of ARM Processor

Developer's Name:
Developer's Email id:
Date: 

*/


/* myARMSim.cpp
   Purpose of this file: implementation file for myARMSim
*/

#include "myARMSim.h"
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>

using namespace std;
//Number of registers and size of memory
const int kRegisters = 16, kMemory = 4000;
const int PC = 15;
const int AND = 0, XOR = 1, SUB = 2, ADD = 4, OR = 12, MUL = 16, MULACC = 17, LDR = 18, STR = 19, nop = 42, inp = 20;

//Register file
static unsigned int R[kRegisters];
//flags
static bool N, C, V, Z;
//memory
static unsigned char MEM[kMemory];

//intermediate datapath and control path signals
static unsigned int ins;
static unsigned int operand1;
static unsigned int operand2;
static unsigned int operand3; //For Multiply and Accumulate
static unsigned int destination;
static unsigned int opcode;
static unsigned int ALUop;
static unsigned int shift;
static unsigned int ans;
static bool I, S, P, U, B, L, W;
static bool wb;

void run_armsim() {
    while(true) {
        fetch();
        decode();
        execute();
        mem();
        write_back();
    }
}

// it is used to set the reset values
//reset all registers and memory content to 0
void reset_proc() {
    memset(R, 0, sizeof(R));
    memset(MEM, 0, sizeof(MEM));
}

//load_program_memory reads the input memory, and populates the instruction 
// memory
void load_program_memory(char *file_name) {
    FILE *fp;
    unsigned int address, instruction;
    fp = fopen(file_name, "r");
    if(fp == NULL) {
        printf("Error opening input mem file\n");
        exit(1);
    }

    while(fscanf(fp, "%x %x", &address, &instruction) != EOF) {
        write_word(MEM, address, instruction);
    }
    fclose(fp);
}

//writes the data memory in "data_out.mem" file
void write_data_memory() {
    FILE *fp;
    unsigned int i;
    fp = fopen("data_out.mem", "w");
    if(fp == NULL) {
        printf("Error opening dataout.mem file for writing\n");
        return;
    }

    for(i = 0; i < kMemory; i += 4){
        fprintf(fp, "%x %x\n", i, read_word(MEM, i));
    }
    fclose(fp);
}

//should be called when instruction is swi_exit
void swi_exit() {
    write_data_memory();
    exit(0);
}

//should be called when instruction is swi_input
void swi_input(){
    if (R[0] == 0) {
        cin >> ans;
        wb = true;
        opcode = inp;
        destination = 0;
    } 
}

//should be called when instruction is swi_output
void swi_output(){
    if (R[0] == 1) {
        cout << R[1];
    }
}


//reads from the instruction memory and updates the instruction register
void fetch() {
    ins = read_word(MEM, R[PC]);
    printf("FETCH: Fetch instruction %x from address %x", ins, R[PC]);
    // cout << "FETCH: Fetch instruction " << ins << " from address " << R[PC] << endl; // convert to hex 
    R[PC] += 4;
}

//reads the instruction register, reads operand1, operand2 from register file, decides the operation to be performed in execute stage
void decode() {
    //First extracts the conditional flags from the instruction
    unsigned int cond = (ins >> 28);
    wb     = true;
    opcode = nop;
    ALUop  = nop;
    shift  = 0;

    cout << "\nDECODE: Operation is ";

    //If instruction is an swi instruction
    if (ins == 0xEF000011) {
        cout << "SWI Exit" << endl;
        swi_exit();
    } else if (ins == 0xEF00006C) {
        cout << "SWI Input" << endl;
        swi_input();
        return;
    } else if (ins == 0xEF00006B) {
        cout << "SWI Output" << endl;
        swi_output();
        return;
    }

    //Checks the flags and determines whether instruction will execute
    //At least this looks better than 13 else if statements
    if  ((cond == 0 and !Z)
      or (cond == 1 and Z)
      or (cond == 2 and !C)
      or (cond == 3 and C)
      or (cond == 4 and !N)
      or (cond == 5 and N)
      or (cond == 6 and !V)
      or (cond == 7 and V)
      or (cond == 8 and !(C and !Z))
      or (cond == 9 and !(!C or Z))
      or (cond == 10 and N != V)
      or (cond == 11 and N == V)
      or (cond == 12 and !(!Z and (N == V)))
      or (cond == 13 and !(Z or (N != V)))) {
        cout << "a nop since condition flags don't match";
        opcode = ALUop = nop;
        return;
    }

    if (get_bits(ins, 25, 27) == 5) {
        //If instruction is a branch instruction
        //Sign extend a.k.a. magic
        wb = false;
        int offset = ((ins << 8) >> 6);
        if (offset & (1 << 25)) {
            offset |= ((~0) << 26);
        }
        R[PC] += offset + 4;
        cout << "Branch";
    } else if (get_bits(ins, 22, 27) == 0 and get_bits(ins, 4, 7) == 9) {
        //Multiply instruction (MUL or MULACC)
        bool A = ins & (1 << 21);
        S = ins & (1 << 20);
        opcode = ALUop = (A ? MULACC : MUL);
        operand1    = R[get_bits(ins, 8, 11)];
        operand2    = R[get_bits(ins, 0, 3)];
        operand3    = R[get_bits(ins, 12, 15)]; //for accumulate
        destination = get_bits(ins, 16, 19); //destination
        cout << "MUL";
        cout << ", Operand 1: " << operand1 << ", Operand 2: " << operand2 << ", Destination Register: R" << destination;
    } else if (get_bits(ins, 26, 27) == 1) {
        //Load or Store instruction
        I = (ins & (1 << 25));
        P = (ins & (1 << 24));
        U = (ins & (1 << 23));
        W = (ins & (1 << 21));
        L = (ins & (1 << 20));
        destination = get_bits(ins, 12, 15);
        operand1    = R[get_bits(ins, 16, 19)];
        if (U) {
            ALUop = ADD;
        } else {
            ALUop = SUB;
        }

        if (I) {
            shift    = get_bits(ins, 4, 11);
            operand2 = R[get_bits(ins, 0, 3)];
        } else {
            shift    = 0;
            operand2 = get_bits(ins, 0, 11);
        }

        if (L) {
            opcode = LDR;
            cout << "Load";
        } else {
            opcode = STR;
            wb = false;
            cout << "Store";
        }
    } else if (get_bits(ins, 26, 27) == 0) {
        //Arithmetic instruction
        I = (ins & (1 << 25));
        S = (ins & (1 << 20));
        ALUop       = get_bits(ins, 21, 24);
        opcode      = ALUop;
        operand1    = R[get_bits(ins, 16, 19)];
        destination = get_bits(ins, 12, 15);
        if (I) {
            operand2 = get_bits(ins, 0, 7);
            shift = (ins >> 8) & (0xF);
        } else {
            operand2 = R[get_bits(ins, 0, 3)];
            shift = get_bits(ins, 4, 11);
        }

        if (ALUop == AND) {
            cout << "AND";
        } else if (ALUop == XOR) {
            cout << "XOR";
        } else if (ALUop == SUB) {
            cout << "SUB";
        } else if (ALUop == ADD) {
            cout << "ADD";
        } else if (ALUop == 8) {
            cout << "TST";
            ALUop = AND;
            wb = false;
        } else if (ALUop == 9) {
            cout << "TEQ";
            ALUop = XOR;
            wb = false;
        } else if (ALUop == 10) {
            cout << "CMP";
            ALUop = SUB;
            wb = false;
        } else if (ALUop == 11) {
            cout << "CMN";
            ALUop = ADD;
            wb = false;
        } else if (ALUop == 12) {
            cout << "OR";
        } else if (ALUop == 13) {
            cout << "MOV";
        } else if (ALUop == 15) {
            cout << "MVN";
        }
        cout << ", Operand 1: " << operand1 << ", Operand 2: " << operand2 << ", Destination Register: R" << destination;
    } else {
        cout << "Instruction not recognized/supported";
    }
}

//executes the ALU operation based on ALUop
void execute() {
    cout << "\nEXECUTE: ";
    if (ALUop == nop) {
        return;
    }

    //MUL or MULACC instruction
    if (ALUop == MUL) {
        ans = operand1 * operand2;
        cout << "MUL " << operand1 << " and " << operand2;
    } else if (ALUop == MULACC) {
        ans = operand1 * operand2 + operand3;
        cout << "MUL " << operand1 << " and " << operand2 << " and ADD " << operand3;
    } else {
        //Applies the appropritate shift to operand 2
        int shiftType = (shift >> 1) & 3;
        shift >>= 3;
        if (shiftType == 0) {
            if (shift and S) {
                C = (operand2 & (1 << (32 - shift)));
            }
            operand2 <<= shift;
            if (shift) {
                cout << "Left shift " << operand2 << " by " << shift << ", ";
            }
        } else if (shiftType == 1) {
            if (shift == 0) {
                shift = 32;
            }
            operand2 >>= shift;
            if (shift) {
                cout << "Right shift " << operand2 << " by " << shift << ", ";
            }
        }

        //Performs the actual ALU operation
        if (ALUop == 0) {
            ans = operand1 & operand2;
            cout << "AND " << operand1 << " and ";
        } else if (ALUop == 1) {
            ans = operand1 ^ operand2;
            cout << "XOR " << operand1 << " and ";
        } else if (ALUop == 2) {
            ans = operand1 - operand2;
            cout << "SUB " << operand1 << " and ";
        } else if (ALUop == 4) {
            ans = operand1 + operand2;
            cout << "ADD " << operand1 << " and ";
        } else if (ALUop == 12) {
            ans = operand1 | operand2;
            cout << "OR " << operand1 << " and ";
        } else if (ALUop == 13) {
            ans = operand2;
            cout << "MOV ";
        } else if (ALUop == 15) {
            ans = ~operand2;
            cout << "MVN ";
        }

        cout << operand2;
    }

    //Updates conditions flags if S was 1
    if (S) {
        Z = (ans == 0);
        N = (ans & (1 << 31));
    }
}

//perform the memory operation
void mem() {
    cout << "\nMEMORY: ";
    if (opcode == nop) {
        return;
    } else if (opcode == LDR) {
        ans = read_word(MEM, ans);
        printf("Load from address %x to register R%d", ans, destination);
    } else if (opcode == STR) {
        write_word(MEM, ans, R[destination]);
        printf("Store to address %x value %d", ans, R[destination]);
    }
}

//writes the results back to register file
void write_back() {
    cout << "\nWRITE BACK: ";
    if (opcode == nop or !wb) {
    } else {
        cout << "Write " << ans << " to R" << destination;
        R[destination] = ans;
    }
    cout << "\n\n";
}

//read one word from memory
int read_word(unsigned char *mem, unsigned int address) {
    int *data = (int*) (mem + address);
    return *data;
}

//write one word to memory
void write_word(unsigned char *mem, unsigned int address, unsigned int data) {
    int *data_p = (int*) (mem + address);
    *data_p = data;
}

//Get bits l to r (l <= r) of num
unsigned int get_bits(unsigned int num, int l, int r) {
    return (num << (31 - r)) >> (l - r + 31);
}