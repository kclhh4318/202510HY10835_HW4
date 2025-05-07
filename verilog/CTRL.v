`timescale 1ns / 1ps
`include "GLOBAL.v"

module CTRL(
	input clk,
	input rst,
	// input opcode and funct
	input [5:0] opcode,
	input [5:0] funct,

	output reg [2:0] state,
	output reg [2:0] next_state,

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
	output reg SavePC,

	output reg IorD,
	output reg PCWrite,
	output reg PCWriteCond,
	output reg IRWrite,
	output reg [1:0] ALUSrcA,
	output reg [1:0] ALUSrcB,
	output reg [1:0] PCSource
);

	always @(posedge clk or posedge rst) begin
		if (rst)
			state <= `STATE_IF;
		else 
			state <= next_state;
	end

	// next_state decision
	always @(*) begin
		// FIXME
		case (state)
			`STATE_IF: next_state = `STATE_ID;
			`STATE_ID: begin
				if ((opcode == `OP_J) || (opcode == `OP_JAL) ||
					(opcode == `OP_RTYPE && funct == `FUNCT_JR))
					next_state = `STATE_IF;
				else
					next_state = `STATE_EX;
			end
			`STATE_EX: begin
				if (opcode == `OP_BEQ || opcode == `OP_BNE)
					next_state = `STATE_IF;
				else if (opcode == `OP_LW || opcode == `OP_SW)
					next_state = `STATE_MEM;
				else
					next_state = `STATE_WB;
			end
			`STATE_MEM: begin
				if (opcode == `OP_LW)
					next_state = `STATE_WB;
				else
					next_state = `STATE_IF;
			end
			`STATE_WB: next_state = `STATE_IF;
			default: next_state = `STATE_IF;
		endcase
	end

	always @(*) begin
		RegDst = 1'b0;
		Jump = 1'b0;
		Branch = 1'b0;
		JR = 1'b0;
		MemRead = 1'b0;
		MemtoReg = 1'b0;
		MemWrite = 1'b0;
		ALUSrc = 1'b0;
		SignExtend = 1'b1;
		RegWrite = 1'b0;
		SavePC = 1'b0;
		ALUOp = 4'b0000;

		IorD = 1'b0;
		PCWrite = 1'b0;
		PCWriteCond = 1'b0;
		IRWrite = 1'b0;
		ALUSrcA = 2'b00;
		ALUSrcB = 2'b00;
		PCSource = 2'b00;

		case (state) 
			`STATE_IF: begin
				MemRead = 1'b1;
				IRWrite = 1'b1;
				ALUSrcA = 2'b00; // PC
				ALUSrcB = 2'b01; // 4
				ALUOp = `ALU_ADDU;
				PCWrite = 1'b1;
				PCSource = 2'b00; // alu_result(PC + 4)
			end

			`STATE_ID: begin
				ALUSrcA = 2'b00; // PC
				ALUSrcB = 2'b11; // branch offset
				ALUOp = `ALU_ADDU;

				if (opcode == `OP_J) begin
					Jump = 1'b1;
					PCWrite = 1'b1;
					PCSource = 2'b10; // Jump address
				end else if (opcode == `OP_JAL) begin
					Jump = 1'b1;
					PCWrite = 1'b1;
					PCSource = 2'b10; // Jump Address
					SavePC = 1'b1;
					RegWrite = 1'b1;
				end else if (opcode == `OP_RTYPE && funct == `FUNCT_JR) begin
					Jump = 1'b1;
					JR = 1'b1;
					PCWrite = 1'b1;
					PCSource = 2'b11; // JR
				end
			end

			`STATE_EX: begin
				if (opcode == `OP_RTYPE) begin
					ALUSrcA = 2'b01; // Register A
					ALUSrcB = 2'b00; // Register B

					case (funct)
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
					endcase
				end else if (opcode == `OP_BEQ || opcode == `OP_BNE) begin
					ALUSrcA = 2'b01;
					ALUSrcB = 2'b00;
					Branch = 1'b1;
					PCWriteCond = 1'b1;
					PCSource = 2'b01;

					if (opcode == `OP_BEQ)
						ALUOp = `ALU_EQ;
					else
						ALUOp = `ALU_NEQ;
				end else begin
					ALUSrcA = 2'b01;
					ALUSrcB = 2'b10;

					case (opcode)
						`OP_ADDIU: begin
							ALUOp = `ALU_ADDU;
							SignExtend = 1'b1;
						end
						`OP_SLTI: begin
							ALUOp = `ALU_SLT;
							SignExtend = 1'b1;
						end
						`OP_SLTIU: begin
							ALUOp = `ALU_SLTU;
							SignExtend = 1'b1;
						end
						`OP_ANDI: begin
							ALUOp = `ALU_AND;
							SignExtend = 1'b0;
						end
						`OP_ORI: begin
							ALUOp = `ALU_OR;
							SignExtend = 1'b0;
						end
						`OP_XORI: begin
							ALUOp = `ALU_XOR;
							SignExtend = 1'b0;
						end
						`OP_LUI: begin
							ALUOp = `ALU_LUI;
							SignExtend = 1'b0;
						end
						`OP_LW, `OP_SW: begin
							ALUOp = `ALU_ADDU;
							SignExtend = 1'b1;
						end
					endcase
				end
			end

			`STATE_MEM: begin
				IorD = 1'b1; //ALUOut

				if (opcode == `OP_LW)
					MemRead = 1'b1;
				else if (opcode == `OP_SW)
					MemWrite = 1'b1;
			end

			`STATE_WB: begin
				RegWrite = 1'b1;

				if (opcode == `OP_RTYPE) begin
					RegDst = 1'b1; // rd
					MemtoReg = 1'b0; // ALUOut
				end else if (opcode == `OP_LW) begin
					RegDst = 1'b0; //rt
					MemtoReg = 1'b1; // MDR
				end else begin
					RegDst = 1'b0; //rt
					MemtoReg = 1'b0; //ALUOut
				end
			end
		endcase
	end
endmodule
