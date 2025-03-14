// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "BinaryFile.h"

BinaryFile::BinaryFile(const char* fname)
  : InFile(fname, std::ios::binary)
  , NeedSwap(false)
{
}

BinaryFile::~BinaryFile() = default;

bool BinaryFile::good() const
{
  return this->InFile.good();
}

int BinaryFile::ReadInt()
{
  return this->ReadOne<int>();
}

double BinaryFile::ReadDouble()
{
  return this->ReadOne<double>();
}

void BinaryFile::ReadCString(char* s, size_t n)
{
  this->InFile.read(s, n);
}

std::string BinaryFile::ReadStdString()
{
  int len = this->ReadInt();
  std::string s(len, '\0');
  this->ReadCString(s.data(), len);
  return s;
}

int BinaryFile::SwapInt(int x)
{
#ifdef VTK_WORDS_BIGENDIAN
  vtkByteSwap::SwapLE(&x);
#else
  vtkByteSwap::SwapBE(&x);
#endif
  return x;
}

void BinaryFile::SetSwap(bool val)
{
  this->NeedSwap = val;
}
