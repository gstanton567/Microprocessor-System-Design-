
module lab7(PSEN,A15,IOM,RAM1,RAM2,ROM1,ROM2,LCD,SEVSEG, RTC, A14, WR);
input PSEN /* synthesis loc="P1" */;
input A15 /* synthesis loc="P2" */;
input IOM /* synthesis loc="P3" */;
input WR /* synthesis loc="P4" */;
input A14/* synthesis loc="P5" */;
output SEVSEG /* synthesis loc="P23" */;
output LCD /* synthesis loc="P22" */;
output ROM2 /* synthesis loc="P21" */;
output ROM1 /* synthesis loc="P20" */;
output RAM2 /* synthesis loc="P19" */;
output RAM1 /* synthesis loc="P18" */;
output RTC /* synthesis loc="P17" */;
assign SEVSEG = (PSEN & !A15 & IOM & !WR & A14);
assign LCD = !(PSEN & A15 & IOM);
assign ROM2 = !(!PSEN & A15);
assign ROM1 = !(!PSEN & !A15);
assign RAM2 = !(PSEN & A15 & !IOM);
assign RAM1 = !(PSEN & !A15 & !IOM);
assign RTC = !(PSEN & !A15 & !A14 & IOM);
endmodule