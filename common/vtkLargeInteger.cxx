/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLargeInteger.cxx
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

// code taken from
// Arbitrarily large numbers
//
// Author  Matthew Caryl
// Created 13.3.97

#include "vtkLargeInteger.h"

const unsigned int BIT_INCREMENT = 32;

int maximum(int a, int b)
{
    return a > b ? a : b;
}

int minimum(int a, int b)
{
    return a < b ? a : b;
}

long vtkpow(long a, long b)
{
    long a1 = a;
    long b1 = b;
    long c = 1;
    
    while (b1 >= 1)
        {
        while (b & 1 == 0)
            {
            b1 = b1 / 2;
            a1 = a1 * a1;
            }
        b1 = b1 - 1;
        c = c * a1;
        }
    return c;
}

void vtkLargeInteger::contract()
{
    while (number[sig] == 0 && sig > 0)
        sig--;
}

vtkLargeInteger::vtkLargeInteger(void)
{
    number = new char[BIT_INCREMENT];
    number[0] = 0;
    negative = false;
    max = BIT_INCREMENT - 1;
    sig = 0;
}

vtkLargeInteger::vtkLargeInteger(long n)
{
    negative = n < 0 ? true : false;
    n = n < 0 ? -n : n; // strip of sign
    number = new char[BIT_INCREMENT];
    for (int i = 0; i < BIT_INCREMENT; i++)
        {
        number[i] = n & 1;
        n >>= 1;
        }
    max = BIT_INCREMENT - 1;
    sig = BIT_INCREMENT - 1;
    contract(); // remove leading 0s
}

vtkLargeInteger::vtkLargeInteger(const vtkLargeInteger& n)
{
    number = new char[n.max + 1];
    negative = n.negative;
    max = n.max;
    sig = n.sig;
    for (int i = sig; i >= 0; i--)
        number[i] = n.number[i];
}

vtkLargeInteger::~vtkLargeInteger(void)
{
    delete number;
}

char vtkLargeInteger::to_char(void) const
{
    return to_long();
}

short vtkLargeInteger::to_short(void) const
{
    return to_long();
}

int vtkLargeInteger::to_int(void) const
{
    return to_long();
}

long vtkLargeInteger::to_long(void) const
{
  if (length() > 31)
    {
    vtkGenericWarningMacro("overflow in converting large integer to long");
    }
  long n = 0;

    for (int i = sig; i >= 0; i--)
        {
        n <<= 1;
        n |= number[i];
        }
    if (negative)
        return -n;
    else
        return n;
}

int vtkLargeInteger::even(void) const
{
    return number[0] == 0;
}

int vtkLargeInteger::odd(void) const
{
    return number[0] == 1;
}

int vtkLargeInteger::length(void) const
{
    return sig + 1;
}

int vtkLargeInteger::bit(unsigned int p) const
{
    if (p <= sig) // check if within current size
        return number[p];
    else
        return false;
}

int vtkLargeInteger::zero(void) const
{
    return (sig == 0 && number[0] == 0);
}

int vtkLargeInteger::sign(void) const
{
    return negative;
}

void vtkLargeInteger::truncate(unsigned int n)
{
    if (n < 1) // either set to zero
        {
        sig = 0;
        number[0] = 0;
        negative = false;
        }
    else if (sig > n - 1) // or chop down
        {
        sig = n - 1;
        contract(); // may have revealed leading zeros
        }
}

void vtkLargeInteger::complement(void)
{
    if (!zero()) // can't have negative zeros
        negative = !negative;
}

int vtkLargeInteger::operator==(const vtkLargeInteger& n) const
{
    if (sig != n.sig) // check size
        return false;
    if (negative != n.negative) // check sign
        return false;
    for (int i = sig; i >= 0; i--)
        if (number[i] != n.number[i]) // check bits
            return false;
    return true;
}

int vtkLargeInteger::operator!=(const vtkLargeInteger& n) const
{
    return !(*this == n);
}

int vtkLargeInteger::smaller(const vtkLargeInteger& n) const
{
    if (sig < n.sig) // check size
        return true;
    else if (sig > n.sig) // check sign
        return false;
    for (int i = sig; i >= 0; i--)
        if (number[i] < n.number[i]) // check bits
            return true;
        else if (number[i] > n.number[i])
            return false;
    return false;
}

int vtkLargeInteger::greater(const vtkLargeInteger& n) const
{
    if (sig > n.sig) // check size
        return true;
    else if (sig < n.sig) // check sign
        return false;
    for (int i = sig; i >= 0; i--)
        if (number[i] > n.number[i]) // check bits
            return true;
        else if (number[i] < n.number[i])
            return false;
    return false;
}

int vtkLargeInteger::operator<(const vtkLargeInteger& n) const
{
    if (negative & !n.negative) // try to make judgement using signs
        return true;
    else if (!negative & n.negative)
        return false;
    else if (negative)
        return !smaller(n);
    else
        return smaller(n);
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
    
void vtkLargeInteger::expand(unsigned int n)
{
    if (n < sig) // don't need to expand
        return;
    if (max < n) // need to make a larger array
        {
        char* new_number = new char[n + 1];
        for (int i = sig; i >= 0; i--)
            new_number[i] = number[i];
        delete number;
        number = new_number;
        max = n;
        }
    for (unsigned int i = sig + 1; i <= max; i++) // zero top of array
      number[i] = 0;
    sig = n;
}

vtkLargeInteger& vtkLargeInteger::operator=(const vtkLargeInteger& n)
{
    if (this == &n) // same object
        return *this;
    expand(n.sig); // make equal sizes
    sig = n.sig; // might have been larger
    for (int i = sig; i >= 0; i--)
        number[i] = n.number[i];
    negative = n.negative;
    return *this;
}

void vtkLargeInteger::plus(const vtkLargeInteger& n)
{
    int m = maximum(sig + 1, n.sig + 1);
    expand(m); // allow for overflow
    unsigned int i = 0;
    int carry = 0;
    for (; i <= n.sig; i++) // add overlap
        {
        carry += number[i] + n.number[i];
        number[i] = carry & 1;
        carry /= 2;
        }
    for (; carry != 0; i++) // continue with carry
        {
        carry += number[i];
        number[i] = carry & 1;
        carry /= 2;
        }
    contract();
}

void vtkLargeInteger::minus(const vtkLargeInteger& n)
{
    int m = maximum(sig, n.sig);
    expand(m);
    unsigned int i = 0;
    int carry = 0;
    for (; i <= n.sig; i++) // subtract overflow
        {
        carry += number[i] - n.number[i];
        number[i] = (carry + 2) & 1;
        carry = carry < 0 ? -1 : 0;
        }
    for (; carry != 0; i++) // continue with carry
        {
        carry += number[i];
        number[i] = (carry + 2) & 1;
        carry = carry < 0 ? -1 : 0;
        }
    contract();
}

vtkLargeInteger& vtkLargeInteger::operator+=(const vtkLargeInteger& n)
{
    if ((negative ^ n.negative) == false) // cope with negatives
        this->plus(n);
    else
        {
        if (smaller(n))
            {
            vtkLargeInteger m(*this);
            *this = n;
            minus(m);
            }
        else
            this->minus(n);
        if (zero())
            negative = false;
        }
    return *this;
}

vtkLargeInteger& vtkLargeInteger::operator-=(const vtkLargeInteger& n)
{
    if ((negative ^ n.negative) == true) // cope with negatives
        this->plus(n);
    else
        {
        if (smaller(n))
            {
            vtkLargeInteger m(*this);
            *this = n;
            minus(m);
            complement();
            }
        else
            this->minus(n);
        if (zero())
            negative = false;
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
  expand(sig + n);
  for (i = sig; i >= n; i--) // copy
    number[i] = number[i - n];
  for (i = n - 1; i >= 0; i--) // then fill with 0s
    number[i] = 0;
  contract();
  return *this;
}

vtkLargeInteger& vtkLargeInteger::operator>>=(int n)
{
    if (n < 0) // avoid negatives
        {
        *this <<= -n;
        return *this;
        }
    // why can't I use - for (int i = 0; i <= sig - n; i++)
    for (unsigned int i = 0; i <= sig; i++) // copy
        number[i] = number[i + n];
    sig = sig - n; // shorten
    if (sig < 0)
        {
        sig = 0;
        number[0] = 0;
        }
    if (zero())
        negative = false;
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
    int m = sig + n.sig;
    expand(m);
    if (n.smaller(*this)) // loop through the smaller number
        {
        for (unsigned int i = 0; i <= n.sig; i++)
            {
            if (n.number[i] == 1)
                c.plus(*this); // add on multiples of two
            *this <<= 1;
            }
        }
    else
        {
        vtkLargeInteger m = n;
        for (unsigned int i = 0; i <= sig; i++)
            {
            if (number[i] == 1)
                c.plus(m); // add on multiples of two
            m <<= 1;
            }
        }
    if (c.zero()) // check negatives
        c.negative = false;
    else
        c.negative = negative ^ n.negative;
    *this = c;
    contract();
    return *this;
}

vtkLargeInteger& vtkLargeInteger::operator/=(const vtkLargeInteger& n)
{
    if (n.zero()) // no divide by zero
        throw;
    vtkLargeInteger c;
    vtkLargeInteger m = n;
    m <<= maximum(sig - n.sig, 0); // vtkpower of two multiple of n
    for (int i = vtkpow(2, sig - n.sig); i > 0; i /= 2)
        {
        if (!m.greater(*this))
            {
            minus(m); // subtract of large chunk at time
            c += i;
            }
        m >>= 1; // shrink chunk down
        }
    if (c.zero()) // check negatives
        c.negative = false;
    else
        c.negative = negative ^ n.negative;
    *this = c;
    return *this;
}

vtkLargeInteger& vtkLargeInteger::operator%=(const vtkLargeInteger& n)
{
    if (n.zero()) // no divide by zero
        throw;
    vtkLargeInteger m = n;
    m <<= maximum(sig - n.sig, 0); // power of two multiple of n
    for (int i = sig - n.sig; i >= 0; i--)
        {
        if (!m.greater(*this))
            minus(m); // subtract of large chunk at time
        m >>= 1; // shrink chunk down
        }
    if (zero())
        negative = false;
    return *this;
}

vtkLargeInteger& vtkLargeInteger::operator&=(const vtkLargeInteger& n)
{
    int m = maximum(sig, n.sig);
    expand(m); // match sizes
    for (int i = minimum(sig, n.sig); i >= 0; i--)
        number[i] &= n.number[i];
    contract();
    return *this;
}

vtkLargeInteger& vtkLargeInteger::operator|=(const vtkLargeInteger& n)
{
    int m = maximum(sig, n.sig);
    expand(m); // match sizes
    for (int i = minimum(sig, n.sig); i >= 0; i--)
        number[i] |= n.number[i];
    contract();
    return *this;
}

vtkLargeInteger& vtkLargeInteger::operator^=(const vtkLargeInteger& n)
{
    int m = maximum(sig, n.sig);
    expand(m); // match sizes
    for (int i = minimum(sig, n.sig); i >= 0; i--)
        number[i] ^= n.number[i];
    contract();
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
    if (n.negative)
        s << '-';
    for (int i = n.sig; i >= 0; i--)
        s << char(n.number[i] + '0');
    return s;
}

istream& operator>>(istream& s, vtkLargeInteger& n)
{
    char c;
    while (s.get(c)) // strip any leading spaces
        if (c != ' ' && c != '\n' && c != '\r')
            {
            s.putback(c);
            break;
            }
    n = 0;
    while (s.get(c)) // check for negative
        {
        if (c != '-' && c != '+')
            {
            s.putback(c);
            break;
            }
        if (c == '-')
            n.negative = !n.negative;
        }
    while (s.get(c)) // build digits in reverse
        {
        if (c != '0' && c != '1')
            {
            s.putback(c);
            break;
            }
        if (n.sig > n.max) {
            n.expand(n.sig + BIT_INCREMENT);
            n.sig -= BIT_INCREMENT;
            }
        n.number[n.sig++] = c - '0';
        }
    if (n.sig > 0) // put digits right way round
        {
        n.sig--;
        for (unsigned int j = n.sig; j > n.sig / 2; j--)
            {
            c = n.number[j];
            n.number[j] = n.number[n.sig - j];
            n.number[n.sig - j] = c;
            }
        n.contract();
        }
    return s;
}
