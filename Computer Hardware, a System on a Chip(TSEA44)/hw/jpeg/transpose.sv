`include "include/timescale.v"

module transpose(input logic clk, rst, wr , rd, 
		 		 input logic [95:0] in, 
		 		 output logic [95:0] ut);
	
	logic [7:0][7:0][11:0]	mem;			//8*8*12
	logic [2:0] 			col_count;
	logic [2:0] 			row_count;
	logic [7:0][11:0]		ut_tmp;
	
	//counter
	always_ff @(posedge clk) begin
		if (rst) begin
			col_count <= 3'b0;
			row_count <= 3'b0;
		end
		else if (rd)  
			col_count <= col_count + 1;
		else if (wr) 
			row_count <= row_count + 1;
		else begin
			col_count <= col_count;
			row_count <= row_count;
		end
	end
		

	// synchronize output
	always_ff @(posedge clk) begin
		if (wr) begin					//if wr signal set, start write column				
			mem[row_count] <= in;		//read in data		
		end
	end

	//combinational read port
	always_ff @(posedge clk) begin 
		if (rd) begin
			ut_tmp[0] <= mem[7][col_count];				
			ut_tmp[1] <= mem[6][col_count];
			ut_tmp[2] <= mem[5][col_count];
			ut_tmp[3] <= mem[4][col_count];
			ut_tmp[4] <= mem[3][col_count];
			ut_tmp[5] <= mem[2][col_count];
			ut_tmp[6] <= mem[1][col_count];
			ut_tmp[7] <= mem[0][col_count];
		end
	end


	assign ut = ut_tmp;
endmodule

// Local Variables:
// verilog-library-directories:("." ".." "../or1200" "../jpeg" "../pkmc" "../dvga" "../uart" "../monitor" "../lab1" "../dafk_tb" "../eth" "../wb" "../leela")
// End:
