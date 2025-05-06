`timescale 1ns / 1ps
`include "GLOBAL.v"

module CTRL(
	// input opcode and funct
	input [5:0] opcode,
	input [5:0] funct,

	// output various ports
	output reg RegDst,
	output reg Jump,
	output reg Branch,
	output reg JR,
	output reg MemRead,
	output reg MemtoReg,
	output reg MemWrite,
	output reg ALUSrc,
	output reg SignExtend,
	output reg RegWrite,
	output reg [3:0] ALUOp,
	output reg SavePC
    );

	always @(*) begin
		// FIXME
		RegDst = 1'b1;
		Jump = 1'b0;
		Branch = 1'b0;
		JR = 1'b0;
		MemRead = 1'b0;
		MemtoReg = 1'b0;
		MemWrite = 1'b0;
		ALUSrc = 1'b0;
		SignExtend = 1'b1;
		RegWrite = 1'b1;
		SavePC = 1'b0;
		ALUOp = 4'b0000;

		if(opcode == `OP_RTYPE) begin
			case(funct)
				`FUNCT_ADDU: ALUOp = `ALU_ADDU;
				`FUNCT_SUBU: ALUOp = `ALU_SUBU;
				`FUNCT_AND: ALUOp = `ALU_AND;
				`FUNCT_OR: ALUOp = `ALU_OR;
				`FUNCT_XOR: ALUOp = `ALU_XOR;
				`FUNCT_NOR: ALUOp = `ALU_NOR;
				`FUNCT_SLT: ALUOp = `ALU_SLT;
				`FUNCT_SLTU: ALUOp = `ALU_SLTU;
				`FUNCT_SLL: ALUOp = `ALU_SLL;
				`FUNCT_SRL: ALUOp = `ALU_SRL;
				`FUNCT_SRA: ALUOp = `ALU_SRA;
				`FUNCT_JR: begin
					Jump = 1'b1;
					JR = 1'b1;
					RegWrite = 1'b0;
				end
			endcase
		end
		else begin
			case(opcode)
				`OP_ADDIU: begin
					RegDst = 1'b0;
					ALUSrc = 1'b1;
					ALUOp = `ALU_ADDU;
				end
				`OP_SLTI: begin
					RegDst = 1'b0;
					ALUSrc = 1'b1;
					ALUOp = `ALU_SLT;
				end
				`OP_SLTIU: begin
					RegDst = 1'b0;
					ALUSrc = 1'b1;
					ALUOp = `ALU_SLTU;
				end
				`OP_ANDI: begin
					RegDst = 1'b0;
					ALUSrc = 1'b1;
					ALUOp = `ALU_AND;
				end
				`OP_ORI: begin
					RegDst = 1'b0;
					ALUSrc = 1'b1;
					ALUOp = `ALU_OR;
				end
				`OP_XORI: begin
					RegDst = 1'b0;
					ALUSrc = 1'b1;
					ALUOp = `ALU_XOR;
				end
				`OP_LUI: begin
					RegDst = 1'b0;
					ALUSrc = 1'b1;
					ALUOp = `ALU_LUI;
				end
				`OP_LW: begin
					RegDst = 1'b0;
					ALUSrc = 1'b1;
					MemtoReg = 1'b1;
					MemRead = 1'b1;
					ALUOp = `ALU_ADDU;
				end
				`OP_SW:begin
					ALUSrc = 1'b1;
					MemWrite = 1'b1;
					RegWrite = 1'b0;
					ALUOp = `ALU_ADDU;
				end
				`OP_BEQ:begin
					RegWrite = 1'b0;
					Branch = 1'b1;
					ALUOp = `ALU_EQ;
				end
				`OP_BNE: begin
					RegWrite = 1'b0;
					Branch = 1'b1;
					ALUOp = `ALU_NEQ;
				end
				`OP_J: begin
					Jump = 1'b1;
					RegWrite = 1'b0;
				end
				`OP_JAL: begin
					Jump = 1'b1;
					SavePC = 1'b1;
				end
			endcase
		end
	end
endmodule
