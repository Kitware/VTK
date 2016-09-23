/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLargeInteger.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkLargeInteger
 * @brief   class for arbitrarily large ints
*/

#ifndef vtkLargeInteger_h
#define vtkLargeInteger_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"

class VTKCOMMONCORE_EXPORT vtkLargeInteger
{
public:
  vtkLargeInteger(void);
  vtkLargeInteger(long n);
  vtkLargeInteger(unsigned long n);
  vtkLargeInteger(int n);
  vtkLargeInteger(unsigned int n);
  vtkLargeInteger(const vtkLargeInteger& n);
  vtkLargeInteger(long long n);
  vtkLargeInteger(unsigned long long n);

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

  bool operator==(const vtkLargeInteger& n) const;
  bool operator!=(const vtkLargeInteger& n) const;
  bool operator<(const vtkLargeInteger& n) const;
  bool operator<=(const vtkLargeInteger& n) const;
  bool operator>(const vtkLargeInteger& n) const;
  bool operator>=(const vtkLargeInteger& n) const;

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
  bool IsSmaller(const vtkLargeInteger& n) const; // unsigned
  bool IsGreater(const vtkLargeInteger& n) const; // unsigned
  void Expand(unsigned int n); // ensure n'th bit exits
  void Contract(); // remove leading 0s
  void Plus(const vtkLargeInteger& n); // unsigned
  void Minus(const vtkLargeInteger& n); // unsigned
};

#endif


// VTK-HeaderTest-Exclude: vtkLargeInteger.h
