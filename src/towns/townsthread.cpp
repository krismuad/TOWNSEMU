#include <iostream>
#include <chrono>

#include "townsthread.h"



TownsThread::TownsThread(void)
{
	runMode=RUNMODE_PAUSE;
}

void TownsThread::Start(FMTowns *townsPtr)
{
	bool terminate=false;
	this->townsPtr=townsPtr;
	for(;true!=terminate;)
	{
		vmLock.lock();
		switch(runMode)
		{
		case RUNMODE_PAUSE:
			break;
		case RUNMODE_FREE:
			townsPtr->cpu.DetachDebugger();
			for(unsigned int clocksPassed=0; 
			    clocksPassed<10000 && true!=townsPtr->CheckAbort();
			    )
			{
				clocksPassed+=townsPtr->RunOneInstruction();
				townsPtr->CheckRenderingTimer();
			}
			if(true==townsPtr->CheckAbort())
			{
				PrintStatus(*townsPtr);
				std::cout << ">";
				runMode=RUNMODE_PAUSE;
			}
			break;
		case RUNMODE_DEBUGGER:
			townsPtr->cpu.AttachDebugger(&townsPtr->debugger);
			for(unsigned int clocksPassed=0; 
			    clocksPassed<1000 && true!=townsPtr->CheckAbort();
			    )
			{
				clocksPassed+=townsPtr->RunOneInstruction();
				townsPtr->CheckRenderingTimer();
				if(true==townsPtr->debugger.stop)
				{
					PrintStatus(*townsPtr);
					std::cout << ">";
					runMode=RUNMODE_PAUSE;
					break;
				}
			}
			if(true==townsPtr->CheckAbort())
			{
				PrintStatus(*townsPtr);
				std::cout << ">";
				runMode=RUNMODE_PAUSE;
			}
			break;
		case RUNMODE_ONE_INSTRUCTION:
			townsPtr->cpu.AttachDebugger(&townsPtr->debugger);
			if(true!=townsPtr->CheckAbort())
			{
				townsPtr->RunOneInstruction();
				townsPtr->CheckRenderingTimer();
			}
			if(true==townsPtr->CheckAbort())
			{
				PrintStatus(*townsPtr);
				std::cout << ">";
				runMode=RUNMODE_PAUSE;
			}
			break;
		case RUNMODE_EXIT:
			terminate=true;
			break;
		default:
			terminate=true;
			std::cout << "Undefined VM RunMode!" << std::endl;
			break;
		}
		vmLock.unlock();
		if(RUNMODE_PAUSE==runMode)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}
}

int TownsThread::GetRunMode(void) const
{
	return runMode;
}
void TownsThread::SetRunMode(int nextRunMode)
{
	runMode=nextRunMode;
}

void TownsThread::PrintStatus(const FMTowns &towns) const
{
	towns.cpu.PrintState();
	towns.PrintStack(32);
	towns.PrintDisassembly();
}