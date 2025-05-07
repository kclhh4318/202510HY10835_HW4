`timescale 1ns / 1ps
`include "GLOBAL.v"

module CPU(
	input		clk,
	input		rst,
	output 		halt
);
	// state-related sig
	wire [2:0] state;
	wire [2:0] next_state;

	// register sig
	reg [31:0]	PC;
	reg [31:0] IR;
	reg [31:0] MDR;
	reg [31:0] A;
	reg [31:0] B;
	reg [31:0] ALUOut;

	// Split the instructions
	wire [5:0]		opcode;
	wire [4:0]		rs;
	wire [4:0]		rt;
	wire [4:0]		rd;
	wire [4:0]		shamt;
	wire [5:0]		funct;
	wire [15:0]		imm;
	wire [25:0]		target;

	assign opcode = IR[31:26];
	assign rs = IR[25:21];
	assign rt = IR[20:16];
	assign rd = IR[15:11];
	assign shamt = IR[10:6];
	assign funct = IR[5:0];
	assign imm = IR[15:0];
	assign target = IR[25:0];

	// control sig
	wire RegDst, Jump, Branch, JR, MemRead, MemtoReg, MemWrite, ALUSrc, SignExtend, RegWrite, SavePC;
	wire IorD, PCWrite, PCWriteCond, IRWrite;
	wire [1:0] ALUSrcA, ALUSrcB, PCSource;
	wire [3:0] ALUOp;

	// data sig
	wire [31:0] ext_imm;
	wire [31:0] mem_addr;
	wire [31:0] mem_read_data;
	wire [31:0] mem_write_data;
	wire [31:0] reg_read_data1;
	wire [31:0] reg_read_data2;
	wire [4:0] reg_write_addr;
	wire [31:0] reg_write_data;
	wire [31:0] alu_in1;
	wire [31:0] alu_in2;
	wire [31:0] alu_result;
	wire alu_zero;
	wire PC_en;

	// SignExtend Immediate
	assign ext_imm = SignExtend ? {{15{imm[15]}}, imm} : {16'b0, imm};

	assign mem_addr = IorD ? ALUOut : PC;
	assign mem_write_data = B;

	assign reg_write_addr = SavePC ? 5'd31 : (RegDst ? rd : rt);
	assign reg_write_data = SavePC ? (PC + 4) : (MemtoReg ? MDR : ALUOut);

	// ALU input MUX
	assign alu_in1 = (ALUSrcA == 2'b00) ? PC :
					 (ALUSrcA == 2'b01) ? A : 32'b0;

	assign alu_in2 = (ALUSrcB == 2'b00) ? B :
					 (ALUSrcB == 2'b01) ? 32'd4 :
					 (ALUSrcB == 2'b10) ? ext_imm :
					 (ALUSrcB == 2'b11) ? (ext_imm << 2) : 32'b0;

	assign PC_en = PCWrite | (PCWriteCond & alu_zero);

	always @(posedge clk or posedge rst) begin
		if (rst) begin
			PC <= 32'b0;
			IR <= 32'b0;
			MDR <= 32'b0;
			A <= 32'b0;
			B <= 32'b0;
			ALUOut <= 32'b0;
		end else begin
			if (PC_en) begin
				case (PCSource)
					2'b00: PC <= alu_result; // PC + 4
					2'b01: PC <= ALUOut; // Branch Target Address
					2'b10: PC <= {PC[31:28], target, 2'b00}; // Jump address
					2'b11: PC <= A; // jr address
				endcase
			end

			if (IRWrite)
				IR <= mem_read_data;

			MDR <= mem_read_data;

			if (state == `STATE_ID) begin
				A <= reg_read_data1;
				B <= reg_read_data2;
			end

			ALUOut <= alu_result;
		end
	end

	assign halt = (IR == 32'b0) && (state == `STATE_ID);

	CTRL ctrl (
		.clk(clk),
		.rst(rst),
		.opcode(opcode),
		.funct(funct),
		.state(state),
		.next_state(next_state),
		.RegDst(RegDst),
		.Jump(Jump),
		.Branch(Branch),
		.JR(JR),
		.MemRead(MemRead),
		.MemtoReg(MemtoReg),
		.MemWrite(MemWrite),
		.ALUSrc(ALUSrc),
		.SignExtend(SignExtend),
		.RegWrite(RegWrite),
		.ALUOp(ALUOp),
		.SavePC(SavePC),
		.IorD(IorD),
		.PCWrite(PCWrite),
		.PCWriteCond(PCWriteCond),
		.IRWrite(IRWrite),
		.ALUSrcA(ALUSrcA),
		.ALUSrcB(ALUSrcB),
		.PCSource(PCSource)
	);

	RF rf (
		.clk(clk),
		.rst(rst),
		.rd_addr1(rs),
		.rd_addr2(rt),
		.rd_data1(reg_read_data1),
		.rd_data2(reg_read_data2),
		.RegWrite(RegWrite),
		.wr_addr(reg_write_addr),
		.wr_data(reg_write_data)
	);

	MEM mem (
		.clk(clk),
		.rst(rst),
		.mem_addr(mem_addr),
		.MemWrite(MemWrite),
		.mem_write_data(mem_write_data),
		.mem_read_data(mem_read_data)
	);

	ALU alu (
		.operand1(alu_in1),
		.operand2(alu_in2),
		.shamt(shamt),
		.funct(ALUOp),
		.alu_result(alu_result)
	);

endmodule
