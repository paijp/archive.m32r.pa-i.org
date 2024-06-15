
#define	TARGET	"main"
#include	"itronts.h"
#include	<hw.h>


static	void	rsrecv_hdr(void)
{
	W	i;
	
	i = *SIO0RXB;
	/*i*/wup_tsk(1);
	
	return;
}


static	void	setup_rs(void)
{
	*SIO0CR = 0x0300;			/* clr-status */
	
	def_int(48, rsrecv_hdr);
	*ICUCR48 = 0x00001001;	/* SIO0RX_INT level=1 */
	
	*SIO0MOD0 = 0x0000;			/* N*1 */
	*SIO0MOD1 = 0x0800;			/* 8bit CMOS LSB-first */
	*SIO0BAUR = 17;				/* 115200bps */
	*SIO0RBAUR = 2;
	*SIO0TRCR = 0x0004;			/* ~TXE-int RX-int */
	*SIO0CR = 0x0003;			/* TX RX */
	
	return;
}


void	task_id1(W stacd)
{
	(void)stacd;
	setup_rs();
	for (;;) {
		switch (tslp_tsk(50)) {
			case	E_OK:
				*SIO0TXB = 'A';
				break;
			case	E_TMOUT:
				*SIO0TXB = 'C';
				break;
			default:
				*SIO0TXB = 'D';
				break;
		}
	}
}


void	task_id2(W stacd)
{
	(void)stacd;
	for (;;) {
		tslp_tsk(100);
		*SIO0TXB = 'B';
/*		rot_rdq(TPRI_RUN);	*/
	}
}


static	void	setup_timer(void)
{
	def_int(21, (void(*)(void))isig_tim);
	*ICUCR21 = 0x00001006;	/* MFT5_INT level=6 */
	
	*MFTCR = 0x00000400;	/* MFT5=stop */
	*MFT5MOD = 0x00008003;	/* OSC NEG 1/128 */
	*MFT5CUT = 0x00000a00;	/* count=2560(*128) : 10ms */
	*MFTCR = 0x00000404;	/* MFT5=start */
}


int	main(void)
{
	D2STR("itronts boot.");
	
	*P6DATA = 0xff;		/* disable LAN */
	
	setup_sdram();
	setup_int();
	
	setup_itron();
	setup_timer();
	dis_dsp();
	{
		static	T_CTSK	ct1 = {TA_ACT, 0, task_id1, 1};
		static	T_CTSK	ct2 = {TA_ACT, 0, task_id2, 1};
		cre_tsk(1, &ct1);
		cre_tsk(2, &ct2);
	}
	ena_dsp();
	idletask();
}


