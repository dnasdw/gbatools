#include "converter.h"
#include "float80.h"

const string CConverter::s_sPackingAlgorithmAif2Agb_1_05 = "aif2agb_1.05";
const string CConverter::s_sPackingAlgorithmAif2Agb_1_06a = "aif2agb_1.06a";
const string CConverter::s_sPackingAlgorithmPok_Aif_1_06a_006 = "pok_aif_1.06a.006";

const u8 CConverter::s_uTable[64] =
{
	0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x05, 0x07, 0x09, 0x0C, 0x0F, 0x12, 0x15, 0x19, 0x1D, 0x21,
	0x25, 0x2A, 0x2E, 0x33, 0x38, 0x3E, 0x43, 0x49, 0x4F, 0x54, 0x5A, 0x60, 0x67, 0x6D, 0x73, 0x79,
	0x7F, 0x86, 0x8C, 0x92, 0x98, 0x9F, 0xA5, 0xAB, 0xB0, 0xB6, 0xBC, 0xC1, 0xC7, 0xCC, 0xD1, 0xD5,
	0xDA, 0xDE, 0xE2, 0xE6, 0xEA, 0xED, 0xF0, 0xF3, 0xF6, 0xF8, 0xFA, 0xFC, 0xFD, 0xFE, 0xFF, 0xFF
};

const n32 CConverter::s_nSquareRoot[65] =
{
	/* 0*/0,
	/* 1*/1, /* 2*/1, /* 3*/1, /* 4*/2, /* 5*/2, /* 6*/2, /* 7*/2, /* 8*/2, /* 9*/3, /*10*/3,
	/*11*/3, /*12*/3, /*13*/3, /*14*/3, /*15*/3, /*16*/4, /*17*/4, /*18*/4, /*19*/4, /*20*/4,
	/*21*/4, /*22*/4, /*23*/4, /*24*/4, /*25*/5, /*26*/5, /*27*/5, /*28*/5, /*29*/5, /*30*/5,
	/*31*/5, /*32*/5, /*33*/5, /*34*/5, /*35*/5, /*36*/6, /*37*/6, /*38*/6, /*39*/6, /*40*/6,
	/*41*/6, /*42*/6, /*43*/6, /*44*/6, /*45*/6, /*46*/6, /*47*/6, /*48*/6, /*49*/7, /*50*/7,
	/*51*/7, /*52*/7, /*53*/7, /*54*/7, /*55*/7, /*56*/7, /*57*/7, /*58*/7, /*59*/7, /*60*/7,
	/*61*/7, /*62*/7, /*63*/7, /*64*/8
};

CConverter::SChunkHeaderInfo::SChunkHeaderInfo()
	: ChunkId(0)
	, ChunkSize(0)
{
}

CConverter::SFormChunkData::SFormChunkData()
	: FormType(0)
{
}

CConverter::SCommonChunkData::SCommonChunkData()
	: NumChannels(0)
	, NumSampleFrames(0)
	, SampleSize(0)
{
	memset(SampleRate, 0, sizeof(SampleRate));
}

CConverter::SLoop::SLoop()
	: PlayMode(0)
	, BeginLoop(0)
	, EndLoop(0)
{
}

CConverter::SInstrumentChunkData::SInstrumentChunkData()
	: BaseNote(0)
	, Detune(0)
	, LowNote(0)
	, HighNote(0)
	, LowVelocity(0)
	, HighVelocity(0)
	, Gain(0)
{
}

CConverter::SSoundDataChunkData0::SSoundDataChunkData0()
	: Offset(0)
	, BlockSize(0)
{
}

CConverter::SMarker::SMarker()
	: Id(0)
	, Position(0)
	, ByteCount(0)
	, Padding(0)
{
}

CConverter::SMarkerChunkData::SMarkerChunkData()
	: NumMarkers(0)
{
}

CConverter::SWaveDataHeader::SWaveDataHeader()
	: Type(0)
	, Stat(0)
	, Freq(0)
	, Loop(0)
	, Size(0)
{
}

CConverter::CConverter()
	: m_bVerbose(false)
	, m_fWave(1.0)
	, m_fVolume(1.0)
	, m_nType(0)
	, m_nSampleSize(8)
	, m_fSampleRate(0.0)
	, m_uEndPosition(0)
	, m_uLoopPosition(0)
	, m_nWaveDataUncompressedSize(0)
	, m_nWaveDataCompressedSize(0)
	, m_nWaveDataUncompressedLoopPosition(0)
	, m_nWaveDataCompressedLoopPosition(0)
	, m_fFrequency(0.0)
	, m_nRetryCount(0)
	, m_nCompressedLastByte(0)
{
}

CConverter::~CConverter()
{
}

void CConverter::SetInputFileName(const UString& a_sInputFileName)
{
	m_sInputFileName = a_sInputFileName;
}

void CConverter::SetOutputFileName(const UString& a_sOutputFileName)
{
	m_sOutputFileName = a_sOutputFileName;
}

void CConverter::SetAlgorithm(const string& a_sAlgorithm)
{
	m_sAlgorithm = a_sAlgorithm;
	transform(m_sAlgorithm.begin(), m_sAlgorithm.end(), m_sAlgorithm.begin(), ::tolower);
}

void CConverter::SetVerbose(bool a_bVerbose)
{
	m_bVerbose = a_bVerbose;
}

void CConverter::SetWave(f64 a_fWave)
{
	m_fWave = a_fWave;
}

void CConverter::SetVolume(f64 a_fVolume)
{
	m_fVolume = a_fVolume;
}

void CConverter::SetType(n16 a_nType)
{
	m_nType = a_nType;
}

void CConverter::SetLabel(const string& a_sLabel)
{
	m_sLabel = a_sLabel;
}

void CConverter::SetSampleSize(n32 a_nSampleSize)
{
	m_nSampleSize = a_nSampleSize;
}

bool CConverter::ConvertFileAif2S()
{
	if (m_sInputFileName.empty())
	{
		UPrintf(USTR("ERROR: input file name is empty\n\n"));
		return false;
	}
	if (m_sOutputFileName.empty())
	{
		m_sOutputFileName = m_sInputFileName;
		UString::size_type uPos = m_sOutputFileName.rfind(USTR('.'));
		if (uPos != UString::npos)
		{
			m_sOutputFileName.erase(uPos);
		}
		m_sOutputFileName += USTR(".s");
	}
	if (m_sLabel.empty())
	{
		UString sLabel = m_sOutputFileName;
		UString::size_type uPos = sLabel.find_last_of(USTR("/\\"));
		if (uPos != UString::npos)
		{
			sLabel.erase(0, uPos + 1);
		}
		uPos = sLabel.rfind(USTR('.'));
		if (uPos != UString::npos)
		{
			sLabel.erase(uPos);
		}
		m_sLabel = UToA(sLabel);
	}
	if (m_bVerbose)
	{
		UPrintf(USTR("Input  = %") PRIUS USTR("\n"), m_sInputFileName.c_str());
		UPrintf(USTR("Output = %") PRIUS USTR("\n"), m_sOutputFileName.c_str());
	}
	if (!readAif())
	{
		return false;
	}
	if (!writeS())
	{
		return false;
	}
	return true;
}

bool CConverter::ConvertFileAif2Bin()
{
	if (m_sInputFileName.empty())
	{
		UPrintf(USTR("ERROR: input file name is empty\n\n"));
		return false;
	}
	if (m_sOutputFileName.empty())
	{
		m_sOutputFileName = m_sInputFileName;
		UString::size_type uPos = m_sOutputFileName.rfind(USTR('.'));
		if (uPos != UString::npos)
		{
			m_sOutputFileName.erase(uPos);
		}
		m_sOutputFileName += USTR(".bin");
	}
	if (m_bVerbose)
	{
		UPrintf(USTR("Input  = %") PRIUS USTR("\n"), m_sInputFileName.c_str());
		UPrintf(USTR("Output = %") PRIUS USTR("\n"), m_sOutputFileName.c_str());
	}
	if (!readAif())
	{
		return false;
	}
	if (!writeBin())
	{
		return false;
	}
	return true;
}

bool CConverter::ConvertFileBin2Aif()
{
	if (m_sInputFileName.empty())
	{
		UPrintf(USTR("ERROR: input file name is empty\n\n"));
		return false;
	}
	if (m_sOutputFileName.empty())
	{
		m_sOutputFileName = m_sInputFileName;
		UString::size_type uPos = m_sOutputFileName.rfind(USTR('.'));
		if (uPos != UString::npos)
		{
			m_sOutputFileName.erase(uPos);
		}
		m_sOutputFileName += USTR(".aif");
	}
	if (m_bVerbose)
	{
		UPrintf(USTR("Input  = %") PRIUS USTR("\n"), m_sInputFileName.c_str());
		UPrintf(USTR("Output = %") PRIUS USTR("\n"), m_sOutputFileName.c_str());
	}
	if (!readBin())
	{
		return false;
	}
	if (!writeAif())
	{
		return false;
	}
	return true;
}

bool CConverter::IsSupportedAlgorithm(const string& a_sAlgorithm)
{
	string sAlgorithm = a_sAlgorithm;
	transform(sAlgorithm.begin(), sAlgorithm.end(), sAlgorithm.begin(), ::tolower);
	if (false
		|| sAlgorithm == s_sPackingAlgorithmAif2Agb_1_05
		|| sAlgorithm == s_sPackingAlgorithmAif2Agb_1_06a
		|| sAlgorithm == s_sPackingAlgorithmPok_Aif_1_06a_006
		)
	{
		return true;
	}
	return false;
}

bool CConverter::readAif()
{
	FILE* fp = UFopen(m_sInputFileName.c_str(), USTR("rb"), true);
	if (fp == nullptr)
	{
		return false;
	}
	Fseek(fp, 0, SEEK_END);
	u32 uFileSize = static_cast<u32>(Ftell(fp));
	if (uFileSize == 0)
	{
		fclose(fp);
		UPrintf(USTR("ERROR: file %") PRIUS USTR(" is empty\n\n"), m_sInputFileName.c_str());
		return false;
	}
	vector<u8> vFile(uFileSize);
	Fseek(fp, 0, SEEK_SET);
	fread(&*vFile.begin(), 1, uFileSize, fp);
	fclose(fp);
	u32 uFilePos = 0;
	if (uFilePos + sizeof(SChunkHeaderInfo) > uFileSize)
	{
		UPrintf(USTR("ERROR: file %") PRIUS USTR(" is too small\n\n"), m_sInputFileName.c_str());
		return false;
	}
	const SChunkHeaderInfo* pFormChunkHeaderInfo = reinterpret_cast<const SChunkHeaderInfo*>(&*vFile.begin() + uFilePos);
	uFilePos += sizeof(SChunkHeaderInfo);
	if (pFormChunkHeaderInfo->ChunkId != SDW_CONVERT_ENDIAN32('FORM'))
	{
		if (uFileSize < 0x80)
		{
			UPrintf(USTR("ERROR: file %") PRIUS USTR(" is too small\n\n"), m_sInputFileName.c_str());
			return false;
		}
		uFilePos = 0x80;
		if (uFilePos + sizeof(SChunkHeaderInfo) > uFileSize)
		{
			UPrintf(USTR("ERROR: file %") PRIUS USTR(" is too small\n\n"), m_sInputFileName.c_str());
			return false;
		}
		pFormChunkHeaderInfo = reinterpret_cast<const SChunkHeaderInfo*>(&*vFile.begin() + uFilePos);
		uFilePos += sizeof(SChunkHeaderInfo);
		if (pFormChunkHeaderInfo->ChunkId != SDW_CONVERT_ENDIAN32('FORM'))
		{
			UPrintf(USTR("ERROR: file %") PRIUS USTR(" has invalid FORM chunk\n\n"), m_sInputFileName.c_str());
			return false;
		}
	}
	if (uFilePos + SDW_CONVERT_ENDIAN32(pFormChunkHeaderInfo->ChunkSize) != uFileSize)
	{
		UPrintf(USTR("ERROR: file %") PRIUS USTR(" has invalid FORM chunk size\n\n"), m_sInputFileName.c_str());
		return false;
	}
	if (uFilePos + sizeof(SFormChunkData) > uFileSize)
	{
		UPrintf(USTR("ERROR: file %") PRIUS USTR(" is too small\n\n"), m_sInputFileName.c_str());
		return false;
	}
	const SFormChunkData* pFormChunkData = reinterpret_cast<const SFormChunkData*>(&*vFile.begin() + uFilePos);
	uFilePos += sizeof(SFormChunkData);
	if (pFormChunkData->FormType != SDW_CONVERT_ENDIAN32('AIFF'))
	{
		UPrintf(USTR("ERROR: file %") PRIUS USTR(" has invalid FORM type\n\n"), m_sInputFileName.c_str());
		return false;
	}
	const SChunkHeaderInfo* pCommonChunkHeaderInfo = nullptr;
	const SChunkHeaderInfo* pMarkerChunkHeaderInfo = nullptr;
	const SChunkHeaderInfo* pInstrumentChunkHeaderInfo = nullptr;
	const SChunkHeaderInfo* pSoundDataChunkHeaderInfo = nullptr;
	while (static_cast<u32>(Align(uFilePos, 2)) + sizeof(SChunkHeaderInfo) < uFileSize)
	{
		uFilePos = static_cast<u32>(Align(uFilePos, 2));
		const SChunkHeaderInfo* pChunkHeaderInfo = reinterpret_cast<const SChunkHeaderInfo*>(&*vFile.begin() + uFilePos);
		uFilePos += sizeof(SChunkHeaderInfo);
		n32 nChunkSize = SDW_CONVERT_ENDIAN32(pChunkHeaderInfo->ChunkSize);
		if (nChunkSize < 0 || uFilePos + nChunkSize > uFileSize)
		{
			UPrintf(USTR("ERROR: file %") PRIUS USTR(" has invalid chunk size\n\n"), m_sInputFileName.c_str());
			return false;
		}
		n32 nPos = 0;
		if (pChunkHeaderInfo->ChunkId == SDW_CONVERT_ENDIAN32('COMM'))
		{
			if (pCommonChunkHeaderInfo != nullptr)
			{
				UPrintf(USTR("ERROR: file %") PRIUS USTR(" has multiple COMM chunks\n\n"), m_sInputFileName.c_str());
				return false;
			}
			pCommonChunkHeaderInfo = pChunkHeaderInfo;
			if (nChunkSize != sizeof(SCommonChunkData))
			{
				UPrintf(USTR("ERROR: file %") PRIUS USTR(" has invalid COMM chunk size\n\n"), m_sInputFileName.c_str());
				return false;
			}
			memcpy(&m_CommonChunkData, &*vFile.begin() + uFilePos + nPos, sizeof(SCommonChunkData));
			nPos += sizeof(SCommonChunkData);
			m_CommonChunkData.NumChannels = SDW_CONVERT_ENDIAN16(m_CommonChunkData.NumChannels);
			m_CommonChunkData.NumSampleFrames = SDW_CONVERT_ENDIAN32(m_CommonChunkData.NumSampleFrames);
			m_CommonChunkData.SampleSize = SDW_CONVERT_ENDIAN16(m_CommonChunkData.SampleSize);
			if (m_CommonChunkData.NumChannels < 1)
			{
				UPrintf(USTR("ERROR: file %") PRIUS USTR(" has invalid number of channels\n\n"), m_sInputFileName.c_str());
				return false;
			}
			if (m_CommonChunkData.SampleSize < 1 || m_CommonChunkData.SampleSize > 32)
			{
				UPrintf(USTR("ERROR: file %") PRIUS USTR(" has invalid sample size\n\n"), m_sInputFileName.c_str());
				return false;
			}
			Float80ToDouble(m_CommonChunkData.SampleRate, &m_fSampleRate, kFloat80EndiannessBigEndian);
			m_uEndPosition = m_CommonChunkData.NumSampleFrames;
		}
		else if (pChunkHeaderInfo->ChunkId == SDW_CONVERT_ENDIAN32('MARK'))
		{
			if (pMarkerChunkHeaderInfo != nullptr)
			{
				UPrintf(USTR("ERROR: file %") PRIUS USTR(" has multiple MARK chunks\n\n"), m_sInputFileName.c_str());
				return false;
			}
			pMarkerChunkHeaderInfo = pChunkHeaderInfo;
			if (nPos + 2 > nChunkSize)
			{
				UPrintf(USTR("ERROR: file %") PRIUS USTR(" has invalid MARK chunk size\n\n"), m_sInputFileName.c_str());
				return false;
			}
			m_MarkerChunkData.NumMarkers = *reinterpret_cast<const u16*>(&*vFile.begin() + uFilePos + nPos);
			nPos += 2;
			m_MarkerChunkData.NumMarkers = SDW_CONVERT_ENDIAN16(m_MarkerChunkData.NumMarkers);
			for (n32 i = 0; i < m_MarkerChunkData.NumMarkers; i++)
			{
				if (nPos + 7 > nChunkSize)
				{
					UPrintf(USTR("ERROR: file %") PRIUS USTR(" has invalid MARK chunk size\n\n"), m_sInputFileName.c_str());
					return false;
				}
				SMarker marker;
				marker.Id = *reinterpret_cast<const MarkerId*>(&*vFile.begin() + uFilePos + nPos);
				nPos += 2;
				marker.Id = SDW_CONVERT_ENDIAN16(marker.Id);
				marker.Position = *reinterpret_cast<const u32*>(&*vFile.begin() + uFilePos + nPos);
				nPos += 4;
				marker.Position = SDW_CONVERT_ENDIAN32(marker.Position);
				marker.ByteCount = *reinterpret_cast<const u8*>(&*vFile.begin() + uFilePos + nPos);
				n32 nByteCount = static_cast<n32>(Align(1 + marker.ByteCount, 2));
				if (nPos + nByteCount > nChunkSize)
				{
					UPrintf(USTR("ERROR: file %") PRIUS USTR(" has invalid MARK chunk size\n\n"), m_sInputFileName.c_str());
					return false;
				}
				marker.MarkerName.assign(reinterpret_cast<const char*>(&*vFile.begin() + uFilePos + nPos + 1), marker.ByteCount);
				nPos += nByteCount;
				m_MarkerChunkData.Markers.push_back(marker);
			}
			if (nPos != nChunkSize)
			{
				UPrintf(USTR("ERROR: file %") PRIUS USTR(" has invalid MARK chunk size\n\n"), m_sInputFileName.c_str());
				return false;
			}
		}
		else if (pChunkHeaderInfo->ChunkId == SDW_CONVERT_ENDIAN32('INST'))
		{
			if (pInstrumentChunkHeaderInfo != nullptr)
			{
				UPrintf(USTR("ERROR: file %") PRIUS USTR(" has multiple INST chunks\n\n"), m_sInputFileName.c_str());
				return false;
			}
			pInstrumentChunkHeaderInfo = pChunkHeaderInfo;
			if (nChunkSize != sizeof(SInstrumentChunkData))
			{
				UPrintf(USTR("ERROR: file %") PRIUS USTR(" has invalid INST chunk size\n\n"), m_sInputFileName.c_str());
				return false;
			}
			memcpy(&m_InstrumentChunkData, &*vFile.begin() + uFilePos + nPos, sizeof(SInstrumentChunkData));
			nPos += sizeof(SInstrumentChunkData);
			m_InstrumentChunkData.Gain = SDW_CONVERT_ENDIAN16(m_InstrumentChunkData.Gain);
			m_InstrumentChunkData.SustainLoop.PlayMode = SDW_CONVERT_ENDIAN16(m_InstrumentChunkData.SustainLoop.PlayMode);
			m_InstrumentChunkData.SustainLoop.BeginLoop = SDW_CONVERT_ENDIAN16(m_InstrumentChunkData.SustainLoop.BeginLoop);
			m_InstrumentChunkData.SustainLoop.EndLoop = SDW_CONVERT_ENDIAN16(m_InstrumentChunkData.SustainLoop.EndLoop);
			m_InstrumentChunkData.ReleaseLoop.PlayMode = SDW_CONVERT_ENDIAN16(m_InstrumentChunkData.ReleaseLoop.PlayMode);
			m_InstrumentChunkData.ReleaseLoop.BeginLoop = SDW_CONVERT_ENDIAN16(m_InstrumentChunkData.ReleaseLoop.BeginLoop);
			m_InstrumentChunkData.ReleaseLoop.EndLoop = SDW_CONVERT_ENDIAN16(m_InstrumentChunkData.ReleaseLoop.EndLoop);
		}
		else if (pChunkHeaderInfo->ChunkId == SDW_CONVERT_ENDIAN32('SSND'))
		{
			if (pSoundDataChunkHeaderInfo != nullptr)
			{
				UPrintf(USTR("ERROR: file %") PRIUS USTR(" has multiple SSND chunks\n\n"), m_sInputFileName.c_str());
				return false;
			}
			pSoundDataChunkHeaderInfo = pChunkHeaderInfo;
			if (nPos + static_cast<n32>(sizeof(SSoundDataChunkData0)) > nChunkSize)
			{
				UPrintf(USTR("ERROR: file %") PRIUS USTR(" has invalid SSND chunk size\n\n"), m_sInputFileName.c_str());
				return false;
			}
			memcpy(&m_SoundDataChunkData0, &*vFile.begin() + uFilePos + nPos, sizeof(SSoundDataChunkData0));
			nPos += sizeof(SSoundDataChunkData0);
			m_SoundDataChunkData0.Offset = SDW_CONVERT_ENDIAN32(m_SoundDataChunkData0.Offset);
			m_SoundDataChunkData0.BlockSize = SDW_CONVERT_ENDIAN32(m_SoundDataChunkData0.BlockSize);
			if (nChunkSize - nPos > 0)
			{
				m_SoundDataChunkData1.SoundData.assign(&*vFile.begin() + uFilePos + nPos, &*vFile.begin() + uFilePos + nChunkSize);
				nPos = nChunkSize;
			}
			m_uLoopPosition = m_SoundDataChunkData0.Offset;
		}
		uFilePos += nChunkSize;
	}
	if (uFilePos != uFileSize)
	{
		UPrintf(USTR("ERROR: file %") PRIUS USTR(" has invalid size\n\n"), m_sInputFileName.c_str());
		return false;
	}
	if (pCommonChunkHeaderInfo == nullptr)
	{
		UPrintf(USTR("ERROR: file %") PRIUS USTR(" has no COMM chunk\n\n"), m_sInputFileName.c_str());
		return false;
	}
	if (pSoundDataChunkHeaderInfo == nullptr)
	{
		UPrintf(USTR("ERROR: file %") PRIUS USTR(" has no SSND chunk\n\n"), m_sInputFileName.c_str());
		return false;
	}
	if (m_SoundDataChunkData0.Offset > m_CommonChunkData.NumSampleFrames)
	{
		UPrintf(USTR("ERROR: file %") PRIUS USTR(" has invalid SSND offset\n\n"), m_sInputFileName.c_str());
		return false;
	}
	if (m_CommonChunkData.NumSampleFrames * m_CommonChunkData.NumChannels * static_cast<u32>(Align(m_CommonChunkData.SampleSize, 8) / 8) > m_SoundDataChunkData1.SoundData.size())
	{
		UPrintf(USTR("ERROR: file %") PRIUS USTR(" has invalid SSND chunk size\n\n"), m_sInputFileName.c_str());
		return false;
	}
	if (m_InstrumentChunkData.SustainLoop.PlayMode > kPlayModeNoLooping)
	{
		if (pMarkerChunkHeaderInfo == nullptr)
		{
			UPrintf(USTR("ERROR: file %") PRIUS USTR(" has no MARK chunk\n\n"), m_sInputFileName.c_str());
			return false;
		}
		for (n32 i = 0; i < m_MarkerChunkData.NumMarkers; i++)
		{
			if (m_MarkerChunkData.Markers[i].Id == m_InstrumentChunkData.SustainLoop.BeginLoop)
			{
				m_uLoopPosition = m_MarkerChunkData.Markers[i].Position;
				if (m_uLoopPosition < m_SoundDataChunkData0.Offset || m_uLoopPosition > m_CommonChunkData.NumSampleFrames)
				{
					UPrintf(USTR("ERROR: file %") PRIUS USTR(" has invalid loop position\n\n"), m_sInputFileName.c_str());
					return false;
				}
			}
			else if (m_MarkerChunkData.Markers[i].Id == m_InstrumentChunkData.SustainLoop.EndLoop)
			{
				m_uEndPosition = m_MarkerChunkData.Markers[i].Position;
				if (m_uEndPosition < m_SoundDataChunkData0.Offset || m_uEndPosition > m_CommonChunkData.NumSampleFrames)
				{
					UPrintf(USTR("ERROR: file %") PRIUS USTR(" has invalid end position\n\n"), m_sInputFileName.c_str());
					return false;
				}
			}
		}
		if (m_uLoopPosition >= m_uEndPosition)
		{
			UPrintf(USTR("ERROR: file %") PRIUS USTR(" has invalid loop position\n\n"), m_sInputFileName.c_str());
			return false;
		}
	}
	m_vSoundDataOriginal.resize(m_uEndPosition);
	if (!readFirstChannel())
	{
		return false;
	}
	if (!ajustSample())
	{
		return false;
	}
	m_vSoundDataCompressed.resize(m_nWaveDataUncompressedSize);
	if (!compressSoundData())
	{
		return false;
	}
	m_WaveDataHeader.Type = m_nType;
	m_WaveDataHeader.Stat = m_InstrumentChunkData.SustainLoop.PlayMode << 14;
	m_WaveDataHeader.Freq = static_cast<u32>(static_cast<n64>(m_fFrequency + 0.5));
	if (m_sAlgorithm == s_sPackingAlgorithmAif2Agb_1_06a)
	{
		m_WaveDataHeader.Loop = m_nWaveDataCompressedLoopPosition;
		m_WaveDataHeader.Size = m_nWaveDataCompressedSize;
	}
	else
	{
		m_WaveDataHeader.Loop = m_nWaveDataUncompressedLoopPosition;
		m_WaveDataHeader.Size = m_nWaveDataUncompressedSize;
	}
	return true;
}

bool CConverter::readFirstChannel()
{
	if (m_CommonChunkData.SampleSize > 16)
	{
		UPrintf(USTR("ERROR: Too big sampleSize %d > 16\n\n"), m_CommonChunkData.SampleSize);
		return false;
	}
	else if (m_CommonChunkData.SampleSize > 8)
	{
		for (u32 i = 0; i < m_uEndPosition; i++)
		{
			n16 nSoundData = *(reinterpret_cast<const n16*>(&*m_SoundDataChunkData1.SoundData.begin()) + i * m_CommonChunkData.NumChannels);
			m_vSoundDataOriginal[i] = SDW_CONVERT_ENDIAN16(nSoundData);
		}
	}
	else
	{
		for (u32 i = 0; i < m_uEndPosition; i++)
		{
			n16 nSoundData = m_SoundDataChunkData1.SoundData[i * m_CommonChunkData.NumChannels];
			m_vSoundDataOriginal[i] = SDW_CONVERT_ENDIAN16(nSoundData);
		}
	}
	return true;
}

bool CConverter::ajustSample()
{
	m_nWaveDataUncompressedSize = static_cast<n32>(static_cast<n64>((m_uEndPosition - m_SoundDataChunkData0.Offset) * m_fWave));
	m_nWaveDataCompressedSize = m_nWaveDataUncompressedSize;
	m_nWaveDataUncompressedLoopPosition = static_cast<n32>(static_cast<n64>((m_uLoopPosition - m_SoundDataChunkData0.Offset) * m_fWave));
	m_nWaveDataCompressedLoopPosition = m_nWaveDataUncompressedLoopPosition;
	m_fFrequency = pow(2.0, (m_InstrumentChunkData.Detune + 100 * (180 - m_InstrumentChunkData.BaseNote)) / 1200.0) * m_fSampleRate * m_fWave;
	vector<n16> vSoundDataWorking(m_nWaveDataUncompressedSize);
	if (m_bVerbose)
	{
		UPrintf(USTR("Wave size = %05Xh\n"), m_nWaveDataUncompressedSize);
		UPrintf(USTR("Now working ...\n"));
	}
	for (n32 i = 0; i < m_nWaveDataUncompressedSize; i++)
	{
		f64 fAdjustedIndex = i / m_fWave;
		n32 nIndex0 = static_cast<n32>(m_SoundDataChunkData0.Offset + static_cast<n64>(floor(fAdjustedIndex)));
		n32 nIndex1 = static_cast<n32>(m_SoundDataChunkData0.Offset + static_cast<n64>(ceil(fAdjustedIndex)));
		n32 nSample0 = nIndex0 < static_cast<n32>(m_uEndPosition) ? m_vSoundDataOriginal[nIndex0] : 0;
		n32 nSample1 = nIndex1 < static_cast<n32>(m_uEndPosition) ? m_vSoundDataOriginal[nIndex1] : 0;
		f64 fT = fAdjustedIndex - floor(fAdjustedIndex);
		f64 fSample = static_cast<f64>(nSample0) + (static_cast<f64>(nSample1) - static_cast<f64>(nSample0)) * fT;
		n64 nSample = ajustVolume(fSample);
		vSoundDataWorking[i] = static_cast<n16>(nSample);
	}
	m_vSoundDataOriginal.swap(vSoundDataWorking);
	return true;
}

n64 CConverter::ajustVolume(f64 a_fSample) const
{
	f64 fSample = a_fSample * m_fVolume;
	if (m_fVolume > 1.0)
	{
		if (fSample > INT16_MAX * 0.875)
		{
			fSample = (fSample - INT16_MAX * 0.875) / (m_fVolume - 0.875) * 0.125 + INT16_MAX * 0.875;
		}
		else if (fSample < INT16_MIN * 0.875)
		{
			fSample = (fSample - INT16_MIN * 0.875) / (m_fVolume - 0.875) * 0.125 + INT16_MIN * 0.875;
		}
	}
	if (fSample > INT16_MAX)
	{
		fSample = INT16_MAX;
	}
	else if (fSample < INT16_MIN)
	{
		fSample = INT16_MIN;
	}
	return static_cast<n64>(fSample);
}

bool CConverter::compressSoundData()
{
	if (m_sAlgorithm == s_sPackingAlgorithmAif2Agb_1_05)
	{
		if (m_nType == 1)
		{
			return lossyCompressSoundDataAif2Agb105();
		}
	}
	else if (m_sAlgorithm == s_sPackingAlgorithmAif2Agb_1_06a)
	{
		if (m_nType == 1)
		{
			return lossyCompressSoundDataAif2Agb106a();
		}
	}
	else if (m_sAlgorithm == s_sPackingAlgorithmPok_Aif_1_06a_006)
	{
		if (m_nType == 1 || m_nType == 11)
		{
			return lossyCompressSoundDataPokAif106a006();
		}
	}
	m_nType = 0;
	for (n32 i = 0; i < m_nWaveDataUncompressedSize; i++)
	{
		m_vSoundDataCompressed[i] = m_vSoundDataOriginal[i] >> 8;
	}
	return true;
}

bool CConverter::lossyCompressSoundDataAif2Agb105()
{
	// compress begin
	vector<n16> vSoundDataWorking(m_nWaveDataUncompressedSize);
	m_nRetryCount = -1;
	n32 nCompressedPos = 0;
	n32 nDeltaMin = 256;
	n32 nRunLengthMin = 1;
	do
	{
		n32 nPrevDelta = 0;
		n32 nRunLength = 0;
		n32 nPrevSample = 0;
		m_nRetryCount++;
		nCompressedPos = 0;
		for (n32 i = 0; i < m_nWaveDataUncompressedSize; i++)
		{
			n32 nCurrSample = m_vSoundDataOriginal[i];
			n32 nCurrDelta = nCurrSample - nPrevSample;
			n32 nDeltaAbs = nCurrDelta < 0 ? -nCurrDelta : nCurrDelta;
			if (nDeltaAbs < nDeltaMin)
			{
				nCurrDelta = 0;
			}
			n32 nPrevSampleAbs = nPrevSample < 0 ? -nPrevSample : nPrevSample;
			if (nDeltaAbs < nPrevSampleAbs * nDeltaMin / 0x4000)
			{
				nCurrDelta = 0;
			}
			if (nRunLength > 0 && nRunLength < nRunLengthMin)
			{
				nCurrDelta = 0;
			}
			bool bCompFlag = false;
			if ((nCurrDelta > 0 && nPrevDelta < 0) || (nCurrDelta < 0 && nPrevDelta > 0))
			{
				bCompFlag = true;
			}
			else
			{
				nRunLength++;
			}
			if (nCurrDelta > 0)
			{
				nPrevDelta = 1;
			}
			else if (nCurrDelta < 0)
			{
				nPrevDelta = -1;
			}
			if (bCompFlag)
			{
				if (nRunLength <= 3)
				{
					nPrevDelta = 0;
					vSoundDataWorking[nCompressedPos++] = static_cast<n16>((((nRunLength - 1) & 0x3) << 6) | (nPrevSample >> 10 & 0x3F));
				}
				else
				{
					vSoundDataWorking[nCompressedPos++] = static_cast<n16>((nRunLength - 1) | 0xC0);
					vSoundDataWorking[nCompressedPos++] = static_cast<n16>(nPrevSample >> 8);
				}
				nRunLength = 1;
			}
			nPrevSample = nCurrSample;
			if (nRunLength > 64)
			{
				vSoundDataWorking[nCompressedPos++] = static_cast<n16>((64 - 1) | 0xC0);
				vSoundDataWorking[nCompressedPos++] = static_cast<n16>(nCurrSample >> 8);
				nRunLength -= 64;
			}
		}
		if (nRunLength <= 3)
		{
			vSoundDataWorking[nCompressedPos++] = static_cast<n16>((((nRunLength - 1) & 0x3) << 6) | (nPrevSample >> 10 & 0x3F));
		}
		else
		{
			vSoundDataWorking[nCompressedPos++] = static_cast<n16>((nRunLength - 1) | 0xC0);
			vSoundDataWorking[nCompressedPos++] = static_cast<n16>(nPrevSample >> 8);
		}
		if (nDeltaMin < 1024)
		{
			nDeltaMin *= 2;
		}
		else
		{
			nRunLengthMin++;
		}
	} while (m_nWaveDataUncompressedSize / 2 < nCompressedPos && nRunLengthMin < 3);
	m_nWaveDataCompressedSize = nCompressedPos;
	// compress end
	bool bResult = true;
	// uncompress begin
	n32 nUncompressedPos = 0;
	n32 nPrevSample = 0;
	nCompressedPos = 0;
	while (m_nWaveDataUncompressedSize - nUncompressedPos > 0)
	{
		if (m_nWaveDataCompressedSize - nCompressedPos < 1)
		{
			bResult = false;
			break;
		}
		n16 nFlags = vSoundDataWorking[nCompressedPos++];
		n32 nFlag = nFlags & 0xC0;
		n32 nLength = 0;
		n32 nCurrSample = 0;
		if (nFlag == 0xC0)
		{
			if (m_nWaveDataCompressedSize - nCompressedPos < 1)
			{
				bResult = false;
				break;
			}
			nLength = (nFlags & 0x3F) + 1;
			nCurrSample = vSoundDataWorking[nCompressedPos++] & 0xFF;
		}
		else
		{
			nLength = (nFlag >> 6) + 1;
			nCurrSample = (nFlags & 0x3F) << 2;
		}
		if ((nCurrSample & 0x80) != 0)
		{
			nCurrSample |= 0xFFFFFF00;
		}
		if (nLength > m_nWaveDataUncompressedSize - nUncompressedPos)
		{
			bResult = false;
			break;
		}
		n32 nReverseDelta = nPrevSample - nCurrSample;
		for (n32 i = 0; i < nLength; i++)
		{
			m_vSoundDataCompressed[nUncompressedPos++] = static_cast<n16>(nCurrSample + nReverseDelta * s_uTable[(nLength - 1 - i) * 64 / nLength] / 256);
		}
		nPrevSample = nCurrSample;
	}
	if (bResult)
	{
		if (nUncompressedPos != m_nWaveDataUncompressedSize || nCompressedPos != m_nWaveDataCompressedSize)
		{
			bResult = false;
		}
	}
	if (!bResult)
	{
		UPrintf(USTR("ERROR: test uncompress error\n\n"));
		return false;
	}
	// uncompress end
	return true;
}

bool CConverter::lossyCompressSoundDataAif2Agb106a()
{
	m_nRetryCount = -1;
	n32 nCompressedPos = 0;
	n32 nDeltaMin = 256;
	n32 nRunLengthMin = 1;
	do
	{
		n32 nAccumulateDelta = 0;
		n32 nPrevDelta = 0;
		n32 nRunLength = 0;
		m_nRetryCount++;
		n32 nCompressedLastSample = 0;
		n32 nPrevSample = 0;
		nCompressedPos = 0;
		for (n32 i = 0; i < m_nWaveDataUncompressedSize; i++)
		{
			n32 nCurrSample = m_vSoundDataOriginal[i];
			nAccumulateDelta += nCurrSample - nPrevSample;
			n32 nCurrDelta = 0;
			n32 nDeltaAbs = nAccumulateDelta < 0 ? -nAccumulateDelta : nAccumulateDelta;
			if (nDeltaAbs >= nDeltaMin)
			{
				nCurrDelta = nAccumulateDelta;
				nAccumulateDelta = 0;
			}
			if (nRunLength > 0 && nRunLength < nRunLengthMin)
			{
				nCurrDelta = 0;
			}
			bool bCompFlag = false;
			if (m_InstrumentChunkData.SustainLoop.PlayMode != kPlayModeNoLooping && i == m_nWaveDataUncompressedLoopPosition + 1)
			{
				m_nWaveDataCompressedLoopPosition = nCompressedPos;
				m_nCompressedLastByte = nCompressedLastSample >> 8;
				bCompFlag = true;
			}
			else if (nRunLength >= 1023 || (nCurrDelta > 0 && nPrevDelta < 0) || (nCurrDelta < 0 && nPrevDelta > 0))
			{
				bCompFlag = true;
			}
			else
			{
				nRunLength++;
			}
			if (bCompFlag)
			{
				nAccumulateDelta = 0;
				if (!lossyRunLengthEncode(nRunLength, nPrevSample, nCompressedLastSample, nCompressedPos))
				{
					return false;
				}
				nRunLength = 1;
			}
			nPrevSample = nCurrSample;
			if (nCurrDelta > 0)
			{
				nPrevDelta = 1;
			}
			else if (nCurrDelta < 0)
			{
				nPrevDelta = -1;
			}
		}
		if (!lossyRunLengthEncode(nRunLength, nPrevSample, nCompressedLastSample, nCompressedPos))
		{
			return false;
		}
		if (nDeltaMin < 1024)
		{
			nDeltaMin *= 2;
		}
		else
		{
			nRunLengthMin++;
		}
	} while (m_nWaveDataUncompressedSize / 2 < nCompressedPos && nRunLengthMin < 3);
	m_nWaveDataCompressedSize = nCompressedPos;
	return true;
}

bool CConverter::lossyRunLengthEncode(n32 a_nRunLength, n32 a_nPrevSample, n32& a_nCompressedLastSample, n32& a_nCompressedPos)
{
	if (a_nRunLength < 1 || a_nRunLength > 1023)
	{
		UPrintf(USTR("ERROR: compress error\n\n"));
		return false;
	}
	if (a_nRunLength <= 3)
	{
		n32 nDelta = a_nPrevSample - a_nCompressedLastSample;
		n32 nCompressedDelta = 0;
		if (nDelta < 0)
		{
			nCompressedDelta = static_cast<n32>(-static_cast<n64>(pow((-nDelta) >> 8, 0.625)));
		}
		else
		{
			nCompressedDelta = static_cast<n32>(static_cast<n64>(pow(nDelta >> 8, 0.625)));
		}
		m_vSoundDataCompressed[a_nCompressedPos++] = static_cast<n16>(((a_nRunLength & 0x3) << 6) | (nCompressedDelta & 0x3F));
		if (nCompressedDelta < 0)
		{
			a_nCompressedLastSample -= static_cast<n32>(static_cast<n64>(pow(-nCompressedDelta, 1.6))) << 8;
		}
		else
		{
			a_nCompressedLastSample += static_cast<n32>(static_cast<n64>(pow(nCompressedDelta, 1.6))) << 8;
		}
	}
	else
	{
		n16 nRunLengthLow = 0;
		if (a_nRunLength <= 63)
		{
			nRunLengthLow = static_cast<n16>(a_nRunLength & 0x3F);
		}
		else
		{
			nRunLengthLow = static_cast<u8>(a_nRunLength);
			m_vSoundDataCompressed[a_nCompressedPos++] = static_cast<n16>(a_nRunLength >> 8 & 0x3);
		}
		m_vSoundDataCompressed[a_nCompressedPos++] = nRunLengthLow;
		m_vSoundDataCompressed[a_nCompressedPos++] = static_cast<n16>(a_nPrevSample >> 8);
		a_nCompressedLastSample = a_nPrevSample >> 8 << 8;
	}
	return true;
}

bool CConverter::lossyCompressSoundDataPokAif106a006()
{
	n32 nPaddingCount = 1;
	// 1 byte + 1 byte + 62 bytes => 1 byte (keep) + 1 byte (1 half byte) + 31 bytes (62 half bytes)
	n32 nCompressedSize = (m_nWaveDataUncompressedSize + nPaddingCount) / 64 * 33;
	n32 nRemainder = (m_nWaveDataUncompressedSize + nPaddingCount) % 64;
	if (nRemainder <= 2)
	{
		nCompressedSize += nRemainder;
	}
	else
	{
		nPaddingCount = (nRemainder - 2) % 2 == 0 ? 1 : 0;
		nCompressedSize += 2 + (nRemainder - 2) / 2;
	}
	// compress begin
	m_vSoundDataCompressed.resize(nCompressedSize);
	n32 nPrevSample = 0;
	n32 nCompressedPos = 0;
	for (n32 i = 0; i < m_nWaveDataUncompressedSize + nPaddingCount; i++)
	{
		n32 nCurrSample = 0;
		if (i < m_nWaveDataUncompressedSize)
		{
			nCurrSample = m_vSoundDataOriginal[i] >> 8;
		}
		if (i % 64 == 0)
		{
			m_vSoundDataCompressed[nCompressedPos++] = static_cast<n16>(nCurrSample);
		}
		else
		{
			n32 nDelta = nCurrSample - nPrevSample;
			if (nDelta > 63)
			{
				nDelta = 63;
			}
			else if (nDelta < -64)
			{
				nDelta = -64;
			}
			n32 nDeltaSquareRoot = 0;
			if (nDelta < 0)
			{
				nDeltaSquareRoot = -s_nSquareRoot[-nDelta];
			}
			else
			{
				nDeltaSquareRoot = s_nSquareRoot[nDelta];
			}
			if (i % 2 == 1)
			{
				if (i % 64 == 1)
				{
					m_vSoundDataCompressed[nCompressedPos++] = nDeltaSquareRoot & 0xF;
				}
				else
				{
					m_vSoundDataCompressed[nCompressedPos++] |= nDeltaSquareRoot & 0xF;
				}
			}
			else
			{
				m_vSoundDataCompressed[nCompressedPos] = static_cast<n16>(nDeltaSquareRoot << 4);
			}
			if (nDeltaSquareRoot < 0)
			{
				nCurrSample = nPrevSample - nDeltaSquareRoot * nDeltaSquareRoot;
			}
			else
			{
				nCurrSample = nPrevSample + nDeltaSquareRoot * nDeltaSquareRoot;
			}
		}
		nPrevSample = nCurrSample;
	}
	if (nCompressedPos != nCompressedSize)
	{
		UPrintf(USTR("ERROR: compress error\n\n"));
		return false;
	}
	m_nWaveDataCompressedSize = nCompressedSize;
	// compress end
	if (m_nType < 10)
	{
		return true;
	}
	// uncompress begin
	vector<n16> vSoundDataWorking(m_nWaveDataUncompressedSize);
	n32 nUncompressedPos = 0;
	nCompressedPos = 0;
	for (; nUncompressedPos < m_nWaveDataUncompressedSize + nPaddingCount && nCompressedPos < m_nWaveDataCompressedSize; nUncompressedPos++)
	{
		n32 nCurrSample = 0;
		if (nUncompressedPos % 64 == 0)
		{
			nCurrSample = m_vSoundDataCompressed[nCompressedPos++];
		}
		else
		{
			n32 nDeltaSquareRoot = 0;
			if (nUncompressedPos % 2 == 1)
			{
				nDeltaSquareRoot = m_vSoundDataCompressed[nCompressedPos++] & 0xF;
			}
			else
			{
				nDeltaSquareRoot = m_vSoundDataCompressed[nCompressedPos] >> 4 & 0xF;
			}
			if ((nDeltaSquareRoot & 0x8) != 0)
			{
				nDeltaSquareRoot |= 0xFFFFFFF0;
			}
			n32 nDelta = nDeltaSquareRoot * nDeltaSquareRoot;
			if (nDeltaSquareRoot < 0)
			{
				nCurrSample = nPrevSample - nDelta;
			}
			else
			{
				nCurrSample = nPrevSample + nDelta;
			}
		}
		if (nUncompressedPos < m_nWaveDataUncompressedSize)
		{
			vSoundDataWorking[nUncompressedPos] = static_cast<n16>(nCurrSample);
		}
		nPrevSample = nCurrSample;
	}
	if (nUncompressedPos != m_nWaveDataUncompressedSize + nPaddingCount || nCompressedPos != m_nWaveDataCompressedSize)
	{
		UPrintf(USTR("ERROR: test uncompress error\n\n"));
		return false;
	}
	m_vSoundDataCompressed.swap(vSoundDataWorking);
	m_nWaveDataCompressedSize = m_nWaveDataUncompressedSize;
	// uncompress end
	return true;
}

bool CConverter::writeS() const
{
	FILE* fp = UFopen(m_sOutputFileName.c_str(), USTR("w"), true);
	if (fp == nullptr)
	{
		return false;
	}
	fprintf(fp, "#TONE NAME     : %s\n", m_sLabel.c_str());
	fprintf(fp, "#FREQUENCY     : ");
	if (m_fWave == 1.0)
	{
		fprintf(fp, "%g\n", m_fSampleRate);
	}
	else
	{
		fprintf(fp, "%g (%g * %g)\n", m_fSampleRate * m_fWave, m_fSampleRate, m_fWave);
	}
	fprintf(fp, "#BASE NOTE#    : %d\n", m_InstrumentChunkData.BaseNote);
	fprintf(fp, "#START ADDRESS : %06d\n", m_SoundDataChunkData0.Offset);
	fprintf(fp, "#LOOP ADDRESS  : %06d\n", m_uLoopPosition);
	fprintf(fp, "#END ADDRESS   : %06d\n", m_uEndPosition);
	fprintf(fp, "#LOOP MODE     : ");
	switch (m_InstrumentChunkData.SustainLoop.PlayMode)
	{
	case kPlayModeNoLooping:
		fprintf(fp, "1Shot\n");
		break;
	case kPlayModeForwardLooping:
		fprintf(fp, "Fwd\n");
		break;
	case kPlayModeForwardBackwardLooping:
	default:
		fprintf(fp, "???\n");
		break;
	}
	fprintf(fp, "#FINE TUNE     : %d\n", m_InstrumentChunkData.Detune);
	fprintf(fp, "#WAVE EXP/COMP : %g\n", m_fWave);
	fprintf(fp, "#VOL EXP/COMP  : %g\n", m_fVolume);
	if (m_nType != 0)
	{
		if (m_sAlgorithm == s_sPackingAlgorithmAif2Agb_1_05)
		{
			fprintf(fp, "#PACKED RATE   : %g %% (retry %d)\n", static_cast<f64>(m_nWaveDataCompressedSize) / static_cast<f64>(m_nWaveDataUncompressedSize) * 100.0, m_nRetryCount);
		}
		else
		{
			fprintf(fp, "#X-PACKED RATE : %g %% (retry %d)\n", static_cast<f64>(m_nWaveDataCompressedSize) / static_cast<f64>(m_nWaveDataUncompressedSize) * 100.0, m_nRetryCount);
		}
	}
	fprintf(fp, "\n\t.section .rodata\n");
	fprintf(fp, "\t.global\t%s\n", m_sLabel.c_str());
	fprintf(fp, "\t.align\t2\n");
	fprintf(fp, "\n%s:\n", m_sLabel.c_str());
	fprintf(fp, "\t.short\t0x%04X\n", m_WaveDataHeader.Type);
	fprintf(fp, "\t.short\t0x%04X\n", m_WaveDataHeader.Stat);
	fprintf(fp, "\t.int\t%d\n", m_WaveDataHeader.Freq);
	fprintf(fp, "\t.int\t%d\n", m_WaveDataHeader.Loop);
	fprintf(fp, "\t.int\t%d\n\n", m_WaveDataHeader.Size);
	n32 nCompressedSize = m_nWaveDataCompressedSize;
	if (m_sAlgorithm == s_sPackingAlgorithmAif2Agb_1_05)
	{
		nCompressedSize = m_nWaveDataUncompressedSize;
	}
	for (n32 i = 0; i < nCompressedSize; i++)
	{
		if (i % 8 == 0)
		{
			fprintf(fp, "\t.byte\t");
		}
		fprintf(fp, "0x%02X", static_cast<u8>(m_vSoundDataCompressed[i]));
		if (i % 8 == 7 || i == nCompressedSize - 1)
		{
			fprintf(fp, "\n");
		}
		else
		{
			fprintf(fp, ",");
		}
	}
	if (m_sAlgorithm != s_sPackingAlgorithmPok_Aif_1_06a_006 || m_nType == 0)
	{
		n16 nLastByte = 0;
		if (m_InstrumentChunkData.SustainLoop.PlayMode != kPlayModeNoLooping)
		{
			if (m_sAlgorithm == s_sPackingAlgorithmAif2Agb_1_05 || m_nType == 0)
			{
				nLastByte = m_vSoundDataCompressed[m_nWaveDataCompressedLoopPosition];
			}
			else
			{
				nLastByte = static_cast<n16>(m_nCompressedLastByte);
			}
		}
		fprintf(fp, "\n\t.byte\t0x%02X\n", static_cast<u8>(nLastByte));
	}
	fprintf(fp, "\n\t.end\n");
	fclose(fp);
	return true;
}

bool CConverter::writeBin() const
{
	FILE* fp = UFopen(m_sOutputFileName.c_str(), USTR("wb"), true);
	if (fp == nullptr)
	{
		return false;
	}
	fwrite(&m_WaveDataHeader, sizeof(m_WaveDataHeader), 1, fp);
	n32 nCompressedSize = m_nWaveDataCompressedSize;
	if (m_sAlgorithm == s_sPackingAlgorithmAif2Agb_1_05)
	{
		nCompressedSize = m_nWaveDataUncompressedSize;
	}
	for (n32 i = 0; i < nCompressedSize; i++)
	{
		fwrite(reinterpret_cast<const u8*>(&*m_vSoundDataCompressed.begin() + i), 1, 1, fp);
	}
	if (m_sAlgorithm != s_sPackingAlgorithmPok_Aif_1_06a_006 || m_nType == 0)
	{
		n16 nLastByte = 0;
		if (m_InstrumentChunkData.SustainLoop.PlayMode != kPlayModeNoLooping)
		{
			if (m_sAlgorithm == s_sPackingAlgorithmAif2Agb_1_05 || m_nType == 0)
			{
				nLastByte = m_vSoundDataCompressed[m_nWaveDataCompressedLoopPosition];
			}
			else
			{
				nLastByte = static_cast<n16>(m_nCompressedLastByte);
			}
		}
		fwrite(&nLastByte, 1, 1, fp);
	}
	fclose(fp);
	return true;
}

bool CConverter::readBin()
{
	FILE* fp = UFopen(m_sInputFileName.c_str(), USTR("rb"), true);
	if (fp == nullptr)
	{
		return false;
	}
	Fseek(fp, 0, SEEK_END);
	u32 uFileSize = static_cast<u32>(Ftell(fp));
	if (uFileSize == 0)
	{
		fclose(fp);
		UPrintf(USTR("ERROR: file %") PRIUS USTR(" is empty\n\n"), m_sInputFileName.c_str());
		return false;
	}
	vector<u8> vFile(uFileSize);
	Fseek(fp, 0, SEEK_SET);
	fread(&*vFile.begin(), 1, uFileSize, fp);
	fclose(fp);
	u32 uFilePos = 0;
	if (uFilePos + sizeof(m_WaveDataHeader) > uFileSize)
	{
		UPrintf(USTR("ERROR: file %") PRIUS USTR(" is too small\n\n"), m_sInputFileName.c_str());
		return false;
	}
	memcpy(&m_WaveDataHeader, &*vFile.begin() + uFilePos, sizeof(m_WaveDataHeader));
	uFilePos += sizeof(m_WaveDataHeader);
	m_nType = m_WaveDataHeader.Type;
	m_InstrumentChunkData.SustainLoop.PlayMode = m_WaveDataHeader.Stat >> 14 & 0x3;
	m_fFrequency = static_cast<f64>(m_WaveDataHeader.Freq);
	m_nWaveDataUncompressedLoopPosition = m_WaveDataHeader.Loop;
	m_nWaveDataUncompressedSize = m_WaveDataHeader.Size;
	m_nWaveDataCompressedLoopPosition = m_WaveDataHeader.Loop;
	m_nWaveDataCompressedSize = m_WaveDataHeader.Size;
	m_InstrumentChunkData.BaseNote = 60;
	m_InstrumentChunkData.Detune = 0;
	m_fSampleRate = m_fFrequency / (pow(2.0, (m_InstrumentChunkData.Detune + 100 * (180 - m_InstrumentChunkData.BaseNote)) / 1200.0) * m_fWave);
	u32 uWaveDataSize = uFileSize - uFilePos;
	m_vSoundDataCompressed.resize(uWaveDataSize);
	for (u32 i = 0; i < uWaveDataSize; i++)
	{
		m_vSoundDataCompressed[i] = vFile[uFilePos + i];
	}
	uFilePos += uWaveDataSize;
	if (m_nType != 0 && m_nType != 1 && m_nType != 11)
	{
		UPrintf(USTR("ERROR: file %") PRIUS USTR(" has invalid type\n\n"), m_sInputFileName.c_str());
		return false;
	}
	string sAlgorithm;
	bool bUncompress = false;
	if (m_nType == 11)
	{
		if (!m_sAlgorithm.empty() && m_sAlgorithm != s_sPackingAlgorithmPok_Aif_1_06a_006)
		{
			UPrintf(USTR("ERROR: file %") PRIUS USTR(" do not support algorithm %") PRIUS USTR("\n\n"), m_sInputFileName.c_str(), AToU(m_sAlgorithm).c_str());
			return false;
		}
		// uncompressed
		if (m_nWaveDataUncompressedSize != uWaveDataSize)
		{
			UPrintf(USTR("ERROR: file %") PRIUS USTR(" has invalid size\n\n"), m_sInputFileName.c_str());
			return false;
		}
		sAlgorithm = s_sPackingAlgorithmPok_Aif_1_06a_006;
	}
	else if (m_nType == 1)
	{
		vector<string> vAlgorithm;
		if (!m_sAlgorithm.empty())
		{
			vAlgorithm.push_back(m_sAlgorithm);
		}
		else
		{
			vAlgorithm.push_back(s_sPackingAlgorithmPok_Aif_1_06a_006);
			vAlgorithm.push_back(s_sPackingAlgorithmAif2Agb_1_06a);
			vAlgorithm.push_back(s_sPackingAlgorithmAif2Agb_1_05);
		}
		bool bFound = false;
		for (n32 i = 0; i < static_cast<n32>(vAlgorithm.size()); i++)
		{
			sAlgorithm = vAlgorithm[i];
			if (sAlgorithm == s_sPackingAlgorithmPok_Aif_1_06a_006)
			{
				do
				{
					n32 nPaddingCount = 1;
					n32 nCompressedSize = (m_nWaveDataUncompressedSize + nPaddingCount) / 64 * 33;
					n32 nRemainder = (m_nWaveDataUncompressedSize + nPaddingCount) % 64;
					if (nRemainder <= 2)
					{
						nCompressedSize += nRemainder;
					}
					else
					{
						nPaddingCount = (nRemainder - 2) % 2 == 0 ? 1 : 0;
						nCompressedSize += 2 + (nRemainder - 2) / 2;
					}
					if (nCompressedSize != uWaveDataSize)
					{
						break;
					}
					if (!lossyUncompressSoundDataPokAif106a006())
					{
						break;
					}
					bFound = true;
					bUncompress = true;
					sAlgorithm = s_sPackingAlgorithmPok_Aif_1_06a_006;
				} while (false);
			}
			else if (sAlgorithm == s_sPackingAlgorithmAif2Agb_1_06a)
			{
				do
				{
					if (m_nWaveDataCompressedSize + 1 != uWaveDataSize)
					{
						break;
					}
					if (!lossyUncompressSoundDataAif2Agb106a())
					{
						break;
					}
					bFound = true;
					bUncompress = true;
					sAlgorithm = s_sPackingAlgorithmAif2Agb_1_06a;
				} while (false);
			}
			else if (sAlgorithm == s_sPackingAlgorithmAif2Agb_1_05)
			{
				do
				{
					// uncompressed
					if (m_nWaveDataUncompressedSize + 1 != uWaveDataSize)
					{
						break;
					}
					bFound = true;
					sAlgorithm = s_sPackingAlgorithmAif2Agb_1_05;
				} while (false);
			}
			if (bFound)
			{
				break;
			}
		}
		if (!bFound)
		{
			if (!m_sAlgorithm.empty())
			{
				UPrintf(USTR("ERROR: file %") PRIUS USTR(" do not support algorithm %") PRIUS USTR("\n\n"), m_sInputFileName.c_str(), AToU(m_sAlgorithm).c_str());
				return false;
			}
			else
			{
				UPrintf(USTR("ERROR: can not determine algorithm for file %") PRIUS USTR("\n\n"), m_sInputFileName.c_str());
				return false;
			}
		}
	}
	else if (m_nType == 0)
	{
		// uncompressed
		if (m_nWaveDataUncompressedSize + 1 != uWaveDataSize)
		{
			UPrintf(USTR("ERROR: file %") PRIUS USTR(" has invalid size\n\n"), m_sInputFileName.c_str());
			return false;
		}
		sAlgorithm = s_sPackingAlgorithmAif2Agb_1_05;
	}
	if (sAlgorithm != s_sPackingAlgorithmPok_Aif_1_06a_006 || m_nType == 0)
	{
		n16 nLastByte = 0;
		if (m_InstrumentChunkData.SustainLoop.PlayMode != kPlayModeNoLooping)
		{
			if (sAlgorithm == s_sPackingAlgorithmAif2Agb_1_05 || m_nType == 0)
			{
				nLastByte = m_vSoundDataCompressed[m_nWaveDataCompressedLoopPosition];
			}
			else
			{
				nLastByte = static_cast<n16>(m_nCompressedLastByte);
			}
		}
		if (m_vSoundDataCompressed[m_nWaveDataCompressedSize] != (nLastByte & 0xFF))
		{
			UPrintf(USTR("ERROR: file %") PRIUS USTR(" has invalid last byte\n\n"), m_sInputFileName.c_str());
			return false;
		}
	}
	if (!bUncompress)
	{
		m_vSoundDataOriginal.resize(m_nWaveDataUncompressedSize);
		for (n32 i = 0; i < m_nWaveDataUncompressedSize; i++)
		{
			m_vSoundDataOriginal[i] = m_vSoundDataCompressed[i] << 8;
		}
		bUncompress = true;
	}
	if (m_nSampleSize != 8 && m_nSampleSize != 16)
	{
		n16 nMask = 0xFFFF << (16 - m_nSampleSize);
		for (n32 i = 0; i < m_nWaveDataUncompressedSize; i++)
		{
			m_vSoundDataOriginal[i] &= nMask;
		}
	}
	m_uEndPosition = m_nWaveDataUncompressedSize;
	m_uLoopPosition = m_nWaveDataUncompressedLoopPosition;
	m_CommonChunkData.NumChannels = SDW_CONVERT_ENDIAN16(1);
	m_CommonChunkData.NumSampleFrames = SDW_CONVERT_ENDIAN32(m_uEndPosition);
	m_CommonChunkData.SampleSize = SDW_CONVERT_ENDIAN16(m_nSampleSize);
	DoubleToFloat80(m_fSampleRate, m_CommonChunkData.SampleRate, kFloat80EndiannessBigEndian);
	n16 nNumMarkers = 0;
	m_MarkerChunkData.Markers.resize(nNumMarkers);
	if (m_InstrumentChunkData.SustainLoop.PlayMode != kPlayModeNoLooping)
	{
		nNumMarkers = 2;
		m_MarkerChunkData.Markers.resize(nNumMarkers);
		m_MarkerChunkData.Markers[0].Id = SDW_CONVERT_ENDIAN16(1);
		m_MarkerChunkData.Markers[0].Position = SDW_CONVERT_ENDIAN32(m_uLoopPosition);
		m_MarkerChunkData.Markers[0].MarkerName = "beg loop";
		m_MarkerChunkData.Markers[0].ByteCount = static_cast<u8>(m_MarkerChunkData.Markers[0].MarkerName.size());
		m_MarkerChunkData.Markers[1].Id = SDW_CONVERT_ENDIAN16(2);
		m_MarkerChunkData.Markers[1].Position = SDW_CONVERT_ENDIAN32(m_uEndPosition);
		m_MarkerChunkData.Markers[1].MarkerName = "end loop";
		m_MarkerChunkData.Markers[1].ByteCount = static_cast<u8>(m_MarkerChunkData.Markers[1].MarkerName.size());
	}
	m_MarkerChunkData.NumMarkers = SDW_CONVERT_ENDIAN16(nNumMarkers);
	m_InstrumentChunkData.LowNote = 0;
	m_InstrumentChunkData.HighNote = 127;
	m_InstrumentChunkData.LowVelocity = 0;
	m_InstrumentChunkData.HighVelocity = 127;
	if (m_InstrumentChunkData.SustainLoop.PlayMode != kPlayModeNoLooping)
	{
		m_InstrumentChunkData.SustainLoop.BeginLoop = SDW_CONVERT_ENDIAN16(1);
		m_InstrumentChunkData.SustainLoop.EndLoop = SDW_CONVERT_ENDIAN16(2);
	}
	m_InstrumentChunkData.SustainLoop.PlayMode = SDW_CONVERT_ENDIAN16(m_InstrumentChunkData.SustainLoop.PlayMode);
	m_SoundDataChunkData0.Offset = SDW_CONVERT_ENDIAN32(0);
	m_SoundDataChunkData0.BlockSize = SDW_CONVERT_ENDIAN32(0);
	if (m_nSampleSize > 8)
	{
		m_SoundDataChunkData1.SoundData.resize(m_uEndPosition * 2);
		for (u32 i = 0; i < m_uEndPosition; i++)
		{
			n16 nSoundData = m_vSoundDataOriginal[i];
			*(reinterpret_cast<n16*>(&*m_SoundDataChunkData1.SoundData.begin()) + i) = SDW_CONVERT_ENDIAN16(nSoundData);
		}
	}
	else
	{
		m_SoundDataChunkData1.SoundData.resize(m_uEndPosition);
		for (u32 i = 0; i < m_uEndPosition; i++)
		{
			n16 nSoundData = m_vSoundDataOriginal[i];
			m_SoundDataChunkData1.SoundData[i] = static_cast<u8>(nSoundData >> 8 & 0xFF);
		}
	}
	return true;
}

bool CConverter::lossyUncompressSoundDataAif2Agb106a()
{
	bool bResult = true;
	n32 nUncompressedSize = 0;
	n32 nCompressedPos = 0;
	do
	{
		if (m_nWaveDataCompressedSize - nCompressedPos < 1)
		{
			break;
		}
		n16 nFlags = m_vSoundDataCompressed[nCompressedPos++];
		n32 nLengthMin = 1;
		n32 nLength = nFlags >> 6 & 0x3;
		if (nLength == 0)
		{
			nLengthMin = 0x3 + 1;
			nLength = nFlags & 0x3F;
			if (nLength <= 3)
			{
				if (m_nWaveDataCompressedSize - nCompressedPos < 1)
				{
					bResult = false;
					break;
				}
				nLengthMin = 0x3F + 1;
				nLength = (nLength << 8) | (m_vSoundDataCompressed[nCompressedPos++] & 0xFF);
			}
		}
		if (nLength < nLengthMin)
		{
			bResult = false;
			break;
		}
		if (nLength > 3)
		{
			if (m_nWaveDataCompressedSize - nCompressedPos < 1)
			{
				bResult = false;
				break;
			}
			nCompressedPos++;
		}
		nUncompressedSize += nLength;
	} while (true);
	if (!bResult)
	{
		return false;
	}
	vector<n16> vSoundDataWorking(nUncompressedSize);
	bool bFoundUncompressedLoopPosition = false;
	if (m_InstrumentChunkData.SustainLoop.PlayMode == kPlayModeNoLooping)
	{
		bFoundUncompressedLoopPosition = true;
	}
	// uncompress begin
	n32 nUncompressedPos = 0;
	n32 nPrevSample = 0;
	nCompressedPos = 0;
	while (nUncompressedSize - nUncompressedPos > 0)
	{
		if (m_nWaveDataCompressedSize - nCompressedPos < 1)
		{
			bResult = false;
			break;
		}
		bool bFoundCompressedLoopPosition = false;
		if (!bFoundUncompressedLoopPosition)
		{
			if (nCompressedPos == m_nWaveDataCompressedLoopPosition)
			{
				bFoundCompressedLoopPosition = true;
			}
			else if (nCompressedPos > m_nWaveDataCompressedLoopPosition)
			{
				bResult = false;
				break;
			}
		}
		n16 nFlags = m_vSoundDataCompressed[nCompressedPos++];
		n32 nLengthMin = 1;
		n32 nLength = nFlags >> 6 & 0x3;
		n32 nCurrSample = 0;
		if (nLength == 0)
		{
			nLengthMin = 0x3 + 1;
			nLength = nFlags & 0x3F;
			if (nLength <= 3)
			{
				if (m_nWaveDataCompressedSize - nCompressedPos < 1)
				{
					bResult = false;
					break;
				}
				nLengthMin = 0x3F + 1;
				nLength = (nLength << 8) | (m_vSoundDataCompressed[nCompressedPos++] & 0xFF);
			}
		}
		if (nLength < nLengthMin)
		{
			bResult = false;
			break;
		}
		if (nLength <= 3)
		{
			n32 nCompressedDelta = nFlags & 0x3F;
			if ((nCompressedDelta & 0x20) != 0)
			{
				nCompressedDelta |= 0xFFFFFFC0;
			}
			n32 nDelta = 0;
			if (nCompressedDelta < 0)
			{
				nDelta = -(static_cast<n32>(static_cast<n64>(pow(-nCompressedDelta, 1.6))) << 8);
			}
			else
			{
				nDelta = static_cast<n32>(static_cast<n64>(pow(nCompressedDelta, 1.6))) << 8;
			}
			nCurrSample = nPrevSample + nDelta;
		}
		else
		{
			if (m_nWaveDataCompressedSize - nCompressedPos < 1)
			{
				bResult = false;
				break;
			}
			nCurrSample = m_vSoundDataCompressed[nCompressedPos++] << 8;
		}
		if ((nCurrSample & 0x8000) != 0)
		{
			nCurrSample |= 0xFFFF0000;
		}
		if (nLength > nUncompressedSize - nUncompressedPos)
		{
			bResult = false;
			break;
		}
		n32 nDelta = nCurrSample - nPrevSample;
		for (n32 i = 0; i < nLength; i++)
		{
			vSoundDataWorking[nUncompressedPos++] = static_cast<n16>(nPrevSample + nDelta * (i + 1) / nLength);
		}
		if (bFoundCompressedLoopPosition)
		{
			bFoundUncompressedLoopPosition = true;
			m_nWaveDataUncompressedLoopPosition = nUncompressedPos - 1;
			m_nCompressedLastByte = nPrevSample >> 8;
		}
		nPrevSample = nCurrSample;
	}
	if (bResult)
	{
		n16 nLastByte = 0;
		if (m_InstrumentChunkData.SustainLoop.PlayMode != kPlayModeNoLooping)
		{
			nLastByte = static_cast<n16>(m_nCompressedLastByte);
		}
		if (m_vSoundDataCompressed[m_nWaveDataCompressedSize] != (nLastByte & 0xFF))
		{
			bFoundUncompressedLoopPosition = false;
			m_nWaveDataUncompressedLoopPosition = m_nWaveDataCompressedLoopPosition;
		}
		if (!bFoundUncompressedLoopPosition || nUncompressedPos != nUncompressedSize || nCompressedPos != m_nWaveDataCompressedSize)
		{
			bResult = false;
		}
	}
	if (!bResult)
	{
		return false;
	}
	m_vSoundDataOriginal.swap(vSoundDataWorking);
	m_nWaveDataUncompressedSize = nUncompressedSize;
	// uncompress end
	return true;
}

bool CConverter::lossyUncompressSoundDataPokAif106a006()
{
	n32 nPaddingCount = 1;
	n32 nCompressedSize = (m_nWaveDataUncompressedSize + nPaddingCount) / 64 * 33;
	n32 nRemainder = (m_nWaveDataUncompressedSize + nPaddingCount) % 64;
	if (nRemainder <= 2)
	{
		nCompressedSize += nRemainder;
	}
	else
	{
		nPaddingCount = (nRemainder - 2) % 2 == 0 ? 1 : 0;
		nCompressedSize += 2 + (nRemainder - 2) / 2;
	}
	n32 nPrevSample = 0;
	// uncompress begin
	vector<n16> vSoundDataWorking(m_nWaveDataUncompressedSize);
	n32 nUncompressedPos = 0;
	n32 nCompressedPos = 0;
	for (; nUncompressedPos < m_nWaveDataUncompressedSize + nPaddingCount && nCompressedPos < m_nWaveDataCompressedSize; nUncompressedPos++)
	{
		n32 nCurrSample = 0;
		if (nUncompressedPos % 64 == 0)
		{
			nCurrSample = m_vSoundDataCompressed[nCompressedPos++];
		}
		else
		{
			n32 nDeltaSquareRoot = 0;
			if (nUncompressedPos % 2 == 1)
			{
				nDeltaSquareRoot = m_vSoundDataCompressed[nCompressedPos++] & 0xF;
			}
			else
			{
				nDeltaSquareRoot = m_vSoundDataCompressed[nCompressedPos] >> 4 & 0xF;
			}
			if ((nDeltaSquareRoot & 0x8) != 0)
			{
				nDeltaSquareRoot |= 0xFFFFFFF0;
			}
			n32 nDelta = nDeltaSquareRoot * nDeltaSquareRoot;
			if (nDeltaSquareRoot < 0)
			{
				nCurrSample = nPrevSample - nDelta;
			}
			else
			{
				nCurrSample = nPrevSample + nDelta;
			}
		}
		if (nUncompressedPos < m_nWaveDataUncompressedSize)
		{
			vSoundDataWorking[nUncompressedPos] = static_cast<n16>(nCurrSample << 8);
		}
		nPrevSample = nCurrSample;
	}
	if (nUncompressedPos != m_nWaveDataUncompressedSize + nPaddingCount || nCompressedPos != nCompressedSize)
	{
		return false;
	}
	m_vSoundDataOriginal.swap(vSoundDataWorking);
	m_nWaveDataCompressedSize = nCompressedSize;
	// uncompress end
	return true;
}

bool CConverter::writeAif() const
{
	SChunkHeaderInfo formChunkHeaderInfo;
	formChunkHeaderInfo.ChunkId = SDW_CONVERT_ENDIAN32('FORM');
	formChunkHeaderInfo.ChunkSize = 0;
	SFormChunkData formChunkData;
	formChunkData.FormType = SDW_CONVERT_ENDIAN32('AIFF');
	formChunkHeaderInfo.ChunkSize += sizeof(formChunkData);
	SChunkHeaderInfo commonChunkHeaderInfo;
	commonChunkHeaderInfo.ChunkId = SDW_CONVERT_ENDIAN32('COMM');
	commonChunkHeaderInfo.ChunkSize = sizeof(m_CommonChunkData);
	formChunkHeaderInfo.ChunkSize = static_cast<n32>(Align(formChunkHeaderInfo.ChunkSize, 2)) + sizeof(commonChunkHeaderInfo) + commonChunkHeaderInfo.ChunkSize;
	SChunkHeaderInfo markerChunkHeaderInfo;
	markerChunkHeaderInfo.ChunkId = SDW_CONVERT_ENDIAN32('MARK');
	markerChunkHeaderInfo.ChunkSize = sizeof(m_MarkerChunkData.NumMarkers);
	n16 nNumMarkers = SDW_CONVERT_ENDIAN16(m_MarkerChunkData.NumMarkers);
	for (n32 i = 0; i < nNumMarkers; i++)
	{
		markerChunkHeaderInfo.ChunkSize += sizeof(m_MarkerChunkData.Markers[i].Id) + sizeof(m_MarkerChunkData.Markers[i].Position) + sizeof(m_MarkerChunkData.Markers[i].ByteCount) + m_MarkerChunkData.Markers[i].ByteCount;
		if (m_MarkerChunkData.Markers[i].ByteCount % 2 == 0)
		{
			markerChunkHeaderInfo.ChunkSize += sizeof(m_MarkerChunkData.Markers[i].Padding);
		}
	}
	formChunkHeaderInfo.ChunkSize = static_cast<n32>(Align(formChunkHeaderInfo.ChunkSize, 2)) + sizeof(markerChunkHeaderInfo) + markerChunkHeaderInfo.ChunkSize;
	SChunkHeaderInfo instrumentChunkHeaderInfo;
	instrumentChunkHeaderInfo.ChunkId = SDW_CONVERT_ENDIAN32('INST');
	instrumentChunkHeaderInfo.ChunkSize = sizeof(m_InstrumentChunkData);
	formChunkHeaderInfo.ChunkSize = static_cast<n32>(Align(formChunkHeaderInfo.ChunkSize, 2)) + sizeof(instrumentChunkHeaderInfo) + instrumentChunkHeaderInfo.ChunkSize;
	SChunkHeaderInfo soundDataChunkHeaderInfo;
	soundDataChunkHeaderInfo.ChunkId = SDW_CONVERT_ENDIAN32('SSND');
	soundDataChunkHeaderInfo.ChunkSize = sizeof(m_SoundDataChunkData0) + static_cast<n32>(m_SoundDataChunkData1.SoundData.size());
	formChunkHeaderInfo.ChunkSize = static_cast<n32>(Align(formChunkHeaderInfo.ChunkSize, 2)) + sizeof(soundDataChunkHeaderInfo) + soundDataChunkHeaderInfo.ChunkSize;
	formChunkHeaderInfo.ChunkSize = SDW_CONVERT_ENDIAN32(formChunkHeaderInfo.ChunkSize);
	commonChunkHeaderInfo.ChunkSize = SDW_CONVERT_ENDIAN32(commonChunkHeaderInfo.ChunkSize);
	markerChunkHeaderInfo.ChunkSize = SDW_CONVERT_ENDIAN32(markerChunkHeaderInfo.ChunkSize);
	instrumentChunkHeaderInfo.ChunkSize = SDW_CONVERT_ENDIAN32(instrumentChunkHeaderInfo.ChunkSize);
	soundDataChunkHeaderInfo.ChunkSize = SDW_CONVERT_ENDIAN32(soundDataChunkHeaderInfo.ChunkSize);
	FILE* fp = UFopen(m_sOutputFileName.c_str(), USTR("wb"), true);
	if (fp == nullptr)
	{
		return false;
	}
	fwrite(&formChunkHeaderInfo, sizeof(formChunkHeaderInfo), 1, fp);
	fwrite(&formChunkData, sizeof(formChunkData), 1, fp);
	n64 nFileSize = Ftell(fp);
	Seek(fp, Align(nFileSize, 2));
	fwrite(&commonChunkHeaderInfo, sizeof(commonChunkHeaderInfo), 1, fp);
	fwrite(&m_CommonChunkData, sizeof(m_CommonChunkData), 1, fp);
	nFileSize = Ftell(fp);
	Seek(fp, Align(nFileSize, 2));
	fwrite(&markerChunkHeaderInfo, sizeof(markerChunkHeaderInfo), 1, fp);
	fwrite(&m_MarkerChunkData.NumMarkers, sizeof(m_MarkerChunkData.NumMarkers), 1, fp);
	for (n32 i = 0; i < nNumMarkers; i++)
	{
		fwrite(&m_MarkerChunkData.Markers[i].Id, sizeof(m_MarkerChunkData.Markers[i].Id), 1, fp);
		fwrite(&m_MarkerChunkData.Markers[i].Position, sizeof(m_MarkerChunkData.Markers[i].Position), 1, fp);
		fwrite(&m_MarkerChunkData.Markers[i].ByteCount, sizeof(m_MarkerChunkData.Markers[i].ByteCount), 1, fp);
		fwrite(m_MarkerChunkData.Markers[i].MarkerName.c_str(), m_MarkerChunkData.Markers[i].ByteCount, 1, fp);
		if (m_MarkerChunkData.Markers[i].ByteCount % 2 == 0)
		{
			fwrite(&m_MarkerChunkData.Markers[i].Padding, sizeof(m_MarkerChunkData.Markers[i].Padding), 1, fp);
		}
	}
	nFileSize = Ftell(fp);
	Seek(fp, Align(nFileSize, 2));
	fwrite(&instrumentChunkHeaderInfo, sizeof(instrumentChunkHeaderInfo), 1, fp);
	fwrite(&m_InstrumentChunkData, sizeof(m_InstrumentChunkData), 1, fp);
	nFileSize = Ftell(fp);
	Seek(fp, Align(nFileSize, 2));
	fwrite(&soundDataChunkHeaderInfo, sizeof(soundDataChunkHeaderInfo), 1, fp);
	fwrite(&m_SoundDataChunkData0, sizeof(m_SoundDataChunkData0), 1, fp);
	if (!m_SoundDataChunkData1.SoundData.empty())
	{
		fwrite(&*m_SoundDataChunkData1.SoundData.begin(), m_SoundDataChunkData1.SoundData.size(), 1, fp);
	}
	fclose(fp);
	return true;
}
