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
    fin.read((char*)array, nitems * sizeof(T));
    if (NeedSwap)
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
    ReadArray(&x, 1);
    return x;
  }

  int ReadInt();

  double ReadDouble();

  void ReadCString(char* s, size_t n = 128);

  std::string ReadStdString();

  static int SwapInt(int x);

  void SetSwap(bool val);

private:
  std::ifstream& fin;
  bool NeedSwap;
};

#endif // BinaryFile_h
