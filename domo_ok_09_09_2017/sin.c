#include "math.h"
#include "stdio.h"

void main()
{
	int l;
	for(l=0;l<40;l++)
	{
		printf("%d,",(int)(sin(2*M_PI/40.f*l)*230*1.414));
	}
}