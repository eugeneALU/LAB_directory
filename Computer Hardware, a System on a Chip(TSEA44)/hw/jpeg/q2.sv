`include "include/timescale.v"

module q2(output[31:0] x_o, 
	  input [31:0] x_i, rec_i);

	logic signed [15:0] x_1_sign;
	logic signed [15:0] rec_1_sign;
	logic signed [15:0] x_2_sign;
	logic signed [15:0] rec_2_sign;
	logic signed [31:0] R1;
	logic signed [31:0] R2;
	logic signed [31:0] R1_shift;
	logic signed [31:0] R2_shift;
	logic signed [15:0] R1_out;
	logic signed [15:0] R2_out;
	logic 	R1_rnd;
	logic   R2_rnd;
	logic 	pos1, pos2;
	logic 	bits1, bits2;

	always_comb begin
		x_1_sign = x_i[31:16]; 
		rec_1_sign = rec_i[31:16];
		R1 = x_1_sign * rec_1_sign;
		R1_rnd = R1[16];
		pos1 = (R1 & 32'h80000000) == 0;
		bits1 = (R1 & 32'h0000ffff) != 0;
		R1_shift = R1 >>> 17;
		R1_out = R1_shift[15:0] + (R1_rnd && (pos1 || bits1));
	end

	always_comb begin
		x_2_sign = x_i[15:0]; 
		rec_2_sign = rec_i[15:0]; 
		R2 = x_2_sign * rec_2_sign;	
		R2_rnd = R2[16];
		pos2 = (R2 & 32'h80000000) == 0;
		bits2 = (R2 & 32'h0000ffff) != 0;
		R2_shift = R2 >>> 17;
		R2_out = R2_shift[15:0] + (R2_rnd && (pos2 || bits2));
	end

   assign x_o = {R1_out, R2_out};
   
endmodule
// Local Variables:
// verilog-library-directories:("." ".." "../or1200" "../jpeg" "../pkmc" "../dvga" "../uart" "../monitor" "../lab1" "../dafk_tb" "../eth" "../wb" "../leela")
// End:
