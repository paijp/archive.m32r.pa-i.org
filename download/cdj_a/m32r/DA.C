
#define	TARGET	"da"
#include	"cdj.h"
#include	<hw.h>

#define	MON_LENGTH	(588 * 4)
#define	MAX_VOLUME	0x2000


UH	*next_wheel_pos = NULL;

static	W	da_bank = 1;
static	union	{
	UW	w[4];
	UH	h[8];
	UB	b[16];
} da_buf[2] = {
	{{0, 0, 0, 0xffffffff}}, 
	{{0, 0, 0, 0xffffffff}}
};


static	W	filtertable[2][256][12];

#if 0
#else
static
#endif
	UH	volume[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};


	/* val=256(pi) ret=16777216/pi(val=128) */
static	W	sin(UW val)
{
	static	const	W	sin_table[129] = {
		0, 65534, 131059, 196564, 
		262039, 327474, 392861, 458188, 
		523446, 588626, 653716, 718709, 
		783593, 848359, 912997, 977498, 
		1041851, 1106048, 1170078, 1233932, 
		1297600, 1361073, 1424340, 1487394, 
		1550223, 1612819, 1675171, 1737272, 
		1799111, 1860679, 1921967, 1982965, 
		2043665, 2104057, 2164132, 2223881, 
		2283296, 2342366, 2401084, 2459440, 
		2517425, 2575032, 2632251, 2689073, 
		2745491, 2801495, 2857077, 2912229, 
		2966942, 3021208, 3075020, 3128368, 
		3181245, 3233644, 3285555, 3336971, 
		3387885, 3438289, 3488175, 3537535, 
		3586363, 3634651, 3682391, 3729577, 
		3776201, 3822256, 3867736, 3912634, 
		3956942, 4000654, 4043764, 4086265, 
		4128150, 4169414, 4210050, 4250052, 
		4289413, 4328129, 4366193, 4403600, 
		4440343, 4476418, 4511818, 4546439, 
		4580576, 4613922, 4646574, 4678526, 
		4709773, 4740311, 4770136, 4799241, 
		4827624, 4855281, 4882205, 4908395, 
		4933846, 4958553, 4982514, 5005724, 
		5028181, 5049880, 5070819, 5090994, 
		5110403, 5129041, 5146908, 5163999, 
		5180313, 5195846, 5210598, 5224564, 
		5237744, 5250134, 5261734, 5272542, 
		5282556, 5291774, 5300195, 5307819, 
		5314642, 5320666, 5325888, 5330308, 
		5333925, 5336739, 5338750, 5339956, 
		5340358
	};
	W	i;
	
	i = val & 0x7f;
	if ((val & 0x80))
		i = 0x80 - i;
	i = sin_table[i];
	return (val & 0x100)? -i : i;
}

	/* val=65536(pi) ret=65535(val=0) */
static	W	cos(UW val)
{
	W	x, y, z;
	
	x = sin(128 - (val >> 8));
	y = sin(127 - (val >> 8));
	z = val & 0xff;
	return (x * (256 - z) + y * z) / 20861;
}


static	W	sinx_x(W val)
{
	if (val == 0)
		return 0x10000;
	return sin(val) / val;
}


static	void	init_filter(void)
{
	static	W	deemp[12] = {0, 10, 30, 66, 134, 520, 134, 66, 30, 10, 0, 0};
	W	i, j, k;
	
	for (i=0; i<256; i++) {
		for (j=0; j<12; j++) {
			filtertable[0][i][j] = (sinx_x((j - 5) * 256 - i) * (2048 + (cos(((j - 5) * 256 - i) * 256 / 6) >> 5))) >> 7;
			filtertable[1][i][j] = 0;
			for (k=-6; k<6; k++) {
				if (j + k < 0)
					continue;
				if (j + k >= 12)
					continue;
				filtertable[1][i][j + k] += filtertable[0][i][j] * deemp[k + 6] / 1000;
			}
		}
	}
	
#if 0
	for (i=0; i<256; i++) {
		for (j=0; j<12; j++) {
			putnum(filtertable[0][i][j]);
			putstring(" ");
		}
		putstring("\n");
	}
	putstring("\n");
	for (i=0; i<256; i++) {
		for (j=0; j<12; j++) {
			putnum(filtertable[1][i][j]);
			putstring(" ");
		}
		putstring("\n");
	}
	putstring("\n");
#endif
	
	return;
}


static	inline	W	comp_addr(struct info_locateunit_struct *p1, struct info_locateunit_struct *p2)
{
	if (p1->addr + p1->pos < p2->addr + p2->pos)
		return -1;
	if (p1->addr + p1->pos > p2->addr + p2->pos)
		return 1;
	if (p1->offset < p2->offset)
		return -1;
	if (p1->offset > p2->offset)
		return 1;
	return 0;
}


static	void	da_hdr(void)
{
	static	UW	mon_count = 0;
	static	W	src[4];
	static	W	wheel_val[MAXWHEEL];
	W	i;
	
	if ((++mon_count) >= MON_LENGTH * 2)
		mon_count = 0;
	
	{
		static	W	val[MAXWHEEL];
		static	UH	*wheel_pos = NULL;
		static	W	last_a = 0;
		static	W	last_b = 0;
		static	W	phase = 0;
		
		wheel_val[0] = wheel_val[1] = 0;
		if ((wheel_pos)) {
#if 0
			val[0] = (((H)wheel_pos[22] - (H)wheel_pos[20]) * phase) >> 3;
			val[1] = (((H)wheel_pos[23] - (H)wheel_pos[21]) * phase) >> 3;
#else
			calc_audioevent(((UW*)wheel_pos) + (phase >> 8), (UW*)wheel_pos, filtertable[1][phase & 0xff], val);
#endif
			wheel_val[0] = ((val[0] - last_a) << 4) * 588;
			last_a = val[0];
			wheel_val[1] = ((val[1] - last_b) << 4) * 588;
			last_b = val[1];
		}
		if ((phase++) >= 256) {
			if ((wheel_pos)) {
				last_a -= ((H)wheel_pos[2]) << 5;
				last_b -= ((H)wheel_pos[3]) << 5;
			} else
				last_a = last_b = 0;
			wheel_pos = next_wheel_pos;
			next_wheel_pos = NULL;
			phase = 1;
			*SIO0TRCR = 0x0005;	/* TXE-int RX-int */
		}
	}
	
	src[0] = src[1] = src[2] = src[3] = 0;
	for (i=0; i<MAXWHEEL; i++) {
		struct	info_locateunit_struct	*l;
		struct	info_wheel_struct	*w;
		W	*p;
		
		w = info_wheel + i;
		l = w->locate[w->drive].unit;
		
		if ((l->status & STATUS_WAIT))
			continue;
		if ((w->mode & (MODE_PAUSE|MODE_FF|MODE_FR)) == MODE_PAUSE)
			;
		else
			wheel_val[i] += w->pitch_step;
		
		if ((l->offset += wheel_val[i]) < 0) {
			if ((--(l->pos)) < 0) {
				l->pos = 0;
				l->offset = 0;
				continue;
			}
			l->offset += (588 * DABUF_UNIT) << 16;
#if 0
D1STR("-");
#endif
		} else if (l->offset >= ((588 * DABUF_UNIT) << 16)) {
			if (++(l->pos) >= MAXBUFFER) {
				l->pos = MAXBUFFER - 1;
				l->offset = ((588 * DABUF_UNIT) << 16) - 1;
				continue;
			}
			l->offset -= (588 * DABUF_UNIT) << 16;
#if 0
D1STR("+");
#endif
#if 0
{
	UW	i;
	
	asm("mv %0, r15" : "=r" (i));
	D1W("sp", i);
}
#endif
		}
		
		if ((w->mode & MODE_LOOP) == 0)
			;
		else if (comp_addr(l + 2, l + 3) < 0) {
			if (w->pitch_step < 0) {
				if (comp_addr(l, l + 2) < 0)
					l[0] = l[3];
			} else if (comp_addr(l, l + 3) > 0)
				l[0] = l[2];
		} else if (w->pitch_step < 0) {
			if (comp_addr(l, l + 3) < 0)
				l[0] = l[2];
		} else if (comp_addr(l, l + 2) > 0)
			l[0] = l[3];
		
		if (info_drive[w->drive].eject_tmr <= 0)
			;
		else if (mon_count < MON_LENGTH)
			;
		else
			continue;
		
		p = src + i * 2;
		{
			W	block, type, pos, offset;
			
			if ((w->mode & (MODE_PAUSE|MODE_MON)) != (MODE_PAUSE|MODE_MON)) {
				pos = l->pos;
				offset = l->offset;
			} else if (mon_count < MON_LENGTH) {
				w->mon_pos = l->pos;
				w->mon_offset = l->offset;
				continue;
			} else if ((offset = w->mon_offset += w->pitch_step) < 0) {
				if ((pos = --(w->mon_pos)) < 0) {
					offset = 0;
					pos = 0;
				} else
					offset = w->mon_offset += (588 * DABUF_UNIT) << 16;
			} else if (offset >= ((588 * DABUF_UNIT) << 16)) {
				pos = ++(w->mon_pos);
				offset = w->mon_offset -= (588 * DABUF_UNIT) << 16;
			} else
				pos = w->mon_pos;
			
			if ((block = l->block[pos]) < 0)
				continue;
			if (l->block[pos + 1] < 0)
				continue;
			type = info_block[block].type;
			if ((type & TYPE_REQ))
				continue;
#if 0
D1W("block", block);
D1W("  type", type);
D1W("  pos", pos);
D1W("  offset", l->offset);
D1W("  pitch_step", w->pitch_step);
D1W("  ptr->data", (UW)(info_block[block].ptr->data));
D1W("  ptr->data1", (UW)(info_block[l->block[pos + 1]].ptr->data));
#else
			if ((type & TYPE_DATA) == 0) {
				*MFTCR = 0x00000800;	/* MFT4=stop */
				calc_audioevent(info_block[block].ptr->data + (offset >> 16), info_block[l->block[pos + 1]].ptr->data, filtertable[type & TYPE_EMP][(offset & 0xff00) >> 8], p);
				*MFTCR = 0x00000808;	/* MFT4=start */
			}
#endif
		}
		{
			W	j;
			UH	*q;
			
			j = p[0] + p[1];
			if (w->lm_min > j)
				w->lm_min = j;
			if (w->lm_max < j)
				w->lm_max = j;
			
			q = volume + (8 + i * 2);
			j = (MAX_VOLUME * info_wheel[i].volume * info_wheel[i].cf) >> 16;
			if (q[0] < j)
				q[5] = q[0] += (q[0] < j - 10)? 10 : 1;
			else if (q[0] > j)
				q[5] = q[0] -= (q[0] > j + 10)? 10 : 1;
		}
	}
	{
		UB	*p;
		
		p = da_buf[da_bank].b;
		*DMA0RSA = (UW)p;	/* reload address */
		mix_audioevent(src, p, volume);
		da_bank = da_bank ^ 1;
	}
	
	return;
}


static	void	dma_hdr(void)
{
	*DMAEDET = 0x8000;	/* clear */
	
	*P7DATA &= 0xbf;
	da_hdr();
	*P7DATA |= 0x40;
	
	return;
}


void	set_monitor(W mon)
{
	
	switch (mon) {
		case	0:
			volume[0] = (MAX_VOLUME * info_wheel[0].volume) >> 9;
			volume[1] = (MAX_VOLUME * info_wheel[0].volume) >> 9;
			volume[2] = 0;
			volume[3] = 0;
			volume[4] = volume[8] >> 1;
			volume[5] = volume[13] >> 1;
			volume[6] = volume[10] >> 1;
			volume[7] = volume[15] >> 1;
			break;
		case	1:
			volume[0] = 0;
			volume[1] = 0;
			volume[2] = (MAX_VOLUME * info_wheel[1].volume) >> 9;
			volume[3] = (MAX_VOLUME * info_wheel[1].volume) >> 9;
			volume[4] = volume[8] >> 1;
			volume[5] = volume[13] >> 1;
			volume[6] = volume[10] >> 1;
			volume[7] = volume[15] >> 1;
			break;
		default:
			volume[0] = volume[8];
			volume[1] = 0;
			volume[2] = volume[10];
			volume[3] = 0;
			volume[4] = 0;
			volume[5] = volume[13];
			volume[6] = 0;
			volume[7] = volume[15];
			break;
	}
	return;
}


void	setup_da(void)
{
	init_filter();
	da_buf[0].w[0] = da_buf[0].w[1] = da_buf[0].w[2] = 0;
	
	def_int(32, dma_hdr);
	*ICUCR32 = 0x00001000;	/* DMA_INT */
	
	*MFTCR = 0x00004000;	/* MFT1=stop */
	*MFT1MOD = 0x00008810;	/* PWM NEG */
	*MFT1CUT = 0x00000001;	/* count=2 */
	*MFT1CMPRLD = 0x00000001;	/* comp=1 */
	*P7MOD = 0x0002;	/* P77=MFT1B */
	*MFTCR = 0x00004040;	/* MFT1=start */
	
	*SIO1CR = 0x00000000;	/* stop */
	*SIO1CR = 0x00000300;	/* reset */
	*SIO1MOD0 = 0x00000020;	/* CSIO */
	*SIO1MOD1 = 0x00000010;	/* 16bits CMOS MSB-first int-clock */
	*SIO1TRCR = 0;		/* DI */
	*SIO1BAUR = 2;		/* 5.33MHz */
	
	*P5DATA = 0x33;	/* P56=H */
	*P5DIR = 0x8a;	/* P56=out */
	*P5MOD = 0x5054;	/* P56=sclk1 */
	
	*DMAEN = 0x00008000;	/* stop DMA */
	*DMA0CR0 = 0x11064007;	/* stop 16bits burst EI 1data reload-count-src src++ int->int SIO1-TX */
	*DMA0CR1 = 0x00000000;	/* no clear */
	*DMA0CSA = *DMA0RSA = (UW)(da_buf[0].h);
	*DMA0CDA = *DMA0RDA = ((UW)SIO1TXB) + 2;
	*DMA0CBCUT = *DMA0RBCUT = 16;
	asm("nop||nop\nnop||nop\nnop||nop\nnop||nop");	/* 60ns */
	*DMAEN = 0x00008080;	/* start DMA */
	
	asm("nop||nop\nnop||nop\nnop||nop\nnop||nop");	/* 60ns */
	asm("nop\nnop\nnop\nnop");
	*SIO1TRCR = 0x11;	/* TX-DMA */
	asm("nop||nop\nnop||nop\nnop||nop\nnop||nop");	/* 60ns */
	*SIO1CR = 0x00000001;	/* SIO1=start */
	
	return;
}


