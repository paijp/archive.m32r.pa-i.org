
#include	"mon.h"
#include	<hw.h>

extern	struct	reg_struct {
	UW	cr0, cr1, cr2, cr3, cr6;
	UW	r[15];
} *sp;

#define	BUFFERSIZE	160
	/* for S-format */
static	UB	line_buffer[BUFFERSIZE + 1];
static	W	line_size = 0;

static	UW	val_adr = 0;
#define	VAL_LINES_DEF	4
static	W	val_lines = VAL_LINES_DEF;


#define	TYPE_B	0
#define	TYPE_H	1
#define	TYPE_W	2


static	W	getline()
{
	UB	c;
	W	i;
	
	line_size = 0;
	for (;;) {
		switch (c = getchar(TMO_FEVR)) {
			case	3:	/* ^C */
				putstring("^C\n");
				line_buffer[line_size = 0] = 0;
				return -1;
			case	8:	/* BS */
			case	0x7f:	/* DEL */
				if (line_size <= 0)
					break;
				line_size--;
				putstring("\010 \010");
				continue;
			case	0xa:
			case	0xd:
				for (i=0; i<line_size; i++)
					if (line_buffer[i] == '#') {
						line_size = i;
					}
				line_buffer[line_size] = 0;
				putstring("\n");
				return line_size;
			case	0x15:	/* ^U */
				while (line_size > 0) {
					line_size--;
					putstring("\010 \010");
				}
				continue;
			default:
				if (c < 0x20)
					break;
				if (c > 0x7e)
					break;
				if (line_size >= BUFFERSIZE)
					break;
				line_buffer[line_size++] = c;
				putchar(c, TMO_POL);
				continue;
		}
		putchar(7, TMO_POL);	/* beep */
	}
}


static	W	dec2val(UB c)
{
	if ((c >= '0')&&(c <= '9'))
		return c - '0';
	return -1;
}


static	W	hex2val(UB c)
{
	if ((c >= '0')&&(c <= '9'))
		return c - '0';
	if ((c >= 'A')&&(c <= 'F'))
		return c - 'A' + 0xa;
	if ((c >= 'a')&&(c <= 'f'))
		return c - 'a' + 0xa;
	return -1;
}


static	void	cmd_d(void)
{
	W	i, c, type, pos, num;
	
	pos = 1;
	type = TYPE_B;
	switch (line_buffer[pos]) {
		case	'h':
		case	'H':
			type = TYPE_H;
			pos++;
			break;
		case	'w':
		case	'W':
			type = TYPE_W;
			pos++;
			break;
	}
	
	while ((line_buffer[pos])) {
		if ((c = hex2val(line_buffer[pos++])) < 0)
			continue;
		val_adr = c;
		val_lines = VAL_LINES_DEF;
		while ((c = hex2val(line_buffer[pos])) >= 0) {
			pos++;
			val_adr = (val_adr << 4) | c;
		}
		break;
	}
	switch (type) {
		default:
		case	TYPE_B:
			num = 16;
			break;
		case	TYPE_H:
			val_adr = (val_adr + 1) & 0xfffffffe;
			num = 8;
			break;
		case	TYPE_W:
			val_adr = (val_adr + 3) & 0xfffffffc;
			num = 4;
			break;
	}
	while ((line_buffer[pos])) {
		if ((c = dec2val(line_buffer[pos++])) < 0)
			continue;
		val_lines = c;
		while ((c = dec2val(line_buffer[pos])) >= 0) {
			pos++;
			val_lines = val_lines * 10 + c;
		}
		if (val_lines < 1)
			val_lines = 1;
		break;
	}
	for (i=0; i<val_lines; i++) {
		putW(val_adr);
		putstring(" :");
		for (pos=0; pos<num; pos++) {
			putchar(' ', TMO_FEVR);
			switch (type) {
				default:
				case	TYPE_B:
					putB(*(((_UB*)val_adr)++));
					break;
				case	TYPE_H:
					putH(*(((_UH*)val_adr)++));
					break;
				case	TYPE_W:
					putW(*(((_UW*)val_adr)++));
					break;
			}
		}
		putstring("\n");
	}
	return;
}


static	void	cmd_m(void)
{
	W	i, c, type, pos, len;
	UW	val;
	
	pos = 1;
	type = TYPE_B;
	switch (line_buffer[pos]) {
		case	'h':
		case	'H':
			type = TYPE_H;
			pos++;
			break;
		case	'w':
		case	'W':
			type = TYPE_W;
			pos++;
			break;
	}
	
	while ((line_buffer[pos])) {
		if ((c = hex2val(line_buffer[pos++])) < 0)
			continue;
		val_adr = c;
		val_lines = VAL_LINES_DEF;
		while ((c = hex2val(line_buffer[pos])) >= 0) {
			pos++;
			val_adr = (val_adr << 4) | c;
		}
		break;
	}
	switch (type) {
		default:
		case	TYPE_B:
			len = 2;
			break;
		case	TYPE_H:
			val_adr = (val_adr + 1) & 0xfffffffe;
			len = 4;
			break;
		case	TYPE_W:
			val_adr = (val_adr + 3) & 0xfffffffc;
			len = 8;
			break;
	}
	i = 0;
	while ((line_buffer[pos])) {
		if ((c = hex2val(line_buffer[pos++])) < 0)
			continue;
		val = c;
		for (i=1; i<len; i++) {
			if ((c = hex2val(line_buffer[pos])) < 0)
				break;
			pos++;
			val = (val << 4) | c;
		}
		putW(val_adr);
		putstring(" : ");
		switch (type) {
			default:
			case	TYPE_B:
				putB(*((_UB*)val_adr));
				putstring(" -> ");
				putB(*(((_UB*)val_adr)++) = val);
				break;
			case	TYPE_H:
				putH(*((_UH*)val_adr));
				putstring(" -> ");
				putH(*(((_UH*)val_adr)++) = val);
				break;
			case	TYPE_W:
				putW(*((_UW*)val_adr));
				putstring(" -> ");
				putW(*(((_UW*)val_adr)++) = val);
				break;
		}
		putstring("\n");
	}
	if ((i))
		return;
	for (;;) {
		putW(val_adr);
		putstring(" : ");
		switch (type) {
			default:
			case	TYPE_B:
				putB(*((_UB*)val_adr));
				break;
			case	TYPE_H:
				putH(*((_UH*)val_adr));
				break;
			case	TYPE_W:
				putW(*((_UW*)val_adr));
				break;
		}
		putstring(" -> ");
		if (getline() < 0)
			return;
		if (line_size <= 0) {
			switch (type) {
				default:
				case	TYPE_B:
					val_adr += sizeof(B);
					break;
				case	TYPE_H:
					val_adr += sizeof(H);
					break;
				case	TYPE_W:
					val_adr += sizeof(W);
					break;
			}
			continue;
		}
		val = 0;
		for (i=0; i<line_size; i++) {
			if ((c = hex2val(line_buffer[i])) < 0) {
				putstring("error\n");
				return;
			}
			val = (val << 4) | c;
		}
		switch (type) {
			default:
			case	TYPE_B:
				*(((_UB*)val_adr)++) = val;
				break;
			case	TYPE_H:
				*(((_UH*)val_adr)++) = val;
				break;
			case	TYPE_W:
				*(((_UW*)val_adr)++) = val;
				break;
		}
	}
}


static	void	cmd_w(void)
{
	W	i, c, type, pos, len;
	UW	val;
	
	pos = 1;
	type = TYPE_B;
	switch (line_buffer[pos]) {
		case	'h':
		case	'H':
			type = TYPE_H;
			pos++;
			break;
		case	'w':
		case	'W':
			type = TYPE_W;
			pos++;
			break;
	}
	
	while ((line_buffer[pos])) {
		if ((c = hex2val(line_buffer[pos++])) < 0)
			continue;
		val_adr = c;
		val_lines = VAL_LINES_DEF;
		while ((c = hex2val(line_buffer[pos])) >= 0) {
			pos++;
			val_adr = (val_adr << 4) | c;
		}
		break;
	}
	switch (type) {
		default:
		case	TYPE_B:
			len = 2;
			break;
		case	TYPE_H:
			val_adr = (val_adr + 1) & 0xfffffffe;
			len = 4;
			break;
		case	TYPE_W:
			val_adr = (val_adr + 3) & 0xfffffffc;
			len = 8;
			break;
	}
	i = 0;
	while ((line_buffer[pos])) {
		if ((c = hex2val(line_buffer[pos++])) < 0)
			continue;
		val = c;
		for (i=1; i<len; i++) {
			if ((c = hex2val(line_buffer[pos])) < 0)
				break;
			pos++;
			val = (val << 4) | c;
		}
		putW(val_adr);
		putstring(" : ");
		switch (type) {
			default:
			case	TYPE_B:
				*(((_UB*)val_adr)++) = val;
				putB(val);
				break;
			case	TYPE_H:
				*(((_UH*)val_adr)++) = val;
				putH(val);
				break;
			case	TYPE_W:
				*(((_UW*)val_adr)++) = val;
				putW(val);
				break;
		}
		putstring("\n");
	}
	if ((i))
		return;
	for (;;) {
		putW(val_adr);
		putstring(" : ");
		if (getline() < 0)
			return;
		if (line_size <= 0) {
			switch (type) {
				default:
				case	TYPE_B:
					val_adr += sizeof(B);
					break;
				case	TYPE_H:
					val_adr += sizeof(H);
					break;
				case	TYPE_W:
					val_adr += sizeof(W);
					break;
			}
			continue;
		}
		val = 0;
		for (i=0; i<line_size; i++) {
			if ((c = hex2val(line_buffer[i])) < 0) {
				putstring("error\n");
				return;
			}
			val = (val << 4) | c;
		}
		switch (type) {
			default:
			case	TYPE_B:
				*(((_UB*)val_adr)++) = val;
				break;
			case	TYPE_H:
				*(((_UH*)val_adr)++) = val;
				break;
			case	TYPE_W:
				*(((_UW*)val_adr)++) = val;
				break;
		}
	}
}


static	UW	putregname(W regnum)
{
	
	if ((regnum >= 0)&&(regnum < 15)) {
		putchar('R', TMO_FEVR);
		putnum(regnum);
		return sp->r[regnum];
	}
	switch (regnum) {
		case	15:	/* SP */
			putstring("R15");
			return ((sp->cr0 & 0x80))? sp->cr3 : sp->cr2;
		case	0xc0:
			putstring("CR0");
			return sp->cr0;
		case	0xc1:
			putstring("CR1");
			return sp->cr1;
		case	0xc2:
			putstring("CR2");
			return sp->cr2;
		case	0xc3:
			putstring("CR3");
			return sp->cr3;
		case	0xc6:
			putstring("CR6");
			return sp->cr6;
	}
	return 0xffffffff;
}


static	void	cmd_r(void)
{
	W	i, c, pos, regnum, val;
	
	pos = 1;
	regnum = -1;
	while ((line_buffer[pos])) {
		if ((c = hex2val(line_buffer[pos++])) < 0)
			continue;
		regnum = c;
		while ((c = hex2val(line_buffer[pos])) >= 0) {
			pos++;
			regnum = (regnum << 4) | c;
		}
		if (regnum < 0)
			regnum = -1;
		else if (regnum < 0x10)
			;
		else if (regnum < 0x16)
			regnum -= 6;
		else if (regnum < 0xc0)
			regnum = -1;
		else if (regnum < 0xc4)
			;
		else if (regnum < 0xc6)
			regnum = -1;
		else if (regnum < 0xc7)
			;
		else
			regnum = -1;
		break;
	}
	
	if (regnum < 0) {
		for (i=0; i<16; i++) {
			val = putregname(i);
			putchar(':', TMO_FEVR);
			putW(val);
			putchar(' ', TMO_FEVR);
			if ((i & 3) == 3)
				putstring("\n");
		};
		val = putregname(0xc0);
		putchar(':', TMO_FEVR);
		putW(val);
		putstring(((val & 0x8000))? " BSM " : " --- ");
		putstring(((val & 0x4000))? "BIE " : "--- ");
		putstring(((val & 0x100))? "BC " : "-- ");
		putstring(((val & 0x80))? "SM " : "-- ");
		putstring(((val & 0x40))? "IE " : "-- ");
		putstring(((val & 0x1))? "C\n" : "-\n");
		
		val = putregname(0xc1);
		putchar(':', TMO_FEVR);
		putW(val);
		putchar(' ', TMO_FEVR);
		val = putregname(0xc2);
		putchar(':', TMO_FEVR);
		putW(val);
		putchar(' ', TMO_FEVR);
		putchar(':', TMO_FEVR);
		val = putregname(0xc3);
		putchar(':', TMO_FEVR);
		putW(val);
		putchar(' ', TMO_FEVR);
		val = putregname(0xc6);
		putchar(':', TMO_FEVR);
		putW(val);
		putstring("\n");
		return;
	}
	
	val = putregname(regnum);
	putstring(" : ");
	putW(val);
	putstring(" -> ");
	
	if (regnum == 15)
		regnum = ((sp->cr0 & 0x80))? 0xc3 : 0xc2;
	
	while ((line_buffer[pos])) {
		if ((c = hex2val(line_buffer[pos++])) < 0)
			continue;
		val = c;
		while ((c = hex2val(line_buffer[pos])) >= 0) {
			pos++;
			val = (val << 4) | c;
		}
		putW(val);
		putstring("\n");
		if (regnum < 0)
			;
		else if (regnum < 15)
			sp->r[regnum] = val;
		else
			switch (regnum) {
				case	0xc0:
					sp->cr0 = val;
					break;
				case	0xc1:
					sp->cr1 = val;
					break;
				case	0xc2:
					sp->cr2 = val;
					break;
				case	0xc3:
					sp->cr3 = val;
					break;
				case	0xc6:
					sp->cr6 = val;
					break;
			}
		return;
	}
	if (getline() <= 0)
		return;
	if (line_size <= 0)
		return;
	val = 0;
	for (i=0; i<line_size; i++) {
		if ((c = hex2val(line_buffer[i])) < 0) {
			putstring("error\n");
			return;
		}
		val = (val << 4) | c;
	}
	
	if ((regnum >= 0)&&(regnum < 15))
		sp->r[regnum] = val;
	else
		switch (regnum) {
			case	0xc0:
				sp->cr0 = val;
				break;
			case	0xc1:
				sp->cr1 = val;
				break;
			case	0xc2:
				sp->cr2 = val;
				break;
			case	0xc3:
				sp->cr3 = val;
				break;
			case	0xc6:
				sp->cr6 = val;
				break;
		}
	return;
}

static	void	cmd_g(void)
{
	W	c, pos, val;
	
	pos = 1;
	while ((line_buffer[pos])) {
		if ((c = hex2val(line_buffer[pos++])) < 0)
			continue;
		val = c;
		while ((c = hex2val(line_buffer[pos])) >= 0) {
			pos++;
			val = (val << 4) | c;
		}
		sp->r[14] = val;	/* update PC */
	}
	return;
}

static	void	cmd_s()
{
	W	pos, len, addr;
	UB	sum;
	
	if (line_size < 4) {
		putstring("-- no data --\n");
		return;
	}
	switch (line_buffer[1]) {
		case	'0':
			putstring("-- START --\n");
			return;
		case	'8':
			putstring("-- END --\n");
			return;
		case	'2':
			break;
		default:
			putstring("-- unknown format --\n");
			return;
	}
	len = (hex2val(line_buffer[2]) << 4) | hex2val(line_buffer[3]);
	if ((len < 4)||(len > 80)||(len * 2 + 4 != line_size)) {
		putstring("-- LEN error --\n");
		return;
	}
	
	sum = 0;
	for (pos=-1; pos<len; pos++)
		sum += (hex2val(line_buffer[pos * 2 + 4]) << 4) | hex2val(line_buffer[pos * 2 + 5]);
	
	if (sum != 0xff) {
		putstring("-- SUM error --\n");
		return;
	}
	
	addr = hex2val(line_buffer[4]) << 20;
	addr |= hex2val(line_buffer[5]) << 16;
	addr |= hex2val(line_buffer[6]) << 12;
	addr |= hex2val(line_buffer[7]) << 8;
	addr |= hex2val(line_buffer[8]) << 4;
	addr |= hex2val(line_buffer[9]);
	
	for (pos=3; pos<len-1; pos++)
		*((_UB*)addr++) = (hex2val(line_buffer[pos * 2 + 4]) << 4) | hex2val(line_buffer[pos * 2 + 5]);
	
	MCCR[2] = 1;
	
	return;
}

void	cli(void)
{
	for (;;) {
		putstring("MON>");
		if (getline() <= 0)
			continue;
		
		switch (line_buffer[0]) {
			case	'd':
			case	'D':
				cmd_d();
				continue;
			case	'm':
			case	'M':
				cmd_m();
				continue;
			case	'w':
			case	'W':
				cmd_w();
				continue;
			case	'r':
			case	'R':
				cmd_r();
				continue;
			case	'g':
			case	'G':
				cmd_g();
				return;
			case	'S':
				cmd_s();
				continue;
			case	'h':
			case	'H':
				break;
			default:
				putstring("invalid command. \'H\' for help.\n");
				continue;
		}
			/* help message */
		putstring("D|DH|DW [<addr> [<lines>]] ---- dump memory\n");
		putstring("M|MH|MW [<addr> [<data>..]] ---- modify memory\n");
		putstring("W|WH|WW [<addr> [<data>..]] ---- write memory (not read)\n");
		putstring("R [<reg> [<data>]] ---- dump / modify register\n");
		putstring("G [<addr>] ---- jump user program\n");
		putstring("(S ---- for S-format download)\n");
	}
}

