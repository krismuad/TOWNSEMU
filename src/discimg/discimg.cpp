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
#include <vector>
#include <unordered_map>

#include "discimg.h"
#include "cpputil.h"



// Uncomment for verbose output.
// #define DEBUG_DISCIMG

/* static */ DiscImage::MinSecFrm DiscImage::MinSecFrm::Zero(void)
{
	MinSecFrm MSF;
	MSF.min=0;
	MSF.sec=0;
	MSF.frm=0;
	return MSF;
}
/* static */ DiscImage::MinSecFrm DiscImage::MinSecFrm::TwoSeconds(void)
{
	MinSecFrm MSF;
	MSF.min=0;
	MSF.sec=2;
	MSF.frm=0;
	return MSF;
}


////////////////////////////////////////////////////////////

static std::string GetExtension(const std::string &fName)
{
	auto pos = fName.rfind('.');
	if (pos == std::string::npos) {
		return "";
	}

	return fName.substr(pos);
}
static void Capitalize(std::string &str)
{
	for(auto &c : str)
	{
		if('a'<=c && c<='z')
		{
			c+=('A'-'a');
		}
	}
}


////////////////////////////////////////////////////////////


DiscImage::DiscImage()
{
	CleanUp();
}
/* static */ const char *DiscImage::ErrorCodeToText(unsigned int errCode)
{
	switch(errCode)
	{
	case ERROR_NOERROR:
		return "No error";
	case ERROR_UNSUPPORTED:
		return "Unsupported file format.";
	case ERROR_CANNOT_OPEN:
		return "Cannot open image file.";
	case ERROR_NOT_YET_SUPPORTED:
		return "File format not yet supported. (I'm working on it!)";
	case ERROR_SECTOR_SIZE:
		return "Binary size is not integer multiple of the sector length.";
	case ERROR_TOO_FEW_ARGS:
		return "Too few arguments.";
	case ERROR_UNSUPPORTED_TRACK_TYPE:
		return "Unsupported track type.";
	case ERROR_SECTOR_LENGTH_NOT_GIVEN:
		return "Sector length of the data track is not given.";
	case ERROR_TRACK_INFO_WITHOTU_TRACK:
		return "Track information is given without a track.";
	case ERROR_INCOMPLETE_MSF:
		return "Incomplete MM:SS:FF.";
	case ERROR_BINARY_FILE_NOT_FOUND:
		return "Image binary file not found.";
	case ERROR_FIRST_TRACK_NOT_STARTING_AT_00_00_00:
		return "First track not starting at 00:00:00";
	case ERROR_BINARY_SIZE_NOT_SECTOR_TIMES_INTEGER:
		return "Binary size is not integer multiple of sector lengths.";
	case ERROR_NUM_MULTI_BIN_NOT_EQUAL_TO_NUM_TRACKS:
		return "Number of binary files and the number of tracks in multi-bin CUE file do not match.";
	case ERROR_MULTI_BIN_DATA_NOT_2352_PER_SEC:
		return "All tracks must use 2352 bytes/sector in multi-bin CUE file.";
	}
	return "Undefined error.";
}
void DiscImage::CleanUp(void)
{
	fileType=FILETYPE_NONE;
	totalBinLength=0;
	fName="";
	num_sectors=0;
	binaries.clear();
	tracks.clear();
	layout.clear();
}
unsigned int DiscImage::Open(const std::string &fName)
{
	auto ext=GetExtension(fName);
	Capitalize(ext);
	if(".CUE"==ext)
	{
		return OpenCUE(fName);
	}
	if(".BIN"==ext)
	{
		auto withoutExt=cpputil::RemoveExtension(fName.c_str());
		auto cue=withoutExt+".cue";
		auto CUE=withoutExt+".CUE";
		auto Cue=withoutExt+".Cue";
		if(cpputil::FileExists(cue))
		{
			return OpenCUE(cue);
		}
		if(cpputil::FileExists(CUE))
		{
			return OpenCUE(CUE);
		}
		if(cpputil::FileExists(Cue))
		{
			return OpenCUE(Cue);
		}
	}
	if(".ISO"==ext)
	{
		return OpenISO(fName);
	}
	return ERROR_UNSUPPORTED;
}
unsigned int DiscImage::OpenCUE(const std::string &fName)
{
	std::ifstream ifp;
	ifp.open(fName);
	if(true!=ifp.is_open())
	{
		return ERROR_CANNOT_OPEN;
	}

	CleanUp();
	this->fName=fName;

	// https://en.wikipedia.org/wiki/Cue_sheet_(computing)
	enum
	{
		CMD_FILE,
		CMD_TRACK,
		CMD_INDEX,
		CMD_PREGAP,
		CMD_POSTGAP,
	};
	std::unordered_map <std::string,int> cmdMap;
	cmdMap["FILE"]=CMD_FILE;
	cmdMap["TRACK"]=CMD_TRACK;
	cmdMap["INDEX"]=CMD_INDEX;
	cmdMap["PREGAP"]=CMD_PREGAP;
	cmdMap["POSTGAP"]=CMD_POSTGAP;

	while(true!=ifp.eof())
	{
		std::string line;
		std::getline(ifp,line);

		std::vector <std::string> argv=cpputil::Parser(line.c_str());
		if(0<argv.size())
		{
			cpputil::Capitalize(argv[0]);
			auto found=cmdMap.find(argv[0]);
			if(cmdMap.end()==found)
			{
				continue;
			}
			switch(found->second)
			{
			case CMD_FILE:
				if(2<=argv.size())
				{
					Binary bin;
					bin.fName=argv[1];
					binaries.push_back(bin);
				}
				break;
			case CMD_TRACK:
				if(3<=argv.size())
				{
					Track t;
					cpputil::Capitalize(argv[2]);
					if("AUDIO"==argv[2])
					{
						t.trackType=TRACK_AUDIO;
						t.sectorLength=2352;
					}
					else if(true==cpputil::StrStartsWith(argv[2],"MODE1"))
					{
						t.trackType=TRACK_MODE1_DATA;
						auto sectLenStr=cpputil::StrSkip(argv[2].c_str(),"/");
						if(nullptr!=sectLenStr)
						{
							t.sectorLength=cpputil::Atoi(sectLenStr);
						}
						else
						{
							return ERROR_SECTOR_LENGTH_NOT_GIVEN;
						}
					}
					tracks.push_back(t);
				}
				else
				{
					return ERROR_TOO_FEW_ARGS;
				}
				break;
			case CMD_INDEX:
				if(0==tracks.size())
				{
					return ERROR_TRACK_INFO_WITHOTU_TRACK;
				}
				if(3<=argv.size())
				{
					auto indexType=cpputil::Atoi(argv[1].c_str());
					MinSecFrm msf;
					if(true!=StrToMSF(msf,argv[2].c_str()))
					{
						return ERROR_INCOMPLETE_MSF;
					}
					if(0==indexType)
					{
						tracks.back().index00=msf;
					}
					else if(1==indexType)
					{
						tracks.back().start=msf;
					}
				}
				else
				{
					return ERROR_TOO_FEW_ARGS;
				}
				break;
			case CMD_PREGAP:
				if(0==tracks.size())
				{
					return ERROR_TRACK_INFO_WITHOTU_TRACK;
				}
				if(2<=argv.size())
				{
					MinSecFrm msf;
					if(true!=StrToMSF(msf,argv[1].c_str()))
					{
						return ERROR_INCOMPLETE_MSF;
					}
					tracks.back().preGap=msf;
				}
				else
				{
					return ERROR_TOO_FEW_ARGS;
				}
				break;
			case CMD_POSTGAP:
				if(0==tracks.size())
				{
					return ERROR_TRACK_INFO_WITHOTU_TRACK;
				}
				if(2<=argv.size())
				{
					MinSecFrm msf;
					if(true!=StrToMSF(msf,argv[1].c_str()))
					{
						return ERROR_INCOMPLETE_MSF;
					}
					tracks.back().postGap=msf;
				}
				else
				{
					return ERROR_TOO_FEW_ARGS;
				}
				break;
			default:
				std::cout << "Unrecognized: " << line << std::endl;
				break;
			}
		}
	}
	return OpenCUEPostProcess();
}
unsigned int DiscImage::OpenCUEPostProcess(void)
{
	if(0==tracks.size())
	{
		return ERROR_NOERROR;
	}
	if(MinSecFrm::Zero()!=tracks.front().start ||
	   MinSecFrm::Zero()!=tracks.front().preGap ||
	   MinSecFrm::Zero()!=tracks.front().index00)
	{
		return ERROR_FIRST_TRACK_NOT_STARTING_AT_00_00_00;
	}


	if(1<binaries.size()) // Multi-bin (1)
	{
		if(binaries.size()!=tracks.size())
		{
			// The number of binaries needs to match the number of tracks.
			return ERROR_NUM_MULTI_BIN_NOT_EQUAL_TO_NUM_TRACKS;
		}
		if(2352!=tracks[0].sectorLength)
		{
			// Non-2352 bytes/sec not supported in multi-bin format.
			return ERROR_MULTI_BIN_DATA_NOT_2352_PER_SEC;
		}
		// I have absolutely no idea if I am interpreting it right.
		// Multi-bin CUE files are so inconsistent.
		for(int trk=0; trk<tracks.size(); ++trk)
		{
			// I hope it is a correct interpretation.
			binaries[trk].bytesToSkip=2352*MSFtoHSG(tracks[trk].preGap+tracks[trk].start);
		}
	}


	uint64_t binLength=0;
	for(auto &bin : binaries)
	{
		std::vector <std::string> binFileCandidate;
		{
			std::string path,file;
			cpputil::SeparatePathFile(path,file,fName);
			binFileCandidate.push_back(path+bin.fName);
		}
		{
			std::string base=cpputil::RemoveExtension(fName.c_str());
			binFileCandidate.push_back(base+".BIN");
			binFileCandidate.push_back(base+".IMG");
			binFileCandidate.push_back(base+".bin");
			binFileCandidate.push_back(base+".img");
			binFileCandidate.push_back(base+".Bin");
			binFileCandidate.push_back(base+".Img");
		}
		bin.fName="";
		for(auto fn : binFileCandidate)
		{
			auto len=cpputil::FileSize(fn);
			if(0<len)
			{
				binLength+=len;
				binLength-=bin.bytesToSkip;
				bin.fName=fn;
				bin.fileSize=len;
				break;
			}
		}
	}
	if(0==binLength)
	{
		return ERROR_BINARY_FILE_NOT_FOUND;
	}
	this->totalBinLength=binLength;


	if(1<binaries.size()) // Multi-bin (2)
	{
		// Information in multi-bin CUE file is very erratic and unreliable.
		// I need to re-calculate based on the binary file sizes.
		// Whoever designed .CUE data format did a poor job by having PREGAP and INDEX 00.
		// INDEX 00 shouldn't have existed at all.  So confusing.
		unsigned int startSector=0;
		uint64_t locationInFile=0;
		for(int i=0; i<tracks.size(); ++i)
		{
			uint32_t numBytes=(binaries[i].fileSize-binaries[i].bytesToSkip);
			unsigned int numSec=numBytes/2352;
			tracks[i].start=HSGtoMSF(startSector);
			tracks[i].end=HSGtoMSF(startSector+numSec-1);
			tracks[i].locationInFile=locationInFile;
			binaries[i].byteOffsetInDisc=locationInFile;
			locationInFile+=numBytes;
			startSector+=numSec;
		}
	}

	if(1==tracks.size())
	{
		unsigned int numSec=(unsigned int)binLength/tracks[0].sectorLength;
		if(0!=(binLength%tracks[0].sectorLength))
		{
			return ERROR_BINARY_SIZE_NOT_SECTOR_TIMES_INTEGER;
		}
		--numSec;
		tracks[0].end=HSGtoMSF(numSec);
		tracks[0].locationInFile=0;
	}
	else
	{
		if(true!=TryAnalyzeTracksWithAbsurdCUEInterpretation() &&
		   true!=TryAnalyzeTracksWithMoreReasonableCUEInterpretation())
		{
			return ERROR_BINARY_SIZE_NOT_SECTOR_TIMES_INTEGER;
		}
	}

	if(0<tracks.size())
	{
		num_sectors=tracks.back().end.ToHSG()+1;  // LastSectorNumber+1
	}

	for(long long int i=0; i<tracks.size(); ++i)
	{
		DiscLayout L;
		switch(tracks[i].trackType)
		{
		case TRACK_MODE1_DATA:
		case TRACK_MODE2_DATA:
			L.layoutType=LAYOUT_DATA;
			break;
		case TRACK_AUDIO:
			L.layoutType=LAYOUT_AUDIO;
			break;
		}
		L.numSectors=tracks[i].end.ToHSG()-tracks[i].start.ToHSG()+1;
		L.sectorLength=tracks[i].sectorLength;
		L.startHSG=tracks[i].start.ToHSG();
		L.indexToBinary=(1==binaries.size() ? 0 : i);
		if(0==i)
		{
			L.locationInFile=0;
			layout.push_back(L);
		}
		else
		{
			auto locationInFile=tracks[i].locationInFile;
			auto preGapInHSG=tracks[i].preGap.ToHSG();
			if(0<preGapInHSG)
			{
				DiscLayout preGap;
				preGap.layoutType=LAYOUT_GAP;
				preGap.sectorLength=tracks[i].preGapSectorLength;
				preGap.startHSG=tracks[i].start.ToHSG()-preGapInHSG;
				preGap.numSectors=preGapInHSG;
				preGap.locationInFile=locationInFile;
				preGap.indexToBinary=(1==binaries.size() ? 0 : i);
				layout.push_back(preGap);
				locationInFile+=tracks[i].preGapSectorLength*preGapInHSG;
			}
			L.startHSG+=preGapInHSG;
			L.locationInFile=locationInFile;
			layout.push_back(L);
		}
	}
	{
		DiscLayout L;
		L.layoutType=LAYOUT_END;
		L.sectorLength=0;
		L.numSectors=0;
		L.indexToBinary=(1==binaries.size() ? 0 : binaries.size()-1);
		if(0<layout.size())
		{
			L.startHSG=layout.back().startHSG+layout.back().numSectors;
			L.locationInFile=layout.back().locationInFile+layout.back().sectorLength*layout.back().numSectors;
		}
		else
		{
			L.startHSG=0;
			L.locationInFile=0;
		}
		layout.push_back(L);
	}

#ifdef DEBUG_DISCIMG
	for(auto L : layout)
	{
		std::cout << "LAYOUT ";
		switch(L.layoutType)
		{
		case LAYOUT_DATA:
			std::cout << "DATA  ";
			break;
		case LAYOUT_AUDIO:
			std::cout << "AUDIO ";
			break;
		case LAYOUT_GAP:
			std::cout << "GAP   ";
			break;
		case LAYOUT_END:
			std::cout << "END   ";
			break;
		default:
			std::cout << "????? ";
			break;
		}
		std::cout << L.startHSG << " " << L.locationInFile << " " << totalBinLength << std::endl;
	}
#endif

	return ERROR_NOERROR;
}
bool DiscImage::TryAnalyzeTracksWithAbsurdCUEInterpretation(void)
{
	for(long long int i=0; i<(int)tracks.size(); ++i)
	{
		// Why I say this interpretation is absurd.
		// Why pre-gap sector length of this sector needs to be the sector length of the previous track?
		tracks[i].preGapSectorLength=(0==i ? 0 : tracks[i-1].sectorLength);
	}

	long long int prevTrackSizeInBytes=0;
	for(long long int i=0; i<(int)tracks.size(); ++i)
	{
		long long int trackLength=0,gapLength=0;
		if(i+1<tracks.size())
		{
			auto endMSF=tracks[i+1].start-tracks[i+1].preGap;
			auto endHSG=endMSF.ToHSG();
			tracks[i].end=HSGtoMSF(endHSG-1);
			trackLength=(endHSG-tracks[i].start.ToHSG())*tracks[i].sectorLength;
		}
		if(1<=i)
		{
			tracks[i].locationInFile=tracks[i-1].locationInFile+prevTrackSizeInBytes;
			auto preGap=tracks[i].preGap.ToHSG();
			gapLength=preGap*tracks[i].preGapSectorLength;
		}
		prevTrackSizeInBytes=trackLength+gapLength;
	}

	auto lastTrackBytes=totalBinLength-tracks.back().locationInFile;
	if(0!=(lastTrackBytes%tracks.back().sectorLength))
	{
		return false;
	}
	auto lastTrackNumSec=(totalBinLength-tracks.back().locationInFile)/tracks.back().sectorLength;
	auto lastSectorHSG=MSFtoHSG(tracks.back().start)+lastTrackNumSec-1;
	tracks.back().end=HSGtoMSF((unsigned int)lastSectorHSG);

	return true;
}
bool DiscImage::TryAnalyzeTracksWithMoreReasonableCUEInterpretation(void)
{
	// More straight-forward interpretation, but I am not sure if it is correct.
	for(long long int i=0; i<(int)tracks.size(); ++i)
	{
		tracks[i].preGapSectorLength=2352;
	}

	for(long long int i=0; i<tracks.size(); ++i)
	{
		if(i+1<tracks.size())
		{
			tracks[i].end=HSGtoMSF(tracks[i+1].start.ToHSG()-tracks[i+1].preGap.ToHSG()-1);
		}
		tracks[i].locationInFile=(tracks[i].start.ToHSG()-tracks[i].preGap.ToHSG())*2352;
	}

	auto lastTrackBytes=totalBinLength-tracks.back().locationInFile;
	if(0!=(lastTrackBytes%tracks.back().sectorLength))
	{
		return false;
	}
	auto lastTrackNumSec=(totalBinLength-tracks.back().locationInFile)/tracks.back().sectorLength;
	auto lastSectorHSG=MSFtoHSG(tracks.back().start)+lastTrackNumSec-1;
	tracks.back().end=HSGtoMSF((unsigned int)lastSectorHSG);

	return true;
}

unsigned int DiscImage::OpenISO(const std::string &fName)
{
	std::ifstream ifp;
	ifp.open(fName,std::ios::binary);
	if(true!=ifp.is_open())
	{
		return ERROR_CANNOT_OPEN;
	}

	auto begin=ifp.tellg();
	ifp.seekg(0,std::ios::end);
	auto end=ifp.tellg();

	auto fSize=end-begin;

	fSize&=(~2047); // Some image-generation tools adds extra bytes that need to be ignored.
	// if(0!=fSize%2048)
	// {
	// 	return ERROR_SECTOR_SIZE;
	// }

	CleanUp();


	fileType=FILETYPE_ISO;
	this->fName=fName;

	Binary bin;
	bin.fName=fName;
	binaries.push_back(bin);

	num_sectors=(unsigned int)(fSize/2048);

	tracks.resize(1);
	tracks[0].trackType=TRACK_MODE1_DATA;
	tracks[0].sectorLength=2048;
	tracks[0].start=MinSecFrm::Zero();
	tracks[0].end=HSGtoMSF(num_sectors);
	tracks[0].postGap=MinSecFrm::Zero();

	return ERROR_NOERROR;
}
unsigned int DiscImage::GetNumTracks(void) const
{
	return (unsigned int)tracks.size();
}
unsigned int DiscImage::GetNumSectors(void) const
{
	return num_sectors;
}
const std::vector <DiscImage::Track> &DiscImage::GetTracks(void) const
{
	return tracks;
}
/* static */ DiscImage::MinSecFrm DiscImage::HSGtoMSF(unsigned int HSG)
{
	MinSecFrm MSF;
	MSF.FromHSG(HSG);
	return MSF;
}
/* static */ unsigned int DiscImage::MSFtoHSG(MinSecFrm MSF)
{
	return MSF.ToHSG();
}
/* static */ unsigned int DiscImage::BinToBCD(unsigned int bin)
{
	unsigned int high=bin/10;
	unsigned int low=bin%10;
	return (high<<4)+low;
}
/* static */ unsigned int DiscImage::BCDToBin(unsigned int bin)
{
	unsigned int high=(bin>>4);
	unsigned int low=(bin&15);
	return high*10+low;
}

std::vector <unsigned char> DiscImage::ReadSectorMODE1(unsigned int HSG,unsigned int numSec) const
{
	std::vector <unsigned char> data;

	if(0<binaries.size())
	{
		std::ifstream ifp;
		ifp.open(binaries[0].fName,std::ios::binary);
		if(true==ifp.is_open() && 0<tracks.size() && (tracks[0].trackType==TRACK_MODE1_DATA || tracks[0].trackType==TRACK_MODE2_DATA))
		{
			if(HSG+numSec<=tracks[0].end.ToHSG()+1)
			{
				auto sectorIntoTrack=HSG-tracks[0].start.ToHSG();
				auto locationInTrack=sectorIntoTrack*tracks[0].sectorLength;

				ifp.seekg(tracks[0].locationInFile+locationInTrack,std::ios::beg);
				data.resize(numSec*MODE1_BYTES_PER_SECTOR);
				if(MODE1_BYTES_PER_SECTOR==tracks[0].sectorLength)
				{
					ifp.read((char *)data.data(),MODE1_BYTES_PER_SECTOR*numSec);
				}
				else if(RAW_BYTES_PER_SECTOR==tracks[0].sectorLength)
				{
					unsigned int dataPointer=0;
					char skipper[288];
					for(int i=0; i<(int)numSec; ++i)
					{
						ifp.read(skipper,16);
						ifp.read((char *)data.data()+dataPointer,MODE1_BYTES_PER_SECTOR);
						ifp.read(skipper,288);
						dataPointer+=MODE1_BYTES_PER_SECTOR;
					}
				}
			}
		}
	}
	return data;
}

int DiscImage::GetTrackFromMSF(MinSecFrm MSF) const
{
	if(0<tracks.size())
	{
		int tLow=0,tHigh=(int)tracks.size()-1;
		while(tLow!=tHigh)
		{
			auto tMid=(tLow+tHigh)/2;
			if(MSF<tracks[tMid].start)
			{
				tHigh=tMid;
			}
			else if(tracks[tMid].end<MSF)
			{
				tLow=tMid;
			}
			else
			{
				return tMid+1;
			}
		}
		return tLow+1;
	}
	return -1;
}

std::vector <unsigned char> DiscImage::GetWave(MinSecFrm startMSF,MinSecFrm endMSF) const
{
	std::vector <unsigned char> wave;

	if(0<tracks.size() && startMSF<endMSF)
	{
		auto startHSG=startMSF.ToHSG();
		auto endHSG=endMSF.ToHSG();

	#ifdef DEBUG_DISCIMG
		std::cout << "From " << startHSG << " To " << endHSG << " (" << endHSG-startHSG << ")" << std::endl;
	#endif

		for(int i=0; i+1<layout.size(); ++i)  // Condition i<layout.size()-1 will crash when layout.size()==0 because it is unsigned.
		{
			unsigned long long int readFrom=0,readTo=0;

			if(startHSG<=layout[i].startHSG)
			{
				if(LAYOUT_DATA==layout[i].layoutType)
				{
					wave.clear();
					return wave;
				}
				readFrom=layout[i].locationInFile;
			}
			else if(startHSG<layout[i+1].startHSG)
			{
				readFrom=layout[i].locationInFile+layout[i].sectorLength*(startHSG-layout[i].startHSG);
			}
			else
			{
				continue;
			}

			auto &bin=binaries[layout[i].indexToBinary]; // Do it before (*1)

			if(layout[i+1].startHSG<=endHSG)
			{
				readTo=layout[i+1].locationInFile;
			}
			else
			{
				readTo=layout[i].locationInFile+layout[i].sectorLength*(endHSG-layout[i].startHSG);
				i=layout.size(); // Let it loop-out. (*1)
			}

			if(readFrom<readTo)
			{
				auto readSize=(readTo-readFrom)&(~3);

			#ifdef DEBUG_DISCIMG
				std::cout << readFrom << " " << readTo << " " << readSize << " " << std::endl;
			#endif

				std::ifstream ifp;
				ifp.open(bin.fName,std::ios::binary);
				if(ifp.is_open())
				{
					ifp.seekg(readFrom-bin.byteOffsetInDisc+bin.bytesToSkip,std::ios::beg);
					auto curSize=wave.size();
					wave.resize(wave.size()+readSize);
					for(auto i=curSize; i<wave.size(); ++i)
					{
						wave[i]=0;
					}
					ifp.read((char *)(wave.data()+curSize),readSize);
					ifp.close();
				}
			}
		}
	}

	return wave;
}

DiscImage::TrackTime DiscImage::DiscTimeToTrackTime(MinSecFrm discMSF) const
{
	TrackTime trackTime;
	for(int i=0; i<tracks.size(); ++i)
	{
		if(tracks[i].start<=discMSF && discMSF<=tracks[i].end)
		{
			trackTime.track=i+1;
			trackTime.MSF=discMSF-tracks[i].start;
			break;
		}
	}
	return trackTime;
}

/* static */ bool DiscImage::StrToMSF(MinSecFrm &msf,const char str[])
{
	msf.min=cpputil::Atoi(str);
	str=cpputil::StrSkip(str,":");
	if(nullptr==str)
	{
		return false;
	}
	msf.sec=cpputil::Atoi(str);
	str=cpputil::StrSkip(str,":");
	if(nullptr==str)
	{
		return false;
	}
	msf.frm=cpputil::Atoi(str);
	return true;
}
