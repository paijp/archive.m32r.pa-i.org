
#include	"mon.h"
#include	<hw.h>

static	const	UB	hex[16] = "0123456789abcdef";

#define	BUFFERSIZE	4096
static	UB	tx_buffer[BUFFERSIZE];
static	W	tx_wpos = 0;
static	W	tx_rpos = 0;
static	W	tx_size = 0;
static	UB	rx_buffer[BUFFERSIZE];
static	W	rx_wpos = 0;
static	W	rx_rpos = 0;
static	W	rx_size = 0;


W	init_serial(void)
{
	*SIO0CR = 0x0300;			/* 送受信ステータスクリア */
	*SIO0MOD0 = 0x0000;			/* パリティなし, 送受信STOP:1bit */
	*SIO0MOD1 = 0x0800;			/* ビット長:8, CMOS出力, LSBファースト, PCLK=33MHz */
	*SIO0BAUR = 17;				/* ボーレートレジスタ: 115,200bps */
	*SIO0RBAUR = 2;				/* ボーレート補正レジスタ */
	*SIO0CR = 0x0003;			/* TXイネーブル, RXイネーブル */
	tx_rpos = tx_wpos = tx_size = rx_rpos = rx_wpos = rx_size = 0;
	return 0;
}


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
	if ((*SIO0STS & 0x1)) {
		*SIO0TXB = data;
		return 1;
	}
	return -1;		/* buffer full */
}


W	getchar(W tmout)
{
	W	c;
	
	if (rx_size > 0) {
		W	val;
		
		val = rx_buffer[rx_rpos++];
		if (rx_rpos >= BUFFERSIZE)
			rx_rpos = 0;
		rx_size--;
		return val;
	}
	do {
		if (tx_size <= 0)
			;
		else if (hw_putchar(tx_buffer[tx_rpos]) > 0) {
			tx_rpos++;
			if (tx_rpos >= BUFFERSIZE)
				tx_rpos = 0;
			tx_size--;
		}
		if ((c = hw_getchar()) >= 0)
			return c;
	} while ((tmout));
	return -1;	/* no-data */
}


W	putchar(UW data, W tmout)
{
	W	c;
	
	do {
		if (rx_size >= BUFFERSIZE)
			;
		else if ((c = hw_getchar()) >= 0) {
			rx_buffer[rx_wpos++] = c;
			if (rx_wpos >= BUFFERSIZE)
				rx_wpos = 0;
			rx_size++;
		}
		if (tx_size <= 0) {
			if ((c = hw_putchar(data)) > 0)
				return c;
		} else if (hw_putchar(tx_buffer[tx_rpos]) > 0) {
			tx_rpos++;
			if (tx_rpos >= BUFFERSIZE)
				tx_rpos = 0;
			tx_size--;
		}
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