/*=========================================================================

  Program:   MetaIO
  Module:    metaUtils.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#include "metaUtils.h"

#include <stdio.h>
#include <ctype.h>

#include <sys/stat.h>
#include <fcntl.h>

#ifndef _WIN32
#include <unistd.h>
#include <arpa/inet.h>
#endif

#include <stdlib.h>
#include <string>

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif

int META_DEBUG = 0;

char MET_SeperatorChar = '=';

MET_FieldRecordType * 
MET_GetFieldRecord(const char * _fieldName,
                   METAIO_STL::vector<MET_FieldRecordType *> * _fields)
  {
  METAIO_STL::vector<MET_FieldRecordType *>::iterator fieldIter;
  for(fieldIter=_fields->begin(); fieldIter!=_fields->end(); fieldIter++)
    {
    if(!strcmp((*fieldIter)->name, _fieldName))
      {
      return *fieldIter;
      }
    }
  return NULL;
  }


int
MET_GetFieldRecordNumber(const char * _fieldName,
                         METAIO_STL::vector<MET_FieldRecordType *> * _fields)
  {
  int i;
  for(i=0; i<(int)_fields->size(); i++)
    {
    if(!strcmp((*_fields)[i]->name, _fieldName))
      {
      return i;
      }
    }
  return -1;
  }


//
// Sizeof METTYPE
//
bool MET_SizeOfType(MET_ValueEnumType _vType, int *s)
  {
  *s = MET_ValueTypeSize[_vType];
  if(_vType < MET_STRING)
    {
    return true;
    }
  else
    {
    return false;
    }
  }


//
//
//
bool MET_SystemByteOrderMSB(void)
  {
  const int l = 1;
  const char * u = (const char *) & l;

  if (u[0])
    {
    return false;
    }
   else
    {
    return true;
    }
  }


//
// Read the type of the object
//
METAIO_STL::string MET_ReadForm(METAIO_STREAM::istream &_fp)
  {
  unsigned int pos = _fp.tellg();
  METAIO_STL::vector<MET_FieldRecordType *> fields;
  MET_FieldRecordType* mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "Form", MET_STRING, false);
  mF->required = false;
  mF->terminateRead = true;
  fields.push_back(mF);

  MET_Read(_fp, &fields, '=', true);
  _fp.seekg(pos);

  METAIO_STL::string value;

  if(mF && mF->defined)
    {
    value = (char *)(mF->value);
    delete mF;
    return value;
    }
    
  value[0] = '\0';
  delete mF;
  return value;
  }

//
// Read the type of the object
//
METAIO_STL::string MET_ReadType(METAIO_STREAM::istream &_fp)
  {
  unsigned int pos = _fp.tellg();
  METAIO_STL::vector<MET_FieldRecordType *> fields;
  MET_FieldRecordType* mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "ObjectType", MET_STRING, false);
  mF->required = false;
  mF->terminateRead = true;
  fields.push_back(mF);

  MET_Read(_fp, &fields, '=', true);
  _fp.seekg(pos);

  METAIO_STL::string value;

  if(mF && mF->defined)
    {
    value = (char *)(mF->value);
    delete mF;
    return value;
    }
    
  value[0] = '\0';
  delete mF;
  return value;
  }

//
// Read the subtype of the object
//
char* MET_ReadSubType(METAIO_STREAM::istream &_fp)
  {
  unsigned int pos = _fp.tellg();
  METAIO_STL::vector<MET_FieldRecordType *> fields;
  MET_FieldRecordType* mF;  
  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "ObjectType", MET_STRING, false);
  mF->required = false;
  fields.push_back(mF);

  MET_Read(_fp, &fields, '=', true);

  // Find the line right after the ObjectType
  char s[1024];
  _fp.getline( s, 500 );
  METAIO_STL::string value = s;
  int position = value.find("=");
  if(position!=-1)
    {
    value = value.substr(position+2,value.size()-position);
    }
  _fp.seekg(pos);

  char* ret = new char[value.size()+1];
  strncpy(ret,value.c_str(),value.size());
  ret[value.size()] = '\0';
  delete mF;
  return ret;
  }


//
// String To Type
//
bool MET_StringToType(const char *_s, MET_ValueEnumType *_vType)
  {
  int i;
  for(i=0; i<MET_NUM_VALUE_TYPES; i++)
    {
    if(!strcmp(_s, MET_ValueTypeName[i]))
      {
      *_vType = (MET_ValueEnumType)i;
      return true;
      }
    }
    
  *_vType = MET_OTHER;  
  return false;
  }

//
// METType To String
//
bool MET_TypeToString(MET_ValueEnumType _vType, char *_s)
  {
  if(_vType>=0 && _vType<=MET_NUM_VALUE_TYPES)
    {
    sprintf(_s, MET_ValueTypeName[_vType]);
    return true;
    }
  
  return false;
  }



//
// Value to Double
//
bool MET_ValueToDouble(MET_ValueEnumType _type, const void *_data, int _index,
                       double *_value)
  {
  switch(_type)
    {
    case MET_ASCII_CHAR:
    case MET_CHAR:
    case MET_CHAR_ARRAY:
      *_value = (double)(((const MET_CHAR_TYPE *)_data)[_index]);
      return true;
    case MET_UCHAR:
    case MET_UCHAR_ARRAY:
      *_value = (double)(((const MET_UCHAR_TYPE *)_data)[_index]);
      return true;
    case MET_SHORT:
    case MET_SHORT_ARRAY:
      *_value = (double)(((const MET_SHORT_TYPE *)_data)[_index]);
      return true;
    case MET_USHORT:
    case MET_USHORT_ARRAY:
      *_value = (double)(((const MET_USHORT_TYPE *)_data)[_index]);
      return true;
    case MET_INT:
    case MET_INT_ARRAY:
      *_value = (double)(((const MET_INT_TYPE *)_data)[_index]);
      return true;
    case MET_LONG:
    case MET_LONG_ARRAY:
      *_value = (double)(((const MET_LONG_TYPE *)_data)[_index]);
      return true;
    case MET_UINT:
    case MET_UINT_ARRAY:
      *_value = (double)(((const MET_UINT_TYPE *)_data)[_index]);
      return true;
    case MET_ULONG:
    case MET_ULONG_ARRAY:
      *_value = (double)(((const MET_ULONG_TYPE *)_data)[_index]);
      return true;
    case MET_LONG_LONG:
    case MET_LONG_LONG_ARRAY:
      *_value = (double)(((const MET_LONG_LONG_TYPE *)_data)[_index]);
      return true;
    case MET_ULONG_LONG:
    case MET_ULONG_LONG_ARRAY:
#if defined(_MSC_VER) // NOTE: you cannot use __int64 in an ostream in MSV6
      *_value = (double)((MET_LONG_LONG_TYPE)((((const MET_ULONG_LONG_TYPE *)_data)[_index])));
#else
      *_value = (double)((((const MET_ULONG_LONG_TYPE *)_data)[_index]));
#endif
      return true;
    case MET_FLOAT:
    case MET_FLOAT_ARRAY:
    case MET_FLOAT_MATRIX:
      *_value = (double)(((const MET_FLOAT_TYPE *)_data)[_index]);
      return true;
    case MET_DOUBLE:
    case MET_DOUBLE_ARRAY:
      *_value = (double)(((const MET_DOUBLE_TYPE *)_data)[_index]);
      return true;
    case MET_STRING:
      *_value = atof(&(((const MET_CHAR_TYPE *)_data)[_index]));
      return true;
    default:
      *_value = 0;
      return false;
    }
  }

bool MET_DoubleToValue(double _value, MET_ValueEnumType _type, void *_data, int _index)
  {
  switch(_type)
    {
    case MET_ASCII_CHAR:
    case MET_CHAR:
    case MET_CHAR_ARRAY:
      ((MET_CHAR_TYPE *)_data)[_index] = (MET_CHAR_TYPE)_value;
      return true;
    case MET_UCHAR:
    case MET_UCHAR_ARRAY:
      ((MET_UCHAR_TYPE *)_data)[_index] = (MET_UCHAR_TYPE)_value;
      return true;
    case MET_SHORT:
    case MET_SHORT_ARRAY:
      ((MET_SHORT_TYPE *)_data)[_index] = (MET_SHORT_TYPE)_value;
      return true;
    case MET_USHORT:
    case MET_USHORT_ARRAY:
      ((MET_USHORT_TYPE *)_data)[_index] = (MET_USHORT_TYPE)_value;
      return true;
    case MET_INT:
    case MET_INT_ARRAY:
      ((MET_INT_TYPE *)_data)[_index] = (MET_INT_TYPE)_value;
      return true;
    case MET_LONG:
    case MET_LONG_ARRAY:
      ((MET_LONG_TYPE *)_data)[_index] = (MET_LONG_TYPE)_value;
      return true;
    case MET_UINT:
    case MET_UINT_ARRAY:
      ((MET_UINT_TYPE *)_data)[_index] = (MET_UINT_TYPE)_value;
      return true;
    case MET_ULONG:
    case MET_ULONG_ARRAY:
      ((MET_ULONG_TYPE *)_data)[_index] = (MET_ULONG_TYPE)_value;
      return true;
    case MET_LONG_LONG:
    case MET_LONG_LONG_ARRAY:
      ((MET_LONG_LONG_TYPE *)_data)[_index] = (MET_LONG_LONG_TYPE)_value;
      return true;
    case MET_ULONG_LONG:
    case MET_ULONG_LONG_ARRAY:
      ((MET_ULONG_LONG_TYPE *)_data)[_index] = (MET_ULONG_LONG_TYPE)_value;
      return true;
    case MET_FLOAT:
    case MET_FLOAT_ARRAY:
    case MET_FLOAT_MATRIX:
      ((MET_FLOAT_TYPE *)_data)[_index] = (MET_FLOAT_TYPE)_value;
      return true;
    case MET_DOUBLE:
    case MET_DOUBLE_ARRAY:
      ((MET_DOUBLE_TYPE *)_data)[_index] = (MET_DOUBLE_TYPE)_value;
      return true;
    case MET_STRING:
      sprintf(&(((MET_CHAR_TYPE *)_data)[_index]), "%f", _value);
      return true;
    default:
      return false;
    }
  }

bool MET_ValueToValue(MET_ValueEnumType _fromType, const void *_fromData,
                      int _index,
                      MET_ValueEnumType _toType, void *_toData,
                      double _fromMin, double _fromMax,
                      double _toMin, double _toMax)
  {
  double tf;
  MET_ValueToDouble(_fromType, _fromData, _index, &tf);
  if(_toMin != _toMax && _fromMin != _fromMax)
    {
    tf = (tf-_fromMin)/(_fromMax-_fromMin) * (_toMax-_toMin) + _toMin;
    if(tf<_toMin)
      {
      tf = _toMin;
      }
    else if(tf>_toMax)
      {
      tf = _toMax;
      }
    }
  switch(_toType)
    {
    case MET_ASCII_CHAR:
    case MET_CHAR:
    case MET_CHAR_ARRAY:
      (((MET_CHAR_TYPE *)_toData)[_index]) = (MET_CHAR_TYPE)tf;
      return true;
    case MET_UCHAR:
    case MET_UCHAR_ARRAY:
      (((MET_UCHAR_TYPE *)_toData)[_index]) = (MET_UCHAR_TYPE)tf;
      return true;
    case MET_SHORT:
    case MET_SHORT_ARRAY:
      (((MET_SHORT_TYPE *)_toData)[_index]) = (MET_SHORT_TYPE)tf;
      return true;
    case MET_USHORT:
    case MET_USHORT_ARRAY:
      (((MET_USHORT_TYPE *)_toData)[_index]) = (MET_USHORT_TYPE)tf;
      return true;
    case MET_INT:
    case MET_INT_ARRAY:
      (((MET_INT_TYPE *)_toData)[_index]) = (MET_INT_TYPE)tf;
      return true;
    case MET_LONG:
    case MET_LONG_ARRAY:
      (((MET_LONG_TYPE *)_toData)[_index]) = (MET_LONG_TYPE)tf;
      return true;
    case MET_UINT:
    case MET_UINT_ARRAY:
      (((MET_UINT_TYPE *)_toData)[_index]) = (MET_UINT_TYPE)tf;
      return true;
    case MET_ULONG:
    case MET_ULONG_ARRAY:
      (((MET_ULONG_TYPE *)_toData)[_index]) = (MET_ULONG_TYPE)tf;
      return true;
    case MET_LONG_LONG:
    case MET_LONG_LONG_ARRAY:
      (((MET_LONG_LONG_TYPE *)_toData)[_index]) = (MET_LONG_LONG_TYPE)tf;
      return true;
    case MET_ULONG_LONG:
    case MET_ULONG_LONG_ARRAY:
      (((MET_ULONG_LONG_TYPE *)_toData)[_index]) = (MET_ULONG_LONG_TYPE)tf;
      return true;
    case MET_DOUBLE:
    case MET_DOUBLE_ARRAY:
      (((MET_DOUBLE_TYPE *)_toData)[_index]) = (MET_DOUBLE_TYPE)tf;
      return true;
    case MET_FLOAT:
    case MET_FLOAT_ARRAY:
    case MET_FLOAT_MATRIX:
      (((MET_FLOAT_TYPE *)_toData)[_index]) = (MET_FLOAT_TYPE)tf;
      return true;
    case MET_STRING:
      sprintf(&(((MET_CHAR_TYPE *)_toData)[_index]), "%f", tf);
      return true;
    default:
      return false;
    }
  }

//
//
//
unsigned char * MET_PerformCompression(const unsigned char * source,
                                       int sourceSize,
                                       unsigned int * compressedDataSize)
  {
  unsigned char * compressedData;

  z_stream  z;
  z.zalloc  = (alloc_func)0;
  z.zfree   = (free_func)0;
  z.opaque  = (voidpf)0;

  // Compression rate
  // Choices are Z_BEST_SPEED,Z_BEST_COMPRESSION,Z_DEFAULT_COMPRESSION
  int compression_rate = Z_DEFAULT_COMPRESSION;

  int             buffer_size     = sourceSize;
  unsigned char * input_buffer    = const_cast<unsigned char *>(source);
  unsigned char * output_buffer   = new unsigned char[buffer_size];

  compressedData                  = new unsigned char[buffer_size];

  deflateInit(&z, compression_rate);

  z.avail_in   = buffer_size;
  z.next_in    = input_buffer;
  z.next_out   = output_buffer;
  z.avail_out  = buffer_size;
   
  int count;
  unsigned long j=0;
  // Perform the compression 
  for ( ; ; ) 
    {
    if ( z.avail_in == 0 ) 
      {
      deflate( &z, Z_FINISH );
      count = buffer_size - z.avail_out;
      if ( count ) 
        {
        memcpy((char*)compressedData+j, (char *)output_buffer, count);
        }
      break;
      }

    deflate( &z, Z_NO_FLUSH );
    count = buffer_size - z.avail_out;
    if ( count ) 
      {
      memcpy((char*)compressedData+j, (char*)output_buffer, count);
      }

    j += count;
    z.next_out = output_buffer;
    z.avail_out = buffer_size;
    }
    
  delete output_buffer;

  *compressedDataSize = z.total_out;

  // Print the result
  deflateEnd(&z);

  return compressedData;
  }

//
//
//
bool MET_PerformUncompression(const unsigned char * sourceCompressed,
                              int sourceCompressedSize,
                              unsigned char * uncompressedData,
                              int uncompressedDataSize)
  {
  z_stream d_stream;
 
  d_stream.zalloc = (alloc_func)0;
  d_stream.zfree = (free_func)0;
  d_stream.opaque = (voidpf)0;
  
  inflateInit(&d_stream);
  d_stream.next_in  = const_cast<unsigned char *>(sourceCompressed);
  d_stream.avail_in = sourceCompressedSize;
 
  for (;;) 
    {
    d_stream.next_out = (unsigned char *)uncompressedData; 
    d_stream.avail_out = uncompressedDataSize;
    int err = inflate(&d_stream, Z_NO_FLUSH);
    if((err == Z_STREAM_END))
      {
      break;
      }
    }

  inflateEnd(&d_stream);

  return true;
  }

//
//
//
bool MET_StringToWordArray(const char *s, int *n, char ***val)
{
  long l = static_cast<long>( strlen(s) );

  int p = 0;
  while(p<l && s[p] == ' ')
    {
    p++;
    }
  
  *n = 0;
  int pp = p;
  bool space = false;
  while(pp<l)
    {
    if(s[pp] == ' ' && !space)
      {
      (*n)++;
      space = true;
      }
    else
      {
      space = false;
      }
    pp++;
    }
  pp=l-1;
  if(s[pp] == ' ')   
    {
    while(pp>=0 && s[pp] == ' ')
      {
      (*n)--;
      pp--;
      }
    }
  else
    {
    (*n)++;
    }
  
  *val = new char *[*n];
  
  long i, j;
  for(i=0; i<*n; i++) 
    {
    if(p == l)
      {
      return false;
      }
    
    (*val)[i] = new char [80];
    while(p<l && s[p] == ' ')
      {
      p++;
      }
    j = 0;
    while(p<l && s[p] != ' ')
      {
      (*val)[i][j++] = s[p++];
      }
    (*val)[i][j] = '\0';
    }
  
  return true;
}

//
//
//
bool MET_GetFilePath(const char *_fName, char *_fPath)
  {
  long i;
  
  long l = static_cast<long>( strlen(_fName) );
  
  for(i=l-1; i>=0; i--)
    if(_fName[i] == '\\' || _fName[i] == '/')
      break;
    
    if(i >= 0 && (_fName[i] == '/' || _fName[i] == '\\'))
      {
      strcpy(_fPath, _fName);
      _fPath[i+1] = '\0';
      return true;
      }
    else
      {
      _fPath[0] = '\0';
      return false;
      }
  }

//
//
//
bool MET_GetFileSuffixPtr(const char *_fName, int *i)
  {
  *i = static_cast<int>( strlen(_fName) );
  int j = *i - 5;
  if(j<0)
    {
    j = 0;
    }
  while(*i>j)
    {
    if(_fName[(*i)-1] == '.')
      {
      return true;
      }
    else
      {
      (*i)--;
      }
    }
  *i = 0;
  return false;
  }

//
//
//
bool MET_SetFileSuffix(char *_fName, const char *_suf)
  {
  int i;
  MET_GetFileSuffixPtr(_fName, &i);
  if(i>0)
    {
    if(_suf[0] == '.')
      _fName[i-1] = '\0';
    else
      _fName[i] = '\0';
    strcat(_fName, _suf);
    return true;
    }
  else
    {
    strcat(_fName, _suf);
    return true;
    }
  }

//
//
//
bool MET_InitWriteField(MET_FieldRecordType * _mf, 
                        const char *_name, 
                        MET_ValueEnumType _type, 
                        double _v)
  {
  strcpy(_mf->name, _name);
  _mf->type = _type;
  _mf->defined = true;
  _mf->length = 1;
  _mf->dependsOn = -1;
  _mf->required = false;
  _mf->terminateRead = false;
  _mf->value[0] = _v;
  return true;
  }

bool MET_InitReadField(MET_FieldRecordType * _mf, 
                                  const char *_name, 
                                  MET_ValueEnumType _type, 
                                  bool _required,
                                  int _dependsOn, 
                                  int _length)
  {
  strcpy(_mf->name, _name);
  _mf->type = _type;
  _mf->defined = false;
  _mf->dependsOn = _dependsOn;
  _mf->required = _required;
  _mf->terminateRead = false;
  _mf->length = _length;
  _mf->value[0] = 0;
  return true;
  }

//
//
//
bool MET_SkipToVal(METAIO_STREAM::istream &fp)
  {
  char c;
  if( fp.eof() )
    {
    return false;
    }
  
  c = fp.get();

  while( c != MET_SeperatorChar && c != ':' && !fp.eof() )
    {
    c = fp.get();
    }

  while( ( c == MET_SeperatorChar || c == ':' || isspace(c) ) && !fp.eof() )
    {
    c = fp.get();
    }
  
  if( fp.eof() )
    {
    METAIO_STREAM::cerr << "Incomplete file record definition" << METAIO_STREAM::endl;
    return false;
    }
  
  fp.putback(c);

  return true;
  }

//
//
//
bool MET_IsComplete(METAIO_STL::vector<MET_FieldRecordType *> * fields)
  {
  METAIO_STL::vector<MET_FieldRecordType *>::iterator fieldIter;
  for(fieldIter=fields->begin(); fieldIter!=fields->end(); fieldIter++)
    {
    if((*fieldIter)->required && !(*fieldIter)->defined)
      {
      METAIO_STREAM::cerr << (*fieldIter)->name << " required and not defined." 
                << METAIO_STREAM::endl;
      return false;
      }
    }
  return true;
  }

//
bool MET_Read(METAIO_STREAM::istream &fp, METAIO_STL::vector<MET_FieldRecordType *> * fields,
              char _MET_SeperatorChar, bool oneLine, bool display_warnings)
  {

  char s[1024];
  int i, j;

  METAIO_STL::vector<MET_FieldRecordType *>::iterator fieldIter;
  
  MET_SeperatorChar = _MET_SeperatorChar;
  
  bool found;
  
  unsigned char c;
  while(!fp.eof())
    {
    i = 0;
    c = fp.get();
    while(!fp.eof() && c != MET_SeperatorChar && c != ':'
          && (c == '\n' || isspace(c)))
      {
      c = fp.get();
      }
    while(!fp.eof() && c != MET_SeperatorChar && c != ':' && c != '\n' && i<500)
      {
      s[i++] = c;
      c = fp.get();
      }
    if(fp.eof() || i >= 500)
      {
      break;
      }
    fp.putback(c);
    s[i] = '\0';
    
    i--;
    while((s[i] == ' ' || s[i] == '\t') && i>0)
      {
      s[i--] = '\0';
      }

    found = false;
    for(fieldIter=fields->begin(); fieldIter!=fields->end(); fieldIter++)
      {
      if(!strcmp((*fieldIter)->name, s))
        {
        if((*fieldIter)->dependsOn >= 0)
          if(!(*fields)[(*fieldIter)->dependsOn]->defined)
            {
            METAIO_STREAM::cerr << (*fieldIter)->name << " defined prior to defining ";
            METAIO_STREAM::cerr << (*fields)[(*fieldIter)->dependsOn]->name << METAIO_STREAM::endl;
            return false;
            }
        switch((*fieldIter)->type)
          {
          case MET_NONE:
            fp.getline( s, 500 );
            break;
          case MET_ASCII_CHAR:
            {
            MET_SkipToVal(fp);
            if(fp.eof())
              {
              break;
              }
            MET_CHAR_TYPE c = fp.get();
            c = fp.get();
            (*fieldIter)->value[0] = (double)c;
            fp.getline( s, 500 );
            break;
            }
          default:
          case MET_CHAR:
          case MET_UCHAR:
          case MET_SHORT:
          case MET_USHORT:
          case MET_INT:
          case MET_UINT:
          case MET_LONG:
          case MET_ULONG:
          case MET_LONG_LONG:
          case MET_ULONG_LONG:
          case MET_FLOAT:
          case MET_DOUBLE:
            {
            MET_SkipToVal(fp);
            if(fp.eof())
              {
              break;
              }
            fp >> (*fieldIter)->value[0];
            fp.getline( s, 500 );
            break;
            }
          case MET_STRING:
            {
            MET_SkipToVal(fp);
            if(fp.eof())
              {
              break;
              }
            MET_CHAR_TYPE * str = (MET_CHAR_TYPE *)((*fieldIter)->value);
            fp.getline( str, 500 );
            j = static_cast<long>( strlen(str) ) - 1;
            while(!isprint(str[j]) || isspace(str[j]))
              {
              str[j--] = '\0';
              }
            (*fieldIter)->length = static_cast<int>( strlen( str ) );
            break;
            }
          case MET_CHAR_ARRAY:
          case MET_UCHAR_ARRAY:
          case MET_SHORT_ARRAY:
          case MET_USHORT_ARRAY:
          case MET_INT_ARRAY:
          case MET_UINT_ARRAY:
          case MET_LONG_ARRAY:
          case MET_ULONG_ARRAY:
          case MET_LONG_LONG_ARRAY:
          case MET_ULONG_LONG_ARRAY:
          case MET_FLOAT_ARRAY:
          case MET_DOUBLE_ARRAY:
            {
            MET_SkipToVal(fp);
            if(fp.eof())
              {
              break;
              }
            if((*fieldIter)->dependsOn >= 0)
              {
              (*fieldIter)->length =
                    (int)((*fields)[(*fieldIter)->dependsOn]->value[0]);
              for(j=0; j<(*fieldIter)->length; j++) 
                {
                fp >> (*fieldIter)->value[j];
                }
              }
            else
              {
              if((*fieldIter)->length <= 0)
                {
                METAIO_STREAM::cerr << 
                  "Arrays must have dependency or pre-specified lengths"
                  << METAIO_STREAM::endl;
                return false;
                }
              for(j=0; j<(*fieldIter)->length; j++)
                {
                fp >> (*fieldIter)->value[j];
                }
              }
            fp.getline( s, 500 );
            break;
            }
          case MET_FLOAT_MATRIX:
            {
            MET_SkipToVal(fp);
            if(fp.eof())
              {
              break;
              }
            if((*fieldIter)->dependsOn >= 0)
              {
              (*fieldIter)->length =
                    (int)((*fields)[(*fieldIter)->dependsOn]->value[0]);
              for(j=0; j<(*fieldIter)->length*(*fieldIter)->length;
                  j++) 
                {
                fp >> (*fieldIter)->value[j];
                }
              }
            else
              {
              if((*fieldIter)->length <= 0)
                {
                METAIO_STREAM::cerr << 
                  "Arrays must have dependency or pre-specified lengths"
                  << METAIO_STREAM::endl;
                return false;
                }
              for(j=0; j<(*fieldIter)->length*(*fieldIter)->length; j++)
                {
                fp >> (*fieldIter)->value[j];
                }
              }
            fp.getline( s, 500 );
            break;
            }
          case MET_OTHER:
            {
            fp.getline( s, 500 );
            break;
            }
          }
        found = true;
        (*fieldIter)->defined = true;
        if((*fieldIter)->terminateRead)
          {
          return MET_IsComplete(fields);
          }
        break;
        }
      }
    if(!found)
      {
      if(display_warnings)
        {
        METAIO_STREAM::cerr << "Skipping unrecognized field " << s << METAIO_STREAM::endl;
        }
      fp.getline( s, 500 );
      }
    if(oneLine)
      {
      return MET_IsComplete(fields);
      }
    }
    
  return MET_IsComplete(fields);
  }
  
//
bool MET_Write(METAIO_STREAM::ostream &fp, METAIO_STL::vector<MET_FieldRecordType *> * fields,
               char _MET_SeperatorChar)
  {
  MET_SeperatorChar = _MET_SeperatorChar;
  
  int j;
  METAIO_STL::vector<MET_FieldRecordType *>::iterator fieldIter;
  for(fieldIter=fields->begin(); fieldIter!=fields->end(); fieldIter++)
    {
    switch((*fieldIter)->type)
      {
      case MET_NONE:
        {
        fp << (*fieldIter)->name << " " << MET_SeperatorChar << " " 
           << METAIO_STREAM::endl;
        break;
        }
      case MET_ASCII_CHAR:
        {
        fp << (*fieldIter)->name << " " << MET_SeperatorChar << " ";
        fp << (MET_CHAR_TYPE)(*fieldIter)->value[0] << METAIO_STREAM::endl;
        break;
        }
      case MET_CHAR:
      case MET_SHORT:
      case MET_LONG:
      case MET_INT:
        {
        fp << (*fieldIter)->name << " " << MET_SeperatorChar << " ";
        fp << (MET_LONG_TYPE)((*fieldIter)->value[0]) << METAIO_STREAM::endl;
        break;
        }
      case MET_LONG_LONG:
        {
#if defined(_MSC_VER) // NOTE: you cannot use __int64 in an ostream in MSV6
        fp << (double)((MET_LONG_LONG_TYPE)((*fieldIter)->value[0])) 
           << METAIO_STREAM::endl;
        METAIO_STREAM::cerr << "Programs compiled using MSV6 cannot write 64 bit ints"
                  << METAIO_STREAM::endl;
        METAIO_STREAM::cerr << "  Writing as double instead.  Loss of precision results."
                  << METAIO_STREAM::endl;
#else
        fp << (MET_LONG_LONG_TYPE)((*fieldIter)->value[0]) << METAIO_STREAM::endl;
#endif
        break;
        }
      case MET_UCHAR:
      case MET_USHORT:
      case MET_UINT:
      case MET_ULONG:
        {
        fp << (*fieldIter)->name << " " << MET_SeperatorChar << " ";
        fp << (MET_ULONG_TYPE)((*fieldIter)->value[0]) << METAIO_STREAM::endl;
        break;
        }
      case MET_ULONG_LONG:
        {
#if defined(_MSC_VER) // NOTE: you cannot use __int64 in an ostream in MSV6
        fp << (double)((MET_LONG_LONG_TYPE)((MET_ULONG_LONG_TYPE)((*fieldIter)->value[0]))) 
           << METAIO_STREAM::endl;
        METAIO_STREAM::cerr << "Programs compiled using MSV6 cannot write 64 bit ints"
                  << METAIO_STREAM::endl;
        METAIO_STREAM::cerr << "  Writing as double instead.  Loss of precision results."
                  << METAIO_STREAM::endl;
#else
        fp << (MET_ULONG_LONG_TYPE)((*fieldIter)->value[0]) << METAIO_STREAM::endl;
#endif
        break;
        }
      case MET_FLOAT:
      case MET_DOUBLE:
        {
        fp << (*fieldIter)->name << " " << MET_SeperatorChar << " ";
        fp << (MET_DOUBLE_TYPE)(*fieldIter)->value[0] << METAIO_STREAM::endl;
        break;
        }
      case MET_STRING:
        {
        fp << (*fieldIter)->name << " " << MET_SeperatorChar << " ";
        if((*fieldIter)->dependsOn >= 0) 
          {
          if((*fieldIter)->length != 
             (*fields)[(*fieldIter)->dependsOn]->value[0])
            {
            METAIO_STREAM::cerr << "Warning:";
            METAIO_STREAM::cerr << "length and dependsOn values not equal in write";
            METAIO_STREAM::cerr << METAIO_STREAM::endl;
            }
          }
        fp.write( (char *)((*fieldIter)->value), (*fieldIter)->length );
        fp << METAIO_STREAM::endl;
        break;
        }
      case MET_CHAR_ARRAY:
      case MET_SHORT_ARRAY:
      case MET_INT_ARRAY:
      case MET_LONG_ARRAY:
        {
        fp << (*fieldIter)->name << " " << MET_SeperatorChar;
        if((*fieldIter)->dependsOn >= 0)
          {
          if((*fieldIter)->length != 
             (*fields)[(*fieldIter)->dependsOn]->value[0])
            {
            METAIO_STREAM::cerr << "Warning: ";
            METAIO_STREAM::cerr << "Length and dependsOn values not equal in write";
            METAIO_STREAM::cerr << METAIO_STREAM::endl;
            }
          }
        for(j=0; j<(*fieldIter)->length; j++)
          {
          fp << " " << (MET_LONG_TYPE)((*fieldIter)->value[j]);
          }
        fp << METAIO_STREAM::endl;
        break;
        }
      case MET_LONG_LONG_ARRAY:
        {
        fp << (*fieldIter)->name << " " << MET_SeperatorChar;
        if((*fieldIter)->dependsOn >= 0)
          {
          if((*fieldIter)->length != 
             (*fields)[(*fieldIter)->dependsOn]->value[0])
            {
            METAIO_STREAM::cerr << "Warning: ";
            METAIO_STREAM::cerr << "Length and dependsOn values not equal in write";
            METAIO_STREAM::cerr << METAIO_STREAM::endl;
            }
          }
        for(j=0; j<(*fieldIter)->length; j++)
          {
#if defined(_MSC_VER) // NOTE: you cannot use __int64 in an ostream in MSV6
          fp << " " << (double)((MET_LONG_LONG_TYPE)((*fieldIter)->value[j]));
          METAIO_STREAM::cerr << "Programs compiled using MSV6 cannot write 64 bit ints"
                    << METAIO_STREAM::endl;
          METAIO_STREAM::cerr << "  Writing as double instead. Loss of precision results."
                    << METAIO_STREAM::endl;
#else
          fp << " " << (MET_LONG_LONG_TYPE)((*fieldIter)->value[j]);
#endif
          }
        fp << METAIO_STREAM::endl;
        break;
        }

      case MET_UCHAR_ARRAY:
      case MET_USHORT_ARRAY:
      case MET_UINT_ARRAY:
      case MET_ULONG_ARRAY:
        {
        fp << (*fieldIter)->name << " " << MET_SeperatorChar;
        if((*fieldIter)->dependsOn >= 0)
          {
          if((*fieldIter)->length != 
             (*fields)[(*fieldIter)->dependsOn]->value[0])
            {
            METAIO_STREAM::cerr << "Warning: ";
            METAIO_STREAM::cerr << "Length and dependsOn values not equal in write";
            METAIO_STREAM::cerr << METAIO_STREAM::endl;
            }
          }
        for(j=0; j<(*fieldIter)->length; j++)
          {
          fp << " " << (MET_ULONG_TYPE)((*fieldIter)->value[j]);
          }
        fp << METAIO_STREAM::endl;
        break;
        }
      case MET_ULONG_LONG_ARRAY:
        {
        fp << (*fieldIter)->name << " " << MET_SeperatorChar;
        if((*fieldIter)->dependsOn >= 0)
          {
          if((*fieldIter)->length != 
             (*fields)[(*fieldIter)->dependsOn]->value[0])
            {
            METAIO_STREAM::cerr << "Warning: ";
            METAIO_STREAM::cerr << "Length and dependsOn values not equal in write";
            METAIO_STREAM::cerr << METAIO_STREAM::endl;
            }
          }
        for(j=0; j<(*fieldIter)->length; j++)
          {
#if defined(_MSC_VER) // NOTE: you cannot use __int64 in an ostream in MSV6
          fp << " " << (double)((MET_LONG_LONG_TYPE)((MET_ULONG_LONG_TYPE)((*fieldIter)->value[j])));
          METAIO_STREAM::cerr << "Programs compiled using MSV6 cannot write 64 bit ints"
                    << METAIO_STREAM::endl;
          METAIO_STREAM::cerr << "  Writing as double instead. Loss of precision results."
                    << METAIO_STREAM::endl;
#else
          fp << " " << (MET_ULONG_LONG_TYPE)((*fieldIter)->value[j]);
#endif
          }
        fp << METAIO_STREAM::endl;
        break;
        }

      case MET_FLOAT_ARRAY:
      case MET_DOUBLE_ARRAY:
        {
        fp << (*fieldIter)->name << " " << MET_SeperatorChar;
        if((*fieldIter)->dependsOn >= 0)
          {
          if((*fieldIter)->length != 
             (*fields)[(*fieldIter)->dependsOn]->value[0])
            {
            METAIO_STREAM::cerr << "Warning: ";
            METAIO_STREAM::cerr << "length and dependsOn values not equal in write";
            METAIO_STREAM::cerr << METAIO_STREAM::endl;
            }
          }
        for(j=0; j<(*fieldIter)->length; j++)
          {
          fp << " " << (double)(*fieldIter)->value[j];
          }
        fp << METAIO_STREAM::endl;
        break;
        }
      case MET_FLOAT_MATRIX:
        {
        fp << (*fieldIter)->name << " " << MET_SeperatorChar;
        if((*fieldIter)->dependsOn >= 0)
          {
          if((*fieldIter)->length != 
             (*fields)[(*fieldIter)->dependsOn]->value[0])
            {
            METAIO_STREAM::cerr << "Warning: ";
            METAIO_STREAM::cerr << "length and dependsOn values not equal in write";
            METAIO_STREAM::cerr << METAIO_STREAM::endl;
            }
          }
        for(j=0; j<(*fieldIter)->length*(*fieldIter)->length; j++)
          {
          fp << " " << (double)(*fieldIter)->value[j];
          }
        fp << METAIO_STREAM::endl;
        break;
        }
      case MET_OTHER:
        {
        break;
        }
      }
    }
  return true;
}

bool MET_WriteFieldToFile(METAIO_STREAM::ostream & _fp, const char *_fieldName,
                          MET_ValueEnumType _pType, int _n, const void *_v)
  {
  int i;
  MET_FieldRecordType f;
  
  sprintf(f.name, "%s", _fieldName);
  f.defined = false;
  f.dependsOn = -1;
  f.length = _n;
  f.required = false;
  f.type = _pType;
  switch(_pType)
    {
    case MET_ASCII_CHAR:
    case MET_CHAR:
    case MET_CHAR_ARRAY:
      for(i = 0; i < _n; i++)
        {
        f.value[i] = (double)(((const MET_CHAR_TYPE *)_v)[i]);
        }
      break;
    case MET_UCHAR:
    case MET_UCHAR_ARRAY:
      for(i = 0; i < _n; i++)
        {
        f.value[i] = (double)(((const MET_UCHAR_TYPE *)_v)[i]);
        }
      break;
    case MET_SHORT:
    case MET_SHORT_ARRAY:
      for(i=0; i<_n; i++)
        {
        f.value[i] = (double)(((const MET_SHORT_TYPE *)_v)[i]);
        }
      break;
    case MET_USHORT:
    case MET_USHORT_ARRAY:
      for(i=0; i<_n; i++)
        {
        f.value[i] = (double)(((const MET_USHORT_TYPE *)_v)[i]);
        }
      break;
    case MET_INT:
    case MET_INT_ARRAY:
      for(i=0; i<_n; i++)
        {
        f.value[i] = (double)(((const MET_INT_TYPE *)_v)[i]);
        }
      break;
    case MET_UINT:
    case MET_UINT_ARRAY:
      for(i=0; i<_n; i++)
        {
        f.value[i] = (double)(((const MET_UINT_TYPE *)_v)[i]);
        }
      break;
    case MET_LONG:
    case MET_LONG_ARRAY:
      for(i=0; i<_n; i++)
        {
        f.value[i] = (double)(((const MET_LONG_TYPE *)_v)[i]);
        }
      break;
    case MET_ULONG:
    case MET_ULONG_ARRAY:
      for(i=0; i<_n; i++)
        {
        f.value[i] = (double)(((const MET_ULONG_TYPE *)_v)[i]);
        }
      break;
    case MET_LONG_LONG:
    case MET_LONG_LONG_ARRAY:
      for(i=0; i<_n; i++)
        {
        f.value[i] = (double)(((const MET_LONG_LONG_TYPE *)_v)[i]);
        }
      break;
    case MET_ULONG_LONG:
    case MET_ULONG_LONG_ARRAY:
      for(i=0; i<_n; i++)
        {
#if defined(_MSC_VER) // NOTE: you cannot use __int64 in an ostream in MSV6
        f.value[i] = (double)((MET_LONG_LONG_TYPE)(((const MET_ULONG_LONG_TYPE *)_v)[i]));
#else
        f.value[i] = (double)(((const MET_ULONG_LONG_TYPE *)_v)[i]);
#endif
        }
      break;
    case MET_FLOAT:
    case MET_FLOAT_ARRAY:
      for(i=0; i<_n; i++)
        {
        f.value[i] = (double)((const MET_FLOAT_TYPE *)_v)[i];
        }
      break;
    case MET_DOUBLE:
    case MET_DOUBLE_ARRAY:
      for(i=0; i<_n; i++)
        {
        f.value[i] = (double)(((const MET_DOUBLE_TYPE *)_v)[i]);
        }
      break;
    case MET_STRING:
      strcpy((MET_CHAR_TYPE *)(f.value), (const MET_CHAR_TYPE *)_v);
      break;
    case MET_FLOAT_MATRIX:
      for(i=0; i<_n*_n; i++)
        {
        f.value[i] = (double)((const MET_FLOAT_TYPE *)_v)[i];
        }
      break;
    default:
      break;
    }
  
  METAIO_STL::vector<MET_FieldRecordType *> l;
  l.clear();
  l.push_back(&f);
  MET_Write(_fp, &l);
  
  return true;
  }

bool MET_WriteFieldToFile(METAIO_STREAM::ostream & _fp, const char *_fieldName,
  MET_ValueEnumType _pType, double _v)
  {
  MET_FieldRecordType f;
  
  sprintf(f.name, "%s", _fieldName);
  f.defined = false;
  f.dependsOn = -1;
  f.length = 1;
  f.required = false;
  f.type = _pType;
  f.value[0] = _v;
  
  METAIO_STL::vector<MET_FieldRecordType *> l;
  l.clear();
  l.push_back(&f);
  MET_Write(_fp, &l);
  
  return true;
  }

bool MET_StringToInterpolationType(const char * _str,
                               MET_InterpolationEnumType * _type)
  {
  int i;

  for(i=0; i<MET_NUM_INTERPOLATION_TYPES; i++)
    if(!strcmp(MET_InterpolationTypeName[i], _str))
      {
      *_type = (MET_InterpolationEnumType)i;
      return true;
      }

  *_type = MET_NO_INTERPOLATION;

  return false;
  }

bool MET_InterpolationTypeToString(MET_InterpolationEnumType _type,
                               char * _str)
  {
  strcpy(_str, MET_InterpolationTypeName[(int)_type]);
  return true;
  }

/** Make sure that all the byte are read and written as LSB */
void MET_SwapByteIfSystemMSB(void* val, MET_ValueEnumType _type)
    {
    if(!MET_SystemByteOrderMSB())
      {
      return;
      }
  
    int eSize;
    MET_SizeOfType(_type, &eSize);    
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
        MET_ByteOrderSwap2(val);
        break;
        }
      case 4:
        {
        MET_ByteOrderSwap4(val);
        break;
        }
      case 8:
        {
        MET_ByteOrderSwap8(val);
        break;
        }
      }
    }

#if (METAIO_USE_NAMESPACE)
};
#endif

