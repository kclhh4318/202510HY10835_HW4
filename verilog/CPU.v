`timescale 1ns / 1ps
`include "GLOBAL.v"

module CPU(
    input       clk,
    input       rst,
    output      halt
);
    wire [2:0] state;
    wire [2:0] next_state;

    reg [31:0] PC;
    reg [31:0] IR;
    reg [31:0] MDR;
    reg [31:0] A;
    reg [31:0] B;
    reg [31:0] ALUOut;

    wire [5:0] opcode;
    wire [4:0] rs;
    wire [4:0] rt;
    wire [4:0] rd;
    wire [4:0] shamt;
    wire [5:0] funct;
    wire [15:0] immi;
    wire [25:0] immj;

    assign opcode = IR[31:26];
    assign rs = IR[25:21];
    assign rt = IR[20:16];
    assign rd = IR[15:11];
    assign shamt = IR[10:6];
    assign funct = IR[5:0];
    assign immi = IR[15:0];
    assign immj = IR[25:0];

    wire RegDst, Jump, Branch, JR, MemRead, MemtoReg, MemWrite, ALUSrc, SignExtend, RegWrite, SavePC;
    wire IorD, PCWrite, PCWriteCond, IRWrite;
    wire [1:0] ALUSrcA, ALUSrcB;
    wire [1:0] PCSource;
    wire [3:0] ALUOp;

    wire [31:0] ext_imm;
    wire [31:0] mem_addr;
    wire [31:0] mem_read_data;
    wire [31:0] mem_write_data;
    wire [31:0] rd_data1, rd_data2;
    wire [4:0] wr_addr;
    wire [31:0] wr_data;
    wire [31:0] alu_in1, alu_in2;
    wire [31:0] alu_result;
    wire PC_en;

    assign ext_imm = SignExtend ? 
                     ((immi[15]) ? {16'hFFFF, immi} : {16'h0000, immi}) : 
                     {16'h0000, immi};

    assign mem_addr = IorD ? ALUOut : PC;
    assign mem_write_data = B;

    assign wr_addr = SavePC ? 5'd31 : (RegDst ? rd : rt);
    
    assign wr_data = SavePC ? PC : (MemtoReg ? MDR : ALUOut);

    assign alu_in1 = (ALUSrcA == 2'b00) ? PC : 
                     (ALUSrcA == 2'b01) ? A : 32'b0;
                     
    assign alu_in2 = (ALUSrcB == 2'b00) ? B : 
                     (ALUSrcB == 2'b01) ? 32'd4 : 
                     (ALUSrcB == 2'b10) ? ext_imm : 
                     (ALUSrcB == 2'b11) ? (ext_imm << 2) : 32'b0;

    assign PC_en = PCWrite | (PCWriteCond & alu_result);

    always @(posedge clk or posedge rst) begin
        if (rst) begin
            PC <= 32'b0;
            IR <= 32'b0;
            MDR <= 32'b0;
            A <= 32'b0;
            B <= 32'b0;
            ALUOut <= 32'b0;
        end 
        else begin
            if (PC_en) begin
                case (PCSource)
                    2'b00: PC <= alu_result;              // PC + 4
                    2'b01: PC <= ALUOut;                  // Branch target
                    2'b10: PC <= {PC[31:28], immj, 2'b00}; // Jump target
                    2'b11: PC <= rd_data1;                // JR target - rd_data1
                endcase
            end
            
            if (IRWrite)
                IR <= mem_read_data;
                
            MDR <= mem_read_data;
            
            A <= rd_data1;
            B <= rd_data2;
            
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
        .rd_data1(rd_data1),
        .rd_data2(rd_data2),
        .RegWrite(RegWrite),
        .wr_addr(wr_addr),
        .wr_data(wr_data)
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