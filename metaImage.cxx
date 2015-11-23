/*============================================================================
  MetaIO
  Copyright 2000-2010 Insight Software Consortium

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/
#include "metaImage.h"

#include <stdio.h>
#include <ctype.h>
#include <string>
#include <string.h> // for memcpy
#include <stdlib.h> // for atoi
#include <math.h>

#if defined (__BORLANDC__) && (__BORLANDC__ >= 0x0580)
#include <mem.h>
#endif

#if defined(_WIN32)
# include <io.h>
#endif

// support for access
#ifndef _WIN32
#include <limits.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <pwd.h>
#include <termios.h>
#include <signal.h>    /* sigprocmask */
#endif

namespace {

void openReadStream(METAIO_STREAM::ifstream & inputStream, const char * fname)
{
#ifdef __sgi
  inputStream.open( fname, METAIO_STREAM::ios::in );
#else
  inputStream.open( fname, METAIO_STREAM::ios::in |
                           METAIO_STREAM::ios::binary );
#endif
}

void openWriteStream(METAIO_STREAM::ofstream & outputStream, const char * fname, bool append)
{
// Some older sgi compilers have a error in the ofstream constructor
// that requires a file to exist for output
#ifdef __sgi
  {
  METAIO_STREAM::ofstream tFile(fname, METAIO_STREAM::ios::out);
  tFile.close();
  }
#endif

  if(!append)
    {
#ifdef __sgi
    outputStream.open(fname, METAIO_STREAM::ios::out);
#else
    outputStream.open(fname, METAIO_STREAM::ios::binary |
                             METAIO_STREAM::ios::out);
#endif
    }
  else
    {
#ifdef __sgi
    outputStream.open(fname, METAIO_STREAM::ios::app |
                             METAIO_STREAM::ios::out);
#else
    outputStream.open(fname, METAIO_STREAM::ios::binary |
                             METAIO_STREAM::ios::app |
                             METAIO_STREAM::ios::out);
#endif
    }
}

const unsigned int MAXPATHLENGHT = 2048;

} // end anonymous namespace

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif

// 1 Gigabyte is the maximum chunk to read/write in on function call
static const METAIO_STL::streamoff MaxIOChunk = 1024*1024*1024;

//
// MetaImage Constructors
//
MetaImage::
MetaImage()
:MetaObject()
  {
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaImage()" << METAIO_STREAM::endl;
    }

  m_CompressionTable = new MET_CompressionTableType;
  m_CompressionTable->compressedStream = NULL;
  m_CompressionTable->buffer = NULL;
  Clear();
  }

//
MetaImage::
MetaImage(const char *_headerName)
:MetaObject()
  {
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaImage()" << METAIO_STREAM::endl;
    }

  m_CompressionTable = new MET_CompressionTableType;
  m_CompressionTable->compressedStream = NULL;
  m_CompressionTable->buffer = NULL;
  Clear();

  Read(_headerName);
  }

//
MetaImage::
MetaImage(MetaImage *_im)
:MetaObject()
  {
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaImage()" << METAIO_STREAM::endl;
    }

  m_CompressionTable = new MET_CompressionTableType;
  m_CompressionTable->compressedStream = NULL;
  m_CompressionTable->buffer = NULL;
  Clear();

  InitializeEssential(_im->NDims(),
                      _im->DimSize(),
                      _im->ElementSpacing(),
                      _im->ElementType(),
                      _im->ElementNumberOfChannels(),
                      _im->ElementData(),
                      false);
  CopyInfo(_im);
  }

//
MetaImage::
MetaImage(int _nDims,
          const int * _dimSize,
          const float * _elementSpacing,
          MET_ValueEnumType _elementType,
          int _elementNumberOfChannels,
          void *_elementData)
:MetaObject()
  {
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaImage()" << METAIO_STREAM::endl;
    }

  m_CompressionTable = new MET_CompressionTableType;
  m_CompressionTable->buffer = NULL;
  m_CompressionTable->compressedStream = NULL;
  Clear();

  if(_elementData == NULL)
    {
    InitializeEssential(_nDims,
                        _dimSize,
                        _elementSpacing,
                        _elementType,
                        _elementNumberOfChannels,
                        NULL, true);
    }
  else
    {
    InitializeEssential(_nDims,
                        _dimSize,
                        _elementSpacing,
                        _elementType,
                        _elementNumberOfChannels,
                        _elementData, false);
    }

  }

//
MetaImage::
MetaImage(int _x, int _y,
          float _elementSpacingX, float _elementSpacingY,
          MET_ValueEnumType _elementType,
          int _elementNumberOfChannels, void *_elementData)
:MetaObject()
  {
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaImage()" << METAIO_STREAM::endl;
    }

  m_CompressionTable = new MET_CompressionTableType;
  m_CompressionTable->compressedStream = NULL;
  m_CompressionTable->buffer = NULL;
  Clear();

  int ds[2];
  ds[0] = _x;
  ds[1] = _y;

  float es[2];
  es[0] = _elementSpacingX;
  es[1] = _elementSpacingY;

  if(_elementData == NULL)
    {
    InitializeEssential(2,
                        ds,
                        es,
                        _elementType,
                        _elementNumberOfChannels,
                        NULL,
                        true);
    }
  else
    {
    InitializeEssential(2,
                        ds,
                        es,
                        _elementType,
                        _elementNumberOfChannels,
                        _elementData,
                        false);
    }
  }

//
MetaImage::
MetaImage(int _x, int _y, int _z,
          float _elementSpacingX,
          float _elementSpacingY,
          float _elementSpacingZ,
          MET_ValueEnumType _elementType,
          int _elementNumberOfChannels,
          void *_elementData)
:MetaObject()
  {
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaImage()" << METAIO_STREAM::endl;
    }

  m_CompressionTable = new MET_CompressionTableType;
  m_CompressionTable->compressedStream = NULL;
  m_CompressionTable->buffer = NULL;
  Clear();

  int ds[3];
  ds[0] = _x;
  ds[1] = _y;
  ds[2] = _z;

  float es[3];
  es[0] = _elementSpacingX;
  es[1] = _elementSpacingY;
  es[2] = _elementSpacingZ;

  if(_elementData == NULL)
    {
    InitializeEssential(3,
                        ds,
                        es,
                        _elementType,
                        _elementNumberOfChannels,
                        NULL,
                        true);
    }
  else
    {
    InitializeEssential(3,
                        ds,
                        es,
                        _elementType,
                        _elementNumberOfChannels,
                        _elementData,
                        false);
    }
  }

//
MetaImage::
~MetaImage()
  {
  M_Destroy();
  }

//
void MetaImage::
PrintInfo() const
  {
  int i;

  MetaObject::PrintInfo();

  char s[MAXPATHLENGHT];
  MET_ImageModalityToString(m_Modality, s);
  METAIO_STREAM::cout << "Modality = " << s << METAIO_STREAM::endl;

  METAIO_STREAM::cout << "DimSize = ";
  for(i=0; i<m_NDims; i++)
    {
    METAIO_STREAM::cout << m_DimSize[i] << " ";
    }
  METAIO_STREAM::cout << METAIO_STREAM::endl;
  METAIO_STREAM::cout << "SubQuantity = ";
  for(i=0; i<m_NDims; i++)
    {
    METAIO_STREAM::cout << m_SubQuantity[i] << " ";
    }
  METAIO_STREAM::cout << METAIO_STREAM::endl;

  METAIO_STREAM::cout << "Quantity = " << m_Quantity << METAIO_STREAM::endl;


  METAIO_STREAM::cout << "HeaderSize = " << m_HeaderSize << METAIO_STREAM::endl;

  METAIO_STREAM::cout << "SequenceID = ";
  for(i=0; i<m_NDims; i++)
    {
    METAIO_STREAM::cout << m_SequenceID[i] << " ";
    }
  METAIO_STREAM::cout << METAIO_STREAM::endl;

  METAIO_STREAM::cout << "ElementSizeValid = " << (int)m_ElementSizeValid
            << METAIO_STREAM::endl;
  METAIO_STREAM::cout << "ElementSize = ";
  for(i=0; i<m_NDims; i++)
    {
    METAIO_STREAM::cout << m_ElementSize[i] << " ";
    }
  METAIO_STREAM::cout << METAIO_STREAM::endl;

  char str[MAXPATHLENGHT];
  MET_TypeToString(m_ElementType, str);
  METAIO_STREAM::cout << "ElementType = " << str << METAIO_STREAM::endl;

  METAIO_STREAM::cout << "ElementNumberOfChannels = "
                      << m_ElementNumberOfChannels << METAIO_STREAM::endl;

  if(m_ElementMinMaxValid)
    {
    METAIO_STREAM::cout << "Min and Max are valid" << METAIO_STREAM::endl;
    METAIO_STREAM::cout << "   Min = " << m_ElementMin << METAIO_STREAM::endl;
    METAIO_STREAM::cout << "   Max = " << m_ElementMax << METAIO_STREAM::endl;
    }
  else
    {
    METAIO_STREAM::cout << "Min and Max are not valid" << METAIO_STREAM::endl;
    }

  METAIO_STREAM::cout << "ElementToIntensityFunctionSlope = "
                      << m_ElementToIntensityFunctionSlope
                      << METAIO_STREAM::endl;
  METAIO_STREAM::cout << "ElementToIntensityFunctionOffset = "
                      << m_ElementToIntensityFunctionOffset
                      << METAIO_STREAM::endl;


  METAIO_STREAM::cout << "AutoFreeElementData = "
                      << ((m_AutoFreeElementData)?"True":"False")
                      << METAIO_STREAM::endl;

  METAIO_STREAM::cout << "ElementData = "
                      << ((m_ElementData==NULL)?"NULL":"Valid")
                      << METAIO_STREAM::endl;

  METAIO_STREAM::cout << "ElementDataFileName = "
                      << m_ElementDataFileName << METAIO_STREAM::endl;

  }

void MetaImage::
CopyInfo(const MetaObject * _object)
  {
  MetaObject::CopyInfo(_object);

  if(_object)
    {
    const MetaImage * im;
    try
      {
      im = (const MetaImage *)(_object);
      }
    catch( ... )
      {
      return;
      }

    if( im )
      {
      Modality(im->Modality());

      HeaderSize(im->HeaderSize());

      SequenceID(im->SequenceID());

      ElementSizeValid(im->ElementSizeValid());
      if(im->ElementSizeValid())
        {
        ElementSize(im->ElementSize());
        }

      ElementMinMaxValid(im->ElementMinMaxValid());
      if(im->ElementMinMaxValid())
        {
        ElementMin(im->ElementMin());
        ElementMax(im->ElementMax());
        }

      ElementToIntensityFunctionSlope(im->ElementToIntensityFunctionSlope());
      ElementToIntensityFunctionOffset(im->ElementToIntensityFunctionOffset());
      }
    }
  }

/** Clear function */
void MetaImage::Clear(void)
{
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaImage: Clear" << METAIO_STREAM::endl;
    }

  m_Modality = MET_MOD_UNKNOWN;

  m_DimSize[0] = 0;
  m_SubQuantity[0] = 0;
  m_Quantity = 0;

  m_HeaderSize = 0;

  memset(m_SequenceID, 0, 4*sizeof(float));

  m_ElementSizeValid = false;
  memset(m_ElementSize, 0, 10*sizeof(float));

  m_ElementType = MET_NONE;

  m_ElementNumberOfChannels = 1;

  m_ElementMinMaxValid = false;
  m_ElementMin = 0;
  m_ElementMax = 0;

  m_ElementToIntensityFunctionSlope = 1;
  m_ElementToIntensityFunctionOffset = 0;

  m_AutoFreeElementData = true;

  m_ElementData = NULL;

  strcpy(m_ElementDataFileName, "");

  MetaObject::Clear();

  // Change the default for this object
  m_BinaryData = true;

  if(m_CompressionTable)
    {
    if(m_CompressionTable->compressedStream)
      {
      inflateEnd(m_CompressionTable->compressedStream);
      delete m_CompressionTable->compressedStream;
      delete [] m_CompressionTable->buffer;
      m_CompressionTable->buffer = NULL;
      }
    m_CompressionTable->compressedStream = NULL;
    m_CompressionTable->offsetList.clear();
    }
  else
    {
    m_CompressionTable = new MET_CompressionTableType;
    m_CompressionTable->compressedStream = NULL;
    }

}

bool MetaImage::
InitializeEssential(int _nDims,
                    const int * _dimSize,
                    const float * _elementSpacing,
                    MET_ValueEnumType _elementType,
                    int _elementNumberOfChannels,
                    void * _elementData,
                    bool _allocElementMemory)
  {
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaImage: Initialize" << METAIO_STREAM::endl;
    }

  MetaObject::InitializeEssential(_nDims);

  int i;
  if(!m_CompressionTable)
    {
    m_CompressionTable = new MET_CompressionTableType;
    m_CompressionTable->buffer = NULL;
    m_CompressionTable->compressedStream = NULL;
    }
  m_SubQuantity[0] = 1;
  m_Quantity = 1;
  m_ElementSizeValid = false;
  for(i=0; i<m_NDims; i++)
    {
    m_DimSize[i] = _dimSize[i];
    m_Quantity *= _dimSize[i];
    if(i>0)
      {
      m_SubQuantity[i] = m_SubQuantity[i-1]*m_DimSize[i-1];
      }
    m_ElementSpacing[i] = _elementSpacing[i];
    if(m_ElementSize[i] == 0)
      {
      m_ElementSize[i] = m_ElementSpacing[i];
      }
    else
      {
      m_ElementSizeValid = true;
      }
    }

  m_ElementType = _elementType;

  m_ElementNumberOfChannels = _elementNumberOfChannels;

  if(_elementData != NULL)
    {
    m_AutoFreeElementData = false;
    m_ElementData = (void *)_elementData;
    }
  else if(_allocElementMemory)
    {
    m_AutoFreeElementData = true;
    MET_SizeOfType(m_ElementType, &i);
    m_ElementData = new char[static_cast<size_t>(m_Quantity*m_ElementNumberOfChannels*i)];
    if(m_ElementData == NULL)
      {
      METAIO_STREAM::cerr << "MetaImage:: M_Allocate:: Insufficient memory"
                          << METAIO_STREAM::endl;
      return false;
      }
    }
  else
    {
    m_AutoFreeElementData = true;
    m_ElementData = NULL;
    }

  return true;
  }


//
//
//
int MetaImage::
HeaderSize(void) const
  {
  return m_HeaderSize;
  }

void MetaImage::
HeaderSize(int _headerSize)
  {
  m_HeaderSize = _headerSize;
  }

//
//
//
MET_ImageModalityEnumType MetaImage::
Modality(void) const
  {
  return m_Modality;
  }

void MetaImage::
Modality(MET_ImageModalityEnumType _modality)
  {
  m_Modality = _modality;
  }

//
//
//
const int * MetaImage::
DimSize(void) const
  {
  return m_DimSize;
  }

int MetaImage::
DimSize(int _i) const
  {
  return m_DimSize[_i];
  }

//
//
//
METAIO_STL::streamoff MetaImage::
Quantity(void) const
  {
  return m_Quantity;
  }

//
//
//
const METAIO_STL::streamoff * MetaImage::
SubQuantity(void) const
  {
  return m_SubQuantity;
  }

METAIO_STL::streamoff MetaImage::
SubQuantity(int _i) const
  {
  return m_SubQuantity[_i];
  }

//
//
//
const float * MetaImage::
SequenceID(void) const
  {
  return m_SequenceID;
  }

float MetaImage::
SequenceID(int _i) const
  {
  return m_SequenceID[_i];
  }

void MetaImage::
SequenceID(const float *_sequenceID)
  {
  memcpy(m_SequenceID, _sequenceID, m_NDims*sizeof(float));
  }

void MetaImage::
SequenceID(int _i, float _value)
  {
  m_SequenceID[_i] = _value;
  }

//
//
//
bool MetaImage::
ElementSizeValid(void) const
  {
  return m_ElementSizeValid;
  }

void MetaImage::
ElementSizeValid(bool _elementSizeValid)
  {
  m_ElementSizeValid = _elementSizeValid;
  }

const float * MetaImage::
ElementSize(void) const
  {
  return m_ElementSize;
  }

float MetaImage::
ElementSize(int _i) const
  {
  return m_ElementSize[_i];
  }

void MetaImage::
ElementSize(const float *_elementSize)
  {
  memcpy(m_ElementSize, _elementSize, m_NDims*sizeof(float));
  m_ElementSizeValid = true;
  }

void MetaImage::
ElementSize(int _i, float _value)
  {
  m_ElementSize[_i] = _value;
  m_ElementSizeValid = true;
  }

//
//
//
MET_ValueEnumType MetaImage::
ElementType(void) const
  {
  return m_ElementType;
  }

void MetaImage::
ElementType(MET_ValueEnumType _elementType)
  {
  m_ElementType = _elementType;
  }

//
//
//
int MetaImage::
ElementNumberOfChannels(void) const
  {
  return m_ElementNumberOfChannels;
  }

void MetaImage::
ElementNumberOfChannels(int _elementNumberOfChannels)
  {
  m_ElementNumberOfChannels = _elementNumberOfChannels;
  }

//
//
//
void MetaImage::
ElementByteOrderSwap(METAIO_STL::streamoff _quantity)
  {

  // use the user provided value if provided or the internal ivar
  METAIO_STL::streamoff quantity = _quantity ? _quantity : m_Quantity;

  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaImage: ElementByteOrderSwap"
                        << METAIO_STREAM::endl;
    }

  int eSize;
  MET_SizeOfType(m_ElementType, &eSize);
  switch(eSize)
    {
    default:
    case 0:
    case 1:
      {
      break;
      }
    case 2:
      {
      int i;
      for(i=0; i<quantity*m_ElementNumberOfChannels; i++)
        {
        ((MET_USHORT_TYPE *)m_ElementData)[i] =
              MET_ByteOrderSwapShort(((MET_USHORT_TYPE *)m_ElementData)[i]);
        }
      break;
      }
    case 4:
      {
      int i;
      for(i=0; i<quantity*m_ElementNumberOfChannels; i++)
        {
        ((MET_UINT_TYPE *)m_ElementData)[i] =
              MET_ByteOrderSwapLong(((MET_UINT_TYPE *)m_ElementData)[i]);
        }
      break;
      }
    case 8:
      {
      int i;
      char* data = (char*)m_ElementData;
      for(i=0; i<quantity*m_ElementNumberOfChannels; i++)
        {
        MET_ByteOrderSwap8(data);
        data += 8;
        }
      break;
      }
    }
  m_BinaryDataByteOrderMSB = !m_BinaryDataByteOrderMSB;
  }

bool MetaImage::
ElementByteOrderFix(METAIO_STL::streamoff _quantity)
  {
  if(m_BinaryDataByteOrderMSB != MET_SystemByteOrderMSB())
    {
    ElementByteOrderSwap(_quantity);
    return true;
    }
  return true;
  }

//
//
//
bool MetaImage::
ElementMinMaxValid(void) const
  {
  return m_ElementMinMaxValid;
  }

void MetaImage::
ElementMinMaxValid(bool _elementMinMaxValid)
  {
  m_ElementMinMaxValid = _elementMinMaxValid;
  }

void MetaImage::
ElementMinMaxRecalc(void)
  {
  int i;
  double tf;

  if(m_ElementData == NULL)
    return;

  ElementByteOrderFix();

  MET_ValueToDouble(m_ElementType, m_ElementData, 0, &tf);
  m_ElementMin = tf;
  m_ElementMax = tf;
  for(i=1; i<m_Quantity*m_ElementNumberOfChannels; i++)
    {
    MET_ValueToDouble(m_ElementType, m_ElementData, i, &tf);
    if(tf<m_ElementMin)
      {
      m_ElementMin = tf;
      }
    else if(tf>m_ElementMax)
      {
      m_ElementMax = tf;
      }
    }

  m_ElementMinMaxValid = true;
  }

double MetaImage::
ElementMin(void) const
  {
  return m_ElementMin;
  }

void MetaImage::
ElementMin(double _elementMin)
  {
  m_ElementMin = _elementMin;
  }

double MetaImage::
ElementMax(void) const
  {
  return m_ElementMax;
  }

void MetaImage::
ElementMax(double _elementMax)
  {
  m_ElementMax = _elementMax;
  }

//
//
//
double MetaImage::
ElementToIntensityFunctionSlope(void) const
  {
  return m_ElementToIntensityFunctionSlope;
  }

void MetaImage::
ElementToIntensityFunctionSlope(double _elementToIntensityFunctionSlope)
  {
  m_ElementToIntensityFunctionSlope = _elementToIntensityFunctionSlope;
  }

double MetaImage::
ElementToIntensityFunctionOffset(void) const
  {
  return m_ElementToIntensityFunctionOffset;
  }

void MetaImage::
ElementToIntensityFunctionOffset(double _elementOffset)
  {
  m_ElementToIntensityFunctionOffset = _elementOffset;
  }

//
//
//
bool MetaImage::
AutoFreeElementData(void) const
  {
  return m_AutoFreeElementData;
  }

void MetaImage::
AutoFreeElementData(bool _autoFreeElementData)
  {
  m_AutoFreeElementData = _autoFreeElementData;
  }

//
//
//
const char * MetaImage::
ElementDataFileName(void) const
  {
  return m_ElementDataFileName;
  }

void MetaImage::
ElementDataFileName(const char * _elementDataFileName)
  {
  strcpy(m_ElementDataFileName, _elementDataFileName);
  }

//
//
//
void * MetaImage::
ElementData(void)
  {
  return m_ElementData;
  }

double MetaImage::
ElementData(METAIO_STL::streamoff _i) const
  {
  double tf = 0;
  MET_ValueToDouble(m_ElementType, m_ElementData, _i, &tf);

  return tf;
  }

bool MetaImage::
ElementData(METAIO_STL::streamoff _i, double _v)
  {
  if(_i<m_Quantity)
    {
    MET_DoubleToValue(_v, m_ElementType, m_ElementData, _i);
    return true;
    }
  return false;
  }

void MetaImage::
ElementData(void * _elementData, bool _autoFreeElementData)
  {
  if(m_AutoFreeElementData)
    {
    delete [] (char *)m_ElementData;
    }
  m_ElementData = _elementData;
  m_AutoFreeElementData = _autoFreeElementData;
  }

//
//
//
bool MetaImage::
ConvertElementDataTo(MET_ValueEnumType _elementType,
                     double _toMin, double _toMax)
  {
  int eSize;
  MET_SizeOfType(_elementType, &eSize);
  void * newElementData = new char[static_cast<size_t>(m_Quantity*m_ElementNumberOfChannels*eSize)];

  ElementByteOrderFix();
  if(!ElementMinMaxValid())
    {
    ElementMinMaxRecalc();
    }

  int i;
  for(i=0; i<m_Quantity*m_ElementNumberOfChannels; i++)
    {
    MET_ValueToValue(m_ElementType, m_ElementData, i, _elementType,
                     newElementData, m_ElementMin, m_ElementMax,
                     _toMin, _toMax);
    }

  if(m_AutoFreeElementData)
    {
    delete [] (char *)m_ElementData;
    }
  m_ElementData = newElementData;
  m_ElementType = _elementType;
  m_ElementMinMaxValid = true;
  m_ElementMin = _toMin;
  m_ElementMax = _toMax;
  m_AutoFreeElementData = true;

  return true;
  }

bool MetaImage::
ConvertElementDataToIntensityData(MET_ValueEnumType _elementType)
  {
  ElementByteOrderFix();
  if(!ElementMinMaxValid())
    {
    ElementMinMaxRecalc();
    }

  double toMin = m_ElementMin + m_ElementToIntensityFunctionOffset;
  double toMax = (m_ElementMax-m_ElementMin)
                   * m_ElementToIntensityFunctionSlope
                   + m_ElementMin;

  return ConvertElementDataTo(_elementType, toMin, toMax);
  }

bool MetaImage::
ConvertIntensityDataToElementData(MET_ValueEnumType _elementType)
  {
  ElementByteOrderFix();
  if(!ElementMinMaxValid())
    {
    ElementMinMaxRecalc();
    }

  double toMin = m_ElementMin - m_ElementToIntensityFunctionOffset;
  double toMax = (m_ElementMax - m_ElementMin)
                   / m_ElementToIntensityFunctionSlope
                   + toMin;

  return ConvertElementDataTo(_elementType, toMin, toMax);
  }

// return true if the file exists
bool MetaImage::M_FileExists(const char* filename) const
{
#ifdef _MSC_VER
# define access _access
#endif
#ifndef R_OK
# define R_OK 04
#endif
  if ( access(filename, R_OK) != 0 )
    {
    return false;
    }
  else
    {
    return true;
    }
}

bool MetaImage::FileIsFullPath(const char* in_name) const
{
#if defined(_WIN32) || defined(__CYGWIN__)
  // On Windows, the name must be at least two characters long.
  if(strlen(in_name) < 2)
    {
    return false;
    }
  if(in_name[1] == ':')
    {
    return true;
    }
  if(in_name[0] == '\\')
    {
    return true;
    }
#else
  // On UNIX, the name must be at least one character long.
  if(strlen(in_name) < 1)
    {
    return false;
    }
#endif
#if !defined(_WIN32)
  if(in_name[0] == '~')
    {
    return true;
    }
#endif
  // On UNIX, the name must begin in a '/'.
  // On Windows, if the name begins in a '/', then it is a full
  // network path.
  if(in_name[0] == '/')
    {
    return true;
    }
  return false;
}

// Return the value of a tag
METAIO_STL::string
MetaImage::
M_GetTagValue(const METAIO_STL::string & buffer, const char* tag) const
{
  size_t stringPos = buffer.find(tag);
  if( stringPos == METAIO_STL::string::npos )
    {
    return "";
    }

  size_t pos2 = buffer.find("=",stringPos);
  if(pos2 == METAIO_STL::string::npos )
    {
    pos2 = buffer.find(":",stringPos);
    }

  if(pos2 == METAIO_STL::string::npos )
    {
    return "";
    }

  size_t posend = buffer.find('\r',pos2);
  if(posend == METAIO_STL::string::npos )
    {
    posend = buffer.find('\n',pos2);
    }

  // Get the element data filename
  METAIO_STL::string value = "";
  bool firstspace = true;
  size_t index = pos2+1;
  while(index<buffer.size()
        && buffer[index] != '\r'
        && buffer[index] != '\n'
        )
    {
    if(buffer[index] != ' ')
      {
      firstspace = false;
      }
    if(!firstspace)
      {
      value += buffer[index];
      }
    index++;
    }

  return value;
}

//
//
//
bool MetaImage::
CanRead(const char *_headerName) const
  {
  // First check the extension
  METAIO_STL::string fname = _headerName;
  if(  fname == "" )
    {
    return false;
    }

  bool extensionFound = false;

  METAIO_STL::string::size_type stringPos = fname.rfind(".mhd");
  if ((stringPos != METAIO_STL::string::npos)
      && (stringPos == fname.length() - 4))
    {
    extensionFound = true;
    }

  stringPos = fname.rfind(".mha");
  if ((stringPos != METAIO_STL::string::npos)
      && (stringPos == fname.length() - 4))
    {
    extensionFound = true;
    }

  if( !extensionFound )
    {
    return false;
    }

  // Now check the file content
  METAIO_STREAM::ifstream inputStream;

  openReadStream(inputStream, fname.c_str());

  if( inputStream.fail() )
    {
    return false;
    }

  char* buf = new char[8001];
  inputStream.read(buf,8000);
  unsigned long fileSize = static_cast<unsigned long>(inputStream.gcount());
  buf[fileSize] = 0;
  METAIO_STL::string header(buf);
  header.resize(fileSize);
  delete [] buf;
  inputStream.close();

  stringPos = header.find("NDims");
  if( stringPos == METAIO_STL::string::npos )
    {
    return false;
    }

  METAIO_STL::string elementDataFileName = M_GetTagValue(header,"ElementDataFile");

  return true;
  }

bool MetaImage::
Read(const char *_headerName, bool _readElements, void * _buffer)
  {
  M_Destroy();

  Clear();

  M_SetupReadFields();

  if(_headerName != NULL)
    {
    strcpy(m_FileName, _headerName);
    }

  M_PrepareNewReadStream();

  METAIO_STREAM::ifstream * tmpReadStream = new METAIO_STREAM::ifstream;

  openReadStream(*tmpReadStream, m_FileName);

  if(!tmpReadStream->is_open())
    {
    delete tmpReadStream;
    return false;
    }

  if( !this->ReadStream(0, tmpReadStream, _readElements, _buffer) )
    {
    tmpReadStream->close();
    delete tmpReadStream;
    return false;
    }

  tmpReadStream->close();

  delete tmpReadStream;

  return true;
  }

bool MetaImage::
CanReadStream(METAIO_STREAM::ifstream * _stream) const
  {
  if(!strncmp(MET_ReadType(*_stream).c_str(), "Image", 5))
    {
    return true;
    }
  return false;
  }


bool MetaImage::
ReadStream(int _nDims,
           METAIO_STREAM::ifstream * _stream,
           bool _readElements,
           void * _buffer)
  {
  if(!MetaObject::ReadStream(_nDims, _stream))
    {
    METAIO_STREAM::cerr << "MetaImage: Read: Cannot parse file"
                        << METAIO_STREAM::endl;
    return false;
    }

  if(_readElements)
    {
    if(_buffer == NULL)
      {
      InitializeEssential(m_NDims,
                          m_DimSize,
                          m_ElementSpacing,
                          m_ElementType,
                          m_ElementNumberOfChannels,
                          NULL, true);
      }
    else
      {
      InitializeEssential(m_NDims,
                          m_DimSize,
                          m_ElementSpacing,
                          m_ElementType,
                          m_ElementNumberOfChannels,
                          _buffer, false);
      }

    int i;
    size_t j;
    bool usePath;
    char pathName[MAXPATHLENGHT];
    char fName[MAXPATHLENGHT];
    usePath = MET_GetFilePath(m_FileName, pathName);

    if(!strcmp("Local", m_ElementDataFileName) ||
       !strcmp("LOCAL", m_ElementDataFileName) ||
       !strcmp("local", m_ElementDataFileName))
      {
      M_ReadElements(_stream, m_ElementData, m_Quantity);
      }
    else if(!strncmp("LIST", m_ElementDataFileName,4))
      {
      int fileImageDim = m_NDims - 1;
      int nWrds;
      char **wrds;
      MET_StringToWordArray(m_ElementDataFileName, &nWrds, &wrds);
      if(nWrds > 1)
        {
        fileImageDim = (int)atof(wrds[1]);
        }
      for(i=0; i<nWrds; i++)
        {
        delete [] wrds[i];
        }
      delete [] wrds;
      if ( (fileImageDim == 0) || (fileImageDim > m_NDims) )
        {
        // if optional file dimension size is not given or is larger than
        // overall dimension then default to a size of m_NDims - 1.
        fileImageDim = m_NDims-1;
        }
      char s[1024];
      METAIO_STREAM::ifstream* readStreamTemp = new METAIO_STREAM::ifstream;
      int elementSize;
      MET_SizeOfType(m_ElementType, &elementSize);
      elementSize *= m_ElementNumberOfChannels;
      int totalFiles = 1;
      for (i = m_NDims; i > fileImageDim; i--)
        {
        totalFiles *= m_DimSize[i-1];
        }
      for(i=0; i< totalFiles && !_stream->eof(); i++)
        {
        _stream->getline(s, 1024);
        if(!_stream->eof())
          {
          j = strlen(s)-1;
          while(j>0 && (isspace(s[j]) || !isprint(s[j])))
            {
            s[j--] = '\0';
            }
          if(usePath && !FileIsFullPath(s))
            {
            sprintf(fName, "%s%s", pathName, s);
            }
          else
            {
            strcpy(fName, s);
            }

          openReadStream(*readStreamTemp, fName);
          if(!readStreamTemp->is_open())
            {
            METAIO_STREAM::cerr << "MetaImage: Read: cannot open slice"
                                << METAIO_STREAM::endl;
            continue;
            }
          M_ReadElements(readStreamTemp,
                       &(((char *)m_ElementData)[i*m_SubQuantity[fileImageDim]*
                                                 elementSize]),
                       m_SubQuantity[fileImageDim]);
          readStreamTemp->close();
          }
        }
      delete readStreamTemp;
      }
    else if(strstr(m_ElementDataFileName, "%"))
      {
      int elementSize;
      MET_SizeOfType(m_ElementType, &elementSize);
      elementSize *= m_ElementNumberOfChannels;

      int nWrds;
      char **wrds;
      int minV = 1;
      int maxV = m_DimSize[m_NDims-1];
      int stepV = 1;
      char s[MAXPATHLENGHT];
      METAIO_STREAM::ifstream* readStreamTemp = new METAIO_STREAM::ifstream;
      MET_StringToWordArray(m_ElementDataFileName, &nWrds, &wrds);
      if(nWrds >= 2)
        {
        minV = (int)atof(wrds[1]);
        maxV = minV + m_DimSize[m_NDims-1] - 1;
        }
      if(nWrds >= 3)
        {
        maxV = (int)atof(wrds[2]);
        stepV = (maxV-minV)/(m_DimSize[m_NDims-1]);
        }
      if(nWrds >= 4)
        {
        stepV = (int)atof(wrds[3]);
        }
      if(nWrds >= 5 )
      {
        // In this case, the filename must have had spaces in the
        // name.  The filename was parsed into multiple pieces by the
        // MET_StringToWordArray, which parses based on spaces.
        // Thus, we need to reconstruct the filename in this case.
        // The last three wrds must be numbers.  If they are not, we give an error.
        for( i = nWrds-3; i < nWrds; i++ )
        {
          for( j = 0; j < strlen(wrds[i]); j++ )
          {
            if( !isdigit(wrds[i][j]) )
            {
              METAIO_STREAM::cerr << "MetaImage: Read: Last three arguments must be numbers!"
                  << METAIO_STREAM::endl;
              continue;
            }
          }
        }
        stepV = (int)atof(wrds[nWrds-1]);
        maxV =  (int)atof(wrds[nWrds-2]);
        minV =  (int)atof(wrds[nWrds-3]);
        for( i = 1; i < nWrds-3; i++ )
        {
          strcat(wrds[0]," ");
          strcat(wrds[0],wrds[i]);
        }
      }
      // If the specified size of the third dimension is less than the size
      // specified by the regular expression, we should only read a volume with the specified
      // size.  Otherwise, the code will crash when trying to fill m_ElementData more than it can hold.
      // Therefore, we modify maxV to ensure that the images spanned by minV:stepV:maxV are less than or equal
      // to the size in the last dimension.
      int numberOfImages = 1 + (maxV - minV)/stepV;
      if( numberOfImages > m_DimSize[m_NDims-1] )
      {
        maxV = (m_DimSize[m_NDims-1]-1)*stepV + minV;
      }
      int cnt = 0;
      for(i=minV; i<=maxV; i += stepV)
        {
        sprintf(s, wrds[0], i);
        if(usePath && !FileIsFullPath(s))
          {
          sprintf(fName, "%s%s", pathName, s);
          }
        else
          {
          strcpy(fName, s);
          }
        openReadStream(*readStreamTemp,fName);
        if(!readStreamTemp->is_open())
          {
          METAIO_STREAM::cerr << "MetaImage: Read: cannot construct file"
                              << METAIO_STREAM::endl;
          continue;
          }

        M_ReadElements(readStreamTemp,
                       &(((char *)m_ElementData)[cnt*m_SubQuantity[m_NDims-1]*
                                                 elementSize]),
                       m_SubQuantity[m_NDims-1]);
        cnt++;

        readStreamTemp->close();
        }
      delete readStreamTemp;
      for(i=0; i<nWrds; i++)
        {
        delete [] wrds[i];
        }
      delete [] wrds;
      }
    else
      {
      if(usePath && !FileIsFullPath(m_ElementDataFileName))
        {
        sprintf(fName, "%s%s", pathName, m_ElementDataFileName);
        }
      else
        {
        strcpy(fName, m_ElementDataFileName);
        }

      METAIO_STREAM::ifstream* readStreamTemp = new METAIO_STREAM::ifstream;

      const char *extensions[] = { "", ".gz", ".Z", 0 };
      for(unsigned ii = 0; extensions[ii] != 0; ii++)
        {
        METAIO_STL::string tempFName(fName);
        tempFName += extensions[ii];
        openReadStream(*readStreamTemp,tempFName.c_str());
        if(readStreamTemp->is_open())
          {
          if(ii > 0)
            {
            this->CompressedData(true);
            this->BinaryData(true);
            }
          break;
          }
        }

      if(!readStreamTemp->is_open())
        {
        METAIO_STREAM::cerr << "MetaImage: Read: Cannot open data file"
                            << METAIO_STREAM::endl;
        if(m_ReadStream)
          {
          m_ReadStream->close();
          }
        return false;
        }
      M_ReadElements(readStreamTemp, m_ElementData, m_Quantity);

      readStreamTemp->close();
      delete readStreamTemp;
      }
    }

  return true;
  }


//
//
//
bool MetaImage::
Write(const char *_headName,
      const char *_dataName,
      bool _writeElements,
      const void * _constElementData,
      bool _append)
  {
  if(_headName != NULL)
    {
    FileName(_headName);
    }

  bool userDataFileName = true;
  if(_dataName == NULL && strlen(m_ElementDataFileName) == 0)
    {
    userDataFileName = false;
    int sPtr = 0;
    MET_GetFileSuffixPtr(m_FileName, &sPtr);
    if(!strcmp(&m_FileName[sPtr], "mha"))
      {
      ElementDataFileName("LOCAL");
      }
    else
      {
      if(!_append)
        {
        MET_SetFileSuffix(m_FileName, "mhd");
        }
      strcpy(m_ElementDataFileName, m_FileName);
      if(m_CompressedData)
        {
        MET_SetFileSuffix(m_ElementDataFileName, "zraw");
        }
      else
        {
        MET_SetFileSuffix(m_ElementDataFileName, "raw");
        }
      }
    }
  else if(_dataName != NULL)
    {
    userDataFileName = false;
    ElementDataFileName(_dataName);
    }

  // make sure suffix is valid
  if(!_append)
    {
    if(!strcmp(m_ElementDataFileName, "LOCAL"))
      {
      MET_SetFileSuffix(m_FileName, "mha");
      }
     else
      {
      MET_SetFileSuffix(m_FileName, "mhd");
      }
    }

  char pathName[MAXPATHLENGHT];
  bool usePath = MET_GetFilePath(m_FileName, pathName);
  if(usePath)
    {
    char elementPathName[MAXPATHLENGHT];
    MET_GetFilePath(m_ElementDataFileName, elementPathName);
    if(!strcmp(pathName, elementPathName))
      {
      strcpy(elementPathName, &m_ElementDataFileName[strlen(pathName)]);
      strcpy(m_ElementDataFileName, elementPathName);
      }
    }

  METAIO_STREAM::ofstream * tmpWriteStream = new METAIO_STREAM::ofstream;

  openWriteStream(*tmpWriteStream, m_FileName, _append);

  if(!tmpWriteStream->is_open())
    {
    if(!userDataFileName)
      {
      ElementDataFileName("");
      }

    delete tmpWriteStream;

    return false;
    }

  bool result = MetaImage::WriteStream(tmpWriteStream,
                                       _writeElements,
                                       _constElementData);

  if(!userDataFileName)
    {
    ElementDataFileName("");
    }

  tmpWriteStream->close();
  delete tmpWriteStream;

  return result;
  }

bool MetaImage::
WriteStream(METAIO_STREAM::ofstream * _stream,
            bool _writeElements,
            const void * _constElementData)
  {
  if(m_WriteStream != NULL)
    {
    METAIO_STREAM::cerr << "MetaArray: WriteStream: two files open?"
                        << METAIO_STREAM::endl;
    delete m_WriteStream;
    }

  m_WriteStream = _stream;

  unsigned char * compressedElementData = NULL;
  if(m_BinaryData && m_CompressedData && !strstr(m_ElementDataFileName, "%"))
    // compressed & !slice/file
    {
    int elementSize;
    MET_SizeOfType(m_ElementType, &elementSize);
    int elementNumberOfBytes = elementSize*m_ElementNumberOfChannels;

    if(_constElementData == NULL)
      {
      compressedElementData = MET_PerformCompression(
                                  (const unsigned char *)m_ElementData,
                                  m_Quantity * elementNumberOfBytes,
                                  & m_CompressedDataSize );
      }
    else
      {
      compressedElementData = MET_PerformCompression(
                                  (const unsigned char *)_constElementData,
                                  m_Quantity * elementNumberOfBytes,
                                  & m_CompressedDataSize );
      }
    }

  M_SetupWriteFields();

  M_Write();

  if(_writeElements)
    {
    if(m_BinaryData && m_CompressedData && !strstr(m_ElementDataFileName, "%"))
      // compressed & !slice/file
      {
      M_WriteElements(m_WriteStream,
                      compressedElementData,
                      m_CompressedDataSize);

      delete [] compressedElementData;
      m_CompressedDataSize = 0;
      }
    else
      {
      if(_constElementData == NULL)
        {
        M_WriteElements(m_WriteStream,
                        m_ElementData,
                        m_Quantity);
        }
      else
        {
        M_WriteElements(m_WriteStream,
                        _constElementData,
                        m_Quantity);
        }
      }
    }

  m_WriteStream = NULL;

  return true;
  }


/** Write a portion of an image */
bool MetaImage::WriteROI( int * _indexMin, int * _indexMax,
                          const char *_headName,
                          const char *_dataName,
                          bool _writeElements,
                          const void * _constElementData,
                          bool _append
                          )
{
  if( _headName != NULL )
    {
    FileName( _headName );
    }

  if( !_writeElements )
    {
    return false;
    }

  // Check if the file exists
  if( M_FileExists(_headName) )
    {
    char* elementData = const_cast<char*>(
                            static_cast<const char*>(_constElementData) );
    if( elementData == NULL )
      {
      elementData = (char*)m_ElementData;
      }
    if( elementData == NULL )
      {
      METAIO_STREAM::cerr << "Element data is NULL" << METAIO_STREAM::endl;
      return false;
      }

    // Find the start of the data
    METAIO_STREAM::ifstream * readStream = new METAIO_STREAM::ifstream;
    readStream->open( m_FileName, METAIO_STREAM::ios::binary |
                                  METAIO_STREAM::ios::in);

    // File must be readable
    if( !MetaObject::ReadStream( m_NDims, readStream ) )
      {
      METAIO_STREAM::cerr << "MetaImage: Read: Cannot parse file"
                          << METAIO_STREAM::endl;
      delete readStream;
      return false;
      }

    // File must not be compressed
    if(m_CompressedData)
      {
      METAIO_STREAM::cerr
               << "MetaImage cannot insert ROI into a compressed file."
               << METAIO_STREAM::endl;
      readStream->close();
      delete readStream;
      return false;
      }

    InitializeEssential( m_NDims,
                         m_DimSize,
                         m_ElementSpacing,
                         m_ElementType,
                         m_ElementNumberOfChannels,
                         NULL, false ); // no memory allocation

    METAIO_STL::string  filename = ElementDataFileName();
    METAIO_STL::streampos dataPos = 0;

    // local file
    if( filename == "LOCAL" )
      {
      filename = m_FileName;
      dataPos = readStream->tellg();
      }
    else if( filename == "LIST"
             || strstr(filename.c_str(), "%") )
      {
      METAIO_STREAM::cerr
               << "MetaImage cannot insert ROI into a list of files."
               << METAIO_STREAM::endl;
      readStream->close();
      delete readStream;
      return false;
      }

    readStream->close();
    delete readStream;

    // Write the region
    if( !M_FileExists(filename.c_str()) )
      {
      char pathName[MAXPATHLENGHT];
      MET_GetFilePath(_headName, pathName);
      filename = pathName+filename;
      }

    METAIO_STREAM::ofstream * tmpWriteStream = new METAIO_STREAM::ofstream;
    tmpWriteStream->open( filename.c_str(),
                          METAIO_STREAM::ios::binary |
                          METAIO_STREAM::ios::in |
                          METAIO_STREAM::ios::out );

    if( !tmpWriteStream->is_open() )
      {
      METAIO_STREAM::cerr << "Cannot open ROI file: "
                          << filename.c_str()
                          << METAIO_STREAM::endl;
      delete tmpWriteStream;
      return false;
      }

    int elementSize;
    MET_SizeOfType( m_ElementType, &elementSize );
    int elementNumberOfBytes = elementSize*m_ElementNumberOfChannels;

    // seek to the end and write one byte to allocate the entire file size
    METAIO_STL::streamoff seekoff = m_Quantity*elementNumberOfBytes;
    tmpWriteStream->seekp(0, METAIO_STREAM::ios::end);
    if (tmpWriteStream->tellp() != (dataPos+seekoff))
      {
      seekoff = seekoff - 1;
      tmpWriteStream->seekp(dataPos+seekoff, METAIO_STREAM::ios::beg);
      const char zerobyte = 0;
      tmpWriteStream->write(&zerobyte, 1);
      }

    if( !elementData )
      {
      METAIO_STREAM::cerr << "Element data is NULL" << METAIO_STREAM::endl;
      delete tmpWriteStream;
      return false;
      }

    M_WriteElementsROI(tmpWriteStream, elementData, dataPos,
                       _indexMin, _indexMax);

    tmpWriteStream->close();
    delete tmpWriteStream;
    }
  else // the file doesn't exist
    {
    if(m_CompressedData)
      {
      METAIO_STREAM::cerr
               << "MetaImage cannot write an ROI using compression."
               << METAIO_STREAM::endl;
      return false;
      }

    // Get the data filename right...
    bool userDataFileName = true;
    if( _dataName == NULL && strlen(m_ElementDataFileName) == 0 )
      {
      userDataFileName = false;
      int sPtr = 0;
      MET_GetFileSuffixPtr(m_FileName, &sPtr);
      if( !strcmp(&m_FileName[sPtr], "mha") )
        {
        ElementDataFileName( "LOCAL" );
        }
      else
        {
        if(!_append)
          {
          MET_SetFileSuffix(m_FileName, "mhd");
          }
        strcpy(m_ElementDataFileName, m_FileName);
        if(m_CompressedData)
          {
          MET_SetFileSuffix(m_ElementDataFileName, "zraw");
          }
        else
          {
          MET_SetFileSuffix(m_ElementDataFileName, "raw");
          }
        }
      }
    else if(_dataName != NULL)
      {
      userDataFileName = false;
      ElementDataFileName(_dataName);
      }

    if( !strcmp(m_ElementDataFileName, "LIST")
        || strstr(m_ElementDataFileName, "%") )
      {
      METAIO_STREAM::cerr
               << "MetaImage cannot insert ROI into a list of files."
               << METAIO_STREAM::endl;
      return false;
      }

    // make sure the header suffix is valid, unless forcing to match an
    // existing file via the append bool argument.
    if(!_append)
      {
      if(!strcmp(m_ElementDataFileName, "LOCAL"))
        {
        MET_SetFileSuffix(m_FileName, "mha");
        }
       else
        {
        MET_SetFileSuffix(m_FileName, "mhd");
        }
      }

    char pathName[MAXPATHLENGHT];
    bool usePath = MET_GetFilePath(m_FileName, pathName);
    if(usePath)
      {
      char elementPathName[MAXPATHLENGHT];
      MET_GetFilePath(m_ElementDataFileName, elementPathName);
      if(!strcmp(pathName, elementPathName))
        {
        strcpy(elementPathName, &m_ElementDataFileName[strlen(pathName)]);
        strcpy(m_ElementDataFileName, elementPathName);
        }
      }

    METAIO_STREAM::ofstream * tmpWriteStream = new METAIO_STREAM::ofstream;

    openWriteStream(*tmpWriteStream, m_FileName, _append);

    if(!tmpWriteStream->is_open())
      {
      if(!userDataFileName)
        {
        ElementDataFileName("");
        }
      delete tmpWriteStream;
      return false;
      }

    // Write the ROI header file
    char* elementData = const_cast<char*>(
                          static_cast<const char*>(_constElementData) );
    if(elementData == NULL)
      {
      elementData = (char*)m_ElementData;
      }

    m_WriteStream = tmpWriteStream;
    M_SetupWriteFields();
    M_Write();

    METAIO_STL::streampos dataPos = m_WriteStream->tellp();

    // If data is in a separate file, set dataPos and point to that file.
    //   ( we've already verified the name isn't LIST and doesn't
    //     contain % )
    if( strcmp( m_ElementDataFileName, "LOCAL" ) )
      {
      m_WriteStream = NULL;
      tmpWriteStream->close();

      dataPos = 0;

      char dataFileName[MAXPATHLENGHT];
      if(usePath&& !FileIsFullPath(m_ElementDataFileName))
        {
        sprintf(dataFileName, "%s%s", pathName, m_ElementDataFileName);
        }
      else
        {
        strcpy(dataFileName, m_ElementDataFileName);
        }

      openWriteStream(*tmpWriteStream, dataFileName, _append);
      m_WriteStream = tmpWriteStream;
      }

    int elementSize;
    MET_SizeOfType( m_ElementType, &elementSize );
    int elementNumberOfBytes = elementSize*m_ElementNumberOfChannels;

    // write the last byte in the file to allocate it
    METAIO_STL::streamoff seekoff = m_Quantity * elementNumberOfBytes;
    seekoff -= 1;
    m_WriteStream->seekp(seekoff, METAIO_STREAM::ios::cur);
    const char zerobyte = 0;
    m_WriteStream->write(&zerobyte, 1);

    M_WriteElementsROI(m_WriteStream, elementData, dataPos,
                       _indexMin, _indexMax);

    m_WriteStream = NULL;

    if(!userDataFileName)
      {
      ElementDataFileName("");
      }

    tmpWriteStream->close();
    delete tmpWriteStream;
    }

  return true;
}

bool  MetaImage::
M_WriteElementsROI(METAIO_STREAM::ofstream * _fstream,
                   const void * _data,
                   METAIO_STL::streampos _dataPos,
                   int * _indexMin,
                   int* _indexMax )
{
  const char* data = static_cast<const char*>(_data);

  int elementSize;
  MET_SizeOfType(m_ElementType, &elementSize);
  const int elementNumberOfBytes = elementSize*m_ElementNumberOfChannels;

  // Write the IO region line by line
  int * currentIndex = new int[m_NDims];
  for(int i=0; i<m_NDims; i++)
    {
    currentIndex[i] = _indexMin[i];
    }

  // Optimize the size of the buffer to written depending on the
  // region shape
  // This calculate the number of continuous bytes in the file
  // which can be written
  METAIO_STL::streamoff elementsToWrite = 1;
  int movingDirection = 0;
  do
    {
    elementsToWrite *= _indexMax[movingDirection] - _indexMin[movingDirection] + 1;
    ++movingDirection;
    }
  while(movingDirection < m_NDims
        && _indexMin[movingDirection-1] == 0
        && _indexMax[movingDirection-1] == m_DimSize[movingDirection-1]-1);

  // write line by line
  bool done = false;
  while(!done)
    {
    // Seek to the right position
    METAIO_STL::streamoff seekoff = _dataPos;
    for(int i=0; i<m_NDims; i++)
      {
      seekoff += m_SubQuantity[i] * currentIndex[i] * elementNumberOfBytes;
      }
    _fstream->seekp( seekoff, METAIO_STREAM::ios::beg );

    M_WriteElementData( _fstream, data, elementsToWrite );
    data += elementsToWrite * elementNumberOfBytes;

    // check if there is only one write needed
    if( movingDirection >= m_NDims )
      {
      break;
      }

    ++currentIndex[movingDirection];

    // Check if we are still in the region
    for( int j=movingDirection; j<m_NDims; j++ )
      {
      if( currentIndex[j] > _indexMax[j] )
        {
        if( j == m_NDims-1 )
          {
          done = true;
          break;
          }
        else
          {
          currentIndex[j] = _indexMin[j];
          currentIndex[j+1]++;
          }
        }
      }
    } // end writing  loop

  delete [] currentIndex;

  return true;
}

bool MetaImage::
Append(const char *_headName)
  {
  if(META_DEBUG)
   {
   METAIO_STREAM::cout << "MetaImage: Append" << METAIO_STREAM::endl;
   }

  return this->Write(_headName, NULL, true, NULL, true);
  }

void MetaImage::
M_Destroy(void)
  {
  if(m_AutoFreeElementData && m_ElementData != NULL)
    {
    delete [] (char *)m_ElementData;
    }

  m_ElementData = NULL;

  if(m_CompressionTable && m_CompressionTable->compressedStream)
    {
    inflateEnd(m_CompressionTable->compressedStream);
    delete m_CompressionTable->compressedStream;
    delete [] m_CompressionTable->buffer;
    m_CompressionTable->buffer = NULL;
    }
  delete m_CompressionTable;
  m_CompressionTable = NULL;

  MetaObject::M_Destroy();
  }

void MetaImage::
M_SetupReadFields(void)
  {
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaImage: M_SetupReadFields"
                        << METAIO_STREAM::endl;
    }

  MetaObject::M_SetupReadFields();

  MET_FieldRecordType * mF;

  int nDimsRecNum = MET_GetFieldRecordNumber("NDims", &m_Fields);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "DimSize", MET_INT_ARRAY, true, nDimsRecNum);
  mF->required = true;
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "HeaderSize", MET_INT, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "Modality", MET_STRING, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "ImagePosition", MET_FLOAT_ARRAY, false, nDimsRecNum);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "SequenceID", MET_INT_ARRAY, false, nDimsRecNum);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "ElementMin", MET_FLOAT, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "ElementMax", MET_FLOAT, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "ElementNumberOfChannels", MET_INT, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "ElementSize", MET_FLOAT_ARRAY, false, nDimsRecNum);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;  // Set but not used...
  MET_InitReadField(mF, "ElementNBits", MET_INT, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;  // Used by ConvertElementToIntensity funcs
  MET_InitReadField(mF, "ElementToIntensityFunctionSlope", MET_FLOAT, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;  // Used by ConvertElementToIntensity funcs
  MET_InitReadField(mF, "ElementToIntensityFunctionOffset", MET_FLOAT, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "ElementType", MET_STRING, true);
  mF->required = true;
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "ElementDataFile", MET_STRING, true);
  mF->required = true;
  mF->terminateRead = true;
  m_Fields.push_back(mF);
  }

void MetaImage::
M_SetupWriteFields(void)
  {
  strcpy(m_ObjectTypeName,"Image");
  MetaObject::M_SetupWriteFields();

  MET_FieldRecordType * mF;

  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "DimSize", MET_INT_ARRAY, m_NDims, m_DimSize);
  m_Fields.push_back(mF);

  char s[MAXPATHLENGHT];
  if(m_HeaderSize > 0 || m_HeaderSize == -1)
    {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "HeaderSize", MET_INT);
    m_Fields.push_back(mF);
    }

  int i;
  if(m_Modality != MET_MOD_UNKNOWN)
    {
    mF = new MET_FieldRecordType;
    strcpy(s, MET_ValueTypeName[m_Modality]);
    MET_InitWriteField(mF, "Modality", MET_STRING, strlen(s), s);
    m_Fields.push_back(mF);
    }

  i = MET_GetFieldRecordNumber("AnatomicalOrientation", &m_Fields);
  if(i < 0)
    {
    const char * str = AnatomicalOrientationAcronym();
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "AnatomicalOrientation",
                       MET_STRING, strlen(str), str);
    m_Fields.push_back(mF);
    }

  bool valid = false;
  for(i=0; i<4; i++)
    {
    if(m_SequenceID[i] != 0)
      {
      valid = true;
      break;
      }
    }
  if(valid)
    {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "SequenceID", MET_FLOAT_ARRAY, m_NDims,
                       m_SequenceID);
    m_Fields.push_back(mF);
    }

  if(m_ElementMinMaxValid)
    {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "ElementMin", MET_FLOAT, m_ElementMin);
    m_Fields.push_back(mF);

    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "ElementMax", MET_FLOAT, m_ElementMax);
    m_Fields.push_back(mF);
    }

  if(m_ElementNumberOfChannels>1)
    {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "ElementNumberOfChannels", MET_INT,
                       m_ElementNumberOfChannels);
    m_Fields.push_back(mF);
    }

  if(m_ElementSizeValid)
    {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "ElementSize", MET_FLOAT_ARRAY, m_NDims,
                       m_ElementSize);
    m_Fields.push_back(mF);
    }

  if(m_ElementToIntensityFunctionSlope != 1 ||
     m_ElementToIntensityFunctionOffset != 0)
    {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "ElementToIntensityFunctionSlope",
                       MET_FLOAT, m_ElementToIntensityFunctionSlope);
    m_Fields.push_back(mF);
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "ElementToIntensityFunctionOffset",
                       MET_FLOAT, m_ElementToIntensityFunctionOffset);
    m_Fields.push_back(mF);
    }

  mF = new MET_FieldRecordType;
  MET_TypeToString(m_ElementType, s);
  MET_InitWriteField(mF, "ElementType", MET_STRING, strlen(s), s);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "ElementDataFile", MET_STRING,
                     strlen(m_ElementDataFileName),
                     m_ElementDataFileName);
  mF->terminateRead = true;
  m_Fields.push_back(mF);
  }

//
//
//
bool MetaImage::
M_Read(void)
  {
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaImage: M_Read: Loading Header"
                        << METAIO_STREAM::endl;
    }
  if(!MetaObject::M_Read())
    {
    METAIO_STREAM::cerr << "MetaImage: M_Read: Error parsing file"
                        << METAIO_STREAM::endl;
    return false;
    }

  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaImage: M_Read: Parsing Header"
                        << METAIO_STREAM::endl;
    }
  MET_FieldRecordType * mF;

  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "metaImage: M_Read: elementSpacing[" << 0 << "] = "
                        << m_ElementSpacing[0] << METAIO_STREAM::endl;
    }
  mF = MET_GetFieldRecord("DimSize", &m_Fields);
  if(mF && mF->defined)
    {
    int i;
    for(i=0; i<m_NDims; i++)
      {
      m_DimSize[i] = (int)mF->value[i];
      }
    }

  mF = MET_GetFieldRecord("HeaderSize", &m_Fields);
  if(mF && mF->defined)
    {
    m_HeaderSize = (int)mF->value[0];
    }

  mF = MET_GetFieldRecord("Modality", &m_Fields);
  if(mF && mF->defined)
    {
    MET_StringToImageModality((char *)mF->value, &m_Modality);
    }

  mF = MET_GetFieldRecord("SequenceID", &m_Fields);
  if(mF && mF->defined)
    {
    int i;
    for(i=0; i<m_NDims; i++)
      {
      m_SequenceID[i] = (float)(mF->value[i]);
      }
    }

  mF = MET_GetFieldRecord("ImagePosition", &m_Fields);
  if(mF && mF->defined)
    {
    int i;
    for(i=0; i<m_NDims; i++)
      {
      m_Offset[i] = static_cast<double>(mF->value[i]);
      }
    }

  mF = MET_GetFieldRecord("ElementMin", &m_Fields);
  if(mF && mF->defined)
    {
    m_ElementMin = mF->value[0];
    }

  mF = MET_GetFieldRecord("ElementMax", &m_Fields);
  if(mF && mF->defined)
    {
    m_ElementMax = mF->value[0];
    }

  mF = MET_GetFieldRecord("ElementNumberOfChannels", &m_Fields);
  if(mF && mF->defined)
    {
    m_ElementNumberOfChannels = (int)mF->value[0];
    }


  mF = MET_GetFieldRecord("ElementSize", &m_Fields);
  if(mF && mF->defined)
    {
    m_ElementSizeValid = true;
    int i;
    for(i=0; i<m_NDims; i++)
      {
      m_ElementSize[i] = (float)(mF->value[i]);
      }
    mF = MET_GetFieldRecord("ElementSpacing", &m_Fields);
    if(mF && !mF->defined)
      {
      for(i=0; i<m_NDims; i++)
        {
        m_ElementSpacing[i] = m_ElementSize[i];
        }
      }
    }
  else
    {
    int i;
    m_ElementSizeValid = false;
    for(i=0; i<m_NDims; i++)
      {
      m_ElementSize[i] = m_ElementSpacing[i];
      }
    }

  m_ElementToIntensityFunctionSlope = 1;
  m_ElementToIntensityFunctionOffset = 0;
  mF = MET_GetFieldRecord("ElementToIntensityFunctionSlope", &m_Fields);
  if(mF && mF->defined)
    {
    m_ElementToIntensityFunctionSlope = mF->value[0];
    }
  mF = MET_GetFieldRecord("ElementToIntensityFunctionOffset", &m_Fields);
  if(mF && mF->defined)
    {
    m_ElementToIntensityFunctionOffset = mF->value[0];
    }

  mF = MET_GetFieldRecord("ElementType", &m_Fields);
  if(mF && mF->defined)
    {
    MET_StringToType((char *)(mF->value), &m_ElementType);
    }

  mF = MET_GetFieldRecord("ElementDataFile", &m_Fields);
  if(mF && mF->defined)
    {
    strcpy(m_ElementDataFileName, (char *)(mF->value));
    }

  return true;
  }

//
//
//
bool MetaImage::
M_ReadElements(METAIO_STREAM::ifstream * _fstream, void * _data,
               METAIO_STL::streamoff _dataQuantity)
  {
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaImage: M_ReadElements" << METAIO_STREAM::endl;
    }

  if(m_HeaderSize>(int)0)
    {
    _fstream->seekg(m_HeaderSize, METAIO_STREAM::ios::beg);
    if(!_fstream->good())
      {
      METAIO_STREAM::cerr << "MetaImage: Read: header not read correctly"
                          << METAIO_STREAM::endl;
      return false;
      }
    }

  int elementSize;
  MET_SizeOfType(m_ElementType, &elementSize);
  METAIO_STL::streamoff readSize = _dataQuantity*m_ElementNumberOfChannels*elementSize;
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaImage: M_ReadElements: ReadSize = "
                        << readSize << METAIO_STREAM::endl;
    }

  if(m_HeaderSize == -1)
    {
    if(META_DEBUG)
      {
      METAIO_STREAM::cout << "MetaImage: M_ReadElements: Skipping header"
                          << METAIO_STREAM::endl;
      }
    _fstream->seekg(-readSize, METAIO_STREAM::ios::end);
    }

  // If compressed we inflate
  if(m_BinaryData && m_CompressedData)
    {
    // if m_CompressedDataSize is not defined we assume the size of the
    // file is the size of the compressed data
    bool compressedDataDeterminedFromFile = false;
    if(m_CompressedDataSize==0)
      {
      compressedDataDeterminedFromFile = true;
      _fstream->seekg(0, METAIO_STREAM::ios::end);
      m_CompressedDataSize = _fstream->tellg();
      _fstream->seekg(0, METAIO_STREAM::ios::beg);
      }

    unsigned char* compr = new unsigned char[static_cast<size_t>(m_CompressedDataSize)];

    M_ReadElementData( _fstream, compr, m_CompressedDataSize );

    MET_PerformUncompression(compr, m_CompressedDataSize,
                             (unsigned char *)_data, readSize);

    if (compressedDataDeterminedFromFile)
      {
      m_CompressedDataSize = 0;
      }

    delete [] compr;
    }
  else // if not compressed
    {
    if(!m_BinaryData)
      {

      M_ReadElementData( _fstream, _data, _dataQuantity );

      }
    else
      {

      if ( !M_ReadElementData( _fstream, _data, _dataQuantity ) )
        return false;

      }
    }

  return true;
  }

bool MetaImage::
M_WriteElements(METAIO_STREAM::ofstream * _fstream,
                const void * _data,
                METAIO_STL::streamoff _dataQuantity)
  {

  if(!strcmp(m_ElementDataFileName, "LOCAL"))
    {
    MetaImage::M_WriteElementData(_fstream, _data, _dataQuantity);
    }
  else // write the data in a separate file
    {
    char dataFileName[MAXPATHLENGHT];
    char pathName[MAXPATHLENGHT];
    bool usePath = MET_GetFilePath(m_FileName, pathName);
    if(usePath&& !FileIsFullPath(m_ElementDataFileName))
      {
      sprintf(dataFileName, "%s%s", pathName, m_ElementDataFileName);
      }
    else
      {
      strcpy(dataFileName, m_ElementDataFileName);
      }

    if(strstr(dataFileName, "%")) // write slice by slice
      {
      int i;
      char fName[MAXPATHLENGHT];
      int elementSize;
      MET_SizeOfType(m_ElementType, &elementSize);
      METAIO_STL::streamoff elementNumberOfBytes = elementSize*m_ElementNumberOfChannels;
      METAIO_STL::streamoff sliceNumberOfBytes = m_SubQuantity[m_NDims-1]*elementNumberOfBytes;

      METAIO_STREAM::ofstream* writeStreamTemp = new METAIO_STREAM::ofstream;
      for(i=1; i<=m_DimSize[m_NDims-1]; i++)
        {
        sprintf(fName, dataFileName, i);

        openWriteStream(*writeStreamTemp, fName, false);

        if(!m_CompressedData)
          {
          // BUG? This looks wrong to me as the third parameter should
          // contain the number of elements/quantity, not number of bytes -BCL
          MetaImage::M_WriteElementData(writeStreamTemp,
                             &(((const char *)_data)[(i-1)*sliceNumberOfBytes]),
                             sliceNumberOfBytes);
          }
        else
          {
          unsigned char * compressedData = NULL;
          METAIO_STL::streamoff compressedDataSize = 0;

          // Compress the data slice by slice
          compressedData = MET_PerformCompression(
                  &(((const unsigned char *)_data)[(i-1)*sliceNumberOfBytes]),
                  sliceNumberOfBytes,
                  & compressedDataSize );

          // Write the compressed data
          MetaImage::M_WriteElementData( writeStreamTemp,
                              compressedData,
                              compressedDataSize );

          delete [] compressedData;
          }

        writeStreamTemp->close();
        }

      delete writeStreamTemp;
      }
    else // write the image in one unique other file
      {
      METAIO_STREAM::ofstream* writeStreamTemp = new METAIO_STREAM::ofstream;
      openWriteStream(*writeStreamTemp, dataFileName, false);

      MetaImage::M_WriteElementData(writeStreamTemp, _data, _dataQuantity);

      writeStreamTemp->close();
      delete writeStreamTemp;
      }
    }

  return true;
  }


bool MetaImage::
M_WriteElementData(METAIO_STREAM::ofstream * _fstream,
                   const void * _data,
                   METAIO_STL::streamoff _dataQuantity)
  {
  if(!m_BinaryData)
    {

    double tf;
    for(METAIO_STL::streamoff i=0; i<_dataQuantity; i++)
      {
      MET_ValueToDouble(m_ElementType, _data, i, &tf);
      if((i+1)/10 == (double)(i+1.0)/10.0)
        {
        (*_fstream) << tf << METAIO_STREAM::endl;
        }
      else
        {
        (*_fstream) << tf << " ";
        }
      }
    }
  else
    {
    if(m_CompressedData)
      {
      // the data is writen in writes no bigger then MaxIOChunk
      METAIO_STL::streamoff bytesRemaining = _dataQuantity;
      while ( bytesRemaining )
        {
        METAIO_STL::streamoff chunkToWrite = bytesRemaining > MaxIOChunk ? MaxIOChunk : bytesRemaining;
        _fstream->write( (const char *)_data, (size_t)chunkToWrite );
        _data = (const char *)(_data) + chunkToWrite; // <- Note: data is changed
        bytesRemaining -= chunkToWrite;
        }
      }
    else
      {
      int elementSize;
      MET_SizeOfType(m_ElementType, &elementSize);
      METAIO_STL::streamoff elementNumberOfBytes = elementSize*m_ElementNumberOfChannels;

      // the data is writen in writes no bigger then MaxIOChunk
      METAIO_STL::streamoff bytesRemaining = _dataQuantity * elementNumberOfBytes;
      while ( bytesRemaining )
        {
        METAIO_STL::streamoff chunkToWrite = bytesRemaining > MaxIOChunk ? MaxIOChunk : bytesRemaining;
        _fstream->write( (const char *)_data, (size_t)chunkToWrite );
        _data = (const char *)(_data) + chunkToWrite; // <- Note: _data is changed
        bytesRemaining -= chunkToWrite;
        }
      }
    }

  // check the the io stream did not fail in the process of writing
  if ( _fstream->fail() )
    {
    METAIO_STREAM::cerr
      << "MetaImage: M_WriteElementsData: file stream is fail after write"
      << METAIO_STREAM::endl;
    return false;
    }

  return true;
  }

/** Streaming related functions */
bool MetaImage::
ReadROI(int * _indexMin, int * _indexMax,
        const char *_headerName,
        bool _readElements,
        void * _buffer,
        unsigned int subSamplingFactor)
{
  M_Destroy();

  Clear();

  M_SetupReadFields();

  if(_headerName != NULL)
    {
    strcpy(m_FileName, _headerName);
    }

  M_PrepareNewReadStream();

  METAIO_STREAM::ifstream * tmpReadStream = new METAIO_STREAM::ifstream;

  openReadStream(*tmpReadStream, m_FileName);

  if(!tmpReadStream->is_open())
    {
    delete tmpReadStream;
    return false;
    }

  if( !this->ReadROIStream(_indexMin, _indexMax,
                           0, tmpReadStream, _readElements, _buffer,subSamplingFactor) )
    {
    tmpReadStream->close();
    delete tmpReadStream;
    return false;
    }

  tmpReadStream->close();

  delete tmpReadStream;

  return true;
}

/** Read the ROI Stream */
bool MetaImage::ReadROIStream(int * _indexMin, int * _indexMax,
                              int _nDims,
                              METAIO_STREAM::ifstream * _stream,
                              bool _readElements,
                              void * _buffer,
                              unsigned int subSamplingFactor)
{
  if(!MetaObject::ReadStream(_nDims, _stream))
    {
    METAIO_STREAM::cerr << "MetaImage: Read: Cannot parse file"
                        << METAIO_STREAM::endl;
    return false;
    }

  if(_readElements)
    {
    if(_buffer == NULL)
      {
      InitializeEssential(m_NDims,
                          m_DimSize,
                          m_ElementSpacing,
                          m_ElementType,
                          m_ElementNumberOfChannels,
                          NULL, true);
      }
    else
      {
      InitializeEssential(m_NDims,
                          m_DimSize,
                          m_ElementSpacing,
                          m_ElementType,
                          m_ElementNumberOfChannels,
                          _buffer, false);
      }

    // Streaming related. We need to update some of the fields
    METAIO_STL::streamoff quantity = 1;
    int i;
    size_t j;
    for(i=0; i<m_NDims; i++)
      {
      quantity *= (_indexMax[i] - _indexMin[i] + 1);
      }

    bool usePath;
    char pathName[MAXPATHLENGHT];
    char fName[MAXPATHLENGHT];
    usePath = MET_GetFilePath(m_FileName, pathName);

    if(!strcmp("Local", m_ElementDataFileName) ||
       !strcmp("LOCAL", m_ElementDataFileName) ||
       !strcmp("local", m_ElementDataFileName))
      {
      M_ReadElementsROI(_stream, m_ElementData, quantity,
                        _indexMin, _indexMax, subSamplingFactor,
                        m_Quantity);
      }
    else if(!strncmp("LIST", m_ElementDataFileName,4))
      {
      int fileImageDim = m_NDims - 1;
      int nWrds;
      char **wrds;
      MET_StringToWordArray(m_ElementDataFileName, &nWrds, &wrds);
      if(nWrds > 1)
        {
        fileImageDim = (int)atof(wrds[1]);
        }
      for(i=0; i<nWrds; i++)
        {
        delete [] wrds[i];
        }
      delete [] wrds;
      if ( (fileImageDim == 0) || (fileImageDim > m_NDims) )
        {
        // if optional file dimension size is not given or is larger than
        // overall dimension then default to a size of m_NDims - 1.
        fileImageDim = m_NDims-1;
        }
      char s[1024];
      METAIO_STREAM::ifstream* readStreamTemp = new METAIO_STREAM::ifstream;
      int elementSize;
      MET_SizeOfType(m_ElementType, &elementSize);
      elementSize *= m_ElementNumberOfChannels;

      int minV = _indexMin[m_NDims-1];
      int maxV = minV + (_indexMax[m_NDims-1]-_indexMin[m_NDims-1]);

      int cnt=0;

      // Read the previous lines
      for(i=0;i<minV;i++)
        {
        _stream->getline(s, 1024);
        }

      for(i=minV; i<=maxV; i+=1)
        {
        _stream->getline(s, 1024);
        if(!_stream->eof())
          {
          j = strlen(s)-1;
          while(j>0 && (isspace(s[j]) || !isprint(s[j])))
            {
            s[j--] = '\0';
            }
          if(usePath && !FileIsFullPath(s))
            {
            sprintf(fName, "%s%s", pathName, s);
            }
          else
            {
            strcpy(fName, s);
            }

          openReadStream(*readStreamTemp, fName);
          if(!readStreamTemp->is_open())
            {
            METAIO_STREAM::cerr << "MetaImage: Read: cannot open slice"
                                << METAIO_STREAM::endl;
            continue;
            }

          // read only one slice
          int * indexMin = new int[m_NDims];
          int * indexMax = new int[m_NDims];
          quantity = 1;
          for(int k = 0;k<m_NDims-1;k++)
            {
            quantity *= _indexMax[k]-_indexMin[k]+1;
            indexMin[k]= _indexMin[k];
            indexMax[k]= _indexMax[k];
            }
          indexMin[m_NDims-1]=0;
          indexMax[m_NDims-1]=0;

          M_ReadElementsROI(readStreamTemp,
                             &(((char *)m_ElementData)[cnt*quantity*
                                                     elementSize]),
                             quantity, indexMin, indexMax,
                             subSamplingFactor,
                             m_SubQuantity[m_NDims-1]);

          cnt++;
          readStreamTemp->close();
          }
        }
      delete readStreamTemp;
      }
    else if(strstr(m_ElementDataFileName, "%"))
      {
      int elementSize;
      MET_SizeOfType(m_ElementType, &elementSize);
      elementSize *= m_ElementNumberOfChannels;

      int nWrds;
      char **wrds;
      int minV = 1;
      int maxV = m_DimSize[m_NDims-1];
      int stepV = 1;
      char s[MAXPATHLENGHT];
      METAIO_STREAM::ifstream* readStreamTemp = new METAIO_STREAM::ifstream;
      MET_StringToWordArray(m_ElementDataFileName, &nWrds, &wrds);
      if(nWrds >= 2)
        {
        minV = (int)atof(wrds[1]);
        maxV = minV + m_DimSize[m_NDims-1] - 1;
        }
      if(nWrds >= 3)
        {
        maxV = (int)atof(wrds[2]);
        stepV = (maxV-minV)/(m_DimSize[m_NDims-1]);
        }
      if(nWrds >= 4)
        {
        stepV = (int)atof(wrds[3]);
        }
      if(nWrds >= 5 )
      {
        // In this case, the filename must have had spaces in the
        // name.  The filename was parsed into multiple pieces by the
        // MET_StringToWordArray, which parses based on spaces.
        // Thus, we need to reconstruct the filename in this case.
        // The last three wrds must be numbers.  If they are not, we give an error.
        for( i = nWrds-3; i < nWrds; i++ )
        {
          for( j = 0; j < strlen(wrds[i]); j++ )
          {
            if( !isdigit(wrds[i][j]) )
            {
              METAIO_STREAM::cerr << "MetaImage: Read: Last three arguments must be numbers!"
                  << METAIO_STREAM::endl;
              continue;
            }
          }
        }
        stepV = (int)atof(wrds[nWrds-1]);
        maxV =  (int)atof(wrds[nWrds-2]);
        minV =  (int)atof(wrds[nWrds-3]);
        for( i = 1; i < nWrds-3; i++ )
        {
          strcat(wrds[0]," ");
          strcat(wrds[0],wrds[i]);
        }
      }
      // If the specified size of the third dimension is less than the size
      // specified by the regular expression, we should only read a volume with the specified
      // size.  Otherwise, the code will crash when trying to fill m_ElementData more than it can hold.
      // Therefore, we modify maxV to ensure that the images spanned by minV:stepV:maxV are less than or equal
      // to the size in the last dimension.
      int numberOfImages = 1 + (maxV - minV)/stepV;
      if( numberOfImages > m_DimSize[m_NDims-1] )
      {
        maxV = (m_DimSize[m_NDims-1]-1)*stepV + minV;
      }

      int cnt = 0;

      // Uses the _indexMin and _indexMax
      minV += _indexMin[m_NDims-1];
      maxV = minV + (_indexMax[m_NDims-1]-_indexMin[m_NDims-1])*stepV;

      for(i=minV; i<=maxV; i += stepV)
        {
        sprintf(s, wrds[0], i);
        if(usePath && !FileIsFullPath(s))
          {
          sprintf(fName, "%s%s", pathName, s);
          }
        else
          {
          strcpy(fName, s);
          }


        openReadStream(*readStreamTemp, fName);
        if(!readStreamTemp->is_open())
          {
          METAIO_STREAM::cerr << "MetaImage: Read: cannot construct file"
                              << METAIO_STREAM::endl;
          continue;
          }

        // read only one slice
        int * indexMin = new int[m_NDims];
        int * indexMax = new int[m_NDims];
        quantity = 1;
        for(int k = 0;k<m_NDims-1;k++)
          {
          quantity *= _indexMax[k]-_indexMin[k]+1;
          indexMin[k]= _indexMin[k];
          indexMax[k]= _indexMax[k];
          }
        indexMin[m_NDims-1]=0;
        indexMax[m_NDims-1]=0;

        M_ReadElementsROI(readStreamTemp,
                       &(((char *)m_ElementData)[cnt*quantity*
                                                 elementSize]),
                       quantity, indexMin, indexMax,
                       subSamplingFactor,
                       m_SubQuantity[m_NDims-1]);

        cnt++;

        delete [] indexMin;
        delete [] indexMax;

        readStreamTemp->close();
        }

      for(i=0; i<nWrds; i++)
        {
        delete [] wrds[i];
        }
      delete [] wrds;

      delete readStreamTemp;
      }
    else
      {
      if(usePath && !FileIsFullPath(m_ElementDataFileName))
        {
        sprintf(fName, "%s%s", pathName, m_ElementDataFileName);
        }
      else
        {
        strcpy(fName, m_ElementDataFileName);
        }

      METAIO_STREAM::ifstream* readStreamTemp = new METAIO_STREAM::ifstream;

      const char *extensions[] = { "", ".gz", ".Z", 0 };
      for(unsigned ii = 0; extensions[ii] != 0; ii++)
        {
        METAIO_STL::string tempFName(fName);
        tempFName += extensions[ii];
        openReadStream(*readStreamTemp,tempFName.c_str());
        if(readStreamTemp->is_open())
          {
          if(ii > 0)
            {
            this->CompressedData(true);
            this->BinaryData(true);
            }
          break;
          }
        }

      if(!readStreamTemp->is_open())
        {
        METAIO_STREAM::cerr << "MetaImage: ReadROI: Cannot open data file"
                            << METAIO_STREAM::endl;
        if(m_ReadStream)
          {
          m_ReadStream->close();
          }
        delete readStreamTemp;
        return false;
        }

      M_ReadElementsROI(readStreamTemp, m_ElementData, quantity,
                        _indexMin, _indexMax, subSamplingFactor,
                        m_Quantity);

      readStreamTemp->close();
      delete readStreamTemp;
      }
    }
  return true;
}

/** Read an ROI */
bool MetaImage::
M_ReadElementsROI(METAIO_STREAM::ifstream * _fstream, void * _data,
                  METAIO_STL::streamoff _dataQuantity,
                  int* _indexMin, int* _indexMax,unsigned int subSamplingFactor,
                  METAIO_STL::streamoff _totalDataQuantity)
{
  if(_totalDataQuantity ==0)
    {
    _totalDataQuantity = _dataQuantity;
    }

  for(int dim=0;dim<m_NDims;dim++)
    {
    _indexMin[dim] *= subSamplingFactor;
    _indexMax[dim] *= subSamplingFactor;
    }


  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaImage: M_ReadElementsROI" << METAIO_STREAM::endl;
    }

  if(m_HeaderSize>(int)0)
    {
    _fstream->seekg(m_HeaderSize, METAIO_STREAM::ios::beg);
    if(!_fstream->good())
      {
      METAIO_STREAM::cerr << "MetaImage: M_ReadElementsROI: header not read correctly"
                          << METAIO_STREAM::endl;
      return false;
      }
    }

  int elementSize;
  MET_SizeOfType(m_ElementType, &elementSize);
  METAIO_STL::streamoff readSize = _dataQuantity*m_ElementNumberOfChannels*elementSize;
  int elementNumberOfBytes = elementSize*m_ElementNumberOfChannels;

  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaImage: M_ReadElementsROI: ReadSize = "
                        << readSize << METAIO_STREAM::endl;
    }

  if(m_HeaderSize == -1)
    {
    if(META_DEBUG)
      {
      METAIO_STREAM::cout << "MetaImage: M_ReadElementsROI: Skipping header"
                          << METAIO_STREAM::endl;
      }
    METAIO_STL::streamoff headSize = _totalDataQuantity*m_ElementNumberOfChannels*elementSize;
    _fstream->seekg(-headSize, METAIO_STREAM::ios::end);
    }

  METAIO_STL::streampos dataPos = _fstream->tellg();
  METAIO_STL::streamoff i;

  // If compressed we inflate
  if(m_BinaryData && m_CompressedData)
    {
    // if m_CompressedDataSize is not defined we assume the size of the
    // file is the size of the compressed data
    if(m_CompressedDataSize==0)
      {
      _fstream->seekg(0, METAIO_STREAM::ios::end);
      m_CompressedDataSize = _fstream->tellg();
      _fstream->seekg(0, METAIO_STREAM::ios::beg);
      }

      unsigned char* data = static_cast<unsigned char*>(_data);
      // Initialize the index
      int* currentIndex = new int[m_NDims];
      for(i=0;i<m_NDims;i++)
        {
        currentIndex[i] = _indexMin[i];
        }

      // Optimize the size of the buffer to read depending on the
      // region shape
      // This calculate the number of continuous bytes in the file
      // which can be read
      METAIO_STL::streamoff elementsToRead = 1;
      int movingDirection = 0;
      do
        {
        elementsToRead *= _indexMax[movingDirection] - _indexMin[movingDirection] + 1;
        ++movingDirection;
        }
      while(subSamplingFactor == 1
            && movingDirection < m_NDims
            && _indexMin[movingDirection-1] == 0
            && _indexMax[movingDirection-1] == m_DimSize[movingDirection-1]-1);

      METAIO_STL::streamoff bytesToRead = elementsToRead*elementNumberOfBytes;
      METAIO_STL::streamoff gc = 0;

      bool done = false;
      while(!done)
        {
        // Seek to the right position
        METAIO_STL::streamoff seekoff = 0;
        for(i=0; i<m_NDims; i++)
          {
          seekoff += m_SubQuantity[i]*elementNumberOfBytes*currentIndex[i];
          }


        if(subSamplingFactor > 1)
          {
          unsigned char* subdata = new unsigned char[static_cast<size_t>(bytesToRead)];
          METAIO_STL::streamoff rOff =
            MET_UncompressStream(_fstream, seekoff, subdata,
                                 bytesToRead, m_CompressedDataSize,
                                 m_CompressionTable);
          // if there was a read error
          if(rOff == -1)
            {
            delete [] currentIndex;
            return false;
            }

          for(METAIO_STL::streamoff p=0;
              p<bytesToRead;
              p+=(subSamplingFactor*m_ElementNumberOfChannels*elementSize))
            {
            for(int s=0; s<m_ElementNumberOfChannels*elementSize; s++)
              {
              *data = subdata[p+s];
              gc++;
              data++;
              }
            }
          delete [] subdata;
          }
        else
          {
          METAIO_STL::streamoff rOff =
            MET_UncompressStream(_fstream, seekoff, data,
                                 bytesToRead, m_CompressedDataSize,
                                 m_CompressionTable);
          if(rOff == -1)
            {
            delete [] currentIndex;
            return false;
            }
          data += bytesToRead;
          gc += rOff;
          }

        if(gc == readSize)
          {
          break;
          }

        // Go forward
        if(m_NDims == 1)
          {
          break;
          }

        currentIndex[movingDirection] += subSamplingFactor;

        // Check if we are still in the region
        for(i=1;i<m_NDims;i++)
          {
          if(currentIndex[i]>_indexMax[i])
            {
            if(i==m_NDims-1)
              {
              done = true;
              break;
              }
            else
              {
              currentIndex[i] = _indexMin[i];
              currentIndex[i+1] += subSamplingFactor;
              }
            }
          }
        }

      if(gc != readSize)
        {
        METAIO_STREAM::cerr
                  << "MetaImage: M_ReadElementsROI: data not read completely"
                  << METAIO_STREAM::endl;
        METAIO_STREAM::cerr << "   ideal = " << readSize << " : actual = " << gc
                  << METAIO_STREAM::endl;
        delete [] currentIndex;
        return false;
        }

      delete [] currentIndex;
    }
  else // if not compressed
    {
    double tf;
    MET_SizeOfType(m_ElementType, &elementSize);

    char* data = static_cast<char*>(_data);
    // Initialize the index
    int* currentIndex = new int[m_NDims];
    for(i=0;i<m_NDims;i++)
      {
      currentIndex[i] = _indexMin[i];
      }

    // Optimize the size of the buffer to read depending on the
    // region shape
    // This calculate the number of continuous bytes in the file
    // which can be read
    METAIO_STL::streamoff elementsToRead = 1;
    int movingDirection = 0;
    do
      {
      elementsToRead *= _indexMax[movingDirection] - _indexMin[movingDirection] + 1;
      ++movingDirection;
      }
    while(subSamplingFactor == 1
          && movingDirection < m_NDims
          && _indexMin[movingDirection-1] == 0
          && _indexMax[movingDirection-1] == m_DimSize[movingDirection-1]-1);

    //readLine *= m_ElementNumberOfChannels*elementSize;
    METAIO_STL::streamoff gc = 0;

    bool done = false;
    while(!done)
      {
      // Seek to the right position
      METAIO_STL::streamoff seekoff = 0;
      for(i=0;i<m_NDims;i++)
        {
        seekoff += m_SubQuantity[i]*m_ElementNumberOfChannels*elementSize*currentIndex[i];
        }

      _fstream->seekg(dataPos+seekoff, METAIO_STREAM::ios::beg);

      // Read a line
      if(subSamplingFactor > 1)
        {
        if(!m_BinaryData) // Not binary data
          {
          for(i=0; i<elementsToRead; i+=subSamplingFactor)
            {
            *_fstream >> tf;
            MET_DoubleToValue(tf, m_ElementType, _data, i);

            for(unsigned int j=0;j<subSamplingFactor;j++)
              {
              _fstream->get();
              }
            }
          }
        else // Binary data
          {
          char* subdata = new char[static_cast<size_t>(elementsToRead*elementNumberOfBytes)];

          _fstream->read(subdata, size_t(elementsToRead*elementNumberOfBytes));

          for(METAIO_STL::streamoff p=0;
              p<elementsToRead*elementNumberOfBytes;
              p+=(subSamplingFactor*elementNumberOfBytes))
            {
            for(int s=0;s<elementNumberOfBytes;s++)
              {
              *data = subdata[p+s];
              gc++;
              data++;
              }
            }
          delete [] subdata;
          }
        }
      else
        {
        if(!m_BinaryData) // Not binary data
          {
          // anyone using ROI reading of ASCII??
          // does this work? what about incrementing data?
          // what about data sizes and random access of file?
          METAIO_STL::streamoff blockSize = elementsToRead*m_ElementNumberOfChannels*elementSize;
          M_ReadElementData(  _fstream, data, (size_t)blockSize  );
          gc += blockSize;

          }
        else // binary data
          {

          M_ReadElementData(  _fstream, data, elementsToRead );
          gc += elementsToRead*elementNumberOfBytes;
          data += elementsToRead*elementNumberOfBytes;
          }
        }

      // I don't think this check is really needed -BCL
      if(gc == readSize)
        {
        break;
        }

      // check if there is only one read needed
      if ( movingDirection >= m_NDims )
        {
        break;
        }

      // Go forward
      currentIndex[movingDirection] += subSamplingFactor;

      // Check if we are still in the region
      for(i=movingDirection;i<m_NDims;i++)
        {
        if(currentIndex[i]>_indexMax[i])
          {
          if(i==m_NDims-1)
            {
            done = true;
            break;
            }
          else
            {
            currentIndex[i] = _indexMin[i];
            currentIndex[i+1] += subSamplingFactor;
            }
          }
        }
      }

    delete [] currentIndex;

    if(gc != readSize)
      {
      METAIO_STREAM::cerr
                << "MetaImage: M_ReadElementsROI: data not read completely"
                << METAIO_STREAM::endl;
      METAIO_STREAM::cerr << "   ideal = " << readSize << " : actual = " << gc
                << METAIO_STREAM::endl;
      return false;
      }
    }

  return true;
}


bool MetaImage::
M_ReadElementData(METAIO_STREAM::ifstream * _fstream,
                  void * _data,
                  METAIO_STL::streamoff _dataQuantity)
{
  // NOTE: this method is different from WriteElementData
  METAIO_STL::streamoff gc = 0;

  if(!m_BinaryData)
    {
    double tf;

    for(int i=0; i<_dataQuantity; i++)
      {
      *_fstream >> tf;
      MET_DoubleToValue(tf, m_ElementType, _data, i);
      _fstream->get();
      ++gc;
      }
    }
  else
    {
    if(m_CompressedData)
      {

      // the data is read with calls no bigger then MaxIOChunk
      METAIO_STL::streamoff bytesRemaining = _dataQuantity;
      while ( bytesRemaining )
        {
        METAIO_STL::streamoff chunkToRead = bytesRemaining > MaxIOChunk ? MaxIOChunk : bytesRemaining;
        _fstream->read( (char *)_data, (size_t)chunkToRead );
        _data = (char *)(_data) + chunkToRead;
        bytesRemaining -= chunkToRead;
        METAIO_STL::streamsize numberOfBytesRead = _fstream->gcount();
        gc += numberOfBytesRead;
        }

      }
    else
      {
      int elementSize;
      MET_SizeOfType(m_ElementType, &elementSize);
      METAIO_STL::streamoff elementNumberOfBytes = elementSize*m_ElementNumberOfChannels;

      // the data is read with calls no bigger than MaxIOChunk
      METAIO_STL::streamoff bytesRemaining = _dataQuantity * elementNumberOfBytes;
      while ( bytesRemaining )
        {
        METAIO_STL::streamoff chunkToRead = bytesRemaining > MaxIOChunk ? MaxIOChunk : bytesRemaining;
        _fstream->read( (char *)_data, (size_t)chunkToRead );
        _data = (char *)(_data) + chunkToRead;
        bytesRemaining -= chunkToRead;
        METAIO_STL::streamsize numberOfBytesRead = _fstream->gcount();
        gc += numberOfBytesRead;
        }
      // convert to number of bytes so that it'll match gc's units
      _dataQuantity *= elementNumberOfBytes;
      }
    }

  // check that we actually read the correct number of bytes
  if( gc != _dataQuantity )
    {
    METAIO_STREAM::cerr
      << "MetaImage: M_ReadElementsData: data not read completely"
      << METAIO_STREAM::endl;
    METAIO_STREAM::cerr << "   ideal = " << _dataQuantity << " : actual = " << gc
                        << METAIO_STREAM::endl;
    return false;
    }

  // check the the io stream did not fail in the process of reading
  if ( _fstream->fail() )
    {
    METAIO_STREAM::cerr
      << "MetaImage: M_ReadElementsData: file stream is fail after read"
      << METAIO_STREAM::endl;
    return false;
    }

  return true;
  }


#if (METAIO_USE_NAMESPACE)
}
#endif
