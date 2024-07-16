#ifndef FLOAT80_H_
#define FLOAT80_H_

// float80 1 sign bit, 15 exponent bits, 1 integer bit, 63 fraction bits
// double 1 sign bit, 11 exponent bits, 52 fraction bits
// float 1 sign bit, 8 exponent bits, 23 fraction bits

enum EFloat80Endianness
{
	kFloat80EndiannessLittleEndian,
	kFloat80EndiannessBigEndian
};

void DoubleToFloat(const double* a_pDouble, float* a_pFloat);
void FloatToDouble(float a_fFloat, double* a_pDouble);

void Float80ToDouble(const void* a_pFloat80, double* a_pDouble, int a_nFloat80Endianness = kFloat80EndiannessLittleEndian);
void DoubleToFloat80(double a_fDouble, void* a_pFloat80, int a_nFloat80Endianness = kFloat80EndiannessLittleEndian);

#endif	// FLOAT80_H_
