#include <iostream>
#include "CTRL.h"
#include "ALU.h"
#include "globals.h"

// 1. controlsignal을 찾아내는 것
// 2. instruction split
// 3. sign-extension 수행

CTRL::CTRL() {}

void CTRL::controlSignal(uint32_t opcode, uint32_t funct, uint32_t state, Controls *controls) {
	// FILLME
	//default ctrl sig 설정: 추후 I-type, J-type을 편히 바꾸기 위함
	controls -> RegDst = 1;
	controls -> Jump = 0;
	controls -> Branch = 0;
	controls -> JR = 0;
	controls -> MemRead = 0;
	controls -> MemtoReg = 0;
	controls -> MemWrite = 0;
	controls -> ALUSrc = 0;
	controls -> SignExtend = 1;
	controls -> RegWrite = 0;
	controls -> SavePC = 0;
	controls -> ALUOp = 0;

	controls -> IorD = 0;
	controls -> PCWrite = 0;
	controls -> PCWriteCond = 0;
	controls -> IRWrite = 0;
	controls -> ALUSrcA = 0;
	controls -> ALUSrcB = 0;
	controls -> PCSource = 0;

	switch (state) {
		case STATE_IF:
			controls -> MemRead = 1;
			controls -> IRWrite = 1;
			controls -> ALUSrcA = 0;
			controls -> ALUSrcB = 1;
			controls -> ALUOp = ALU_ADDU;
			controls -> PCWrite = 1;
			controls -> PCSource = 0;
			break;

		case STATE_ID:
			controls -> ALUSrcA = 0;
			controls -> ALUSrcB = 3;
			controls -> ALUOp = ALU_ADDU;

			if (opcode == OP_J) {
				controls -> PCWrite = 1;
				controls -> PCSource = 2;
				controls -> Jump = 1;
			} else if (opcode == OP_JAL) {
				controls -> PCWrite = 1;
				controls -> PCSource = 2;
				controls -> SavePC = 1;
				controls -> RegWrite = 1;
				controls -> Jump = 1;
			} else if (opcode == OP_RTYPE && funct == FUNCT_JR) {
				controls -> PCWrite = 1;
				controls -> PCSource = 3;
				controls -> JR = 1;
				controls -> Jump = 1;
			}
			break;
			
		case STATE_EX:
			controls -> ALUSrcA = 1;

			if (opcode == OP_RTYPE) {
				controls -> ALUSrcB = 0;

				switch (funct) {
					case FUNCT_ADDU: controls -> ALUOp = ALU_ADDU; break;
					case FUNCT_SUBU: controls -> ALUOp = ALU_SUBU; break;
					case FUNCT_AND: controls -> ALUOp = ALU_AND; break;
					case FUNCT_OR: controls -> ALUOp = ALU_OR; break;
					case FUNCT_SLL: controls -> ALUOp = ALU_SLL; break;
					case FUNCT_SRL: controls -> ALUOp = ALU_SRL; break;
					case FUNCT_SRA: controls -> ALUOp = ALU_SRA; break;
					case FUNCT_SLT: controls -> ALUOp = ALU_SLT; break;
					case FUNCT_SLTU: controls -> ALUOp = ALU_SLTU; break;
					case FUNCT_XOR: controls -> ALUOp = ALU_XOR; break;
					case FUNCT_NOR: controls -> ALUOp = ALU_NOR; break;
				}
			} else if (opcode == OP_BEQ || opcode == OP_BNE) {
				controls -> ALUSrcA = 0;
				controls -> ALUSrcB = 3;
				controls -> ALUOp = ALU_ADDU;
				controls -> Branch = 1;
				controls -> PCWriteCond = 1;
				controls -> PCSource = 1;
				controls -> RegWrite = 0;

				if (opcode == OP_BEQ) {
					controls -> ALUOp = ALU_EQ;
				} else {
					controls -> ALUOp = ALU_NEQ;
				}
			} else {
				controls -> ALUSrcB = 2;

				switch (opcode) {
					case OP_ADDIU: controls -> ALUOp = ALU_ADDU; controls -> SignExtend = 1; break;
					case OP_SLTI: controls -> ALUOp = ALU_SLT; controls -> SignExtend = 1; break;
					case OP_SLTIU: controls -> ALUOp = ALU_SLTU; controls -> SignExtend = 1; break;
					case OP_ANDI: controls -> ALUOp = ALU_AND; controls -> SignExtend = 0; break;
					case OP_ORI: controls -> ALUOp = ALU_OR; controls -> SignExtend = 0; break;
					case OP_XORI: controls -> ALUOp = ALU_XOR; controls -> SignExtend = 0; break;
					case OP_LUI: controls -> ALUOp = ALU_LUI; controls -> SignExtend = 0; break;
					case OP_LW:
					case OP_SW:
						controls -> ALUOp = ALU_ADDU;
						controls -> SignExtend = 1;
						break;
				}
			}
			break;

		case STATE_MEM:
			controls -> IorD = 1;

			if (opcode == OP_LW) {
				controls -> MemRead = 1;
			} else if (opcode == OP_SW) {
				controls -> MemWrite = 1;
			}
			break;
			
		case STATE_WB:
			controls -> RegWrite = 1;

			if (opcode == OP_RTYPE) {
				controls -> RegDst = 1;
				controls -> MemtoReg = 0;
			} else if (opcode == OP_LW) {
				controls -> RegDst = 0;
				controls -> MemtoReg = 1;
			} else {
				controls -> RegDst = 0;
				controls -> MemtoReg = 0;
			}
			break;
	}
}

void CTRL::splitInst(uint32_t inst, ParsedInst *parsed_inst) {
	// FILLME
	parsed_inst -> opcode = (inst >> 26) & 0x3F;
	parsed_inst -> rs = (inst >> 21) & 0x1F;
	parsed_inst -> rt = (inst >> 16) & 0x1F;
	parsed_inst -> rd = (inst >> 11) & 0x1F;
	parsed_inst -> shamt = (inst >> 6) & 0x1F;
	parsed_inst -> funct = inst & 0x3F;
	parsed_inst -> immi = inst & 0xFFFF;
	parsed_inst -> immj = inst & 0x03FFFFFF;
}

// Sign extension using bitwise shift
// 어떤 i-type inst는 sign-extension, 다른 i-type inst는 zero-extension
void CTRL::signExtend(uint32_t immi, uint32_t SignExtend, uint32_t *ext_imm) {
	// FILLME
	if(SignExtend){
		*ext_imm = ((immi & 0x8000) ? (immi | 0xFFFF0000) : immi);
	} else{
		*ext_imm = immi & 0xFFFF;
	}
}
