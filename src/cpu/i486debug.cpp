#include <iostream>
#include <fstream>

#include "i486.h"
#include "i486inst.h"
#include "i486debug.h"



i486Debugger::i486Debugger()
{
	CleanUp();
}
void i486Debugger::CleanUp(void)
{
	breakPoint.clear();
	stop=true;
	disassembleEveryStep=false;
	lastDisassembleAddr.Nullify();
}
void i486Debugger::AddBreakPoint(unsigned int CS,unsigned int EIP)
{
	CS_EIP bp;
	bp.SEG=CS;
	bp.OFFSET=EIP;
	auto iter=breakPoint.find(bp);
	if(breakPoint.end()!=iter)
	{
		breakPoint.insert(bp);
	}
}
void i486Debugger::RemoveBreakPoint(unsigned int CS,unsigned int EIP)
{
	CS_EIP bp;
	bp.SEG=CS;
	bp.OFFSET=EIP;
	auto iter=breakPoint.find(bp);
	if(breakPoint.end()!=iter)
	{
		breakPoint.erase(iter);
	}
}

void i486Debugger::SetOneTimeBreakPoint(unsigned int CS,unsigned int EIP)
{
	oneTimeBreakPoint.SEG=CS;
	oneTimeBreakPoint.OFFSET=EIP;
}

void i486Debugger::BeforeRunOneInstruction(i486DX &cpu,Memory &mem,InOut &io,const i486DX::Instruction &inst)
{
	CS_EIP cseip;
	cseip.SEG=cpu.state.CS().value;
	cseip.OFFSET=cpu.state.EIP;

	if(true==disassembleEveryStep && lastDisassembleAddr!=cseip)
	{
		auto inst=cpu.FetchInstruction(mem);
		auto disasm=cpu.Disassemble(inst,cpu.state.CS(),cpu.state.EIP,mem);
		lastDisassembleAddr.SEG=cpu.state.CS().value;
		lastDisassembleAddr.OFFSET=cpu.state.EIP;
		std::cout << disasm << std::endl;
	}
}

void i486Debugger::AfterRunOneInstruction(unsigned int clocksPassed,i486DX &cpu,Memory &mem,InOut &io,const i486DX::Instruction &inst)
{
	CS_EIP cseip;
	cseip.SEG=cpu.state.CS().value;
	cseip.OFFSET=cpu.state.EIP;

	if(breakPoint.find(cseip)!=breakPoint.end())
	{
		stop=true;
	}
	if(oneTimeBreakPoint==cseip)
	{
		stop=true;
		oneTimeBreakPoint.Nullify();
	}
}