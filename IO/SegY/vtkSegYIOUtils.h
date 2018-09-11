/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSegYIOUtils.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkSegYIOUtils_h
#define vtkSegYIOUtils_h
#ifndef __VTK_WRAP__

#include <fstream>

class vtkSegYIOUtils
{
public:
  char readChar(std::ifstream& in);
  short readShortInteger(std::streamoff pos, std::ifstream& in);
  short readShortInteger(std::ifstream& in);
  int readLongInteger(std::streamoff pos, std::ifstream& in);
  int readLongInteger(std::ifstream& in);
  float readFloat(std::ifstream& in);
  float readIBMFloat(std::ifstream& in);
  unsigned char readUChar(std::ifstream& in);
  void swap(char* a, char* b);
  static vtkSegYIOUtils* Instance();
  std::streamoff getFileSize(std::ifstream& in);

  bool IsBigEndian;

private:
  vtkSegYIOUtils();
  bool checkIfBigEndian()
  {
    unsigned short a = 0x1234;
    if (*((unsigned char*)&a) == 0x12)
      return true;
    return false;
  }
};

#endif
#endif // vtkSegYIOUtils_h
// VTK-HeaderTest-Exclude: vtkSegYIOUtils.h
