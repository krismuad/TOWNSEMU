#ifndef DISCIMG_IS_INCLUDED
#define DISCIMG_IS_INCLUDED
/* { */

#include <vector>
#include <string>

class DiscImage
{
public:
	enum
	{
		ERROR_NOERROR,
		ERROR_UNSUPPORTED,
		ERROR_CANNOT_OPEN,
		ERROR_NOT_YET_SUPPORTED,
		ERROR_SECTOR_SIZE,
	};
	enum
	{
		HSG_BASE=150,
		MODE1_BYTES_PER_SECTOR=2048,
		RAW_BYTES_PER_SECTOR=2352,
	};
	static const char *ErrorCodeToText(unsigned int errCode)
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
		}
		return "Undefined error.";
	}
	enum
	{
		FILETYPE_NONE,
		FILETYPE_ISO,
		FILETYPE_CUE
	};
	enum
	{
		TRACK_UNKNOWNTYPE,
		TRACK_MODE1_DATA,
		TRACK_MODE2_DATA,
		TRACK_AUDIO,
	};
	class MinSecFrm
	{
	public:
		short min=0,sec=0,frm=0;
		static MinSecFrm Zero(void);
		static MinSecFrm TwoSeconds(void);

		inline void Set(short m,short s,short f)
		{
			min=m;
			sec=s;
			frm=f;
		}


		inline unsigned int Encode(void) const  // Different from HSG
		{
			return (min<<16)|(sec<<8)|frm;
		}


		inline void Add(const MinSecFrm incoming)
		{
			this->frm+=incoming.frm;
			if(75<=this->frm)
			{
				this->frm-=75;
				++this->sec;
			}
			this->sec+=incoming.sec;
			if(60<=this->sec)
			{
				this->sec-=60;
				++this->min;
			}
			this->min+=incoming.min;
		}
		inline MinSecFrm &operator+=(const MinSecFrm incoming)
		{
			Add(incoming);
			return *this;
		}
		inline MinSecFrm operator+(const MinSecFrm incoming)
		{
			auto ret=*this;
			ret.Add(incoming);
			return ret;
		}


		inline void Sub(const MinSecFrm incoming)
		{
			this->frm-=incoming.frm;
			if(this->frm<0)
			{
				this->frm+=75;
				--this->sec;
			}
			this->sec-=incoming.sec;
			if(this->sec<0)
			{
				this->sec+=60;
				--this->min;
			}
			this->min-=incoming.min;
		}
		inline MinSecFrm &operator-=(const MinSecFrm incoming)
		{
			Sub(incoming);
			return *this;
		}
		inline MinSecFrm operator-(const MinSecFrm incoming)
		{
			auto ret=*this;
			ret.Sub(incoming);
			return ret;
		}

		inline bool operator==(const MinSecFrm incoming)
		{
			return this->Encode()==incoming.Encode();
		}
		inline bool operator!=(const MinSecFrm incoming)
		{
			return this->Encode()!=incoming.Encode();
		}
		inline bool operator>(const MinSecFrm incoming)
		{
			return this->Encode()>incoming.Encode();
		}
		inline bool operator<(const MinSecFrm incoming)
		{
			return this->Encode()<incoming.Encode();
		}
		inline bool operator>=(const MinSecFrm incoming)
		{
			return this->Encode()>=incoming.Encode();
		}
		inline bool operator<=(const MinSecFrm incoming)
		{
			return this->Encode()<=incoming.Encode();
		}


		inline unsigned int ToHSG(void) const
		{
			return (min*60+sec)*75+frm;
		}
		inline void FromHSG(unsigned int HSG)
		{
			frm=HSG%75;
			HSG/=75;
			sec=HSG%60;
			HSG/=60;
			min=HSG;
		}
	};
	class Track
	{
	public:
		unsigned int trackType=TRACK_UNKNOWNTYPE;
		unsigned int sectorLength=2352; // Default 2352 bytes per sector.
		unsigned long long int locationInFile=0;
		mutable std::vector <unsigned char> dataCache;
		MinSecFrm start,end;  // end must be 1-frame before the next track or the disc length.
		MinSecFrm preGap,postGap;

		inline Track()
		{
			CleanUp();
		}
		inline void CleanUp(void)
		{
			trackType=TRACK_UNKNOWNTYPE;
			sectorLength=2352;
			locationInFile=0;
			dataCache.clear();
			start=MinSecFrm::Zero();
			end=MinSecFrm::Zero();
			preGap=MinSecFrm::Zero();
			postGap=MinSecFrm::Zero();
		}
	};
	unsigned int fileType=FILETYPE_NONE;
	unsigned long long int binFileSize=0;
	std::string fName,binFName;
	unsigned int num_sectors;
	std::vector <Track> tracks;



	DiscImage();
	void CleanUp(void);
	unsigned int Open(const std::string &fName);
	unsigned int OpenCUE(const std::string &fName);
	unsigned int OpenISO(const std::string &fName);

	/*! Returns the number of tracks.
	    Returns zero if image is not loaded.
	*/
	unsigned int GetNumTracks(void) const;

	/*! Returns theu number of sectors.
	*/
	unsigned int GetNumSectors(void) const;

	/*! Returns tracks.
	*/
	const std::vector <Track> &GetTracks(void) const;

	/*! Read data sectors MODE1 (2048 bytes/sector).
	    If it cannot be read (no data track, unsupported sector length, go beyond the data-track limit, etc.),
	    it returns zero byte.
	*/
	std::vector <unsigned char> ReadSectorMODE1(unsigned int HSG,unsigned int numSec) const;

	static MinSecFrm HSGtoMSF(unsigned int HSG);
	static unsigned int MSFtoHSG(MinSecFrm MSF);
	static unsigned int BinToBCD(unsigned int bin);
	static unsigned int BCDToBin(unsigned int bin);
};



/* } */
#endif