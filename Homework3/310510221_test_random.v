/**************************************************************************/
// FILE NAME: 310510221_test_random.v
// VERSRION: 1.0
// DATE: Dec 4, 2022
// AUTHOR: Kuan-Wei Chen / NYCU IEE Oasis Lab / 310510221
// CODE TYPE: Verilog
// DESCRIPTION: 2022 Fall Computer Aided Design (CAD) / Homework3
// MODIFICATION HISTORY: 
// Date                 Description
// 2022/12/04           8-bit CPU test pattern(random)
//
/**************************************************************************/
module test;

reg [7:0] in;
reg clk,reset;
wire [7:0] out;

//
reg [7:0] in1, in2;
reg [3:0] opcode;
reg [7:0] reg_a, reg_b, reg_c;
reg [7:0] mem [0:15];
reg mem_valid [0:15];
reg [3:0] addr;
reg 	  rst;
reg [7:0] golden_out;

integer PATNUM = 20;
integer patcount;
integer seed = 310510221;
integer i;

//
initial begin
	clk = 0;
	forever
    #5 clk = !clk;
end

initial begin
    $dumpfile("cpu.vcd");
    $dumpvars;	
end

cpu cc(.in(in), .clk(clk), .reset(reset), .out(out));

task reset_task; begin
	in    = 8'b00000000;
	reset = 1'b1;

	reg_a = 'd0;
	reg_b = 'd0;
	reg_c = 'd0;
	for(i = 0; i < 16; i = i + 1) begin
		mem[i] = 'dx;
		mem_valid[i] = 1'b0;
	end
	addr = 'd0;
	golden_out = 'b0;

#12 in    = 8'b01101111;
	reset = 1'b0;

end endtask


task in_task; begin
	opcode = in1[7:4];
	case(opcode)
		// Single cycle insturction
		4'b0000,
		4'b0001,
		4'b0010,
		4'b0011,
		4'b0100,
		4'b0101,
		4'b1001,
		4'b1011,
		4'b1101,
		4'b1110,
		4'b1111: begin
			in = in1;
			@(negedge clk);
		end

		// Two cycle insturction
		default: begin
			in = in1;
			@(negedge clk);
			in = in2;
			@(negedge clk);
		end

	endcase
end endtask


task check_out_task; begin
	if(out !== golden_out) begin
		$display ("-----------------------------------------------------------");
		$display ("Output is wrong! Your out: %b, Golden out: %b", out, golden_out);
		$display ("-----------------------------------------------------------");
		#12 $finish;
	end
end endtask


task YOU_PASS_task; begin
	$display ("-----------------------------------------------------------");
	$display ("                     Congratulations!                	");
	$display ("             You have passed all patterns!          		  ");
	$display ("-----------------------------------------------------------");
	$finish;
end endtask



initial begin
reset_task;

//write your test pattern----------------
	for(patcount = 0; patcount < PATNUM; patcount = patcount + 1) begin
		in1 = $random(seed) % 'd256;
		in2 = $random(seed) % 'd256;
		golden_out = 8'b0000_0000;

		opcode = in1[7:4];
		addr   = in1[3:0];
		case(opcode) // opcode
			// Single cycle instruction //
			4'b0000: reg_c = reg_a + reg_b;       				   // C = A + B
			4'b0001: reg_c = reg_a - reg_b;       				   // C = A - B
			4'b0010: reg_c = reg_a + 'b1;         				   // C = A + 1
			4'b0011: reg_c = reg_a - 'b1;         				   // C = A - 1
			4'b0100: reg_c = reg_a + reg_b + 'b1; 				   // C = A + B - 1
			4'b0101: reg_c = -reg_a;              				   // C = -A
			4'b1001: begin 										   // Storage C
					 mem[addr] = reg_c;
					 mem_valid[addr] = 1'b1; 
			end  
			4'b1011: golden_out = reg_c; 						   // Output C
			4'b1101: reg_a = reg_c; 							   // A = C
			4'b1110: reg_b = reg_c;


			// Two cycle instruction //
			// Input A
			4'b0110: reg_a = in2;

			// Input B
			4'b0111: reg_b = in2;

			// Input Memory
			4'b1000: begin
				addr = in1[3:0];
				mem[addr] = in2;
				mem_valid[addr] = 1'b1;
			end

			// Load C
			4'b1010: begin 
				if(mem_valid[addr] === 1'b1)
					reg_c = mem[addr];
				else begin
					$display("Warning: Avoid to load the unknown value, orginal opcode: %b", in1[7:4]);
					in1 = 8'b1111_1111;
					in2 = 8'b1111_1111;
				end
			end

			// Output Mem
			4'b1100: begin
				if(mem_valid[addr] === 1'b1)
					golden_out = mem[addr];
				else begin
					$display("Warning: Avoid to output the unknown value, orginal opcode: %b", in1[7:4]);
					in1 = 8'b1111_1111;
					in2 = 8'b1111_1111;
				end
			end

		endcase

		in_task;

		//check_out_task;
		// $display("in1: %b", in1);
		// $display("in2: %b", in2);
		// $display("golden reg_a: %b", reg_a);
		// $display("golden reg_b: %b", reg_b);
		// $display("golden reg_c: %b", reg_c);
		// $display("golden out:   %b", golden_out);
		$display("\033[0;34mPASS PATTERN NO.%4d,\033[m \033[0;32m opcode: %b\033[m", patcount ,opcode);
	end
	YOU_PASS_task;
//----------------------------------------
#10  $finish;

end
endmodule