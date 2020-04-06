/*============================================================================
  MetaIO
  Copyright 2000-2010 Insight Software Consortium

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/
#ifdef _MSC_VER
#pragma warning(disable:4702)
#pragma warning(disable:4996)
namespace {
inline bool IsBlank(int c)
{
  return c == '\t' || c == ' ';
}
}
#else
//# ifdef isblank
#  define IsBlank(c) isblank((c))
//# else
//#  define IsBlank(x) (((x)==32) || ((x)==9))
//# endif
#endif

#include "metaUtils.h"

#include <cassert>
#include <cctype>
#include <cstddef>
#include <cstdio>

#include <sys/stat.h>
#include <fcntl.h>

#if defined(_WIN32) && !defined(__CYGWIN__)
#define NOMINMAX
#include <winsock2.h>
#else
#include <unistd.h>
#include <arpa/inet.h>
#endif

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>

#if defined (__BORLANDC__) && (__BORLANDC__ >= 0x0580)
#include <mem.h>
#endif

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif

int META_DEBUG = 0;

static char MET_SeperatorChar = '=';

static const std::streamoff MET_MaxChunkSize = 1024*1024*1024;

MET_FieldRecordType *
MET_GetFieldRecord(const char * _fieldName,
                   std::vector<MET_FieldRecordType *> * _fields)
{
  std::vector<MET_FieldRecordType *>::iterator fieldIter;
  for(fieldIter=_fields->begin(); fieldIter!=_fields->end(); ++fieldIter)
    {
    if(!strcmp((*fieldIter)->name, _fieldName))
      {
      return *fieldIter;
      }
    }
  return nullptr;
}


int
MET_GetFieldRecordNumber(const char * _fieldName,
                         std::vector<MET_FieldRecordType *> * _fields)
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


bool MET_SystemByteOrderMSB()
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
std::string MET_ReadForm(std::istream &_fp)
{
  std::streampos pos = _fp.tellg();
  std::vector<MET_FieldRecordType *> fields;
  MET_FieldRecordType* mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "FormTypeName", MET_STRING, false);
  mF->required = false;
  mF->terminateRead = true;
  fields.push_back(mF);

  MET_Read(_fp, &fields, '=', true);
  _fp.seekg(pos);

  if(mF->defined)
    {
    std::string value = (char *)(mF->value);
    delete mF;
    return value;
    }

  delete mF;
  return std::string();
}

//
// Read the type of the object
//
std::string MET_ReadType(std::istream &_fp)
{
  std::streampos pos = _fp.tellg();
  std::vector<MET_FieldRecordType *> fields;
  MET_FieldRecordType* mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "ObjectType", MET_STRING, false);
  mF->required = false;
  mF->terminateRead = true;
  fields.push_back(mF);

  MET_Read(_fp, &fields, '=', true);
  _fp.seekg(pos);

  if(mF->defined)
    {
    std::string value  = (char *)(mF->value);
    delete mF;
    return value;
    }

  delete mF;
  return std::string();
}

//
// Read the subtype of the object
//
char* MET_ReadSubType(std::istream &_fp)
{
  std::streampos pos = _fp.tellg();
  std::vector<MET_FieldRecordType *> fields;
  MET_FieldRecordType* mF;
  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "ObjectType", MET_STRING, false);
  mF->required = false;
  fields.push_back(mF);

  MET_Read(_fp, &fields, '=', true);

  // Find the line right after the ObjectType
  char s[1024];
  _fp.getline( s, 500 );
  std::string value = s;
  size_t position = value.find('=');
  if(position!=std::string::npos)
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
  assert(_vType>=0);
  if(_vType<MET_NUM_VALUE_TYPES)
    {
    strcpy(_s, MET_ValueTypeName[_vType]);
    return true;
    }

  return false;
}



//
// Value to Double
//
bool MET_ValueToDouble(MET_ValueEnumType _type, const void *_data,
                       std::streamoff _index,
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
#if defined(_MSC_VER) || defined(__HP_aCC)
      // NOTE: you cannot use __int64 in an ostream in MSV6 or HPUX
      *_value = (double)((MET_LONG_LONG_TYPE)
                         ((((const MET_ULONG_LONG_TYPE *)_data)[_index])));
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
      *_value = atof(&(((const MET_ASCII_CHAR_TYPE *)_data)[_index]));
      return true;
    case MET_NONE:
    case MET_OTHER:
    default:
      *_value = 0;
      return false;
    }
}

bool MET_DoubleToValue(double _value,
                       MET_ValueEnumType _type,
                       void *_data,
                       std::streamoff _index)
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
      sprintf(&(((MET_ASCII_CHAR_TYPE *)_data)[_index]), "%f", _value);
      return true;
    case MET_NONE:
    case MET_OTHER:
    default:
      return false;
    }
}

bool MET_ValueToValue(MET_ValueEnumType _fromType, const void *_fromData,
                      std::streamoff _index,
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
      sprintf(&(((MET_ASCII_CHAR_TYPE *)_toData)[_index]), "%f", tf);
      return true;
    case MET_NONE:
    case MET_OTHER:
    default:
      return false;
    }
}

// Uncompress a stream given an uncompressedSeekPosition
METAIO_EXPORT
std::streamoff MET_UncompressStream(std::ifstream * stream,
                          std::streamoff uncompressedSeekPosition,
                          unsigned char * uncompressedData,
                          std::streamoff uncompressedDataSize,
                          std::streamoff compressedDataSize,
                          MET_CompressionTableType * compressionTable
                          )
{
  // Keep the currentpos of the string
  std::streampos currentPos = stream->tellg();
  if(currentPos == std::streampos(-1))
    {
    std::cout << "MET_UncompressStream: ERROR Stream is not valid!" << std::endl;
    return -1;
    }

  std::streamoff read = 0;

  //std::cout << "Wanted Seek = " << uncompressedSeekPosition << std::endl;
  //std::cout << "Wanted size = " << uncompressedDataSize << std::endl;

  // Size of the output buffer
  std::streamoff buffersize = 1000;

  // We try to guess the compression rate
  // Note that sometime the size of the input buffer
  // has to be bigger than the output buffer (bad compression)
  // We assume that they are equal
  double compressionRate = 1;

  std::streamoff zseekpos = 0;
  std::streamoff seekpos = 0;
  bool firstchunk = true;

  // Allocate the stream if necessary
  z_stream* d_stream = compressionTable->compressedStream;
  if(compressionTable->compressedStream == nullptr)
    {
    d_stream = new z_stream;
    d_stream->zalloc = (alloc_func)nullptr;
    d_stream->zfree = (free_func)nullptr;
    d_stream->opaque = (voidpf)nullptr;
    inflateInit2(d_stream,47); // allow both gzip and zlib compression headers
    compressionTable->compressedStream = d_stream;
    compressionTable->buffer = new char[1001];
    compressionTable->bufferSize = 0;
    }


  // Try to find the current seek position in the compressed
  // and uncompressed stream from the compressionTable
  // The table is stored in order
  if(compressionTable->offsetList.size()>0)
    {
    MET_CompressionOffsetListType::const_iterator it = compressionTable->offsetList.end();
    --it;

    if(uncompressedSeekPosition < (*it).uncompressedOffset)
      {
      if((*it).uncompressedOffset-uncompressedSeekPosition > compressionTable->bufferSize)
        {
        std::cout << "ERROR: Cannot go backward by more than the buffer size (1000)"
                  << std::endl;
        return 0;
        }

      char* buffer = compressionTable->buffer;
      std::streamoff start = uncompressedSeekPosition-((*it).uncompressedOffset-compressionTable->bufferSize);
      buffer += start;

      std::streamoff readSize = uncompressedDataSize;
      std::streamoff sizeInBuffer = compressionTable->bufferSize-start;
      if(readSize>sizeInBuffer)
        {
        memcpy(uncompressedData,buffer,(size_t)sizeInBuffer);
        uncompressedData += sizeInBuffer;
        zseekpos = (*it).compressedOffset;
        seekpos = (*it).uncompressedOffset;
        uncompressedSeekPosition += sizeInBuffer;
        uncompressedDataSize -= sizeInBuffer;
        }
      else // read from buffer and return
        {
        memcpy(uncompressedData,buffer,(size_t)readSize);
        return uncompressedDataSize;
        }
      }
    else
      {
      zseekpos = (*it).compressedOffset;
      seekpos = (*it).uncompressedOffset;
      }
    }

  //std::cout << "Using = " << seekpos << " : " << zseekpos << std::endl;

  while(seekpos < uncompressedSeekPosition+uncompressedDataSize)
    {
    // If we are reading the current buffer we read everything
    if(seekpos >= uncompressedSeekPosition)
      {
      buffersize = uncompressedSeekPosition+uncompressedDataSize-seekpos;
      firstchunk = false;
      }

    unsigned char* outdata = new unsigned char[static_cast<size_t>(buffersize)];

    d_stream->avail_out = (uInt)( buffersize );

    // How many byte from compressed streamed should we read
    std::streamoff inputBufferSize = (std::streamoff)(buffersize/compressionRate);

    if(inputBufferSize == 0)
      {
      inputBufferSize = 1;
      }
    if((currentPos+zseekpos+inputBufferSize) > compressedDataSize)
      {
      inputBufferSize = compressedDataSize-zseekpos;
      }

    unsigned char* inputBuffer = new unsigned char[static_cast<size_t>(inputBufferSize)];
    stream->seekg(currentPos+zseekpos,std::ios::beg);
    stream->read((char *)inputBuffer, (size_t)inputBufferSize);

    d_stream->next_in  = inputBuffer;
    d_stream->avail_in = static_cast<int>(stream->gcount());
    d_stream->next_out = outdata;

    int inflate_error = inflate(d_stream, Z_NO_FLUSH);
    if(inflate_error < 0)
      {
      return -1;
      }

    std::streampos previousSeekpos = seekpos;

    seekpos += buffersize-d_stream->avail_out;
    zseekpos += stream->gcount()-d_stream->avail_in;

    // Store the last buffer into memory in case we need it
    // in the near future.
    std::streamoff previousBufferSize = seekpos-previousSeekpos;
    if(previousBufferSize>1000)
      {
      // WARNING: We probably need to offset outdata at some point...
      previousBufferSize = 1000;
      }

    memcpy(compressionTable->buffer,outdata,(size_t)previousBufferSize);
    compressionTable->bufferSize = previousBufferSize;

    //std::cout << "Current pos = " << seekpos << " : " << zseekpos << std::endl;

    // If go further than the uncompressedSeekPosition we start writing the stream
    if(seekpos >= uncompressedSeekPosition)
      {
      if(firstchunk)
        {
        outdata += uncompressedSeekPosition-previousSeekpos;
        std::streamoff writeSize = seekpos-uncompressedSeekPosition;

        if(writeSize > uncompressedDataSize)
          {
          writeSize = uncompressedDataSize;
          }

        memcpy(uncompressedData,outdata,(size_t)writeSize);

        // Restore the position of the buffer
        outdata -= uncompressedSeekPosition-previousSeekpos;

        uncompressedData += writeSize;
        read += writeSize;

        firstchunk = false;
        }
      else // read everything
        {
        std::streamoff writeSize = seekpos-previousSeekpos;
        memcpy(uncompressedData,outdata,(size_t)writeSize);
        if(writeSize > uncompressedDataSize)
          {
          writeSize = uncompressedDataSize;
          }
        uncompressedData += writeSize;
        read += writeSize;
        }
      }
    delete [] outdata;
    delete [] inputBuffer;
    }

  // Save the state of the compression for later use
  MET_CompressionOffsetType offset;
  offset.compressedOffset = zseekpos; // compressed
  offset.uncompressedOffset = seekpos; // uncompressed
  compressionTable->offsetList.push_back(offset);

  // Seek to the current position
  stream->seekg(currentPos,std::ios::beg);
  return read;
}


unsigned char * MET_PerformCompression(const unsigned char * source,
                                       std::streamoff sourceSize,
                                       std::streamoff * compressedDataSize,
                                       int compressionLevel)
{

  z_stream  z;
  z.zalloc  = (alloc_func)nullptr;
  z.zfree   = (free_func)nullptr;
  z.opaque  = (voidpf)nullptr;

  std::streamoff buffer_out_size = sourceSize;
  std::streamoff max_chunk_size = MET_MaxChunkSize;
  std::streamoff chunk_size  = std::min(sourceSize, max_chunk_size);
  unsigned char * input_buffer      = const_cast<unsigned char *>(source);
  unsigned char * output_buffer     = new unsigned char[chunk_size];
  unsigned char * compressed_data   = new unsigned char[buffer_out_size];

  /*int ret =*/ deflateInit(&z, compressionLevel);
  //assert(ret == Z_OK);

  std::streamoff cur_in_start = 0;
  std::streamoff cur_out_start = 0;
  int flush;
  do
    {
    z.avail_in = static_cast<uInt>(std::min(sourceSize - cur_in_start, chunk_size));
    z.next_in  = input_buffer + cur_in_start;
    bool last_chunk = (cur_in_start + z.avail_in) >= sourceSize;
    flush = last_chunk ? Z_FINISH : Z_NO_FLUSH;
    cur_in_start += z.avail_in;
    do
      {
      z.avail_out = static_cast<uInt>(chunk_size);
      z.next_out  = output_buffer;
      /*ret =*/ deflate(&z, flush);
      //assert(ret != Z_STREAM_ERROR);
      std::streamoff count_out = chunk_size - z.avail_out;
      if ( (cur_out_start + count_out) >= buffer_out_size )
        {
        // if we don't have enough allocation for the output buffer
        // when the output is bigger than the input (true for small images)
        unsigned char* compressed_data_temp = new unsigned char[cur_out_start+count_out+1];
        memcpy(compressed_data_temp, compressed_data, (size_t)buffer_out_size);
        delete [] compressed_data;
        compressed_data = compressed_data_temp;
        buffer_out_size = cur_out_start+count_out+1;
        }
      memcpy((char*)compressed_data + cur_out_start, (char*)output_buffer, (size_t)count_out);
      cur_out_start += count_out;
      }
    while (z.avail_out == 0);
    //assert(z.avail_in == 0);
    }
  while (flush != Z_FINISH);
  //assert(ret == Z_STREAM_END);
  delete [] output_buffer;
  *compressedDataSize = cur_out_start;  // don't use z.total_out, it's limited to 2^32!
  deflateEnd(&z);
  return compressed_data;
}

bool MET_PerformUncompression(const unsigned char * sourceCompressed,
                              std::streamoff sourceCompressedSize,
                              unsigned char * uncompressedData,
                              std::streamoff uncompressedDataSize)
{
  z_stream d_stream;

  d_stream.zalloc = (alloc_func)nullptr;
  d_stream.zfree = (free_func)nullptr;
  d_stream.opaque = (voidpf)nullptr;

  inflateInit2(&d_stream,47); // allow both gzip and zlib compression headers

  std::streamoff max_chunk_size = MET_MaxChunkSize;
  std::streamoff source_pos = 0;
  std::streamoff dest_pos = 0;
  int err;
  do
    {
    d_stream.next_in = const_cast<unsigned char *>(sourceCompressed + source_pos);
    d_stream.avail_in = static_cast<uInt>(std::min(
      sourceCompressedSize - source_pos,
      max_chunk_size)
    );
    source_pos += d_stream.avail_in;
    do
      {
      uInt cur_remain_chunk = static_cast<uInt>( std::min( uncompressedDataSize - dest_pos,
                                                           MET_MaxChunkSize) );
      d_stream.next_out = static_cast<unsigned char *>(uncompressedData) + dest_pos;
      d_stream.avail_out = cur_remain_chunk;
      err = inflate(&d_stream, Z_NO_FLUSH);
      if (err == Z_STREAM_END || err < 0)
        {
        if (err != Z_STREAM_END &&
            err != Z_BUF_ERROR) // Z_BUF_ERROR means there is still data to uncompress,
          {                     // but no space left in buffer; non-fatal
          std::cerr << "Uncompress failed" << std::endl;
          }
          break;
        }
      uInt count_uncompressed = cur_remain_chunk - d_stream.avail_out;
      dest_pos += count_uncompressed;
      }
    while (d_stream.avail_out == 0);
    }
  while (err != Z_STREAM_END && err >= 0);
  inflateEnd(&d_stream);
  return true;
}

bool MET_StringToWordArray(const char *s, int *n, char ***val)
{
  ptrdiff_t l = strlen(s);

  ptrdiff_t p = 0;
  while(p<l && s[p] == ' ')
    {
    p++;
    }

  *n = 0;
  ptrdiff_t pp = p;
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

  ptrdiff_t i, j;
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

bool MET_GetFilePath(const std::string& _fName, std::string& _fPath)
{
  auto const pos = _fName.find_last_of("/\\");
  if (pos == std::string::npos)
    {
    _fPath = "";
    return false;
    }

  _fPath = _fName.substr(0, pos + 1);
  return true;
}

bool MET_GetFileSuffixPtr(const std::string& _fName, int *i)
{
  *i = static_cast<int>( _fName.length() );
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

bool MET_SetFileSuffix(std::string& _fName, const std::string& _suf)
{
  int i;
  MET_GetFileSuffixPtr(_fName, &i);
  if(i>0)
    {
    const char * suffixStart;
    if(_suf[0] == '.')
      suffixStart = &_suf[1];
    else
      suffixStart = &_suf[0];
    _fName.resize(i);
    _fName.append(suffixStart);
    return true;
    }
  else
    {
    if( _suf[0] != '.')
      {
      _fName.append(1, '.');
      }
    _fName.append(_suf);
    return true;
    }
}

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
                                  size_t _length)
{
  strcpy(_mf->name, _name);
  _mf->type = _type;
  _mf->defined = false;
  _mf->dependsOn = _dependsOn;
  _mf->required = _required;
  _mf->terminateRead = false;
  _mf->length = static_cast<int>(_length);
  _mf->value[0] = 0;
  return true;
}

static bool MET_SkipToVal(std::istream &fp)
{
  int c;
  if( fp.eof() )
    {
    return false;
    }

  c = fp.get();

  while(  !fp.eof() && c != MET_SeperatorChar && c != ':' )
    {
    c = fp.get();
    }

  while( !fp.eof() && ( c == MET_SeperatorChar || c == ':' || IsBlank(c) ) )
    {
    c = fp.get();
    }

  if( fp.eof() )
    {
    std::cerr << "Incomplete file record definition"
                        << std::endl;
    return false;
    }

  fp.putback(static_cast<char>(c));

  return true;
}

static bool MET_IsComplete(std::vector<MET_FieldRecordType *> * fields)
{
  std::vector<MET_FieldRecordType *>::iterator fieldIter;
  for(fieldIter=fields->begin(); fieldIter!=fields->end(); ++fieldIter)
    {
    if((*fieldIter)->required && !(*fieldIter)->defined)
      {
      std::cerr << (*fieldIter)->name << " required and not defined."
                << std::endl;
      return false;
      }
    }
  return true;
}

//
bool MET_Read(std::istream &fp,
              std::vector<MET_FieldRecordType *> * fields,
              char _MET_SeperatorChar, bool oneLine, bool display_warnings,
              std::vector<MET_FieldRecordType *> * newFields)
{

  char s[1024];
  int i;
  size_t j;

  std::vector<MET_FieldRecordType *>::iterator fieldIter;

  MET_SeperatorChar = _MET_SeperatorChar;

  bool found;

  unsigned char c;
  while(!fp.eof())
    {
    i = 0;
    c = static_cast<unsigned char>(fp.get());
    while(!fp.eof() && c != MET_SeperatorChar && c != ':'
          && isspace(c))
      {
      c = static_cast<unsigned char>(fp.get());
      }
    // save name up to separator or end of line
    while(!fp.eof() && c != MET_SeperatorChar && c != ':' && c != '\r' && c != '\n' && i<500)
      {
      s[i++] = c;
      c = static_cast<unsigned char>(fp.get());
      }
    if(fp.eof() || i >= 500)
      {
      break;
      }
    fp.putback(c);
    s[i] = '\0';

    // trim white space on name
    i--;
    while(i>0 && IsBlank(s[i]))
      {
      s[i--] = '\0';
      }

    found = false;
    for(fieldIter=fields->begin(); fieldIter!=fields->end(); ++fieldIter)
      {
      if(!strcmp((*fieldIter)->name, s))
        {
        if((*fieldIter)->dependsOn >= 0)
          if(!(*fields)[(*fieldIter)->dependsOn]->defined)
            {
            std::cerr << (*fieldIter)->name
                                << " defined prior to defining ";
            std::cerr << (*fields)[(*fieldIter)->dependsOn]->name
                                << std::endl;
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
            c = static_cast<unsigned char>(fp.get());
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
            MET_ASCII_CHAR_TYPE * str =
                               (MET_ASCII_CHAR_TYPE *)((*fieldIter)->value);
            fp.getline( str, 500 );
            MET_StringStripEnd(str);
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
              for(j=0; j<(size_t)(*fieldIter)->length; j++)
                {
                fp >> (*fieldIter)->value[j];
                }
              }
            else
              {
              if((*fieldIter)->length <= 0)
                {
                std::cerr <<
                  "Arrays must have dependency or pre-specified lengths"
                  << std::endl;
                return false;
                }
              for(j=0; j<(size_t)(*fieldIter)->length; j++)
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
              for(j=0; j<(size_t)(*fieldIter)->length*(*fieldIter)->length;
                  j++)
                {
                fp >> (*fieldIter)->value[j];
                }
              }
            else
              {
              if((*fieldIter)->length <= 0)
                {
                std::cerr <<
                  "Arrays must have dependency or pre-specified lengths"
                  << std::endl;
                return false;
                }
              for(j=0; j<(size_t)(*fieldIter)->length*(*fieldIter)->length; j++)
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
      if( newFields != nullptr )
        {
        MET_SkipToVal(fp);
        if(fp.eof())
          {
          break;
          }
        MET_FieldRecordType * mF = new MET_FieldRecordType;
        MET_InitReadField(mF, s, MET_STRING, false);
        MET_ASCII_CHAR_TYPE * str = (MET_ASCII_CHAR_TYPE *)(mF->value);
        fp.getline( str, 500 );
        MET_StringStripEnd(str);
        mF->length = static_cast<int>( strlen( str ) );
        newFields->push_back(mF);
        }
      else
        {
        if(display_warnings)
          {
          std::cerr << "Skipping unrecognized field "
                              << s << std::endl;
          }
        fp.getline( s, 500 );
        }
      }
    if(oneLine)
      {
      return MET_IsComplete(fields);
      }
    }

  return MET_IsComplete(fields);
}

// Workaround for ancient compilers.
#if defined(_MSC_VER) || defined(__HP_aCC)
static std::string convert_ulonglong_to_string(MET_ULONG_LONG_TYPE val)
{
  std::string result;
  while (val > 0)
    {
    result = static_cast<char>((val % 10)+ '0') + result;
    val /= 10;
    }
  return result;
}
#endif

//
bool MET_Write(std::ostream &fp,
               std::vector<MET_FieldRecordType *> * fields,
               char _MET_SeperatorChar)
{
  MET_SeperatorChar = _MET_SeperatorChar;

  int j;
  std::vector<MET_FieldRecordType *>::iterator fieldIter;
  for(fieldIter=fields->begin(); fieldIter!=fields->end(); ++fieldIter)
    {
    switch((*fieldIter)->type)
      {
      case MET_NONE:
        {
        fp << (*fieldIter)->name << " " << MET_SeperatorChar << " "
           << std::endl;
        break;
        }
      case MET_ASCII_CHAR:
        {
        fp << (*fieldIter)->name << " " << MET_SeperatorChar << " ";
        fp << (MET_CHAR_TYPE)(*fieldIter)->value[0] << std::endl;
        break;
        }
      case MET_CHAR:
      case MET_SHORT:
      case MET_LONG:
      case MET_INT:
        {
        fp << (*fieldIter)->name << " " << MET_SeperatorChar << " ";
        fp << (MET_LONG_TYPE)((*fieldIter)->value[0]) << std::endl;
        break;
        }
      case MET_LONG_LONG:
        {
#if defined(_MSC_VER) || defined(__HP_aCC)
        // NOTE: you cannot use __int64 in an ostream in MSV6 or HPUX
        fp << (double)((MET_LONG_LONG_TYPE)((*fieldIter)->value[0]))
           << std::endl;
        std::cerr << "Programs compiled using MSV6 or HPUX cannot"
                            << " write 64 bit ints" << std::endl;
        std::cerr << "  Writing as double instead."
                            << "  Loss of precision results."
                            << std::endl;
#else
        fp << (MET_LONG_LONG_TYPE)((*fieldIter)->value[0])
           << std::endl;
#endif
        break;
        }
      case MET_UCHAR:
      case MET_USHORT:
      case MET_UINT:
      case MET_ULONG:
        {
        fp << (*fieldIter)->name << " " << MET_SeperatorChar << " ";
        fp << (MET_ULONG_TYPE)((*fieldIter)->value[0]) << std::endl;
        break;
        }
      case MET_ULONG_LONG:
        { // ToDo: check why name was not printed here previously!
        fp << (*fieldIter)->name << " " << MET_SeperatorChar << " ";
#if defined(_MSC_VER) || defined(__HP_aCC)
        // NOTE: you cannot use __int64 in an ostream in MSV6 or HPUX
        fp << convert_ulonglong_to_string((MET_ULONG_LONG_TYPE)((*fieldIter)->value[0])) << std::endl;
#else
        fp << (MET_ULONG_LONG_TYPE)((*fieldIter)->value[0]) << std::endl;
#endif
        break;
        }
      case MET_FLOAT:
      case MET_DOUBLE:
        {
        fp << (*fieldIter)->name << " " << MET_SeperatorChar << " ";
        fp << (MET_DOUBLE_TYPE)(*fieldIter)->value[0] << std::endl;
        break;
        }
      case MET_STRING:
        {
        if ( (*fieldIter)->length == 0 )
          {
          std::cerr << "Warning:";
          std::cerr << "The field " << (*fieldIter)->name
                              << "has zero length. "
                              << "Refusing to write empty string value.";
          std::cerr << std::endl;
          }
        fp << (*fieldIter)->name << " " << MET_SeperatorChar << " ";
        if((*fieldIter)->dependsOn >= 0)
          {
          if((*fieldIter)->length !=
             (*fields)[(*fieldIter)->dependsOn]->value[0])
            {
            std::cerr << "Warning:";
            std::cerr << "length and dependsOn values not equal"
                                << " in write";
            std::cerr << std::endl;
            }
          }
        fp.write( (char *)((*fieldIter)->value), (*fieldIter)->length );
        fp << std::endl;
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
            std::cerr << "Warning: ";
            std::cerr << "Length and dependsOn values not equal"
                                << " in write";
            std::cerr << std::endl;
            }
          }
        for(j=0; j<(*fieldIter)->length; j++)
          {
          fp << " " << (MET_LONG_TYPE)((*fieldIter)->value[j]);
          }
        fp << std::endl;
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
            std::cerr << "Warning: ";
            std::cerr << "Length and dependsOn values not equal"
                                << " in write";
            std::cerr << std::endl;
            }
          }
        for(j=0; j<(*fieldIter)->length; j++)
          {
#if defined(_MSC_VER) || defined(__HP_aCC)
          // NOTE: you cannot use __int64 in an ostream in MSV6 or HPUX
          fp << " " << (double)((MET_LONG_LONG_TYPE)((*fieldIter)->value[j]));
          std::cerr << "Programs compiled using MSV6 cannot"
                              << " write 64 bit ints"
                              << std::endl;
          std::cerr << "  Writing as double instead."
                              << " Loss of precision results."
                              << std::endl;
#else
          fp << " " << (MET_LONG_LONG_TYPE)((*fieldIter)->value[j]);
#endif
          }
        fp << std::endl;
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
            std::cerr << "Warning: ";
            std::cerr << "Length and dependsOn values not equal"
                                << " in write";
            std::cerr << std::endl;
            }
          }
        for(j=0; j<(*fieldIter)->length; j++)
          {
          fp << " " << (MET_ULONG_TYPE)((*fieldIter)->value[j]);
          }
        fp << std::endl;
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
            std::cerr << "Warning: ";
            std::cerr << "Length and dependsOn values not equal"
                                << " in write";
            std::cerr << std::endl;
            }
          }
        for(j=0; j<(*fieldIter)->length; j++)
          {
#if defined(_MSC_VER) || defined(__HP_aCC)
          // NOTE: you cannot use __int64 in an ostream in MSV6
          fp << " " << (double)((MET_LONG_LONG_TYPE)((MET_ULONG_LONG_TYPE)
                                ((*fieldIter)->value[j])));
          std::cerr << "Programs compiled using MSV6 or HPUX"
                              << " cannot write 64 bit ints"
                              << std::endl;
          std::cerr << " Writing as double instead."
                              << " Loss of precision results."
                              << std::endl;
#else
          fp << " " << (MET_ULONG_LONG_TYPE)((*fieldIter)->value[j]);
#endif
          }
        fp << std::endl;
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
            std::cerr << "Warning: ";
            std::cerr << "length and dependsOn values not equal in write";
            std::cerr << std::endl;
            }
          }
        for(j=0; j<(*fieldIter)->length; j++)
          {
          fp << " " << (double)(*fieldIter)->value[j];
          }
        fp << std::endl;
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
            std::cerr << "Warning: ";
            std::cerr << "length and dependsOn values not equal in write";
            std::cerr << std::endl;
            }
          }
        for(j=0; j<(*fieldIter)->length*(*fieldIter)->length; j++)
          {
          fp << " " << (double)(*fieldIter)->value[j];
          }
        fp << std::endl;
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

bool MET_WriteFieldToFile(std::ostream & _fp, const char *_fieldName,
                          MET_ValueEnumType _pType, size_t _n, const void *_v)
{
  size_t i;
  MET_FieldRecordType f;

  snprintf(f.name, sizeof(f.name), "%s", _fieldName);
  f.defined = false;
  f.dependsOn = -1;
  f.length = static_cast<int>(_n);
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
#if defined(_MSC_VER) || defined(__HP_aCC)
        // NOTE: you cannot use __int64 in an ostream in MSV6 or HPUX
        f.value[i] = (double)((MET_LONG_LONG_TYPE)
                              (((const MET_ULONG_LONG_TYPE *)_v)[i]));
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
      strcpy((MET_ASCII_CHAR_TYPE *)(f.value),
             (const MET_ASCII_CHAR_TYPE *)_v);
      break;
    case MET_FLOAT_MATRIX:
      for(i=0; i<_n*_n; i++)
        {
        f.value[i] = (double)((const MET_FLOAT_TYPE *)_v)[i];
        }
      break;
    case MET_NONE:
    case MET_OTHER:
    default:
      break;
    }

  std::vector<MET_FieldRecordType *> l;
  l.clear();
  l.push_back(&f);
  MET_Write(_fp, &l);

  return true;
}

bool MET_WriteFieldToFile(std::ostream & _fp, const char *_fieldName,
  MET_ValueEnumType _pType, double _v)
{
  MET_FieldRecordType f;

  snprintf(f.name, sizeof(f.name), "%s", _fieldName);
  f.defined = false;
  f.dependsOn = -1;
  f.length = 1;
  f.required = false;
  f.type = _pType;
  f.value[0] = _v;

  std::vector<MET_FieldRecordType *> l;
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
