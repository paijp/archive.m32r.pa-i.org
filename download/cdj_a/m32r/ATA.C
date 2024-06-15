
#define	TARGET	"ata"
#include	"cdj.h"
#include	<hw.h>

struct	info_drive_struct	info_drive[MAXDRIVE];


static	_UW*	atabase = NULL;
static	_UW*	atabase_d = NULL;

#define	ATAREG_ASTS	0xe
#define	ATAREG_DEVC	0xe
#define	ATAREG_DATA	0
#define	ATAREG_ERROR	1
#define	ATAREG_FEAT	1
#define	ATAREG_INTR	2
#define	ATAREG_SCNT	2
#define	ATAREG_SNUM	3
#define	ATAREG_BCL	4
#define	ATAREG_BCH	5
#define	ATAREG_DEV	6
#define	ATAREG_CMD	7
#define	ATAREG_STS	7

#define	ATASTS_BSY	0x80
#define	ATASTS_DRDY	0x40
#define	ATASTS_DRQ	8
#define	ATASTS_CHK	1

#define	ATAINTR_REL	4
#define	ATAINTR_IO	2
#define	ATAINTR_CD	1


	/* PIO mode 2 */
static	inline	void	setreg_ata2(W reg, W val)
{
	((_UH*)atabase)[reg * 2 + 1] = val;
	return;
}

	/* PIO mode 2 */
static	inline	W	getreg_ata2(W reg)
{
	return ((_UH*)atabase)[reg * 2 + 1];
}

	/* PIO mode 0 */
static	inline	void	setreg_ata0(W reg, W val)
{
	atabase[reg] = val;
	return;
}

	/* PIO mode 0 */
static	inline	W	getreg_ata0(W reg)
{
	return atabase[reg] & 0xffff;
}

	/* PIO mode 2 data */
static	inline	void	setdata_ata2(W val)
{
	((_UH*)atabase_d)[1] = val;
	return;
}

	/* PIO mode 2 data */
static	inline	W	getdata_ata2(void)
{
	return ((_UH*)atabase_d)[1];
}


#if 0
static	void	setdatar_ata2(W size, const UH* buf)
{
	while (size-- > 0)
		setdata_ata2(*(buf++));
	return;
}


static	void	getdatar_ata2(W size, UH* buf)
{
	while (size-- > 0)
		*(buf++) = getdata_ata2();
	return;
}
#endif


static	void	select_drive(W drive)
{
	if ((drive < 0)||(drive >= MAXDRIVE)) {
		D2W("select_drive", drive);
		return;
	}
	atabase = info_drive[drive].ata_r;
	atabase_d = info_drive[drive].ata_d;
	return;
}


static	W	sndcmd_ata(W drive, const UH *cmd)
{
	W	i;
	
	select_drive(drive);
	
	(void)getreg_ata0(ATAREG_STS);
#if 1
	(void)getreg_ata0(ATAREG_ASTS);
	(void)getreg_ata0(ATAREG_ASTS);
	(void)getreg_ata0(ATAREG_ASTS);
#endif
	while ((getreg_ata2(ATAREG_ASTS) & ATASTS_BSY)) {
		rot_rdq(TPRI_RUN);
		select_drive(drive);
	}
	(void)getreg_ata2(ATAREG_STS);
	
		/* packet */
	setreg_ata2(ATAREG_FEAT, 0x00);
	setreg_ata2(ATAREG_BCL, 0x00);
	setreg_ata2(ATAREG_BCH, 0x10);	/* 4KB */
	setreg_ata2(ATAREG_CMD, 0xa0);
	
	(void)getreg_ata0(ATAREG_ASTS);	/* >400nS */
#if 1
	(void)getreg_ata0(ATAREG_ASTS);
	(void)getreg_ata0(ATAREG_ASTS);
	(void)getreg_ata0(ATAREG_ASTS);
#endif
	while (((i = getreg_ata2(ATAREG_ASTS)) & ATASTS_BSY)) {
		rot_rdq(TPRI_RUN);
		select_drive(drive);
	}
	(void)getreg_ata2(ATAREG_STS);
	
	if ((i & ATASTS_CHK)) {
		D4B("1.ATAREG_STS CHK", i);
		return -1;
	}
	if ((i & ATASTS_DRQ) == 0) {
		D4B("ATAREG_STS ~DRQ", i);
		return -2;
	}
	
	i = getreg_ata2(ATAREG_INTR);
	if ((i & ATAINTR_CD) == 0) {
		D4B("ATAREG_INTR ~CD", i);
		return -3;
	}
	if ((i & ATAINTR_IO)) {
		D4B("ATAREG_INTR IO", i);
		return -4;
	}
	
	if ((cmd))
		for (i=0; i<6; i++)
			setreg_ata2(ATAREG_DATA, cmd[i]);
	return 0;
}


	/* send=0:no-data >0:host to dev <0:dev to host */
	/* ret:0xffQQAASS SS=sence-key QQ=ASCQ AA=ASC */
static	W	chkerr_ata(W drive, W send)
{
	const	UH	reqsense[6] = {0x0003, 0, 0x0012, 0, 0, 0};
	W	i, err;
	
	select_drive(drive);
	
	(void)getreg_ata0(ATAREG_ASTS);
#if 1
	(void)getreg_ata0(ATAREG_ASTS);
	(void)getreg_ata0(ATAREG_ASTS);
	(void)getreg_ata0(ATAREG_ASTS);
#endif
	while (((i = getreg_ata2(ATAREG_ASTS)) & ATASTS_BSY)) {
		rot_rdq(TPRI_RUN);
		select_drive(drive);
	}
	(void)getreg_ata2(ATAREG_STS);
	
	if ((i & ATASTS_CHK) == 0) {
		if (send == 0) {
			if ((i & ATASTS_DRQ) == 0)
				return 0;
			D4B("ATAREG_STS DRQ", i);
			return -5;
		}
		if ((i & ATASTS_DRQ) == 0) {
			D4B("ATAREG_STS ~DRQ", i);
			return -6;
		}
		i = getreg_ata2(ATAREG_INTR);
		if ((i & ATAINTR_CD)) {
			D4B("ATAREG_INTR CD", i);
			return -7;
		}
		if (send > 0) {
			if ((i & ATAINTR_IO)) {
				D4B("ATAREG_INTR IO", i);
				return -8;
			}
		} else if ((i & ATAINTR_IO) == 0) {
			D4B("ATAREG_INTR ~IO", i);
			return -9;
		}
		return 0;
	}
	
	D4B("2.ATAREG_STS CHK", i);
	
	if ((err = sndcmd_ata(drive, reqsense)) < 0)
		return err;
	
	while (((i = getreg_ata2(ATAREG_ASTS)) & ATASTS_BSY)) {
		rot_rdq(TPRI_RUN);
		select_drive(drive);
	}
	(void)getreg_ata2(ATAREG_STS);
	
	if ((i & ATASTS_CHK)) {
		D4B("3.ATAREG_STS CHK", i);
		return -10;
	}
	if ((i & ATASTS_DRQ) == 0) {
		D4B("ATAREG_STS ~DRQ", i);
		return -11;
	}
	i = getreg_ata2(ATAREG_INTR);
	if ((i & ATAINTR_CD)) {
		D4B("ATAREG_INTR CD", i);
		return -12;
	}
	if ((i & ATAINTR_IO) == 0) {
		D4B("ATAREG_INTR ~IO", i);
		return -13;
	}
	
	err = 0xff000000;
	(void)getdata_ata2();
	err |= getdata_ata2() & 0xf;
	(void)getdata_ata2();
	(void)getdata_ata2();
	(void)getdata_ata2();
	(void)getdata_ata2();
	err |= (getdata_ata2() & 0xffff) << 8;
	(void)getdata_ata2();
	(void)getdata_ata2();
	
	(void)getreg_ata2(ATAREG_STS);
	D4W("0xffQQAASS", err);
	return err;
}


W	setup_ata(void)
{
	W	i;
	
	*P0MOD = 0x155;	/* bsel1-5 enable */
	*BSEL4CR = 0x09993101;	/* Tr=Tw=10 16bit Tbsel=1 Tstb=1 Trec=1 Tn=1 */
	*BSEL5CR = 0x03333101;	/* Tr=Tw=4 16bit Tbsel=1 Tstb=1 Trec=1 Tn=1 */
	
	*MFTCR = 0x00000800;	/* MFT4=stop */
	*MFT4MOD = 0x00008000;	/* OSC NEG */
	*MFT4CUT = 0x00000019;	/* count=25 */
	*MFTCR = 0x00000808;	/* MFT4=start */
	
	*DMAEN = 0x00004000;	/* stop DMA */
	*DMA1CR0 = 0x20041314;	/* stop 32bits burst DI 1data reload-count dst++ ext->ext mft4 */
	*DMA1CR1 = 0x01000000;	/* auto-clear */
	*DMA1CSA = *DMA1RSA = 0;
	*DMA1CDA = *DMA1RDA = 0;
	*DMA1CBCUT = *DMA1RBCUT = 2352;
	asm("nop||nop\nnop||nop\nnop||nop\nnop||nop");	/* 60ns */
	
	for (i=0; i<MAXDRIVE; i++) {
		info_drive[i].maxtrack = -1;
		info_drive[i].maxaddr = 0;
		info_drive[i].ata_r = ((i))? (void*)0x04000000 : (void*)0x04000040;
		info_drive[i].ata_d = ((i))? (void*)0x05000000 : (void*)0x05000040;
		info_drive[i].req_block = -1;
		info_drive[i].req_eject = 0;
		info_drive[i].ataerr = 0;
	}
	return 0;
}


static	W	setup_ataunit(W drive)
{
	W	i;
#if 0
	W	err;
#endif
	
	D4STR("setup_ataunit");
	select_drive(drive);
	
	setreg_ata0(ATAREG_DEVC, 6);	/* SRST */
	
	rot_rdq(TPRI_RUN);
	select_drive(drive);
	
	setreg_ata0(ATAREG_DEVC, 2);
	
	rot_rdq(TPRI_RUN);
	select_drive(drive);
	
	while ((getreg_ata0(ATAREG_ASTS) & ATASTS_BSY)) {
		rot_rdq(TPRI_RUN);
		select_drive(drive);
	}
	(void)getreg_ata0(ATAREG_STS);
	
#if 0
	if ((i = getreg_ata0(ATAREG_BCL) & 0xff) != 0x14) {
		D4B("ATAREG_BCL != 0x14", i);
		return -64;
	}
	if ((i = getreg_ata0(ATAREG_BCH) & 0xff) != 0xeb) {
		D4B("ATAREG_BCH != 0xeb", i);
		return -65;
	}
#endif
	
	setreg_ata0(ATAREG_DEVC, 0x0a);
#if 0
	setreg_ata0(ATAREG_DEV, 0x10);
#else
	setreg_ata0(ATAREG_DEV, 0x00);
#endif
	(void)getreg_ata0(ATAREG_ASTS);	/* >400nS */
#if 1
	(void)getreg_ata0(ATAREG_ASTS);
	(void)getreg_ata0(ATAREG_ASTS);
	(void)getreg_ata0(ATAREG_ASTS);
#endif
	
		/* set features */
	setreg_ata0(ATAREG_FEAT, 0x03);
	setreg_ata0(ATAREG_SCNT, 0x0a);	/* PIO mode 2 */
	setreg_ata0(ATAREG_CMD, 0xef);
	
	(void)getreg_ata0(ATAREG_ASTS);
#if 1
	(void)getreg_ata0(ATAREG_ASTS);
	(void)getreg_ata0(ATAREG_ASTS);
	(void)getreg_ata0(ATAREG_ASTS);
#endif
	while (((i = getreg_ata0(ATAREG_ASTS)) & ATASTS_BSY)) {
		rot_rdq(TPRI_RUN);
		select_drive(drive);
	}
	(void)getreg_ata0(ATAREG_STS);
	
	if ((i & ATASTS_CHK)) {
		D4B("4.ATAREG_STS CHK", i);
		return -66;
	}
	
		/* idle immediate */
	setreg_ata0(ATAREG_CMD, 0xe1);
	
	(void)getreg_ata0(ATAREG_ASTS);
#if 1
	(void)getreg_ata0(ATAREG_ASTS);
	(void)getreg_ata0(ATAREG_ASTS);
	(void)getreg_ata0(ATAREG_ASTS);
#endif
	while (((i = getreg_ata0(ATAREG_ASTS)) & ATASTS_BSY)) {
		rot_rdq(TPRI_RUN);
		select_drive(drive);
	}
	(void)getreg_ata0(ATAREG_STS);
	
	if ((i & ATASTS_CHK)) {
		D4B("5.ATAREG_STS CHK", i);
		return -67;
	}
	
#if 0
	for (i=0; i<5; i++) {
		const	UH	testunit[6] = {0x0000, 0, 0, 0, 0, 0};
		
		if ((err = sndcmd_ata(drive, testunit)) < 0)
			return -68;
		if ((err = chkerr_ata(drive, 0)) >= 0)
			break;
	}
#endif
	
	D4STR("....done");
	return 0;
}


static	inline	UW	swapH(UW i)
{
	return ((i << 8) & 0xff00) | ((i >> 8) & 0xff);
}


static	inline	UW	swapW(UW i)
{
	return ((i << 24) & 0xff000000) | ((i << 8) & 0xff0000) | ((i >> 8) & 0xff00) | ((i >> 24) & 0xff);
}


static	W	setup_infodrive(W drive)
{
	const	UH	spinup[6] = {0x001b, 0, 0x0001, 0, 0, 0};
	const	UH	readtoc[6] = {0x0043, 0x0000, 0, 0x0400, 0x0000, 0};
	struct	info_drive_struct	*d;
	UH	data;
	W	i, err, addr, maxtrack;
	
	D4NUM("setup_infodrive", drive);
	if ((drive < 0)||(drive >= MAXDRIVE))
		return -1;
	d = info_drive + drive;
	
	if ((err = sndcmd_ata(drive, spinup)) < 0)
		return d->ataerr = err & 0xfeffffff;
	if ((err = chkerr_ata(drive, 0)) < 0)
		return d->ataerr = err & 0xfeffffff;
	
	if ((err = sndcmd_ata(drive, readtoc)) < 0)
		return d->ataerr = err & 0xfdffffff;
	if ((err = chkerr_ata(drive, -1)) < 0)
		return d->ataerr = err & 0xfdffffff;
	
	select_drive(drive);
	
	d->maxtrack = -1;
	(void)getdata_ata2();		/* len */
	data = getdata_ata2();
	maxtrack = ((data & 0xff00) >> 8) - (data & 0xff) + 1;
	if (maxtrack < 1)
		return -1;
	if (maxtrack > MAXTRACK)
		maxtrack = MAXTRACK;
	
	D4NUM("drive", drive);
	addr = 0;
	for (i=0; i<=maxtrack; i++) {
		D4NUM("track", i);
		data = getdata_ata2();
		switch ((data & 0x0d00) >> 8) {
			case	0:
				d->track[i].type = 0;
				break;
			case	1:
				d->track[i].type = TYPE_EMP;
				break;
			default:
				d->track[i].type = TYPE_DATA;
				break;
		}
#if 0
d->track[i].type |= (i & 3);
#endif
		(void)getdata_ata2();
		addr = (UW)swapH(getdata_ata2()) << 16;
		addr |= (UW)swapH(getdata_ata2());
		if (addr < 0)
			addr = 0;
		d->track[i].addr = addr / DABUF_UNIT;
		d->track[i].offset = ((addr % DABUF_UNIT) * 588) << 16;
		D4W("  type", d->track[i].type);
		D4W("  addr", d->track[i].addr);
		D4W("  offset", d->track[i].offset);
	}
	(void)getreg_ata2(ATAREG_STS);
	
	if ((d->maxaddr = addr / DABUF_UNIT) < 8)
		return -1;
	DI();
	d->maxtrack = maxtrack;
	
	for (i=0; i<MAXWHEEL; i++) {
		struct	info_locateunit_struct	*l;
		struct	info_wheel_struct	*w;
		W	track;
		
		w = info_wheel + i;
		l = w->locate[drive].unit;
		if ((track = w->locate[drive].track) > 0) {
			while (track >= d->maxtrack)
				track -= d->maxtrack;
			init_locate(l, (drive << 28) | d->track[track].addr, d->track[track].offset);
		} else if (track < 0) {
			while ((track = w->locate[drive].track += d->maxtrack) < 0)
				;
			init_locate(l, (drive << 28) | d->track[track].addr, d->track[track].offset);
		} else
			init_locate(l + 0, drive << 28, 0);
		init_locate(l + 1, drive << 28, 0);
		init_locate(l + 2, drive << 28, 0);
		init_locate(l + 3, (drive << 28)|(d->maxaddr - 1), ((588 * DABUF_UNIT) << 16) - 1);
	}
	EI();
	D1W("maxaddr", d->maxaddr);
	D1NUM("maxtrack", d->maxtrack);
	
	D4STR("....done");
	d->ataerr = 0;
	return 0;
}


static	W	read_cddma(W drive, UW lba, UW size, UB* buf)
{
	W	err;
	
	D4STR("read_cddma");
	if ((err = chkerr_ata(drive, 0)) < 0)
		return err;
	
	if ((err = sndcmd_ata(drive, NULL)) < 0)
		return err;
	setreg_ata2(ATAREG_DATA, 0x04be);
	setreg_ata2(ATAREG_DATA, swapH(lba >> 16));
	setreg_ata2(ATAREG_DATA, swapH(lba));
	setreg_ata2(ATAREG_DATA, swapH(size >> 8));
	setreg_ata2(ATAREG_DATA, (size & 0xff) | 0x1000);
	setreg_ata2(ATAREG_DATA, 0);
	
	while (size-- > 0) {
		if ((err = chkerr_ata(drive, -1)) < 0)
			return err;
		
		if (wai_sem(1) < 0) {
			for (;;)
				;
		}
		select_drive(drive);
		
		*DMA1CSA = *DMA1RSA = (UW)(atabase_d + ATAREG_DATA);
		*DMA1CDA = *DMA1RDA = (UW)(buf);
		
		*DMAEN = 0x00004040;	/* start DMA */
		while ((*DMAEN & 0x00000040))
			rot_rdq(TPRI_RUN);
		buf += 2352;
		
		sig_sem(1);
	}
	D4STR("....done");
	return 0;
}


void	ata_task(W drive)
{
	struct	info_drive_struct	*d;
	W	err;
	
#if 0
{
	UW	i;
	
	D1NUM("drive", drive);
	asm("mv %0, r15" : "=r" (i));
	D1W("sp", i);
}
#endif
	
	d = info_drive + drive;
	setup_ataunit(drive);
	setup_infodrive(drive);
	
	for (;;) {
		struct	info_block_struct	*b;
		W	j, req, type, addr;
		
#if DEBUGPRINT
		stktest();
#endif
		can_wup(TSK_SELF);
		if (d->req_eject > 0) {
			const	UH	eject[6] = {0x001b, 0, 0x0002, 0, 0, 0};
			const	UH	load[6] = {0x001b, 0, 0x0003, 0, 0, 0};
			
			switch (d->req_eject) {
				case	1:
					d->maxtrack = -1;
					sndcmd_ata(drive, eject);
					if (0) {
				case	4:
						setreg_ata0(ATAREG_CMD, 8);
						if ((err = setup_ataunit(drive)) < 0) {
							d->ataerr = err;
							continue;
						}
						setup_infodrive(drive);
					}
				default:
					DI();
					d->maxtrack = -1;
					init_drive(drive);
					EI();
					break;
				case	3:
					sndcmd_ata(drive, load);
					break;
			}
			d->req_eject = 0;
			continue;
		}
		
		if (d->maxtrack <= 0) {
#if 0
			const	UH	start[6] = {0, 0, 0, 0, 0, 0}; /* test unit ready */
#else
			const	UH	start[6] = {0x001b, 0, 0x0001, 0, 0, 0};
#endif
			if (tslp_tsk(200) >= E_OK)
				continue;	/* 2s */
			if ((err = sndcmd_ata(drive, start)) >= 0)
				setup_infodrive(drive);
			else {
				d->ataerr = err & 0xf0ffffff;
			}
			continue;
		}
		
		DI();
		req = d->req_block;
		EI();
		if (req < 0) {
#if 1
			const	UH	idle[6] = {0x001b, 0, 0x0021, 0, 0, 0};
#else
			const	UH	idle[6] = {0, 0, 0, 0, 0, 0}; /* test unit ready */
#endif
			
			if (tslp_tsk(3000) >= E_OK)
				continue;	/* 30s */
			switch ((err = sndcmd_ata(drive, idle)) & 0xff00ffff) {
				case	0xff003a02:
				case	0xff002806:
					d->req_eject = 2;
			}
			continue;
		}
		
		b = info_block + req;
		addr = b->addr & 0x0fffffff;
		err = read_cddma(drive, addr * DABUF_UNIT, DABUF_UNIT, (void*)(b->ptr->data));
		if (err < 0) {
			W	i;
			UW	*p;
			
			p = (void*)(b->ptr->data);
			for (i=0; i<DABUF_UNIT * 588; i++)
				*(p++) = 0;
			switch (err & 0xff00ffff) {
				case	0xff003a02:
				case	0xff002806:
					d->req_eject = 2;
			}
		}
#if 0
D1STR("...done");
#endif
		
		type = 0;
		for (j=0; j<d->maxtrack; j++) {
			if (d->track[j + 1].addr < addr)
				continue;
			type |= d->track[j].type;
			if (d->track[j + 1].addr > addr)
				break;
		}
		if (j >= d->maxtrack)
			j = d->maxtrack - 1;
		
		DI();
		d->req_block = -1;
		b->type = type & (TYPE_EMP | TYPE_DATA);
		b->track = j;
		EI();
	}
	
	return;
}




