// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLargeInteger
 * @brief   class for arbitrarily large ints
 */

#ifndef vtkLargeInteger_h
#define vtkLargeInteger_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT VTK_WRAPEXCLUDE vtkLargeInteger
{
public:
  vtkLargeInteger();
  vtkLargeInteger(long n);
  vtkLargeInteger(unsigned long n);
  vtkLargeInteger(int n);
  vtkLargeInteger(unsigned int n);
  vtkLargeInteger(const vtkLargeInteger& n);
  vtkLargeInteger(long long n);
  vtkLargeInteger(unsigned long long n);

  ~vtkLargeInteger();

  char CastToChar() const;
  short CastToShort() const;
  int CastToInt() const;
  long CastToLong() const;
  unsigned long CastToUnsignedLong() const;

  int IsEven() const;
  int IsOdd() const;
  int GetLength() const;            // in bits
  int GetBit(unsigned int p) const; // p'th bit (from zero)
  int IsZero() const;               // is zero
  int GetSign() const;              // is negative

  void Truncate(unsigned int n); // reduce to lower n bits
  void Complement();             // * -1

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
  vtkLargeInteger& operator++();
  vtkLargeInteger& operator--();
  vtkLargeInteger operator++(int);
  vtkLargeInteger operator--(int);
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
  void Expand(unsigned int n);                    // ensure n'th bit exits
  void Contract();                                // remove leading 0s
  void Plus(const vtkLargeInteger& n);            // unsigned
  void Minus(const vtkLargeInteger& n);           // unsigned
};

VTK_ABI_NAMESPACE_END
#endif

// VTK-HeaderTest-Exclude: vtkLargeInteger.h
