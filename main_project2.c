#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "project2mips.h"

//#define MEMORY_STRUCT
//#define INSTR_DETAILS

#define filename "gcd.bin"

int main()
{
	int ret;
	FILE* file;

	//open file and load to memory
	if ((file = fopen(filename, "rb")) == NULL) {
		perror("cannot open file");
		exit(1);
	}
	load_instruction(file);
	fclose(file); //close file

	//initialize to start
	initialize();

	//start loop
	for (;;) {
		InstructionFetch();
		InstructionDecode();
		execution();
		write_back();
	}

	return 0;
}

//load instruction from file to memory
void load_instruction(FILE* fd) {
	size_t ret = 0;
	int mips_val, i = 0;
	int mem_val;

	do
	{
		mips_val = 0;
		ret = fread(&mips_val, 1, 4, fd);
		mem_val = swap(mips_val);
		memory[i] = mem_val;

#ifdef MEMORY_STRUCT
		printf("(%d) load Mem[%x] pa: 0x%x val: 0x%x\n", (int)ret, i, &memory[i], memory[i]);
#endif
		i++;
	} while (ret == 4);
}

//instruction fetch phase
void InstructionFetch() {
	check(PC, "program end"); //if pc == 0xFFFFFFFF, end program

	memset(&IR, 0, sizeof(IR));
	IR = memory[PC / 4];
	printf("PC: %x\t\t --> FET: %08x\n", PC, IR);
}

//instruction decode phase
int InstructionDecode() {
	//assign instruction fields
	if (IR == 0x0) {
		return 0;
	}
	instr.opcode = bit(IR, 6, 27);
	instr.rs = bit(IR, 5, 22);
	instr.rt = bit(IR, 5, 17);
	instr.rd = bit(IR, 5, 12);
	instr.shamt = bit(IR, 5, 7);
	instr.funct = bit(IR, 6, 1);
	instr.imm = bit(IR, 16, 1);
	instr.addr = bit(IR, 26, 1);
	instr.pc_4_bit = bit(PC, 4, 29);
	return 0;
}

//execution phase, in the report it is called as EX
int execution() {
	char instr_type;
	init_control_unit();
	instr_type = send_control_signals();

#ifdef INSTR_DETAILS
	switch (instr_type)
	{
	case 'R':
		printf("op:0x%x\t rs:%x\t rt:%x\t rd:%x\t shamt:0x%x\t funct:0x%x\n", instr.opcode, instr.rs, instr.rt, instr.rd, instr.shamt, instr.funct);
		break;
	case 'I':
		printf("op:0x%x\t rs:%x\t rt:%x\t imm:0x%x\n", instr.opcode, instr.rs, instr.rt, instr.imm);
		break;
	case 'J':
		printf("op:0x%x\t addr:0x%x\n", instr.opcode, instr.addr);
		break;
	default:
		printf("error\n");
		break;
	}
#endif

	unsigned int opcode, rs, rd, rt, imm, shamt, funct, imm_j;
	opcode = instr.opcode;
	rs = instr.rs;
	rd = instr.rd;
	rt = instr.rt;
	shamt = instr.shamt;
	funct = instr.funct;
	imm = instr.imm;
	imm_j = instr.addr;

	int wdata_data_mem, addr_data_mem, z_ext_imm, * b_addr;
	unsigned int* j_addr;

	imm = check_sign_extend(imm);
	j_addr = (unsigned int)(instr.pc_4_bit | (imm_j << 2));
	b_addr = (short)((int32_t)imm << 2);
	
	//operand inputs for mux and ALU
	mux_write_reg.opnd1 = rd;
	mux_write_reg.opnd2 = rt;
	PC_jump_or_branch.opnd1 = PC + 4;
	PC_jump_or_branch.opnd2 = b_addr;
	mux_main_ALU.opnd1 = (int32_t)imm;

	//both input for Data Memory
	wdata_data_mem = reg_operation(instr.rs, instr.rt);
	addr_data_mem = ALU_calculator(main_ALU);

	ALU_zero_output(addr_data_mem);
	mux_write_data.opnd2 = addr_data_mem;

	//mux that control branch address input, and branch control signal
	mux_branch.opnd1 = ALU_branch();
	mux_branch.opnd2 = PC + 4;
	if (instr.opcode == 0x5) { //if bne
		control.Branch = control.Branch & !main_ALU.zero;
	}
	else {
		control.Branch = control.Branch & main_ALU.zero;
	}

	//mux input for jump address
	mux_jump.opnd1 = j_addr;
	mux_jump.opnd2 = mux(mux_branch, control.Branch);

	//Memory write stage
	MEM(wdata_data_mem, addr_data_mem, control.MemWrite, control.MemRead);
	return 0;
}

int ALU_zero_output(int result) {
	if (result == 0) {
		return main_ALU.zero = 1;
	}
	else {
		return main_ALU.zero = 0;
	}
}

int check_sign_extend(unsigned int imm) {
	if ((instr.opcode == 0xc) | (instr.opcode == 0xd)) {
		imm = (unsigned int)imm;
	}
	else {
		if (imm << 15) {
			imm = (short)imm;
		}
		imm = (unsigned int)imm;
	}
	return imm;
}

int reg_operation(int reg1, int reg2) {
	//ALU main
	mux_main_ALU.opnd2 = reg[reg2];
	main_ALU.opnd1 = reg[reg1];
	main_ALU.opnd2 = mux(mux_main_ALU, control.ALUSrc);
	return reg2;
}

int MEM(int data, int addr, int w, int r) {
	if (w == 1) {
		printf("memory updated\t --> 0x%x: 0x%x\n", addr >> 2, reg[data]);
		memory[addr >> 2] = reg[data];
	} else if (r == 1) {
		mux_write_data.opnd1 = memory[addr/sizeof(unsigned int)];
	}
	else {
		mux_write_data.opnd1 = 0;
	}
	return 0;
}

int write_back() {
	if (IR == 0x0) {
		PC = PC + 4;
		printf("PC updated\t --> PC: %x\n\n", PC);
		return 0;
	}

	int wreg_register = mux(mux_write_reg, control.RegDst);
	int wdata_register = mux(mux_write_data, control.MemtoReg);

	if (instr.opcode == 0x3) { //jal
		wreg_register = 31;
		wdata_register = PC + 8;
	}
	if (control.RegWrite == 1) {
		reg[wreg_register] = wdata_register;
		printf("register updated --> $%d: 0x%x\n", wreg_register, wdata_register);
	}
	//else skip/do nothing
	pc_update();

	//if the complete branch address (branch addr + PC + 4) is the current PC
	if (PC == mux_branch.opnd1) {
		instr.branch_taken++;
	}
	return 0;
}

int pc_update() {
	PC = mux(mux_jump, control.Jump);
	if (control.JumpReg == 1) { //jr exception
		PC = reg[instr.rs];
	}
	printf("PC updated\t --> PC: %x\n\n", PC);
	return 0;
}

void initialize() {
	memset(reg, 0, sizeof(reg));
	PC = 0;
	reg[31] = 0xFFFFFFFF; //$31 = -1
	reg[29] = (unsigned int*)0x1000000;
	instr.count = 0;
	instr.i_type_count = 0;
	instr.j_type_count = 0;
	instr.r_type_count = 0;
	instr.branch_taken = 0;
	instr.mem_instr_count = 0;
	return;
}

void init_control_unit() {
	control.RegDst = 0;
	control.Jump = 0;
	control.Branch = 0;
	control.MemRead = 0;
	control.MemtoReg = 0;
	control.MemWrite = 0;
	control.ALUSrc = 0;
	control.RegWrite = 0;
	control.ALUOp1 = 0;
	control.ALUOp0 = 0;
	control.JumpReg = 0;
	control.ALUOp = 0;
	return;
}

//mux --> s = 1, output = operand1
int mux(MUX type, int s) {
	if (s == 1)
		return type.opnd1;
	else //s == 0
		return type.opnd2;
}

//the ALU control program that give out ALU signal
int ALUOp_signal(int op1, int op0) {
	int f[5];
	for (int i = 1; i <= 4; i++) {
		f[i] = bit(instr.funct, 1, i);
	}
	int out = ((f[1] | f[4]) & op1);
	int out1 = (!(f[2]) | !op1);
	int out2 = ((f[3] & op1) | op0);
	return ((out2 << 2) | (out1 << 1) | out);
}

//only perform add
int ALU_branch() {
	control.ALUOp = 0x2;
	return ALU_calculator(PC_jump_or_branch);
}

//main ALU
int ALU_calculator(ALU type) {
	if (IR == 0x0) {
		return 0;
	}
	int result = 0;
	switch (control.ALUOp)
	{
	case 0:
		result = type.opnd1 & type.opnd2;
		//printf("and:0x%x & 0x%x\n", type.opnd1, type.opnd2);
		break;
	case 1:
		result = type.opnd1 | type.opnd2;
		//printf("or:0x%x | 0x%x\n", type.opnd1, type.opnd2);
		break;
	case 2:
	case 3:
		result = type.opnd1 + type.opnd2;
		//printf("0x%x : 0x%x + 0x%x\n", result, type.opnd1, type.opnd2);
		break;
	case 5:
	case 6:
		result = type.opnd1 - type.opnd2;
		//printf("0x%x : 0x%x - 0x%x\n", result, type.opnd1, type.opnd2);
		break;
	case 7:
		result = (type.opnd1 < type.opnd2) ? 1 : 0;
		//printf("slt:0x%x < 0x%x? 1 : 0\n", type.opnd1, type.opnd2);
		break;
	default:
		perror("Error in ALU calculation");
		result = 0;
		break;
	}
	return result;
}

//number to bit converter
int bit(int number, int numBit, int start)
{
	return (((1 << numBit) - 1) & (number >> (start - 1)));
}

//to deal with the big-little endian
int swap(int a) {
	a = ((a & (0x0000FFFF)) << 16) | ((a & (0xFFFF0000)) >> 16);
	a = ((a & (0x00FF00FF)) << 8) | ((a & (0xFF00FF00)) >> 8);
	return a;
}

//error check function
int check(int exp, const char* msg) {
	if (exp == (-1)) {
		print_output();
		perror(msg);
		exit(1);
	}
	return exp;
}

int print_output() {
	printf("final result:\t\t $v0: %d\n", reg[2]);
	printf("instructions executed:\nTotal\t\t\t --> %d instructions\n", instr.count);
	printf("R-type instruction\t --> %d instructions\n", instr.r_type_count);
	printf("I-type instruction\t --> %d instructions\n", instr.i_type_count);
	printf("J-type instruction\t --> %d instructions\n", instr.j_type_count);
	printf("Memory instruction\t --> %d instructions\n", instr.mem_instr_count);
	printf("Branch Taken\t\t --> %d instructions\n\n", instr.branch_taken);
	return 0;
}

/* 
the control unit program that send out control signals 
this is not completed (not all of the mips core instruction)
*/
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
			control.Jump = 1;
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
	case 0x23:
		instr.name = "lw";
		instr.mem_instr_count++;
		control.MemRead = 1;
		control.MemtoReg = 1;
		control.ALUSrc = 1;
		control.RegWrite = 1;
		temp = 'I';
		break;
	case 0xd:
		instr.name = "ori";
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
		check((-1), "opcode does not exist/yet for this program");
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
