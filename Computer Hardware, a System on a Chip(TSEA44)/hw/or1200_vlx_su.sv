`include "include/timescale.v"

module or1200_vlx_su(/*AUTOARG*/
   // Outputs
   vlx_addr_o, dat_o, last_byte_o, store_byte_o, 
   // Inputs
   clk_i, rst_i, ack_i, dat_i, store_byte_i, set_init_addr_i
   );

	input clk_i;
	input rst_i;
	input ack_i; //ack is high when write completes

	input [31:0] dat_i; 			//the data to be stored
	input 		 store_byte_i; 		//start storing data in the next clock cycle if high 	
	input 		 set_init_addr_i; 	//set the address in the next clock cycle if high

	output 	reg [31:0] vlx_addr_o; 	//address where data is stored
	output 	reg [31:0] dat_o; 		//actual data stored 
	output 	last_byte_o; 			//high when the last byte is being stored.
	output 	store_byte_o; 			//high when a byte should be stored
	
	logic store_byte;

	always_ff @(posedge clk_i) begin
		if(rst_i) 								// reset the VLX addr
			vlx_addr_o <= 32'h0;
		else if (set_init_addr_i) 
			vlx_addr_o <= dat_i;		
		else if (ack_i && store_byte)
			vlx_addr_o <= vlx_addr_o + 32'b1;	// spr_dat_i
		else 
			vlx_addr_o <= vlx_addr_o;
	end

	always_ff @(posedge clk_i) begin	
		if(rst_i)							// reset the send data
			dat_o <= 32'h0;
		else if (store_byte_i)
			dat_o <= dat_i;
	end

	always_ff @(posedge clk_i) begin
	//always_comb begin
		if (ack_i) 
			store_byte <= 1'b0;
		else begin
			if (store_byte_i)
				store_byte <= 1'b1;
			else 
				store_byte <= store_byte;
		end
	end

	assign store_byte_o = store_byte;
	assign last_byte_o = 1'b0;
   
endmodule // or1200_vlx_su
// Local Variables:
// verilog-library-directories:("." ".." "../or1200" "../jpeg" "../pkmc" "../dvga" "../uart" "../monitor" "../lab1" "../dafk_tb" "../eth" "../wb" "../leela")
// End:
