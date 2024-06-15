
#define	TARGET	"stktest"
#include	"cdj.h"
#include	<hw.h>


#define	STACK_TEST_VAL	0x59fbeac9
W	stack_test_val;


void	stktest(void)
{
	static	UW	count = 0;
	
	if (count == 0)
		stack_test_val = STACK_TEST_VAL;
	if ((++count) == 0x01000000) {
		D1STR("running.");
		count = 1;
	}
	
	if (stack_test_val == STACK_TEST_VAL)
		return;
	D1W("stack overflow", (UW)(&stack_test_val));
	DI();
	for (;;)
		;
}

