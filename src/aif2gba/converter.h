#ifndef CONVERTER_H_
#define CONVERTER_H_

#include <sdw.h>

class CConverter
{
public:
	typedef n16 MarkerId;

	enum EPlayMode
	{
		kPlayModeNoLooping = 0,
		kPlayModeForwardLooping = 1,
		kPlayModeForwardBackwardLooping = 2
	};

#include SDW_MSC_PUSH_PACKED
	struct SChunkHeaderInfo
	{
		u32 ChunkId;
		n32 ChunkSize;
		SChunkHeaderInfo();
	} SDW_GNUC_PACKED;

	struct SFormChunkData
	{
		u32 FormType;
		SFormChunkData();
	} SDW_GNUC_PACKED;

	struct SCommonChunkData
	{
		n16 NumChannels;
		u32 NumSampleFrames;
		n16 SampleSize;
		u8 SampleRate[10];
		SCommonChunkData();
	} SDW_GNUC_PACKED;

	struct SLoop
	{
		n16 PlayMode;
		MarkerId BeginLoop;
		MarkerId EndLoop;
		SLoop();
	} SDW_GNUC_PACKED;

	struct SInstrumentChunkData
	{
		n8 BaseNote;
		n8 Detune;
		n8 LowNote;
		n8 HighNote;
		n8 LowVelocity;
		n8 HighVelocity;
		n16 Gain;
		SLoop SustainLoop;
		SLoop ReleaseLoop;
		SInstrumentChunkData();
	} SDW_GNUC_PACKED;

	struct SSoundDataChunkData0
	{
		u32 Offset;
		u32 BlockSize;
		SSoundDataChunkData0();
	} SDW_GNUC_PACKED;
#include SDW_MSC_POP_PACKED

	struct SMarker
	{
		MarkerId Id;
		u32 Position;
		u8 ByteCount;
		string MarkerName;
		u8 Padding;
		SMarker();
	};

	struct SMarkerChunkData
	{
		u16 NumMarkers;
		vector<SMarker> Markers;
		SMarkerChunkData();
	};

	struct SSoundDataChunkData1
	{
		vector<u8> SoundData;
	};

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

	CConverter();
	~CConverter();
	void SetInputFileName(const UString& a_sInputFileName);
	void SetOutputFileName(const UString& a_sOutputFileName);
	void SetAlgorithm(const string& a_sAlgorithm);
	void SetVerbose(bool a_bVerbose);
	void SetWave(f64 a_fWave);
	void SetVolume(f64 a_fVolume);
	void SetType(n16 a_nType);
	void SetLabel(const string& a_sLabel);
	void SetSampleSize(n32 a_nSampleSize);
	bool ConvertFileAif2S();
	bool ConvertFileAif2Bin();
	bool ConvertFileBin2Aif();
	static bool IsSupportedAlgorithm(const string& a_sAlgorithm);
	static const string s_sPackingAlgorithmAif2Agb_1_05;
	static const string s_sPackingAlgorithmAif2Agb_1_06a;
	static const string s_sPackingAlgorithmPok_Aif_1_06a_006;
private:
	bool readAif();
	bool readFirstChannel();
	bool ajustSample();
	n64 ajustVolume(f64 a_fSample) const;
	bool compressSoundData();
	bool lossyCompressSoundDataAif2Agb105();
	bool lossyCompressSoundDataAif2Agb106a();
	bool lossyRunLengthEncode(n32 a_nRunLength, n32 a_nPrevSample, n32& a_nCompressedLastSample, n32& a_nCompressedSize);
	bool lossyCompressSoundDataPokAif106a006();
	bool writeS() const;
	bool writeBin() const;
	bool readBin();
	bool lossyUncompressSoundDataAif2Agb106a();
	bool lossyUncompressSoundDataPokAif106a006();
	bool writeAif() const;
	static const u8 s_uTable[64];
	static const n32 s_nSquareRoot[65];
	UString m_sInputFileName;
	UString m_sOutputFileName;
	string m_sAlgorithm;
	bool m_bVerbose;
	f64 m_fWave;
	f64 m_fVolume;
	n16 m_nType;
	string m_sLabel;
	n32 m_nSampleSize;
	SCommonChunkData m_CommonChunkData;
	SMarkerChunkData m_MarkerChunkData;
	SInstrumentChunkData m_InstrumentChunkData;
	SSoundDataChunkData0 m_SoundDataChunkData0;
	SSoundDataChunkData1 m_SoundDataChunkData1;
	f64 m_fSampleRate;
	u32 m_uEndPosition;
	u32 m_uLoopPosition;
	vector<n16> m_vSoundDataOriginal;
	n32 m_nWaveDataUncompressedSize;
	n32 m_nWaveDataCompressedSize;
	n32 m_nWaveDataUncompressedLoopPosition;
	n32 m_nWaveDataCompressedLoopPosition;
	f64 m_fFrequency;
	vector<n16> m_vSoundDataCompressed;
	n32 m_nRetryCount;
	n32 m_nCompressedLastByte;
	SWaveDataHeader m_WaveDataHeader;
};

#endif	// CONVERTER_H_
