

#include	"itrondef.h"

#define	cre_tsk(tskid, pk_ctsk)	itron_syscall(-5, tskid, pk_ctsk)
W	icre_tsk(W tskid, T_CTSK *pk_ctsk);
#define	del_tsk(tskid)	itron_syscall(-6, tskid)
W	idel_tsk(W tskid);
#define	act_tsk(tskid)	itron_syscall(-7, tskid)
W	iact_tsk(W tskid);
#define	can_act(tskid)	itron_syscall(-8, tskid)
W	ican_act(W tskid);
#define	sta_tsk(tskid, stacd)	itron_syscall(-9, tskid, stacd)
W	ista_tsk(W tskid, UW stacd);
#define	ext_tsk()	itron_syscall(-10)
#define	exd_tsk()	itron_syscall(-11)
#define	ter_tsk(tskid)	itron_syscall(-12, tskid)
W	iter_tsk(W tskid);
#define	chg_pri(tskid, tskpri)	itron_syscall(-13, tskid, tskpri)
W	ichg_pri(W tskid, W tskpri);
#define	ref_tsk()	itron_syscall(-15, )
W	iref_tsk();
#define	tslp_tsk(tmout)	itron_syscall(-18, tmout)
#define	slp_tsk()	itron_syscall(-18, TMO_FEVR)
#define	wup_tsk(tskid)	itron_syscall(-19, tskid)
W	iwup_tsk(W tskid);
#define	can_wup(tskid)	itron_syscall(-20, tskid)
W	ican_wup(W tskid);
#define	rel_wai(tskid)	itron_syscall(-21, tskid)
W	irel_wai(W tskid);

#define	sus_tsk(tskid)	itron_syscall(-22, tskid)
W	isus_tsk(W tskid);
#define	rsm_tsk(tskid)	itron_syscall(-23, tskid)
W	irsm_tsk(W tskid);
#define	frsm_tsk(tskid)	itron_syscall(-24, tskid)
W	ifrsm_tsk(W tskid);
#define	dly_tsk(dlytim)	itron_syscall(-25, dlytim)

#define	cre_sem(semid, pk_csem)	itron_syscall(-33, semid, pk_csem)
W	icre_sem(W semid, T_CSEM *pk_csem);
#define	del_sem(semid)	itron_syscall(-34, semid)
W	idel_sem(W semid);
#define	sig_sem(semid)	itron_syscall(-35, semid)
W	isig_sem(W semid);
#define	wai_sem(semid)	itron_syscall(-39, semid, TMO_FEVR)
#define	pol_sem(semid)	itron_syscall(-39, semid, TMO_POL)
#define	twai_sem(semid, tmout)	itron_syscall(-39, semid, tmout)
W	itwai_sem(W semid, W tmout);
#define	ref_sem()	itron_syscall(-40, )

#define	cre_flg(flgid, pk_cflg)	itron_syscall(-41, flgid, pk_cflg)
W	icre_flg(W flgid, T_CFLG *pk_cflg);
#define	del_flg(flgid)	itron_syscall(-42, flgid)
W	idel_flg(W flgid);
#define	set_flg(flgid, setptn)	itron_syscall(-43, flgid, setptn)
W	iset_flg(W flgid, UW setptn);
#define	clr_flg(flgid, clrptn)	itron_syscall(-44, flgid, clrptn)
W	iclr_flg(W flgid, UW clrptn);
#define	wai_flg(flgid, waiptn, wfmode, p_flgptn)	itron_syscall(-47, flgid, waiptn, wfmode, p_flgptn, TMO_FEVR)
#define	pol_flg(flgid, waiptn, wfmode, p_flgptn)	itron_syscall(-47, flgid, waiptn, wfmode, p_flgptn, TMO_POL)
#define	twai_flg(flgid, waiptn, wfmode, p_flgptn, tmout)	itron_syscall(-47, flgid, waiptn, wfmode, p_flgptn, tmout)
	/* too many arguments : i..._... */
#define	ref_flg()	itron_syscall(-48, )

#define	cre_dtq(dtqid, pk_cdtq)	itron_syscall(-49, dtqid, pk_cdtq)
W	icre_dtq(W dtqid, T_CDTQ *pk_cdtq);
#define	del_dtq(dtqid)	itron_syscall(-50, dtqid)
W	idel_dtq(W dtqid);
W	itsnd_dtq(W dtqid, UW data, W tmout);
#define	ipsnd_dtq(dtqid, data)	itsnd_dtq(dtqid, data, TMO_POL)
#define	tsnd_dtq(dtqid, data, tmout)	itron_syscall(-55, dtqid, data, tmout)
#define	snd_dtq(dtqid, data)	itron_syscall(-55, dtqid, data, TMO_FEVR)
#define	psnd_dtq(dtqid, data)	itron_syscall(-55, dtqid, data, TMO_POL)
W	ifsnd_dtq(W dtqid, UW data);
#define	fsnd_dtq(dtqid, data)	itron_syscall(-56, dtqid, data, TMO_POL)
W	itrcv_dtq(W dtqid, UW *p_data, W tmout);
#define	iprcv_dtq(dtqid, data)	itrcv_dtq(dtqid, data, TMO_POL)
#define	trcv_dtq(dtqid, p_data, tmout)	itron_syscall(-59, dtqid, p_data, tmout)
#define	rcv_dtq(dtqid, p_data)	itron_syscall(-59, dtqid, p_data, TMO_FEVR)
#define	prcv_dtq(dtqid, p_data)	itron_syscall(-59, dtqid, p_data, TMO_POL)
#define	ref_dtq()	itron_syscall(-60, )

#define	cre_mbx(mbxid, pk_cmbx)	itron_syscall(-61, mbxid, pk_cmbx)
W	icre_mbx(W mbxid, T_CMBX *pk_cmbx);
#define	del_mbx(mbxid)	itron_syscall(-62, mbxid)
W	idel_mbx(W mbxid);
#define	snd_msg(mbxid, pk_msg)	itron_syscall(-63, mbxid, pk_msg)
W	isnd_msg(W mbxid, T_MSG *pk_msg);
#define	rcv_msg(mbxid, pk_msg)	itron_syscall(-65, mbxid, pk_msg, TMO_FEVR)
#define	prcv_msg(mbxid, pk_msg)	itron_syscall(-66, mbxid, pk_msg, TMO_POL)
#define	trcv_msg(mbxid, pk_msg, tmout)	itron_syscall(-67, mbxid, pk_msg, tmout)
W	itrcv_msg(W mbxid, T_MSG **ppk_msg, W tmout);
#define	ref_mbx()	itron_syscall(-68, )

#define	set_tim()	itron_syscall(-77, )
#define	get_tim()	itron_syscall(-78, )
#define	cre_cyc()	itron_syscall(-79, )
#define	del_cyc()	itron_syscall(-80, )
#define	sta_cyc()	itron_syscall(-81, )
#define	stp_cyc()	itron_syscall(-82, )
#define	ref_cyc()	itron_syscall(-83, )

#define	rot_rdq(tskpri)	itron_syscall(-85, tskpri)
W	irot_rdq(W tskpri);
#define	get_tid()	itron_syscall(-86)
W	iget_tid(void);

#define	dis_dsp()	itron_syscall(-91)
#define	ena_dsp()	itron_syscall(-92)

#define	ref_sys()	itron_syscall(-97, )
W	iref_sys();
#define	ref_ver()	itron_syscall(-102, )
W	iref_ver();

#define	sig_tim()	itron_syscall(-125)
W	isig_tim(void);

#define	loc_cpu()	DI()
#define	unl_cpu()	EI()

void	setup_itron(void);
void	idletask(void);

