`include "include/timescale.v"


module lab1_uart_top (
	wishbone.slave wb,
	output 	wire int_o,
	input	wire srx_pad_i,
	output 	wire stx_pad_o);

	//interconnection
	logic 		clk_baud_tx;
	logic 		clk_baud_rx;
	logic 		tx_o;
	logic 		rd;
	logic 		wr;
	logic 		send_i;
	logic 		rx_full;
	logic [1:0]	tx_empty;
	logic [7:0] tx_reg;
	logic [7:0] rx_reg;
	logic [7:0] rx_shift_reg;
	logic 		end_char_rx;
	logic 		end_char_tx;
	logic 		ack_tmp;
	logic 		load_succeed;

	//instantiate 
	Generate1 	BaudGen (wb.clk, wb.rst, clk_baud_tx, clk_baud_rx);
	Transmitter1	Tx 		(clk_baud_tx, wb.clk, wb.rst, tx_reg, send_i, tx_o, end_char_tx);
	Receiver1 	Rx 		(clk_baud_rx, wb.clk, wb.rst, srx_pad_i, rx_shift_reg, end_char_rx);

	//combination logic of {rd} and {wr}
	always_comb begin 
		rd <= wb.stb & !wb.we & wb.sel[3] & !wb.adr[2];
		wr <= wb.stb & wb.we & wb.sel[3] & !wb.adr[2];
	end

	//syn {wr} to {send_i}
	always @(posedge wb.clk) begin
		send_i <= wr;
	end

	//{tx_empty} FF
	always @(posedge wb.clk) begin
		tx_empty <= tx_empty;
		if (wr)
			tx_empty <= 2'b00;
		else begin
			if (end_char_tx || wb.rst)
				tx_empty <= 2'b11;
		end
	end

	//{rx_full} FF
	always @(posedge wb.clk) begin
		rx_full <= rx_full;
		if (rd || wb.rst)
			rx_full <= 1'b0;
		else if (end_char_rx)	
			rx_full <= 1'b1;
	end

	//tx_reg
	always @(posedge wb.clk) begin 
		if (wr) begin
			tx_reg <= wb.dat_o[31:24];
			load_succeed <= 1;
		end
		else begin 
			tx_reg <= tx_reg;
			load_succeed <= 0;
		end
	end

	//rx_reg 
	always @(posedge wb.clk) begin 
		if (end_char_rx)
			rx_reg <= rx_shift_reg;
		else 
			rx_reg <= rx_reg;
	end

	//ack 
	always @(posedge wb.clk) begin 
		ack_tmp <= 0;
		if (wb.rst || ack_tmp == 1)
			ack_tmp <= 0;
		else begin
			if (wb.stb)
				ack_tmp <= 1;
			else
				ack_tmp <= 0;
			end 
		end

	assign wb.dat_i[16] = rx_full;
	assign wb.dat_i[31:24] = rx_reg;
	assign wb.dat_i[22:21] = tx_empty;

	assign wb.ack = ack_tmp;   		
	assign stx_pad_o = tx_o; 	

	assign int_o = 1'b0;  // Interrupt, not used in this lab
	assign wb.err = 1'b0; // Error, not used in this lab
	assign wb.rty = 1'b0; // Retry, not used in this course
endmodule

module Generate1 (
	input clk_i,
	input rst_i,
	output clk_baud_tx,
	output clk_baud_rx);
	
	parameter COUNT_MAX_TX = 217;					// (25M/115200)
	parameter COUNT_MAX_RX = 217/16;				// (25M/115200)/16
	
	logic [9:0] count_tx = 0;
	logic [9:0] count_rx = 0;
	logic 		clk_baud_tx_tmp;
	logic 		clk_baud_rx_tmp;
	
	always @(posedge clk_i) begin
		if (rst_i) begin
			count_tx <= 0;
			clk_baud_tx_tmp <= 0;
			count_rx <= 0;
			clk_baud_rx_tmp <= 0;
		end
		else begin
			if (count_tx < COUNT_MAX_TX) begin
				count_tx <= count_tx + 10'b1;
				clk_baud_tx_tmp <= 0;
			end
			else begin 
				count_tx <= 0;
				clk_baud_tx_tmp <= 1;
			end
			
			if (count_rx < COUNT_MAX_RX) begin
				count_rx <= count_rx + 10'b1;
				clk_baud_rx_tmp <= 0;
			end
			else begin 
				count_rx <= 0;
				clk_baud_rx_tmp <= 1;
			end
		end	
	end

	assign clk_baud_tx = clk_baud_tx_tmp;
	assign clk_baud_rx = clk_baud_rx_tmp;
endmodule

module Transmitter1 (
	input	clk_baud_tx,
	input 	clk_i,
	input 	rst_i,
	input [7:0] data_tx,
	input 	send_i,
	output	tx_o,
	output	end_char_tx);
	

	parameter SEND_IDLE = 3'b000;
	parameter SEND_START = 3'b001;
	parameter SEND_DATA = 3'b010;
	parameter SEND_STOP = 3'b011;
	parameter SEND_RESTRICT = 3'b100;

	logic [3:0] b = 0;
	logic [2:0] state = 0;
	logic 		tx_o_tmp;
	logic 		send_i_tmp;
	logic 		end_char_tx_tmp;

	always @(posedge clk_i) begin	
		send_i_tmp <= send_i;
	end

	always @(posedge clk_i) begin	
		if (rst_i) begin
			b <= 0;
			state <= 0;
			tx_o_tmp <= 1;
			end_char_tx_tmp <= 0;
		end
		else begin 
			case(state)
				SEND_IDLE: begin
					tx_o_tmp <= 1;
					b <= 0;
					end_char_tx_tmp <= 0;
					if (send_i_tmp) begin
						state <= SEND_START;
					end		
				end
				SEND_START: begin
					if (clk_baud_tx) begin
						tx_o_tmp <= 0;
						state <= SEND_DATA;	
					end
				end
				SEND_DATA: begin
					if (clk_baud_tx) begin
						if (b == 4'b1000) begin			
							tx_o_tmp <= 1;
							state <= SEND_STOP;						
						end				
						else begin
							tx_o_tmp <= data_tx[b];
							b <= b + 4'b1;	
						end		
					end
				end
				SEND_STOP: begin
					if (clk_baud_tx) begin
						end_char_tx_tmp <= 1;
						tx_o_tmp <= 1;
						state <= SEND_RESTRICT;
					end	
				end
				SEND_RESTRICT: begin
					tx_o_tmp <= 1;
					end_char_tx_tmp <= 0;
					if (send_i_tmp) begin
						state <= SEND_RESTRICT;
					end
					else begin
						state <= SEND_IDLE;
						//end_char_tx_tmp <= 1;
					end	
				end
				default: begin
					state <= SEND_IDLE;	
					tx_o_tmp <= 1;	
				end
			endcase
		end
	end		
	
	assign tx_o = tx_o_tmp;
	assign end_char_tx = end_char_tx_tmp;
endmodule 

module Receiver1 (
	input 		  clk_baud_rx,
	input 		  clk_i,
	input 		  rst_i,
	input 		  rx_i,
	output	[7:0] data_rx,
	output 		  end_char_rx);

	parameter RECEIVE_IDLE = 2'b00;
	parameter RECEIVE_START = 2'b01;
	parameter RECEIVE_STOP = 2'b10;
	parameter COUNT_MAX = 217;					// (25M/115200)

	logic [3:0] b = 0;
	logic [1:0] state = 0;
	logic [7:0]	data_rx_tmp;
	logic [3:0] sample = 0;
	logic 		end_char_rx_tmp;
	logic [9:0] count = 0;
	logic       rx_i_old;

	always @(posedge clk_i) begin	
		if (rst_i) begin
			rx_i_old <= 1;
		end
		else begin 
			rx_i_old <= rx_i;
		end	
	end

	always @(posedge clk_i) begin	
		if (rst_i) begin
			state <= RECEIVE_IDLE;
			sample <= 0;
			end_char_rx_tmp <= 0;
		end
		else begin 
/*
			case(state)
				RECEIVE_IDLE: begin
					end_char_rx_tmp <= 0;
					if (rx_i_old==1 && rx_i==0) begin		//falling edge detect
						state <= RECEIVE_START;
						count <= 0;
						b <= 4'b1111;
					end
				end
				RECEIVE_START: begin
					count <= count + 1;
					if (count == COUNT_MAX) begin
						count <= 0;
					end
					if (b == 4'b1000 && count == COUNT_MAX) begin	
						b <= 0;
						count <= 0;
						state <= RECEIVE_STOP;
					end		
					else if (b == 4'b1111 && count == COUNT_MAX) begin 
						b <= b + 4'b1;
					end		
					else if (count == COUNT_MAX/2) begin
						data_rx_tmp[b] <= rx_i;
						b <= b + 4'b1;
					end	
				end
				RECEIVE_STOP: begin		
					if (count == COUNT_MAX/2) begin 	//won't receive full stop bit
						state <= RECEIVE_IDLE;
						count <= 0;
						end_char_rx_tmp <= 1;
					end
					else begin 
						count <= count + 1;
					end
				end
				default: begin
					b <= 0;
					count <= 0;
					state <= RECEIVE_IDLE;
				end
			endcase
*/

			case(state)
				RECEIVE_IDLE: begin
					end_char_rx_tmp <= 0;
					if (clk_baud_rx) begin
						if (!rx_i || sample != 0)
							sample <= sample + 4'b1;	

						if (sample == 15) begin
							state <= RECEIVE_START;
							b <= 0;
							sample <= 0;
						end
					end
				end
				RECEIVE_START: begin
					if (clk_baud_rx) begin
						sample <= sample + 4'b1;
						if (b == 4'b1000 && sample == 4'd15) begin	
							b <= 0;
							state <= RECEIVE_STOP;
							//end_char_rx_tmp <= 1;
						end				
						else if (sample == 4'h8) begin
							data_rx_tmp[b] <= rx_i;
							b <= b + 4'b1;
						end	
					end
				end
				RECEIVE_STOP: begin	
					if (clk_baud_rx) begin	
						if (sample == 8) begin 			//won't receive full stop bit
							state <= RECEIVE_IDLE;
							end_char_rx_tmp <= 1;
							b <= 0;
							sample <= 0;
						end
						else begin 
							sample <= sample + 4'b1;
						end
					end
				end
				default: begin
					b <= 0;
					state <= RECEIVE_IDLE;	
				end
			endcase

		end
	end

	assign data_rx = data_rx_tmp;	
	assign end_char_rx = end_char_rx_tmp;
endmodule
// Local Variables:
// verilog-library-directories:("." ".." "../or1200" "../jpeg" "../pkmc" "../dvga" "../uart" "../monitor" "../lab1" "../dafk_tb" "../eth" "../wb" "../leela")
// End:
