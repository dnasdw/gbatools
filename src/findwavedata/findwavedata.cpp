#include "findwavedata.h"

const n32 CFindWaveData::s_nWaveDataSizeMax = 0xFFFF;
const string CFindWaveData::s_sPackingAlgorithmAif2Agb_1_06a = "aif2agb_1.06a";
const string CFindWaveData::s_sPackingAlgorithmPok_Aif_1_06a_006 = "pok_aif_1.06a.006";

CFindWaveData::SWaveDataHeader::SWaveDataHeader()
	: Type(0)
	, Stat(0)
	, Freq(0)
	, Loop(0)
	, Size(0)
{
}

CFindWaveData::CFindWaveData()
	: m_uFilePos(0)
	, m_nType(0)
	, m_nPlayMode(0)
	, m_nWaveDataUncompressedSize(0)
	, m_nWaveDataCompressedSize(0)
	, m_nWaveDataUncompressedLoopPosition(0)
	, m_nWaveDataCompressedLoopPosition(0)
	, m_nCompressedLastByte(0)
{
}

CFindWaveData::~CFindWaveData()
{
}

void CFindWaveData::SetInputFileName(const UString& a_sInputFileName)
{
	m_sInputFileName = a_sInputFileName;
}

void CFindWaveData::SetOutputDirName(const UString& a_sOutputDirName)
{
	m_sOutputDirName = a_sOutputDirName;
}

bool CFindWaveData::Find()
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
	m_vFile.resize(uFileSize);
	Fseek(fp, 0, SEEK_SET);
	fread(&*m_vFile.begin(), 1, uFileSize, fp);
	fclose(fp);
	set<u32> sFreq;
	for (n32 i = static_cast<n32>(ceil((180 - 128) / 12.0)); i <= (180 / 12); i++)
	{
		sFreq.insert(5734 << i);
		sFreq.insert(7884 << i);
		sFreq.insert(10512 << i);
		sFreq.insert(13379 << i);
		sFreq.insert(15768 << i);
		sFreq.insert(18157 << i);
		sFreq.insert(21024 << i);
		sFreq.insert(26758 << i);
		sFreq.insert(31536 << i);
		sFreq.insert(36314 << i);
		sFreq.insert(40137 << i);
		sFreq.insert(42048 << i);
		//sFreq.insert(44100 << i);
	}
	map<u32, u32> mOffsetSize0;
	map<u32, u32> mOffsetSize1;
	set<u32> sValidBeginOffset;
	m_uFilePos = 0;
	while (m_uFilePos + 16 <= uFileSize)
	{
		SWaveDataHeader* pWaveDataHeader = reinterpret_cast<SWaveDataHeader*>(&*m_vFile.begin() + m_uFilePos);
		if (pWaveDataHeader->Type != 0 && pWaveDataHeader->Type != 1)
		{
			m_uFilePos += 4;
			continue;
		}
		if (pWaveDataHeader->Stat != 0 && pWaveDataHeader->Stat != 0x4000)
		{
			m_uFilePos += 4;
			continue;
		}
		bool bFoundFreq = sFreq.find(pWaveDataHeader->Freq) != sFreq.end();
		if (pWaveDataHeader->Stat == 0 && pWaveDataHeader->Loop != 0)
		{
			m_uFilePos += 4;
			continue;
		}
		if (static_cast<n32>(pWaveDataHeader->Loop) < 0 || static_cast<n32>(pWaveDataHeader->Size) < 0)
		{
			m_uFilePos += 4;
			continue;
		}
		if (pWaveDataHeader->Loop >= pWaveDataHeader->Size)
		{
			m_uFilePos += 4;
			continue;
		}
		if (pWaveDataHeader->Size > s_nWaveDataSizeMax)
		{
			m_uFilePos += 4;
			continue;
		}
		m_nType = pWaveDataHeader->Type;
		m_nPlayMode = pWaveDataHeader->Stat >> 14 & 0x3;
		m_nWaveDataCompressedLoopPosition = pWaveDataHeader->Loop;
		m_nWaveDataCompressedSize = pWaveDataHeader->Size;
		m_nWaveDataUncompressedLoopPosition = pWaveDataHeader->Loop;
		m_nWaveDataUncompressedSize = pWaveDataHeader->Size;
		string sAlgorithm;
		if (m_nType == 1)
		{
			vector<string> vAlgorithm;
			vAlgorithm.push_back(s_sPackingAlgorithmAif2Agb_1_06a);
			vAlgorithm.push_back(s_sPackingAlgorithmPok_Aif_1_06a_006);
			bool bFound = false;
			for (n32 i = 0; i < static_cast<n32>(vAlgorithm.size()); i++)
			{
				sAlgorithm = vAlgorithm[i];
				if (sAlgorithm == s_sPackingAlgorithmPok_Aif_1_06a_006)
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
					if (m_uFilePos + 16 + nCompressedSize > uFileSize)
					{
						continue;
					}
					m_nWaveDataCompressedSize = nCompressedSize;
					bFound = true;
				}
				else if (sAlgorithm == s_sPackingAlgorithmAif2Agb_1_06a)
				{
					if (m_uFilePos + 16 + m_nWaveDataCompressedSize + 1 > uFileSize)
					{
						continue;
					}
					if (!lossyUncompressSoundDataAif2Agb106a())
					{
						continue;
					}
					bFound = true;
				}
				if (bFound)
				{
					break;
				}
			}
			if (!bFound)
			{
				m_uFilePos += 4;
				continue;
			}
		}
		else if (m_nType == 0)
		{
			if (m_uFilePos + 16 + m_nWaveDataUncompressedSize + 1 > uFileSize)
			{
				m_uFilePos += 4;
				continue;
			}
			sAlgorithm = s_sPackingAlgorithmAif2Agb_1_06a;
		}
		u32 uFilePos = m_uFilePos + 16;
		if (sAlgorithm != s_sPackingAlgorithmPok_Aif_1_06a_006 || m_nType == 0)
		{
			if (m_uFilePos + 16 + m_nWaveDataCompressedSize + 1 > uFileSize)
			{
				m_uFilePos += 4;
				continue;
			}
			n16 nLastByte = 0;
			if (m_nPlayMode != 0)
			{
				if (m_nType == 0)
				{
					nLastByte = m_vFile[m_uFilePos + 16 + m_nWaveDataUncompressedLoopPosition];
				}
				else
				{
					nLastByte = static_cast<n16>(m_nCompressedLastByte);
				}
			}
			if (m_vFile[m_uFilePos + 16 + m_nWaveDataCompressedSize] != (nLastByte & 0xFF))
			{
				m_uFilePos += 4;
				continue;
			}
			uFilePos += m_nWaveDataCompressedSize + 1;
		}
		else
		{
			uFilePos += m_nWaveDataCompressedSize;
		}
		bool bAligned = true;
		n32 nPaddingCount = (4 - uFilePos % 4) % 4;
		if (uFilePos == uFileSize)
		{
			nPaddingCount = 0;
		}
		if (uFilePos + nPaddingCount > uFileSize)
		{
			m_uFilePos += 4;
			continue;
		}
		for (n32 i = 0; i < nPaddingCount; i++)
		{
			if (m_vFile[uFilePos + i] != 0)
			{
				bAligned = false;
				break;
			}
		}
		if (!bAligned)
		{
			m_uFilePos += 4;
			continue;
		}
		if (bFoundFreq)
		{
			mOffsetSize0[m_uFilePos] = uFilePos - m_uFilePos;
			sValidBeginOffset.insert(uFilePos + nPaddingCount);
		}
		else
		{
			mOffsetSize1[m_uFilePos] = uFilePos - m_uFilePos;
			m_uFilePos += 4;
			continue;
		}
		m_uFilePos = uFilePos + nPaddingCount;
	}
	for (map<u32, u32>::const_iterator it = mOffsetSize1.begin(); it != mOffsetSize1.end(); ++it)
	{
		m_uFilePos = it->first;
		if (sValidBeginOffset.find(m_uFilePos) != sValidBeginOffset.end())
		{
			u32 uFilePos = m_uFilePos + it->second;
			n32 nPaddingCount = (4 - uFilePos % 4) % 4;
			if (uFilePos == uFileSize)
			{
				nPaddingCount = 0;
			}
			mOffsetSize0[m_uFilePos] = uFilePos - m_uFilePos;
			sValidBeginOffset.insert(uFilePos + nPaddingCount);
		}
	}
	if (!mOffsetSize0.empty())
	{
		for (map<u32, u32>::const_iterator it = mOffsetSize0.begin(); it != mOffsetSize0.end(); ++it)
		{
			u32 uOffset = it->first;
			UPrintf(USTR("%x\n"), it->first);
		}
		bool bExport = !m_sOutputDirName.empty();
		if (bExport)
		{
			if (!UMakeDir(m_sOutputDirName))
			{
				UPrintf(USTR("ERROR: failed to create directory %") PRIUS USTR("\n\n"), m_sOutputDirName.c_str());
				return false;
			}
			bool bResult = true;
			for (map<u32, u32>::const_iterator it = mOffsetSize0.begin(); it != mOffsetSize0.end(); ++it)
			{
				u32 uOffset = it->first;
				u32 uSize = it->second;
				UString sOutputFileName = m_sOutputDirName + USTR("/") + Format(USTR("%x"), uOffset) + USTR(".bin");
				fp = UFopen(sOutputFileName.c_str(), USTR("wb"), true);
				if (fp == nullptr)
				{
					bResult = false;
					continue;
				}
				fwrite(&*m_vFile.begin() + uOffset, 1, uSize, fp);
				fclose(fp);
			}
			if (!bResult)
			{
				return false;
			}
		}
	}
	return true;
}

int CFindWaveData::Help()
{
	UPrintf(USTR("findwavedata %") PRIUS USTR(" by dnasdw\n\n"), AToU(GBATOOLS_VERSION).c_str());
	UPrintf(USTR("usage: findwavedata input_file [output_dir]\n\n"));
	UPrintf(USTR("sample:\n"));
	UPrintf(USTR("# find and export wave data\n"));
	UPrintf(USTR("findwavedata pokeemerald.gba sound/direct_sound_samples\n\n"));
	UPrintf(USTR("# find wave data only\n"));
	UPrintf(USTR("findwavedata pokeemerald.gba\n\n"));
	return 0;
}

bool CFindWaveData::lossyUncompressSoundDataAif2Agb106a()
{
	bool bResult = true;
	u32 uFilePos = m_uFilePos + 16;
	n32 nUncompressedSize = 0;
	n32 nCompressedPos = 0;
	do
	{
		if (m_nWaveDataCompressedSize - nCompressedPos < 1)
		{
			break;
		}
		n16 nFlags = m_vFile[uFilePos + nCompressedPos++];
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
				nLength = (nLength << 8) | (m_vFile[uFilePos + nCompressedPos++] & 0xFF);
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
	if (nUncompressedSize > s_nWaveDataSizeMax)
	{
		return false;
	}
	bool bFoundUncompressedLoopPosition = false;
	if (m_nPlayMode == 0)
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
		n16 nFlags = m_vFile[uFilePos + nCompressedPos++];
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
				nLength = (nLength << 8) | (m_vFile[uFilePos + nCompressedPos++] & 0xFF);
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
			nCurrSample = m_vFile[uFilePos + nCompressedPos++] << 8;
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
		nUncompressedPos += nLength;
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
		if (m_nPlayMode != 0)
		{
			nLastByte = static_cast<n16>(m_nCompressedLastByte);
		}
		if (m_vFile[uFilePos + m_nWaveDataCompressedSize] != (nLastByte & 0xFF))
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
	m_nWaveDataUncompressedSize = nUncompressedSize;
	// uncompress end
	return true;
}

int UMain(int argc, UChar* argv[])
{
	if (argc != 2 && argc != 3)
	{
		UPrintf(USTR("ERROR: invalid arguments\n\n"));
		return CFindWaveData::Help();
	}
	CFindWaveData findWaveData;
	findWaveData.SetInputFileName(argv[1]);
	if (argc > 2)
	{
		findWaveData.SetOutputDirName(argv[2]);
	}
	bool bResult = findWaveData.Find();
	return bResult ? 0 : 1;
}
