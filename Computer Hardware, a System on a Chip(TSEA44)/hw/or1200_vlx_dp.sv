`include "include/timescale.v"

module or1200_vlx_dp(/*AUTOARG*/
   // Outputs
   spr_dat_o, bit_reg_o, stall_CPU_o, need_send_o,
   // Inputs
   clk_i, rst_i, bit_vector_i, num_bits_to_write_i, spr_addr, 
   write_dp_spr_i, spr_dat_i, ack_i, set_bit_op_i
   );

	input clk_i;
	input rst_i;
	input ack_i;

	input [31:0] 	bit_vector_i;
	input [4:0] 	num_bits_to_write_i;
	input 			spr_addr;
	input 			write_dp_spr_i;
	input [31:0]	spr_dat_i;
	input 			set_bit_op_i;

	output [31:0] spr_dat_o;
	output [31:0] bit_reg_o;
	output 		  stall_CPU_o;
	output 		  need_send_o;

	reg [31:0] 	bit_reg;
	reg [5:0] 	bit_reg_wr_pos; // count how many bits of bit_reg_wr_pos


	logic [7:0]	bit_reg_send;
	logic 		need_send;
	logic 		stall_CPU;
	logic 		sending;
	logic 		minus;
	logic 		send00;

   	//Here you must write code for packing bits to a register.
	always_ff @(posedge clk_i or posedge rst_i) begin
		if(rst_i) begin
			bit_reg <= 0;
			bit_reg_wr_pos <= 0;			// store reg start from LSB(bit0), differ from software version
		end
		else begin
			if(write_dp_spr_i) begin
				if(spr_addr) begin
					bit_reg <= spr_dat_i;
				end
				else begin
					bit_reg_wr_pos <= spr_dat_i[5:0];
				end
			end
			else begin	//handle data input
				if(set_bit_op_i) begin											
					bit_reg_wr_pos <= bit_reg_wr_pos + num_bits_to_write_i;		// if start from MSB(bit31), will face data dependency here; we need get {bit_reg_wr_pos} first then store {bit_reg}
					bit_reg <= (bit_reg << num_bits_to_write_i) |  bit_vector_i; // & {(32-num_bit_to_write_i)'b0, (num_bit_to_write_i)'b1}; //legal semantic?? dynamic filter out
 				end			
				
				if(minus) begin
					bit_reg_wr_pos <= bit_reg_wr_pos - 8;
				end 
			end
		end
	end

	always_comb begin
		if(bit_reg_wr_pos > 7 || bit_reg_send == 8'hff) begin
			need_send = 1'b1;
		end
		else begin 
			need_send = 1'b0;
		end
	end
			
	
	// * @Name   : FSM, handle sending data(bit_reg)
	// * @Relate : state_reg, bit_reg_wr_pos, need_send, bit_reg
	parameter REG_IDLE 	= 2'd0;
	parameter REG_SEND	= 2'd1;
	parameter REG_SEND00= 2'd2;

	logic [1:0] state_reg;

	always_ff @(posedge clk_i) begin
		if(rst_i) begin
			send00 <= 1'b0;
			minus <= 1'b0;
			bit_reg_send <=	8'h00;
			state_reg <= REG_IDLE;
		end
		else begin
			case(state_reg)
				REG_IDLE: begin
					send00 <= 1'b0;
					minus <= 1'b0;
					sending <= 1'b0;
					if(bit_reg_wr_pos > 7) begin
						bit_reg_send[7] <= bit_reg[bit_reg_wr_pos-1]; //[x-1]
						bit_reg_send[6] <= bit_reg[bit_reg_wr_pos-2]; //[x-2]
						bit_reg_send[5] <= bit_reg[bit_reg_wr_pos-3]; //[x-3]
						bit_reg_send[4] <= bit_reg[bit_reg_wr_pos-4]; //[x-4]
						bit_reg_send[3] <= bit_reg[bit_reg_wr_pos-5]; //[x-5]
						bit_reg_send[2] <= bit_reg[bit_reg_wr_pos-6]; //[x-6]
						bit_reg_send[1] <= bit_reg[bit_reg_wr_pos-7]; //[x-7]
						bit_reg_send[0] <= bit_reg[bit_reg_wr_pos-8]; //[x-8]
						minus <= 1'b1;	
						sending <= 1'b1;
						state_reg <= REG_SEND;
					end
				end
				REG_SEND: begin
					minus <= 1'b0;	
					sending <= 1'b1;
					if(bit_reg_send == 8'hff && ack_i) begin	//detect ff and finshed previous sending
						send00 <= 1'b1;
						bit_reg_send <=	8'h00;					//sending 8'h00 after sending 8'hff							
						state_reg <= REG_SEND00;
					end
					else if(ack_i) begin	//send finish
						if(bit_reg_wr_pos > 7) begin	//if still more than 7 bit, send again
							bit_reg_send[7] <= bit_reg[bit_reg_wr_pos-1]; //[x-1]
							bit_reg_send[6] <= bit_reg[bit_reg_wr_pos-2]; //[x-2]
							bit_reg_send[5] <= bit_reg[bit_reg_wr_pos-3]; //[x-3]
							bit_reg_send[4] <= bit_reg[bit_reg_wr_pos-4]; //[x-4]
							bit_reg_send[3] <= bit_reg[bit_reg_wr_pos-5]; //[x-5]
							bit_reg_send[2] <= bit_reg[bit_reg_wr_pos-6]; //[x-6]
							bit_reg_send[1] <= bit_reg[bit_reg_wr_pos-7]; //[x-7]
							bit_reg_send[0] <= bit_reg[bit_reg_wr_pos-8]; //[x-8]
							minus <= 1'b1;
							sending <= 1'b1;
							state_reg <= REG_SEND;
						end
						else begin
							sending <= 1'b0;
							state_reg <= REG_IDLE;
						end	
					end
				end
				REG_SEND00: begin	
					send00 <= 1'b0;
					minus <= 1'b0;
					sending <= 1'b1;
					if(ack_i) begin				//send finish
						if(bit_reg_wr_pos > 7) begin
							bit_reg_send[7] <= bit_reg[bit_reg_wr_pos-1]; //[x-1]
							bit_reg_send[6] <= bit_reg[bit_reg_wr_pos-2]; //[x-2]
							bit_reg_send[5] <= bit_reg[bit_reg_wr_pos-3]; //[x-3]
							bit_reg_send[4] <= bit_reg[bit_reg_wr_pos-4]; //[x-4]
							bit_reg_send[3] <= bit_reg[bit_reg_wr_pos-5]; //[x-5]
							bit_reg_send[2] <= bit_reg[bit_reg_wr_pos-6]; //[x-6]
							bit_reg_send[1] <= bit_reg[bit_reg_wr_pos-7]; //[x-7]
							bit_reg_send[0] <= bit_reg[bit_reg_wr_pos-8]; //[x-8]
							//need_send <= 1'b1;
							minus <= 1'b1;
							sending <= 1'b1;
							state_reg <= REG_SEND;
							end
						else begin
							sending <= 1'b0;
							state_reg <= REG_IDLE;
						end				
					end
				end
				default: begin
					send00 <= 1'b0;
					minus <= 1'b0;
					sending <= 1'b0;
					state_reg <= REG_IDLE;
				end
			endcase 
		end
	end 

	always_comb begin			
		if(set_bit_op_i && bit_reg_wr_pos + num_bits_to_write_i > 7) // sending data so stall the CPU
			stall_CPU = 1'b1;
		else
			stall_CPU = 1'b0;
	end

	assign spr_dat_o = spr_addr ? bit_reg : {26'b0,bit_reg_wr_pos};
	assign bit_reg_o = {24'b0, bit_reg_send};
	assign need_send_o = (sending && need_send) || send00;		//
	assign stall_CPU_o = stall_CPU || sending || need_send;

endmodule // or1200_vlx_dp
// Local Variables:
// verilog-library-directories:("." ".." "../or1200" "../jpeg" "../pkmc" "../dvga" "../uart" "../monitor" "../lab1" "../dafk_tb" "../eth" "../wb" "../leela")
// End:
