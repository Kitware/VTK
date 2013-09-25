/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLargeInteger.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// code taken from
// Arbitrarily large numbers
//
// Author  Matthew Caryl
// Created 13.3.97

#include "vtkLargeInteger.h"

const unsigned int BIT_INCREMENT = 32;

static int maximum(int a, int b)
{
  return a > b ? a : b;
}

static int minimum(int a, int b)
{
  return a < b ? a : b;
}

void vtkLargeInteger::Contract()
{
  while (this->Number[this->Sig] == 0 && this->Sig > 0)
    {
    this->Sig--;
    }
}


vtkLargeInteger::vtkLargeInteger(void)
{
    this->Number = new char[BIT_INCREMENT];
    this->Number[0] = 0;
    this->Negative = 0;
    this->Max = BIT_INCREMENT - 1;
    this->Sig = 0;
}

#if defined(VTK_TYPE_USE_LONG_LONG)
vtkLargeInteger::vtkLargeInteger(long long n)
{
  this->Negative = n < 0 ? 1 : 0;
  n = n < 0 ? -n : n; // strip of sign
  this->Number = new char[BIT_INCREMENT];
  for (unsigned int i = 0; i < BIT_INCREMENT; i++)
    {
    this->Number[i] = n & 1;
    n >>= 1;
    }
  this->Max = BIT_INCREMENT - 1;
  this->Sig = BIT_INCREMENT - 1;
  this->Contract(); // remove leading 0s
}

vtkLargeInteger::vtkLargeInteger(unsigned long long n)
{
  this->Negative = 0;
  this->Number = new char[BIT_INCREMENT];
  for (unsigned int i = 0; i < BIT_INCREMENT; i++)
    {
    this->Number[i] = n & 1;
    n >>= 1;
    }
  this->Max = BIT_INCREMENT - 1;
  this->Sig = BIT_INCREMENT - 1;
  this->Contract(); // remove leading 0s
}
#endif

#if defined(VTK_TYPE_USE___INT64)
vtkLargeInteger::vtkLargeInteger(__int64 n)
{
  this->Negative = n < 0 ? 1 : 0;
  n = n < 0 ? -n : n; // strip of sign
  this->Number = new char[BIT_INCREMENT];
  for (unsigned int i = 0; i < BIT_INCREMENT; i++)
    {
    this->Number[i] = n & 1;
    n >>= 1;
    }
  this->Max = BIT_INCREMENT - 1;
  this->Sig = BIT_INCREMENT - 1;
  this->Contract(); // remove leading 0s
}

vtkLargeInteger::vtkLargeInteger(unsigned __int64 n)
{
  this->Negative = 0;
  this->Number = new char[BIT_INCREMENT];
  for (unsigned int i = 0; i < BIT_INCREMENT; i++)
    {
    this->Number[i] = n & 1;
    n >>= 1;
    }
  this->Max = BIT_INCREMENT - 1;
  this->Sig = BIT_INCREMENT - 1;
  this->Contract(); // remove leading 0s
}
#endif

vtkLargeInteger::vtkLargeInteger(long n)
{
  this->Negative = n < 0 ? 1 : 0;
  n = n < 0 ? -n : n; // strip of sign
  this->Number = new char[BIT_INCREMENT];
  for (unsigned int i = 0; i < BIT_INCREMENT; i++)
    {
    this->Number[i] = n & 1;
    n >>= 1;
    }
  this->Max = BIT_INCREMENT - 1;
  this->Sig = BIT_INCREMENT - 1;
  this->Contract(); // remove leading 0s
}

vtkLargeInteger::vtkLargeInteger(unsigned long n)
{
  this->Negative = 0;
  this->Number = new char[BIT_INCREMENT];
  for (unsigned int i = 0; i < BIT_INCREMENT; i++)
    {
    this->Number[i] = n & 1;
    n >>= 1;
    }
  this->Max = BIT_INCREMENT - 1;
  this->Sig = BIT_INCREMENT - 1;
  this->Contract(); // remove leading 0s
}

vtkLargeInteger::vtkLargeInteger(unsigned int n)
{
  this->Negative = 0;
  this->Number = new char[BIT_INCREMENT];
  for (unsigned int i = 0; i < BIT_INCREMENT; i++)
    {
    this->Number[i] = n & 1;
    n >>= 1;
    }
  this->Max = BIT_INCREMENT - 1;
  this->Sig = BIT_INCREMENT - 1;
  this->Contract(); // remove leading 0s
}

vtkLargeInteger::vtkLargeInteger(int n)
{
  this->Negative = n < 0 ? 1 : 0;
  n = n < 0 ? -n : n; // strip of sign
  this->Number = new char[BIT_INCREMENT];
  for (unsigned int i = 0; i < BIT_INCREMENT; i++)
    {
    this->Number[i] = n & 1;
    n >>= 1;
    }
  this->Max = BIT_INCREMENT - 1;
  this->Sig = BIT_INCREMENT - 1;
  this->Contract(); // remove leading 0s
}

vtkLargeInteger::vtkLargeInteger(const vtkLargeInteger& n)
{
  this->Number = new char[n.Max + 1];
  this->Negative = n.Negative;
  this->Max = n.Max;
  this->Sig = n.Sig;
  for (int i = this->Sig; i >= 0; i--)
    {
    this->Number[i] = n.Number[i];
    }
}

vtkLargeInteger::~vtkLargeInteger(void)
{
  delete []this->Number;
}

char vtkLargeInteger::CastToChar(void) const
{
  return this->CastToLong();
}

short vtkLargeInteger::CastToShort(void) const
{
  return this->CastToLong();
}

int vtkLargeInteger::CastToInt(void) const
{
  return this->CastToLong();
}

long vtkLargeInteger::CastToLong(void) const
{
  long n = 0;

  for (int i = this->Sig; i >= 0; i--)
    {
    n <<= 1;
    n |= this->Number[i];
    }
  if (this->Negative)
    {
    return -n;
    }

  return n;
}

// convert to an unsigned long, return max if bigger than unsigned long
unsigned long vtkLargeInteger::CastToUnsignedLong(void) const
{
  unsigned long n = 0;

  if (this->Sig >= (8*sizeof(unsigned long)))
    {
    for (int i = (8*sizeof(unsigned long)); i > 0; i--)
      {
      n <<= 1;
      n |= 1;
      }
    }
  else
    {
    for (int i = this->Sig; i >= 0; i--)
      {
      n <<= 1;
      n |= this->Number[i];
      }
    }

  return n;
}

int vtkLargeInteger::IsEven(void) const
{
  return this->Number[0] == 0;
}

int vtkLargeInteger::IsOdd(void) const
{
  return this->Number[0] == 1;
}

int vtkLargeInteger::GetLength(void) const
{
  return this->Sig + 1;
}

int vtkLargeInteger::GetBit(unsigned int p) const
{
  if (p <= this->Sig) // check if within current size
    {
    return this->Number[p];
    }

  return 0;
}

int vtkLargeInteger::IsZero(void) const
{
  return (this->Sig == 0 && this->Number[0] == 0);
}

int vtkLargeInteger::GetSign(void) const
{
  return this->Negative;
}

void vtkLargeInteger::Truncate(unsigned int n)
{
  if (n < 1) // either set to zero
    {
    this->Sig = 0;
    this->Number[0] = 0;
    this->Negative = 0;
    }
  else if (this->Sig > n - 1) // or chop down
    {
    this->Sig = n - 1;
    this->Contract(); // may have revealed leading zeros
    }
}

void vtkLargeInteger::Complement(void)
{
  if (!this->IsZero()) // can't have negative zeros
    {
    this->Negative = !this->Negative;
    }
}

int vtkLargeInteger::operator==(const vtkLargeInteger& n) const
{
  if (this->Sig != n.Sig) // check size
    {
    return 0;
    }

  if (this->Negative != n.Negative) // check sign
    {
    return 0;
    }

  for (int i = this->Sig; i >= 0; i--)
    {
    if (this->Number[i] != n.Number[i]) // check bits
      {
      return 0;
      }
    }

  return 1;
}

int vtkLargeInteger::operator!=(const vtkLargeInteger& n) const
{
  return !(*this == n);
}

int vtkLargeInteger::IsSmaller(const vtkLargeInteger& n) const
{
  if (this->Sig < n.Sig) // check size
    {
    return 1;
    }
  if (this->Sig > n.Sig) // check size
    {
    return 0;
    }

  for (int i = this->Sig; i >= 0; i--)
    {
    if (this->Number[i] < n.Number[i]) // check bits
      {
      return 1;
      }
    if (this->Number[i] > n.Number[i])
      {
      return 0;
      }
    }

  return 0;
}

int vtkLargeInteger::IsGreater(const vtkLargeInteger& n) const
{
  if (this->Sig > n.Sig) // check size
    {
    return 1;
    }
  if (this->Sig < n.Sig) // check sign
    {
    return 0;
    }

  for (int i = this->Sig; i >= 0; i--)
    {
    if (this->Number[i] > n.Number[i]) // check bits
      {
      return 1;
      }
    if (this->Number[i] < n.Number[i])
      {
      return 0;
      }
    }

  return 0;
}

int vtkLargeInteger::operator<(const vtkLargeInteger& n) const
{
    if (this->Negative & !n.Negative) // try to make judgement using signs
        return 1;
    else if (!this->Negative & n.Negative)
        return 0;
    else if (this->Negative)
        return !this->IsSmaller(n);
    else
        return this->IsSmaller(n);
}

int vtkLargeInteger::operator<=(const vtkLargeInteger& n) const
{
  return *this < n || *this == n;
}

int vtkLargeInteger::operator>(const vtkLargeInteger& n) const
{
  return !(*this <= n);
}

int vtkLargeInteger::operator>=(const vtkLargeInteger& n) const
{
  return !(*this < n);
}

void vtkLargeInteger::Expand(unsigned int n)
{
  if (n < this->Sig) // don't need to expand
    {
    return;
    }
  if (this->Max < n) // need to make a larger array
    {
    char* new_number = new char[n + 1];
    for (int i = this->Sig; i >= 0; i--)
      {
      new_number[i] = this->Number[i];
      }
    delete []this->Number;
    this->Number = new_number;
    this->Max = n;
    }
  // zero top of array
  for (unsigned int i = this->Sig + 1; i <= this->Max; i++)
    {
    this->Number[i] = 0;
    }
  this->Sig = n;
}

vtkLargeInteger& vtkLargeInteger::operator=(const vtkLargeInteger& n)
{
  if (this == &n) // same object
    {
    return *this;
    }
  this->Expand(n.Sig); // make equal sizes
  this->Sig = n.Sig; // might have been larger
  for (int i = this->Sig; i >= 0; i--)
    {
    this->Number[i] = n.Number[i];
    }
  this->Negative = n.Negative;
  return *this;
}

void vtkLargeInteger::Plus(const vtkLargeInteger& n)
{
  int m = maximum(this->Sig + 1, n.Sig + 1);
  this->Expand(m); // allow for overflow
  unsigned int i = 0;
  int carry = 0;
  for (; i <= n.Sig; i++) // add overlap
    {
    carry += this->Number[i] + n.Number[i];
    this->Number[i] = carry & 1;
    carry /= 2;
    }
  for (; carry != 0; i++) // continue with carry
    {
    carry += this->Number[i];
    this->Number[i] = carry & 1;
    carry /= 2;
    }
  this->Contract();
}

void vtkLargeInteger::Minus(const vtkLargeInteger& n)
{
  int m = maximum(this->Sig, n.Sig);
  this->Expand(m);
  unsigned int i = 0;
  int carry = 0;
  for (; i <= n.Sig; i++) // subtract overflow
    {
    carry += this->Number[i] - n.Number[i];
    this->Number[i] = (carry + 2) & 1;
    carry = carry < 0 ? -1 : 0;
    }
  for (; carry != 0; i++) // continue with carry
    {
    carry += this->Number[i];
    this->Number[i] = (carry + 2) & 1;
    carry = carry < 0 ? -1 : 0;
    }
  this->Contract();
}

vtkLargeInteger& vtkLargeInteger::operator+=(const vtkLargeInteger& n)
{
  if ((this->Negative ^ n.Negative) == 0) // cope with negatives
    {
    this->Plus(n);
    }
  else
    {
    if (this->IsSmaller(n))
      {
      vtkLargeInteger m(*this);
      *this = n;
      this->Minus(m);
      }
    else
      {
      this->Minus(n);
      }
    if (this->IsZero())
      {
      this->Negative = 0;
      }
    }
  return *this;
}

vtkLargeInteger& vtkLargeInteger::operator-=(const vtkLargeInteger& n)
{
  if ((this->Negative ^ n.Negative) == 1) // cope with negatives
    {
    this->Plus(n);
    }
  else
    {
    if (this->IsSmaller(n))
      {
      vtkLargeInteger m(*this);
      *this = n;
      this->Minus(m);
      this->Complement();
      }
    else
      {
      this->Minus(n);
      }
    if (this->IsZero())
      {
      this->Negative = 0;
      }
    }
  return *this;
}

vtkLargeInteger& vtkLargeInteger::operator<<=(int n)
{
  int i;

  if (n < 0) // avoid negatives
    {
    *this >>= -n;
    return *this;
    }
  this->Expand(this->Sig + n);

  // first shift
  for (i = this->Sig; i >= n; i--)
    {
    this->Number[i] = this->Number[i - n];
    }
  for (i = n - 1; i >= 0; i--) // then fill with 0s
    {
    this->Number[i] = 0;
    }
  this->Contract();
  return *this;
}

vtkLargeInteger& vtkLargeInteger::operator>>=(int n)
{
  if (n < 0) // avoid negatives
    {
    *this <<= -n;
    return *this;
    }

  // first shift the data
  unsigned int i;
  if (this->Sig >= static_cast<unsigned long>(n))
    {
    for (i = 0; i <= (this->Sig - n); i++)
      {
      this->Number[i] = this->Number[i + n];
      }
    }

  // then clear the other values to be safe
  int start = (this->Sig - n + 1);
  if (start < 0)
    {
    start = 0;
    }
  for (i = start; i <= this->Sig; i++)
    {
    this->Number[i] = 0;
    }

  start = start - 1;
  if (start < 0)
    {
    start = 0;
    }
  this->Sig = start;
  if (this->IsZero())
    {
    this->Negative = 0;
    }
  return *this;
}

vtkLargeInteger& vtkLargeInteger::operator++(void)
{
  return (*this += 1);
}

vtkLargeInteger& vtkLargeInteger::operator--(void)
{
  return (*this -= 1);
}

vtkLargeInteger vtkLargeInteger::operator++(int)
{
  vtkLargeInteger c = *this;
  *this += 1;
  return c;
}

vtkLargeInteger vtkLargeInteger::operator--(int)
{
  vtkLargeInteger c = *this;
  *this -= 1;
  return c;
}

vtkLargeInteger& vtkLargeInteger::operator*=(const vtkLargeInteger& n)
{
  vtkLargeInteger c;
  int m2 = this->Sig + n.Sig + 1;
  this->Expand(m2);
  if (n.IsSmaller(*this)) // loop through the smaller number
    {
    for (unsigned int i = 0; i <= n.Sig; i++)
      {
      if (n.Number[i] == 1)
        {
        c.Plus(*this); // add on multiples of two
        }
      *this <<= 1;
      }
    }
  else
    {
    vtkLargeInteger m = n;
    for (unsigned int i = 0; i <= this->Sig; i++)
      {
      if (this->Number[i] == 1)
        {
        c.Plus(m); // add on multiples of two
        }
      m <<= 1;
      }
    }
  if (c.IsZero()) // check negatives
    {
    c.Negative = 0;
    }
  else
    {
    c.Negative = this->Negative ^ n.Negative;
    }

  *this = c;
  this->Contract();
  return *this;
}

vtkLargeInteger& vtkLargeInteger::operator/=(const vtkLargeInteger& n)
{
  if (n.IsZero()) // no divide by zero
    {
    vtkGenericWarningMacro("Divide by zero!");
    return *this;
    }

  vtkLargeInteger c;
  vtkLargeInteger m = n;
  m <<= maximum(this->Sig - n.Sig, 0); // vtkpower of two multiple of n
  vtkLargeInteger i = 1;
  i = i << (this->Sig - n.Sig);
  for (; i > 0; i = i >> 1)
    {
    if (!m.IsGreater(*this))
      {
      this->Minus(m); // subtract of large chunk at time
      c += i;
      }
    m >>= 1; // shrink chunk down
    }
  if (c.IsZero()) // check negatives
    {
    c.Negative = 0;
    }
  else
    {
    c.Negative = this->Negative ^ n.Negative;
    }
  *this = c;
  return *this;
}

vtkLargeInteger& vtkLargeInteger::operator%=(const vtkLargeInteger& n)
{
  if (n.IsZero()) // no divide by zero
    {
    vtkGenericWarningMacro("Divide by zero!");
    return *this;
    }

  vtkLargeInteger m = n;
  m <<= maximum(this->Sig - n.Sig, 0); // power of two multiple of n
  for (int i = this->Sig - n.Sig; i >= 0; i--)
    {
    if (!m.IsGreater(*this))
      {
      this->Minus(m); // subtract of large chunk at time
      }
    m >>= 1; // shrink chunk down
    }
  if (this->IsZero())
    {
    this->Negative = 0;
    }
  return *this;
}

vtkLargeInteger& vtkLargeInteger::operator&=(const vtkLargeInteger& n)
{
  int m = maximum(this->Sig, n.Sig);
  this->Expand(m); // match sizes
  for (int i = minimum(this->Sig, n.Sig); i >= 0; i--)
    {
    this->Number[i] &= n.Number[i];
    }
  this->Contract();
  return *this;
}

vtkLargeInteger& vtkLargeInteger::operator|=(const vtkLargeInteger& n)
{
  int m = maximum(this->Sig, n.Sig);
  this->Expand(m); // match sizes
  for (int i = minimum(this->Sig, n.Sig); i >= 0; i--)
    {
    this->Number[i] |= n.Number[i];
    }
  this->Contract();
  return *this;
}

vtkLargeInteger& vtkLargeInteger::operator^=(const vtkLargeInteger& n)
{
  int m = maximum(this->Sig, n.Sig);
  this->Expand(m); // match sizes
  for (int i = minimum(this->Sig, n.Sig); i >= 0; i--)
    {
    this->Number[i] ^= n.Number[i];
    }
  this->Contract();
  return *this;
}

vtkLargeInteger vtkLargeInteger::operator+(const vtkLargeInteger& n) const
{
  vtkLargeInteger c = *this;
  c += n;
  return c;
}

vtkLargeInteger vtkLargeInteger::operator-(const vtkLargeInteger& n) const
{
  vtkLargeInteger c = *this;
  c -= n;
  return c;
}

vtkLargeInteger vtkLargeInteger::operator*(const vtkLargeInteger& n) const
{
  vtkLargeInteger c = *this;
  c *= n;
  return c;
}

vtkLargeInteger vtkLargeInteger::operator/(const vtkLargeInteger& n) const
{
  vtkLargeInteger c = *this;
  c /= n;
  return c;
}

vtkLargeInteger vtkLargeInteger::operator%(const vtkLargeInteger& n) const
{
  vtkLargeInteger c = *this;
  c %= n;
  return c;
}

vtkLargeInteger vtkLargeInteger::operator&(const vtkLargeInteger& n) const
{
  vtkLargeInteger c = *this;
  c &= n;
  return c;
}

vtkLargeInteger vtkLargeInteger::operator|(const vtkLargeInteger& n) const
{
  vtkLargeInteger c = *this;
  c |= n;
  return c;
}

vtkLargeInteger vtkLargeInteger::operator^(const vtkLargeInteger& n) const
{
  vtkLargeInteger c = *this;
  c ^= n;
  return c;
}

vtkLargeInteger vtkLargeInteger::operator<<(int n) const
{
  vtkLargeInteger c = *this;
  c <<= n;
  return c;
}

vtkLargeInteger vtkLargeInteger::operator>>(int n) const
{
  vtkLargeInteger c = *this;
  c >>= n;
  return c;
}

ostream& operator<<(ostream& s, const vtkLargeInteger& n)
{
  if (n.Negative)
    {
    s << '-';
    }
  for (int i = n.Sig; i >= 0; i--)
    {
    s << char(n.Number[i] + '0');
    }
  return s;
}

istream& operator>>(istream& s, vtkLargeInteger& n)
{
  char c=0;
  while (s.get(c)) // strip any leading spaces
    if (c != ' ' && c != '\n' && c != '\r')
      {
      s.putback(c);
      break;
      }
  n = 0;
  while (s.get(c)) // check for this->Negative
    {
    if (c != '-' && c != '+')
      {
      s.putback(c);
      break;
      }
    if (c == '-')
      n.Negative = !n.Negative;
    }
  while (s.get(c)) // build digits in reverse
    {
    if (c != '0' && c != '1')
      {
      s.putback(c);
      break;
      }
    if (n.Sig > n.Max) {
    n.Expand(n.Sig + BIT_INCREMENT);
    n.Sig -= BIT_INCREMENT;
    }
    n.Number[n.Sig++] = c - '0';
    }
  if (n.Sig > 0) // put digits right way round
    {
    n.Sig--;
    for (unsigned int j = n.Sig; j > n.Sig / 2; j--)
      {
      c = n.Number[j];
      n.Number[j] = n.Number[n.Sig - j];
      n.Number[n.Sig - j] = c;
      }
    n.Contract();
    }
  return s;
}
