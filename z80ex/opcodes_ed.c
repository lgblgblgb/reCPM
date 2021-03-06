/* autogenerated from ./opcodes_ed.dat, do not edit */

/*IN B,(C)*/
static void op_ED_0x40(void)
{
	IN(B,BC, /*rd*/5);
	T_WAIT_UNTIL(8);
	return;
}

/*OUT (C),B*/
static void op_ED_0x41(void)
{
	OUT(BC,B, /*wr*/5);
	T_WAIT_UNTIL(8);
	return;
}

/*SBC HL,BC*/
static void op_ED_0x42(void)
{
	SBC16(HL,BC);
	T_WAIT_UNTIL(11);
	return;
}

/*LD (@),BC*/
static void op_ED_0x43(void)
{
	temp_addr.b.l=READ_OP();
	temp_addr.b.h=READ_OP();
	LD_RP_TO_ADDR_MPTR_16(temp_word.w,BC, temp_addr.w);
	WRITE_MEM(temp_addr.w,temp_word.b.l,10);
	WRITE_MEM(temp_addr.w+1,temp_word.b.h,13);
	T_WAIT_UNTIL(16);
	return;
}

/*NEG*/
static void op_ED_0x44(void)
{
	NEG();
	T_WAIT_UNTIL(4);
	return;
}

/*RETN*/
static void op_ED_0x45(void)
{
	RETN(/*rd*/4,7);
	T_WAIT_UNTIL(10);
	return;
}

/*IM 0*/
static void op_ED_0x46(void)
{
	IM_(IM0);
	T_WAIT_UNTIL(4);
	return;
}

/*LD I,A*/
static void op_ED_0x47(void)
{
	LD(I,A);
	T_WAIT_UNTIL(5);
	return;
}

/*IN C,(C)*/
static void op_ED_0x48(void)
{
	IN(C,BC, /*rd*/5);
	T_WAIT_UNTIL(8);
	return;
}

/*OUT (C),C*/
static void op_ED_0x49(void)
{
	OUT(BC,C, /*wr*/5);
	T_WAIT_UNTIL(8);
	return;
}

/*ADC HL,BC*/
static void op_ED_0x4a(void)
{
	ADC16(HL,BC);
	T_WAIT_UNTIL(11);
	return;
}

/*LD BC,(@)*/
static void op_ED_0x4b(void)
{
	temp_addr.b.l=READ_OP();
	temp_addr.b.h=READ_OP();
	READ_MEM(temp_word.b.l,temp_addr.w,10);
	READ_MEM(temp_word.b.h,temp_addr.w+1,13);
	LD_RP_FROM_ADDR_MPTR_16(BC,temp_word.w, temp_addr.w);
	T_WAIT_UNTIL(16);
	return;
}

/*NEG*/
static void op_ED_0x4c(void)
{
	NEG();
	T_WAIT_UNTIL(4);
	return;
}

/*RETI*/
static void op_ED_0x4d(void)
{
	RETI(/*rd*/4,7);
	T_WAIT_UNTIL(10);
	return;
}

/*IM 0*/
static void op_ED_0x4e(void)
{
	IM_(IM0);
	T_WAIT_UNTIL(4);
	return;
}

/*LD R,A*/
static void op_ED_0x4f(void)
{
	LD_R_A();
	T_WAIT_UNTIL(5);
	return;
}

/*IN D,(C)*/
static void op_ED_0x50(void)
{
	IN(D,BC, /*rd*/5);
	T_WAIT_UNTIL(8);
	return;
}

/*OUT (C),D*/
static void op_ED_0x51(void)
{
	OUT(BC,D, /*wr*/5);
	T_WAIT_UNTIL(8);
	return;
}

/*SBC HL,DE*/
static void op_ED_0x52(void)
{
	SBC16(HL,DE);
	T_WAIT_UNTIL(11);
	return;
}

/*LD (@),DE*/
static void op_ED_0x53(void)
{
	temp_addr.b.l=READ_OP();
	temp_addr.b.h=READ_OP();
	LD_RP_TO_ADDR_MPTR_16(temp_word.w,DE, temp_addr.w);
	WRITE_MEM(temp_addr.w,temp_word.b.l,10);
	WRITE_MEM(temp_addr.w+1,temp_word.b.h,13);
	T_WAIT_UNTIL(16);
	return;
}

/*NEG*/
static void op_ED_0x54(void)
{
	NEG();
	T_WAIT_UNTIL(4);
	return;
}

/*RETN*/
static void op_ED_0x55(void)
{
	RETN(/*rd*/4,7);
	T_WAIT_UNTIL(10);
	return;
}

/*IM 1*/
static void op_ED_0x56(void)
{
	IM_(IM1);
	T_WAIT_UNTIL(4);
	return;
}

/*LD A,I*/
static void op_ED_0x57(void)
{
	LD_A_I();
	T_WAIT_UNTIL(5);
	return;
}

/*IN E,(C)*/
static void op_ED_0x58(void)
{
	IN(E,BC, /*rd*/5);
	T_WAIT_UNTIL(8);
	return;
}

/*OUT (C),E*/
static void op_ED_0x59(void)
{
	OUT(BC,E, /*wr*/5);
	T_WAIT_UNTIL(8);
	return;
}

/*ADC HL,DE*/
static void op_ED_0x5a(void)
{
	ADC16(HL,DE);
	T_WAIT_UNTIL(11);
	return;
}

/*LD DE,(@)*/
static void op_ED_0x5b(void)
{
	temp_addr.b.l=READ_OP();
	temp_addr.b.h=READ_OP();
	READ_MEM(temp_word.b.l,temp_addr.w,10);
	READ_MEM(temp_word.b.h,temp_addr.w+1,13);
	LD_RP_FROM_ADDR_MPTR_16(DE,temp_word.w, temp_addr.w);
	T_WAIT_UNTIL(16);
	return;
}

/*NEG*/
static void op_ED_0x5c(void)
{
	NEG();
	T_WAIT_UNTIL(4);
	return;
}

/*RETI*/
static void op_ED_0x5d(void)
{
	RETI(/*rd*/4,7);
	T_WAIT_UNTIL(10);
	return;
}

/*IM 2*/
static void op_ED_0x5e(void)
{
	IM_(IM2);
	T_WAIT_UNTIL(4);
	return;
}

/*LD A,R*/
static void op_ED_0x5f(void)
{
	LD_A_R();
	T_WAIT_UNTIL(5);
	return;
}

/*IN H,(C)*/
static void op_ED_0x60(void)
{
	IN(H,BC, /*rd*/5);
	T_WAIT_UNTIL(8);
	return;
}

/*OUT (C),H*/
static void op_ED_0x61(void)
{
	OUT(BC,H, /*wr*/5);
	T_WAIT_UNTIL(8);
	return;
}

/*SBC HL,HL*/
static void op_ED_0x62(void)
{
	SBC16(HL,HL);
	T_WAIT_UNTIL(11);
	return;
}

/*LD (@),HL*/
static void op_ED_0x63(void)
{
	temp_addr.b.l=READ_OP();
	temp_addr.b.h=READ_OP();
	LD_RP_TO_ADDR_MPTR_16(temp_word.w,HL, temp_addr.w);
	WRITE_MEM(temp_addr.w,temp_word.b.l,10);
	WRITE_MEM(temp_addr.w+1,temp_word.b.h,13);
	T_WAIT_UNTIL(16);
	return;
}

/*NEG*/
static void op_ED_0x64(void)
{
	NEG();
	T_WAIT_UNTIL(4);
	return;
}

/*RETN*/
static void op_ED_0x65(void)
{
	RETN(/*rd*/4,7);
	T_WAIT_UNTIL(10);
	return;
}

/*IM 0*/
static void op_ED_0x66(void)
{
	IM_(IM0);
	T_WAIT_UNTIL(4);
	return;
}

/*RRD*/
static void op_ED_0x67(void)
{
	RRD(/*rd*/4, /*wr*/11);
	T_WAIT_UNTIL(14);
	return;
}

/*IN L,(C)*/
static void op_ED_0x68(void)
{
	IN(L,BC, /*rd*/5);
	T_WAIT_UNTIL(8);
	return;
}

/*OUT (C),L*/
static void op_ED_0x69(void)
{
	OUT(BC,L, /*wr*/5);
	T_WAIT_UNTIL(8);
	return;
}

/*ADC HL,HL*/
static void op_ED_0x6a(void)
{
	ADC16(HL,HL);
	T_WAIT_UNTIL(11);
	return;
}

/*LD HL,(@)*/
static void op_ED_0x6b(void)
{
	temp_addr.b.l=READ_OP();
	temp_addr.b.h=READ_OP();
	READ_MEM(temp_word.b.l,temp_addr.w,10);
	READ_MEM(temp_word.b.h,temp_addr.w+1,13);
	LD_RP_FROM_ADDR_MPTR_16(HL,temp_word.w, temp_addr.w);
	T_WAIT_UNTIL(16);
	return;
}

/*NEG*/
static void op_ED_0x6c(void)
{
	NEG();
	T_WAIT_UNTIL(4);
	return;
}

/*RETI*/
static void op_ED_0x6d(void)
{
	RETI(/*rd*/4,7);
	T_WAIT_UNTIL(10);
	return;
}

/*IM 0*/
static void op_ED_0x6e(void)
{
	IM_(IM0);
	T_WAIT_UNTIL(4);
	return;
}

/*RLD*/
static void op_ED_0x6f(void)
{
	RLD(/*rd*/4, /*wr*/11);
	T_WAIT_UNTIL(14);
	return;
}

/*IN_F (C)*/
static void op_ED_0x70(void)
{
	IN_F(BC, /*rd*/5);
	T_WAIT_UNTIL(8);
	return;
}

/*OUT (C),0*/
static void op_ED_0x71(void)
{
	OUT(BC,z80ex.nmos ? 0: 0xFF, /*wr*/5); /* LGB: CMOS CPU uses 0xFF here! I guess ... */
	T_WAIT_UNTIL(8);
	return;
}

/*SBC HL,SP*/
static void op_ED_0x72(void)
{
	SBC16(HL,SP);
	T_WAIT_UNTIL(11);
	return;
}

/*LD (@),SP*/
static void op_ED_0x73(void)
{
	temp_addr.b.l=READ_OP();
	temp_addr.b.h=READ_OP();
	LD_RP_TO_ADDR_MPTR_16(temp_word.w,SP, temp_addr.w);
	WRITE_MEM(temp_addr.w,temp_word.b.l,10);
	WRITE_MEM(temp_addr.w+1,temp_word.b.h,13);
	T_WAIT_UNTIL(16);
	return;
}

/*NEG*/
static void op_ED_0x74(void)
{
	NEG();
	T_WAIT_UNTIL(4);
	return;
}

/*RETN*/
static void op_ED_0x75(void)
{
	RETN(/*rd*/4,7);
	T_WAIT_UNTIL(10);
	return;
}

/*IM 1*/
static void op_ED_0x76(void)
{
	IM_(IM1);
	T_WAIT_UNTIL(4);
	return;
}

/*IN A,(C)*/
static void op_ED_0x78(void)
{
	IN(A,BC, /*rd*/5);
	T_WAIT_UNTIL(8);
	return;
}

/*OUT (C),A*/
static void op_ED_0x79(void)
{
	OUT(BC,A, /*wr*/5);
	T_WAIT_UNTIL(8);
	return;
}

/*ADC HL,SP*/
static void op_ED_0x7a(void)
{
	ADC16(HL,SP);
	T_WAIT_UNTIL(11);
	return;
}

/*LD SP,(@)*/
static void op_ED_0x7b(void)
{
	temp_addr.b.l=READ_OP();
	temp_addr.b.h=READ_OP();
	READ_MEM(temp_word.b.l,temp_addr.w,10);
	READ_MEM(temp_word.b.h,temp_addr.w+1,13);
	LD_RP_FROM_ADDR_MPTR_16(SP,temp_word.w, temp_addr.w);
	T_WAIT_UNTIL(16);
	return;
}

/*NEG*/
static void op_ED_0x7c(void)
{
	NEG();
	T_WAIT_UNTIL(4);
	return;
}

/*RETI*/
static void op_ED_0x7d(void)
{
	RETI(/*rd*/4,7);
	T_WAIT_UNTIL(10);
	return;
}

/*IM 2*/
static void op_ED_0x7e(void)
{
	IM_(IM2);
	T_WAIT_UNTIL(4);
	return;
}

/*LDI*/
static void op_ED_0xa0(void)
{
	LDI(/*rd*/4, /*wr*/7);
	T_WAIT_UNTIL(12);
	return;
}

/*CPI*/
static void op_ED_0xa1(void)
{
	CPI(/*rd*/4);
	T_WAIT_UNTIL(12);
	return;
}

/*INI*/
static void op_ED_0xa2(void)
{
	INI(/*rd*/6, /*wr*/9);
	T_WAIT_UNTIL(12);
	return;
}

/*OUTI*/
static void op_ED_0xa3(void)
{
	OUTI(/*rd*/5, /*wr*/9);
	T_WAIT_UNTIL(12);
	return;
}

/*LDD*/
static void op_ED_0xa8(void)
{
	LDD(/*rd*/4, /*wr*/7);
	T_WAIT_UNTIL(12);
	return;
}

/*CPD*/
static void op_ED_0xa9(void)
{
	CPD(/*rd*/4);
	T_WAIT_UNTIL(12);
	return;
}

/*IND*/
static void op_ED_0xaa(void)
{
	IND(/*rd*/6, /*wr*/9);
	T_WAIT_UNTIL(12);
	return;
}

/*OUTD*/
static void op_ED_0xab(void)
{
	OUTD(/*rd*/5, /*wr*/9);
	T_WAIT_UNTIL(12);
	return;
}

/*LDIR*/
static void op_ED_0xb0(void)
{
	LDIR(/*t:*/ /*t1*/12,/*t2*/17, /*rd*/4, /*wr*/7);
	return;
}

/*CPIR*/
static void op_ED_0xb1(void)
{
	CPIR(/*t:*/ /*t1*/12,/*t2*/17, /*rd*/4);
	return;
}

/*INIR*/
static void op_ED_0xb2(void)
{
	INIR(/*t:*/ /*t1*/12,/*t2*/17, /*rd*/6, /*wr*/9);
	return;
}

/*OTIR*/
static void op_ED_0xb3(void)
{
	OTIR(/*t:*/ /*t1*/12,/*t2*/17, /*rd*/5, /*wr*/9);
	return;
}

/*LDDR*/
static void op_ED_0xb8(void)
{
	LDDR(/*t:*/ /*t1*/12,/*t2*/17, /*rd*/4, /*wr*/7);
	return;
}

/*CPDR*/
static void op_ED_0xb9(void)
{
	CPDR(/*t:*/ /*t1*/12,/*t2*/17, /*rd*/4);
	return;
}

/*INDR*/
static void op_ED_0xba(void)
{
	INDR(/*t:*/ /*t1*/12,/*t2*/17, /*rd*/6, /*wr*/9);
	return;
}

/*OTDR*/
static void op_ED_0xbb(void)
{
	OTDR(/*t:*/ /*t1*/12,/*t2*/17, /*rd*/5, /*wr*/9);
	return;
}



/**/
static const z80ex_opcode_fn opcodes_ed[0x100] = {
 NULL          , NULL          , NULL          , NULL          ,
 NULL          , NULL          , NULL          , NULL          ,
 NULL          , NULL          , NULL          , NULL          ,
 NULL          , NULL          , NULL          , NULL          ,
 NULL          , NULL          , NULL          , NULL          ,
 NULL          , NULL          , NULL          , NULL          ,
 NULL          , NULL          , NULL          , NULL          ,
 NULL          , NULL          , NULL          , NULL          ,
 NULL          , NULL          , NULL          , NULL          ,
 NULL          , NULL          , NULL          , NULL          ,
 NULL          , NULL          , NULL          , NULL          ,
 NULL          , NULL          , NULL          , NULL          ,
 NULL          , NULL          , NULL          , NULL          ,
 NULL          , NULL          , NULL          , NULL          ,
 NULL          , NULL          , NULL          , NULL          ,
 NULL          , NULL          , NULL          , NULL          ,
 op_ED_0x40    , op_ED_0x41    , op_ED_0x42    , op_ED_0x43    ,
 op_ED_0x44    , op_ED_0x45    , op_ED_0x46    , op_ED_0x47    ,
 op_ED_0x48    , op_ED_0x49    , op_ED_0x4a    , op_ED_0x4b    ,
 op_ED_0x4c    , op_ED_0x4d    , op_ED_0x4e    , op_ED_0x4f    ,
 op_ED_0x50    , op_ED_0x51    , op_ED_0x52    , op_ED_0x53    ,
 op_ED_0x54    , op_ED_0x55    , op_ED_0x56    , op_ED_0x57    ,
 op_ED_0x58    , op_ED_0x59    , op_ED_0x5a    , op_ED_0x5b    ,
 op_ED_0x5c    , op_ED_0x5d    , op_ED_0x5e    , op_ED_0x5f    ,
 op_ED_0x60    , op_ED_0x61    , op_ED_0x62    , op_ED_0x63    ,
 op_ED_0x64    , op_ED_0x65    , op_ED_0x66    , op_ED_0x67    ,
 op_ED_0x68    , op_ED_0x69    , op_ED_0x6a    , op_ED_0x6b    ,
 op_ED_0x6c    , op_ED_0x6d    , op_ED_0x6e    , op_ED_0x6f    ,
 op_ED_0x70    , op_ED_0x71    , op_ED_0x72    , op_ED_0x73    ,
 op_ED_0x74    , op_ED_0x75    , op_ED_0x76    , NULL          ,
 op_ED_0x78    , op_ED_0x79    , op_ED_0x7a    , op_ED_0x7b    ,
 op_ED_0x7c    , op_ED_0x7d    , op_ED_0x7e    , NULL          ,
 NULL          , NULL          , NULL          , NULL          ,
 NULL          , NULL          , NULL          , NULL          ,
 NULL          , NULL          , NULL          , NULL          ,
 NULL          , NULL          , NULL          , NULL          ,
 NULL          , NULL          , NULL          , NULL          ,
 NULL          , NULL          , NULL          , NULL          ,
 NULL          , NULL          , NULL          , NULL          ,
 NULL          , NULL          , NULL          , NULL          ,
 op_ED_0xa0    , op_ED_0xa1    , op_ED_0xa2    , op_ED_0xa3    ,
 NULL          , NULL          , NULL          , NULL          ,
 op_ED_0xa8    , op_ED_0xa9    , op_ED_0xaa    , op_ED_0xab    ,
 NULL          , NULL          , NULL          , NULL          ,
 op_ED_0xb0    , op_ED_0xb1    , op_ED_0xb2    , op_ED_0xb3    ,
 NULL          , NULL          , NULL          , NULL          ,
 op_ED_0xb8    , op_ED_0xb9    , op_ED_0xba    , op_ED_0xbb    ,
 NULL          , NULL          , NULL          , NULL          ,
 NULL          , NULL          , NULL          , NULL          ,
 NULL          , NULL          , NULL          , NULL          ,
 NULL          , NULL          , NULL          , NULL          ,
 NULL          , NULL          , NULL          , NULL          ,
 NULL          , NULL          , NULL          , NULL          ,
 NULL          , NULL          , NULL          , NULL          ,
 NULL          , NULL          , NULL          , NULL          ,
 NULL          , NULL          , NULL          , NULL          ,
 NULL          , NULL          , NULL          , NULL          ,
 NULL          , NULL          , NULL          , NULL          ,
 NULL          , NULL          , NULL          , NULL          ,
 NULL          , NULL          , NULL          , NULL          ,
 NULL          , NULL          , NULL          , NULL          ,
 NULL          , NULL          , NULL          , NULL          ,
 NULL          , NULL          , NULL          , NULL          ,
 NULL          , NULL          , NULL          , NULL          
};
