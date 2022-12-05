/**************************************************************************/
// FILE NAME: 310510221_test.v
// VERSRION: 1.0
// DATE: Dec 4, 2022
// AUTHOR: Kuan-Wei Chen / NYCU IEE Oasis Lab / 310510221
// CODE TYPE: Verilog
// DESCRIPTION: 2022 Fall Computer Aided Design (CAD) / Homework3
// MODIFICATION HISTORY: 
// Date                 Description
// 2022/12/04           8-bit CPU test pattern(direct)
//
/**************************************************************************/
module test;

reg [7:0] in;
reg clk,reset;
wire [7:0] out;
integer p;
initial begin
	clk=0;
	forever
    #5 clk = !clk;
end

initial begin
    $dumpfile("cpu.vcd");
    $dumpvars;	
end

cpu cc(.in(in), .clk(clk), .reset(reset), .out(out));

initial begin
	in    = 8'b00000000;
	reset = 1'b1;

#12 in    = 8'b01101111; // Input A
	reset = 1'b0;
     
//write your test pattern----------------

// 01
// #10 in = 8'b0110_1111; // Input A
#10 in = 8'b1111_1111; // A = 1111_1111

// 02
#10 in = 8'b0111_1111; // Input B
#10 in = 8'b1111_1111; // B = 1111_1111;

#10 reset = 1'b1;
#10 reset = 1'b0;

// 03
#10 in = 8'b0110_1111; // Input A
#10 in = 8'b1111_1111; // A = 1111_1111

// 04
#10 in = 8'b0111_1111; // Input B
#10 in = 8'b1111_1111; // B = 1111_1111;

// 05
#10 in = 8'b1000_1111; // Input Mem, addr = 1111
#10 in = 8'b1111_1111; // mem[addr] = 1111_1111

// 06
#10 in = 8'b1010_1111; // Load C, addr = 1111
#10 in = 8'b1111_1111; // Reserved

// 07
#10 in = 8'b1100_1111; // Output Mem, addr = 1111
#10 in = 8'b1111_1111; // Reserved

////////////////////////////////////////////////////////

// 08
#10 in = 8'b0000_0000; // C = A + B

// 09
#10 in = 8'b0001_0000; // C = A - B

// 10
#10 in = 8'b1001_1111; // Storage C, addr = 1111

// 11
#10 in = 8'b1100_1111; // Output Mem, addr = 1111
#10 in = 8'b1111_1111; // Reserved

// 12
#10 in = 8'b0010_0000; // C = A + 1

// 13
#10 in = 8'b0011_0000; // C = A - 1

// 14
#10 in = 8'b0100_0000; // C = A + B + 1

// 15
#10 in = 8'b1001_1111; // Storage C, addr = 1111

// 16
#10 in = 8'b1100_1111; // Output Mem, addr = 1111
#10 in = 8'b1111_1111; // Reserved

// 17
#10 in = 8'b0101_0000; // C = -A

// 18
#10 in = 8'b1011_1111; // Output C

// 19
#10 in = 8'b1101_0000; // A = C

// 20
#10 in = 8'b1110_0000; // B = C


//----------------------------------------
#10  $finish;

end
endmodule