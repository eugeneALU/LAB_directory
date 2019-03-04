`include "include/timescale.v"

module perf_top
(
	wishbone.slave wb,
	// Master signals
	wishbone.monitor m0, m1 //m6 performance counter for DMA
);

	logic [31:0] counter_m0_time, counter_m0_ack, counter_m1_time, counter_m1_ack;
	logic [31:0] data_i_t;
	logic 		 ack_t; 

	always @(posedge wb.clk) begin
		if (wb.rst == 1) begin
			 counter_m0_time <= 0;  //m0 cyc & stb
			 counter_m0_ack  <= 0;  //m0 ack
			 counter_m1_time <= 0;  //m1 cyc & stb
			 counter_m1_ack  <= 0;  //m1 ack
		end
		else begin
			 if ((wb.we == 1) && (wb.stb == 1)) begin
			  case(wb.adr) 
				32'h99000000: counter_m0_time <= wb.dat_o;
				32'h99000004: counter_m0_ack  <= wb.dat_o;
				32'h99000008: counter_m1_time <= wb.dat_o;
				32'h9900000c: counter_m1_ack  <= wb.dat_o;
			  endcase 
			end
			else begin
				if ((m0.cyc == 1)&&(m0.stb == 1))	counter_m0_time <= counter_m0_time + 1;
				if (m0.ack == 1)					counter_m0_ack  <= counter_m0_ack + 1;
				if ((m1.cyc == 1)&&(m1.stb == 1))	counter_m1_time <= counter_m1_time + 1;
				if (m1.ack == 1)					counter_m1_ack  <= counter_m1_ack + 1;
			end
		end
	end
   
	always @(posedge wb.clk) begin
		if (wb.rst == 1)
		  data_i_t <= 0;
		else begin
			if ((wb.stb == 1) && (wb.we == 0))	begin
				case(wb.adr) 
					32'h99000000:data_i_t <= counter_m0_time;
					32'h99000004:data_i_t <= counter_m0_ack;
					32'h99000008:data_i_t <= counter_m1_time;
					32'h9900000c:data_i_t <= counter_m1_ack;
				endcase 
			end
		end 
	end 
  
	always @(posedge wb.clk) begin
		if (wb.rst || ack_t) ack_t <= 0;
		else if (wb.stb == 1) ack_t <= 1;
		else ack_t <= 0;
	end

	assign wb.dat_i = data_i_t;
	assign wb.ack = ack_t;  
	assign 	wb.rty = 1'b0;	// not used in this course
	assign 	wb.err = 1'b0;  // not used in this course 
   
endmodule // perf_top
// Local Variables:
// verilog-library-directories:("." "or1200" "jpeg" "pkmc" "dvga" "uart" "monitor" "lab1" "dafk_tb" "eth" "wb" "leela")
// End:
