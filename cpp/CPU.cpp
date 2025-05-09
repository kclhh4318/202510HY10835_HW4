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

	uint32_t alu_result;
	
	uint32_t mem_addr;

	uint32_t mem_data;

	switch (state) {
		case STATE_IF:
			mem_addr = controls.IorD ? ALUOut : PC;
			mem.memAccess(mem_addr, &mem_data, B, controls.MemRead, controls.MemWrite);

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
			alu.compute(alu_in1, alu_in2, parsed_inst.shamt, controls.ALUOp, &alu_result);

			if (controls.IRWrite) {
				IR = mem_data;
			}

			if (controls.PCWrite) {
				switch(controls.PCSource){
					case 0: PC = alu_result; break;
					case 1: PC = ALUOut; break;
					case 2: PC = ((PC & 0xF0000000) | (parsed_inst.immj << 2)); break;
					case 3: PC = A; break;
				}
			}

			state = STATE_ID;
			break;

		case STATE_ID:
			rf.read(parsed_inst.rs, parsed_inst.rt, &A, &B);

			if(controls.Branch) {
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
				alu.compute(alu_in1, alu_in2, parsed_inst.shamt, controls.ALUOp, &alu_result);
				ALUOut = alu_result;
			}

			if (controls.Jump) {
				if (controls.SavePC) {
					temp = PC;
				} 
				switch(controls.PCSource){
					case 0: PC = alu_result; break;
					case 1: PC = ALUOut; break;
					case 2: PC = ((PC & 0xF0000000) | (parsed_inst.immj << 2)); break;
					case 3: PC = A; break;
				}

				state = STATE_IF;
				if (controls.SavePC){
					state = STATE_WB;
				}
			} else {
				state = STATE_EX;
			}
			break;

		case STATE_EX:
			if (controls.Branch) {
				uint32_t cmp_result;
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
				alu.compute(alu_in1, alu_in2, 0, controls.ALUOp, &cmp_result); 

				if (cmp_result && controls.PCWriteCond) {
					switch(controls.PCSource){
						case 0: PC = alu_result; break;
						case 1: PC = ALUOut; break;
						case 2: PC = ((PC & 0xF0000000) | (parsed_inst.immj << 2)); break;
						case 3: PC = A; break;
					}
				}

				state = STATE_IF;
			} else {
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
				alu.compute(alu_in1, alu_in2, parsed_inst.shamt, controls.ALUOp, &alu_result);
				ALUOut = alu_result;
	
				if (controls.MemRead || controls.MemWrite) {
					state = STATE_MEM;
				} else {
					state = STATE_WB;
				}
			}
			break;

		case STATE_MEM:
			mem_addr = controls.IorD ? ALUOut : PC;
			mem.memAccess(mem_addr, &mem_data, B, controls.MemRead, controls.MemWrite); 
			MDR = mem_data;
				
			if (controls.IorD && controls.MemRead) {
				state = STATE_WB;
			} else {
				state = STATE_IF;
			}
			break;

		case STATE_WB:
			if (controls.RegWrite) {
				uint32_t write_reg;
				uint32_t write_data;
				switch(controls.RegDst){
					case 0: write_reg = parsed_inst.rt; break;
					case 1: write_reg = parsed_inst.rd; break;
					case 2: write_reg = 31; break;
				}
				switch(controls.MemtoReg){
					case 0: write_data = ALUOut; break;
					case 1: write_data = MDR; break;
					case 2: write_data = temp; break;
				}
				
				rf.write(write_reg, write_data, controls.RegWrite);
			}
			
			state = STATE_IF;
			break;
	}

	if (IR == 0 && state == STATE_ID) {
		status = TERMINATE;
	}
		
	return 1;
}