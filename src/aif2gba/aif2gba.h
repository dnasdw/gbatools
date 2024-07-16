#ifndef AIF2GBA_H_
#define AIF2GBA_H_

#include <sdw.h>

class CAif2Gba
{
public:
	enum EParseOptionReturn
	{
		kParseOptionReturnSuccess,
		kParseOptionReturnIllegalOption,
		kParseOptionReturnNoArgument,
		kParseOptionReturnUnknownArgument,
		kParseOptionReturnOptionConflict
	};
	enum EAction
	{
		kActionNone,
		kActionAif2S,
		kActionAif2Bin,
		kActionBin2Aif,
		kActionSample,
		kActionHelp
	};
	struct SOption
	{
		const UChar* Name;
		int Key;
		const UChar* Doc;
	};
	CAif2Gba();
	~CAif2Gba();
	int ParseOptions(int a_nArgc, UChar* a_pArgv[]);
	int CheckOptions() const;
	int Help() const;
	int Action() const;
	static const SOption s_Option[];
private:
	EParseOptionReturn parseOptions(const UChar* a_pName, int& a_nIndex, int a_nArgc, UChar* a_pArgv[]);
	EParseOptionReturn parseOptions(int a_nKey, int& a_nIndex, int a_nArgc, UChar* a_pArgv[]);
	bool convertFileAif2S() const;
	bool convertFileAif2Bin() const;
	bool convertFileBin2Aif() const;
	int sample() const;
	EAction m_eAction;
	UString m_sInputFileName;
	UString m_sOutputFileName;
	string m_sAlgorithm;
	bool m_bVerbose;
	f64 m_fWave;
	f64 m_fVolume;
	n16 m_nType;
	string m_sLabel;
	n32 m_nSampleSize;
	UString m_sMessage;
};

#endif	// AIF2GBA_H_
