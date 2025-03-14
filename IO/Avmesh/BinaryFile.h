// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef BinaryFile_h
#define BinaryFile_h

#include "vtkByteSwap.h"
#include <fstream>
#include <string>

class BinaryFile
{
public:
  BinaryFile(const char* fname);
  ~BinaryFile();

  bool good() const;

  template <typename T>
  void ReadArray(T* array, size_t nitems)
  {
    this->InFile.read((char*)array, nitems * sizeof(T));
    if (this->NeedSwap)
    {
#ifdef VTK_WORDS_BIGENDIAN
      vtkByteSwap::SwapBERange(array, nitems);
#else
      vtkByteSwap::SwapLERange(array, nitems);
#endif
    }
  }

  template <typename T>
  T ReadOne()
  {
    T x;
    this->ReadArray(&x, 1);
    return x;
  }

  int ReadInt();

  double ReadDouble();

  void ReadCString(char* s, size_t n = 128);

  std::string ReadStdString();

  static int SwapInt(int x);

  void SetSwap(bool val);

private:
  std::ifstream InFile;
  bool NeedSwap;
};

#endif // BinaryFile_h
