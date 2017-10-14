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
  int readShortInteger(int pos, std::ifstream& in);
  int readLongInteger(int pos, std::ifstream& in);
  int readLongInteger(std::ifstream& in);
  float readFloat(std::ifstream& in);
  float readIBMFloat(std::ifstream& in);
  char readChar(std::ifstream& in);
  unsigned char readUChar(std::ifstream& in);
  void swap(char* a, char* b);
  bool isBigEndian;
  static vtkSegYIOUtils* Instance();
  int getFileSize(std::ifstream& in);

private:
  vtkSegYIOUtils();
  bool checkIfBigEndian()
  {
    ushort a = 0x1234;
    if (*((unsigned char*)&a) == 0x12)
      return true;
    return false;
  }
};

#endif // vtkSegYIOUtils_h
// VTK-HeaderTest-Exclude: vtkSegYIOUtils.h
