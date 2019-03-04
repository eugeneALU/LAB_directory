`timescale 1ns / 1ps

module lab0(
    input clk_i,
    input rst_i,
    input rx_i,
    output tx_o,
    output [7:0] led_o,
    input [7:0] switch_i,
    input send_i);

	logic clk_baud_tx;
	logic clk_baud_rx;
	
	Generate BaudGen (.*);
	Transmitter Tx (.*);
	Receiver Rx (.*);

endmodule



module Generate (
	input clk_i,
	input rst_i,
	output clk_baud_tx,
	output clk_baud_rx);
	
	parameter COUNT_MAX_TX = 347;					// (40M/115200)
	parameter COUNT_MAX_RX = 347/16;				// (40M/115200)/16
	
	logic [9:0] count_tx = 0;
	logic [9:0] count_rx = 0;
	logic clk_baud_tx_tmp;
	logic clk_baud_rx_tmp;
	
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




module Transmitter (
	input clk_baud_tx,
	input clk_i,
	input rst_i,
	input [7:0] switch_i,
	input send_i,
	output tx_o);
	

	parameter SEND_IDLE = 3'b000;
	parameter SEND_START = 3'b001;
	parameter SEND_DATA = 3'b010;
	parameter SEND_STOP = 3'b011;
	parameter SEND_RESTRICT = 3'b100;

	logic [3:0] b = 0;
	logic [2:0] state = 0;
	logic tx_o_tmp;
	logic send_i_tmp;

	always @(posedge clk_i) begin	
		send_i_tmp <= send_i;
	end

	always @(posedge clk_i) begin	
		if (rst_i) begin
			b <= 0;
			state <= 0;
			tx_o_tmp <= 1;
		end
		else begin 
			case(state)
				SEND_IDLE: begin
					tx_o_tmp <= 1;
					b <= 0;
					if (send_i) begin
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
							tx_o_tmp <= switch_i[b];
							b <= b + 4'b1;	
						end		
					end
				end
				SEND_STOP: begin
					if (clk_baud_tx) begin
						tx_o_tmp <= 1;
						state <= SEND_RESTRICT;
					end	
				end
				SEND_RESTRICT: begin
					tx_o_tmp <= 1;
					if (send_i_tmp) begin
						state <= SEND_RESTRICT;
					end
					else begin
						state <= SEND_IDLE;
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
endmodule 



module Receiver (
	input clk_baud_rx,
	input clk_i,
	input rst_i,
	input rx_i,
	output [7:0] led_o);

	parameter RECEIVE_IDLE = 2'b00;
	parameter RECEIVE_START = 2'b01;
	parameter RECEIVE_STOP = 2'b10;

	logic [3:0] b = 0;
	logic [1:0] state = 0;
	logic [7:0] led_o_tmp;
	logic [3:0] sample = 0;

	always @(posedge clk_i) begin	
		if (rst_i) begin
			led_o_tmp <= 8'd10;
			state <= RECEIVE_IDLE;
			sample <= 0;
		end
		else begin 
			case(state)
				RECEIVE_IDLE: begin
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
							//led_o_tmp[b] <= rx_i;
							b <= 0;
							state <= RECEIVE_STOP;
						end				
						else if (sample == 4'h8) begin
							led_o_tmp[b] <= rx_i;
							b <= b + 4'b1;
						end	
					end
				end
				RECEIVE_STOP: begin	
					if (clk_baud_rx) begin	
						if (sample == 15) begin 
							state <= RECEIVE_IDLE;
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

	assign led_o = led_o_tmp;	
endmodule



/*module Receiver (
	input clk_baud_rx,
	input clk_i,
	input rst_i,
	input rx_i,
	output [7:0] led_o);

	parameter RECEIVE_IDLE = 2'b00;
	parameter RECEIVE_START = 2'b01;
	parameter RECEIVE_DATA = 2'b10;
	parameter RECEIVE_STOP = 2'b11;
	parameter COUNT_MAX = 347;					// (40M/115200)

	logic [3:0] b = 0;
	logic [1:0] state = 0;
	logic [7:0] led_o_tmp;
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
			led_o_tmp <= 8'd10;
			state <= RECEIVE_IDLE;
			count <= 0;
		end
		else begin 
			case(state)
				RECEIVE_IDLE: begin
					if (rx_i_old==1 && rx_i==0) begin		//falling edge detect
						state <= RECEIVE_START;
						count <= 0;
					end
				end
				RECEIVE_START: begin
					if (count == COUNT_MAX/2) begin
						state <= RECEIVE_DATA;
						count <= 0;
					end
					else begin
						count <= count + 1;
					end				
				end
				RECEIVE_DATA: begin
					count <= count + 1;
					if (b == 4'd7 && count == COUNT_MAX) begin	
						led_o_tmp[b] <= rx_i;
						count <= 0;
						state <= RECEIVE_STOP;
					end				
					else if (count == COUNT_MAX) begin
						led_o_tmp[b] <= rx_i;
						b <= b + 4'b1;
						count <= 0;
					end	
				end
				RECEIVE_STOP: begin		
					if (count == COUNT_MAX/2) begin 
						state <= RECEIVE_IDLE;
						b <= 0;
						count <= 0;
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
		end
	end

	assign led_o = led_o_tmp;	
endmodule*/
// Local Variables:
// verilog-library-directories:("." "or1200" "jpeg" "pkmc" "dvga" "uart" "monitor" "lab1" "dafk_tb" "eth" "wb" "leela")
// End:























