
#define	TARGET	"block"
#include	"cdj.h"
#include	<hw.h>


struct	info_block_struct	info_block[MAXBLOCK];

#define	FF_MUL	3
#define	POS_TARGET	((MAXBUFFER - 1) / 2)


void	init_locate(struct info_locateunit_struct *l, W addr, W offset)
{
	W	i;
	
	l->status |= STATUS_WAIT;
	l->addr = addr;
	for (i=0; i<MAXBUFFER; i++)
		l->block[i] = -1;
	l->pos = 0;
	l->offset = offset;
	
	return;
}


void	init_drive(W drive)
{
	W	i;
	
	for (i=0; i<MAXBLOCK; i++) {
		if ((info_block[i].addr >> 28) == drive)
			info_block[i].addr = -1;
	}
	for (i=0; i<MAXWHEEL; i++) {
		struct	info_wheel_struct	*w;
		struct	info_locateunit_struct	*l;
		
		w = info_wheel + i;
		w->locate[drive].track = 0;
		if (w->drive == drive) {
			w->track_tmr = 1;
			w->mode = (w->mode | MODE_PAUSE) & (~(MODE_LOOP|MODE_CUE));
		}
		l = w->locate[drive].unit;
		init_locate(l + 0, drive << 28, 0);
		init_locate(l + 1, drive << 28, 0);
		init_locate(l + 2, drive << 28, 0);
		init_locate(l + 3, drive << 28, 0);
	}
	
	return;
}


void	block_proc10ms(void)
{
	static	UB	blockflg[MAXBLOCK];
	struct	info_wheel_struct	*w;
	struct	info_locateunit_struct	*l;
	W	i, j, k;
	W	block, wheel;
	
	for (i=0; i<MAXBLOCK; i++)
		blockflg[i] = 0;
	for (wheel=0; wheel<MAXWHEEL; wheel++) {
		w = info_wheel + wheel;
		for (i=0; i<MAXLOCATEUNIT; i++) {
			W	top, addr;
			
			l = w->locate[w->drive].unit + i;
			addr = l->addr;
			
			top = (addr & 0x0fffffff) + (l->pos << l->mul);
			
			{
				W	mul;
				
				{
					W	mode;
					
					mode = (i > 0)? 0 : (w->mode & (MODE_FF|MODE_FR));
					mul = (mode & (MODE_FF|MODE_FR))? FF_MUL : 0;
				}
				
				if (l->mul != mul) {
					DI();
#if 0
					l->status |= STATUS_WAIT;
#endif
					for (k=0; k<MAXBUFFER; k++)
						l->block[k] = -1;
					EI();
				}
				top -= POS_TARGET << mul;
				{
					W	max;
					
					max = info_drive[w->drive].maxaddr - (MAXBUFFER << mul);
					if (max <= 0)
						continue;
					if (top > max)
						top = max;
				}
				if (top < 0)
					top = 0;
				top |= addr & 0x70000000;
				if (l->mul != mul) {
					l->addr = addr = top;
					l->pos = POS_TARGET;
					l->mul = mul;
				}
			}
			
			DI();
			{
				W	*p;
				
				p = l->block;
				j = (addr - top) >> l->mul;
				if (j >= MAXBUFFER) {
					for (k=0; k<MAXBUFFER; k++)
						p[k] = -1;
					l->pos += j;
					l->addr = addr -= j << l->mul;
D4STR("[");
				} else if (j >= 1) {
					k = MAXBUFFER - 1;
					while (k >= j) {
						if ((block = p[k] = p[k - j]) >= 0)
							blockflg[block] = 1;
						k--;
					}
					while (k >= 0)
						p[k--] = -1;
					
					l->pos += j;
					l->addr = addr -= j << l->mul;
D4STR("<");
				} else if (j >= -1) {
					for (k=0; k<MAXBUFFER; k++)
						if ((block = p[k]) >= 0)
							blockflg[block] = 1;
				} else if (j >= -MAXBUFFER) {
					j = -1 - j;
					k = 0;
					while (k < MAXBUFFER - j) {
						if ((block = p[k] = p[k + j]) >= 0)
							blockflg[block] = 1;
						k++;
					}
					while (k < MAXBUFFER)
						p[k++] = -1;
					
					l->pos -= j;
					l->addr = addr += j << l->mul;
D4STR(">");
				} else {
					for (k=0; k<MAXBUFFER; k++)
						p[k] = -1;
					l->pos += j;
					l->addr = addr -= j << l->mul;
D4STR("]");
				}
			}
			if (l->pos < 0) {
				l->pos = 0;
				l->offset = 0;
			} else if (l->pos >= MAXBUFFER) {
				l->pos = MAXBUFFER - 1;
				l->offset = ((588 * DABUF_UNIT) << 16) - 1;
			}
			EI();
		}
	}
	for (i=0; i<MAXLOCATEUNIT; i++) {
		for (wheel=0; wheel<MAXWHEEL; wheel++) {
			W	comp, addr;
			
			w = info_wheel + (wheel ^ ((info_wheel[0].cf > info_wheel[1].cf)? 0 : 1));
			if (info_drive[w->drive].req_block >= 0)
				continue;
			if (info_drive[w->drive].maxtrack <= 0)
				continue;
			l = w->locate[w->drive].unit + i;
			
			comp = 1;
			for (j=0; j<MAXBUFFER; j++) {
				addr = l->addr + (j << l->mul);
				
				if ((block = l->block[j]) >= 0) {
					if ((info_block[block].type & TYPE_REQ))
						comp = 0;
					continue;
				}
				
				block = -1;
				for (k=0; k<MAXBLOCK; k++) {
					if (info_block[k].addr == addr) {
						if ((info_block[k].type & TYPE_REQ))
							comp = 0;
						blockflg[k] = 1;
						l->block[j] = k;
						block = -1;
						break;
					}
					if ((info_block[k].type & TYPE_REQ))
						;
					else if (blockflg[k] == 0)
						block = k;
				}
				if (block < 0)
					continue;
				
				comp = 0;
#if 0
D1W("req block", block);
D1W("req addr", addr);
#endif
				DI();
				info_block[block].type = TYPE_REQ;
				info_block[block].addr = addr;
				info_drive[w->drive].req_block = block;
				l->block[j] = block;
				EI();
				wup_tsk(w->drive + 1);
				blockflg[block] = 1;
				
				break;
			}
			
			if ((comp)) {
#if 0
if ((l->status & STATUS_WAIT)) {
D1W("comp locate", i);
}
#endif
				l->status &= (~STATUS_WAIT);
			}
		}
	}
	return;
}


void	setup_block(void)
{
	W	i;
	
	for (i=0; i<MAXBLOCK; i++) {
		info_block[i].addr = -1;
		info_block[i].type = 0;
		info_block[i].ptr = (void*)(0x20000 * i);
	}
	return;
}




