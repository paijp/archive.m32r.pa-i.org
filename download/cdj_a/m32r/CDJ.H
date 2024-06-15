

#include	<basic.h>
#include	"itron.h"

#define	TMO_FEVR	(-1)
#define	TMO_POL	(0)

#ifndef	TARGET
#TARGET	"(none)"
#endif

#ifndef	DEBUGPRINT
#define	DEBUGPRINT	0
#endif

#if DEBUGPRINT
#define	D1CHAR(x)	{putchar(x, TMO_FEVR);}
#define	D1STR(x)	{putstring(TARGET);putstring(">");putstring(x);putstring("\n");}
#define	D1NUM(x, y)	{putstring(TARGET);putstring(">");putstring(x);putstring("(");putnum(y);putstring(")\n");}
#define	D1W(x, y)	{putstring(TARGET);putstring(">");putstring(x);putstring("(");putW(y);putstring(")\n");}
#define	D1H(x, y)	{putstring(TARGET);putstring(">");putstring(x);putstring("(");putH(y);putstring(")\n");}
#define	D1B(x, y)	{putstring(TARGET);putstring(">");putstring(x);putstring("(");putB(y);putstring(")\n");}
#else
#define	D1CHAR(x)	/**/
#define	D1STR(x)	/**/
#define	D1NUM(x, y)	/**/
#define	D1W(x, y)	/**/
#define	D1H(x, y)	/**/
#define	D1B(x, y)	/**/
#endif
#if DEBUGPRINT > 1
#define	D2CHAR(x)	{putchar(x, TMO_FEVR);}
#define	D2STR(x)	{putstring(TARGET);putstring(">");putstring(x);putstring("\n");}
#define	D2NUM(x, y)	{putstring(TARGET);putstring(">");putstring(x);putstring("(");putnum(y);putstring(")\n");}
#define	D2W(x, y)	{putstring(TARGET);putstring(">");putstring(x);putstring("(");putW(y);putstring(")\n");}
#define	D2H(x, y)	{putstring(TARGET);putstring(">");putstring(x);putstring("(");putH(y);putstring(")\n");}
#define	D2B(x, y)	{putstring(TARGET);putstring(">");putstring(x);putstring("(");putB(y);putstring(")\n");}
#else
#define	D2CHAR(x)	/**/
#define	D2STR(x)	/**/
#define	D2NUM(x, y)	/**/
#define	D2W(x, y)	/**/
#define	D2H(x, y)	/**/
#define	D2B(x, y)	/**/
#endif
#if DEBUGPRINT > 2
#define	D3CHAR(x)	{putchar(x, TMO_FEVR);}
#define	D3STR(x)	{putstring(TARGET);putstring(">");putstring(x);putstring("\n");}
#define	D3NUM(x, y)	{putstring(TARGET);putstring(">");putstring(x);putstring("(");putnum(y);putstring(")\n");}
#define	D3W(x, y)	{putstring(TARGET);putstring(">");putstring(x);putstring("(");putW(y);putstring(")\n");}
#define	D3H(x, y)	{putstring(TARGET);putstring(">");putstring(x);putstring("(");putH(y);putstring(")\n");}
#define	D3B(x, y)	{putstring(TARGET);putstring(">");putstring(x);putstring("(");putB(y);putstring(")\n");}
#else
#define	D3CHAR(x)	/**/
#define	D3STR(x)	/**/
#define	D3NUM(x, y)	/**/
#define	D3W(x, y)	/**/
#define	D3H(x, y)	/**/
#define	D3B(x, y)	/**/
#endif
#if DEBUGPRINT > 3
#define	D4CHAR(x)	{putchar(x, TMO_FEVR);}
#define	D4STR(x)	{putstring(TARGET);putstring(">");putstring(x);putstring("\n");}
#define	D4NUM(x, y)	{putstring(TARGET);putstring(">");putstring(x);putstring("(");putnum(y);putstring(")\n");}
#define	D4W(x, y)	{putstring(TARGET);putstring(">");putstring(x);putstring("(");putW(y);putstring(")\n");}
#define	D4H(x, y)	{putstring(TARGET);putstring(">");putstring(x);putstring("(");putH(y);putstring(")\n");}
#define	D4B(x, y)	{putstring(TARGET);putstring(">");putstring(x);putstring("(");putB(y);putstring(")\n");}
#else
#define	D4CHAR(x)	/**/
#define	D4STR(x)	/**/
#define	D4NUM(x, y)	/**/
#define	D4W(x, y)	/**/
#define	D4H(x, y)	/**/
#define	D4B(x, y)	/**/
#endif


	/* serial.c */

W	init_serial(void);
W	getchar(W tmout);
W	putchar(UW data, W tmout);
void	putstring(UB *p);
void	putnum(W data);
void	putB(UW data);
void	putH(UW data);
void	putW(UW data);

	/* itron.c */

void	setup_itron(void);
void	idletask(void);

	/* ata.c */

#define	MAXDRIVE	2
#define	MAXTRACK	99
extern	struct	info_drive_struct {
	W	maxtrack;	/* <=0:not ready */
	W	maxaddr;	/* lba / 48 */
	struct	{
		W	type;
		W	addr;	/* MSB[0dddaa..aaaa] d:drive a:LBA/48 */
		W	offset;	/* 1/64k sample */
	} track[MAXTRACK + 1];
	UW	*ata_r;
	UW	*ata_d;
	W	req_block;	/* <0:not requested */
	W	req_eject;
	W	eject_tmr;
	W	ataerr;
} info_drive[MAXDRIVE];

W	setup_ata(void);
void	ata_task(W drive);
void	ata_proc10ms(void);

	/* block.c */

#define	DABUF_UNIT	48

#define	MAXBLOCK	64
extern	struct	info_block_struct {
	W	addr;		/* MSB[0dddaa..aaaa] d:drive a:LBA/48 */
	W	track;		/* for UI */
	W	type;		/* copy of track-type */
	struct	dabuf_struct {
		union {
			UW	dummy[128 * 1024 / 4 - 588 * DABUF_UNIT];
		} h;
		UW	data[588 * DABUF_UNIT];
	} *ptr;
} info_block[MAXBLOCK];
#define	TYPE_EMP	1
	/* da.c: must 1 */
#define	TYPE_DATA	2
#define	TYPE_REQ	0x10

#define	MAXLOCATEUNIT	4	/* CURRENT CUE LOOP-START LOOP-END */
#define	MAXBUFFER	(MAXBLOCK / MAXLOCATEUNIT / MAXDRIVE)
struct	info_locategroup_struct {
	W	track;		/* for UI */
	struct	info_locateunit_struct {
		W	status;
		W	addr;	/* MSB[0dddaa..aaaa] d:drive a:LBA/48 */
		W	block[MAXBUFFER + 1];	/* block[MAXBUFFER]=0(dummy) */
		W	pos;		/* 0-7 */
		W	offset;		/* 1/64k sample */
		W	mul;		/* addr/pos (PLAY:1 FForFR:10) */
	} unit[MAXLOCATEUNIT];
};
#define	STATUS_WAIT	1


void	init_drive(W drive);
void	init_locate(struct info_locateunit_struct *l, W addr, W offset);
void	block_proc10ms(void);
void	setup_block(void);

	/* dsp.s */

void	calc_audioevent(UW *in1, UW *in2, UW *filter, W *out);
void	mix_audioevent(W *in, UB *out, UH *volume);

	/* da.c */

void	put_da(UW al, UW ar, UW bl, UW br);
void	set_monitor(W mon);
void	setup_da(void);

extern	UH	*next_wheel_pos;

	/* hmi.c */

#define	MAXWHEEL	2
extern	struct	info_wheel_struct {
	W	mode;
	W	pitch;		/* 0=100% 1024=200% */
	W	upitch;		/* 0=100% 1024=200% */
	W	pitch_step;
	W	mon_pos;	/* 1/64k sample */
	W	mon_offset;	/* 1/64k sample(max 48 frame) */
	W	drive;		/* 0 - (MAXDRIVE - 1) */
	struct	info_locategroup_struct	locate[MAXDRIVE];
	W	track_tmr;
	W	track_repeat;
	W	lm_max, lm_min;	/* level meter */
	W	volume;		/* 0-255 */
	W	cf;		/* 0-255 */
	W	wh_base;
	W	wh_vel;
	W	wh_first;
} info_wheel[MAXWHEEL];
#define	MODE_PAUSE	1
#define	MODE_MON	2
#define	MODE_LOOP	4
#define	MODE_CDJ	8
#define	MODE_REV	0x10
#define	MODE_FF		0x20
#define	MODE_FR		0x40
#define	MODE_OPITCH	0x80
#define	MODE_START	0x100
#define	MODE_DSTART	0x200
#define	MODE_CUE	0x400

void	prockey(W stacd);
void	setup_hmi(void);

	/* inthdr.c */

void	def_int(W num, void (*func)());
void	setup_int(void);
void	setup_sdram(void);

	/* main.c */

int	main(void);

	/* startup.s */

void	ei_hdr_asm(void);
void	trap1_hdr_asm(void);
W	itron_syscall();

	/* stktest.c */

void	stktest(void);

