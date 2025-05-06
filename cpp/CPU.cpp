#include <iomanip>
#include <iostream>
#include "CPU.h"
#include "globals.h"

#define VERBOSE 0

using namespace std;

CPU::CPU() {}

// Reset stateful modules
void CPU::init(string inst_file) {
	// Initialize the register file
	rf.init(false);
	// Load the instructions from the memory
	mem.load(inst_file);
	// Reset the program counter
	// 저장이 되어야 하는 storage element인 pc는 header file에 class variable로 생성
	// tick이 끝나도 값이 계속 저장될 수 있음(RF와 동일)
	PC = 0;

	// Set the debugging status
	status = CONTINUE;
}

// This is a cycle-accurate simulation
uint32_t CPU::tick() {
	// wire는 function 안에 variable 형태로 declare된 형태

	// These are just one of the implementations ...
	
	// wire for instruction
	uint32_t inst;

	// parsed & control signals (wire)
	CTRL::ParsedInst parsed_inst;
	CTRL::Controls controls;
	uint32_t ext_imm;

	// Default wires and control signals
	uint32_t rs_data, rt_data;
	uint32_t wr_addr;
	uint32_t wr_data;
	uint32_t operand1;
	uint32_t operand2;
	uint32_t alu_result;

	// PC_next
	uint32_t PC_next;

	// You can declare your own wires (if you want ...)
	uint32_t mem_data;

	// Access the instruction memory
	mem.imemAccess(PC, &inst);
	// 원치 않는 OF 값으로 접근했을 시 incorrect status 배출됨
	// last instruction 실행 시에도 0을 return 하도록 함
	if (status != CONTINUE) return 0;
	
	// Split the instruction & set the control signals
	ctrl.splitInst(inst, &parsed_inst);
	ctrl.controlSignal(parsed_inst.opcode, parsed_inst.funct, &controls);
	ctrl.signExtend(parsed_inst.immi, controls.SignExtend, &ext_imm);
	if (status != CONTINUE) return 0;

	rf.read(parsed_inst.rs, parsed_inst.rt, &rs_data, &rt_data);

	//R-type, I-type 별 operand 불러오는 방법 다르니 setting
	operand1 = rs_data;
	operand2 = (controls.ALUSrc) ? ext_imm : rt_data;

	alu.compute(operand1, operand2, parsed_inst.shamt, controls.ALUOp, &alu_result);
	if (status != CONTINUE) return 0;

	// MEM (+PC Update)
	mem.dmemAccess(alu_result, &mem_data, rt_data, controls.MemRead, controls.MemWrite);
	if (status != CONTINUE) return 0;
	
    if(controls.SavePC){
        wr_addr = 31;
        wr_data = PC + 4;
    } else{
        wr_addr = controls.RegDst ? parsed_inst.rd : parsed_inst.rt;
        wr_data = controls.MemtoReg ? mem_data : alu_result;
    }

    rf.write(wr_addr, wr_data, controls.RegWrite);

    if(controls.Jump){
        if(controls.JR){
            PC_next = rs_data;
        } else{
            PC_next = ((PC + 4) & 0xF0000000) | (parsed_inst.immj << 2);
        }
    }
	else if(controls.Branch){
        if(alu_result){
            PC_next = PC + 4 + (ext_imm << 2);
        } else{
            PC_next = PC + 4;
        }
    } else{
        PC_next = PC + 4;
    }

    PC = PC_next;

	// 추가로 중간중간 빠져있는 reg_read, reg_write 처리
	
	return 1;
	
}

