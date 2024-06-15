

#include	<basic.h>
#include	"itron.h"


#ifndef	TARGET
#TARGET	"(none)"
#endif

#ifndef	DEBUGPRINT
#define	DEBUGPRINT	2
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

	/* inthdr.c */

void	def_int(W num, void (*func)());
void	setup_int(void);
void	setup_sdram(void);

	/* main.c */

int	main(void);
void	task_id1(W stacd);
void	task_id2(W stacd);

	/* startup.s */

void	ei_hdr_asm(void);
void	trap1_hdr_asm(void);
W	itron_syscall();

