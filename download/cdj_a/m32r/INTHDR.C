
#define	TARGET	"inthdr"
#include	"cdj.h"
#include	<hw.h>

#define	EIT_VENT	((UW*)0)
#define	INT_VENT	((UW*)0xf00100)


#if DEBUGPRINT
static	void	putstring_and_sleep(char *str)
{
	UW	bpc;
	
	D1STR(str);
	asm("mvfc %0, bpc" : "=r" (bpc));
	D1W("BPC", bpc)
	for (;;)
		;
	
}
#else
#define	putstring_and_sleep(x)	{for(;;);}
#endif


static	void	ri_hdr(void)
{
	putstring_and_sleep("RI called.\n");
}


static	void	sbi_hdr(void)
{
	putstring_and_sleep("SBI called.\n");
}


static	void	rie_hdr(void)
{
	putstring_and_sleep("RIE called.\n");
}


static	void	ae_hdr(void)
{
	putstring_and_sleep("AE called.\n");
}


static	void	trap0_hdr(void)
{
	putstring_and_sleep("trap0 called.\n");
}


static	void	trap1_hdr(void)
{
	putstring_and_sleep("trap1 called.\n");
}


static	void	trap2_hdr(void)
{
	putstring_and_sleep("trap2 called.\n");
}


static	void	trap3_hdr(void)
{
	putstring_and_sleep("trap3 called.\n");
}


static	void	trap4_hdr(void)
{
	putstring_and_sleep("trap4 called.\n");
}


static	void	trap5_hdr(void)
{
	putstring_and_sleep("trap5 called.\n");
}


static	void	trap6_hdr(void)
{
	putstring_and_sleep("trap6 called.\n");
}


static	void	trap7_hdr(void)
{
	putstring_and_sleep("trap7 called.\n");
}


static	void	trap8_hdr(void)
{
	putstring_and_sleep("trap8 called.\n");
}


static	void	trap9_hdr(void)
{
	putstring_and_sleep("trap9 called.\n");
}


static	void	trap10_hdr(void)
{
	putstring_and_sleep("trap10 called.\n");
}


static	void	trap11_hdr(void)
{
	putstring_and_sleep("trap11 called.\n");
}


static	void	trap12_hdr(void)
{
	putstring_and_sleep("trap12 called.\n");
}


static	void	trap13_hdr(void)
{
	putstring_and_sleep("trap13 called.\n");
}


static	void	trap14_hdr(void)
{
	putstring_and_sleep("trap14 called.\n");
}


static	void	trap15_hdr(void)
{
	putstring_and_sleep("trap15 called.\n");
}


static	void	ei_hdr(void)
{
	putstring_and_sleep("EI called.\n");
}


static	void	dummy_hdr(void)
{
	return;
}


static	void	store_bra24(UW* adr, void (*func)())
{
	*adr = 0xff000000 | ((((UW)func) - ((UW)adr)) >> 2);
}


	/* 0x00-0x3f:int 0x100-0x10f:trap 0x200-0x203:RI SBI RIE AE */
void	def_int(W num, void (*func)())
{
	if (num < 0)
		;
	else if (num < 0x40) {
		INT_VENT[num] = (UW)func;
		return;
	} else if (num < 0x100)
		;
	else if (num < 0x110) {
		store_bra24(EIT_VENT + 0x10 + (num - 0x100), func);
		return;
	} else if (num < 0x200)
		;
	else if (num < 0x204) {
		store_bra24(EIT_VENT + 4 * (num - 0x100), func);
		return;
	}
	
#if DEBUGPRINT
	putstring("def_int(");
	putW(num);
	putstring(")\n");
#endif
	return;
}


	/* call after setup_sdram() */
void	setup_int(void)
{
	W	i;
	
	DI();
	
	*ICUCR1 = 0x00000107;	/* DI clear */
	*ICUCR2 = 0x00000107;	/* DI clear */
	*ICUCR3 = 0x00000107;	/* DI clear */
	*ICUCR4 = 0x00000107;	/* DI clear */
	*ICUCR5 = 0x00000107;	/* DI clear */
	*ICUCR6 = 0x00000107;	/* DI clear */
	*ICUCR7 = 0x00000107;	/* DI clear */
	*ICUCR16 = 0x00000107;	/* DI clear */
	*ICUCR17 = 0x00000107;	/* DI clear */
	*ICUCR18 = 0x00000107;	/* DI clear */
	*ICUCR19 = 0x00000107;	/* DI clear */
	*ICUCR20 = 0x00000107;	/* DI clear */
	*ICUCR21 = 0x00000107;	/* DI clear */
	*ICUCR32 = 0x00000107;	/* DI clear */
	*ICUCR33 = 0x00000107;	/* DI clear */
	*ICUCR48 = 0x00000107;	/* DI clear */
	*ICUCR49 = 0x00000107;	/* DI clear */
	*ICUCR50 = 0x00000107;	/* DI clear */
	*ICUCR51 = 0x00000107;	/* DI clear */
	*ICUCR52 = 0x00000107;	/* DI clear */
	*ICUCR53 = 0x00000107;	/* DI clear */
	*ICUCR54 = 0x00000107;	/* DI clear */
	*ICUCR55 = 0x00000107;	/* DI clear */
	*ICUCR56 = 0x00000107;	/* DI clear */
	*ICUCR57 = 0x00000107;	/* DI clear */
	
	*ICUISTS = 0x10000000;
	*ICUIMASK = 0x00070000;
	
	def_int(0, dummy_hdr);
	for (i=1; i<0x40; i++)
		def_int(i, ei_hdr);
	def_int(0x100, trap0_hdr);
	def_int(0x101, trap1_hdr);
	def_int(0x102, trap2_hdr);
	def_int(0x103, trap3_hdr);
	def_int(0x104, trap4_hdr);
	def_int(0x105, trap5_hdr);
	def_int(0x106, trap6_hdr);
	def_int(0x107, trap7_hdr);
	def_int(0x108, trap8_hdr);
	def_int(0x109, trap9_hdr);
	def_int(0x10a, trap10_hdr);
	def_int(0x10b, trap11_hdr);
	def_int(0x10c, trap12_hdr);
	def_int(0x10d, trap13_hdr);
	def_int(0x10e, trap14_hdr);
	def_int(0x10f, trap15_hdr);
	def_int(0x200, ri_hdr);
	def_int(0x201, sbi_hdr);
	def_int(0x202, rie_hdr);
	def_int(0x203, ae_hdr);
	store_bra24(EIT_VENT + 0x20, ei_hdr_asm);
	
	EI();
	return;
}


void	setup_sdram(void)
{
	DI();
	
	*SD0ER = 0;		/* disable SDRAM access */
	*SDRF1 &= 0xfffeffff;	/* stop auto-ref */
	while ((*SDRF1 & 0x10000))
		;
	*SD0ADR &= 0xf81fffff;	/* DADR=0 */
	
	asm("nop\nnop\nnop\nnop");
	
	*SDRF1 |= 0x10000;	/* start auto-ref */
	*SD0ER = 1;		/* enable SDRAM access */
	return;
}










