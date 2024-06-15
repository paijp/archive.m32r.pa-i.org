
#define	TARGET	"hmi"
#include	"cdj.h"
#include	<hw.h>

#define	TRACK_TIMER	80	/* x10ms */

struct	info_wheel_struct	info_wheel[MAXWHEEL];
W	ataerr[MAXWHEEL] = {0, 0};

static	UW	hmi_cmdbuf = 0;
static	W	mon_drive = 2;

static	W	dump_screen = 1;
#define	SCREENSIZE	24
#define	LEDCMDSIZE	12
#define	CHAR_EOO	0x1f
static	UB	screen_buf0[] = {
	0xc, '-', '-', '/', '-', '-', ' ', '+', '0', '0', '.', '0', 
		' ', ' ', 
			'-', '-', '/', '-', '-', ' ', '+', '0', '0', '.', '0', 
	CHAR_EOO
};
static	UB	screen_buf1[] = {
	0xa, ' ', '-', '0', '0', ':', '0', '0', '.', '0', '0', ' ', 
    		' ', ' ', 
			' ', '-', '0', '0', ':', '0', '0', '.', '0', '0', ' ', 
	0x9, 
		'0', '0', '0', '0', '0', '0', '1', '0', 
		'0', '0', '2', '0', '0', '0', '3', '0', 
		'0', '0', '4', '0', '0', '0', '5', '0', 
	CHAR_EOO
};
static	UB	*screen_ptr = screen_buf0;
static	W	screen_phase = 0;


	/* cmd 0:play 1:cue-press 2:cue-release */
static	void	cmd_play(W wheel, W cmd)
{
	struct	info_wheel_struct	*w;
	struct	info_locateunit_struct	*l;
	
	w = info_wheel + wheel;
	l = w->locate[w->drive].unit;
	
	if (info_drive[w->drive].maxtrack <= 0) {
		switch (cmd) {
			case	0:
				w->mode ^= MODE_PAUSE;
				break;
			case	1:
				w->mode |= MODE_PAUSE;
				break;
			default:
				return;
		}
		info_drive[w->drive].req_eject = 3;	/* load */
		wup_tsk(w->drive + 1);
		return;
	}
	
	switch (cmd) {
		case	0:
			if ((w->mode & MODE_CUE))
				;
			else if ((w->mode & MODE_PAUSE) == 0) {
				w->mode |= MODE_PAUSE;
				if ((w->mode & MODE_CDJ))
					w->mode |= MODE_MON;
				D2STR("pause");
				break;
			} else if ((w->mode & MODE_CDJ) == 0) {
				DI();
				l[1] = l[0];	/* cue */
				EI();
			}
			if ((w->mode & MODE_CDJ))
				w->mode &= (~MODE_MON);
			w->mode &= (~(MODE_PAUSE|MODE_CUE));
			D2STR("play");
			break;
		case	1:
			if ((w->mode & MODE_PAUSE) == 0) {
				DI();
				l[0] = l[1];
				EI();
				w->mode = (w->mode | MODE_PAUSE) & (~(MODE_CUE|MODE_LOOP|MODE_CUE));
				D2STR("cue return");
				break;
			}
			if ((w->mode & (MODE_MON|MODE_CDJ)) == (MODE_MON|MODE_CDJ)) {
				w->mode = (w->mode | MODE_PAUSE) & (~(MODE_MON|MODE_LOOP|MODE_CUE));
				DI();
				l[1] = l[0];	/* cue */
				EI();
				D2STR("cue set");
				break;
			}
			DI();
			l[0] = l[1];
			EI();
			w->mode = (w->mode | MODE_CUE) & (~(MODE_PAUSE|MODE_LOOP));
			D2STR("cue play");
			break;
		case	2:
			if ((w->mode & MODE_PAUSE))
				break;
			if ((w->mode & MODE_CUE) == 0)
				break;
			w->mode = (w->mode | MODE_PAUSE) & (~(MODE_LOOP|MODE_CUE));
			DI();
			l[0] = l[1];
			EI();
			break;
	}
	return;
}


static	void	cmd_cdj(W wheel, W cmd)
{
	struct	info_wheel_struct	*w;
	
	w = info_wheel + wheel;
	
	if ((cmd & 1)) {
		if ((w->mode & MODE_CDJ))
			return;
		w->mode |= MODE_CDJ;
		if ((w->mode & MODE_PAUSE))
			w->mode |= MODE_MON;
	} else if ((cmd & 2)) {
		w->mode &= (~MODE_CDJ);
		w->mode |= MODE_MON;
	} else {
		w->mode &= (~MODE_CDJ);
		w->mode &= (~MODE_MON);
	}
	return;
}


	/* 0:START 1:END 2:LOOP 3:START-REL 4:DSTART 5:DSTART-REL */
static	void	cmd_loop(W wheel, W cmd)
{
	struct	info_wheel_struct	*w;
	struct	info_locateunit_struct	*l;
	
	w = info_wheel + wheel;
	l = w->locate[w->drive].unit;
	
	if (info_drive[w->drive].maxtrack <= 0)
		return;
	
	switch (cmd) {
		case	4:
			w->mode |= MODE_DSTART;
			if (0) {
		case	5:
				w->mode &= (~MODE_DSTART);
			}
			if ((w->mode & MODE_START))
				return;
			if (0) {
		case	0:
				w->mode |= MODE_START;
				if ((w->mode & MODE_DSTART))
					return;
			}
			if ((w->mode & MODE_LOOP)) {
				DI();
				l[0] = l[2];
				EI();
			} else {
				DI();
				l[2] = l[0];
				EI();
				if (w->pitch_step < 0)
					init_locate(w->locate[w->drive].unit + 3, 0, 0);
				else
					init_locate(w->locate[w->drive].unit + 3, (w->drive << 28)|(info_drive[w->drive].maxaddr - 1), ((588 * DABUF_UNIT) << 16) - 1);
				w->mode |= MODE_LOOP;
			}
			D2STR("loop in");
			if ((w->mode & MODE_PAUSE))
				cmd_play(wheel, 0);
			break;
		case	1:
			DI();
			l[3] = l[0];
			l[0] = l[2];
			EI();
			w->mode |= MODE_LOOP;
			D2STR("loop out");
			if ((w->mode & MODE_PAUSE))
				cmd_play(wheel, 0);
			break;
		case	2:
			if (((w->mode ^= MODE_LOOP) & MODE_LOOP)) {
				DI();
				l[0] = l[2];
				EI();
				D2STR("reloop");
			} else {
				D2STR("exit loop");
			}
			if ((w->mode & MODE_PAUSE))
				cmd_play(wheel, 0);
			break;
		case	3:
			w->mode &= (~MODE_START);
			return;
	}
	return;
}


	/* cmd 0:back 1:next 2:release 3:repeat */
static	void	cmd_track(W wheel, W cmd)
{
	struct	info_wheel_struct	*w;
	struct	info_drive_struct	*d;
	W	c;
	
	w = info_wheel + wheel;
	d = info_drive + w->drive;
	
	switch (cmd) {
		case	2:
		default:
			w->track_repeat = 0;
			return;
		case	3:
			if (w->track_repeat == 0) {
				if (d->maxtrack <= 0)
					w->track_tmr = 4;
				return;
			}
			break;
		case	0:
			if (w->track_tmr == 0)
				w->track_repeat = 0;
			else
				w->track_repeat = -1;
			break;
		case	1:
			w->track_repeat = 1;
			break;
	}
	c = w->locate[w->drive].track + w->track_repeat;
	if (d->maxtrack <= 0) {
		if (c < -9)
			c = -9;
		else if (c > 98)
			c = 98;
	} else {
		if (c < 0)
			c = d->maxtrack - 1;
		else if (c >= d->maxtrack)
			c = 0;
	}
	if (w->track_repeat == 0)
		w->track_repeat = -1;
	w->track_tmr = (cmd == 3)? (TRACK_TIMER / 4) : TRACK_TIMER;
	
	w->mode &= (~MODE_LOOP);
	w->locate[w->drive].track = c;
	if (d->maxtrack > 0)
		init_locate(w->locate[w->drive].unit, (w->drive << 28) | d->track[c].addr, d->track[c].offset);
	return;
}


	/* cmd = MODE_FF || MODE_FR */
static	void	cmd_ff(W wheel, W cmd)
{
	info_wheel[wheel].mode = (info_wheel[wheel].mode & (~(MODE_FF|MODE_FR))) | cmd;
	return;
}


static	void	cmd_eject(W drive, W cmd)
{
	struct	info_drive_struct	*d;
	
	d = info_drive + drive;
	DI();
	if ((cmd)) {
		if (d->eject_tmr <= 0)
			d->eject_tmr = 1;
	} else {
		if (d->eject_tmr > 0)
			d->eject_tmr = 0;
	}
	EI();
	return;
}


static	void	cmd_rev(W wheel, W cmd)
{
	info_wheel[wheel].mode = (info_wheel[wheel].mode & (~(MODE_REV|MODE_OPITCH))) | cmd;
	return;
}


static	void	cmd_mon_dsel(W wheel, W cmd)
{
	struct	info_wheel_struct	*w;
	
	w = info_wheel + wheel;
	if ((cmd & 1))
		mon_drive = wheel;
	else if (mon_drive == wheel)
		mon_drive = 2;
	if (w->drive != (wheel ^ ((cmd & 2)? 1 : 0))) {
		W	i;
		
		DI();
		w->drive = wheel ^ ((cmd & 2)? 1 : 0);
		for (i=0; i<MAXLOCATEUNIT; i++) {
			struct	info_locateunit_struct	*l;
			
			l = w->locate[w->drive].unit + i;
			init_locate(l, l->addr + (l->pos << l->mul), l->offset);
		}
		EI();
		if (info_drive[w->drive].maxtrack <= 0)
			w->track_tmr = 1;
	}
	return;
}


static	void	cmd_copy(W wheel, W cmd)
{
	W	drive;
	
	(void)cmd;
	if ((drive = info_wheel[0].drive) != info_wheel[1].drive)
		return;
	DI();
	info_wheel[wheel].mode &= (~MODE_LOOP);
	info_wheel[wheel].locate[drive] = info_wheel[wheel ^ 1].locate[drive];
	EI();
}


static	void	cmd_null(W val1, W val2)
{
	(void)(val1);
	(void)(val2);
	return;
}


static	void	vol_mvol(W wheel, W val)
{
#define	SLACK	4
	W	i;
	
	i = (val & 0xfff) >> 3;
	
	if (info_wheel[wheel].volume > i)
		info_wheel[wheel].volume = i;
	else if (info_wheel[wheel].volume < i - SLACK)
		info_wheel[wheel].volume = i - SLACK;
	return;
#undef	SLACK
}


static	void	vol_upitch(W wheel, W val)
{
#define	SLACK	15
	W	i, j;
	
	i = (((val & 0xfff) * 5) >> 4) - 640;
	if (i > SLACK - 6) {
		i -= SLACK - 6;
		j = i - SLACK;
		if (j < 0)
			j = 0;
		if (info_wheel[wheel].upitch > i)
			info_wheel[wheel].upitch = i;
		else if (info_wheel[wheel].upitch < j)
			info_wheel[wheel].upitch = j;
	} else if (i < -(SLACK - 5)) {
		i += (SLACK - 5);
		j = i + SLACK;
		if (j > 0)
			j = 0;
		if (info_wheel[wheel].upitch > j)
			info_wheel[wheel].upitch = j;
		else if (info_wheel[wheel].upitch < i)
			info_wheel[wheel].upitch = i;
	} else
		info_wheel[wheel].upitch = 0;
	return;
#undef	SLACK
}


static	void	vol_pitch(W wheel, W val)
{
#define	SLACK	3
	W	i, j;
	
	i = ((val >> 4) & 0xff) - 0x80;
	if (i > SLACK - 2) {
		i -= (SLACK - 2);
		j = i - SLACK;
		if (j < 0)
			j = 0;
		if (info_wheel[wheel].pitch > i)
			info_wheel[wheel].pitch = i;
		else if (info_wheel[wheel].pitch < j)
			info_wheel[wheel].pitch = j;
	} else if (i < -(SLACK - 1)) {
		i += (SLACK - 1);
		j = i + SLACK;
		if (j > 0)
			j = 0;
		if (info_wheel[wheel].pitch > j)
			info_wheel[wheel].pitch = j;
		else if (info_wheel[wheel].pitch < i)
			info_wheel[wheel].pitch = i;
	} else
		info_wheel[wheel].pitch = 0;
	return;
#undef	SLACK
}


static	void	vol_cf(W wheel, W val)
{
#define	SLACK	4
	W	i, j;
	
	(void)wheel;
	i = (val & 0xff) * 2;
	if (i < 256)
		j = 255;
	else {
		j = 510 - i;
		i = 255;
	}
	
	if (info_wheel[0].cf > i)
		info_wheel[0].cf = i;
	else if (info_wheel[0].cf < i - SLACK)
		info_wheel[0].cf = i - SLACK;
	if (info_wheel[1].cf > j)
		info_wheel[1].cf = j;
	else if (info_wheel[1].cf < j - SLACK)
		info_wheel[1].cf = j - SLACK;
	return;
#undef	SLACK
}


static	void	vol_wheel(W wheel, W val)
{
#define	MAXVEL	(0x40 * 10 * 4)
#define	SLACK	(0x40)
	static	UH	wheel_pos[2][24] = {{0}};
	struct	info_wheel_struct	*w;
	W	c;
	
	val = 0xfff - val;
	
	w = info_wheel + wheel;
	if ((wheel)) {
		for (c=2; c<22; c++)
			wheel_pos[screen_phase ^ 1][c] = wheel_pos[screen_phase][c + 2] - wheel_pos[screen_phase][(c & 1) + 2];
		screen_phase ^= 1;
	}
	
	do {
		if ((w->wh_first)) {
			w->wh_base = val;
			w->wh_first = 0;
			c = 0;
			break;
		}
		c = (w->wh_base - val) & 0x0fff;
		if ((c & 0x0800))
			c -= 0x1000;
		if (c > SLACK)
			c -= SLACK;
		else if (c > -SLACK)
			c = 0;
		else
			c += SLACK;
		
		c -= w->wh_vel;
		if ((c & 0x80000001) == 1)
			c++;
		w->wh_vel += c >> 1;
		if (w->wh_vel > MAXVEL)
			w->wh_vel = MAXVEL;
		else if (w->wh_vel < -MAXVEL)
			w->wh_vel = -MAXVEL;
		
		w->wh_base -= (c = w->wh_vel);
	} while (0);
	wheel_pos[screen_phase][22 + wheel] = wheel_pos[screen_phase][20 + wheel] + c;
	if (wheel == 0)
		next_wheel_pos = wheel_pos[screen_phase];
	return;
#undef	SLACK
#undef	MAXVEL
}


void	prockey(W stacd)
{
	(void)stacd;
	for (;;) {
		slp_tsk();
		if (hmi_cmdbuf < 0xf000) {
			static	struct	{
				void	(*func)(W, W);
				W	val;
			} *p, func_ary[0xf] = {
				{cmd_null, 0}, 
				{vol_mvol, 1}, 
				{vol_upitch, 1}, 
				{vol_pitch, 1}, 
				
				{cmd_null, 0}, 
				{vol_mvol, 0}, 
				{vol_upitch, 0}, 
				{vol_pitch, 0}, 
				
				{cmd_null, 0}, 
				{cmd_null, 0}, 
				{cmd_null, 0}, 
				{cmd_null, 0}, 
				
				{vol_wheel, 1}, 
				{vol_wheel, 0}, 
				{vol_cf, 0}
			};
			p = func_ary + ((hmi_cmdbuf >> 12) & 0xf);
			p->func(p->val, hmi_cmdbuf & 0xfff);
		} else {
			static	struct	{
				void	(*func)(W, W);
				H	val1, val2;
			} *p, func_ary[0x80] = {
				{cmd_loop, 0, 0},	/*00*/
				{cmd_loop, 0, 3}, 
				{cmd_loop, 0, 1}, 
				{cmd_null, 0, 0}, 
				{cmd_loop, 0, 2}, 
				{cmd_null, 0, 0}, 
				{cmd_loop, 0, 4}, 
				{cmd_loop, 0, 5}, 
				
				{cmd_mon_dsel, 1, 3}, 
				{cmd_mon_dsel, 1, 1}, 
				{cmd_mon_dsel, 1, 2}, 
				{cmd_mon_dsel, 1, 0}, 
				{cmd_null, 0, 0}, 
				{cmd_null, 0, 0}, 
				{cmd_null, 0, 0}, 
				{cmd_null, 0, 0}, 
				
				{cmd_track, 0, 0},	/*10*/
				{cmd_track, 0, 2}, 
				{cmd_ff, 0, MODE_FR}, 
				{cmd_ff, 0, 0}, 
				{cmd_play, 0, 1}, 
				{cmd_play, 0, 2}, 
				{cmd_copy, 0, 0}, 
				{cmd_null, 0, 0}, 
				
				{cmd_cdj, 0, 3}, 
				{cmd_cdj, 0, 2}, 
				{cmd_cdj, 0, 1}, 
				{cmd_cdj, 0, 0}, 
				{cmd_null, 0, 0}, 
				{cmd_null, 0, 0}, 
				{cmd_null, 0, 0}, 
				{cmd_null, 0, 0}, 
				
				{cmd_track, 0, 1},	/*20*/
				{cmd_track, 0, 2}, 
				{cmd_ff, 0, MODE_FF}, 
				{cmd_ff, 0, 0}, 
				{cmd_play, 0, 0}, 
				{cmd_null, 0, 0}, 
				{cmd_eject, 0, 1}, 
				{cmd_eject, 0, 0}, 
				
				{cmd_rev, 0, MODE_REV|MODE_OPITCH}, 
				{cmd_rev, 0, MODE_OPITCH}, 
				{cmd_rev, 0, MODE_REV}, 
				{cmd_rev, 0, 0}, 
				{cmd_null, 0, 0}, 
				{cmd_null, 0, 0}, 
				{cmd_null, 0, 0}, 
				{cmd_null, 0, 0}, 
				
				
				{cmd_loop, 1, 0},	/*30*/
				{cmd_loop, 1, 3}, 
				{cmd_loop, 1, 1}, 
				{cmd_null, 1, 0}, 
				{cmd_loop, 1, 2}, 
				{cmd_null, 1, 0}, 
				{cmd_loop, 1, 4}, 
				{cmd_loop, 1, 5}, 
				
				{cmd_mon_dsel, 0, 3}, 
				{cmd_mon_dsel, 0, 1}, 
				{cmd_mon_dsel, 0, 2}, 
				{cmd_mon_dsel, 0, 0}, 
				{cmd_null, 0, 0}, 
				{cmd_null, 0, 0}, 
				{cmd_null, 0, 0}, 
				{cmd_null, 0, 0}, 
				
				{cmd_track, 1, 0},	/*40*/
				{cmd_track, 1, 2}, 
				{cmd_ff, 1, MODE_FR}, 
				{cmd_ff, 1, 0}, 
				{cmd_play, 1, 1}, 
				{cmd_play, 1, 2}, 
				{cmd_copy, 1, 0}, 
				{cmd_null, 1, 0}, 
				
				{cmd_cdj, 1, 3}, 
				{cmd_cdj, 1, 2}, 
				{cmd_cdj, 1, 1}, 
				{cmd_cdj, 1, 0}, 
				{cmd_null, 0, 0}, 
				{cmd_null, 0, 0}, 
				{cmd_null, 0, 0}, 
				{cmd_null, 0, 0}, 
				
				{cmd_track, 1, 1},	/*50*/
				{cmd_track, 1, 2}, 
				{cmd_ff, 1, MODE_FF}, 
				{cmd_ff, 1, 0}, 
				{cmd_play, 1, 0}, 
				{cmd_null, 1, 0}, 
				{cmd_eject, 1, 1}, 
				{cmd_eject, 1, 0}, 
				
				{cmd_rev, 1, MODE_REV|MODE_OPITCH}, 
				{cmd_rev, 1, MODE_OPITCH}, 
				{cmd_rev, 1, MODE_REV}, 
				{cmd_rev, 1, 0}, 
				{cmd_null, 0, 0}, 
				{cmd_null, 0, 0}, 
				{cmd_null, 0, 0}, 
				{cmd_null, 0, 0}, 
				
				
				{cmd_null, 0, 0},	/*60*/
				{cmd_null, 0, 0}, 
				{cmd_null, 0, 0}, 
				{cmd_null, 0, 0}, 
				{cmd_null, 0, 0}, 
				{cmd_null, 0, 0}, 
				{cmd_null, 0, 0}, 
				{cmd_null, 0, 0}, 
				
				{cmd_null, 0, 0}, 
				{cmd_null, 0, 0}, 
				{cmd_null, 0, 0}, 
				{cmd_null, 0, 0}, 
				{cmd_null, 0, 0}, 
				{cmd_null, 0, 0}, 
				{cmd_null, 0, 0}, 
				{cmd_null, 0, 0}, 
				
				{cmd_null, 0, 0},	/*70*/
				{cmd_null, 0, 0}, 
				{cmd_null, 0, 0}, 
				{cmd_null, 0, 0}, 
				{cmd_null, 0, 0}, 
				{cmd_null, 0, 0}, 
				{cmd_null, 0, 0}, 
				{cmd_null, 0, 0}, 
				
				{cmd_null, 0, 0}, 
				{cmd_null, 0, 0}, 
				{cmd_null, 0, 0}, 
				{cmd_null, 0, 0}, 
				{cmd_null, 0, 0}, 
				{cmd_null, 0, 0}, 
				{cmd_null, 0, 0}, 
				{cmd_null, 0, 0}, 
			};
			
			p = func_ary + (hmi_cmdbuf & 0x7f);
			p->func(p->val1, p->val2);
		}
	}
}


static	void	checkkey(W c)
{
	static	UW	val = 0;
	
	if (c == '/') {
		dump_screen = 1;
		return;
	}
	if ((c >= '0')&&(c <= '9'))
		c -= '0';
	else if ((c >= 'A')&&(c <= 'F'))
		c -= 'A' - 0xa;
	else if ((c >= 'a')&&(c <= 'f'))
		c -= 'a' - 0xa;
	else
		return;
	val = (val << 4) | c;
	if ((dump_screen++) < 4)
		return;
	dump_screen = 1;
	hmi_cmdbuf = val & 0xffff;
	iwup_tsk(3);
	return;
}


static	void	wheel_proc10ms(void)
{
	W	i;
	
	for (i=0; i<MAXWHEEL; i++) {
		struct	info_locateunit_struct	*l;
		struct	info_wheel_struct	*w, *w0;
		W	block;
		
		w = info_wheel + i;
		l = w->locate[w->drive].unit;
		w0 = info_wheel + (i ^ ((w->mode & MODE_OPITCH)? 1 : 0));
		DI();
		w->pitch_step = (1024 + w0->pitch + w0->upitch) * 66;	/* 44100 * 65536 / 5330000 * 128 * w->pitch / 1024 */
		if ((w->mode & MODE_FR)) {
			if ((w->mode & MODE_REV) == 0)
				w->pitch_step = -w->pitch_step;
		} else if ((w->mode & MODE_REV))
			w->pitch_step = -w->pitch_step;
		EI();
		
		DI();
		if (w->track_tmr > 0) {
			if ((--w->track_tmr) == 0)
				cmd_track(i, 3);
		} else if ((l->status & STATUS_WAIT))
			;
		else if ((block = l->block[l->pos]) < 0)
			;
		else if ((info_block[block].type & TYPE_REQ))
			;
		else
			w->locate[w->drive].track = info_block[block].track;
		EI();
	}
	for (i=0; i<MAXDRIVE; i++) {
		struct	info_drive_struct	*d;
		
		d = info_drive + i;
		if (d->eject_tmr <= 0)
			continue;
		if ((d->req_eject))
			continue;
		DI();
		d->eject_tmr++;
		if (d->eject_tmr == 80) {
			d->req_eject = 1;
			wup_tsk(i + 1);
		}
		if (d->eject_tmr == 300) {
			d->req_eject = 4;
			wup_tsk(i + 1);
		}
		EI();
	}
	return;
}


static	W	lm2chr(W i)
{
	if (i <= 0)
		return ' ';
	else if ((i & 0xffffc000))
		return 7;
	else if ((i & 0x00002000))
		return (i & 0x00001000)? 7 : 6;
	else if ((i & 0x00001000))
		return (i & 0x00000800)? 5 : 4;
	else if ((i & 0x00000800))
		return (i & 0x00000400)? 3 : 2;
	else if ((i & 0x00000400))
		return (i & 0x00000200)? 1 : 0;
	return ' ';
}


static	void	update_screen0(void)
{
	UB	*p;
	W	i;
	
	p = screen_buf0 + 1;
	for (i=0; i<MAXWHEEL; i++) {
		struct	info_drive_struct	*d;
		struct	info_wheel_struct	*w;
		W	val;
		
		w = info_wheel + i;
		d = info_drive + w->drive;
		
		if ((val = w->locate[w->drive].track + 1) > 0) {
			*(p++) = '0' + (val / 10);
			*(p++) = '0' + (val % 10);
		} else {
			*(p++) = '-';
			*(p++) = '1' - val;
		}
		p++;
		if (d->maxtrack > 0) {
			val = d->maxtrack;
			*(p++) = '0' + (val / 10);
			*(p++) = '0' + (val % 10);
		} else {
			*(p++) = '-';
			*(p++) = '-';
		}
		p++;
		val = (w->pitch + w->upitch) * 1000 / 1024;
		*(p++) = (val >= 0)? '+' : '-';
		if (val < 0)
			val = (-val);
		*(p++) = '0' + ((val / 100) % 10);
		*(p++) = '0' + ((val / 10) % 10);
		p++;
		*(p++) = '0' + (val % 10);
		
		p += 2;
	}
	p--;
	
	screen_ptr = screen_buf0;
	return;
}


static	void	update_screen1(void)
{
	static	UW	count = 0;
	UB	*p;
	W	i;
	
	count++;
	p = screen_buf1 + 1;
	for (i=0; i<MAXWHEEL; i++) {
		struct	info_drive_struct	*d;
		struct	info_wheel_struct	*w;
		W	val;
		
		w = info_wheel + i;
		d = info_drive + w->drive;
		
		p++;
		if ((d->ataerr)) {
			const	char	*num2hex = "0123456789abcdef";
			
			*(p++) = num2hex[(d->ataerr >> 24) & 0xf];
			*(p++) = num2hex[(d->ataerr >> 20) & 0xf];
			*(p++) = num2hex[(d->ataerr >> 16) & 0xf];
			p++;
			*(p++) = num2hex[(d->ataerr >> 12) & 0xf];
			*(p++) = num2hex[(d->ataerr >> 8) & 0xf];
			p++;
			*(p++) = num2hex[(d->ataerr >> 4) & 0xf];
			*(p++) = num2hex[d->ataerr & 0xf];
			p++;
			p += 2;
			continue;
		}
		if (d->maxtrack > 0) {
			struct	info_locateunit_struct	*l;
			
			l = w->locate[w->drive].unit;
			val = w->locate[w->drive].track + 1;
			val = (((l->addr & 0x0fffffff) + (l->pos << l->mul)) * 48 + l->offset / (588 * 0x10000)) - (d->track[val].addr * 48 + d->track[val].offset / (588 * 0x10000));
		} else
			val = 0;
		*(p++) = (val >= 0)? '+' : '-';
		if (val < 0)
			val = (-val);
		*(p++) = '0' + ((val / (60 * 75 * 10)) % 10);
		*(p++) = '0' + ((val / (60 * 75)) % 10);
		p++;
		*(p++) = '0' + ((val / 75) % 60) / 10;
		*(p++) = '0' + ((val / 75) % 10);
		p++;
		*(p++) = '0' + (val % 75) / 10;
		*(p++) = '0' + ((val % 75) % 10);
		
		p++;
		p += 2;
	}
	p--;
	
	for (i=0; i<MAXWHEEL; i++) {
		struct	info_wheel_struct	*w;
		W	val;
		
		w = info_wheel + i;
		val = (w->mode & MODE_LOOP)? 2 : 0;
		val |= (w->drive != i)? 4 : 0;
		p[3] = (count & 0x20)? ('0' + val) : '0';
		
		if (info_drive[w->drive].maxtrack <= 0) {
			p[7] = '0';
			p[0xb] = ((count & 0x20)&&((w->mode & MODE_PAUSE) == 0))? '1' : '0';
		} else if ((w->mode & MODE_PAUSE) == 0) {
			p[7] = '4';
			p[0xb] = '1';
		} else if ((w->mode & MODE_CDJ)) {
			p[7] = ((count & 0x10)&&(w->mode & MODE_MON))? '0' : '4';
			p[0xb] = (count & 0x20)? '1' : '0';
		} else {
			p[7] = '4';
			p[0xb] = '2';
		}
		
		if ((w->mode & MODE_REV))
			p[0xb] += 4;
		else if ((w->mode & MODE_OPITCH)&&(count & 0x20))
			p[0xb] += 4;
		if ((w->mode & MODE_CDJ))
			p[7]++;
		else if ((w->mode & MODE_MON)&&(count & 0x20))
			p[7] |= 3;
		p += LEDCMDSIZE;
	}
	
	screen_ptr = screen_buf1;
	return;
}


static	void	update_screen2(void)
{
	W	i;
	
	for (i=0; i<MAXWHEEL; i++) {
		struct	info_wheel_struct	*w;
		W	val;
		
		w = info_wheel + i;
		DI();
		val = (w->lm_max >> 3) - (w->lm_min >> 3);
		w->lm_max -= val;
		w->lm_min += val;
		EI();
		screen_buf0[12 + i] = lm2chr(val >> 6);
		screen_buf1[12 + i] = lm2chr(val >> 2);
	}
	return;
}


static	void	hmi_hdr(void)
{
	isig_tim();
	set_monitor(mon_drive);
	block_proc10ms();
	wheel_proc10ms();
	
	if (((*SIO0STS) & 0x00e0)) {
		/* dump_screen = 0; */
		*SIO0CR = 0x0300;		/* clr-status */
		*SIO0CR = 0x0003;		/* TX RX */
	}
	return;
}


static	void	rsrecv_hdr(void)
{
	checkkey(*SIO0RXB);
	
	return;
}


static	void	rssend_hdr(void)
{
	UB	c;
	static	W	first = 1;
	
	if (dump_screen == 0)
		;
	else if ((c = *(screen_ptr++)) == CHAR_EOO)
		first = 1;
	else {
		if (first == 0)
			;
		else if ((screen_phase)) {
			update_screen1();
		} else {
			update_screen2();
			update_screen0();
		}
		first = 0;
		*SIO0TXB = c;
		return;
	}
	*SIO0TRCR = 0x0004;			/* ~TXE-int RX-int */
	return;
}


void	setup_hmi(void)
{
	static	struct	info_wheel_struct	init_info_wheel = {MODE_PAUSE, 0, 0, 0, 0, 0, 0, {}, 1, 0, 0, 0, 255, 0, 0, 0, 1};
	static	struct	info_locateunit_struct	init_info_locateunit = {STATUS_WAIT, 0, {-1}, 0, 0, 0};
	W	i, j;
	
	init_info_locateunit.block[MAXBUFFER] = 0;
	for (i=0; i<MAXWHEEL; i++) {
		info_wheel[i] = init_info_wheel;
		info_wheel[i].cf = (i == 0)? 255 : 0;
		info_wheel[i].drive = i;
		for (j=0; j<MAXDRIVE; j++) {
			struct	info_locategroup_struct	*g;
			W	k;
			
			g = info_wheel[i].locate + j;
			g->track = 0;
			for (k=0; k<MAXLOCATEUNIT; k++) {
				g->unit[k] = init_info_locateunit;
				g->unit[k].addr = j << 28;
			}
		}
	}
	
	*SIO0CR = 0x0300;			/* clr-status */
	
	def_int(48, rsrecv_hdr);
	*ICUCR48 = 0x00001001;	/* SIO0RX_INT level=1 */
	def_int(49, rssend_hdr);
	*ICUCR49 = 0x00001002;	/* SIO0TX_INT level=2 */
	
	*SIO0MOD0 = 0x0000;			/* N*1 */
	*SIO0MOD1 = 0x0800;			/* 8bit CMOS LSB-first */
	*SIO0BAUR = 17;				/* 115200bps */
	*SIO0RBAUR = 2;
	*SIO0TRCR = 0x0004;			/* ~TXE-int RX-int */
	*SIO0CR = 0x0003;			/* TX RX */
	
	def_int(21, hmi_hdr);
	*ICUCR21 = 0x00001006;	/* MFT5_INT level=6 */
	
	*MFTCR = 0x00000400;	/* MFT5=stop */
	*MFT5MOD = 0x00008003;	/* OSC NEG 1/128 */
	*MFT5CUT = 0x00000a00;	/* count=2560(*128) : 10ms */
	*MFTCR = 0x00000404;	/* MFT5=start */
	
	return;
}



