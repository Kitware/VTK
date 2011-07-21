/*============================================================================
  MetaIO
  Copyright 2000-2010 Insight Software Consortium

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/
/**
 *    MetaUtils (.h and .cpp)
 *
 * Description:
 *    This file provides generic ascii file parsing capabilities.
 *    It assumes that the files consist of a set of fields
 *    Each field is list of variable = value pairs
 *
 * Features:
 *    There can be dependencies between fields, required fields,
 *       and optional fields.
 *    Undefined fields are skipped.
 *    Values must conform to expected types.   There can be default
 *       values for fields.
 *
 * Author:
 *    Stephen R. Aylward
 *
 * Date:
 *    February 22, 2002
 *
 **/
#include "metaTypes.h"

#ifndef ITKMetaIO_METAUTILS_H
#define ITKMetaIO_METAUTILS_H

#ifdef _MSC_VER
#pragma warning( disable:4251 )
#pragma warning( disable:4511 )
#pragma warning( disable:4512 )
#pragma warning( disable:4284 )
#pragma warning( disable:4702 )
#pragma warning( disable:4786 )
#pragma warning( disable:4996 )
#endif

#include <vector>
#include <string>
#include <sstream>
#include <iostream>
//#include <iomanip>
#include <typeinfo>
#include <string.h>

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif

extern int META_DEBUG;

// Types used for storing the compression table
typedef struct MET_CompressionOffset
  {
  METAIO_STL::streamoff uncompressedOffset;
  METAIO_STL::streamoff compressedOffset;
  } MET_CompressionOffsetType;

typedef METAIO_STL::vector<MET_CompressionOffsetType>
                                                 MET_CompressionOffsetListType;

typedef struct MET_CompressionTable
  {
  MET_CompressionOffsetListType offsetList;
  z_stream* compressedStream;
  char*     buffer;
  METAIO_STL::streamoff bufferSize;
  } MET_CompressionTableType;

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
METAIO_EXPORT MET_FieldRecordType *
  MET_GetFieldRecord(const char * _fieldName,
                           METAIO_STL::vector<MET_FieldRecordType *> * _fields);

METAIO_EXPORT
int MET_GetFieldRecordNumber(const char * _fieldName,
                           METAIO_STL::vector<MET_FieldRecordType *> * _fields);

METAIO_EXPORT
bool MET_SizeOfType(MET_ValueEnumType _type, int *_size);

/////////////////////////////////////////////////////////
// Byte Order
/////////////////////////////////////////////////////////
METAIO_EXPORT
bool MET_SystemByteOrderMSB(void);

inline
unsigned short MET_ByteOrderSwapShort(unsigned short x)
    {
    return (unsigned short)((unsigned short)(x<<8) | (unsigned short)(x>>8));
    }

inline
unsigned long MET_ByteOrderSwapLong(unsigned int x)
    {
    return (((x<<24) & 0xff000000) |
            ((x<<8)  & 0x00ff0000) |
            ((x>>8)  & 0x0000ff00) |
            ((x>>24) & 0x000000ff));
    }

inline
void MET_ByteOrderSwap2(void* x)
    {
    char one_byte;
    char *p = reinterpret_cast<char *>(x);

    one_byte    = p[0];
    p[0] = p[1];
    p[1] = one_byte;
    }

inline
void MET_ByteOrderSwap4(void* x)
    {
    char one_byte;
    char *p = reinterpret_cast<char *>(x);

    one_byte    = p[0];
    p[0] = p[3];
    p[3] = one_byte;

    one_byte    = p[1];
    p[1] = p[2];
    p[2] = one_byte;
    }

inline
void MET_ByteOrderSwap8(void* x)
    {
    char one_byte;
    char* p = reinterpret_cast<char*>(x);
    one_byte    = p[0];
    p[0] = p[7];
    p[7] = one_byte;

    one_byte    = p[1];
    p[1] = p[6];
    p[6] = one_byte;

    one_byte    = p[2];
    p[2] = p[5];
    p[5] = one_byte;

    one_byte    = p[3];
    p[3] = p[4];
    p[4] = one_byte;
    }

/** Make sure that all the byte are read and written as LSB */
void MET_SwapByteIfSystemMSB(void* val, MET_ValueEnumType _type);


/////////////////////////////////////////////////////////
// STRINGS AND TYPES
/////////////////////////////////////////////////////////
METAIO_EXPORT
bool MET_StringToWordArray(const char *s, int *n, char ***val);

template <class T>
void MET_StringToVector( const METAIO_STL::string & s,
                         METAIO_STL::vector< T > & vec )
{
  vec.clear();

  METAIO_STL::string::size_type prevPos = 0;
  METAIO_STL::string::size_type pos = s.find(",", prevPos);
  T tVal;
  while( pos != METAIO_STL::string::npos )
    {
    METAIO_STL::stringstream ss;
    METAIO_STL::string tmpString = s.substr( prevPos, (pos-prevPos) );
    ss << tmpString;
    ss >> tVal;
    vec.push_back( tVal );

    prevPos = pos+1;
    pos = s.find(",", prevPos);
    }
  METAIO_STL::stringstream ss;
  METAIO_STL::string tmpString = s.substr( prevPos, (s.size()-prevPos) );
  ss << tmpString;
  ss >> tVal;
  vec.push_back( tVal );
}

METAIO_EXPORT
bool MET_StringToType(const char *_str, MET_ValueEnumType *_type);

METAIO_EXPORT
bool MET_TypeToString(MET_ValueEnumType _type, char *_str);

METAIO_EXPORT
bool MET_StringToInterpolationType(const char * _str,
                                MET_InterpolationEnumType * _type);

METAIO_EXPORT
bool MET_InterpolationTypeToString(MET_InterpolationEnumType _type,
                                char * _str);

inline
MET_ValueEnumType MET_GetPixelType(const METAIO_STL::type_info & ptype)
    {
    if( ptype == typeid(MET_UCHAR_TYPE) )
      {
      return MET_UCHAR;
      }
    else if( ptype == typeid(MET_CHAR_TYPE) )
      {
      return MET_CHAR;
      }
    else if( ptype == typeid(MET_USHORT_TYPE))
      {
      return MET_USHORT;
      }
    else if( ptype == typeid(MET_SHORT_TYPE) )
      {
      return MET_SHORT;
      }
    else if( ptype == typeid(MET_UINT_TYPE) )
      {
      return MET_UINT;
      }
    else if( ptype == typeid(MET_INT_TYPE) )
      {
      return MET_INT;
      }
    else if( ptype == typeid(MET_ULONG_TYPE))
      {
      return MET_ULONG;
      }
    else if( ptype == typeid(MET_LONG_TYPE) )
      {
      return MET_LONG;
      }
    else if( ptype == typeid(MET_ULONG_LONG_TYPE) )
      {
      return MET_ULONG_LONG;
      }
    else if( ptype == typeid(MET_LONG_LONG_TYPE) )
      {
      return MET_LONG_LONG;
      }
    else if( ptype == typeid(MET_FLOAT_TYPE) )
      {
      return MET_FLOAT;
      }
    else if( ptype == typeid(MET_DOUBLE_TYPE) )
      {
      return MET_DOUBLE;
      }
    else
      {
      METAIO_STREAM::cerr << "MET_GetPixelType: Couldn't convert pixel type : "
                << ptype.name() << METAIO_STREAM::endl;
      return MET_NONE;
      }
    }

inline
MET_ValueEnumType MET_GetValueEnumType(const METAIO_STL::type_info & ptype)
    {
    return MET_GetPixelType( ptype );
    }

/////////////////////////////////////////////////////////
// VALUES
/////////////////////////////////////////////////////////
METAIO_EXPORT
bool MET_ValueToDouble(MET_ValueEnumType _pType,
                              const void *_data,
                              METAIO_STL::streamoff _index,
                              double *_value);

METAIO_EXPORT
bool MET_DoubleToValue(double _value,
                              MET_ValueEnumType _type,
                              void *_data,
                              METAIO_STL::streamoff _index);

METAIO_EXPORT
bool MET_ValueToValue(MET_ValueEnumType _fromType,
                             const void *_fromData,
                             METAIO_STL::streamoff _index,
                             MET_ValueEnumType _toType,
                             void  *_toData,
                             double _fromMin=0, double _fromMax=0,
                             double _toMin=0, double _toMax=0);

METAIO_EXPORT
unsigned char * MET_PerformCompression(const unsigned char * source,
                                       METAIO_STL::streamoff sourceSize,
                                       METAIO_STL::streamoff * compressedDataSize);

METAIO_EXPORT
bool MET_PerformUncompression(const unsigned char * sourceCompressed,
                              METAIO_STL::streamoff sourceCompressedSize,
                              unsigned char * uncompressedData,
                              METAIO_STL::streamoff uncompressedDataSize);

// Uncompress a stream given an uncompressedSeekPosition
METAIO_EXPORT
METAIO_STL::streamoff MET_UncompressStream(METAIO_STREAM::ifstream * stream,
                          METAIO_STL::streamoff uncompressedSeekPosition,
                          unsigned char * uncompressedData,
                          METAIO_STL::streamoff uncompressedDataSize,
                          METAIO_STL::streamoff compressedDataSize,
                          MET_CompressionTableType * compressionTable);


/////////////////////////////////////////////////////////
// FILES NAMES
/////////////////////////////////////////////////////////
METAIO_EXPORT
bool MET_GetFilePath(const char *_fName, char *_fPath);

METAIO_EXPORT
bool MET_GetFileSuffixPtr(const char *_fName, int *i);

METAIO_EXPORT
bool MET_SetFileSuffix(char *_fName, const char *_suf);

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
METAIO_EXPORT
bool MET_InitWriteField(MET_FieldRecordType * _mf,
                                   const char *_name,
                                   MET_ValueEnumType _type,
                                   double _v=0);

template <class T>
bool MET_InitWriteField(MET_FieldRecordType * _mf,
                                   const char *_name,
                                   MET_ValueEnumType _type,
                                   size_t _length,
                                   T *_v)
  {
  strncpy(_mf->name, _name,254);
  _mf->name[254] = '\0';
  _mf->type = _type;
  _mf->defined = true;
  _mf->length = static_cast<int>(_length);
  _mf->dependsOn = -1;
  _mf->required = false;
  _mf->terminateRead = false;
  if(_type == MET_FLOAT_MATRIX)
    {
    size_t i;
    for(i=0; i < 255 && i < _length*_length; i++)
      {
      _mf->value[i] = (double)(_v[i]);
      }
    }
  else if(_type != MET_STRING)
    {
    size_t i;
    for(i=0; i < 255 && i < _length; i++)
      {
      _mf->value[i] = (double)(_v[i]);
      }
    }
  else
    {
    strncpy((char *)(_mf->value), (const char *)_v,
            (sizeof(_mf->value)-1));
    ((char *)(_mf->value))[(sizeof(_mf->value)-1)] = '\0';
    }
  return true;
  }

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
METAIO_EXPORT
bool MET_Write(METAIO_STREAM::ostream &fp,
            METAIO_STL::vector<MET_FieldRecordType *> * fields,
            char _sepChar='=');

METAIO_EXPORT
bool MET_WriteFieldToFile(METAIO_STREAM::ostream &_fp, const char *_fieldName,
                       MET_ValueEnumType _pType, size_t _n, const void *_v);

METAIO_EXPORT
bool MET_WriteFieldToFile(METAIO_STREAM::ostream &_fp, const char *_fieldName,
                       MET_ValueEnumType _pType, double _v);


/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
METAIO_EXPORT
bool MET_InitReadField(MET_FieldRecordType * _mf,
                                  const char *_name,
                                  MET_ValueEnumType _type,
                                  bool _required=true,
                                  int _dependsOn=-1,
                                  size_t _length=0);

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
METAIO_EXPORT
bool MET_Read(METAIO_STREAM::istream &fp,
           METAIO_STL::vector<MET_FieldRecordType *> * fields,
           char _sepChar='=', bool oneLine=false,
           bool display_warnings=true,
           METAIO_STL::vector<MET_FieldRecordType *> * newFields=NULL);


/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
METAIO_EXPORT
METAIO_STL::string MET_ReadForm(METAIO_STREAM::istream & _fp);

METAIO_EXPORT
METAIO_STL::string MET_ReadType(METAIO_STREAM::istream & _fp);

METAIO_EXPORT
char * MET_ReadSubType(METAIO_STREAM::istream & _fp);

#if (METAIO_USE_NAMESPACE)
};
#endif


#endif
