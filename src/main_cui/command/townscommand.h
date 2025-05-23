/* LICENSE>>
Copyright 2020 Soji Yamakawa (CaptainYS, http://www.ysflight.com)

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

<< LICENSE */
#ifndef TOWNS_CMD_IS_INCLUDED
#define TOWNS_CMD_IS_INCLUDED
/* { */

#include <vector>
#include <string>
#include <set>
#include <unordered_map>

#include "towns.h"
#include "townsthread.h"
#include "i486symtable.h"

class TownsCommandInterpreter
{
private:
	std::unordered_map <std::string,unsigned int> primaryCmdMap;
	std::unordered_map <std::string,unsigned int> featureMap;
	std::unordered_map <std::string,unsigned int> dumpableMap;
	std::unordered_map <std::string,unsigned int> breakEventMap;

	std::set <unsigned int> memFilter;

public:
	enum
	{
		CMD_NONE,

		CMD_QUIT,

		CMD_RESET,

		CMD_HELP,

		CMD_RUN,
		CMD_RUN_ONE_INSTRUCTION,
		CMD_PAUSE,
		CMD_WAIT,

		CMD_INTERRUPT,
		CMD_EXCEPTION,

		CMD_MAKE_MEMORY_FILTER,
		CMD_UPDATE_MEMORY_FILTER,

		CMD_FIND,
		CMD_FIND_STRING,
		CMD_FIND_CALLER,

		CMD_RETURN_FROM_PROCEDURE,

		CMD_ENABLE,
		CMD_DISABLE,
		// ENABLE CMDLOG
		// DISABLE CMDLOG

		CMD_DUMP,
		CMD_PRINT_STATUS,
		CMD_PRINT_HISTORY,
		CMD_PRINT_SYMBOL,
		CMD_PRINT_SYMBOL_LABEL_PROC,
		CMD_PRINT_SYMBOL_PROC,
		CMD_PRINT_SYMBOL_FIND,
		CMD_SAVE_HISTORY,

		CMD_ADD_BREAKPOINT,
		CMD_ADD_BREAKPOINT_WITH_PASSCOUNT,
		CMD_ADD_MONITORPOINT,
		CMD_DELETE_BREAKPOINT,
		CMD_LIST_BREAKPOINTS,

		CMD_BREAK_ON,
		CMD_DONT_BREAK_ON,

		CMD_DISASM,
		CMD_DISASM16,
		CMD_DISASM32,
		CMD_TRANSLATE_ADDRESS,

		CMD_ADD_SYMBOL,
		CMD_ADD_LABEL,
		CMD_ADD_DATALABEL,
		CMD_ADD_COMMENT,
		CMD_DEF_RAW_BYTES,
		CMD_IMM_IS_IOPORT,
		CMD_DEL_SYMBOL,

		CMD_TYPE_KEYBOARD,
		CMD_KEYBOARD,

		CMD_LET,
		CMD_EDIT_MEMORY_BYTE,
		CMD_EDIT_MEMORY_WORD,
		CMD_EDIT_MEMORY_DWORD,
		CMD_EDIT_MEMORY_STRING,
		CMD_REPLACE,

		CMD_CRTC_PAGE,

		CMD_CALCULATE,

		CMD_CMOSSAVE,
		CMD_CMOSLOAD,
		CMD_CDLOAD,
		CMD_SCSICD0LOAD,
		CMD_SCSICD1LOAD,
		CMD_SCSICD2LOAD,
		CMD_SCSICD3LOAD,
		CMD_SCSICD4LOAD,
		CMD_SCSICD5LOAD,
		CMD_SCSICD6LOAD,
		CMD_CDOPENCLOSE,
		CMD_CDDASTOP,
		CMD_FD0LOAD,
		CMD_FD0EJECT,
		CMD_FD0WRITEPROTECT,
		CMD_FD0WRITEUNPROTECT,
		CMD_FD1LOAD,
		CMD_FD1EJECT,
		CMD_FD1WRITEPROTECT,
		CMD_FD1WRITEUNPROTECT,

		CMD_SAVE_EVENTLOG,
		CMD_LOAD_EVENTLOG,
		CMD_PLAY_EVENTLOG,
		CMD_STOP_EVENTLOG,

		CMD_SAVE_KEYMAP,
		CMD_LOAD_KEYMAP,

		CMD_SAVE_YM2612LOG,
		CMD_YM2612_CH_ON_OFF,

		CMD_FMVOL,
		CMD_PCMVOL,

		CMD_HOST_TO_VM_FILE_TRANSFER,
		CMD_VM_TO_HOST_FILE_TRANSFER,

		CMD_FREQUENCY,

		CMD_XMODEM_CLEAR,
		CMD_XMODEM_TO_VM,
		CMD_XMODEM_FROM_VM,
		CMD_XMODEMCRC_FROM_VM,

		CMD_SAVE_SCREENSHOT,
		CMD_SAVE_VRAMLAYER,

		CMD_SPECIAL_DEBUG,
		CMD_DOSSEG,

		CMD_START_FMPCM_RECORDING,
		CMD_END_FMPCM_RECORDING,
		CMD_SAVE_FMPCM_RECORDING,
	};

	enum
	{
		ENABLE_NONE,
		ENABLE_CMDLOG,
		ENABLE_DISASSEMBLE_EVERY_INST,
		ENABLE_IOMONITOR,
		ENABLE_EVENTLOG,
		ENABLE_DEBUGGER,
		ENABLE_MOUSEINTEGRATION,
		ENABLE_YM2612_LOG,
		ENABLE_SCSICMDMONITOR,
	};

	enum
	{
		DUMP_NONE,
		DUMP_CURRENT_STATUS,
		DUMP_CALLSTACK,
		DUMP_BREAKPOINT,
		DUMP_PIC,
		DUMP_DMAC,
		DUMP_FDC,
		DUMP_CRTC,
		DUMP_HIRESCRTC,
		DUMP_PALETTE,
		DUMP_HIRESPALETTE,
		DUMP_TIMER,
		DUMP_GDT,
		DUMP_LDT,
		DUMP_IDT,
		DUMP_TSS,
		DUMP_TSS_IOMAP,
		DUMP_SOUND,
		DUMP_SEGMENT_REGISTER_DETAILS,
		DUMP_REAL_MODE_INT_VECTOR,
		DUMP_CSEIP_LOG,
		DUMP_SYMBOL_TABLE,
		DUMP_MEMORY,
		DUMP_CMOS,
		DUMP_CDROM,
		DUMP_EVENTLOG,
		DUMP_SCSI,
		DUMP_SCHEDULE,
		DUMP_TIME_BALANCE,
		DUMP_SPRITE,
		DUMP_SPRITE_AT,
		DUMP_SPRITE_PALETTE,
		DUMP_SPRITE_PATTERN_4BIT,
		DUMP_SPRITE_PATTERN_16BIT,
		DUMP_MOUSE,
		DUMP_YM2612_LOG,
		DUMP_DOS_INFO,
		DUMP_MEMORY_FILTER,
	};

	enum
	{
		BREAK_ON_PIC_IWC1,
		BREAK_ON_PIC_IWC4,
		BREAK_ON_DMAC_REQUEST,
		BREAK_ON_FDC_COMMAND,
		BREAK_ON_INT,
		BREAK_ON_CVRAM_READ,
		BREAK_ON_CVRAM_WRITE,
		BREAK_ON_FMRVRAM_READ,
		BREAK_ON_FMRVRAM_WRITE,
		BREAK_ON_IOREAD,
		BREAK_ON_IOWRITE,
		BREAK_ON_VRAMREAD,
		BREAK_ON_VRAMWRITE,
		BREAK_ON_VRAMREADWRITE,
		BREAK_ON_CDC_COMMAND,
		BREAK_ON_CDC_DEI,
		BREAK_ON_CDC_DATAREADY,
		BREAK_ON_LBUTTON_UP,
		BREAK_ON_LBUTTON_DOWN,
		BREAK_ON_RETURN_KEY,
		BREAK_ON_SCSI_COMMAND,
		BREAK_ON_SCSI_DMA_TRANSFER,
		BREAK_ON_MEM_READ,
		BREAK_ON_MEM_WRITE,
	};

	enum
	{
		ERROR_TOO_FEW_ARGS,
		ERROR_DUMP_TARGET_UNDEFINED,
		ERROR_CANNOT_OPEN_FILE,
		ERROR_CANNOT_SAVE_FILE,
		ERROR_INCORRECT_FILE_SIZE,
		ERROR_SYMBOL_NOT_FOUND,
		ERROR_COULD_NOT_DELETE_SYMBOL,
		ERROR_UNDEFINED_KEYBOARD_MODE,
		ERROR_NO_DATA_GIVEN,
		ERROR_WRONG_PARAMETER,
		ERROR_VRAM_LAYER_UNAVAILABLE,
		ERROR_UNDEFINED_BREAK_POINT_OPTION,
	};

	class Command
	{
	public:
		std::string cmdline;
		std::vector <std::string> argv;
		int primaryCmd;
	};

	bool waitVM;

	TownsCommandInterpreter();

	void PrintHelp(void) const;
	void PrintError(int errCode) const;

	Command Interpret(const std::string &cmdline) const;

	/*! Executes a command.
	    VM must be locked before calling.
	*/
	void Execute(TownsThread &thr,FMTowns &towns,class Outside_World *outside_world,Command &cmd);

	void Execute_Enable(FMTowns &towns,Command &cmd);
	void Execute_Disable(FMTowns &towns,Command &cmd);

	void Execute_AddBreakPoint(FMTowns &towns,Command &cmd);
	void Execute_AddBreakPointWithPassCount(FMTowns &towns,Command &cmd);
	void Execute_AddMonitorPoint(FMTowns &towns,Command &cmd);
	void Execute_DeleteBreakPoint(FMTowns &towns,Command &cmd);
	void Execute_ListBreakPoints(FMTowns &towns,Command &cmd);

	void Execute_Dump(FMTowns &towns,Command &cmd);
	void Execute_Dump_DOSInfo(FMTowns &towns,Command &cmd);

	void Execute_Calculate(FMTowns &towns,Command &cmd);

	void Execute_BreakOn(FMTowns &towns,Command &cmd);
	void Execute_ClearBreakOn(FMTowns &towns,Command &cmd);

	void Execute_AddressTranslation(FMTowns &towns,Command &cmd);
	void Execute_Disassemble(FMTowns &towns,Command &cmd);
	void Execute_Disassemble16(FMTowns &towns,Command &cmd);
	void Execute_Disassemble32(FMTowns &towns,Command &cmd);

	void Execute_PrintHistory(FMTowns &towns,unsigned int n);
	void Execute_SaveHistory(FMTowns &towns,const std::string &fName);

	void Execute_SaveEventLog(FMTowns &towns,const std::string &fName);

	void Execute_AddSymbol(FMTowns &towns,Command &cmd);
	void Execute_DelSymbol(FMTowns &towns,Command &cmd);

	void Execute_SymbolInquiry(FMTowns &towns,Command &cmd);

	void Execute_TypeKeyboard(FMTowns &towns,Command &cmd);

	void Execute_Let(FMTowns &towns,Command &cmd);
	void Execute_EditMemory(FMTowns &towns,Command &cmd,unsigned int numBytes);
	void Execute_Replace(FMTowns &towns,Command &cmd);

	void Execute_CRTCPage(FMTowns &towns,Command &cmd);

	void Execute_CMOSLoad(FMTowns &towns,Command &cmd);
	void Execute_CMOSSave(FMTowns &towns,Command &cmd);
	void Execute_CDLoad(FMTowns &towns,Command &cmd);
	void Execute_SCSICDLoad(unsigned int SCSIID,FMTowns &towns,const Command &cmd);
	void Execute_FDLoad(int drv,FMTowns &towns,Command &cmd);
	void Execute_FDEject(int drv,FMTowns &towns,Command &cmd);

	void Execute_MakeMemoryFilter(FMTowns &towns,Command &cmd);
	void Execute_UpdateMemoryFilter(FMTowns &towns,Command &cmd);

	void Execute_Exception(FMTowns &towns,Command &cmd);

	void Execute_SaveYM2612Log(FMTowns &towns,std::string fName);

	void Execute_SaveKeyMap(const Outside_World &outside_world,const Command &cmd);
	void Execute_LoadKeyMap(Outside_World &outside_world,const Command &cmd);

	void Execute_Search_Bytes(FMTowns &towns,Command &cmd);
	void Execute_Search_String(FMTowns &towns,Command &cmd);
	void Execute_Search_ByteSequence(FMTowns &towns,const std::vector <unsigned char> &bytes);
	void FoundAt(FMTowns &towns,unsigned int physAddr);
	void FoundAt(std::string segLabel,unsigned int linearBase,unsigned int linearAddr);

	void Execute_Find_Caller(FMTowns &towns,Command &cmd);

	void Execute_XMODEMtoVM(FMTowns &towns,Command &cmd);
	void Execute_XMODEMfromVM(FMTowns &towns,Command &cmd);
	void Execute_XMODEMCRCfromVM(FMTowns &towns,Command &cmd);

	void Execute_SaveScreenShot(FMTowns &towns,Command &cmd);
	void Execute_SaveVRAMLayer(FMTowns &towns,Command &cmd);

	void Execute_SpecialDebug(FMTowns &towns,Command &cmd);
};


/* } */
#endif
