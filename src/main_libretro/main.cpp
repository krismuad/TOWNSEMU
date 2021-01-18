/* LICENSE>>
Copyright 2020 Soji Yamakawa (CaptainYS, http://www.ysflight.com)

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

<< LICENSE */
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <string>
#include <chrono>

//muad
#include "libretro.h"
#include "towns.h"
#include "townsargv.h"
#include "i486symtable.h"
#include "cpputil.h"
#include "discimg.h"
#include "icons.h"
#include "render.h"

//outside_word_retroarch wrapper
#include "outside_world.h"
#include "yssimplesound.h"
#include "diskimg.h"

/*
Tsugaru libretro port : muad
date : 20210118


I. Port Status 

Support iso and bin/cue file ( no zip, no chd, no mdf/mds )
The libretro port try to find associated [BootDisk] ( or [SystemDisk]  or [UserDisk]) ans insert it in FD0
Generate a save disk per game called "gamename]SaveDisk[.hdm" and inserted in FD0 or FD1

if the mouse isn't working, try the fix in "Application Settings" menu option.
After change in a game setting, you have to reset the emulator. 

Emulation: ok
Video : ok
Sound :  ok ( sound will desync if the emulator is paused )
Gamepad : ok ( second gamepad isn't working )
Mouse : ok ( hit F11 to hide host cursor )
keyboard : ok ( qwerty - no shift - capslock  )


II. Compilation help ( reminder )

cd TOWNSEMU-master
mkdir build
cd build
cmake ../src -DCMAKE_GENERATOR_PLATFORM=x64

Open Tsugaru.sln with MS Visual Studio ( I use MS VS 2017 )

Select Tsugaru_CUI project
for both : release and debug
Project->Settings->
General : Extension : ".exe" -> ".dll"
General : Type : "Application .exe" -> "Dynamic link Library .dll"
General : Target Name : "libretro_tsugaru.dll"



III. Retroarch config:

Settings->Input->Hotkeys->Hotkey Enable : ( i use 'tilde' but you can choose what you want )
 with this settings, you have to hit 'tilde' + F1 to access menu while emulation
 and you can send all keystrock to the emulator

While in emulation il you hit [yourhotkey] + F11 , you can hide the host mouse cursor.
 usefull for mouse game.
 

Emulator default setting : 
	Port1 : Gamepad
	Port2 : Mouse



IV. TODO 
Handle the need of hdd
Split main.cpp
SaveStates


V. Notes :

At least for MS Visual Studio, add #include <algorithm>	if compile error for std::min


VI. Bonus File

File RetroArch\info\tsugaru_libretro.info

# Software Information
display_name = "Fujitsu FM TOWN (Tsugaru)"
authors = "CaptainYS"
supported_extensions = "iso|cue"
corename = "tsugaru_libretro"
categories = "Emulator"
license = "Open Source with 3-clause BSD License."
permissions = ""
display_version = "v20201126"

# Hardware Information
manufacturer = "Fujitsu"
systemname = "FM TOWNS"
systemid = "FM TOWNS"

# Libretro Features
database = "FM TOWNS"
supports_no_game = "false"
savestate = "false"
savestate_features = "basic"
cheats = "false"
input_descriptors = "true"
memory_descriptors = "false"
libretro_saves = "false"
core_options = "true"
load_subsystem = "false"
hw_render = "false"
needs_fullpath = "true"
disk_control = "false"
is_experimental = "false"

# BIOS/Firmware
firmware_count = 5

firmware0_desc = "FMT_DIC.ROM (BIOS)"
firmware0_path = "FMT_DIC.ROM"
firmware0_opt = "false"
notes = "(!) FMT_DIC.ROM (md5): 8fa4e553f28cfc0c30a0a1e589799942"

firmware1_desc = "FMT_DOS.ROM (BIOS)"
firmware1_path = "FMT_DOS.ROM"
firmware1_opt = "false"
notes = "(!) FMT_DOS.ROM (md5): 0585b19930d4a7f4c71bcc8a33746588"

firmware2_desc = "FMT_F20.ROM (BIOS)"
firmware2_path = "FMT_F20.ROM"
firmware2_opt = "false"
notes = "(!) FMT_F20.ROM (md5): ac0c7021e9bf48ca84b51ab651169a88"

firmware3_desc = "FMT_FNT.ROM (BIOS)"
firmware3_path = "FMT_FNT.ROM"
firmware3_opt = "false"
notes = "(!) FMT_FNT.ROM (md5): b91300e55b70227ce98b59c5f02fa8dd"

firmware4_desc = "FMT_SYS.ROM (BIOS)"
firmware4_path = "FMT_SYS.ROM"
firmware4_opt = "false"
notes = "(!) FMT_SYS.ROM (md5): 86fb6f7280689259f0ca839dd3dd6cde"

End File RetroArch\info\tsugaru_libretro.info

*/



//*****************************************
//***			libretro	 			***
//***			 Globals				***
//*****************************************

retro_environment_t environment_cb;
retro_video_refresh_t video_cb;
retro_audio_sample_t audio_cb;
retro_audio_sample_batch_t audio_batch_cb;
retro_input_poll_t input_poll_cb;
retro_input_state_t input_state_cb;



struct retro_variable options[5] = {
	{ "gameport_0", "Select Gameport 0 type; gamepad|mouse|none" },
	{ "gameport_1", "Select Gameport 1 type; mouse|none|gamepad" },
	{ "app_specific", "Application compatibility settings; none|WINGCOMMANDER1|WINGCOMMANDER2|SUPERDAISEN|LEMMINGS|LEMMINGS2|STRIKECOMMANDER|AMARANTH3|ULTIMAUNDERWORLD" },
	{ "keyboard", "Select keyboard mode; DIRECT|TRANS|TRANS2|TRANS3" },
	{ NULL, NULL }
};


// Used when reset the game in void retro_reset(void)
retro_game_info game_info;


//*****************************************
//***			Tsugaru		 			***
//***			 Globals				***
//*****************************************

TownsARGV argv;
static FMTowns towns;
Outside_World *outside_world = NULL;

TownsRender render;
int runMode;

enum
{
	NANOSECONDS_PER_TIME_SYNC = 1000000,
};

enum
{
	RUNMODE_PAUSE,
	RUNMODE_RUN,
	RUNMODE_ONE_INSTRUCTION, // Always with debugger.
	RUNMODE_EXIT,
};

//*****************************************
//***			Render Stuff			***
//***		   ( Globals  )				***
//***		from townsrenderthread.cpp  ***
//*****************************************
enum
{
	STATE_IDLE,
	STATE_RENDERING,
};
enum
{
	NO_COMMAND,
	RENDER,
	QUIT
};


TownsRender *rendererPtr;

unsigned char VRAMCopy[TOWNS_VRAM_SIZE];
TownsCRTC::AnalogPalette paletteCopy;
TownsCRTC::ChaseHQPalette chaseHQPaletteCopy;

bool imageReady = false;
unsigned int command = NO_COMMAND;
unsigned int state = STATE_IDLE;
int64_t checkImageAfterThisTIme;
int64_t checkKeyAfterThisTIme;


//*****************************************
//***			Outside world 			***
//***			RetroArch wrapper		***
//*****************************************
class FsRetroArchConnection : public Outside_World
{
public:
	#define PAUSE_KEY_CODE FSKEY_SCROLLLOCK

	bool gamePadInitialized = false;
	unsigned int *FSKEYtoTownsKEY = nullptr;
	unsigned int *FSKEYState = nullptr;
	FsRetroArchConnection();
	~FsRetroArchConnection();

	// For mouse emulation by pad digital axes.
	int mouseDX = 0, mouseDY = 0;

	int winWid = 640, winHei = 480;
	unsigned int sinceLastResize = 0;


	virtual std::vector <std::string> MakeKeyMappingText(void) const;
	virtual void LoadKeyMappingFromText(const std::vector <std::string> &text);

	virtual void Start(void);
	virtual void Stop(void);
	virtual void DevicePolling(class FMTowns &towns);
	void PollGamePads(void);
	virtual void UpdateStatusBitmap(class FMTowns &towns);
	virtual void Render(const TownsRender::Image &img);
	virtual bool ImageNeedsFlip(void);

	virtual void SetKeyboardLayout(unsigned int layout);

	YsSoundPlayer soundPlayer;
	YsSoundPlayer::SoundData cddaChannel;
	unsigned long long cddaStartHSG;
	virtual void CDDAPlay(const DiscImage &discImg, DiscImage::MinSecFrm from, DiscImage::MinSecFrm to, bool repeat, unsigned int leftLevel, unsigned int rightLevel);
	virtual void CDDAStop(void);
	virtual void CDDAPause(void);
	virtual void CDDAResume(void);
	virtual bool CDDAIsPlaying(void);
	virtual DiscImage::MinSecFrm CDDACurrentPosition(void);
	/*
	YsSoundPlayer::SoundData PCMChannel;
	virtual void PCMPlay(std::vector <unsigned char > &wave);
	virtual void PCMPlayStop(void);
	virtual bool PCMChannelPlaying(void);

	YsSoundPlayer::SoundData FMChannel;
	virtual void FMPlay(std::vector <unsigned char> &wave);
	virtual void FMPlayStop(void);
	virtual bool FMChannelPlaying(void);
	*/
	YsSoundPlayer::SoundData FMPCMChannel;
	virtual void FMPCMPlay(std::vector <unsigned char > &wave);
	virtual void FMPCMPlayStop(void);
	virtual bool FMPCMChannelPlaying(void);


	virtual void BeepPlay(int samplingRate, std::vector<unsigned char>& wave) ;
	virtual void BeepPlayStop() ;
	virtual bool BeepChannelPlaying() const;
	virtual void Render(const TownsRender::Image &img, const class FMTowns &towns) ;


	void processMouseH( float mouse_speed);	// muad : why H ? why not.
	int FsGetKeyState(int fsKeyCode);

};



FsRetroArchConnection::FsRetroArchConnection()
{
	FSKEYtoTownsKEY = new unsigned int[RETROK_LAST];
	FSKEYState = new unsigned int[RETROK_LAST];

	SetKeyboardMode(TOWNS_KEYBOARD_MODE_DIRECT);
	SetKeyboardLayout(KEYBOARD_LAYOUT_US);

	for (int i = 0; i<RETROK_LAST; ++i)
	{
		FSKEYState[i] = TOWNS_KEYFLAG_JIS_RELEASE;
	}


}
FsRetroArchConnection::~FsRetroArchConnection()
{
	delete[] FSKEYtoTownsKEY;
	delete[] FSKEYState;
}


int FsRetroArchConnection::FsGetKeyState(int fsKeyCode)
{

	if (FSKEYState[fsKeyCode] == TOWNS_KEYFLAG_JIS_PRESS ) return 1;

	return 0;
}



/* virtual */ std::vector <std::string> FsRetroArchConnection::MakeKeyMappingText(void) const
{
	std::vector <std::string> text;

	return text;
}
/* virtual */ void FsRetroArchConnection::LoadKeyMappingFromText(const std::vector <std::string> &text)
{

}

/* virtual */ void FsRetroArchConnection::Start(void)
{
	int wid = 640 * scaling / 100;
	int hei = 480 * scaling / 100;

	int winY0 = 0;
	if (true == windowShift)
	{
		winY0 = 48;
	}


	soundPlayer.Start();
	cddaStartHSG = 0;


	// Make initial status bitmap
	Put16x16Invert(0, 15, CD_IDLE);
	for (int fd = 0; fd<2; ++fd)
	{
		Put16x16Invert(16 + 16 * fd, 15, FD_IDLE);
	}
	for (int hdd = 0; hdd<6; ++hdd)
	{
		Put16x16Invert(48 + 16 * hdd, 15, HDD_IDLE);
	}
}



/* virtual */ void FsRetroArchConnection::Stop(void) { 	soundPlayer.End(); }



// processMouseH : Function taken from scummvm libretroarch
#define BASE_CURSOR_SPEED 4
float _mouseXAcc;
float _mouseYAcc;
int _mouseX;
int _mouseY;
int _screen_w = 640;
int _screen_h = 480;

void FsRetroArchConnection::processMouseH( float mouse_speed)
{

	int mouse_acc_int;
	bool  do_mouse;
	int16_t x = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
	int16_t y = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);

	// Process input from physical mouse
	do_mouse = false;
	// > X Axis
	if (x != 0)
	{
		if (x > 0) {
			// Reset accumulator when changing direction
			_mouseXAcc = (_mouseXAcc < 0.0) ? 0.0 : _mouseXAcc;
		}
		if (x < 0) {
			// Reset accumulator when changing direction
			_mouseXAcc = (_mouseXAcc > 0.0) ? 0.0 : _mouseXAcc;
		}
		// Update accumulator
		_mouseXAcc += (float)x * mouse_speed;
		// Get integer part of accumulator
		mouse_acc_int = (int)_mouseXAcc;
		if (mouse_acc_int != 0)
		{
			// Set mouse position
			_mouseX += mouse_acc_int;
			_mouseX = (_mouseX < 0) ? 0 : _mouseX;
			_mouseX = (_mouseX >= _screen_w) ? _screen_w : _mouseX;
			do_mouse = true;
			// Update accumulator
			_mouseXAcc -= (float)mouse_acc_int;
		}
	}
	// > Y Axis
	if (y != 0)
	{
		if (y > 0) {
			// Reset accumulator when changing direction
			_mouseYAcc = (_mouseYAcc < 0.0) ? 0.0 : _mouseYAcc;
		}
		if (y < 0) {
			// Reset accumulator when changing direction
			_mouseYAcc = (_mouseYAcc > 0.0) ? 0.0 : _mouseYAcc;
		}
		// Update accumulator
		_mouseYAcc += (float)y * mouse_speed;
		// Get integer part of accumulator
		mouse_acc_int = (int)_mouseYAcc;
		if (mouse_acc_int != 0)
		{
			// Set mouse position
			_mouseY += mouse_acc_int;
			_mouseY = (_mouseY < 0) ? 0 : _mouseY;
			_mouseY = (_mouseY >= _screen_h) ? _screen_h : _mouseY;
			do_mouse = true;
			// Update accumulator
			_mouseYAcc -= (float)mouse_acc_int;
		}
	}


	bool mouse_l = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT);
	bool mouse_r = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_RIGHT);

	if (do_mouse)
	{
		unsigned int portId = 0;
		this->ProcessMouse(towns, mouse_l, 0, mouse_r, _mouseX, _mouseY);
	}

	if(mouse_l || mouse_r)
		this->ProcessMouse(towns, mouse_l, 0, mouse_r, _mouseX, _mouseY);

}



// see : void FsSimpleWindowConnection::DevicePolling(class FMTowns &towns)
/* virtual */ void FsRetroArchConnection::DevicePolling(class FMTowns &towns)
{
	input_poll_cb();

	//PAD1
	if (towns.gameport.state.ports[0].device == TownsGamePort::GAMEPAD) {

		bool Abutton = (0 != input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A));
		bool Bbutton = (0 != input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B));
		bool run = (0 != input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START));
		bool pause = (0 != input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT));
		bool left = (0 != input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT));
		bool right = (0 != input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT));
		if (true == left && true == right)
		{
			right = false;
		}
		bool up = (0 != input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP));
		bool down = (0 != input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN));
		if (true == up && true == down)
		{
			down = false;
		}

		unsigned int portId = 0;
		towns.SetGamePadState(portId, Abutton, Bbutton, left, right, up, down, run, pause);
	}


	//PAD2
	if (towns.gameport.state.ports[1].device == TownsGamePort::GAMEPAD) {

		bool Abutton = (0 != input_state_cb(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A));
		bool Bbutton = (0 != input_state_cb(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B));
		bool run = (0 != input_state_cb(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START));
		bool pause = (0 != input_state_cb(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT));
		bool left = (0 != input_state_cb(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT));
		bool right = (0 != input_state_cb(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT));
		if (true == left && true == right)
		{
			right = false;
		}
		bool up = (0 != input_state_cb(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP));
		bool down = (0 != input_state_cb(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN));
		if (true == up && true == down)
		{
			down = false;
		}

		unsigned int portId = 1;
		towns.SetGamePadState(portId, Abutton, Bbutton, left, right, up, down, run, pause);
	}


	//** Mouse
	// RETRO_DEVICE_MOUSE
	if (towns.gameport.state.ports[0].device == TownsGamePort::MOUSE)  
	{
		processMouseH(1.0f);
	}

	if (towns.gameport.state.ports[1].device == TownsGamePort::MOUSE) 
	{
		processMouseH(1.0f);

	}




	/** keyboard time */

	if (checkKeyAfterThisTIme > towns.state.townsTime) return;
	

	checkKeyAfterThisTIme = towns.state.townsTime + 150000000; // 
	int key;

	//if (STATE_RENDERING == state && checkImageAfterThisTIme<towns.state.townsTime)


	//check release
	for (key = 0; key < RETROK_LAST; key++)
	{
		int ret;
		unsigned char byteData = 0;
		
		if (FSKEYState[key] == TOWNS_KEYFLAG_JIS_PRESS)
		{
			ret = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, key);

			if (ret == 0)
			{
				byteData |= (0 != FsGetKeyState(RETROK_RCTRL) ? TOWNS_KEYFLAG_CTRL : 0);
				byteData |= (0 != FsGetKeyState(RETROK_LSHIFT) ? TOWNS_KEYFLAG_SHIFT : 0);

				towns.keyboard.PushFifo(byteData | TOWNS_KEYFLAG_JIS_RELEASE, FSKEYtoTownsKEY[key]);

				FSKEYState[key] = TOWNS_KEYFLAG_JIS_RELEASE;
			}

		}

	}

	//check press
	for (key = 0; key < RETROK_LAST; key++)
	{
		int ret;
		unsigned char byteData = 0;

		ret = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, key);

		if (ret != 0)
		{
				byteData |= (0 != FsGetKeyState(RETROK_RCTRL) ? TOWNS_KEYFLAG_CTRL : 0);
				byteData |= (0 != FsGetKeyState(RETROK_LSHIFT) ? TOWNS_KEYFLAG_SHIFT : 0);

				towns.keyboard.PushFifo(byteData | TOWNS_KEYFLAG_JIS_PRESS, FSKEYtoTownsKEY[key]);

				FSKEYState[key] = TOWNS_KEYFLAG_JIS_PRESS;
		}

	}



}

// done in : void FsRetroArchConnection::DevicePolling(class FMTowns &towns)
void FsRetroArchConnection::PollGamePads(void) {  }

/* virtual */ void FsRetroArchConnection::UpdateStatusBitmap(class FMTowns &towns)
{
	// Update Status Bitmap
	{
		bool busy = (true != towns.cdrom.state.DRY);
		if (cdAccessLamp != busy)
		{
			Put16x16SelectInvert(0, 15, CD_IDLE, CD_BUSY, busy);
			cdAccessLamp = busy;
		}
	}
	for (int fd = 0; fd<2; ++fd)
	{
		bool busy = (fd == towns.fdc.DriveSelect() && true == towns.fdc.state.busy);
		if (fdAccessLamp[fd] != busy)
		{
			Put16x16SelectInvert(16 + 16 * fd, 15, FD_IDLE, FD_BUSY, busy);
			fdAccessLamp[fd] = busy;
		}
	}
	for (int hdd = 0; hdd<6; ++hdd)
	{
		bool busy = (hdd == towns.scsi.state.selId && true == towns.scsi.state.BUSY);
		if (scsiAccessLamp[hdd] != busy)
		{
			Put16x16SelectInvert(48 + 16 * hdd, 15, HDD_IDLE, HDD_BUSY, busy);
			scsiAccessLamp[hdd] = busy;
		}
	}

}


// 
/* virtual */ void FsRetroArchConnection::Render(const TownsRender::Image &img) { }


/* virtual */ bool FsRetroArchConnection::ImageNeedsFlip(void)
{
	return true;  // OpenGL does require flip.
}

/* virtual */ void FsRetroArchConnection::SetKeyboardLayout(unsigned int layout)
{
	for (int i = 0; i<RETROK_LAST; ++i)
	{
		FSKEYtoTownsKEY[i] = TOWNS_JISKEY_NULL;
	}
	FSKEYtoTownsKEY[RETROK_UNKNOWN] = TOWNS_JISKEY_NULL;
	FSKEYtoTownsKEY[RETROK_SPACE] = TOWNS_JISKEY_SPACE;
	FSKEYtoTownsKEY[RETROK_0] = TOWNS_JISKEY_0;
	FSKEYtoTownsKEY[RETROK_1] = TOWNS_JISKEY_1;
	FSKEYtoTownsKEY[RETROK_2] = TOWNS_JISKEY_2;
	FSKEYtoTownsKEY[RETROK_3] = TOWNS_JISKEY_3;
	FSKEYtoTownsKEY[RETROK_4] = TOWNS_JISKEY_4;
	FSKEYtoTownsKEY[RETROK_5] = TOWNS_JISKEY_5;
	FSKEYtoTownsKEY[RETROK_6] = TOWNS_JISKEY_6;
	FSKEYtoTownsKEY[RETROK_7] = TOWNS_JISKEY_7;
	FSKEYtoTownsKEY[RETROK_8] = TOWNS_JISKEY_8;
	FSKEYtoTownsKEY[RETROK_9] = TOWNS_JISKEY_9;
	FSKEYtoTownsKEY[RETROK_a] = TOWNS_JISKEY_A;
	FSKEYtoTownsKEY[RETROK_b] = TOWNS_JISKEY_B;
	FSKEYtoTownsKEY[RETROK_c] = TOWNS_JISKEY_C;
	FSKEYtoTownsKEY[RETROK_d] = TOWNS_JISKEY_D;
	FSKEYtoTownsKEY[RETROK_e] = TOWNS_JISKEY_E;
	FSKEYtoTownsKEY[RETROK_f] = TOWNS_JISKEY_F;
	FSKEYtoTownsKEY[RETROK_g] = TOWNS_JISKEY_G;
	FSKEYtoTownsKEY[RETROK_h] = TOWNS_JISKEY_H;
	FSKEYtoTownsKEY[RETROK_i] = TOWNS_JISKEY_I;
	FSKEYtoTownsKEY[RETROK_j] = TOWNS_JISKEY_J;
	FSKEYtoTownsKEY[RETROK_k] = TOWNS_JISKEY_K;
	FSKEYtoTownsKEY[RETROK_l] = TOWNS_JISKEY_L;
	FSKEYtoTownsKEY[RETROK_m] = TOWNS_JISKEY_M;
	FSKEYtoTownsKEY[RETROK_n] = TOWNS_JISKEY_N;
	FSKEYtoTownsKEY[RETROK_o] = TOWNS_JISKEY_O;
	FSKEYtoTownsKEY[RETROK_p] = TOWNS_JISKEY_P;
	FSKEYtoTownsKEY[RETROK_q] = TOWNS_JISKEY_Q;
	FSKEYtoTownsKEY[RETROK_r] = TOWNS_JISKEY_R;
	FSKEYtoTownsKEY[RETROK_s] = TOWNS_JISKEY_S;
	FSKEYtoTownsKEY[RETROK_t] = TOWNS_JISKEY_T;
	FSKEYtoTownsKEY[RETROK_u] = TOWNS_JISKEY_U;
	FSKEYtoTownsKEY[RETROK_v] = TOWNS_JISKEY_V;
	FSKEYtoTownsKEY[RETROK_w] = TOWNS_JISKEY_W;
	FSKEYtoTownsKEY[RETROK_x] = TOWNS_JISKEY_X;
	FSKEYtoTownsKEY[RETROK_y] = TOWNS_JISKEY_Y;
	FSKEYtoTownsKEY[RETROK_z] = TOWNS_JISKEY_Z;
	FSKEYtoTownsKEY[RETROK_ESCAPE] = TOWNS_JISKEY_BREAK;
	FSKEYtoTownsKEY[RETROK_F1] = TOWNS_JISKEY_PF01;
	FSKEYtoTownsKEY[RETROK_F2] = TOWNS_JISKEY_PF02;
	FSKEYtoTownsKEY[RETROK_F3] = TOWNS_JISKEY_PF03;
	FSKEYtoTownsKEY[RETROK_F4] = TOWNS_JISKEY_PF04;
	FSKEYtoTownsKEY[RETROK_F5] = TOWNS_JISKEY_PF05;
	FSKEYtoTownsKEY[RETROK_F6] = TOWNS_JISKEY_PF06;
	FSKEYtoTownsKEY[RETROK_F7] = TOWNS_JISKEY_PF07;
	FSKEYtoTownsKEY[RETROK_F8] = TOWNS_JISKEY_PF08;
	FSKEYtoTownsKEY[RETROK_F9] = TOWNS_JISKEY_PF09;
	FSKEYtoTownsKEY[RETROK_F10] = TOWNS_JISKEY_PF10;
	FSKEYtoTownsKEY[RETROK_F11] = TOWNS_JISKEY_PF11;
	FSKEYtoTownsKEY[RETROK_F12] = TOWNS_JISKEY_PF12;
	FSKEYtoTownsKEY[RETROK_PRINT] = TOWNS_JISKEY_NULL;
	FSKEYtoTownsKEY[RETROK_CAPSLOCK] = TOWNS_JISKEY_CAPS;
	FSKEYtoTownsKEY[RETROK_SCROLLOCK] = TOWNS_JISKEY_NULL; // Can assign something later.
	FSKEYtoTownsKEY[RETROK_BREAK] = TOWNS_JISKEY_BREAK;
	FSKEYtoTownsKEY[RETROK_BACKSPACE] = TOWNS_JISKEY_BACKSPACE;
	FSKEYtoTownsKEY[RETROK_TAB] = TOWNS_JISKEY_TAB;
	FSKEYtoTownsKEY[RETROK_RETURN] = TOWNS_JISKEY_RETURN;
	FSKEYtoTownsKEY[RETROK_RSHIFT] = TOWNS_JISKEY_SHIFT;
	FSKEYtoTownsKEY[RETROK_RCTRL] = TOWNS_JISKEY_CTRL;
	FSKEYtoTownsKEY[RETROK_RALT] = TOWNS_JISKEY_NULL; // Can assign something later.
	FSKEYtoTownsKEY[RETROK_INSERT] = TOWNS_JISKEY_INSERT;
	FSKEYtoTownsKEY[RETROK_DELETE] = TOWNS_JISKEY_DELETE;
	FSKEYtoTownsKEY[RETROK_HOME] = TOWNS_JISKEY_HOME;
	FSKEYtoTownsKEY[RETROK_END] = TOWNS_JISKEY_NULL; // Should be translated as SHIFT+DEL
	FSKEYtoTownsKEY[RETROK_PAGEUP] = TOWNS_JISKEY_PREV;
	FSKEYtoTownsKEY[RETROK_PAGEDOWN] = TOWNS_JISKEY_NEXT;
	FSKEYtoTownsKEY[RETROK_UP] = TOWNS_JISKEY_UP;
	FSKEYtoTownsKEY[RETROK_DOWN] = TOWNS_JISKEY_DOWN;
	FSKEYtoTownsKEY[RETROK_LEFT] = TOWNS_JISKEY_LEFT;
	FSKEYtoTownsKEY[RETROK_RIGHT] = TOWNS_JISKEY_RIGHT;
	FSKEYtoTownsKEY[RETROK_NUMLOCK] = TOWNS_JISKEY_NULL; // Can assign something later.
	FSKEYtoTownsKEY[RETROK_TILDE] = TOWNS_JISKEY_ESC;
	FSKEYtoTownsKEY[RETROK_MINUS] = TOWNS_JISKEY_MINUS;
	FSKEYtoTownsKEY[RETROK_PLUS] = TOWNS_JISKEY_HAT;
	//FSKEYtoTownsKEY[RETROK_LBRACKET] = TOWNS_JISKEY_LEFT_SQ_BRACKET;
	//FSKEYtoTownsKEY[RETROK_RBRACKET] = TOWNS_JISKEY_RIGHT_SQ_BRACKET;
	FSKEYtoTownsKEY[RETROK_BACKSLASH] = TOWNS_JISKEY_BACKSLASH;
	FSKEYtoTownsKEY[RETROK_SEMICOLON] = TOWNS_JISKEY_SEMICOLON;
	//FSKEYtoTownsKEY[RETROK_SINGLEQUOTE] = TOWNS_JISKEY_COLON;
	FSKEYtoTownsKEY[RETROK_COMMA] = TOWNS_JISKEY_COMMA;
	//FSKEYtoTownsKEY[RETROK_DOT] = TOWNS_JISKEY_DOT;
	FSKEYtoTownsKEY[RETROK_SLASH] = TOWNS_JISKEY_SLASH;
	//FSKEYtoTownsKEY[RETROK_KPN0] = TOWNS_JISKEY_NUM_0;
	FSKEYtoTownsKEY[RETROK_KP1] = TOWNS_JISKEY_NUM_1;
	FSKEYtoTownsKEY[RETROK_KP2] = TOWNS_JISKEY_NUM_2;
	FSKEYtoTownsKEY[RETROK_KP3] = TOWNS_JISKEY_NUM_3;
	FSKEYtoTownsKEY[RETROK_KP4] = TOWNS_JISKEY_NUM_4;
	FSKEYtoTownsKEY[RETROK_KP5] = TOWNS_JISKEY_NUM_5;
	FSKEYtoTownsKEY[RETROK_KP6] = TOWNS_JISKEY_NUM_6;
	FSKEYtoTownsKEY[RETROK_KP7] = TOWNS_JISKEY_NUM_7;
	FSKEYtoTownsKEY[RETROK_KP8] = TOWNS_JISKEY_NUM_8;
	FSKEYtoTownsKEY[RETROK_KP9] = TOWNS_JISKEY_NUM_9;
	FSKEYtoTownsKEY[RETROK_KP_PERIOD] = TOWNS_JISKEY_NUM_DOT;
	FSKEYtoTownsKEY[RETROK_KP_DIVIDE] = TOWNS_JISKEY_NUM_SLASH;
	FSKEYtoTownsKEY[RETROK_KP_MULTIPLY] = TOWNS_JISKEY_NUM_STAR;
	FSKEYtoTownsKEY[RETROK_KP_MINUS] = TOWNS_JISKEY_NUM_MINUS;
	FSKEYtoTownsKEY[RETROK_KP_PLUS] = TOWNS_JISKEY_NUM_PLUS;
	FSKEYtoTownsKEY[RETROK_KP_ENTER] = TOWNS_JISKEY_NUM_RETURN;
	//FSKEYtoTownsKEY[RETROK_WHEELUP] = TOWNS_JISKEY_UP;
	//FSKEYtoTownsKEY[RETROK_WHEELDOWN] = TOWNS_JISKEY_DOWN;
	FSKEYtoTownsKEY[RETROK_KP_EQUALS] = TOWNS_JISKEY_ALT; // Can assign something later.

														// Japanese keyboard
	//FSKEYtoTownsKEY[RETROK_CONVERT] = TOWNS_JISKEY_CONVERT;
	//FSKEYtoTownsKEY[RETROK_NONCONVERT] = TOWNS_JISKEY_NO_CONVERT;
	//FSKEYtoTownsKEY[RETROK_KANA] = TOWNS_JISKEY_KATAKANA;
	// FSKEYtoTownsKEY[RETROK_COLON]=       TOWNS_JISKEY_COLON; // Need to switch with single quote
	// FSKEYtoTownsKEY[RETROK_AT]=          TOWNS_JISKEY_AT;  // RETROK_AT collides with RETROK_TILDA. This disables ESC.
	//FSKEYtoTownsKEY[RETROK_RO] = TOWNS_JISKEY_DOUBLEQUOTE;

	// The following key codes won't be returned by FsInkey()
	// These may return non zero for FsGetKeyState
	//FSKEYtoTownsKEY[RETROK_LEFT_CTRL] = TOWNS_JISKEY_CTRL;
	//FSKEYtoTownsKEY[RETROK_RIGHT_CTRL] = TOWNS_JISKEY_CTRL;
	//FSKEYtoTownsKEY[RETROK_LEFT_SHIFT] = TOWNS_JISKEY_SHIFT;
	//FSKEYtoTownsKEY[RETROK_RIGHT_SHIFT] = TOWNS_JISKEY_SHIFT;
	//FSKEYtoTownsKEY[RETROK_LEFT_ALT] = TOWNS_JISKEY_NULL;
	//FSKEYtoTownsKEY[RETROK_RIGHT_ALT] = TOWNS_JISKEY_NULL;
}

/* virtual */ void FsRetroArchConnection::CDDAPlay(const DiscImage &discImg, DiscImage::MinSecFrm from, DiscImage::MinSecFrm to, bool repeat, unsigned int leftLevel, unsigned int rightLevel)
{
	if (256<leftLevel)
	{
		leftLevel = 256;
	}
	if (256<rightLevel)
	{
		rightLevel = 256;
	}
	auto wave = discImg.GetWave(from, to);
	if (leftLevel<256 || rightLevel<256)
	{
		for (long long int i = 0; i + 3<wave.size(); i += 4)
		{
			int left = cpputil::GetWord(wave.data() + i);
			int right = cpputil::GetWord(wave.data() + i + 2);;
			left = (left & 0x7FFF) - (left & 0x8000);
			right = (right & 0x7FFF) - (right & 0x8000);
			left = left * leftLevel / 256;
			right = right * rightLevel / 256;
			cpputil::PutWord(wave.data() + i, left);
			cpputil::PutWord(wave.data() + i + 2, right);
		}
	}
	cddaChannel.CreateFromSigned16bitStereo(44100, wave);
	if (true == repeat)
	{
		soundPlayer.PlayBackground(cddaChannel);
	}
	else
	{
		soundPlayer.PlayOneShot(cddaChannel);
	}
	cddaStartHSG = from.ToHSG();
}
/* virtual */ void FsRetroArchConnection::CDDAStop(void)	{	soundPlayer.Stop(cddaChannel);	}
/* virtual */ void FsRetroArchConnection::CDDAPause(void)	{	soundPlayer.Pause(cddaChannel);	}
/* virtual */ void FsRetroArchConnection::CDDAResume(void)	{	soundPlayer.Resume(cddaChannel);}

/* virtual */ bool FsRetroArchConnection::CDDAIsPlaying(void){	return (YSTRUE == soundPlayer.IsPlaying(cddaChannel)); }
/* virtual */ DiscImage::MinSecFrm FsRetroArchConnection::CDDACurrentPosition(void) {
	double sec = soundPlayer.GetCurrentPosition(cddaChannel);
	unsigned long long secHSG = (unsigned long long)(sec*75.0);
	unsigned long long posInDisc = secHSG + cddaStartHSG;

	DiscImage::MinSecFrm msf;
	msf.FromHSG(posInDisc);
	return msf;
}
/*
 void FsRetroArchConnection::PCMPlay(std::vector <unsigned char > &wave)
{
	PCMChannel.CreateFromSigned16bitStereo(RF5C68::FREQ, wave);
	soundPlayer.PlayOneShot(PCMChannel);
}
void FsRetroArchConnection::PCMPlayStop(void) {	soundPlayer.Stop(PCMChannel);}
bool FsRetroArchConnection::PCMChannelPlaying(void){	return YSTRUE == soundPlayer.IsPlaying(PCMChannel);}

 void FsRetroArchConnection::FMPlay(std::vector <unsigned char> &wave)
{
	FMChannel.CreateFromSigned16bitStereo(YM2612::WAVE_SAMPLING_RATE, wave);

	// std::string fName;
	// fName="tone";
	// fName+=cpputil::Itoa(ch);
	// fName+=".wav";
	// auto waveFile=FMChannel[ch].MakeWavByteData();
	// cpputil::WriteBinaryFile(fName,waveFile.size(),waveFile.data());

	soundPlayer.PlayOneShot(FMChannel);
}
void FsRetroArchConnection::FMPlayStop(void) { }
bool FsRetroArchConnection::FMChannelPlaying(void) {	return YSTRUE == soundPlayer.IsPlaying(FMChannel); }
*/

/* virtual */ void FsRetroArchConnection::FMPCMPlay(std::vector <unsigned char> &wave)
{
	FMPCMChannel.CreateFromSigned16bitStereo(YM2612::WAVE_SAMPLING_RATE, wave);

	// std::string fName;
	// fName="tone";
	// fName+=cpputil::Itoa(ch);
	// fName+=".wav";
	// auto waveFile=FMChannel[ch].MakeWavByteData();
	// cpputil::WriteBinaryFile(fName,waveFile.size(),waveFile.data());

	soundPlayer.PlayOneShot(FMPCMChannel);
}
/* virtual */ void FsRetroArchConnection::FMPCMPlayStop(void)
{
}
/* virtual */ bool FsRetroArchConnection::FMPCMChannelPlaying(void)
{
	return YSTRUE == soundPlayer.IsPlaying(FMPCMChannel);
}


/*virtual*/ void FsRetroArchConnection::BeepPlay(int samplingRate, std::vector<unsigned char>& wave){ }
/*virtual*/ void FsRetroArchConnection::BeepPlayStop() { }
/*virtual*/ bool FsRetroArchConnection::BeepChannelPlaying() const  { return 0; }

void FsRetroArchConnection::Render(const TownsRender::Image &img, const class FMTowns &towns) {
// compatibilty sake.
	
}



//*****************************************
//***			Outside world 			***
//***			( end )					***
//*****************************************



//*****************************************
//***			Setup					***
//***		( call in retro_load()  )	***
//*****************************************
bool Setup(FMTowns &towns,Outside_World *outside_world,const TownsARGV &argv)
{
	if(""==argv.ROMPath)
	{
		std::cout << "Usage:" << std::endl;
		std::cout << "  Tsugaru_CUI rom_directory_name" << std::endl;
		std::cout << "or," << std::endl;
		std::cout << "  Tsugaru_CUI -HELP" << std::endl;
		return false;
	}

	if(true!=towns.LoadROMImages(argv.ROMPath.c_str()))
	{
		std::cout << towns.vmAbortReason << std::endl;
		return false;
	}
	for(int drv=0; drv<2; ++drv)
	{
		if(""!=argv.fdImgFName[drv])
		{
			towns.fdc.LoadD77orRAW(drv,argv.fdImgFName[drv].c_str());
			if(true==argv.fdImgWriteProtect[drv])
			{
				// D77 image may have write-protect switch.
				// Enable write protect only when specified by the parameter.
				towns.fdc.SetWriteProtect(drv,true);
			}
		}
	}

	if(TOWNSTYPE_UNKNOWN!=argv.townsType)
	{
		towns.townsType=argv.townsType;
		towns.physMem.SetUpMemoryAccess(towns.TownsTypeToCPUType(argv.townsType));
	}

	if(""!=argv.cdImgFName)
	{
		auto errCode=towns.cdrom.state.GetDisc().Open(argv.cdImgFName);
		if(DiscImage::ERROR_NOERROR!=errCode)
		{
			std::cout << DiscImage::ErrorCodeToText(errCode);
		}
	}

	for(auto &scsi : argv.scsiImg)
	{
		if(scsi.imageType==TownsARGV::SCSIIMAGE_HARDDISK)
		{
			if(true!=towns.scsi.LoadHardDiskImage(scsi.scsiID,scsi.imgFName))
			{
				std::cout << "Failed to load hard disk image." << std::endl;
			}
		}
		else if(scsi.imageType==TownsARGV::SCSIIMAGE_CDROM)
		{
			if(true!=towns.scsi.LoadCDImage(scsi.scsiID,scsi.imgFName))
			{
				std::cout << "Failed to load SCSI CD image." << std::endl;
			}
		}
	}

	if(0!=argv.freq)
	{
		towns.state.freq=argv.freq;
	}

	if(0!=argv.memSizeInMB)
	{
		int megabyte=argv.memSizeInMB;
		if(64<megabyte)
		{
			megabyte=64;
		}
		towns.SetMainRAMSize(megabyte*1024*1024);
	}

	if(0<argv.symbolFName.size())
	{
		towns.debugger.GetSymTable().fName=argv.symbolFName;
		if(true==towns.debugger.GetSymTable().Load(argv.symbolFName.c_str()))
		{
			std::cout << "Loaded Symbol Table: " << argv.symbolFName << std::endl;
		}
		else
		{
			std::cout << "Failed to Load Symbol Table: " << argv.symbolFName << std::endl;
			std::cout << "Will create a new file." << argv.symbolFName << std::endl;
		}
	}

	if(0<argv.playbackEventLogFName.size())
	{
		if(true==towns.eventLog.LoadEventLog(argv.playbackEventLogFName))
		{
			towns.eventLog.BeginPlayback();
		}
	}


	towns.var.freeRunTimerShift=0;

	if(true==argv.autoSaveCMOS)
	{
		towns.var.CMOSFName=argv.CMOSFName;
	}
	else
	{
		towns.var.CMOSFName="";
	}
	if(0<argv.CMOSFName.size())
	{
		auto CMOSBinary=cpputil::ReadBinaryFile(argv.CMOSFName);
		if(0<CMOSBinary.size())
		{
			towns.physMem.SetCMOS(CMOSBinary);
		}
	}
	if(""!=argv.keyMapFName)
	{
		std::ifstream ifp(argv.keyMapFName);
		if(ifp.is_open())
		{
			std::vector <std::string> text;
			while(true!=ifp.eof())
			{
				std::string str;
				std::getline(ifp,str);
				text.push_back(str);
			}
			outside_world->LoadKeyMappingFromText(text);
		}
		else
		{
			std::cout << "Cannot load key-mapping file." << std::endl;
			return false;
		}
	}

	towns.keyboard.SetBootKeyCombination(argv.bootKeyComb);
	towns.gameport.SetBootKeyCombination(argv.bootKeyComb);

	towns.state.mouseIntegrationSpeed=argv.mouseIntegrationSpeed;
	towns.state.noWait=argv.noWait;
	towns.state.pretend386DX=argv.pretend386DX;
	towns.var.noWaitStandby=argv.noWaitStandby;
	towns.state.appSpecificSetting=argv.appSpecificSetting;

	towns.var.catchUpRealTime=argv.catchUpRealTime;

	if(0<=argv.fmVol)
	{
		towns.sound.state.ym2612.state.volume=argv.fmVol;
	}
	if(0<=argv.pcmVol)
	{
		towns.sound.state.rf5c68.state.volume=argv.fmVol;
	}

	if(true==argv.powerOffAtBreakPoint)
	{
		towns.var.powerOffAt.MakeFromString(argv.powerOffAt);
		towns.debugger.AddBreakPoint(towns.var.powerOffAt);
	}

	std::cout << "Loaded ROM Images.\n";

	towns.Reset();
	towns.physMem.takeJISCodeLog=true;

	std::cout << "Virtual Machine Reset.\n";

	for(int i=0; i<TOWNS_NUM_GAMEPORTS; ++i)
	{
		outside_world->gamePort[i]=argv.gamePort[i];
		switch(argv.gamePort[i])
		{
		case TOWNS_GAMEPORTEMU_NONE:
			towns.gameport.state.ports[i].device=TownsGamePort::NONE;
			break;
		case TOWNS_GAMEPORTEMU_MOUSE:
		case TOWNS_GAMEPORTEMU_MOUSE_BY_KEY:
		case TOWNS_GAMEPORTEMU_MOUSE_BY_NUMPAD:
		case TOWNS_GAMEPORTEMU_MOUSE_BY_PHYSICAL0:
		case TOWNS_GAMEPORTEMU_MOUSE_BY_PHYSICAL1:
		case TOWNS_GAMEPORTEMU_MOUSE_BY_PHYSICAL2:
		case TOWNS_GAMEPORTEMU_MOUSE_BY_PHYSICAL3:
		case TOWNS_GAMEPORTEMU_MOUSE_BY_PHYSICAL4:
		case TOWNS_GAMEPORTEMU_MOUSE_BY_PHYSICAL5:
		case TOWNS_GAMEPORTEMU_MOUSE_BY_PHYSICAL6:
		case TOWNS_GAMEPORTEMU_MOUSE_BY_PHYSICAL7:
		case TOWNS_GAMEPORTEMU_MOUSE_BY_ANALOG0:
		case TOWNS_GAMEPORTEMU_MOUSE_BY_ANALOG1:
		case TOWNS_GAMEPORTEMU_MOUSE_BY_ANALOG2:
		case TOWNS_GAMEPORTEMU_MOUSE_BY_ANALOG3:
		case TOWNS_GAMEPORTEMU_MOUSE_BY_ANALOG4:
		case TOWNS_GAMEPORTEMU_MOUSE_BY_ANALOG5:
		case TOWNS_GAMEPORTEMU_MOUSE_BY_ANALOG6:
		case TOWNS_GAMEPORTEMU_MOUSE_BY_ANALOG7:
			towns.gameport.state.ports[i].device=TownsGamePort::MOUSE;
			break;
		case TOWNS_GAMEPORTEMU_PHYSICAL0_AS_CYBERSTICK:
		case TOWNS_GAMEPORTEMU_PHYSICAL1_AS_CYBERSTICK:
		case TOWNS_GAMEPORTEMU_PHYSICAL2_AS_CYBERSTICK:
		case TOWNS_GAMEPORTEMU_PHYSICAL3_AS_CYBERSTICK:
		case TOWNS_GAMEPORTEMU_PHYSICAL4_AS_CYBERSTICK:
		case TOWNS_GAMEPORTEMU_PHYSICAL5_AS_CYBERSTICK:
		case TOWNS_GAMEPORTEMU_PHYSICAL6_AS_CYBERSTICK:
		case TOWNS_GAMEPORTEMU_PHYSICAL7_AS_CYBERSTICK:
			towns.gameport.state.ports[i].device=TownsGamePort::CYBERSTICK;
			break;
		default:
			towns.gameport.state.ports[i].device=TownsGamePort::GAMEPAD;
			break;
		}
	}

	for(auto i=0; i<argv.sharedDir.size() && i<TownsVnDrv::MAX_NUM_SHARED_DIRECTORIES; ++i)
	{
		towns.vndrv.sharedDir[i].hostPath=argv.sharedDir[i];
		towns.vndrv.sharedDir[i].linked=true;
	}

	towns.crtc.state.highResAvailable=argv.highResAvailable;

	if(TOWNS_KEYBOARD_MODE_DEFAULT!=argv.keyboardMode)
	{
		outside_world->keyboardMode=argv.keyboardMode;
	}
	for(auto vk : argv.virtualKeys)
	{
		auto townsKey=TownsStrToKeyCode(vk.townsKey);
		if(TOWNS_JISKEY_NULL==townsKey)
		{
			townsKey=TownsStrToKeyCode("TOWNS_JISKEY_"+vk.townsKey);
		}
		if(TOWNS_JISKEY_NULL!=townsKey)
		{
			outside_world->AddVirtualKey(townsKey,vk.physicalId,vk.button);
		}
	}

	if(TOWNS_MEMCARD_TYPE_NONE!=argv.memCardType)
	{
		if(true==towns.physMem.state.memCard.LoadRawImage(argv.memCardImgFName))
		{
			towns.physMem.state.memCard.memCardType=argv.memCardType;
			towns.physMem.state.memCard.fName=argv.memCardImgFName;
			towns.physMem.state.memCard.changed=false;  // Because it was already in upon power-on.
		}
	}

	towns.var.damperWireLine=argv.damperWireLine;
	towns.var.scanLineEffectIn15KHz=argv.scanLineEffectIn15KHz;

	outside_world->strikeCommanderThrottlePhysicalId=argv.strikeCommanderThrottlePhysicalId;
	outside_world->strikeCommanderThrottleAxis=argv.strikeCommanderThrottleAxis;

	outside_world->scaling=argv.scaling;
	outside_world->windowShift=argv.windowShift;
	outside_world->autoScaling=argv.autoScaling;
	outside_world->maximizeOnStartUp=argv.maximizeOnStartUp;

	outside_world->mouseByFlightstickAvailable=argv.mouseByFlightstickAvailable;
	outside_world->mouseByFlightstickPhysicalId=argv.mouseByFlightstickPhysicalId;
	outside_world->mouseByFlightstickZeroZoneX=argv.mouseByFlightstickZeroZoneX;
	outside_world->mouseByFlightstickZeroZoneY=argv.mouseByFlightstickZeroZoneY;
	outside_world->mouseByFlightstickCenterX=argv.mouseByFlightstickCenterX;
	outside_world->mouseByFlightstickCenterY=argv.mouseByFlightstickCenterY;
	outside_world->mouseByFlightstickScaleX=argv.mouseByFlightstickScaleX;
	outside_world->mouseByFlightstickScaleY=argv.mouseByFlightstickScaleY;

	outside_world->CacheGamePadIndicesThatNeedUpdates();

	return true;
}

//*****************************************
//***			Setup					***
//***		   ( end  )					***
//*****************************************



//*****************************************
//***			Render Stuff			***
//***		( call from retro_run()  )	***
//***		from townsrenderthread.cpp  ***
//*****************************************

void CheckRenderingTimer(FMTowns &towns, TownsRender &render)
{
	if (STATE_IDLE == state &&
		towns.var.nextRenderingTime <= towns.state.townsTime &&
		true != towns.crtc.InVSYNC(towns.state.townsTime))
	{
		render.Prepare(towns.crtc);
		render.damperWireLine = towns.var.damperWireLine;
		render.scanLineEffectIn15KHz = towns.var.scanLineEffectIn15KHz;
		rendererPtr = &render;
		memcpy(VRAMCopy, towns.physMem.state.VRAM.data(), towns.crtc.GetEffectiveVRAMSize());
		paletteCopy = towns.crtc.GetPalette();
		chaseHQPaletteCopy = towns.crtc.chaseHQPalette;

		state = STATE_RENDERING;
		towns.var.nextRenderingTime = towns.state.townsTime + (TOWNS_RENDERING_FREQUENCY);

		checkImageAfterThisTIme = towns.state.townsTime + 3000000; // Give sub-thread some time. - 3000000

		command = RENDER;
		imageReady = false;

	}
}


void CheckImageReady(FMTowns &towns, Outside_World &world)
{
	if (STATE_RENDERING == state && checkImageAfterThisTIme<towns.state.townsTime)
	{

		// from townsrenderthread.cpp
		if (RENDER == command)
		{
			rendererPtr->BuildImage(VRAMCopy, paletteCopy, chaseHQPaletteCopy);
			command = NO_COMMAND;
			imageReady = true;
		}

		if (true == imageReady)
		{
			TownsRender::Image tt = rendererPtr->GetImage();

			//RGBA -> BGRA
			for (int i = 0; i < tt.hei * tt.wid * 4; i=i+4) {
				unsigned char v;
				v = *(tt.rgba + i);
				*((char *)tt.rgba + i) = *(tt.rgba + i + 2);
				*((char *)tt.rgba + i + 2) = v;
			}


			//  Not so bad - Only displayed when accessed.
			world.UpdateStatusBitmap(towns);
			if (world.cdAccessLamp || world.fdAccessLamp[0] || world.fdAccessLamp[1])
			{
				//copy everything
				//memcpy((char*)tt.rgba + (640 * 4 * (480 - 16)), world.statusBitmap, sizeof(char) * 640 * 4 * 16);

				//copy only first 144pixel of each line
				for (int i = 0; i < 16; i++)
					memcpy( (char*)tt.rgba + (640 * 4 * (480 - 16)) + (i * 640 * 4),  (char*)world.statusBitmap + (i * 640 * 4), sizeof(char) * 144 * 4 );
			}

			// main video ram
			video_cb(tt.rgba, tt.wid, tt.hei, tt.wid * 4);

			state = STATE_IDLE;
		}
	}
}
//*****************************************
//***			Render Stuff			***
//***			  ( end  )				***
//*****************************************




//*****************************************
//***			AdjustRealTime			***
//***		( call from retro_run()  )	***
//*****************************************

void AdjustRealTime(FMTowns *townsPtr, long long int cpuTimePassed, std::chrono::time_point<std::chrono::high_resolution_clock> time0, Outside_World *outside_world)
{
	long long int realTimePassed = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - time0).count();

	townsPtr->var.timeAdjustLog[townsPtr->var.timeAdjustLogPtr] = cpuTimePassed - realTimePassed;
	townsPtr->var.timeAdjustLogPtr = (townsPtr->var.timeAdjustLogPtr + 1)&(FMTowns::Variable::TIME_ADJUSTMENT_LOG_LEN - 1);

	if (cpuTimePassed<realTimePassed) // VM lagging
	{
		// Just record the time deficit here.
		// In the next cycle, VM timer will be fast-forwarded 512 nanoseconds per instruction
		// and 512 is subtracted from the deficit until deficit becomes zero.
		if (true == townsPtr->var.catchUpRealTime)
		{
			townsPtr->state.timeDeficit = (realTimePassed - cpuTimePassed)&(~(FMTowns::State::CATCHUP_PER_INSTRUCTION - 1));
		}
		else
		{
			townsPtr->state.timeDeficit = 0;
		}
	}
	else
	{
		if (true != townsPtr->state.noWait)
		{
			while (realTimePassed<cpuTimePassed)
			{
				realTimePassed = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - time0).count();
				townsPtr->ProcessSound(outside_world);
			}
		}
		townsPtr->state.timeDeficit = 0;
	}
}
//*****************************************
//***			AdjustRealTime			***
//***				( end  )			***
//*****************************************




//*****************************************
//***			libretro	 			***
//***				     				***
//*****************************************

RETRO_API void retro_set_environment(retro_environment_t env)
{
	environment_cb = env;
	env(RETRO_ENVIRONMENT_SET_VARIABLES, options);
}

RETRO_API void retro_set_video_refresh(retro_video_refresh_t vr)
{
	video_cb = vr;
}

RETRO_API void retro_set_audio_sample(retro_audio_sample_t sample)
{
	audio_cb = sample;
}

RETRO_API void retro_set_audio_sample_batch(retro_audio_sample_batch_t batch)
{
	audio_batch_cb = batch;
}

RETRO_API void retro_set_input_poll(retro_input_poll_t ipoll)
{
	input_poll_cb = ipoll;
}

RETRO_API void retro_set_input_state(retro_input_state_t istate)
{
	input_state_cb = istate;
}




RETRO_API void retro_init(void) {  }

RETRO_API void retro_deinit(void) { }

RETRO_API unsigned retro_api_version(void) { return RETRO_API_VERSION; }

RETRO_API void retro_get_system_info(struct retro_system_info *info)
{
	info->library_name = "Tsugaru";
	info->library_version = "v.20210118";
	info->valid_extensions = "iso|cue";
	info->need_fullpath = true;
	info->block_extract = false;
}

RETRO_API void retro_get_system_av_info(struct retro_system_av_info *info)
{
	
	info->geometry.base_width = 640;
	info->geometry.base_height = 480;
	info->geometry.max_width = 640;
	info->geometry.max_height = 480;
	info->geometry.aspect_ratio = 0.0f;

	info->timing.fps = 60;
	info->timing.sample_rate = 20725;		// taken from tugaru sound somewhere ( don't remember )
}

RETRO_API void retro_set_controller_port_device(unsigned port, unsigned device) { }

RETRO_API void retro_reset(void)
{
	retro_unload_game();
	retro_load_game(&game_info);
}

RETRO_API void retro_run(void)
{
	auto realTime0 = std::chrono::high_resolution_clock::now();
	auto cpuTime0 = towns.state.cpuTime;

	int runModeCopy = 0;

	runModeCopy = runMode;

	bool clockTicking = false;  // Will be made true if VM is running.

	switch (runMode)
	{
	case RUNMODE_PAUSE:
		towns.ForceRender(render, *outside_world);
		outside_world->DevicePolling(towns);
		if (true == outside_world->PauseKeyPressed())
		{
			runMode = RUNMODE_RUN;
			towns.debugger.stop = false;
		}
		break;
	case RUNMODE_RUN:
		clockTicking = true;
		{
			auto nextTimeSync = towns.state.cpuTime + NANOSECONDS_PER_TIME_SYNC;
			while (towns.state.cpuTime<nextTimeSync && true != towns.CheckAbort())
			{
				towns.RunOneInstruction();
				towns.pic.ProcessIRQ(towns.cpu, towns.mem);
				towns.RunFastDevicePolling();
				towns.RunScheduledTasks();
				if (true == towns.debugger.stop)
				{
					if (towns.cpu.state.CS().value == towns.var.powerOffAt.SEG &&
						towns.cpu.state.EIP == towns.var.powerOffAt.OFFSET)
					{
						std::cout << "Break at the power-off point." << std::endl;
						std::cout << "Normal termination of a unit testing." << std::endl;
						towns.var.powerOff = true;
						break;
					}
					towns.PrintStatus();
					std::cout << ">";
					runMode = RUNMODE_PAUSE;
					break;
				}
			}
		}
		//audio
		towns.ProcessSound(outside_world);
		towns.cdrom.UpdateCDDAState(towns.state.townsTime, *outside_world);

		//video
		CheckRenderingTimer(towns, render);
		CheckImageReady(towns, *outside_world);

		//devices
		outside_world->ProcessAppSpecific(towns);
		outside_world->DevicePolling(towns);		
		towns.eventLog.Interval(towns);

		if (true == towns.CheckAbort() || outside_world->PauseKeyPressed())
		{
			towns.PrintStatus();
			std::cout << ">";
			runMode = RUNMODE_PAUSE;
		}

		break;
	case RUNMODE_ONE_INSTRUCTION:
		if (true != towns.CheckAbort())
		{
			towns.RunOneInstruction();
			towns.pic.ProcessIRQ(towns.cpu, towns.mem);
			towns.RunFastDevicePolling();
			towns.RunScheduledTasks();
		}
		towns.PrintStatus();
		std::cout << ">";
		runMode = RUNMODE_PAUSE;
		break;


	default:
		std::cout << "Undefined VM RunMode!" << std::endl;
		break;
	}

	if (true == towns.var.powerOff)
	{
		if (true != towns.var.pauseOnPowerOff)
		{
			runMode = RUNMODE_EXIT;
		}
		else if (RUNMODE_PAUSE != runMode)
		{
			towns.PrintStatus();
			std::cout << ">";
			runMode = RUNMODE_PAUSE;
		}
	}

	if (towns.state.nextSecondInTownsTime <= towns.state.townsTime)
	{
		towns.state.nextSecondInTownsTime += PER_SECOND;
		towns.fdc.SaveModifiedDiskImages();
		towns.physMem.state.memCard.SaveRawImageIfModified();
	}

	if (RUNMODE_PAUSE == runModeCopy)
	{
		towns.fdc.SaveModifiedDiskImages();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}



	if (true == clockTicking)
	{
		AdjustRealTime(&towns, towns.state.cpuTime - cpuTime0, realTime0, outside_world);
	}


}

RETRO_API size_t retro_serialize_size(void) { return 0; }

RETRO_API bool retro_serialize(void *data, size_t size) { return false; }

RETRO_API bool retro_unserialize(const void *data, size_t size) { return false; }

RETRO_API void retro_cheat_reset(void) { }

RETRO_API void retro_cheat_set(unsigned index, bool enabled, const char *code){ }


RETRO_API bool retro_load_game(const struct retro_game_info *game)
{

	enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
	if (!environment_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
	{
		printf("XRGB8888 is not supported\n");
		return false;
	}

	char szCD[512];

	char szGamePad0[64];
	char szGamePad1[64];
	char szAppSpecific[64];
	char szKeyboard[64];

	//Check needed variables
	struct retro_variable var = { 0 };
	var.key = "gameport_0";					// { "gameport_0", "Select Gameport 0 type; gamepad|mouse|key|none" },
	environment_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var);
	if (!strcmp(var.value, "gamepad")) strcpy_s(szGamePad0, "PHYS0");
	if (!strcmp(var.value, "mouse")) strcpy_s(szGamePad0, "MOUSE");
	if (!strcmp(var.value, "key")) strcpy_s(szGamePad0, "KEY");
	if (!strcmp(var.value, "none")) strcpy_s(szGamePad0, "NONE");


	var.key = "gameport_1";					// { "gameport_1", "Select Gameport 0 type; gamepad|mouse|key|none" },
	environment_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var);
	if (!strcmp(var.value, "gamepad")) strcpy_s(szGamePad1, "PHYS1");
	if (!strcmp(var.value, "mouse")) strcpy_s(szGamePad1, "MOUSE");
	if (!strcmp(var.value, "key")) strcpy_s(szGamePad1, "KEY");
	if (!strcmp(var.value, "none")) strcpy_s(szGamePad1, "NONE");

	var.key = "app_specific";					
	environment_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var);
	if (!strcmp(var.value, "WINGCOMMANDER1")) strcpy_s(szAppSpecific, "WINGCOMMANDER1");
	if (!strcmp(var.value, "WINGCOMMANDER2")) strcpy_s(szAppSpecific, "WINGCOMMANDER2");
	if (!strcmp(var.value, "SUPERDAISEN")) strcpy_s(szAppSpecific, "SUPERDAISEN");
	if (!strcmp(var.value, "LEMMINGS")) strcpy_s(szAppSpecific, "LEMMINGS");
	if (!strcmp(var.value, "LEMMINGS2")) strcpy_s(szAppSpecific, "LEMMINGS2");
	if (!strcmp(var.value, "STRIKECOMMANDER")) strcpy_s(szAppSpecific, "STRIKECOMMANDER");
	if (!strcmp(var.value, "AMARANTH3")) strcpy_s(szAppSpecific, "AMARANTH3");
	if (!strcmp(var.value, "ULTIMAUNDERWORLD")) strcpy_s(szAppSpecific, "ULTIMAUNDERWORLD");
	if (!strcmp(var.value, "none")) strcpy_s(szAppSpecific, "NONE");

	var.key = "keyboard";					//"keyboard" - "-KEYBOARD"   - DIRECT , TRANS , TRANS2 , TRANS3
	environment_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var);
	if (!strcmp(var.value, "DIRECT")) strcpy_s(szKeyboard, "DIRECT");
	if (!strcmp(var.value, "TRANS")) strcpy_s(szKeyboard, "TRANS");
	if (!strcmp(var.value, "TRANS2")) strcpy_s(szKeyboard, "TRANS2");
	if (!strcmp(var.value, "TRANS3")) strcpy_s(szKeyboard, "TRANS3");


	// Creating run parameters string
	sprintf_s(szCD, "%s", game->path);

	// Find UserDisk 
	char* szUserDiskTemplate[] = {	"[UserDisk].hdm",	"[UserDisk].bin" ,	"[UserDisk].D88", "[UserDisk].xdf", " (User disk).hdm",
									"[BootDisk].hdm",	"[BootDisk].bin",	"[BootDisk].D88", " (Boot disk).hdm",
									"[SystemDisk].hdm", "[SystemDisk].bin", "[SystemDisk].D88", " (System disk).hdm",
									"[GameDisk].hdm",	"[GameDisk].bin",	"[GameDisk].D88", " (Game disk).hdm",
									"[SaveDisk].hdm",	"[SaveDisk].bin",	"[SaveDisk].D88",
									
									//powermonger ?
									"[BootDisk-En].bin" 
	
	}; 
	int nbUserDiskTemplate = 21;

	char szUserDisk[512];
	char bFindUserDisk = false;
	auto  withoutExt = cpputil::RemoveExtension(game->path);
	for (int j = 0; j < nbUserDiskTemplate; j++)
	{
		
		sprintf_s(szUserDisk, "%s%s", withoutExt.c_str(), szUserDiskTemplate[j]);
		

		if (cpputil::FileExists(szUserDisk))
		{
			bFindUserDisk = true;
			//sprintf_s(szCD, "%s -FD0 %s", game->path, szUserDisk);
			printf("UserDisk : %s\n", szUserDisk);
			break;
		}

	}

	//Find a save disk
	char szSaveDisk[512];
	char bFindSaveDisk = false;
	sprintf_s(szSaveDisk, "%s%s", withoutExt.c_str(), "]SaveDisk[.hdm");
	if (cpputil::FileExists(szSaveDisk))
	{
		bFindSaveDisk = true;
		printf("SaveDisk : %s\n", szSaveDisk);
		
	}
	else
	{
		//Generate a SaveDisk
		std::vector <unsigned char> img;
		img = Get1232KBFloppyDiskImage();


		if (true != cpputil::WriteBinaryFile(szSaveDisk, img.size(), img.data()))
		{
			std::cout << "Failed to write file: " << szSaveDisk << std::endl;
			bFindSaveDisk = false;
		}
		else
		{
			std::cout << "Created FD Image: " << szSaveDisk << std::endl;
			bFindSaveDisk = true;
		}


	}

	//If Generated SaveDisk exist and there is no other Floppy, 
	// load Generated SaveDisk in FD0
	//else load Generated SaveDisk in FD1
	if ((bFindSaveDisk == true) && (bFindUserDisk == false))
	{
		strcpy(szUserDisk, szSaveDisk);
		strcpy(szSaveDisk, "NULL");

	}


	int ac = 18;
	char *av[] = { "","./system", "-SCALE" , "100" , "-GAMEPORT0",  szGamePad0, "-GAMEPORT1",  szGamePad1, "-APP", szAppSpecific, "-KEYBOARD",szKeyboard,  "-CD" , szCD ,"-FD0", szUserDisk ,"-FD1", szSaveDisk };


	
	// Check parameters string
	if (true != argv.AnalyzeCommandParameter(ac, av))
	{
		return false;
	}


	//workaround for the issue : no sound when loading another roms
	if (outside_world != NULL)
		outside_world->Stop();


	outside_world = new FsRetroArchConnection;		// class must be in own file

	// Create / Init
	if (true != Setup(towns, outside_world, argv))
	{
		return false;
	}

	// Backup game info for retro_reset()
	memcpy(&game_info, game, sizeof(retro_game_info));

	// Force some settings
	argv.autoStart = false;
	argv.debugger = false;
	argv.interactive = false;
	runMode = RUNMODE_RUN;

	//
	towns.cdrom.SetOutsideWorld(outside_world);
	towns.sound.SetOutsideWorld(outside_world);
	towns.scsi.SetOutsideWorld(outside_world);
	outside_world->Start();

	return true;
}

RETRO_API bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info)
{
	return false;
}

RETRO_API void retro_unload_game(void)
{

	std::cout << "Ending Towns Thread." << std::endl;
	towns.fdc.SaveModifiedDiskImages();

	if (0<towns.var.CMOSFName.size())
	{
		if (true != cpputil::WriteBinaryFile(towns.var.CMOSFName, TOWNS_CMOS_SIZE, towns.physMem.state.CMOSRAM))
		{
			std::cout << "Failed to save CMOS." << std::endl;
		}
	}

	outside_world->Stop();
	



}

RETRO_API unsigned retro_get_region(void) { return RETRO_REGION_NTSC; }

RETRO_API void *retro_get_memory_data(unsigned id) { return NULL; }

RETRO_API size_t retro_get_memory_size(unsigned id) { return 0; }