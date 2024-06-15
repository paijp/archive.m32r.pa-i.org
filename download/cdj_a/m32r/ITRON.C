
#define	TARGET	"itron"
#include	"cdj.h"
#include	<hw.h>

#define	MAXTSKID	3
#define	MAXTSKPRI	4
#define	MAXSEMID	2
#define	MAXFLGID	0
#define	MAXDTQID	0
#define	MAXDTQSIZE	4
#define	MAXMBXID	0

struct	struct_queue {
	struct	struct_queue	*f, *b;
};


static	struct	struct_readyqueue {
	struct	struct_queue	queue;
	UH	magic;		/* 0xff */
} readyqueue[MAXTSKPRI + 1];


static	struct	struct_tcb {
	struct	struct_queue	queue;
	UH	sus_ttw;	/* common to ready-queue */
	UB	tskid;		/* for reverse-lookup */
	B	tskpri, itskpri;
	UB	wupcnt;
	W	tmout;
	UW*	sp;
} tcb[MAXTSKID + 1] = {
	{{NULL, NULL}, 0, 0, 0, 0, 0, 0, (void*)0x00f0f4b8}, 
	{{NULL, NULL}, 0, 1, 2, 2, 0, 0, (void*)0x00f0efb8}, 
	{{NULL, NULL}, 0, 2, 2, 2, 0, 0, (void*)0x00f0e7b8}, 
	{{NULL, NULL}, 0, 3, 1, 1, 0, 0, (void*)0x00f0dfb8}
};

#define	TTW_ORW		0x80		/* wai_flg(TWF_ORW) */

static	W	currenttskid = 0;	/* idle task */
static	W	sysstat = TSS_DDSP;	/* dispatchable */
static	UW	psw = 0;


static	inline	void	queue_delete(struct struct_queue *p)
{
	p->b->f = p->f;
	p->f->b = p->b;
	return;
}


static	inline	void	queue_insert(struct struct_queue *p, struct struct_queue *q)
{
	struct	struct_queue	*b;
	
	b = p->b;
	p->b = q;
	q->f = p;
	b->f = q;
	q->b = b;
	return;
}


void	reschedule(void)
{
	struct	struct_tcb	*p;
	UW	*spu;
	
	if ((sysstat))
		return;
	DI();
	p = (void*)(readyqueue[0].queue.f);
	while ((p->sus_ttw))
		p = (void*)(p->queue.f);
#if 0
putstring("dispatch to tskid(");
putW(p->tskid);
putstring(")\r\n");
#endif
	
	if (currenttskid == p->tskid)
		return;
	
	asm("mvfc %0, spu" : "=r" (spu));
	tcb[currenttskid].sp = spu;
	currenttskid = p->tskid;
	spu = p->sp;
	asm("mvtc %0, spu" : : "r" (spu));
	return;
}


static	inline	struct	struct_readyqueue	*find_readyqueue(W tskpri)
{
	if (tskpri < 0)
		return NULL;
	if (tskpri > MAXTSKPRI)
		return NULL;
	return readyqueue + tskpri;
}


static	inline	struct	struct_tcb	*find_tcb(W id)
{
	if (id == TSK_SELF)
		return tcb + currenttskid;
	if (id <= 0)
		return NULL;
	if (id > MAXTSKID)
		return NULL;
	return tcb + id;
}


static	inline	void	getpsw_di(void)
{
	GETPSW_DI(psw);
}


static	inline	W	ei_ret(W ret)
{
	SETPSW(psw);
	return ret;
}


static	inline	W	check_ctx(W tmout)
{
	if ((tmout < 0)&&(tmout != TMO_FEVR))
		return E_PAR;
	if ((tmout != TMO_POL)&&(sysstat))
		return E_CTX;
	return E_OK;
}


static	W	readyqueue_insert(struct struct_tcb *p)
{
	if (p->tskpri <= 0)
		return E_NOEXS;
	p->tmout = 0;
	if ((p->sus_ttw &= 0xff00))
		return E_OBJ;
	queue_insert((void*)find_readyqueue(p->tskpri), (void*)p);
	return E_OK;
}


	/* API */


W	ichg_pri(W tskid, W tskpri)
{
	struct	struct_tcb	*p;
	
	getpsw_di();
	if ((p = find_tcb(tskid)) == NULL)
		return ei_ret(E_ID);
	if (p->tskpri <= 0)
		return ei_ret(E_NOEXS);
	if (tskpri == TPRI_INI)
		p->tskpri = p->itskpri;
	else if ((tskpri <= 0)||(tskpri > MAXTSKPRI))
		return ei_ret(E_PAR);
	else
		p->tskpri = tskpri;
	if (p->sus_ttw == 0) {
		queue_delete((void*)p);
		readyqueue_insert(p);
	}
	return ei_ret(E_OK);
}


static	W	itslp_tsk(W tmout)
{
	W	err;
	struct	struct_tcb	*p;
	
	getpsw_di();
	if ((err = check_ctx(1)) < 0)
		return ei_ret(err);
	p = find_tcb(TSK_SELF);
	if ((p->wupcnt--) > 0)
		return ei_ret(E_OK);
	p->wupcnt = 0;
	if (tmout == TMO_POL)
		return ei_ret(E_TMOUT);
	p->sus_ttw = TTW_SLP;	/* sus == 0 */
	p->tmout = tmout + 1;
	queue_delete((void*)p);
	return E_OK;		/* no ei_ret() */
}


W	iwup_tsk(W tskid)
{
	struct	struct_tcb	*p;
	
	getpsw_di();
	if ((p = find_tcb(tskid)) == NULL)
		return ei_ret(E_ID);
	if (p->tskpri <= 0)
		return ei_ret(E_NOEXS);
	if ((p->sus_ttw & TTW_SLP)) {
		readyqueue_insert(p);
		return ei_ret(E_OK);
	}
	if ((++p->wupcnt) == 0) {
		p->wupcnt--;
		return ei_ret(E_QOVR);
	}
	return ei_ret(E_OK);
}


W	ican_wup(W tskid)
{
	struct	struct_tcb	*p;
	W	err;
	
	getpsw_di();
	if ((p = find_tcb(tskid)) == NULL)
		return ei_ret(E_ID);
	if (p->tskpri <= 0)
		return ei_ret(E_NOEXS);
	err = p->wupcnt;
	p->wupcnt = 0;
	return ei_ret(err);
}


W	irel_wai(W tskid)
{
	struct	struct_tcb	*p;
	
	getpsw_di();
	if ((p = find_tcb(tskid)) == NULL)
		return ei_ret(E_ID);
	if (p->tskpri <= 0)
		return ei_ret(E_NOEXS);
	if ((p->sus_ttw & 0xff) == 0)
		return ei_ret(E_OBJ);
	if ((p->sus_ttw & (TTW_SEM|TTW_FLG|TTW_SDTQ|TTW_RDTQ|TTW_MBX)))
		queue_delete((void*)p);
	readyqueue_insert(p);
	p->sp[0x12] = E_RLWAI;
	return ei_ret(E_OK);
}


W	isus_tsk(W tskid)
{
	struct	struct_tcb	*p;
	
	getpsw_di();
	if ((p = find_tcb(tskid)) == NULL)
		return ei_ret(E_ID);
	if (p->tskpri <= 0)
		return ei_ret(E_NOEXS);
	if (p->sus_ttw == 0)
		queue_delete((void*)p);
	if (((p->sus_ttw += 0x100) & 0xff00) == 0) {
		p->sus_ttw -= 0x100;
		return ei_ret(E_QOVR);
	}
	return ei_ret(E_OK);
}


W	irsm_tsk(W tskid)
{
	struct	struct_tcb	*p;
	
	getpsw_di();
	if ((p = find_tcb(tskid)) == NULL)
		return ei_ret(E_ID);
	if (p->tskpri <= 0)
		return ei_ret(E_NOEXS);
	if (((p->sus_ttw -= 0x100) & 0xff00) == 0) {
		p->sus_ttw += 0x100;
		return ei_ret(E_OBJ);
	}
	if (p->sus_ttw == 0)
		readyqueue_insert(p);
	return ei_ret(E_OK);
}


W	ifrsm_tsk(W tskid)
{
	struct	struct_tcb	*p;
	
	getpsw_di();
	if ((p = find_tcb(tskid)) == NULL)
		return ei_ret(E_ID);
	if (p->tskpri <= 0)
		return ei_ret(E_NOEXS);
	if ((p->sus_ttw & 0xff00) == 0)
		return ei_ret(E_OBJ);
	if ((p->sus_ttw &= 0xff) == 0)
		readyqueue_insert(p);
	return ei_ret(E_OK);
}


static	W	idly_tsk(W dlytim)
{
	W	err;
	struct	struct_tcb	*p;
	
	getpsw_di();
	if ((err = check_ctx(1)) < 0)
		return ei_ret(err);
	p = find_tcb(TSK_SELF);
	if (dlytim < 0)
		return ei_ret(E_PAR);
	p->sus_ttw = TTW_DLY;	/* sus == 0 */
	p->tmout = dlytim + 1;
	queue_delete((void*)p);
	return E_OK;		/* no ei_ret() */
}


#if MAXSEMID > 0

static	struct	struct_sem {
	H	maxcnt, cnt;
	struct	struct_queue	wtask;
} sem[MAXSEMID] = {
	{1, 1, {NULL, NULL}}, 
	{1, 1, {NULL, NULL}}
};


static	inline	struct	struct_sem	*find_sem(W id)
{
	if (id <= 0)
		return NULL;
	if (id > MAXSEMID)
		return NULL;
	return sem + (id - 1);
}


W	isig_sem(W semid)
{
	struct	struct_tcb	*p;
	struct	struct_sem	*q;
	
	getpsw_di();
	if ((q = find_sem(semid)) == NULL)
		return ei_ret(E_ID);
	if (q->maxcnt < 0)
		return ei_ret(E_NOEXS);
	if ((p = (void*)q->wtask.f) != (void*)(&q->wtask)) {
		queue_delete((void*)p);
		readyqueue_insert(p);
		return ei_ret(E_OK);
	}
	if ((++q->cnt) > q->maxcnt) {
		q->cnt = q->maxcnt;
		return ei_ret(E_QOVR);
	}
	return ei_ret(E_OK);
}


W	itwai_sem(W semid, W tmout)
{
	W	err;
	struct	struct_tcb	*p;
	struct	struct_sem	*q;
	
	getpsw_di();
	if ((err = check_ctx(tmout)) < 0)
		return ei_ret(err);
	if ((q = find_sem(semid)) == NULL)
		return ei_ret(E_ID);
	if (q->maxcnt < 0)
		return ei_ret(E_NOEXS);
	if (q->cnt > 0) {
		q->cnt--;
		return ei_ret(E_OK);
	}
	if (tmout == TMO_POL)
		return ei_ret(E_TMOUT);
	p = find_tcb(TSK_SELF);
	p->sus_ttw = TTW_SEM;	/* sus == 0 */
	p->tmout = tmout + 1;
	queue_delete((void*)p);
	queue_insert((void*)(&q->wtask), (void*)p);
	return E_OK;		/* no ei_ret() */
}

#endif

#if MAXDTQID > 0

static	struct	struct_dtq {
	B	size, remain;
	UB	wpos, rpos;
	UW	data[MAXDTQSIZE];
	struct	struct_queue	rtask;
	struct	struct_queue	stask;
} dtq[MAXDTQID] = {
	{0, 0, 0, 0, {0}, {NULL, NULL}, {NULL, NULL}}, 
	{1, 1, 0, 0, {0}, {NULL, NULL}, {NULL, NULL}}
};


static	inline	struct	struct_dtq	*find_dtq(W id)
{
	if (id <= 0)
		return NULL;
	if (id > MAXDTQID)
		return NULL;
	return dtq + (id - 1);
}


W	itsnd_dtq(W dtqid, UW data, W tmout)
{
	W	err;
	struct	struct_tcb	*p;
	struct	struct_dtq	*q;
	
	getpsw_di();
	if ((err = check_ctx(tmout)) < 0)
		return ei_ret(err);
	if ((q = find_dtq(dtqid)) == NULL)
		return ei_ret(E_ID);
	if (q->size < 0)
		return ei_ret(E_NOEXS);
	if ((p = (void*)q->rtask.f) != (void*)(&q->rtask)) {
		queue_delete((void*)p);
		readyqueue_insert(p);
		*((UW*)(p->sp[0x10])) = data;	/* r2 */
		return ei_ret(E_OK);
	}
	if (q->remain > 0) {
		q->data[q->wpos++] = data;
		if (q->wpos >= q->size)
			q->wpos = 0;
		q->remain--;
		return ei_ret(E_OK);
	}
	if (tmout == TMO_POL)
		return ei_ret(E_TMOUT);
	p = find_tcb(TSK_SELF);
	p->sus_ttw = TTW_SDTQ;	/* sus == 0 */
	p->tmout = tmout + 1;
	queue_delete((void*)p);
	queue_insert((void*)(&q->stask), (void*)p);
	return E_OK;		/* no ei_ret() */
}


W	ifsnd_dtq(W dtqid, UW data)
{
	struct	struct_tcb	*p;
	struct	struct_dtq	*q;
	
	getpsw_di();
	if ((q = find_dtq(dtqid)) == NULL)
		return ei_ret(E_ID);
	if (q->size < 0)
		return ei_ret(E_NOEXS);
	if (q->size == 0)
		return ei_ret(E_ILUSE);
	if ((p = (void*)q->rtask.f) != (void*)(&q->rtask)) {
		readyqueue_insert(p);
		*((UW*)(p->sp[0x10])) = data;	/* r2 */
		return ei_ret(E_OK);
	}
	if (q->remain > 0) {
		q->data[q->wpos++] = data;
		if (q->wpos >= q->size)
			q->wpos = 0;
		q->remain--;
		return ei_ret(E_OK);
	}
	q->data[q->wpos++] = data;
	if (q->wpos >= q->size)
		q->wpos = 0;
	if ((++q->rpos) >= q->size)
		q->rpos = 0;
	return ei_ret(E_OK);
}


W	itrcv_dtq(W dtqid, UW* data, W tmout)
{
	W	err;
	struct	struct_tcb	*p;
	struct	struct_dtq	*q;
	
	getpsw_di();
	if ((err = check_ctx(tmout)) < 0)
		return ei_ret(err);
	if ((q = find_dtq(dtqid)) == NULL)
		return ei_ret(E_ID);
	if (q->size < 0)
		return ei_ret(E_NOEXS);
	if (q->size == 0) {
		if ((p = (void*)q->stask.f) != (void*)(&q->stask)) {
			queue_delete((void*)p);
			readyqueue_insert(p);
			*data = *((UW*)(p->sp[0x10]));	/* r2 */
			return ei_ret(E_OK);
		}
	} else if (q->rpos != q->wpos) {
		*data = q->data[q->rpos++];
		if (q->rpos >= q->size)
			q->rpos = 0;
		q->remain++;
		if ((p = (void*)q->stask.f) != (void*)(&q->stask)) {
			queue_delete((void*)p);
			readyqueue_insert(p);
			q->data[q->wpos++] = *((UW*)(p->sp[0x10]));	/* r2 */
			if (q->wpos >= q->size)
				q->wpos = 0;
			q->remain--;
		}
		return ei_ret(E_OK);
	}
	if (tmout == TMO_POL)
		return ei_ret(E_TMOUT);
	p = find_tcb(TSK_SELF);
	p->sus_ttw = TTW_RDTQ;	/* sus == 0 */
	p->tmout = tmout + 1;
	queue_delete((void*)p);
	queue_insert((void*)(&q->rtask), (void*)p);
	return E_OK;		/* no ei_ret() */
}

#endif


W	irot_rdq(W tskpri)
{
	struct	struct_tcb	*p;
	struct	struct_readyqueue	*q;
	
	getpsw_di();
	if (tskpri == TPRI_RUN) {
		if (currenttskid == 0)
			return ei_ret(E_OK);
		p = find_tcb(currenttskid);
	} else if ((q = find_readyqueue(tskpri - 1)) == NULL)
		return ei_ret(E_PAR);
	else if (((p = (void*)(q->queue.f))->sus_ttw & 0xff))
		return ei_ret(E_OK);		/* no task */
	
	queue_delete((void*)p);
	readyqueue_insert(p);
	return ei_ret(E_OK);
}


static	W	idis_dsp(void)
{
	getpsw_di();
	sysstat |= TSS_DDSP;
	return ei_ret(E_OK);
}


static	W	iena_dsp(void)
{
	getpsw_di();
	sysstat = 0;
	return ei_ret(E_OK);
}


static	W	iloc_cpu(void)
{
/*	DI();	*/
	sysstat |= TSS_LOC;
	return E_OK;
}


static	W	iunl_cpu(void)
{
/*	DI();	*/
	sysstat &= (~TSS_LOC);
	return E_OK;
}


	/* system */


void	isig_tim(void)
{
	UW	psw0;
	W	i = 0;
	
	GETPSW_DI(psw0);
	for (i=1; i<=MAXTSKID; i++) {
		struct	struct_tcb	*p;
		
		p = tcb + i;
		if (p->tskpri <= 0)
			continue;
		if ((p->sus_ttw & 0xff) == 0)
			continue;
		if (p->tmout <= 0)
			continue;
		if ((--p->tmout) > 0)
			continue;
		if ((p->sus_ttw & 0xff00) == 0)
			readyqueue_insert(p);
		if ((p->sus_ttw & TTW_DLY) == 0)
			p->sp[0x12] = E_TMOUT;
	}
	SETPSW(psw0);
}


void	setup_itron(void)
{
	W	i;
	
	readyqueue[0].queue.b = NULL;
	for (i=0; i<MAXTSKPRI; i++) {
		readyqueue[i].queue.f = (void*)(readyqueue + i + 1);
		readyqueue[i + 1].queue.b = (void*)(readyqueue + i);
		readyqueue[i].magic = 0xff;
	}
	readyqueue[MAXTSKPRI].queue.f = (void*)tcb;	/* idle task */
	readyqueue[MAXTSKPRI].magic = 0xff;
	
	for (i=0; i<=MAXTSKID; i++) {
		tcb[i].tskid = i;
		if (i <= 0)
			continue;
		tcb[i].sp = (void*)(0x00f0f7b8 - 0x800 * i);
		tcb[i].sp[0] = 0xc080;	/* SM=1 IE=1 */
		tcb[i].sp[18] = 0;	/* r0 */
		readyqueue_insert(tcb + i);
	}
	tcb[1].sp[1] = (UW)(ata_task);	/* bpc */
	tcb[1].sp[18] = 0;		/* r0 */
	tcb[2].sp[1] = (UW)(ata_task);	/* bpc */
	tcb[2].sp[18] = 1;		/* r0 */
	tcb[3].sp[1] = (UW)(prockey);	/* bpc */
	tcb[3].sp[18] = 0;		/* r0 */
	
#if MAXSEMID > 0
	for (i=0; i<MAXSEMID; i++)
		sem[i].wtask.f = sem[i].wtask.b = &(sem[i].wtask);
#endif
#if MAXDTQID > 0
	for (i=0; i<MAXDTQID; i++) {
		dtq[i].rtask.f = dtq[i].rtask.b = &(dtq[i].rtask);
		dtq[i].stask.f = dtq[i].stask.b = &(dtq[i].stask);
	}
#endif
#if 0
putstring("tcb(");
putW((UW)tcb);
putstring(") readyqueue(");
putW((UW)readyqueue);
putstring(")\r\n");
{
	struct	struct_tcb	*p;
	
	p = (void*)readyqueue;
	do {
		putW((UW)p);
		putstring(": sus_ttw(");
		putB(p->sus_ttw);
		if (p->sus_ttw == 0) {
			putstring(") tskid(");
			putB(p->tskid);
			putstring(") tskpri(");
			putB(p->tskpri);
		}
		putstring(")\r\n");
	} while ((p = (void*)(p->queue.f)));
	putstring("\r\n");
	for (i=0; i<=MAXTSKID; i++) {
		p = tcb + i;
		putW((UW)p);
		putstring(": sus_ttw(");
		putB(p->sus_ttw);
		if ((p->sus_ttw))
			continue;
		putstring(") tskid(");
		putB(p->tskid);
		putstring(") tskpri(");
		putB(p->tskpri);
		putstring(")\r\n");
	}
}
#endif
	
	def_int(0x101, trap1_hdr_asm);
}


void	idletask(void) {
	EI();
	ena_dsp();
	for (;;)
		;
}


static	W	re_rsfn()
{
	return E_RSFN;
}


struct	{
	W	(*func)();
} itron_syscall_hdr[128] = {
	{re_rsfn}, {re_rsfn}, {re_rsfn}, {re_rsfn}, 	/* -1 */
	{re_rsfn}, {re_rsfn}, {re_rsfn}, {re_rsfn}, 
	{re_rsfn}, {re_rsfn}, {re_rsfn}, {re_rsfn}, 
	{ichg_pri}, {re_rsfn}, {re_rsfn}, {re_rsfn}, 
	{re_rsfn}, {itslp_tsk}, {iwup_tsk}, {ican_wup}, 	/* -17 */
	{irel_wai}, {isus_tsk}, {irsm_tsk}, {ifrsm_tsk}, 
	{idly_tsk}, {re_rsfn}, {re_rsfn}, {re_rsfn}, 
	{re_rsfn}, {re_rsfn}, {re_rsfn}, {re_rsfn}, 
#if MAXSEMID > 0
	{re_rsfn}, {re_rsfn}, {isig_sem}, {re_rsfn}, 	/* -33 */
	{re_rsfn}, {re_rsfn}, {itwai_sem}, {re_rsfn}, 
#else
	{re_rsfn}, {re_rsfn}, {re_rsfn}, {re_rsfn}, 	/* -33 */
	{re_rsfn}, {re_rsfn}, {re_rsfn}, {re_rsfn}, 
#endif
#if MAXFLGID > 0
	{re_rsfn}, {re_rsfn}, {iset_flg}, {iclr_flg}, 
	{re_rsfn}, {re_rsfn}, {itwai_flg}, {re_rsfn}, 
#else
	{re_rsfn}, {re_rsfn}, {re_rsfn}, {re_rsfn}, 
	{re_rsfn}, {re_rsfn}, {re_rsfn}, {re_rsfn}, 
#endif
#if MAXDTQID > 0
	{re_rsfn}, {re_rsfn}, {re_rsfn}, {re_rsfn}, 	/* -49 */
	{re_rsfn}, {re_rsfn}, {itsnd_dtq}, {ifsnd_dtq}, 
	{re_rsfn}, {re_rsfn}, {itrcv_dtq}, {re_rsfn}, 
#else
	{re_rsfn}, {re_rsfn}, {re_rsfn}, {re_rsfn}, 	/* -49 */
	{re_rsfn}, {re_rsfn}, {re_rsfn}, {re_rsfn}, 
	{re_rsfn}, {re_rsfn}, {re_rsfn}, {re_rsfn}, 
#endif
#if MAXMBXID > 0
	{re_rsfn}, {re_rsfn}, {isnd_msg}, {re_rsfn}, 
	{re_rsfn}, {re_rsfn}, {itrcv_msg}, {re_rsfn}, 	/* -65 */
#else
	{re_rsfn}, {re_rsfn}, {re_rsfn}, {re_rsfn}, 
	{re_rsfn}, {re_rsfn}, {re_rsfn}, {re_rsfn}, 	/* -65 */
#endif
	{re_rsfn}, {re_rsfn}, {re_rsfn}, {re_rsfn}, 
	{re_rsfn}, {re_rsfn}, {re_rsfn}, {re_rsfn}, 
	{re_rsfn}, {re_rsfn}, {re_rsfn}, {re_rsfn}, 
	{re_rsfn}, {re_rsfn}, {re_rsfn}, {re_rsfn}, 	/* -81 */
	{irot_rdq}, {re_rsfn}, {re_rsfn}, {re_rsfn}, 
	{iloc_cpu}, {iunl_cpu}, {idis_dsp}, {iena_dsp}, 
	{re_rsfn}, {re_rsfn}, {re_rsfn}, {re_rsfn}, 
	{re_rsfn}, {re_rsfn}, {re_rsfn}, {re_rsfn}, 	/* -97 */
	{re_rsfn}, {re_rsfn}, {re_rsfn}, {re_rsfn}, 
	{re_rsfn}, {re_rsfn}, {re_rsfn}, {re_rsfn}, 
	{re_rsfn}, {re_rsfn}, {re_rsfn}, {re_rsfn}, 
	{re_rsfn}, {re_rsfn}, {re_rsfn}, {re_rsfn}, 	/* -113 */
	{re_rsfn}, {re_rsfn}, {re_rsfn}, {re_rsfn}, 
	{re_rsfn}, {re_rsfn}, {re_rsfn}, {re_rsfn}, 
	{re_rsfn}, {re_rsfn}, {re_rsfn}, {re_rsfn}
};


