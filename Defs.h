//
// Defs.h
//
#ifndef __H_Defs__
#define __H_Defs__

// REFERENCE_TIME time units per second and per millisecond
#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

#define EXIT_ON_ERROR(hres)  \
              if (FAILED(hres)) { goto Exit; }

#define SAFE_RELEASE(punk)  \
              if ((punk) != nullptr)  \
                { (punk)->Release(); (punk) = nullptr; }

#define SAFE_DELETE(pobj)  \
              if ((pobj) != nullptr)  \
                { delete (pobj); (pobj) = nullptr; }

#define SAFE_DELETE_ARRAY(pobj)  \
              if ((pobj) != nullptr)  \
                { delete[] (pobj); (pobj) = nullptr; }


#pragma pack(push, 1)
struct WavHeader {
	char chunkId[4];
	uint32_t chunkSize;
	char format[4];
	char subchunk1Id[4];
	uint32_t subchunk1Size;
	uint16_t audioFormat;
	uint16_t numChannels;
	uint32_t sampleRate;
	uint32_t byteRate;
	uint16_t blockAlign;
	uint16_t bitsPerSample;
	char subchunk2Id[4];
	uint32_t subchunk2Size;
};
#pragma pack(pop)

#endif // __H_Defs__