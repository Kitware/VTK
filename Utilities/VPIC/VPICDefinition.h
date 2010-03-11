#ifndef Definition_h
#define Definition_h

#include <string>
#include <iostream>

#include "vtkABI.h"

using namespace std;

// Now set up all of the export macros
#if defined(VTK_BUILD_SHARED_LIBS)

 #if defined(VPICReader_EXPORTS)
  #define VPICREADER_EXPORT VTK_ABI_EXPORT
 #else
  #define VPICREADER_EXPORT VTK_ABI_IMPORT
 #endif
#else
 #define VPICREADER_EXPORT
#endif

#define vpicNotUsed(x)


#define WORDSIZE 8
const int LINESIZE      = 1024;

const int VPIC_OK       = 0;
const int VPIC_FAIL     = 1;

const int NONE          = -1;

const double MIN_FLOAT  = -1e07;
const double MAX_FLOAT  =  1e07;

const int VPIC_FIELD            = 1;    // Field data file
const int VPIC_HYDRO            = 2;    // Hydro data file

const int DIMENSION             = 3;    // Grid and vector
const int TENSOR_DIMENSION      = 6;    // Tensor
const int TENSOR9_DIMENSION     = 9;    // Tensor

const int CONSTANT              = 0;    // Structure types
const int SCALAR                = 1;
const int VECTOR                = 2;
const int TENSOR                = 3;
const int TENSOR9               = 4;

const int FLOAT                 = 0;    // Basic data types
const int INTEGER               = 1;

// Read character string from file
string readString(FILE* filePtr, int size);

// Read number of unsigned integer items from file, byte swapping if needed
void readData(
        bool littleEndian,
        unsigned short* data,
        unsigned long dataSize,
        unsigned long dataCount,
        FILE* fp);

// Read number of integer items from file, byte swapping if needed
void readData(
        bool littleEndian,
        int* data,
        unsigned long dataSize,
        unsigned long dataCount,
        FILE* fp);

// Read number of float items from file, byte swapping if needed
void readData(
        bool littleEndian,
        float* data,
        unsigned long dataSize,
        unsigned long dataCount,
        FILE* fp);

// Read number of float items from file, byte swapping if needed
void readData(
        bool littleEndian,
        double* data,
        unsigned long dataSize,
        unsigned long dataCount,
        FILE* fp);

// Greatest Common Divisor
int GCD(int a, int b);

// Templated function BinaryWrite
template< class outDataType >
inline void BinaryWrite(ostream& outStream, const outDataType& outData)
{
  outStream.write(
    reinterpret_cast<const char*>(&outData), sizeof(outDataType));
}

// Templated function BinaryRead
template< class inHolderType >
inline istream& BinaryRead(istream& inStream, inHolderType& inHolder)
{
   return inStream.read(
      reinterpret_cast<char*>(&inHolder), sizeof(inHolderType));
}

#endif
