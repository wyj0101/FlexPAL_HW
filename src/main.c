#include <stdio.h>

#include <zephyr/kernel.h>

int main(void)
{
	while(1)
	{
		printf("welcome to zephyr\n");
		k_sleep(K_SECONDS(1));
	}

	return 0;
}