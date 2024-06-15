

#define	E_OK	0
#define	E_SYS	(-5)
#define	E_NOSPT	(-9)
#define	E_RSFN	(-10)
#define	E_RSATR	(-11)
#define	E_PAR	(-17)
#define	E_ID	(-18)
#define	E_CTX	(-25)
#define	E_OACV	(-27)
#define	E_MACV	(-26)
#define	E_ILUSE	(-28)
#define	E_NOMEM	(-33)
#define	E_NOID	(-34)
#define	E_OBJ	(-41)
#define	E_NOEXS	(-42)
#define	E_QOVR	(-43)
#define	E_RLWAI	(-49)
#define	E_TMOUT	(-50)
#define	E_DLT	(-51)
#define	E_CLS	(-52)
#define	E_WBLK	(-57)
#define	E_BOVR	(-58)

#define	TSK_SELF	0

#define	TMO_FEVR	(-1)
#define	TMO_POL	(0)

#define	TPRI_RUN	0
#define	TPRI_INI	0

#define	TA_ACT	2

#define	TA_TFIFO	0
#define	TA_WSGL		0
#define	TA_WMUL		2
#define	TA_CLR		4

#define	TWF_ANDW	0
#define	TWF_ORW		1

typedef	struct	struct_t_ctsk {
	UW	tskatr;
	UW	exinf;
	void	(*task)();
	UH	itskpri;
#if 0
	UH	stksz;
	void	*stk;
#endif
} T_CTSK;

typedef	struct	struct_t_csem {
	UW	sematr;
	UW	isemcnt;
	UW	maxsem;
} T_CSEM;

typedef	struct	struct_t_cflg {
	UW	flgatr;
	UW	iflgptn;
} T_CFLG;

typedef	struct	struct_t_cdtq {
	UW	dtqatr;
	UW	dtqcnt;
#if 0
	void	*dtq;
#endif
} T_CDTQ;

typedef	struct	struct_t_cmbx {
	UW	mbxatr;
} T_CMBX;

typedef	struct struct_t_msg {
	struct {
		struct	struct_queue	*f, *b;
	} queue;
} T_MSG;

	/* depends on uITRON4.0 */
#define	TTW_SLP		1
#define	TTW_DLY		2
#define	TTW_SEM		4
#define	TTW_FLG		8
#define	TTW_SDTQ	0x10
#define	TTW_RDTQ	0x20
#define	TTW_MBX		0x40

	/* depends on uITRON3.0 */
#define	TSS_DDSP	1
#define	TSS_LOC		2	/* 3 on uITRON3.0 */


