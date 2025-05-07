#include <iomanip>
#include <iostream>
#include "CPU.h"
#include "globals.h"

#define VERBOSE 0

using namespace std;

CPU::CPU() {
	state = STATE_IF;
}

// Reset stateful modules
void CPU::init(string inst_file) {
	// Initialize the register file
	rf.init(false);
	// Load the instructions from the memory
	mem.load(inst_file);
	// Reset the program counter
	PC = 0;

	IR = 0;
	MDR = 0;
	A = 0;
	B = 0;
	ALUOut = 0;

	state = STATE_IF;

	// Set the debugging status
	status = CONTINUE;
}

// This is a cycle-accurate simulation
uint32_t CPU::tick() {
	CTRL::ParsedInst parsed_inst;
	CTRL::Controls controls;

	if (state == STATE_IF) {
		ctrl.splitInst(0, &parsed_inst);
	} else {
		ctrl.splitInst(IR, &parsed_inst);
	}

	ctrl.controlSignal(parsed_inst.opcode, parsed_inst.funct, state, &controls);

	uint32_t ext_imm;
	ctrl.signExtend(parsed_inst.immi, controls.SignExtend, &ext_imm);

	uint32_t alu_in1 = 0;
	uint32_t alu_in2 = 0;

	if (controls.ALUSrcA == 0) {
	alu_in1 = PC;
	} else {
	alu_in1 = A;
	}

	switch (controls.ALUSrcB) {
		case 0: alu_in2 = B; break;
		case 1: alu_in2 = 4; break;
		case 2: alu_in2 = ext_imm; break;
		case 3: alu_in2 = (ext_imm << 2) & 0xFFFFFFFC; break;
	}

	uint32_t alu_result;
	alu.compute(alu_in1, alu_in2, parsed_inst.shamt, controls.ALUOp, &alu_result);

	uint32_t mem_addr = controls.IorD ? ALUOut : PC;

	uint32_t mem_data;
	mem.memAccess(mem_addr, &mem_data, B, controls.MemRead, controls.MemWrite);

	switch (state) {
		case STATE_IF:
			if (controls.IRWrite) {
				IR = mem_data;
			}

			if (controls.PCWrite) {
				PC = alu_result;
			}

			state = STATE_ID;
			break;

		case STATE_ID:
			rf.read(parsed_inst.rs, parsed_inst.rt, &A, &B);

			if ((parsed_inst.opcode == OP_J) || (parsed_inst.opcode == OP_JAL) || (parsed_inst.opcode == OP_RTYPE && parsed_inst.funct == FUNCT_JR)) {
				if (parsed_inst.opcode == OP_JAL) {
					rf.write(31, PC, 1);
				}

				if (parsed_inst.opcode == OP_J || parsed_inst.opcode == OP_JAL) {
					PC = ((PC & 0xF0000000) | (parsed_inst.immj << 2));
				} else {
					PC = A;
				}

				state = STATE_IF;
			} else {
				state = STATE_EX;
			}
			break;

		case STATE_EX:
			if (parsed_inst.opcode == OP_BEQ || parsed_inst.opcode == OP_BNE) {
				uint32_t cmp_result;
				if (parsed_inst.opcode == OP_BEQ) {
					alu.compute(A, B, 0, ALU_EQ, &cmp_result);
				} else {
					alu.compute(A, B, 0, ALU_NEQ, &cmp_result);
				}

				uint32_t branch_target = PC + ((ext_imm << 2) & 0xFFFFFFFC);

				if (cmp_result) {
					PC = branch_target;
				}

				state = STATE_IF;
			} else {
				ALUOut = alu_result;
					
				if (parsed_inst.opcode == OP_LW || parsed_inst.opcode == OP_SW) {
					state = STATE_MEM;
				} else {
					state = STATE_WB;
				}
			}
			break;

		case STATE_MEM:
			MDR = mem_data;
				
			if (parsed_inst.opcode == OP_LW) {
				state = STATE_WB;
			} else {
				state = STATE_IF;
			}
			break;

		case STATE_WB:
			if (controls.RegWrite) {
				uint32_t write_reg = controls.RegDst ? parsed_inst.rd : parsed_inst.rt;
				uint32_t write_data = controls.MemtoReg ? MDR : ALUOut;
				rf.write(write_reg, write_data, 1);
			}
			
			state = STATE_IF;
			break;
	}

	if (IR == 0 && state == STATE_ID) {
		status = TERMINATE;
	}
		
	return 1;
}