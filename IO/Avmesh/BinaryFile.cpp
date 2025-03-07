#include "BinaryFile.h"

BinaryFile::BinaryFile(const char* fname)
  : fin(fname, std::ios::binary)
  , NeedSwap(false)
{
}

BinaryFile::~BinaryFile() {}

bool BinaryFile::good() const
{
  return fin.good();
}

int BinaryFile::ReadInt()
{
  return ReadOne<int>();
}

double BinaryFile::ReadDouble()
{
  return ReadOne<double>();
}

void BinaryFile::ReadCString(char* s, size_t n = 128)
{
  fin.read(s, n);
}

std::string BinaryFile::ReadStdString()
{
  int len = ReadInt();
  std::string s(len);
  ReadCString(s.data(), n);
  return s;
}

int BinaryFile::SwapInt(int x) const
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
  NeedSwap = val;
}
