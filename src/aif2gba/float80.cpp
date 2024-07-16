#include "float80.h"

union UFloat80EndiannessTester
{
	unsigned short short_value;
	unsigned char char_value[2];
};

void DoubleToFloat(const double* a_pDouble, float* a_pFloat)
{
	UFloat80EndiannessTester tester = { 0xFEFF };
	int nEndianness = tester.char_value[0] == 0xFF ? kFloat80EndiannessLittleEndian : kFloat80EndiannessBigEndian;
	const unsigned char* pDouble = reinterpret_cast<const unsigned char*>(a_pDouble);
	long nSign1 = 0;
	long nDoubleExponent11 = 0;
	long nInteger = 1;
	unsigned long uDoubleFractionHigh23 = 0;
	unsigned long uDoubleFractionLow29 = 0;
	if (nEndianness == kFloat80EndiannessLittleEndian)
	{
		nSign1 = pDouble[7] >> 7 & 0x1;
		nDoubleExponent11 = (static_cast<long>(pDouble[7] & 0x7F) << 4)
			| (static_cast<long>(pDouble[6]) >> 4 & 0xF);
		uDoubleFractionHigh23 = (static_cast<unsigned long>(pDouble[6] & 0xF) << 19)
			| (static_cast<unsigned long>(pDouble[5]) << 11)
			| (static_cast<unsigned long>(pDouble[4]) << 3)
			| (static_cast<unsigned long>(pDouble[3]) >> 5 & 0x7);
		uDoubleFractionLow29 = (static_cast<unsigned long>(pDouble[3] & 0x1F) << 24)
			| (static_cast<unsigned long>(pDouble[2]) << 16)
			| (static_cast<unsigned long>(pDouble[1]) << 8)
			| (static_cast<unsigned long>(pDouble[0]));
	}
	else //if (nEndianness == kFloat80EndiannessBigEndian)
	{
		nSign1 = pDouble[0] >> 7 & 0x1;
		nDoubleExponent11 = (static_cast<long>(pDouble[0] & 0x7F) << 4)
			| (static_cast<long>(pDouble[1] >> 4 & 0xF));
		uDoubleFractionHigh23 = (static_cast<unsigned long>(pDouble[1] & 0xF) << 19)
			| (static_cast<unsigned long>(pDouble[2]) << 11)
			| (static_cast<unsigned long>(pDouble[3]) << 3)
			| (static_cast<unsigned long>(pDouble[4]) >> 5 & 0x7);
		uDoubleFractionLow29 = (static_cast<unsigned long>(pDouble[4] & 0x1F) << 24)
			| (static_cast<unsigned long>(pDouble[5]) << 16)
			| (static_cast<unsigned long>(pDouble[6]) << 8)
			| (static_cast<unsigned long>(pDouble[7]));
	}
	unsigned char* pFloat = reinterpret_cast<unsigned char*>(a_pFloat);
	for (int i = 0; i < sizeof(float); i++)
	{
		pFloat[i] = 0;
	}
	long nFloatExponent8 = 0;
	unsigned long uFloatFraction23 = 0;
	do
	{
		if (nDoubleExponent11 == 0)
		{
			nInteger = 0;
			if (uDoubleFractionHigh23 == 0 && uDoubleFractionLow29 == 0)
			{
				// Zero.
				break;
			}
			else //if (uDoubleFractionHigh23 != 0 || uDoubleFractionLow29 != 0)
			{
				// Denormal.
				//                    m = 0b0.fraction;
				//                min_m = 0/*0b0.0*/;
				//                max_m approximately equal 1/*0b1.0*/;
				//         double_value = m * pow(2, -1022);
				//     min_double_value = 0.0;
				//     max_double_value approximately equal pow(2, -1022);
				//      min_float_value = numeric_limits<float>::denorm_min(); // pow(2, -126 - 23);
				//                             0.0 <= min_double_value
				// &&             min_double_value <= double_value
				// &&                 double_value <  max_double_value
				// &&             max_double_value <  min_float_value
				// && fabs(max_double_value - 0.0) <  fabs(min_float_value - max_double_value)
				//          float_value approximately equal 0.0
				break;
			}
		}
		else if (nDoubleExponent11 == 0x7FF)
		{
			nFloatExponent8 = 0xFF;
			if (uDoubleFractionHigh23 == 0 && uDoubleFractionLow29 == 0)
			{
				// Infinity.
				break;
			}
			else //if (uDoubleFractionHigh23 != 0 || uDoubleFractionLow29 != 0)
			{
				uFloatFraction23 = uDoubleFractionHigh23;
				if (uDoubleFractionHigh23 == 0x400000/*(1 << 22)*/ && uDoubleFractionLow29 == 0)
				{
					// Floating-Point Indefinite. This is a special case of a Quiet Not a Number.
					break;
				}
				if ((uDoubleFractionHigh23 >> 22 & 0x1) == 0)
				{
					// Signalling 'Not a Number'.
					// Cast to Quiet 'Not a Number'.
					uFloatFraction23 |= 0x400000/*(1 << 22)*/;
					break;
				}
				else //if ((uDoubleFractionHigh23 >> 22 & 0x1) == 1)
				{
					// Quiet 'Not a Number'.
					break;
				}
			}
		}
		else //if (nDoubleExponent11 > 0 && nDoubleExponent11 < 0x7FF)
		{
			nDoubleExponent11 -= 1023;
			//if (nInteger == 1)
			{
				// Normal.
				if (nDoubleExponent11 < -126 - 1 - 23)
				{
					// Denormal.
					//                    m = 0b0.00....00fraction;
					//                            |--23--|
					//                min_m = 0/*0b0.0*/;
					//                max_m approximately equal pow(2, -23)/*0b0.00....01*/;
					//                                                           |--23--|
					//         double_value = m * pow(2, -1022);
					//     min_double_value = 0.0;
					//     max_double_value approximately equal pow(2, -1022 - 23);
					//      min_float_value = numeric_limits<float>::denorm_min(); // pow(2, -126 - 23);
					//                             0.0 <= min_double_value
					// &&             min_double_value <= double_value
					// &&                 double_value <  max_double_value
					// &&             max_double_value <  min_float_value
					// && fabs(max_double_value - 0.0) <  fabs(min_float_value - max_double_value)
					//          float_value approximately equal 0.0
					break;
				}
				else if (nDoubleExponent11 > 127)
				{
					// Infinity.
					nFloatExponent8 = 0xFF;
					break;
				}
				unsigned long uDoubleSignificandHigh24 = nInteger << 23 | uDoubleFractionHigh23;
				unsigned long uDoubleSignificandLow29 = uDoubleFractionLow29;
				if (nDoubleExponent11 < -126)
				{
					// Denormalize.
					for (int i = 0; i < -126 - nDoubleExponent11; i++)
					{
						uDoubleSignificandLow29 = (uDoubleSignificandLow29 & 0x1)
							| (uDoubleSignificandLow29 >> 1 & 0xFFFFFFF)
							| ((uDoubleSignificandHigh24 & 0x1) << 28);
						uDoubleSignificandHigh24 = uDoubleSignificandHigh24 >> 1 & 0x7FFFFF;
					}
					nDoubleExponent11 = 0 - 127;
				}
				uDoubleSignificandHigh24 &= 0x7FFFFF;
				if (uDoubleSignificandHigh24 == 0x7FFFFF)
				{
					// 0x1FFFFFFF - uDoubleSignificandLow29 < uDoubleSignificandLow29 - 0x00000000
					if (uDoubleSignificandLow29 >= 0x10000000)
					{
						uDoubleSignificandHigh24 = 0;
						nDoubleExponent11++;
					}
				}
				else //if (uDoubleSignificandHigh24 != 0x7FFFFF)
				{
					// 0x20000000 - uDoubleSignificandLow29 < uDoubleSignificandLow29 - 0x00000000
					if (uDoubleSignificandLow29 > 0x10000000)
					{
						uDoubleSignificandHigh24++;
					}
				}
				if (nDoubleExponent11 > 127)
				{
					// Infinity.
					nFloatExponent8 = 0xFF;
					break;
				}
				nFloatExponent8 = nDoubleExponent11 + 127;
				uFloatFraction23 = uDoubleSignificandHigh24;
				break;
			}
		}
	} while (false);
	if (nEndianness == kFloat80EndiannessLittleEndian)
	{
		pFloat[3] = ((nSign1 & 0x1) << 7) | (nFloatExponent8 >> 1 & 0x7F);
		pFloat[2] = ((nFloatExponent8 & 0x1) << 7) | (uFloatFraction23 >> 16 & 0x7F);
		pFloat[1] = uFloatFraction23 >> 8 & 0xFF;
		pFloat[0] = uFloatFraction23 & 0xFF;
	}
	else //if (nEndianness == kFloat80EndiannessBigEndian)
	{
		pFloat[0] = ((nSign1 & 0x1) << 7) | (nFloatExponent8 >> 1 & 0x7F);
		pFloat[1] = ((nFloatExponent8 & 0x1) << 7) | (uFloatFraction23 >> 16 & 0x7F);
		pFloat[2] = uFloatFraction23 >> 8 & 0xFF;
		pFloat[3] = uFloatFraction23 & 0xFF;
	}
}

void FloatToDouble(float a_fFloat, double* a_pDouble)
{
	UFloat80EndiannessTester tester = { 0xFEFF };
	int nEndianness = tester.char_value[0] == 0xFF ? kFloat80EndiannessLittleEndian : kFloat80EndiannessBigEndian;
	const unsigned char* pFloat = reinterpret_cast<const unsigned char*>(&a_fFloat);
	long nSign1 = 0;
	long nFloatExponent8 = 0;
	long nInteger = 1;
	unsigned long uFloatFraction23 = 0;
	if (nEndianness == kFloat80EndiannessLittleEndian)
	{
		nSign1 = pFloat[3] >> 7 & 0x1;
		nFloatExponent8 = (static_cast<long>(pFloat[3] & 0x7F) << 1)
			| (static_cast<long>(pFloat[2]) >> 7 & 0x1);
		uFloatFraction23 = (static_cast<unsigned long>(pFloat[2] & 0x7F) << 16)
			| (static_cast<unsigned long>(pFloat[1]) << 8)
			| (static_cast<unsigned long>(pFloat[0]));
	}
	else //if (nEndianness == kFloat80EndiannessBigEndian)
	{
		nSign1 = pFloat[0] >> 7 & 0x1;
		nFloatExponent8 = (static_cast<long>(pFloat[0] & 0x7F) << 1)
			| (static_cast<long>(pFloat[1] >> 7 & 0x1));
		uFloatFraction23 = (static_cast<unsigned long>(pFloat[1] & 0x7F) << 16)
			| (static_cast<unsigned long>(pFloat[2]) << 8)
			| (static_cast<unsigned long>(pFloat[3]));
	}
	unsigned char* pDouble = reinterpret_cast<unsigned char*>(a_pDouble);
	for (int i = 0; i < sizeof(double); i++)
	{
		pDouble[i] = 0;
	}
	long nDoubleExponent11 = 0;
	unsigned long uDoubleFractionHigh23 = 0;
	unsigned long uDoubleFractionLow29 = 0;
	do
	{
		if (nFloatExponent8 == 0)
		{
			nInteger = 0;
			if (uFloatFraction23 == 0)
			{
				// Zero.
				break;
			}
			else //if (uFloatFraction23 != 0)
			{
				// Denormal.
				// Normalize.
				int nBits = 0;
				for (int i = 0; i < 23 && nBits == 0; i++)
				{
					if ((uFloatFraction23 >> (22 - i) & 0x1) == 1)
					{
						nBits = i + 1;
						break;
					}
				}
				for (int i = 0; i < nBits; i++)
				{
					uFloatFraction23 = (uFloatFraction23 & 0x3FFFFF) << 1;
				}
				nFloatExponent8 = -126 - nBits;
				nInteger = 1;
				nDoubleExponent11 = nFloatExponent8 + 1023;
				uDoubleFractionHigh23 = uFloatFraction23;
				break;
			}
		}
		else if (nFloatExponent8 == 0xFF)
		{
			nDoubleExponent11 = 0x7FF;
			if (uFloatFraction23 == 0)
			{
				// Infinity.
				break;
			}
			else //if (uFloatFraction23 != 0)
			{
				// Not a Number.
				// Cast to Quiet 'Not a Number'.
				uDoubleFractionHigh23 = uFloatFraction23 | 0x400000/*(1 << 22)*/;
				break;
			}
		}
		else //if (nFloatExponent8 > 0 && nFloatExponent8 < 0xFF)
		{
			nFloatExponent8 -= 127;
			//if (nInteger == 1)
			{
				// Normal.
				nDoubleExponent11 = nFloatExponent8 + 1023;
				uDoubleFractionHigh23 = uFloatFraction23;
				break;
			}
		}
	} while (false);
	if (nEndianness == kFloat80EndiannessLittleEndian)
	{
		pDouble[7] = ((nSign1 & 0x1) << 7) | (nDoubleExponent11 >> 4 & 0x7F);
		pDouble[6] = ((nDoubleExponent11 & 0xF) << 4) | (uDoubleFractionHigh23 >> 19 & 0xF);
		pDouble[5] = uDoubleFractionHigh23 >> 11 & 0xFF;
		pDouble[4] = uDoubleFractionHigh23 >> 3 & 0xFF;
		pDouble[3] = ((uDoubleFractionHigh23 & 0x7) << 5) | (uDoubleFractionLow29 >> 24 & 0x1F);
		pDouble[2] = uDoubleFractionLow29 >> 16 & 0xFF;
		pDouble[1] = uDoubleFractionLow29 >> 8 & 0xFF;
		pDouble[0] = uDoubleFractionLow29 & 0xFF;
	}
	else //if (nEndianness == kFloat80EndiannessBigEndian)
	{
		pDouble[0] = ((nSign1 & 0x1) << 7) | (nDoubleExponent11 >> 4 & 0x7F);
		pDouble[1] = ((nDoubleExponent11 & 0xF) << 4) | (uDoubleFractionHigh23 >> 19 & 0xF);
		pDouble[2] = uDoubleFractionHigh23 >> 11 & 0xFF;
		pDouble[3] = uDoubleFractionHigh23 >> 3 & 0xFF;
		pDouble[4] = ((uDoubleFractionHigh23 & 0x7) << 5) | (uDoubleFractionLow29 >> 24 & 0x1F);
		pDouble[5] = uDoubleFractionLow29 >> 16 & 0xFF;
		pDouble[6] = uDoubleFractionLow29 >> 8 & 0xFF;
		pDouble[7] = uDoubleFractionLow29 & 0xFF;
	}
}

void Float80ToDouble(const void* a_pFloat80, double* a_pDouble, int a_nFloat80Endianness/* = kFloat80EndiannessLittleEndian*/)
{
	UFloat80EndiannessTester tester = { 0xFEFF };
	int nEndianness = tester.char_value[0] == 0xFF ? kFloat80EndiannessLittleEndian : kFloat80EndiannessBigEndian;
	const unsigned char* pFloat80 = static_cast<const unsigned char*>(a_pFloat80);
	long nSign1 = 0;
	long nFloat80Exponent15 = 0;
	long nInteger1 = 0;
	unsigned long uFloat80FractionHigh23 = 0;
	unsigned long uFloat80FractionMiddle29 = 0;
	unsigned long uFloat80FractionLow11 = 0;
	if (a_nFloat80Endianness == kFloat80EndiannessLittleEndian)
	{
		nSign1 = pFloat80[9] >> 7 & 0x1;
		nFloat80Exponent15 = (static_cast<long>(pFloat80[9] & 0x7F) << 8)
			| (static_cast<long>(pFloat80[8]));
		nInteger1 = pFloat80[7] >> 7 & 0x1;
		uFloat80FractionHigh23 = (static_cast<unsigned long>(pFloat80[7] & 0x7F) << 16)
			| (static_cast<unsigned long>(pFloat80[6]) << 8)
			| (static_cast<unsigned long>(pFloat80[5]));
		uFloat80FractionMiddle29 = (static_cast<unsigned long>(pFloat80[4]) << 21)
			| (static_cast<unsigned long>(pFloat80[3]) << 13)
			| (static_cast<unsigned long>(pFloat80[2]) << 5)
			| (static_cast<unsigned long>(pFloat80[1]) >> 3 & 0x1F);
		uFloat80FractionLow11 = (static_cast<unsigned long>(pFloat80[1] & 0x7) << 8)
			| (static_cast<unsigned long>(pFloat80[0]));
	}
	else //if (nEndianness == kFloat80EndiannessBigEndian)
	{
		nSign1 = pFloat80[0] >> 7 & 0x1;
		nFloat80Exponent15 = (static_cast<long>(pFloat80[0] & 0x7F) << 8)
			| (static_cast<long>(pFloat80[1]));
		nInteger1 = pFloat80[2] >> 7 & 0x1;
		uFloat80FractionHigh23 = (static_cast<unsigned long>(pFloat80[2] & 0x7F) << 16)
			| (static_cast<unsigned long>(pFloat80[3]) << 8)
			| (static_cast<unsigned long>(pFloat80[4]));
		uFloat80FractionMiddle29 = (static_cast<unsigned long>(pFloat80[5]) << 21)
			| (static_cast<unsigned long>(pFloat80[6]) << 13)
			| (static_cast<unsigned long>(pFloat80[7]) << 5)
			| (static_cast<unsigned long>(pFloat80[8]) >> 3 & 0x1F);
		uFloat80FractionLow11 = (static_cast<unsigned long>(pFloat80[8] & 0x7) << 8)
			| (static_cast<unsigned long>(pFloat80[9]));
	}
	unsigned char* pDouble = reinterpret_cast<unsigned char*>(a_pDouble);
	for (int i = 0; i < sizeof(double); i++)
	{
		pDouble[i] = 0;
	}
	long nDoubleExponent11 = 0;
	unsigned long uDoubleFractionHigh23 = 0;
	unsigned long uDoubleFractionLow29 = 0;
	do
	{
		if (nFloat80Exponent15 == 0)
		{
			if (nInteger1 == 0)
			{
				if (uFloat80FractionHigh23 == 0 && uFloat80FractionMiddle29 == 0 && uFloat80FractionLow11 == 0)
				{
					// Zero.
					break;
				}
				else //if (uFloat80FractionHigh23 != 0 || uFloat80FractionMiddle29 != 0 || uFloat80FractionLow11 != 0)
				{
					// Denormal.
					//                     m = 0b0.fraction;
					//                 min_m = 0/*0b0.0*/;
					//                 max_m approximately equal 1/*0b1.0*/;
					//         float80_value = m * pow(2, -16382);
					//     min_float80_value = 0.0;
					//     max_float80_value approximately equal pow(2, -16382);
					//      min_double_value = numeric_limits<double>::denorm_min(); // pow(2, -1022 - 52);
					//                              0.0 <= min_float80_value
					// &&             min_float80_value <= float80_value
					// &&                 float80_value <  max_float80_value
					// &&             max_float80_value <  min_double_value
					// && fabs(max_float80_value - 0.0) <  fabs(min_double_value - max_float80_value)
					//         float80_value approximately equal 0.0
					break;
				}
			}
			else //if (nInteger1 == 1)
			{
				// Pseudo Denormal.
				//                     m = 0b1.fraction;
				//                 min_m = 1/*0b1.0*/;
				//                 max_m approximately equal 2/*0b10.0*/;
				//         float80_value = m * pow(2, -16382);
				//     min_float80_value = pow(2, -16382);
				//     max_float80_value approximately equal pow(2, -16381);
				//      min_double_value = numeric_limits<double>::denorm_min(); // pow(2, -1022 - 52);
				//                              0.0 <= min_float80_value
				// &&             min_float80_value <= float80_value
				// &&                 float80_value <  max_float80_value
				// &&             max_float80_value <  min_double_value
				// && fabs(max_float80_value - 0.0) <  fabs(min_double_value - max_float80_value)
				//         float80_value approximately equal 0.0
				break;
			}
		}
		else if (nFloat80Exponent15 == 0x7FFF)
		{
			nDoubleExponent11 = 0x7FF;
			if (uFloat80FractionHigh23 == 0 && uFloat80FractionMiddle29 == 0 && uFloat80FractionLow11 == 0)
			{
				if (nInteger1 == 0)
				{
					// Pseudo Infinity.
					break;
				}
				else //if (nInteger1 == 1)
				{
					// Infinity.
					break;
				}
			}
			else //if (uFloat80FractionHigh23 != 0 || uFloat80FractionMiddle29 != 0 || uFloat80FractionLow11 != 0)
			{
				uDoubleFractionHigh23 = uFloat80FractionHigh23;
				uDoubleFractionLow29 = uFloat80FractionMiddle29;
				if (nInteger1 == 1 && uFloat80FractionHigh23 == 0x400000/*(1 << 22)*/ && uFloat80FractionMiddle29 == 0 && uFloat80FractionLow11 == 0)
				{
					// Floating-Point Indefinite. This is a special case of a Quiet Not a Number.
					break;
				}
				if (nInteger1 == 0)
				{
					// Pseudo 'Not a Number'.
					uDoubleFractionHigh23 |= 0x400000/*(1 << 22)*/;
					break;
				}
				else //if (nInteger1 == 1)
				{
					if ((uFloat80FractionHigh23 >> 22 & 0x1) == 0)
					{
						// Signalling 'Not a Number'.
						// Cast to Quiet 'Not a Number'.
						uDoubleFractionHigh23 |= 0x400000/*(1 << 22)*/;
						break;
					}
					else //if ((uFloat80FractionHigh23 >> 22 & 0x1) == 1)
					{
						// Quiet 'Not a Number'.
						break;
					}
				}
			}
		}
		else //if (nFloat80Exponent15 > 0 && nFloat80Exponent15 < 0x7FFF)
		{
			nFloat80Exponent15 -= 16383;
			if (nInteger1 == 0)
			{
				// Denormal.
				if (uFloat80FractionHigh23 == 0 && uFloat80FractionMiddle29 == 0 && uFloat80FractionLow11 == 0)
				{
					// Denormal Zero.
					// Can NOT be represented as a normalized number.
					break;
				}
				// Normalize.
				int nBits = 0;
				for (int i = 0; i < 23 && nBits == 0; i++)
				{
					if ((uFloat80FractionHigh23 >> (22 - i) & 0x1) == 1)
					{
						nBits = i + 1;
						break;
					}
				}
				for (int i = 0; i < 29 && nBits == 0; i++)
				{
					if ((uFloat80FractionMiddle29 >> (28 - i) & 0x1) == 1)
					{
						nBits = i + 1 + 23;
						break;
					}
				}
				for (int i = 0; i < 11 && nBits == 0; i++)
				{
					if ((uFloat80FractionLow11 >> (10 - i) & 0x1) == 1)
					{
						nBits = i + 1 + 23 + 29;
						break;
					}
				}
				for (int i = 0; i < nBits; i++)
				{
					uFloat80FractionHigh23 = ((uFloat80FractionHigh23 & 0x3FFFFF) << 1) | (uFloat80FractionMiddle29 >> 28 & 0x1);
					uFloat80FractionMiddle29 = ((uFloat80FractionMiddle29 & 0xFFFFFFF) << 1) | (uFloat80FractionLow11 >> 10 & 0x1);
					uFloat80FractionLow11 = (uFloat80FractionLow11 & 0x3FF) << 1;
				}
				nFloat80Exponent15 -= nBits;
				nInteger1 = 1;
			}
			//if (nInteger1 == 1)
			{
				// Normal.
				if (nFloat80Exponent15 < -1022 - 1 - 52)
				{
					// Denormal.
					//                     m = 0b0.00....00fraction;
					//                             |--52--|
					//                 min_m = 0/*0b0.0*/;
					//                 max_m approximately equal pow(2, -52)/*0b0.00....01*/;
					//                                                            |--52--|
					//         float80_value = m * pow(2, -16382);
					//     min_float80_value = 0.0;
					//     max_float80_value approximately equal pow(2, -16382 - 52);
					//      min_double_value = numeric_limits<double>::denorm_min(); // pow(2, -1022 - 52);
					//                              0.0 <= min_float80_value
					// &&             min_float80_value <= float80_value
					// &&                 float80_value <  max_float80_value
					// &&             max_float80_value <  min_double_value
					// && fabs(max_float80_value - 0.0) <  fabs(min_double_value - max_float80_value)
					//         float80_value approximately equal 0.0
					break;
				}
				else if (nFloat80Exponent15 > 1023)
				{
					// Infinity.
					nDoubleExponent11 = 0x7FF;
					break;
				}
				unsigned long uFloat80SignificandHigh24 = nInteger1 << 23 | uFloat80FractionHigh23;
				unsigned long uFloat80SignificandMiddle29 = uFloat80FractionMiddle29;
				unsigned long uFloat80SignificandLow11 = uFloat80FractionLow11;
				if (nFloat80Exponent15 < -1022)
				{
					// Denormalize.
					for (int i = 0; i < -1022 - nFloat80Exponent15; i++)
					{
						uFloat80SignificandLow11 = (uFloat80SignificandLow11 & 0x1)
							| (uFloat80SignificandLow11 >> 1 & 0x3FF)
							| ((uFloat80SignificandMiddle29 & 0x1) << 10);
						uFloat80SignificandMiddle29 = (uFloat80SignificandMiddle29 >> 1 & 0xFFFFFFF)
							| ((uFloat80SignificandHigh24 & 0x1) << 28);
						uFloat80SignificandHigh24 = uFloat80SignificandHigh24 >> 1 & 0x7FFFFF;
					}
					nFloat80Exponent15 = 0 - 1023;
				}
				uFloat80SignificandHigh24 &= 0x7FFFFF;
				if (uFloat80SignificandHigh24 == 0x7FFFFF && uFloat80FractionMiddle29 == 0x1FFFFFFF)
				{
					// 0x7FF - uFloat80SignificandLow11 < uFloat80SignificandLow11 - 0x000
					if (uFloat80SignificandLow11 >= 0x400)
					{
						nFloat80Exponent15++;
						uFloat80SignificandHigh24 = 0;
						uFloat80FractionMiddle29 = 0;
					}
				}
				else //if (uFloat80SignificandHigh24 != 0x7FFFFF || uFloat80FractionMiddle29 != 0x1FFFFFFF)
				{
					// 0x800 - uFloat80SignificandLow11 < uFloat80SignificandLow11 - 0x000
					if (uFloat80SignificandLow11 > 0x400)
					{
						uFloat80FractionMiddle29++;
						uFloat80SignificandHigh24 += uFloat80FractionMiddle29 >> 29 & 0x1;
						uFloat80FractionMiddle29 &= 0x1FFFFFFF;
					}
				}
				if (nFloat80Exponent15 > 1023)
				{
					// Infinity.
					nDoubleExponent11 = 0x7FF;
					break;
				}
				nDoubleExponent11 = nFloat80Exponent15 + 1023;
				uDoubleFractionHigh23 = uFloat80SignificandHigh24;
				uDoubleFractionLow29 = uFloat80SignificandMiddle29;
				break;
			}
		}
	} while (false);
	if (nEndianness == kFloat80EndiannessLittleEndian)
	{
		pDouble[7] = ((nSign1 & 0x1) << 7) | (nDoubleExponent11 >> 4 & 0x7F);
		pDouble[6] = ((nDoubleExponent11 & 0xF) << 4) | (uDoubleFractionHigh23 >> 19 & 0xF);
		pDouble[5] = uDoubleFractionHigh23 >> 11 & 0xFF;
		pDouble[4] = uDoubleFractionHigh23 >> 3 & 0xFF;
		pDouble[3] = ((uDoubleFractionHigh23 & 0x7) << 5) | (uDoubleFractionLow29 >> 24 & 0x1F);
		pDouble[2] = uDoubleFractionLow29 >> 16 & 0xFF;
		pDouble[1] = uDoubleFractionLow29 >> 8 & 0xFF;
		pDouble[0] = uDoubleFractionLow29 & 0xFF;
	}
	else //if (nEndianness == kFloat80EndiannessBigEndian)
	{
		pDouble[0] = ((nSign1 & 0x1) << 7) | (nDoubleExponent11 >> 4 & 0x7F);
		pDouble[1] = ((nDoubleExponent11 & 0xF) << 4) | (uDoubleFractionHigh23 >> 19 & 0xF);
		pDouble[2] = uDoubleFractionHigh23 >> 11 & 0xFF;
		pDouble[3] = uDoubleFractionHigh23 >> 3 & 0xFF;
		pDouble[4] = ((uDoubleFractionHigh23 & 0x7) << 5) | (uDoubleFractionLow29 >> 24 & 0x1F);
		pDouble[5] = uDoubleFractionLow29 >> 16 & 0xFF;
		pDouble[6] = uDoubleFractionLow29 >> 8 & 0xFF;
		pDouble[7] = uDoubleFractionLow29 & 0xFF;
	}
}

void DoubleToFloat80(double a_fDouble, void* a_pFloat80, int a_nFloat80Endianness/* = kFloat80EndiannessLittleEndian*/)
{
	UFloat80EndiannessTester tester = { 0xFEFF };
	int nEndianness = tester.char_value[0] == 0xFF ? kFloat80EndiannessLittleEndian : kFloat80EndiannessBigEndian;
	const unsigned char* pDouble = reinterpret_cast<const unsigned char*>(&a_fDouble);
	long nSign1 = 0;
	long nDoubleExponent11 = 0;
	unsigned long uDoubleFractionHigh23 = 0;
	unsigned long uDoubleFractionLow29 = 0;
	if (nEndianness == kFloat80EndiannessLittleEndian)
	{
		nSign1 = pDouble[7] >> 7 & 0x1;
		nDoubleExponent11 = (static_cast<long>(pDouble[7] & 0x7F) << 4)
			| (static_cast<long>(pDouble[6]) >> 4 & 0xF);
		uDoubleFractionHigh23 = (static_cast<unsigned long>(pDouble[6] & 0xF) << 19)
			| (static_cast<unsigned long>(pDouble[5]) << 11)
			| (static_cast<unsigned long>(pDouble[4]) << 3)
			| (static_cast<unsigned long>(pDouble[3]) >> 5 & 0x7);
		uDoubleFractionLow29 = (static_cast<unsigned long>(pDouble[3] & 0x1F) << 24)
			| (static_cast<unsigned long>(pDouble[2]) << 16)
			| (static_cast<unsigned long>(pDouble[1]) << 8)
			| (static_cast<unsigned long>(pDouble[0]));
	}
	else //if (nEndianness == kFloat80EndiannessBigEndian)
	{
		nSign1 = pDouble[0] >> 7 & 0x1;
		nDoubleExponent11 = (static_cast<long>(pDouble[0] & 0x7F) << 4)
			| (static_cast<long>(pDouble[1] >> 4 & 0xF));
		uDoubleFractionHigh23 = (static_cast<unsigned long>(pDouble[1] & 0xF) << 19)
			| (static_cast<unsigned long>(pDouble[2]) << 11)
			| (static_cast<unsigned long>(pDouble[3]) << 3)
			| (static_cast<unsigned long>(pDouble[4]) >> 5 & 0x7);
		uDoubleFractionLow29 = (static_cast<unsigned long>(pDouble[4] & 0x1F) << 24)
			| (static_cast<unsigned long>(pDouble[5]) << 16)
			| (static_cast<unsigned long>(pDouble[6]) << 8)
			| (static_cast<unsigned long>(pDouble[7]));
	}
	unsigned char* pFloat80 = static_cast<unsigned char*>(a_pFloat80);
	for (int i = 0; i < 10; i++)
	{
		pFloat80[i] = 0;
	}
	long nFloat80Exponent15 = 0;
	long nInteger1 = 1;
	unsigned long uFloat80FractionHigh23 = 0;
	unsigned long uFloat80FractionMiddle29 = 0;
	unsigned long uFloat80FractionLow11 = 0;
	do
	{
		if (nDoubleExponent11 == 0)
		{
			nInteger1 = 0;
			if (uDoubleFractionHigh23 == 0 && uDoubleFractionLow29 == 0)
			{
				// Zero.
				break;
			}
			else //if (uDoubleFractionHigh23 != 0 || uDoubleFractionLow29 != 0)
			{
				// Denormal.
				// Normalize.
				int nBits = 0;
				for (int i = 0; i < 23 && nBits == 0; i++)
				{
					if ((uDoubleFractionHigh23 >> (22 - i) & 0x1) == 1)
					{
						nBits = i + 1;
						break;
					}
				}
				for (int i = 0; i < 29 && nBits == 0; i++)
				{
					if ((uDoubleFractionLow29 >> (28 - i) & 0x1) == 1)
					{
						nBits = i + 1 + 23;
						break;
					}
				}
				for (int i = 0; i < nBits; i++)
				{
					uDoubleFractionHigh23 = ((uDoubleFractionHigh23 & 0x3FFFFF) << 1) | (uDoubleFractionLow29 >> 28 & 0x1);
					uDoubleFractionLow29 = (uDoubleFractionLow29 & 0xFFFFFFF) << 1;
				}
				nDoubleExponent11 = -1022 - nBits;
				nInteger1 = 1;
				nFloat80Exponent15 = nDoubleExponent11 + 16383;
				uFloat80FractionHigh23 = uDoubleFractionHigh23;
				uFloat80FractionMiddle29 = uDoubleFractionLow29;
				break;
			}
		}
		else if (nDoubleExponent11 == 0x7FF)
		{
			nFloat80Exponent15 = 0x7FFF;
			if (uDoubleFractionHigh23 == 0 && uDoubleFractionLow29 == 0)
			{
				// Infinity.
				break;
			}
			else //if (uDoubleFractionHigh23 != 0 || uDoubleFractionLow29 != 0)
			{
				// Not a Number.
				// Cast to Quiet 'Not a Number'.
				uFloat80FractionHigh23 = uDoubleFractionHigh23 | 0x400000/*(1 << 22)*/;
				uFloat80FractionMiddle29 = uDoubleFractionLow29;
				break;
			}
		}
		else //if (nDoubleExponent11 > 0 && nDoubleExponent11 < 0x7FF)
		{
			nDoubleExponent11 -= 1023;
			//if (nInteger1 == 1)
			{
				// Normal.
				nFloat80Exponent15 = nDoubleExponent11 + 16383;
				uFloat80FractionHigh23 = uDoubleFractionHigh23;
				uFloat80FractionMiddle29 = uDoubleFractionLow29;
				break;
			}
		}
	} while (false);
	if (a_nFloat80Endianness == kFloat80EndiannessLittleEndian)
	{
		pFloat80[9] = ((nSign1 & 0x1) << 7) | (nFloat80Exponent15 >> 8 & 0x7F);
		pFloat80[8] = nFloat80Exponent15 & 0xFF;
		pFloat80[7] = ((nInteger1 & 0x1) << 7) | (uFloat80FractionHigh23 >> 16 & 0x7F);
		pFloat80[6] = uFloat80FractionHigh23 >> 8 & 0xFF;
		pFloat80[5] = uFloat80FractionHigh23 & 0xFF;
		pFloat80[4] = uFloat80FractionMiddle29 >> 21 & 0xFF;
		pFloat80[3] = uFloat80FractionMiddle29 >> 13 & 0xFF;
		pFloat80[2] = uFloat80FractionMiddle29 >> 5 & 0xFF;
		pFloat80[1] = ((uFloat80FractionMiddle29 & 0x1F) << 3) | (uFloat80FractionLow11 >> 8 & 0x7);
		pFloat80[0] = uFloat80FractionLow11 & 0xFF;
	}
	else //if (a_nFloat80Endianness == kFloat80EndiannessBigEndian)
	{
		pFloat80[0] = ((nSign1 & 0x1) << 7) | (nFloat80Exponent15 >> 8 & 0x7F);
		pFloat80[1] = nFloat80Exponent15 & 0xFF;
		pFloat80[2] = ((nInteger1 & 0x1) << 7) | (uFloat80FractionHigh23 >> 16 & 0x7F);
		pFloat80[3] = uFloat80FractionHigh23 >> 8 & 0xFF;
		pFloat80[4] = uFloat80FractionHigh23 & 0xFF;
		pFloat80[5] = uFloat80FractionMiddle29 >> 21 & 0xFF;
		pFloat80[6] = uFloat80FractionMiddle29 >> 13 & 0xFF;
		pFloat80[7] = uFloat80FractionMiddle29 >> 5 & 0xFF;
		pFloat80[8] = ((uFloat80FractionMiddle29 & 0x1F) << 3) | (uFloat80FractionLow11 >> 8 & 0x7);
		pFloat80[9] = uFloat80FractionLow11 & 0xFF;
	}
}
