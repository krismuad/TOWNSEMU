#include "cpputil.h"
#include "keyboard.h"
#include "townsdef.h"



void TownsKeyboard::State::Reset(void)
{
}
TownsKeyboard::TownsKeyboard()
{
	state.Reset();
	afterReset=SEND_CD_AFTER_RESET;
	nFifoFilled=0;
}

void TownsKeyboard::PushFifo(unsigned char code1,unsigned char code2)
{
	if(nFifoFilled+1<FIFO_BUF_LEN)
	{
		fifoBuf[nFifoFilled  ]=code1;
		fifoBuf[nFifoFilled+1]=code2;
		nFifoFilled+=2;
	}
}

/* virtual */ void TownsKeyboard::IOWriteByte(unsigned int ioport,unsigned int data)
{
	switch(ioport)
	{
	case TOWNSIO_KEYBOARD_DATA://       0x600, // [2] pp.234
		break;
	case TOWNSIO_KEYBOARD_STATUS_CMD:// 0x602, // [2] pp.231
		if(0xA1==data) // RESET pp.232
		{
			state.Reset();
			PushFifo(0xA0,0x7F);
			// pp.235 and 
			// Also based on Reverse Engineering of FM Towns IIMX System ROM
			switch(afterReset)
			{
			case SEND_CD_AFTER_RESET:
				PushFifo(0xA0,0x2C);
				PushFifo(0xA0,0x20);
				PushFifo(0xA0,0x2C);
				PushFifo(0xA0,0x20);
				PushFifo(0xA0,0x2C);
				PushFifo(0xA0,0x20);
				break;
			case SEND_F0_AFTER_RESET:
				PushFifo(0xA0,0x21);
				PushFifo(0xA0,0x0B);
				PushFifo(0xA0,0x21);
				PushFifo(0xA0,0x0B);
				PushFifo(0xA0,0x21);
				PushFifo(0xA0,0x0B);
				break;
			case SEND_F1_AFTER_RESET:
				PushFifo(0xA0,0x21);
				PushFifo(0xA0,0x02);
				PushFifo(0xA0,0x21);
				PushFifo(0xA0,0x02);
				PushFifo(0xA0,0x21);
				PushFifo(0xA0,0x02);
				break;
			}
		}
		break;
	case TOWNSIO_KEYBOARD_IRQ://        0x604, // [2] pp.236
		break;
	}
}
/* virtual */ unsigned int TownsKeyboard::IOReadByte(unsigned int ioport)
{
	switch(ioport)
	{
	case TOWNSIO_KEYBOARD_DATA://       0x600, // [2] pp.234
		if(0<nFifoFilled)
		{
			auto ret=fifoBuf[0];
			for(int i=0; i<nFifoFilled-1; ++i)
			{
				fifoBuf[i]=fifoBuf[i+1];
			}
			--nFifoFilled;
			return ret;
		}
		break;
	case TOWNSIO_KEYBOARD_STATUS_CMD:// 0x602, // [2] pp.231
		if(0<nFifoFilled)
		{
			return 1; // Data (Keyboard -> CPU) Ready.
		}
		return 0;  // IBF=0 Data Empty. Always Ready.
	case TOWNSIO_KEYBOARD_IRQ://        0x604, // [2] pp.236
		return 0; // NO IRQ Implemented yet.
	}
	return 0xff;
}
/* virtual */ void TownsKeyboard::Reset(void)
{
	state.Reset();
}