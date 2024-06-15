

#include	<basic.h>


#define	TMO_FEVR	(-1)
#define	TMO_POL	(0)

	/* main.c */

void	configuration(void);
int	main(void);

	/* trapent.s */

void	trapent(void);

	/* serial.c */

W	init_serial(void);
W	getchar(W tmout);
W	putchar(UW data, W tmout);
void	putstring(UB *p);
void	putnum(W data);
void	putB(UW data);
void	putH(UW data);
void	putW(UW data);

	/* cli.c */
void	cli(void);

