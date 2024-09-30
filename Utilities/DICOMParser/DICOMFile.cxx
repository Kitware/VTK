// SPDX-FileCopyrightText: Copyright (c) 2003 Matt Turek
// SPDX-License-Identifier: BSD-4-Clause
#ifdef _MSC_VER
#pragma warning(disable : 4514)
#pragma warning(disable : 4710)
#pragma warning(push, 3)
#endif

#include "DICOMFile.h"
#include "DICOMConfig.h"

#include "vtksys/FStream.hxx"

#include <iomanip>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <string>

VTK_ABI_NAMESPACE_BEGIN
DICOMFile::DICOMFile()
{
  /* Are we little or big endian?  From Harbison&Steele.  */
  union
  {
    long l;
    char c[sizeof(long)];
  } u;
  u.l = 1;
  PlatformIsBigEndian = (u.c[sizeof(long) - 1] == 1);
  if (PlatformIsBigEndian)
  {
    PlatformEndian = "BigEndian";
  }
  else
  {
    PlatformEndian = "LittleEndian";
  }
  InputStream = nullptr;
}

DICOMFile::~DICOMFile()
{
  this->Close();
}

DICOMFile::DICOMFile(const DICOMFile& in)
{
  if (strcmp(in.PlatformEndian, "LittleEndian") == 0)
  {
    PlatformEndian = "LittleEndian";
  }
  else
  {
    PlatformEndian = "BigEndian";
  }
  //
  // Some compilers can't handle. Comment out for now.
  //
  // InputStream = in.InputStream;
}

void DICOMFile::operator=(const DICOMFile& in)
{
  if (strcmp(in.PlatformEndian, "LittleEndian") == 0)
  {
    PlatformEndian = "LittleEndian";
  }
  else
  {
    PlatformEndian = "BigEndian";
  }
  //
  // Some compilers can't handle. Comment out for now.
  //
  // InputStream = in.InputStream;
}

bool DICOMFile::Open(const std::string& filename)
{
  std::ios_base::openmode mode = std::ios::in;
#ifdef _WIN32
  mode |= std::ios::binary;
#endif
  delete InputStream; // ensure any old streams are closed
  InputStream = new vtksys::ifstream(filename.c_str(), mode);

  if (InputStream && !InputStream->fail())
  {
    return true;
  }

  delete InputStream;
  InputStream = nullptr;
  return false;
}

void DICOMFile::Close()
{
  delete InputStream;
  InputStream = nullptr;
}

long DICOMFile::Tell()
{
  if (InputStream)
  {
    return static_cast<long>(InputStream->tellg());
  }
  return 0;
}

void DICOMFile::SkipToPos(long increment)
{
  if (InputStream)
  {
    InputStream->seekg(increment, std::ios::beg);
  }
}

long DICOMFile::GetSize()
{
  if (InputStream)
  {
    long curpos = this->Tell();

    InputStream->seekg(0, std::ios::end);

    long size = this->Tell();
    this->SkipToPos(curpos);

    return size;
  }
  return 0;
}

void DICOMFile::Skip(long increment)
{
  if (InputStream)
  {
    InputStream->seekg(increment, std::ios::cur);
  }
}

void DICOMFile::SkipToStart()
{
  InputStream->seekg(0, std::ios::beg);
}

void DICOMFile::Read(void* ptr, long nbytes)
{
  InputStream->read(static_cast<char*>(ptr), nbytes);
  // std::cout << (char*) ptr << std::endl;
}

doublebyte DICOMFile::ReadDoubleByte()
{
  doublebyte sh = 0;
  int sz = sizeof(doublebyte);
  this->Read(reinterpret_cast<char*>(&sh), sz);
  if (PlatformIsBigEndian)
  {
    sh = swap2(sh);
  }
  return (sh);
}

doublebyte DICOMFile::ReadDoubleByteAsLittleEndian()
{
  doublebyte sh = 0;
  int sz = sizeof(doublebyte);
  this->Read(reinterpret_cast<char*>(&sh), sz);
  if (PlatformIsBigEndian)
  {
    sh = swap2(sh);
  }
  return (sh);
}

quadbyte DICOMFile::ReadQuadByte()
{
  quadbyte sh;
  int sz = sizeof(quadbyte);
  this->Read(reinterpret_cast<char*>(&sh), sz);
  if (PlatformIsBigEndian)
  {
    sh = static_cast<quadbyte>(swap4(static_cast<uint>(sh)));
  }
  return (sh);
}

quadbyte DICOMFile::ReadNBytes(int len)
{
  quadbyte ret = -1;
  switch (len)
  {
    case 1:
      char ch;
      this->Read(&ch, 1); // from Image
      ret = static_cast<quadbyte>(ch);
      break;
    case 2:
      ret = static_cast<quadbyte>(ReadDoubleByte());
      break;
    case 4:
      ret = ReadQuadByte();
      break;
    default:
      std::cerr << "Unable to read " << len << " bytes" << std::endl;
      break;
  }
  return (ret);
}

float DICOMFile::ReadAsciiFloat(int len)
{
  float ret = 0.0;

  char* val = new char[len + 1];
  this->Read(val, len);
  val[len] = '\0';

#if 0
  //
  // istrstream destroys the data during formatted input.
  //
  int len2 = static_cast<int> (strlen((char*) val));
  char* val2 = new char[len2];
  strncpy(val2, (char*) val, len2);

  std::istrstream data(val2);
  data >> ret;
  delete [] val2;
#else
  sscanf(val, "%e", &ret);
#endif

  std::cout << "Read ASCII float: " << ret << std::endl;

  delete[] val;
  return (ret);
}

int DICOMFile::ReadAsciiInt(int len)
{
  int ret = 0;

  char* val = new char[len + 1];
  this->Read(val, len);
  val[len] = '\0';

#if 0
  //
  // istrstream destroys the data during formatted input.
  //
  int len2 = static_cast<int> (strlen((char*) val));
  char* val2 = new char[len2];
  strncpy(val2, (char*) val, len2);

  std::istrstream data(val2);
  data >> ret;
  delete [] val2;
#else
  sscanf(val, "%d", &ret);
#endif

  std::cout << "Read ASCII int: " << ret << std::endl;

  delete[] val;
  return (ret);
}

char* DICOMFile::ReadAsciiCharArray(int len)
{
  if (len <= 0)
  {
    return nullptr;
  }
  char* val = new char[len + 1];
  this->Read(val, len);
  val[len] = 0; // NULL terminate.
  return val;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
VTK_ABI_NAMESPACE_END
