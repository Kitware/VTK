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

#include <fstream>

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
    if (*((unsigned char*)&a) == 0x12)
      return true;
    return false;
  }
};

#endif // vtkSegYIOUtils_h
// VTK-HeaderTest-Exclude: vtkSegYIOUtils.h
