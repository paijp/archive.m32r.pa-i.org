
#define	TARGET	"itron"
#include	"itronts.h"
#include	<hw.h>

#define	MAXTSKID	2
#define	MAXTSKPRI	8
#define	MAXSEMID	4
#define	MAXFLGID	4
#define	MAXDTQID	4
#define	MAXDTQSIZE	4
#define	MAXMBXID	4

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
	UB	actcnt;
	W	tmout;
	UW*	sp;
	void	(*task)();
	UW	exinf;
} tcb[MAXTSKID + 1] = {
	{{NULL, NULL}, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, 0}
};

#define	TTW_ORW		0x80		/* wai_flg(TWF_ORW) */

static	W	currenttskid = 0;	/* idle task */
static	W	sysstat = 0;		/* dispatchable */
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


static	void	exit_task(void)
{
	ext_tsk();		/* return from task */
}


static	void	startup_task(struct struct_tcb *p, UW stacd)
{
	p->sus_ttw = 0;
	p->tskpri = p->itskpri;
	p->wupcnt = 0;
	p->tmout = 0;
	p->sp = (void*)(0x00f0efb8 - 0x1000 * p->tskid);
	p->sp[0] = 0xc080;	/* SM=1 IE=1 */
	p->sp[1] = (UW)p->task;	/* bpc */
	p->sp[4] = (UW)exit_task;	/* r14 */
	p->sp[18] = stacd;	/* r0 */
	readyqueue_insert(p);
}


	/* API */


W	icre_tsk(W tskid, T_CTSK *pk_ctsk)
{
	struct	struct_tcb	*p;
	
	getpsw_di();
	if ((p = find_tcb(tskid)) == NULL)
		return ei_ret(E_ID);
	if (p->tskpri != 0)
		return ei_ret(E_OBJ);
	if ((pk_ctsk->itskpri <= 0)||(pk_ctsk->itskpri > MAXTSKPRI))
		return ei_ret(E_PAR);
	p->tskid = tskid;
	p->tskpri = -1;		/* dormant */
	p->itskpri = pk_ctsk->itskpri;
	p->actcnt = 0;
	p->task = pk_ctsk->task;
	p->exinf = pk_ctsk->exinf;
	
	if ((pk_ctsk->tskatr & TA_ACT))
		(void)iact_tsk(tskid);
	return ei_ret(E_OK);
}


W	idel_tsk(W tskid)
{
	struct	struct_tcb	*p;
	
	getpsw_di();
	if ((p = find_tcb(tskid)) == NULL)
		return ei_ret(E_ID);
	if (p->tskpri >= 0)
		return ei_ret(E_OBJ);
	p->tskpri = 0;		/* no-exist */
	return ei_ret(E_OK);
}


W	iact_tsk(W tskid)
{
	struct	struct_tcb	*p;
	
	getpsw_di();
	if ((p = find_tcb(tskid)) == NULL)
		return ei_ret(E_ID);
	if (p->tskpri == 0)
		return ei_ret(E_NOEXS);
	if (p->tskpri > 0) {
		if ((++p->actcnt) == 0) {
			p->actcnt--;
			return ei_ret(E_QOVR);
		}
		return ei_ret(E_OK);
	}
	startup_task(p, p->exinf);
	return ei_ret(E_OK);
}


W	ican_act(W tskid)
{
	struct	struct_tcb	*p;
	W	err;
	
	getpsw_di();
	if ((p = find_tcb(tskid)) == NULL)
		return ei_ret(E_ID);
	if (p->tskpri == 0)
		return ei_ret(E_NOEXS);
	err = p->actcnt;
	p->actcnt = 0;
	return ei_ret(err);
}


W	ista_tsk(W tskid, UW stacd)
{
	struct	struct_tcb	*p;
	
	getpsw_di();
	if ((p = find_tcb(tskid)) == NULL)
		return ei_ret(E_ID);
	if (p->tskpri == 0)
		return ei_ret(E_NOEXS);
	if (p->tskpri > 0)
		return ei_ret(E_OBJ);
	startup_task(p, stacd);
	return ei_ret(E_OK);
}


static	W	iext_tsk(void)
{
	struct	struct_tcb	*p;
	
	getpsw_di();
/*
	if ((err = check_ctx(1)) < 0)
		return ei_ret(err);
*/
	p = find_tcb(TSK_SELF);
	p->tskpri = -1;		/* dormant */
	queue_delete((void*)p);
	if (p->actcnt <= 0)
		return E_OK;		/* no ei_ret() */
	p->actcnt--;
	startup_task(p, p->exinf);
	return ei_ret(p->exinf);
}


static	W	iexd_tsk(void)
{
	struct	struct_tcb	*p;
	
	getpsw_di();
/*
	if ((err = check_ctx(1)) < 0)
		return ei_ret(err);
*/
	p = find_tcb(TSK_SELF);
	p->tskpri = 0;		/* no-exist */
	queue_delete((void*)p);
	return E_OK;		/* no ei_ret() */
}


W	iter_tsk(W tskid)
{
	struct	struct_tcb	*p;
	
	getpsw_di();
	if ((p = find_tcb(tskid)) == NULL)
		return ei_ret(E_ID);
	if (p->tskpri <= 0)
		return ei_ret(E_NOEXS);
	p->tskpri = -1;		/* dormant */
	if (p->sus_ttw == 0)
		queue_delete((void*)p);	/* ready-queue */
	else if ((p->sus_ttw & (TTW_SEM|TTW_FLG|TTW_SDTQ|TTW_RDTQ|TTW_MBX)))
		queue_delete((void*)p);	/* object-queue */
	if (p->actcnt <= 0)
		return E_OK;		/* no ei_ret() */
	p->actcnt--;
	startup_task(p, p->exinf);
	return ei_ret(E_OK);
}


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
	{0, 0, {NULL, NULL}}
};


static	inline	struct	struct_sem	*find_sem(W id)
{
	if (id <= 0)
		return NULL;
	if (id > MAXSEMID)
		return NULL;
	return sem + (id - 1);
}


W	icre_sem(W semid, T_CSEM *pk_csem)
{
	struct	struct_sem	*q;
	
	getpsw_di();
	if ((q = find_sem(semid)) == NULL)
		return ei_ret(E_ID);
	if (q->maxcnt >= 0)
		return ei_ret(E_OBJ);
	if ((pk_csem->maxsem <= 0)||(pk_csem->maxsem > 255))
		return ei_ret(E_PAR);
	if (/*(pk_csem->isemcnt < 0)||*/(pk_csem->isemcnt > pk_csem->maxsem))
		return ei_ret(E_PAR);
	q->maxcnt = pk_csem->maxsem;
	q->cnt = pk_csem->isemcnt;
	q->wtask.f = q->wtask.b = (&q->wtask);
	return ei_ret(E_OK);
}


W	idel_sem(W semid)
{
	struct	struct_tcb	*p;
	struct	struct_sem	*q;
	
	getpsw_di();
	if ((q = find_sem(semid)) == NULL)
		return ei_ret(E_ID);
	if (q->maxcnt <= 0)
		return ei_ret(E_NOEXS);
	while ((p = (void*)q->wtask.f) != (void*)(&q->wtask)) {
		queue_delete((void*)p);
		readyqueue_insert(p);
		p->sp[0x12] = E_DLT;
	}
	q->maxcnt = -1;
	return ei_ret(E_OK);
}


W	isig_sem(W semid)
{
	struct	struct_tcb	*p;
	struct	struct_sem	*q;
	
	getpsw_di();
	if ((q = find_sem(semid)) == NULL)
		return ei_ret(E_ID);
	if (q->maxcnt <= 0)
		return ei_ret(E_NOEXS);
	if ((p = (void*)q->wtask.f) != (void*)(&q->wtask)) {
		queue_delete((void*)p);
		readyqueue_insert(p);
		return ei_ret(E_OK);
	}
	if ((q->cnt++) >= q->maxcnt) {
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
	if (q->maxcnt <= 0)
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

#if MAXFLGID > 0

static	struct	struct_flg {
	struct	struct_queue	wtask;	/* common to struct_tcb */
	H	attr;
	UW	flgptn;
} flg[MAXFLGID] = {
	{{NULL, NULL}, -1, 0}
};


static	inline	struct	struct_flg	*find_flg(W id)
{
	if (id <= 0)
		return NULL;
	if (id > MAXFLGID)
		return NULL;
	return flg + (id - 1);
}


W	icre_flg(W flgid, T_CFLG *pk_cflg)
{
	struct	struct_flg	*q;
	
	getpsw_di();
	if ((q = find_flg(flgid)) == NULL)
		return ei_ret(E_ID);
	if (q->attr >= 0)
		return ei_ret(E_OBJ);
	if ((pk_cflg->flgatr & (~(TA_WMUL|TA_CLR))))
		return ei_ret(E_PAR);
	q->attr = pk_cflg->flgatr;
	q->flgptn = pk_cflg->iflgptn;
	q->wtask.f = q->wtask.b = &(q->wtask);
	return	ei_ret(E_OK);
}


W	idel_flg(W flgid)
{
	struct	struct_tcb	*p;
	struct	struct_flg	*q;
	
	getpsw_di();
	if ((q = find_flg(flgid)) == NULL)
		return ei_ret(E_ID);
	if (q->attr < 0)
		return ei_ret(E_NOEXS);
	while ((p = (void*)q->wtask.f) != (void*)(&q->wtask)) {
		queue_delete((void*)p);
		readyqueue_insert(p);
		p->sp[0x12] = E_DLT;
	}
	q->attr = -1;
	return ei_ret(E_OK);
}


W	iset_flg(W flgid, UW setptn)
{
	struct	struct_tcb	*p;
	struct	struct_flg	*q;
	
	getpsw_di();
	if ((q = find_flg(flgid)) == NULL)
		return ei_ret(E_ID);
	if (q->attr < 0)
		return ei_ret(E_NOEXS);
	setptn = q->flgptn |= setptn;
	p = (void*)&q->wtask;
	while ((p = (void*)p->queue.f) != (void*)(&q->wtask)) {
		if ((p->sus_ttw & TTW_ORW)) {
			if ((*((UW*)(p->sp[0x10])) & setptn) == 0)	/* r2 */
				continue;
		} else if ((*((UW*)(p->sp[0x10])) & (~setptn)))
			continue;
		*((UW*)(p->sp[0x13])) = setptn;	/* p_flgptn */
		queue_delete((void*)p);
		readyqueue_insert(p);
		if ((q->attr & TA_CLR)) {
			q->flgptn = 0;
			break;
		}
	}
	return ei_ret(E_OK);
}


W	iclr_flg(W flgid, UW clrptn)
{
	struct	struct_flg	*q;
	
	getpsw_di();
	if ((q = find_flg(flgid)) == NULL)
		return ei_ret(E_ID);
	if (q->attr < 0)
		return ei_ret(E_NOEXS);
	q->flgptn &= clrptn;
	return ei_ret(E_OK);
}


static	W	itwai_flg(W flgid, UW waiptn, UW wfmode, UW *p_flgptn, W tmout)
{
	W	err;
	struct	struct_tcb	*p;
	struct	struct_flg	*q;
	
	p = find_tcb(TSK_SELF);
	p_flgptn = (UW*)(p->sp[0x13]);
	tmout = (W)(p->sp[0x14]);
	
	getpsw_di();
	if ((err = check_ctx(tmout)) < 0)
		return ei_ret(err);
	if ((q = find_flg(flgid)) == NULL)
		return ei_ret(E_ID);
	if (q->attr < 0)
		return ei_ret(E_NOEXS);
	if (waiptn == 0)
		return ei_ret(E_PAR);
	if ((q->attr & TA_WMUL))
		;
	else if ((void*)q->wtask.f != (void*)(&q->wtask))
		return ei_ret(E_ILUSE);
	if ((wfmode & TWF_ORW)) {
		if ((waiptn & q->flgptn)) {
			*p_flgptn = q->flgptn;
			if ((q->attr & TA_CLR))
				q->flgptn = 0;
			return ei_ret(E_OK);
		}
	} else if ((waiptn & (~q->flgptn)) == 0) {
		*p_flgptn = q->flgptn;
		if ((q->attr & TA_CLR))
			q->flgptn = 0;
		return ei_ret(E_OK);
	}
	if (tmout == TMO_POL)
		return ei_ret(E_TMOUT);
	p->sus_ttw = TTW_FLG | ((wfmode & TWF_ORW)? TTW_ORW : 0);
		/* sus == 0 */
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
	{-1, 0, 0, 0, {0}, {NULL, NULL}, {NULL, NULL}}
};


static	inline	struct	struct_dtq	*find_dtq(W id)
{
	if (id <= 0)
		return NULL;
	if (id > MAXDTQID)
		return NULL;
	return dtq + (id - 1);
}


W	icre_dtq(W dtqid, T_CDTQ *pk_cdtq)
{
	struct	struct_dtq	*q;
	
	getpsw_di();
	if ((q = find_dtq(dtqid)) == NULL)
		return ei_ret(E_ID);
	if (q->size >= 0)
		return ei_ret(E_OBJ);
	if ((pk_cdtq->dtqatr))
		return ei_ret(E_PAR);
	if (pk_cdtq->dtqcnt > MAXDTQSIZE)
		return ei_ret(E_PAR);
	q->size = q->remain = pk_cdtq->dtqcnt;
	q->wpos = q->rpos = 0;
	q->rtask.f = q->rtask.b = &(q->rtask);
	q->stask.f = q->stask.b = &(q->stask);
	return ei_ret(E_OK);
}


W	idel_dtq(W dtqid)
{
	struct	struct_tcb	*p;
	struct	struct_dtq	*q;
	
	getpsw_di();
	if ((q = find_dtq(dtqid)) == NULL)
		return ei_ret(E_ID);
	if (q->size < 0)
		return ei_ret(E_NOEXS);
	while ((p = (void*)q->stask.f) != (void*)(&q->stask)) {
		queue_delete((void*)p);
		readyqueue_insert(p);
		p->sp[0x12] = E_DLT;
	}
	while ((p = (void*)q->rtask.f) != (void*)(&q->rtask)) {
		queue_delete((void*)p);
		readyqueue_insert(p);
		p->sp[0x12] = E_DLT;
	}
	q->size = -1;
	return ei_ret(E_OK);
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

#if MAXMBXID > 0

static	struct	struct_mbx {
	H	attr;
	struct	struct_queue	wtask;
	struct	struct_queue	msg;
} mbx[MAXMBXID] = {
	{-1, {NULL, NULL}, {NULL, NULL}}
};


static	inline	struct	struct_mbx	*find_mbx(W id)
{
	if (id <= 0)
		return NULL;
	if (id > MAXMBXID)
		return NULL;
	return mbx + (id - 1);
}


W	icre_mbx(W mbxid, T_CMBX *pk_cmbx)
{
	struct	struct_mbx	*q;
	
	getpsw_di();
	if ((q = find_mbx(mbxid)) == NULL)
		return ei_ret(E_ID);
	if (q->attr >= 0)
		return ei_ret(E_OBJ);
	if ((pk_cmbx->mbxatr))
		return ei_ret(E_PAR);
	q->attr = pk_cmbx->mbxatr;
	q->wtask.f = q->wtask.b = &(q->wtask);
	q->msg.f = q->msg.b = &(q->msg);
	return ei_ret(E_OK);
}


W	idel_mbx(W mbxid)
{
	struct	struct_tcb	*p;
	struct	struct_mbx	*q;
	
	getpsw_di();
	if ((q = find_mbx(mbxid)) == NULL)
		return ei_ret(E_ID);
	if (q->attr < 0)
		return ei_ret(E_NOEXS);
	while ((p = (void*)q->wtask.f) != (void*)(&q->wtask)) {
		queue_delete((void*)p);
		readyqueue_insert(p);
		p->sp[0x12] = E_DLT;
	}
	q->attr = -1;
	return ei_ret(E_OK);
}


W	isnd_msg(W mbxid, T_MSG *pk_msg)
{
	struct	struct_tcb	*p;
	struct	struct_mbx	*q;
	
	getpsw_di();
	if ((q = find_mbx(mbxid)) == NULL)
		return ei_ret(E_ID);
	if (q->attr < 0)
		return ei_ret(E_NOEXS);
	if ((p = (void*)q->wtask.f) != (void*)(&q->wtask)) {
		*((T_MSG**)(p->sp[0x10])) = pk_msg;	/* r2 */
		queue_delete((void*)p);
		readyqueue_insert(p);
		return ei_ret(E_OK);
	}
	queue_insert((void*)&q->msg, (void*)pk_msg);
	return ei_ret(E_OK);
}


W	itrcv_msg(W mbxid, T_MSG **ppk_msg, W tmout)
{
	W	err;
	struct	struct_tcb	*p;
	struct	struct_mbx	*q;
	T_MSG	*r;
	
	getpsw_di();
	if ((err = check_ctx(tmout)) < 0)
		return ei_ret(err);
	if ((q = find_mbx(mbxid)) == NULL)
		return ei_ret(E_ID);
	if (q->attr < 0)
		return ei_ret(E_NOEXS);
	if ((r = (void*)q->msg.f) != (void*)(&q->msg)) {
		queue_delete((void*)r);
		*ppk_msg = r;
		return ei_ret(E_OK);
	}
	if (tmout == TMO_POL)
		return ei_ret(E_TMOUT);
	p = find_tcb(TSK_SELF);
	p->sus_ttw = TTW_MBX;	/* sus == 0 */
	p->tmout = tmout + 1;
	queue_delete((void*)p);
	queue_insert((void*)(&q->wtask), (void*)p);
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


W	isig_tim(void)
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
	return E_OK;
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
	
#if MAXFLGID > 0
	for (i=0; i<MAXFLGID; i++)
		flg[i].attr = -1;
#endif
#if MAXDTQID > 0
	for (i=0; i<MAXDTQID; i++)
		dtq[i].size = -1;
#endif
#if MAXMBXID > 0
	for (i=0; i<MAXMBXID; i++)
		mbx[i].attr = -1;
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
	{icre_tsk}, {idel_tsk}, {iact_tsk}, {ican_act}, 
	{ista_tsk}, {iext_tsk}, {iexd_tsk}, {iter_tsk}, 
	{ichg_pri}, {re_rsfn}, {re_rsfn}, {re_rsfn}, 
	{re_rsfn}, {itslp_tsk}, {iwup_tsk}, {ican_wup}, 	/* -17 */
	{irel_wai}, {isus_tsk}, {irsm_tsk}, {ifrsm_tsk}, 
	{idly_tsk}, {re_rsfn}, {re_rsfn}, {re_rsfn}, 
	{re_rsfn}, {re_rsfn}, {re_rsfn}, {re_rsfn}, 
#if MAXSEMID > 0
	{icre_sem}, {idel_sem}, {isig_sem}, {re_rsfn}, 	/* -33 */
	{re_rsfn}, {re_rsfn}, {itwai_sem}, {re_rsfn}, 
#else
	{re_rsfn}, {re_rsfn}, {re_rsfn}, {re_rsfn}, 	/* -33 */
	{re_rsfn}, {re_rsfn}, {re_rsfn}, {re_rsfn}, 
#endif
#if MAXFLGID > 0
	{icre_flg}, {idel_flg}, {iset_flg}, {iclr_flg}, 	/* -41 */
	{re_rsfn}, {re_rsfn}, {itwai_flg}, {re_rsfn}, 
#else
	{re_rsfn}, {re_rsfn}, {re_rsfn}, {re_rsfn}, 	/* -41 */
	{re_rsfn}, {re_rsfn}, {re_rsfn}, {re_rsfn}, 
#endif
#if MAXDTQID > 0
	{icre_dtq}, {idel_dtq}, {re_rsfn}, {re_rsfn}, 	/* -49 */
	{re_rsfn}, {re_rsfn}, {itsnd_dtq}, {ifsnd_dtq}, 
	{re_rsfn}, {re_rsfn}, {itrcv_dtq}, {re_rsfn}, 
#else
	{re_rsfn}, {re_rsfn}, {re_rsfn}, {re_rsfn}, 	/* -49 */
	{re_rsfn}, {re_rsfn}, {re_rsfn}, {re_rsfn}, 
	{re_rsfn}, {re_rsfn}, {re_rsfn}, {re_rsfn}, 
#endif
#if MAXMBXID > 0
	{icre_mbx}, {idel_mbx}, {isnd_msg}, {re_rsfn}, 
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
	{isig_tim}, {re_rsfn}, {re_rsfn}, {re_rsfn}
};


