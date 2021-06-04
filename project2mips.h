#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#define MAX_MEM 16*1024*1024/sizeof(int)

//global variable
unsigned int PC, IR;
unsigned int reg[32];
unsigned int memory[MAX_MEM];

/* structure of instruction and unit used */

//control unit signals
struct controlUnit {
	int Branch, Jump, JumpReg;
	int ALUSrc, ALUOp1, ALUOp0, ALUOp;
	int MemRead, MemWrite, MemtoReg, RegDst, RegWrite;
};
typedef struct controlUnit CU;
CU control;

//three ALU unit according to the datapath
struct ALU {
	int zero;
	int opnd1;
	int opnd2;
};
typedef struct ALU ALU;
ALU PC_jump_or_branch, PC_default, main_ALU;

//five mux unit according to the datapath
struct MUX {
	int s;
	int opnd1;
	int opnd2;
};
typedef struct MUX MUX;
MUX mux_jump, mux_branch, mux_write_reg, mux_write_data, mux_main_ALU;

//instruction fields and extra information, i.e., count and name
struct instruction {
	//fields
	int opcode, rs, rd, rt, shamt, funct;
	int imm, addr;
	int pc_4_bit;
	
	//extra information
	int count, i_type_count, r_type_count, j_type_count, mem_instr_count, branch_taken;
	char* name;
};
typedef struct instruction instruction;
instruction instr;

/* ************************************** */

/* function call declaration */

//main function call
void load_instruction(FILE* fd);
void InstructionFetch();
int InstructionDecode();
int execution();
int MEM(int data, int addr, int w, int r);
int write_back();

//function call supporting execution
char send_control_signals();
int mux(MUX type, int s);
int ALU_calculator(ALU type);

int reg_operation(int reg1, int reg2);
int ALUOp_signal(int op1, int op0);
int ALU_zero_output(int result);
int ALU_branch();

//function call supporting write back phase
int pc_update();

//initializing for "ready to go" state
void initialize();
void init_control_unit();

//other function call
int check_sign_extend(unsigned int imm);
int bit(int number, int numBit, int start);
int swap(int a);
int check(int exp, const char* msg); 
int print_output();

/*
this is the longer version of send_control_signals()
it has the full list of mips core instruction set, 
however data of each instruction has not been completed yet

char send_control_signals() {
	if (IR == 0x0) {
		return 0;
	}
	instr.count++;
	char temp;
	switch (instr.opcode)
	{
	case 0x0:
		instr.r_type_count++;
		if (instr.funct == 0x8) {
			control.JumpReg = 1;
		}
		else {
			control.RegDst = 1;
			control.RegWrite = 1;
			control.ALUOp1 = 1;
		}
		switch (instr.funct)
		{
		case 0x20:
			instr.name = "add";
			break;
		case 0x21:
			instr.name = "addu";
			control.ALUOp1 = 0;
			break;
		case 0x24:
			instr.name = "and";
			break;
		case 0x08:
			instr.name = "jr";
			break;
		case 0x25:
			instr.name = "or";
			break;
		case 0x27:
			instr.name = "nor";
			break;
		case 0x2a:
			instr.name = "slt";
			break;
		case 0x2b:
			instr.name = "sltu";
			break;
		case 0x00:
			instr.name = "sll";
			break;
		case 0x02:
			instr.name = "srl";
			break;
		case 0x22:
			instr.name = "sub";
			break;
		case 0x23:
			instr.name = "subu";
			break;
		default:
			break;
		}
		temp = 'R';
		break;
	case 0x2:
		instr.name = "j";
		control.Jump = 1;
		temp = 'J';
		break;
	case 0x3:
		instr.name = "jal";
		instr.j_type_count++;
		control.RegWrite = 1;
		control.Jump = 1;
		temp = 'J';
		break;
	case 0x4:
		instr.name = "beq";
		control.Branch = 1;
		control.ALUOp0 = 1;
		temp = 'I';
		break;
	case 0x5:
		instr.name = "bne";
		control.Branch = 1;
		control.ALUOp0 = 1;
		temp = 'I';
		break;
	case 0x8:
		instr.name = "addi";
		control.RegWrite = 1;
		control.ALUSrc = 1;
		temp = 'I';
		break;
	case 0x9:
		instr.name = "addiu";
		control.RegWrite = 1;
		control.ALUSrc = 1;
		temp = 'I';
		break;
	case 0xa:
		instr.name = "slti";
		control.RegWrite = 1;
		control.ALUSrc = 1;
		control.ALUOp1 = 1;
		control.ALUOp0 = 1;
		instr.funct = 0x9;
		temp = 'I';
		break;
	case 0xb:
		instr.name = "sltiu";
		control.RegWrite = 1;
		control.ALUSrc = 1;
		control.ALUOp1 = 1;
		control.ALUOp0 = 1;
		instr.funct = 0x9;
		temp = 'I';
		break;
	case 0xc:
		instr.name = "andi";
		temp = 'I';
		break;
	case 0x23:
		instr.name = "lw";
		instr.mem_instr_count++;
		control.MemRead = 1;
		control.MemtoReg = 1;
		control.ALUSrc = 1;
		control.RegWrite = 1;
		temp = 'I';
		break;
	case 0x24:
		instr.name = "lbu";
		temp = 'I';
		break;
	case 0x25:
		instr.name = "lhu";
		temp = 'I';
		break;
	case 0x30:
		instr.name = "ll";
		temp = 'I';
		break;
	case 0xf:
		instr.name = "lui";
		temp = 'I';
		break;
	case 0xd:
		instr.name = "ori";
		temp = 'I';
		break;
	case 0x28:
		instr.name = "sb";
		temp = 'I';
		break;
	case 0x29:
		instr.name = "sh";
		temp = 'I';
		break;
	case 0x38:
		instr.name = "sc";
		temp = 'I';
		break;
	case 0x2b:
		instr.name = "sw";
		instr.mem_instr_count++;
		control.MemWrite = 1;
		control.ALUSrc = 1;
		temp = 'I';
		break;
	default:
		check((-1), "opcode does not exist");
		break;
	}
	control.ALUOp = ALUOp_signal(control.ALUOp1, control.ALUOp0);

	if (temp == 'I') {
		instr.i_type_count++;
	}
	else if (temp == 'R') {
		instr.r_type_count++;
	}
	else if (temp == 'J') {
		instr.j_type_count++;
	}
	else {
		return;
	}
	return temp;
}
*/

/* ************************* */