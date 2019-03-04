`timescale 1ns / 1ps

// Testbench for lab 0 zedboard

module lab0_zed_tb();  

   reg clk_i;
   reg rst_i;
   reg  send_i;
   reg [7:0] switch_i;
   wire [7:0] led_o;
   wire jumper;
   
   // Instantiate a UART
   lab0_zed uart(.clk_i(clk_i), .rst_i(rst_i), .rx_i(jumper), .tx_o(jumper),  
                .led_o(led_o), .switch_i(switch_i), .send_i(send_i)) ;
   
   always #5 clk_i = ~clk_i;  // 100 MHz clock
   
   initial
     begin
        clk_i = 1'b0;
        switch_i = 8'b11110101; 
        rst_i = 1'b1;
        send_i = 1'b0;
        #100 rst_i = 1'b0; 
        #5000 send_i = 1'b1;
	#100000 send_i = 1'b0;
		switch_i = 8'b00001010;
	#50 send_i = 1'b1;
     end
   
endmodule

// Local Variables:
// verilog-library-directories:("." ".." "../or1200" "../jpeg" "../pkmc" "../dvga" "../uart" "../monitor" "../lab1" "../dafk_tb" "../eth" "../wb" "../leela")
// End: