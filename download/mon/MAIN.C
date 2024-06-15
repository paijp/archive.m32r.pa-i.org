
#include	"mon.h"
#include	<hw.h>

void	configuration(void)
{
	main();
}


int	main(void)
{
	DI();
	init_serial();
	
	if (((*P7DATA) & 0x10) == 0) {
		static	void	(*func)() = (void*)0x00050000;
		
		func();
	}
	
	putstring("\n\n");
	putstring("*******************************\n");
	putstring("* M32102 monitor R1.00        *\n");
	putstring("*******************************\n\n");
	
	trapent();
	return 0;
}


