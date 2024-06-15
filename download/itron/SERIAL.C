
#define	TARGET	"serial"
#include	"itronts.h"
#include	<hw.h>


static	const	UB	hex[16] = "0123456789abcdef";

	/* <0 : no data */
static	W	hw_getchar()
{
	if ((*SIO0STS & 0x20)) {	/* over-run */
		*SIO0CR = 0x0300;			/* 送受信ステータスクリア */
		*SIO0CR = 0x0003;			/* TXイネーブル, RXイネーブル */
	}
	if ((*SIO0STS & 0x4))
		return *SIO0RXB;
	return -1;		/* no data */
}


static	W	hw_putchar(UW data)
{
	if ((*SIO0STS & 0x20)) {	/* over-run */
		*SIO0CR = 0x0300;			/* 送受信ステータスクリア */
		*SIO0CR = 0x0003;			/* TXイネーブル, RXイネーブル */
	}
	if ((*SIO0STS & 0x1)) {
		*SIO0TXB = data;
		return 1;
	}
	return -1;		/* buffer full */
}


W	getchar(W tmout)
{
	W	c;
	
	do {
		if ((c = hw_getchar()) >= 0)
			return c;
	} while ((tmout));
	return -1;	/* no-data */
}


W	putchar(UW data, W tmout)
{
	W	c;
	
	do {
		if ((c = hw_putchar(data)) > 0)
			return c;
	} while ((tmout));
	return -1;
}


void	putstring(UB *p)
{
	UB	c;
	
	while ((c = *(p++))) {
		if (c == '\n')
			putchar('\r', TMO_FEVR);
		putchar(c, TMO_FEVR);
	}
	return;
}


void	putnum(W data)
{
	static	UB	buf[20];
	UB	*p;
	
	if (data < 0) {
		putchar('-', TMO_FEVR);
		data = (-data);
	}
	p = buf + sizeof(buf);
	*(--p) = 0;
	do {
		*(--p) = '0' + (data % 10);
		data /= 10;
	} while (data > 0);
	putstring(p);
	return;
}


void	putB(UW data)
{
	putchar(hex[(data >> 4) & 0xf], TMO_FEVR);
	putchar(hex[data & 0xf], TMO_FEVR);
}


void	putH(UW data)
{
	putchar(hex[(data >> 12) & 0xf], TMO_FEVR);
	putchar(hex[(data >> 8) & 0xf], TMO_FEVR);
	putchar(hex[(data >> 4) & 0xf], TMO_FEVR);
	putchar(hex[data & 0xf], TMO_FEVR);
}


void	putW(UW data)
{
	putchar(hex[(data >> 28) & 0xf], TMO_FEVR);
	putchar(hex[(data >> 24) & 0xf], TMO_FEVR);
	putchar(hex[(data >> 20) & 0xf], TMO_FEVR);
	putchar(hex[(data >> 16) & 0xf], TMO_FEVR);
	putchar(hex[(data >> 12) & 0xf], TMO_FEVR);
	putchar(hex[(data >> 8) & 0xf], TMO_FEVR);
	putchar(hex[(data >> 4) & 0xf], TMO_FEVR);
	putchar(hex[data & 0xf], TMO_FEVR);
}


