#include "egb.h"
#include "io.h"

static char EGB_work[EgbWorkSize];

void SetScreenMode(int m1,int m2);

int main(void)
{
	int scrnMode=10;

	EGB_init(EGB_work,EgbWorkSize);

	SetScreenMode(11,11);

	return 0;
}

void SetScreenMode(int m1,int m2)
{
	EGB_resolution(EGB_work,0,m1);
	EGB_resolution(EGB_work,1,m2);

	IOWriteByte(0x2077,m1);
	IOWriteByte(0x2077,m2);
}