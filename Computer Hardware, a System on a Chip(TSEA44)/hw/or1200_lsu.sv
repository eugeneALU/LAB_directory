
//////////////////////////////////////////////////////////////////////
////                                                              ////
////  OR1200's Load/Store unit                                    ////
////                                                              ////
////  This file is part of the OpenRISC 1200 project              ////
////  http://www.opencores.org/cores/or1k/                        ////
////                                                              ////
////  Description                                                 ////
////  Interface between CPU and DC.                               ////
////                                                              ////
////  To Do:                                                      ////
////   - make it smaller and faster                               ////
////                                                              ////
////  Author(s):                                                  ////
////      - Damjan Lampret, lampret@opencores.org                 ////
////                                                              ////
//////////////////////////////////////////////////////////////////////
////                                                              ////
//// Copyright (C) 2000 Authors and OPENCORES.ORG                 ////
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
//
// CVS Revision History
//
// $Log: or1200_lsu.v,v $
// Revision 1.5  2004/04/05 08:29:57  lampret
// Merged branch_qmem into main tree.
//
// Revision 1.4  2002/03/29 15:16:56  lampret
// Some of the warnings fixed.
//
// Revision 1.3  2002/02/11 04:33:17  lampret
// Speed optimizations (removed duplicate _cyc_ and _stb_). 
// Fixed D/IMMU cache-inhibit attr.
//
// Revision 1.2  2002/01/18 07:56:00  lampret
// No more low/high priority interrupts (PICPR removed). 
// Added tick timer exception. 
// Added exception prefix (SR[EPH]). Fixed single-step bug whenreading NPC.
//
// Revision 1.1  2002/01/03 08:16:15  lampret
// New prefixes for RTL files, prefixed module names. 
// Updated cache controllers and MMUs.
//
// Revision 1.9  2001/11/30 18:59:47  simons
// *** empty log message ***
//
// Revision 1.8  2001/10/21 17:57:16  lampret
// Removed params from generic_XX.v. 
// Added translate_off/on in sprs.v and id.v. 
// Removed spr_addr from dc.v and ic.v. Fixed CR+LF.
//
// Revision 1.7  2001/10/14 13:12:09  lampret
// MP3 version.
//
// Revision 1.1.1.1  2001/10/06 10:18:36  igorm
// no message
//
// Revision 1.2  2001/08/09 13:39:33  lampret
// Major clean-up.
//
// Revision 1.1  2001/07/20 00:46:03  lampret
// Development version of RTL. Libraries are missing.
//
//

// synopsys translate_off
`include "include/timescale.v"
// synopsys translate_on
`include "include/or1200_defines.v"

module or1200_lsu
  (`ifdef OR1200_SBIT_IMPL
   clk, rst, pc_advance_i,
   spr_cs, spr_write, spr_addr, spr_dat_cpu, spr_dat_vlx,
   `endif
   // Internal i/f
   addrbase, addrofs, lsu_op, lsu_datain, lsu_dataout, lsu_stall, lsu_unstall,
   du_stall, except_align, except_dtlbmiss, except_dmmufault, except_dbuserr,
		  
   // External i/f to DC
   dcpu_adr_o, dcpu_cycstb_o, dcpu_we_o, dcpu_sel_o, dcpu_tag_o, dcpu_dat_o,
   dcpu_dat_i, dcpu_ack_i, dcpu_rty_i, dcpu_err_i, dcpu_tag_i
   );

   parameter dw = `OR1200_OPERAND_WIDTH;
   parameter aw = `OR1200_REGFILE_ADDR_WIDTH;

   //
   // I/O
   //

   //
   // Internal i/f
   //
`ifdef OR1200_SBIT_IMPL
   input     clk;
   input     rst;
   input     pc_advance_i;
`endif
   input [31:0] addrbase;
   input [31:0] addrofs;
   input [`OR1200_LSUOP_WIDTH-1:0] lsu_op;
   input [dw-1:0] 		   lsu_datain;
   output [dw-1:0] 		   lsu_dataout;
   output 			   lsu_stall;
   output 			   lsu_unstall;
   input                           du_stall;
   output 			   except_align;
   output 			   except_dtlbmiss;
   output 			   except_dmmufault;
   output 			   except_dbuserr;

   //
   // External i/f to DC
   //
   output [31:0] 		   dcpu_adr_o;
`ifdef OR1200_SBIT_IMPL
   output reg 			   dcpu_cycstb_o;
   input  spr_cs;
   input  spr_write;
   input [31:0] spr_addr;
   input [31:0] spr_dat_cpu;
   output [31:0] spr_dat_vlx;
`else
   output 			   dcpu_cycstb_o;
`endif
   output 			   dcpu_we_o;
   output [3:0] 		   dcpu_sel_o;
   output [3:0] 		   dcpu_tag_o;
   output [31:0] 		   dcpu_dat_o;
   input [31:0] 		   dcpu_dat_i;
   input 			   dcpu_ack_i;
   input 			   dcpu_rty_i;
   input 			   dcpu_err_i;
   input [3:0] 			   dcpu_tag_i;

   //
   // Internal wires/regs
   //
   reg [3:0] 			   dcpu_sel_o;
   `ifdef OR1200_SBIT_IMPL
   wire [dw-1:0] 		   vlx_dataout;
   wire [dw-1:0] 		   vlx_datain;
   wire 			   set_bit_op;
   
   wire [31:0] 			   vlx_addr;
   wire [dw-1:0] 		   reg2mem_data;
   wire 			   store_byte_strobe;
   wire 			   vlx_stall_cpu;
   wire [`OR1200_LSUOP_WIDTH-1:0]  reg2mem_op;
   wire 			   vlx_set_bit_op;
   `endif

   //
   // Internal I/F assignments
   //
   assign 			   lsu_unstall = dcpu_ack_i;
   assign 			   except_align = ((lsu_op == `OR1200_LSUOP_SH) | (lsu_op == `OR1200_LSUOP_LHZ) | (lsu_op == `OR1200_LSUOP_LHS)) & dcpu_adr_o[0]
				   |  ((lsu_op == `OR1200_LSUOP_SW) | (lsu_op == `OR1200_LSUOP_LWZ) | (lsu_op == `OR1200_LSUOP_LWS)) & |dcpu_adr_o[1:0];
   assign 			   except_dtlbmiss = dcpu_err_i & (dcpu_tag_i == `OR1200_DTAG_TE);
   assign 			   except_dmmufault = dcpu_err_i & (dcpu_tag_i == `OR1200_DTAG_PE);
   assign 			   except_dbuserr = dcpu_err_i & (dcpu_tag_i == `OR1200_DTAG_BE);

   //
   // External I/F assignments
   //
   
   `ifdef OR1200_SBIT_IMPL
   assign 			   lsu_stall = (dcpu_rty_i & dcpu_cycstb_o) | vlx_stall_cpu;
   assign 			   dcpu_adr_o = set_bit_op ? vlx_addr : addrbase + addrofs;

   always_comb begin
      if(set_bit_op) begin
	 //Here you must add code to handle the dcpu_cycstb_o correctly. It should be high
	 //when data is written to memory.
	 	dcpu_cycstb_o <= store_byte_strobe;
      end
      else begin
	 dcpu_cycstb_o <= du_stall | lsu_unstall | (except_align ? 1'b0 : |lsu_op);
      end
   end

   `else // !`ifdef OR1200_SBIT_IMPL
   assign 			   lsu_stall = (dcpu_rty_i & dcpu_cycstb_o);
   assign 			   dcpu_adr_o = addrbase + addrofs;
   assign 			   dcpu_cycstb_o = du_stall | lsu_unstall | (except_align ? 1'b0 : |lsu_op);
   `endif

   assign 			   dcpu_we_o = lsu_op[3];
   assign 			   dcpu_tag_o = dcpu_cycstb_o ? `OR1200_DTAG_ND : `OR1200_DTAG_IDLE;

   always_comb
     casex({lsu_op, dcpu_adr_o[1:0]})
       {`OR1200_LSUOP_SB, 2'b00} : dcpu_sel_o = 4'b1000;	// store byte
       {`OR1200_LSUOP_SB, 2'b01} : dcpu_sel_o = 4'b0100;
       {`OR1200_LSUOP_SB, 2'b10} : dcpu_sel_o = 4'b0010;
       {`OR1200_LSUOP_SB, 2'b11} : dcpu_sel_o = 4'b0001;
       {`OR1200_LSUOP_SH, 2'b00} : dcpu_sel_o = 4'b1100;	// store half word
       {`OR1200_LSUOP_SH, 2'b10} : dcpu_sel_o = 4'b0011;
       {`OR1200_LSUOP_SW, 2'b00} : dcpu_sel_o = 4'b1111;	// store word 
       {`OR1200_LSUOP_LBZ, 2'b00}, {`OR1200_LSUOP_LBS, 2'b00} : dcpu_sel_o = 4'b1000;
       {`OR1200_LSUOP_LBZ, 2'b01}, {`OR1200_LSUOP_LBS, 2'b01} : dcpu_sel_o = 4'b0100;
       {`OR1200_LSUOP_LBZ, 2'b10}, {`OR1200_LSUOP_LBS, 2'b10} : dcpu_sel_o = 4'b0010;
       {`OR1200_LSUOP_LBZ, 2'b11}, {`OR1200_LSUOP_LBS, 2'b11} : dcpu_sel_o = 4'b0001;
       {`OR1200_LSUOP_LHZ, 2'b00}, {`OR1200_LSUOP_LHS, 2'b00} : dcpu_sel_o = 4'b1100;
       {`OR1200_LSUOP_LHZ, 2'b10}, {`OR1200_LSUOP_LHS, 2'b10} : dcpu_sel_o = 4'b0011;
       {`OR1200_LSUOP_LWZ, 2'b00}, {`OR1200_LSUOP_LWS, 2'b00} : dcpu_sel_o = 4'b1111;
   `ifdef OR1200_SBIT_IMPL
       //It has the same semantics as a normal wishbone sel_o signal.
       {`OR1200_LSUOP_SBIT, 2'b00} : dcpu_sel_o = 4'b1000;	// store byte
       {`OR1200_LSUOP_SBIT, 2'b01} : dcpu_sel_o = 4'b0100;
       {`OR1200_LSUOP_SBIT, 2'b10} : dcpu_sel_o = 4'b0010;
       {`OR1200_LSUOP_SBIT, 2'b11} : dcpu_sel_o = 4'b0001;
   `endif
       default : dcpu_sel_o = 4'b0000;
     endcase

`ifdef OR1200_SBIT_IMPL
   
   assign set_bit_op = (lsu_op == `OR1200_LSUOP_SBIT);

   assign vlx_datain = {16'b0,addrbase[15:0]}; 
   
   or1200_vlx_top or1200_vlx_top
     (
      .stall_cpu_o(vlx_stall_cpu),
      .vlx_addr_o(vlx_addr),
      .dat_o	(vlx_dataout),
      .store_byte_o(store_byte_strobe),
      .clk_i	(clk),
      .rst_i	(rst),
      .ack_i	(dcpu_ack_i),
      .dat_i	(vlx_datain),
      .set_bit_op_i(vlx_set_bit_op),
      .num_bits_to_write_i(lsu_datain[4:0]),
         
      .spr_cs(spr_cs),
      .spr_write(spr_write),
      .spr_addr(spr_addr[1:0]),
      .spr_dat_i(spr_dat_cpu),
      .spr_dat_o(spr_dat_vlx)
      );

   assign vlx_set_bit_op = set_bit_op & pc_advance_i;
   
   
   or1200_mem2reg or1200_mem2reg(
				 .addr(dcpu_adr_o[1:0]),
				 .lsu_op(lsu_op),
				 .memdata(dcpu_dat_i),
				 .regdata(lsu_dataout)
				 );


   
   or1200_reg2mem or1200_reg2mem(
				 .addr(dcpu_adr_o[1:0]),
				 .lsu_op(reg2mem_op),
				 .regdata(reg2mem_data),
				 .memdata(dcpu_dat_o)
				 );

   assign reg2mem_data = set_bit_op ? vlx_dataout : lsu_datain;
   assign reg2mem_op   = set_bit_op ? `OR1200_LSUOP_SB : lsu_op;
   
   `else
   
   //
   // Instantiation of Memory-to-regfile aligner
   //
   or1200_mem2reg or1200_mem2reg(
				 .addr(dcpu_adr_o[1:0]),
				 .lsu_op(lsu_op),
				 .memdata(dcpu_dat_i),
				 .regdata(lsu_dataout)
				 );


   or1200_reg2mem or1200_reg2mem(
				 .addr(dcpu_adr_o[1:0]),
				 .lsu_op(lsu_op),
				 .regdata(lsu_datain),
				 .memdata(dcpu_dat_o)
				 );

   `endif // !`ifdef OR1200_SBIT_IMPL
   
   //
   // Instantiation of Regfile-to-memory aligner
   //

endmodule
// Local Variables:
// verilog-library-directories:("." ".." "../or1200" "../jpeg" "../pkmc" "../dvga" "../uart" "../monitor" "../lab1" "../dafk_tb" "../eth" "../wb" "../leela")
// End:
