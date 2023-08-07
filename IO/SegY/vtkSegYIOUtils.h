// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkSegYIOUtils_h
#define vtkSegYIOUtils_h

#include "vtkABINamespace.h"

#include <fstream>

VTK_ABI_NAMESPACE_BEGIN
class vtkSegYIOUtils
{
public:
  char readChar(std::istream& in);
  short readShortInteger(std::streamoff pos, std::istream& in);
  short readShortInteger(std::istream& in);
  int readLongInteger(std::streamoff pos, std::istream& in);
  int readLongInteger(std::istream& in);
  float readFloat(std::istream& in);
  float readIBMFloat(std::istream& in);
  unsigned char readUChar(std::istream& in);
  void swap(char* a, char* b);
  static vtkSegYIOUtils* Instance();
  std::streamoff getFileSize(std::istream& in);

  bool IsBigEndian;

private:
  vtkSegYIOUtils();
  bool checkIfBigEndian()
  {
    unsigned short a = 0x1234;
    if (*(reinterpret_cast<unsigned char*>(&a)) == 0x12)
      return true;
    return false;
  }
};

VTK_ABI_NAMESPACE_END
#endif // vtkSegYIOUtils_h
// VTK-HeaderTest-Exclude: vtkSegYIOUtils.h
