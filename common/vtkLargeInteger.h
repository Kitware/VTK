/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLargeInteger.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkLargeInteger - class for arbitrarily large ints

#ifndef __vtkLargeInteger_h
#define __vtkLargeInteger_h

#include "vtkObject.h"

class VTK_EXPORT vtkLargeInteger 
{
public:
  vtkLargeInteger(void);
  vtkLargeInteger(long n);
  vtkLargeInteger(unsigned long n);
  vtkLargeInteger(int n);
  vtkLargeInteger(unsigned int n);
  vtkLargeInteger(const vtkLargeInteger& n);
  ~vtkLargeInteger(void);
  
  char CastToChar(void) const;
  short CastToShort(void) const;
  int CastToInt(void) const;
  long CastToLong(void) const;
  unsigned long CastToUnsignedLong(void) const;
  
  int IsEven(void) const;
  int IsOdd(void) const;
  int GetLength(void) const; // in bits
  int GetBit(unsigned int p) const; // p'th bit (from zero)
  int IsZero() const; // is zero
  int GetSign(void) const; // is negative
  
  void Truncate(unsigned int n); // reduce to lower n bits
  void Complement(void); // * -1
  
  int operator==(const vtkLargeInteger& n) const;
  int operator!=(const vtkLargeInteger& n) const;
  int operator<(const vtkLargeInteger& n) const;
  int operator<=(const vtkLargeInteger& n) const;
  int operator>(const vtkLargeInteger& n) const;
  int operator>=(const vtkLargeInteger& n) const;
  
  vtkLargeInteger& operator=(const vtkLargeInteger& n);
  vtkLargeInteger& operator+=(const vtkLargeInteger& n);
  vtkLargeInteger& operator-=(const vtkLargeInteger& n);
  vtkLargeInteger& operator<<=(int n);
  vtkLargeInteger& operator>>=(int n);
  vtkLargeInteger& operator++(void);
  vtkLargeInteger& operator--(void);
  vtkLargeInteger  operator++(int);
  vtkLargeInteger  operator--(int);
  vtkLargeInteger& operator*=(const vtkLargeInteger& n);
  vtkLargeInteger& operator/=(const vtkLargeInteger& n);
  vtkLargeInteger& operator%=(const vtkLargeInteger& n);
  // no change of sign for following operators
  vtkLargeInteger& operator&=(const vtkLargeInteger& n);
  vtkLargeInteger& operator|=(const vtkLargeInteger& n);
  vtkLargeInteger& operator^=(const vtkLargeInteger& n);
  
  vtkLargeInteger operator+(const vtkLargeInteger& n) const;
  vtkLargeInteger operator-(const vtkLargeInteger& n) const;
  vtkLargeInteger operator*(const vtkLargeInteger& n) const;
  vtkLargeInteger operator/(const vtkLargeInteger& n) const;
  vtkLargeInteger operator%(const vtkLargeInteger& n) const;
  // no change of sign for following operators
  vtkLargeInteger operator&(const vtkLargeInteger& n) const;
  vtkLargeInteger operator|(const vtkLargeInteger& n) const;
  vtkLargeInteger operator^(const vtkLargeInteger& n) const;
  vtkLargeInteger operator<<(int n) const;
  vtkLargeInteger operator>>(int n) const;
  
  friend ostream& operator<<(ostream& s, const vtkLargeInteger& n);
  friend istream& operator>>(istream& s, vtkLargeInteger& n);
  
private:
  char* Number;
  int Negative;
  unsigned int Sig;
  unsigned int Max;
  
  // unsigned operators
  int IsSmaller(const vtkLargeInteger& n) const; // unsigned
  int IsGreater(const vtkLargeInteger& n) const; // unsigned
  void Expand(unsigned int n); // ensure n'th bit exits
  void Contract(); // remove leading 0s
  void Plus(const vtkLargeInteger& n); // unsigned
  void Minus(const vtkLargeInteger& n); // unsigned
};

#endif


