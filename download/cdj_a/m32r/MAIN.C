
#define	TARGET	"main"
#include	"cdj.h"
#include	<hw.h>


int	main(void)
{
	D2STR("CDJ boot.");
	
	*P6DATA = 0xff;		/* disable LAN */
	
	setup_sdram();
	setup_int();
	
	D4STR("setup_itron:");
	setup_itron();
	
	D4STR("setup_ata:");
	setup_ata();
	
	D4STR("setup_block:");
	setup_block();
	
	D4STR("setup_hmi:");
	setup_hmi();
	
	D4STR("setup_da:");
	setup_da();
	
	D4STR("playcd_ata:");
	idletask();
}

