/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSegYIOUtils.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSegYIOUtils.h"

#include <cmath>
#include <cstring>
#include <sys/types.h>

//----------------------------------------------------------------------------
vtkSegYIOUtils::vtkSegYIOUtils()
{
  isBigEndian = checkIfBigEndian();
}

//----------------------------------------------------------------------------
vtkSegYIOUtils* vtkSegYIOUtils::Instance()
{
  static vtkSegYIOUtils vtkSegYIOUtils;
  return &vtkSegYIOUtils;
}

//----------------------------------------------------------------------------
int vtkSegYIOUtils::readShortInteger(int pos, std::ifstream& in)
{
  in.seekg(pos, in.beg);
  char buffer[2];
  in.read(buffer, sizeof(buffer));

  if (!isBigEndian)
  {
    swap(buffer, buffer + 1);
  }

  short num;
  memcpy(&num, buffer, 2);
  return num;
}

//----------------------------------------------------------------------------
int vtkSegYIOUtils::readLongInteger(int pos, std::ifstream& in)
{
  in.seekg(pos, in.beg);
  char buffer[4];
  in.read(buffer, sizeof(buffer));

  if (!isBigEndian)
  {
    swap(buffer, buffer + 3);
    swap(buffer + 1, buffer + 2);
  }

  int num;
  memcpy(&num, buffer, 4);
  return num;
}

//----------------------------------------------------------------------------
int vtkSegYIOUtils::readLongInteger(std::ifstream& in)
{
  char buffer[4];
  in.read(buffer, sizeof(buffer));

  if (!isBigEndian)
  {
    swap(buffer, buffer + 3);
    swap(buffer + 1, buffer + 2);
  }

  int num;
  memcpy(&num, buffer, 4);
  return num;
}

//----------------------------------------------------------------------------
float vtkSegYIOUtils::readFloat(std::ifstream& in)
{
  char buffer[4];
  in.read(buffer, sizeof(buffer));

  if (!isBigEndian)
  {
    swap(buffer, buffer + 3);
    swap(buffer + 1, buffer + 2);
  }

  float num;
  memcpy(&num, buffer, 4);
  return num;
}

//----------------------------------------------------------------------------
float vtkSegYIOUtils::readIBMFloat(std::ifstream& in)
{
  char buffer[4];
  in.read(buffer, sizeof(buffer));

  if (!isBigEndian)
  {
    swap(buffer, buffer + 3);
    swap(buffer + 1, buffer + 2);
  }

  // The IBM Hex single precision floating point representation:
  //
  //  1      7                           24                    (width in bits)
  // +-+----------------+-----------------------------------------+
  // |S|   Exponent     |                Fraction                 |
  // +-+----------------+-----------------------------------------+
  // 31 30           24 23                                        0 (bit index)
  //
  //     Value = (-1^S) (0.F) (16^(E - 64))
  //
  // Key points:
  // - S = sign: 0 = Positive, 1 = Negative
  // - Exponent = power of 16 with a bias of 64
  // - Fraction = Normalized F portion of 24 bit fraction 0.F
  // - Value = 0 if F = 0
  // More details at
  // https://en.m.wikipedia.org/wiki/IBM_Floating_Point_Architecture

  long* longbuffer = reinterpret_cast<long*>(buffer);
  int sign = longbuffer[0] >> 31 & 0x01;
  int exponent = longbuffer[0] >> 24 & 0x7F;
  float fraction = (longbuffer[0] & 0x00ffffff) / float(pow(2.0, 24));
  if (fraction == 0.0f)
  {
    // Value is 0
    return 0.0f;
  }
  float num = (1 - 2 * sign) * fraction * pow(16.0, double(exponent - 64.0));
  return num;
}

//----------------------------------------------------------------------------
char vtkSegYIOUtils::readChar(std::ifstream& in)
{
  char buffer;
  in.read(&buffer, sizeof(buffer));
  return buffer;
}

//----------------------------------------------------------------------------
unsigned char vtkSegYIOUtils::readUChar(std::ifstream& in)
{
  char buffer;
  in.read(&buffer, sizeof(buffer));
  return buffer;
}

//----------------------------------------------------------------------------
void vtkSegYIOUtils::swap(char* a, char* b)
{
  char temp = *a;
  *a = *b;
  *b = temp;
}

//----------------------------------------------------------------------------
int vtkSegYIOUtils::getFileSize(std::ifstream& in)
{
  in.seekg(0, in.end);
  return in.tellg();
}
