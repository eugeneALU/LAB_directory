//////////////////////////////////////////////////////////////////////
////                                                              ////
////  DAFK JPEG Accelerator top                                   ////
////                                                              ////
////  This file is part of the DAFK Lab Course                    ////
////  http://www.da.isy.liu.se/courses/tsea02                     ////
////                                                              ////
////  Description                                                 ////
////  DAFK JPEG Top Level SystemVerilog Version                   ////
////                                                              ////
////  To Do:                                                      ////
////   - make it smaller and faster                               ////
////                                                              ////
////  Author:                                                     ////
////      - Olle Seger, olles@isy.liu.se                          ////
////      - Andreas Ehliar, ehliar@isy.liu.se                     ////
////                                                              ////
//////////////////////////////////////////////////////////////////////
////                                                              ////
//// Copyright (C) 2005-2007 Authors                              ////
////                                                              ////
//// This source file may be used and distributed without         ////
//// restriction provided that this copyright statement is not    ////
//// removed from the file and that any derivative work contains  ////
//// the original copyright notice and the associated disclaimer. ////
////                                                              ////
//// This source file is free software; you can redistribute it   ////
//// and/or modify it under the terms of the GNU Lesser General   ////
//// Public License as published by the Free Software Foundation; ////
//// either version 2.1 of the License, or (at your option) any   ////
//// later version.                                               ////
////                                                              ////
//// This source is distributed in the hope that it will be       ////
//// useful, but WITHOUT ANY WARRANTY; without even the implied   ////
//// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR      ////
//// PURPOSE.  See the GNU Lesser General Public License for more ////
//// details.                                                     ////
////                                                              ////
//// You should have received a copy of the GNU Lesser General    ////
//// Public License along with this source; if not, download it   ////
//// from http://www.opencores.org/lgpl.shtml                     ////
////                                                              ////
//////////////////////////////////////////////////////////////////////
`include "include/timescale.v"
`include "include/dafk_defines.v"

  typedef     enum {TST, CNT, RST} op_t;
  typedef struct packed
	  {logic mux1;			// DCT input select
	   logic dcten;			// DCT enable
	   logic twr;			// t_wr
	   logic trd;			// t_rd
	   logic mux2;			// data_i select
	   logic wren;			// write enable for utmem	
	   } mmem_t;

	module jpeg_top(wishbone.slave wb, wishbone.master wbm);

	logic [5:0] 		rdc;				// read addr for inmem, data to DCT
	logic [31:0] 	 	dob, ut_doa;		// inmem data_out_B(DONE) / utmem data_out_A(DON'T CARE)
	logic [31:0] 	 	doa;				// immem data_out_A
	logic [0:7][11:0] 	x;					// x input to DCT, always 12 bits, deal with first turn input which is 8 bits.
	logic [0:7][11:0]	ut;					// TransposeMEM output
	logic [0:7][15:0] 	y;   				// DCT 16 bits output(DONE)

	logic [31:0] 		dout_res;			// utmem data_out_B
	logic [31:0] 	 	q;					// utmem data_in_A(from Q2)
	logic [4:0] 		wrc;				// write addr for utmem, data from Q2

	logic 		 		csren;				// call control_reg(DONE)
	logic [7:0] 		csr;				// control_reg {bit0 START , bit7 RDY}
	mmem_t 		 		mmem;				// control signal

	logic 		 		ce_in, ce_ut;		// call inmem(DONE) / call utmem(DONE)
	logic 		 		dmaen;				// call DMA(DONE)
	logic 		 		dct_busy;			// 
	logic 		 		dma_start_dct;		// 

	// ********************************************
	// *          Wishbone interface              *
	// ********************************************
	assign 	ce_in = wb.stb && (wb.adr[12:11]==2'b00); // Input mem
	assign 	ce_ut = wb.stb && (wb.adr[12:11]==2'b01); // Output mem
	assign	csren = wb.stb && (wb.adr[12:11]==2'b10); // Control reg
	assign	dmaen = wb.stb && (wb.adr[12:11]==2'b11); // DMA control


	// ********************************************
	// *           INMEM TO DCT PART              *
	// ********************************************
	logic [31:0] 		dob_old;			// reg for store inmem output
	logic 		 		clock_div2;			// 2 times slower clock
/*
	// * @Name   : COUNTER1, counter to divide the clock
	// * @Relate : clock_div2 , csr[0](START bit)
	// * @Function : divide clock by 2
	always @(posedge wb.clk) begin
		clock_div2 <= 0;
		if(wb.rst)
			clock_div2 <= 0;
		else if (csr[0]) begin
			clock_div2 <= !clock_div2;
		end
	end

	// * @Name   : COUNTER2, count for inmem addr
	// * @Relate : rdc, csr[0]
	always @(posedge wb.clk) begin
		if(wb.rst || !csr[0])
			rdc <= 6'b0;				
		else if (csr[0]) begin
			rdc <= rdc + 6'd1;		
		end
	end
*/
	// * @Name   : REG1 , store a piece from dob
	// * @Relate : dob, dob_old
	always @(posedge wb.clk) begin
		dob_old <= dob_old;
		if(wb.rst)
			dob_old <= 32'b0;
		else if (clock_div2) begin
			dob_old <= dob;
		end
	end

	// * @Name   : Input to DCT (include one MUX)
	// * @Relate : x , dob, dob_old, ut
	// * @Select : (1) from inmem ; (0) from TransposeMEM
	always @(posedge wb.clk) begin
		if(wb.rst)
			x <= 96'b0;
		else begin
			if (mmem.mux1)
				x <=  {
					  {{4{dob_old[31]}}, dob_old[31:24]},		//[0]
					  {{4{dob_old[23]}}, dob_old[23:16]},		//[1]
				      {{4{dob_old[15]}}, dob_old[15:8]},		//[2]
					  {{4{dob_old[7]}}, dob_old[7:0]},			//[3]
					  {{4{dob[31]}}, dob[31:24]},				//[4]
					  {{4{dob[23]}}, dob[23:16]},				//[5]
					  {{4{dob[15]}}, dob[15:8]},				//[6]
					  {{4{dob[7]}}, dob[7:0]}					//[7]
					  };
			else
				x <= ut;
		end
	end

	// * @Name : 8 point DCT
	// * @Relate : dcten
	// * @Take : 4 clock cycle
	dct dct0
	(.y(y), .x(x), 
	 .clk_i(wb.clk), .en(mmem.dcten)
	);

	// * @Name : transpose memory
	// * @Relate : trd, twr
	transpose tmem
	 (.clk(wb.clk), .rst(wb.rst), 
	  .wr(mmem.twr) , .rd(mmem.trd), 
	  .in({y[7][11:0],y[6][11:0],y[5][11:0],y[4][11:0],y[3][11:0],y[2][11:0],y[1][11:0],y[0][11:0]}), 
	  .ut(ut));

	// ********************************************
	// *            FSM for DCT, TRANSMEM         *
	// ********************************************
	parameter D_IDLE 		= 3'd0;	
	parameter D_WRITE_INMEM = 3'd1;
	parameter D_DCT1 		= 3'd2;
	parameter D_REST 		= 3'd3;
	parameter D_DCT2 		= 3'd4;

	parameter T_IDLE 		= 3'd0;	
	parameter T_START_WRITE = 3'd1;
	parameter T_END_WRITE 	= 3'd2;
	parameter T_START_READ	= 3'd3;
	parameter T_END_READ 	= 3'd4;

	
	logic [2:0] state_dct;	
	logic [2:0] state_trans;
	logic [5:0] cycle_count_dct;
	logic [4:0] cycle_count_trans;
	logic [4:0] count_trans_rd;

	// * @Name   : FSM, for DCT
	// * @Relate : ce_in, mmem.mux1, mmem.dcten
	always @(posedge wb.clk) begin
		if(wb.rst) begin
			//dct_busy <= 0;
			cycle_count_dct <= 0;	
			mmem.mux1 <= 0;	
			mmem.dcten <= 0;
			state_dct <= D_IDLE;
		end
		else begin
			case(state_dct) 
				D_IDLE: begin
					//dct_busy <= 0;
					mmem.dcten <= 0;
					cycle_count_dct <= 0;
					if((ce_in && wb.we) || (dmaen && wb.we))
						state_dct <= D_WRITE_INMEM;					
				end
				D_WRITE_INMEM: begin
					if(csr[0] || dct_busy) begin
						//dct_busy <= 1;
						state_dct <= D_DCT1;
					end
				end
				D_DCT1: begin
					mmem.mux1 <= 1;
					mmem.dcten <= 1;						// can let dcten = clock_div2 which will let DCT don't do extra work but delay the time we get our output
					cycle_count_dct <= cycle_count_dct + 1;

					if(cycle_count_dct == 16) begin			//15
						cycle_count_dct <= 0;
						mmem.mux1 <= 0;
						state_dct <= D_REST;
					end
				end
				D_REST: begin
					cycle_count_dct <= cycle_count_dct + 1;
					if(cycle_count_dct == 4) begin			
						cycle_count_dct <= 0;
						state_dct <= D_DCT2;
					end
				end
				D_DCT2: begin
					mmem.mux1 <= 0;
					mmem.dcten <= 1;
					cycle_count_dct <= cycle_count_dct + 1;

					if(cycle_count_dct == 32) begin			//might need to be 33 (DCT work for one more  cycle to get stable y??)
						//dct_busy <= 0;						
						cycle_count_dct <= 0;
						state_dct <= D_IDLE;
					end
				end
				default: begin
					state_dct <= D_IDLE;
				end	
			endcase 
		end
	end

	// * @Name   : FSM, for TransposeMEM WRITE
	// * @Relate : state_dct, state_trans, mmem.twr, mmem.trd
	always @(posedge wb.clk) begin
		if(wb.rst) begin
			mmem.twr <= 0;
			mmem.trd <= 0;
			cycle_count_trans <= 0;
			count_trans_rd <= 0;
			state_trans <= T_IDLE;
		end
		else begin
			case(state_trans)
				T_IDLE: begin	
					mmem.twr <= 0;	
					mmem.trd <= 0;
					if (state_dct == D_DCT1)
						cycle_count_trans <= cycle_count_trans + 1;
					else 
						cycle_count_trans <= 0;
					
					if (cycle_count_trans == 5) begin
						mmem.twr <= 1;		
						cycle_count_trans <= 0;	
						state_trans <= T_START_WRITE;			
					end
				end
				T_START_WRITE: begin
					mmem.twr <= !mmem.twr;
					cycle_count_trans <= cycle_count_trans + 1;
					if (cycle_count_trans == 14) begin		
						cycle_count_trans <= 0;
						state_trans <= T_END_WRITE;
					end
				end
				T_END_WRITE: begin
					mmem.twr <= 0;
					mmem.trd <= 1;
					cycle_count_trans <= 0;	
					state_trans <= T_START_READ;
				end
				T_START_READ: begin
					mmem.trd <= 0;
					cycle_count_trans <= cycle_count_trans + 1;
					count_trans_rd <= count_trans_rd + 1;
					if (cycle_count_trans == 3) begin			//start reading every 4 clock cycle for Q2 has enough time to execute
						mmem.trd <= 1;
						cycle_count_trans <= 0;
					end

					if (count_trans_rd  == 30) begin			//8(row)* 4(clock per row) (between 28-30 prevent mmem.trd to be setted)
						state_trans <= T_END_READ;
					end
				end
				T_END_READ: begin
					mmem.trd <= 0;
					count_trans_rd <= 0;
					cycle_count_trans <= 0;
					state_trans <= T_IDLE;
				end
				default: begin
					cycle_count_trans <= 0;
					state_trans <= T_IDLE;
				end
			endcase
		end
	end				


	// ********************************************
	// *               Q2 to UTMEM                *
	// ********************************************
	parameter U_IDLE	   = 2'd0;
	parameter U_START_READ = 2'd1;
	parameter U_END_READ   = 2'd2;

	logic [0:31][31:0]reciprocals = { {16'd2048, 16'd2731},  {16'd2341, 16'd2341}, {16'd1820, 16'd1365}, {16'd669, 16'd455},
									  {16'd2979, 16'd2731},  {16'd2521, 16'd1928}, {16'd1489, 16'd936},  {16'd512, 16'd356},
									  {16'd3277, 16'd2341},  {16'd2048, 16'd1489}, {16'd886, 16'd596},   {16'd420, 16'd345},
									  {16'd2048, 16'd1725},  {16'd1365, 16'd1130}, {16'd565, 16'd512},   {16'd377, 16'd334},
									  {16'd1365, 16'd1260},  {16'd819,  16'd643},  {16'd482, 16'd405},   {16'd318, 16'd293},
									  {16'd819,  16'd565},   {16'd575,  16'd377},  {16'd301, 16'd315},   {16'd271, 16'd328},
									  {16'd643,  16'd546},   {16'd475,  16'd410},  {16'd318, 16'd290},   {16'd273, 16'd318},
									  {16'd537,  16'd596},   {16'd585,  16'd529},  {16'd426, 16'd356},   {16'd324, 16'd331}};
	logic [4:0] reci_count;		//0-31
	logic [31:0]reci_input;
	logic [1:0] xi_count;	 	//0-3
	logic [31:0]xi_input;
		
	logic [1:0] state_utmem;
	logic 		start_wrc;
	logic [4:0] cycle_count_utmem;


	// * @Name   : FSM, for UTMEM
	// * @Relate : state_utmem, start_wrc
	always @(posedge wb.clk) begin
		if(wb.rst) begin
			start_wrc <= 0;
			mmem.wren <= 0;
			cycle_count_utmem <= 0;
			state_utmem <= U_IDLE;	
		end
		else begin
			case(state_utmem)
				U_IDLE: begin
					mmem.wren <= 0;
					start_wrc <= 0;
					if(state_trans == T_START_READ) 
						cycle_count_utmem <= cycle_count_utmem + 1;
					else 
						cycle_count_utmem <= 0;

					if (cycle_count_utmem == 5) begin
						start_wrc <= 1;
						mmem.wren <= 1;
						cycle_count_utmem <= 0;
						state_utmem <= U_START_READ;
					end
				end
				U_START_READ: begin
					cycle_count_utmem <= cycle_count_utmem + 1;
					if (cycle_count_utmem == 31) begin				//need to check 31 or 32??
						cycle_count_utmem <= 0;
						mmem.wren <= 0;
						start_wrc <= 0;
						state_utmem <= U_END_READ;
					end
				end
				U_END_READ: begin
					state_utmem <= U_IDLE;
				end
				default: begin
					cycle_count_utmem <= 0;
					state_utmem <= U_IDLE;	
				end
			endcase
		end
	end	

	// * @Name   : COUNTER3, count for utmem addr
	// * @Relate : wrc, start_wrc
	always @(posedge wb.clk) begin
		if(wb.rst || !start_wrc)		
			wrc <= 5'b0;				
		else if (start_wrc) begin
			wrc <= wrc + 5'd1;		
		end
	end

	// * @Name   : COUNTER4, count for rec_i
	// * @Relate : reci_count, reci_input
	always @(posedge wb.clk) begin
		if(wb.rst || !start_wrc)		
			reci_count <= 4'b0;				
		else if (start_wrc) begin
			reci_count <= reci_count + 4'd1;		
		end
	end

	// * @Name   : COUNTER5, count for x_i(quantize block)
	// * @Relate : xi_count, xi_input
	always @(posedge wb.clk) begin
		if(wb.rst || !start_wrc)		
			xi_count <= 2'b0;				
		else if (start_wrc) begin
			xi_count <= xi_count + 2'd1;		
		end
	end

	// * @Name   : MUX, input for x_i(quantize block)
	// * @Relate : xi_count, xi_input
	always_comb begin
		case(xi_count) 
			2'd0:		xi_input <= {y[0],y[1]};
			2'd1:		xi_input <= {y[2],y[3]};
			2'd2:		xi_input <= {y[4],y[5]};
			2'd3:		xi_input <= {y[6],y[7]};
			default:	xi_input <= {y[0],y[1]};
		endcase
	end

	assign reci_input = reciprocals[reci_count];


	// * @Name : quantization 2 byte
	// * @Relate : y(from DCT2), q(to utmem)
	// * @Take : 2 byte per cycle, total need 4 cycle
	q2 q2block
	(.x_o(q), 
	 .x_i(xi_input), .rec_i(reci_input)
	);

	// ********************************************
	// *                 CSR                      *
	// ********************************************
	parameter C_IDLE = 2'd0;
	parameter C_DCT  = 2'd1;
	parameter C_END  = 2'd2;

	logic [1:0] state_csr;	

	// * @Name   : FSM for CSR
	// * @Relate : csren, csr, state_csr, mmem.mux2
	always @(posedge wb.clk) begin
		if(wb.rst) begin
			csr <= 8'b0;
			mmem.mux2 <= 1'b1;
			dct_busy <= 0;
			state_csr <= C_IDLE;
		end
		else begin
			case(state_csr)
				C_IDLE: begin
					rdc <= 6'b0;
					clock_div2 <= 0;
					if(csren && wb.we) begin
						csr <= wb.dat_o[31:24];				//write START bit
						state_csr <= C_DCT;
					end
		
					if(dma_start_dct) begin
						dct_busy <= 1;
						state_csr <= C_DCT;
					end
				end
				C_DCT: begin
					rdc <= rdc + 6'd1;						// for inmem addr
					clock_div2 <= !clock_div2;				// for dob_old 
					//csr[6:1] <= csr[6:1] + 1;				// DCT timestamps
					mmem.mux2 <= 1'b0;						// read csr
					if (state_utmem == U_END_READ) begin
						csr <= 8'b10000000;					//clear START bit and write RDY bit
						state_csr <= C_END;
					end
				end
				C_END: begin
					dct_busy <= 0;
					if(csren && wb.we) begin
						csr <= wb.dat_o[31:24];				//write CSR
					end			
	
					if (ce_ut) begin						//try to accsee utmem
						mmem.mux2 <= 1'b1;					//read utmem
						state_csr <= C_IDLE;
					end
				end
				default: begin
					mmem.mux2 <= 1'b1;
					state_csr <= C_IDLE;
				end	
			endcase
		end
	end
/*
	// * @Name   : COUNTER1, counter to divide the clock
	// * @Relate : clock_div2 , csr[0](START bit)
	// * @Function : divide clock by 2
	always @(posedge wb.clk) begin
		clock_div2 <= 0;
		if(wb.rst)
			clock_div2 <= 0;
		else if (state_csr == C_DCT) begin
			clock_div2 <= !clock_div2;
		end
	end
*/
	/*// * @Name   : COUNTER2, count for inmem addr
	// * @Relate : rdc, csr[0]
	always @(posedge wb.clk) begin
		if(wb.rst || !csr[0])
			rdc <= 6'b0;				
		else if (state_csr == C_DCT) begin
			rdc <= rdc + 6'd1;		
		end
	end*/

	// ********************************************
	// *               WB.ACK                     *
	// ********************************************
	// * @Relate : wb.ack, wb.stb, wb.cyc
	logic 		ack_tmp;

	always @(posedge wb.clk) begin
		if(wb.rst || ack_tmp == 1)
	 		ack_tmp <= 0;
		else begin
			if (wb.stb)
				ack_tmp <= 1;
			else
				ack_tmp <= 0;
			end 
		end

	assign wb.ack = ack_tmp;
	assign wb.err = 1'b0;
	assign wb.rty = 1'b0;	

	// ********************************************
	// *                   DMA                    *
	// ********************************************
	// Signals to the blockrams...
	logic [31:0] dma_bram_data;
	logic [8:0]  dma_bram_addr;
	logic        dma_bram_we;

	logic [31:0] bram_data;
	logic [8:0]  bram_addr;
	logic        bram_we;
	logic        bram_ce;

	logic [31:0] wb_dma_dat;

	jpeg_dma dma
	(
	  .clk_i(wb.clk), .rst_i(wb.rst),

	  .wb_adr_i	(wb.adr),
	  .wb_dat_i	(wb.dat_o),
	  .wb_we_i	(wb.we),
	  .dmaen_i	(dmaen),
	  .wb_dat_o	(wb_dma_dat),

	  .wbm(wbm),
	  
	  .dma_bram_data		(dma_bram_data[31:0]),
	  .dma_bram_addr		(dma_bram_addr[8:0]),
	  .dma_bram_we			(dma_bram_we),

	  .start_dct (dma_start_dct),
	  .dct_busy (dct_busy)
	);

	// ********************************************
	// *         SIGNAL TO BLOCK RAM              *
	// ********************************************
	always_comb begin
		if (dma_bram_we) begin
			bram_we <= dma_bram_we;
			bram_ce <= dma_bram_we;
			bram_addr <= dma_bram_addr[8:0];								
			bram_data <= dma_bram_data[31:0] ^ 32'h80808080;				//-128 for each piexl in hardware
		end 
		else begin
			bram_we <= ce_in;
			bram_ce <= ce_in;
			bram_addr <= wb.adr[10:2];							
			bram_data <= wb.dat_o ^ 32'h80808080;				//-128 for each piexl in hardware
		end
	end

	RAMB16_S36_S36 #(.SIM_COLLISION_CHECK("NONE")) inmem
	 (// WB read & write
	  .CLKA(wb.clk), .SSRA(wb.rst),
	  .ADDRA(bram_addr),
	  .DIA(bram_data), .DIPA(4'h0), 
	  .ENA(bram_ce), .WEA(bram_we), 
	  .DOA(doa), .DOPA(),
	  // DCT read
	  .CLKB(wb.clk), .SSRB(wb.rst),
	  .ADDRB({3'h0,rdc}),
	  .DIB(32'h0), .DIPB(4'h0), 
	  .ENB(1'b1),.WEB(1'b0), 
	  .DOB(dob), .DOPB());

	RAMB16_S36_S36 #(.SIM_COLLISION_CHECK("NONE")) utmem
	 (// DCT write
	  .CLKA(wb.clk), .SSRA(wb.rst),
	  .ADDRA({4'h0,wrc}),
	  .DIA(q), .DIPA(4'h0), .ENA(1'b1),							//input xi_input to omit Q2
	  .WEA(mmem.wren), .DOA(ut_doa), .DOPA(),
	  // WB read & write
	  .CLKB(wb.clk), .SSRB(wb.rst),
	  .ADDRB(wb.adr[10:2]),
	  .DIB(wb.dat_o), .DIPB(4'h0), .ENB(ce_ut),
	  .WEB(wb.we), .DOB(dout_res), .DOPB());

	// ********************************************
	// *                WB.DAT_i                  *
	// ********************************************
	logic [31:0] dat_i_tmp;
	
	// * @Name   : MUX, output for wb.dat_i
	// * @Relate : wb.dat_i, mmem.mux2, csr, dout_res
	always_comb begin
		if(wb.adr[12:11]==2'b11) begin		
			dat_i_tmp <= wb_dma_dat;
		end
		else begin
			case(mmem.mux2) 
				1'b0:		dat_i_tmp <= {csr, 24'b0};
				1'b1:		dat_i_tmp <= dout_res;
				default:	dat_i_tmp <= {csr, 24'b0};
			endcase
		end
	end
	assign wb.dat_i = dat_i_tmp;

	endmodule

// Local Variables:
// verilog-library-directories:("." ".." "../or1200" "../jpeg" "../pkmc" "../dvga" "../uart" "../monitor" "../lab1" "../dafk_tb" "../eth" "../wb" "../leela")
// End:
