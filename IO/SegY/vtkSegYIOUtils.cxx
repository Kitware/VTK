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

#include <cstring>

/*
 * Downloaded from:
 * https://www.thecodingforums.com/threads/c-code-for-converting-ibm-370-floating-point-to-ieee-754.438469/
 */
/* ibm2ieee - Converts a number from IBM 370 single precision floating
point format to IEEE 754 single precision format. For normalized
numbers, the IBM format has greater range but less precision than the
IEEE format. Numbers within the overlapping range are converted
exactly. Numbers which are too large are converted to IEEE Infinity
with the correct sign. Numbers which are too small are converted to
IEEE denormalized numbers with a potential loss of precision (including
complete loss of precision which results in zero with the correct
sign). When precision is lost, rounding is toward zero (because it's
fast and easy -- if someone really wants round to nearest it shouldn't
be TOO difficult). */

#include <netinet/in.h>
#include <sys/types.h>

//----------------------------------------------------------------------------
void ibm2ieee(void* to, const void* from, int len)
{
  unsigned long fr; /* fraction */
  int exp;          /* exponent */
  int sgn;          /* sign */

  for (; len-- > 0; to = (char*)to + 4, from = (char*)from + 4)
  {
    /* split into sign, exponent, and fraction */
    fr = *(long*)from; /* pick up value */
    sgn = fr >> 31;    /* save sign */
    fr <<= 1;          /* shift sign out */
    exp = fr >> 25;    /* save exponent */
    fr <<= 7;          /* shift exponent out */

    if (fr == 0)
    { /* short-circuit for zero */
      exp = 0;
      goto done;
    }

    /* adjust exponent from base 16 offset 64 radix point before first digit
       to base 2 offset 127 radix point after first digit */
    /* (exp - 64) * 4 + 127 - 1 == exp * 4 - 256 + 126 == (exp << 2) - 130 */
    exp = (exp << 2) - 130;

    /* (re)normalize */
    while (fr < 0x80000000)
    { /* 3 times max for normalized input */
      --exp;
      fr <<= 1;
    }

    if (exp <= 0)
    {                /* underflow */
      if (exp < -24) /* complete underflow - return properly signed zero */
        fr = 0;
      else /* partial underflow - return denormalized number */
        fr >>= -exp;
      exp = 0;
    }
    else if (exp >= 255)
    { /* overflow - return infinity */
      fr = 0;
      exp = 255;
    }
    else
    { /* just a plain old number - remove the assumed high bit */
      fr <<= 1;
    }

  done:
    /* put the pieces back together and return it */
    *(unsigned*)to = (fr >> 9) | (exp << 23) | (sgn << 31);
  }
}

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

  float num;
  ibm2ieee(&num, buffer, 1);
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
