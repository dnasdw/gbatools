#ifndef FINDWAVEDATA_H_
#define FINDWAVEDATA_H_

#include <sdw.h>

class CFindWaveData
{
public:
#include SDW_MSC_PUSH_PACKED
	struct SWaveDataHeader
	{
		u16 Type;
		u16 Stat;
		u32 Freq;
		u32 Loop;
		u32 Size;
		SWaveDataHeader();
	} SDW_GNUC_PACKED;
#include SDW_MSC_POP_PACKED

	CFindWaveData();
	~CFindWaveData();
	void SetInputFileName(const UString& a_sInputFileName);
	void SetOutputDirName(const UString& a_sOutputDirName);
	bool Find();
	static int Help();
	static const n32 s_nWaveDataSizeMax;
	static const string s_sPackingAlgorithmAif2Agb_1_06a;
	static const string s_sPackingAlgorithmPok_Aif_1_06a_006;
private:
	bool lossyUncompressSoundDataAif2Agb106a();
	UString m_sInputFileName;
	UString m_sOutputDirName;
	vector<u8> m_vFile;
	u32 m_uFilePos;
	n16 m_nType;
	n16 m_nPlayMode;
	n32 m_nWaveDataUncompressedSize;
	n32 m_nWaveDataCompressedSize;
	n32 m_nWaveDataUncompressedLoopPosition;
	n32 m_nWaveDataCompressedLoopPosition;
	n32 m_nCompressedLastByte;
};

#endif	// FINDWAVEDATA_H_
