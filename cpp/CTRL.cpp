#include <iostream>
#include "CTRL.h"
#include "ALU.h"
#include "globals.h"

// 1. controlsignal을 찾아내는 것
// 2. instruction split
// 3. sign-extension 수행

CTRL::CTRL() {}

void CTRL::controlSignal(uint32_t opcode, uint32_t funct, Controls *controls) {
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
	controls -> RegWrite = 1;
	controls -> SavePC = 0;
	controls -> ALUOp = 0;

	if(opcode == OP_RTYPE){ //R-type 명령어 처리
		controls -> RegDst = 1;
		controls -> Jump = 0;
		controls -> Branch = 0;
		controls -> JR = 0;
		controls -> MemRead = 0;
		controls -> MemtoReg = 0;
		controls -> MemWrite = 0;
		controls -> ALUSrc = 0;
		controls -> SavePC = 0;
		controls -> RegWrite = 1;
		controls -> SignExtend = 1;
		
		switch(funct){
			case FUNCT_SLL: controls -> ALUOp = ALU_SLL; break;
			case FUNCT_SRL: controls -> ALUOp = ALU_SRL; break;
			case FUNCT_SRA: controls -> ALUOp = ALU_SRA; break;
			case FUNCT_JR:
				controls -> Jump = 1;
				controls -> JR = 1;
				controls -> RegWrite = 0;
				break;
			case FUNCT_ADDU: controls -> ALUOp = ALU_ADDU; break;
			case FUNCT_SUBU: controls -> ALUOp = ALU_SUBU; break;
			case FUNCT_AND: controls -> ALUOp = ALU_AND; break;
			case FUNCT_OR: controls -> ALUOp = ALU_OR; break;
			case FUNCT_XOR: controls -> ALUOp = ALU_XOR; break;
			case FUNCT_NOR: controls -> ALUOp = ALU_NOR; break;
			case FUNCT_SLT: controls -> ALUOp = ALU_SLT; break;
			case FUNCT_SLTU: controls -> ALUOp = ALU_SLTU; break;
			default: status = INVALID_INST;
		}
	} else { //R-type 제외한 나머지 I-type + J-type 처리
		switch(opcode){
			case OP_ADDIU:
				controls -> RegDst = 0;
				controls -> ALUSrc = 1;
				controls -> ALUOp = ALU_ADDU;
				break;
			case OP_SLTI:
				controls -> RegDst = 0;
				controls -> ALUSrc = 1;
				controls -> ALUOp = ALU_SLT;
				break;
			case OP_SLTIU:
				controls -> RegDst = 0;
				controls -> ALUSrc = 1;
				controls -> ALUOp = ALU_SLTU;
				break;
			case OP_ANDI:
				controls -> RegDst = 0;
				controls -> ALUSrc = 1;
				controls -> SignExtend = 0;
				controls -> ALUOp = ALU_AND;
				break;
			case OP_ORI:
				controls -> RegDst = 0;
				controls -> ALUSrc = 1;
				controls -> SignExtend = 0;
				controls -> ALUOp = ALU_OR;
				break;
			case OP_XORI:
				controls -> RegDst = 0;
				controls -> ALUSrc = 1;
				controls -> SignExtend = 0;
				controls -> ALUOp = ALU_XOR;
				break;
			case OP_LUI:
				controls -> RegDst = 0;
				controls -> ALUSrc = 1;
				controls -> ALUOp = ALU_LUI;
				break;
			case OP_LW:
				controls -> RegDst = 0;
				controls -> ALUSrc = 1;
				controls -> MemRead = 1;
				controls -> MemtoReg = 1;
				controls -> ALUOp = ALU_ADDU;
				break;
			case OP_SW:
				controls -> ALUSrc = 1;
				controls -> MemWrite = 1;
				controls -> RegWrite = 0;
				controls -> ALUOp = ALU_ADDU;
				break;
			case OP_BEQ:
				controls -> RegWrite = 0;
				controls -> Branch = 1;
				controls -> ALUOp = ALU_EQ;
				break;
			case OP_BNE:
				controls -> RegWrite = 0;
				controls -> Branch = 1;
				controls -> ALUOp = ALU_NEQ;
				break;
			case OP_J:
				controls -> Jump = 1;
				controls -> RegWrite = 0;
				break;
			case OP_JAL:
				controls -> Jump = 1;
				controls -> SavePC = 1;
				break;
			default:
				status = INVALID_INST;
				break;
		}
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
