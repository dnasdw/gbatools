#include "aif2gba.h"
#include "converter.h"

const CAif2Gba::SOption CAif2Gba::s_Option[] =
{
	{ nullptr, 0, USTR("action:") },
	{ USTR("aif2s"), USTR('s'), USTR("convert AIFF file to AGB") },
	{ USTR("aif2bin"), USTR('b'), USTR("convert AIFF file to binary") },
	{ USTR("bin2aif"), USTR('a'), USTR("convert binary to AIFF file") },
	{ USTR("sample"), 0, USTR("show the samples") },
	{ USTR("help"), USTR('h'), USTR("show this help") },
	{ nullptr, 0, USTR("\ncommon:") },
	{ USTR("input_file"), USTR('i'), USTR("filepath of input file") },
	{ USTR("output_file"), USTR('o'), USTR("filepath for output file (default:input_file)") },
	{ USTR("algo"), 0, USTR("packing algorithm, supported:\n")
				   USTR("\t\t[aif2agb_1.05|aif2agb_1.06a|pok_aif_1.06a.006]") },
	{ USTR("verbose"), 0, USTR("show the info") },
	{ nullptr, 0, USTR(" aif2s/aif2bin:") },
	{ USTR("wave"), USTR('w'), USTR("comp/exp wave (default:1.0)") },
	{ USTR("volume"), USTR('v'), USTR("comp/exp volume (default:1.0)") },
	{ USTR("type"), USTR('p'), USTR("packing type (default:0 (No Packing))") },
	{ nullptr, 0, USTR("  aif2s:") },
	{ USTR("label"), USTR('l'), USTR("label for assembler (default:output_file)") },
	{ nullptr, 0, USTR(" bin2aif:") },
	{ USTR("sample_size"), 0, USTR("[1~16] sample size (default:8)") },
	{ nullptr, 0, nullptr }
};

CAif2Gba::CAif2Gba()
	: m_eAction(kActionNone)
	, m_bVerbose(false)
	, m_fWave(1.0)
	, m_fVolume(1.0)
	, m_nType(0)
	, m_nSampleSize(8)
{
}

CAif2Gba::~CAif2Gba()
{
}

int CAif2Gba::ParseOptions(int a_nArgc, UChar* a_pArgv[])
{
	if (a_nArgc <= 1)
	{
		return 1;
	}
	for (int i = 1; i < a_nArgc; i++)
	{
		int nArgpc = static_cast<int>(UCslen(a_pArgv[i]));
		if (nArgpc == 0)
		{
			continue;
		}
		int nIndex = i;
		if (a_pArgv[i][0] != USTR('-'))
		{
			UPrintf(USTR("ERROR: illegal option\n\n"));
			return 1;
		}
		else if (nArgpc > 1 && a_pArgv[i][1] != USTR('-'))
		{
			for (int j = 1; j < nArgpc; j++)
			{
				switch (parseOptions(a_pArgv[i][j], nIndex, a_nArgc, a_pArgv))
				{
				case kParseOptionReturnSuccess:
					break;
				case kParseOptionReturnIllegalOption:
					UPrintf(USTR("ERROR: illegal option\n\n"));
					return 1;
				case kParseOptionReturnNoArgument:
					UPrintf(USTR("ERROR: no argument\n\n"));
					return 1;
				case kParseOptionReturnUnknownArgument:
					UPrintf(USTR("ERROR: unknown argument \"%") PRIUS USTR("\"\n\n"), m_sMessage.c_str());
					return 1;
				case kParseOptionReturnOptionConflict:
					UPrintf(USTR("ERROR: option conflict\n\n"));
					return 1;
				}
			}
		}
		else if (nArgpc > 2 && a_pArgv[i][1] == USTR('-'))
		{
			switch (parseOptions(a_pArgv[i] + 2, nIndex, a_nArgc, a_pArgv))
			{
			case kParseOptionReturnSuccess:
				break;
			case kParseOptionReturnIllegalOption:
				UPrintf(USTR("ERROR: illegal option\n\n"));
				return 1;
			case kParseOptionReturnNoArgument:
				UPrintf(USTR("ERROR: no argument\n\n"));
				return 1;
			case kParseOptionReturnUnknownArgument:
				UPrintf(USTR("ERROR: unknown argument \"%") PRIUS USTR("\"\n\n"), m_sMessage.c_str());
				return 1;
			case kParseOptionReturnOptionConflict:
				UPrintf(USTR("ERROR: option conflict\n\n"));
				return 1;
			}
		}
		i = nIndex;
	}
	return 0;
}

int CAif2Gba::CheckOptions() const
{
	if (m_eAction == kActionNone)
	{
		UPrintf(USTR("ERROR: nothing to do\n\n"));
		return 1;
	}
	if (m_eAction != kActionSample && m_eAction != kActionHelp && m_sInputFileName.empty())
	{
		UPrintf(USTR("ERROR: no --input_file option\n\n"));
		return 1;
	}
	if (m_eAction == kActionAif2S || m_eAction == kActionAif2Bin)
	{
		if (m_nType != 0 && m_sAlgorithm.empty())
		{
			UPrintf(USTR("ERROR: no --algo option\n\n"));
			return 1;
		}
	}
	return 0;
}

int CAif2Gba::Help() const
{
	UPrintf(USTR("aif2gba %") PRIUS USTR(" by dnasdw\n\n"), AToU(GBATOOLS_VERSION).c_str());
	UPrintf(USTR("usage: aif2gba [option...] [option]...\n\n"));
	UPrintf(USTR("option:\n"));
	const SOption* pOption = s_Option;
	while (pOption->Name != nullptr || pOption->Doc != nullptr)
	{
		if (pOption->Name != nullptr)
		{
			UPrintf(USTR("  "));
			if (pOption->Key != 0)
			{
				UPrintf(USTR("-%c,"), pOption->Key);
			}
			else
			{
				UPrintf(USTR("   "));
			}
			UPrintf(USTR(" --%-8") PRIUS, pOption->Name);
			if (UCslen(pOption->Name) >= 8 && pOption->Doc != nullptr)
			{
				UPrintf(USTR("\n%16") PRIUS, USTR(""));
			}
		}
		if (pOption->Doc != nullptr)
		{
			UPrintf(USTR("%") PRIUS, pOption->Doc);
		}
		UPrintf(USTR("\n"));
		pOption++;
	}
	return 0;
}

int CAif2Gba::Action() const
{
	if (m_eAction == kActionAif2S)
	{
		if (!convertFileAif2S())
		{
			UPrintf(USTR("ERROR: convert file failed\n\n"));
			return 1;
		}
	}
	else if (m_eAction == kActionAif2Bin)
	{
		if (!convertFileAif2Bin())
		{
			UPrintf(USTR("ERROR: convert file failed\n\n"));
			return 1;
		}
	}
	else if (m_eAction == kActionBin2Aif)
	{
		if (!convertFileBin2Aif())
		{
			UPrintf(USTR("ERROR: convert file failed\n\n"));
			return 1;
		}
	}
	else if (m_eAction == kActionSample)
	{
		return sample();
	}
	else if (m_eAction == kActionHelp)
	{
		return Help();
	}
	return 0;
}

CAif2Gba::EParseOptionReturn CAif2Gba::parseOptions(const UChar* a_pName, int& a_nIndex, int a_nArgc, UChar* a_pArgv[])
{
	if (UCscmp(a_pName, USTR("aif2s")) == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionAif2S;
		}
		else if (m_eAction != kActionAif2S && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (UCscmp(a_pName, USTR("aif2bin")) == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionAif2Bin;
		}
		else if (m_eAction != kActionAif2Bin && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (UCscmp(a_pName, USTR("bin2aif")) == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionBin2Aif;
		}
		else if (m_eAction != kActionBin2Aif && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (UCscmp(a_pName, USTR("sample")) == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionSample;
		}
		else if (m_eAction != kActionSample && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (UCscmp(a_pName, USTR("help")) == 0)
	{
		m_eAction = kActionHelp;
	}
	else if (UCscmp(a_pName, USTR("input_file")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sInputFileName = a_pArgv[++a_nIndex];
	}
	else if (UCscmp(a_pName, USTR("output_file")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sOutputFileName = a_pArgv[++a_nIndex];
	}
	else if (UCscmp(a_pName, USTR("algo")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		UString sAlgorithm = a_pArgv[++a_nIndex];
		string sAlgorithmA = UToA(sAlgorithm);
		transform(sAlgorithmA.begin(), sAlgorithmA.end(), sAlgorithmA.begin(), ::tolower);
		if (!CConverter::IsSupportedAlgorithm(sAlgorithmA))
		{
			m_sMessage = sAlgorithm;
			return kParseOptionReturnUnknownArgument;
		}
		m_sAlgorithm = sAlgorithmA;
	}
	else if (UCscmp(a_pName, USTR("verbose")) == 0)
	{
		m_bVerbose = true;
	}
	else if (UCscmp(a_pName, USTR("wave")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		UString sWave = a_pArgv[++a_nIndex];
		m_fWave = SToF64(sWave);
		if (m_fWave <= 0.0)
		{
			m_sMessage = sWave;
			return kParseOptionReturnUnknownArgument;
		}
	}
	else if (UCscmp(a_pName, USTR("volume")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_fVolume = SToF64(a_pArgv[++a_nIndex]);
	}
	else if (UCscmp(a_pName, USTR("type")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_nType = SToN16(a_pArgv[++a_nIndex]);
	}
	else if (UCscmp(a_pName, USTR("label")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sLabel = UToA(a_pArgv[++a_nIndex]);
	}
	else if (UCscmp(a_pName, USTR("sample_size")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		UString sSampleSize = a_pArgv[++a_nIndex];
		n32 nSampleSize = SToN32(sSampleSize);
		if (nSampleSize < 1 || nSampleSize > 16)
		{
			m_sMessage = sSampleSize;
			return kParseOptionReturnUnknownArgument;
		}
		m_nSampleSize = nSampleSize;
	}
	return kParseOptionReturnSuccess;
}

CAif2Gba::EParseOptionReturn CAif2Gba::parseOptions(int a_nKey, int& a_nIndex, int a_nArgc, UChar* a_pArgv[])
{
	for (const SOption* pOption = s_Option; pOption->Name != nullptr || pOption->Key != 0 || pOption->Doc != nullptr; pOption++)
	{
		if (pOption->Key == a_nKey)
		{
			return parseOptions(pOption->Name, a_nIndex, a_nArgc, a_pArgv);
		}
	}
	return kParseOptionReturnIllegalOption;
}

bool CAif2Gba::convertFileAif2S() const
{
	CConverter converter;
	converter.SetInputFileName(m_sInputFileName);
	converter.SetOutputFileName(m_sOutputFileName);
	converter.SetAlgorithm(m_sAlgorithm);
	converter.SetVerbose(m_bVerbose);
	converter.SetWave(m_fWave);
	converter.SetVolume(m_fVolume);
	converter.SetType(m_nType);
	converter.SetLabel(m_sLabel);
	bool bResult = converter.ConvertFileAif2S();
	return bResult;
}

bool CAif2Gba::convertFileAif2Bin() const
{
	CConverter converter;
	converter.SetInputFileName(m_sInputFileName);
	converter.SetOutputFileName(m_sOutputFileName);
	converter.SetAlgorithm(m_sAlgorithm);
	converter.SetVerbose(m_bVerbose);
	converter.SetWave(m_fWave);
	converter.SetVolume(m_fVolume);
	converter.SetType(m_nType);
	bool bResult = converter.ConvertFileAif2Bin();
	return bResult;
}

bool CAif2Gba::convertFileBin2Aif() const
{
	CConverter converter;
	converter.SetInputFileName(m_sInputFileName);
	converter.SetOutputFileName(m_sOutputFileName);
	converter.SetAlgorithm(m_sAlgorithm);
	converter.SetVerbose(m_bVerbose);
	converter.SetSampleSize(m_nSampleSize);
	bool bResult = converter.ConvertFileBin2Aif();
	return bResult;
}

int CAif2Gba::sample() const
{
	UPrintf(USTR("sample:\n"));
	UPrintf(USTR("# convert AIFF file to AGB\n"));
	UPrintf(USTR("aif2gba -sio pv001.aif pv001.s\n\n"));
	UPrintf(USTR("# convert AIFF file to binary\n"));
	UPrintf(USTR("aif2gba -bio pv001.aif pv001.bin\n\n"));
	UPrintf(USTR("# convert binary to AIFF file\n"));
	UPrintf(USTR("aif2gba -aio pv001.bin pv001.aif\n\n"));
	UPrintf(USTR("# convert AIFF file to AGB with packing\n"));
	UPrintf(USTR("aif2gba -sio pv001.aif pv001.s -p 1 --algo pok_aif_1.06a.006\n\n"));
	UPrintf(USTR("# convert binary to AIFF file with packing algorithm\n"));
	UPrintf(USTR("aif2gba -aio pv001.bin pv001.aif --algo pok_aif_1.06a.006\n\n"));
	UPrintf(USTR("# convert binary to AIFF file with sample size\n"));
	UPrintf(USTR("aif2gba -aio pv001.bin pv001.aif --sample_size 16\n\n"));
	return 0;
}

int UMain(int argc, UChar* argv[])
{
	CAif2Gba tool;
	if (tool.ParseOptions(argc, argv) != 0)
	{
		return tool.Help();
	}
	if (tool.CheckOptions() != 0)
	{
		return 1;
	}
	return tool.Action();
}
