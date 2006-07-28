/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationInternals.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkInformationInternals - internal structure for vtkInformation
// .SECTION Description
// vtkInformationInternals is used in internal implementation of
// vtkInformation. This should only be accessed by friends
// and sub-classes of that class.

#ifndef __vtkInformationInternals_h
#define __vtkInformationInternals_h

#include "vtkInformationKey.h"
#include "vtkObjectBase.h"

#include <assert.h>


// Note: assumes long is at least 32 bits.
enum { _stl_num_primes = 16 };
static const unsigned short _stl_prime_list[_stl_num_primes] =
{
  5u,          11u,         23u,        31u,        41u,
  53u,         97u,         193u,       389u,       769u,
  1543u,       3079u,       6151u,      12289u,     24593u,
  49157u
};

// use a mod hash or a bit hash
#define USE_MOD 1

//----------------------------------------------------------------------------
class vtkInformationInternals
{
public:
  // Vector to store ordered key/value pairs for efficient lookup with
  // a binary search.  Typically not many pairs are stored so linear
  // insertion time is okay.
  vtkInformationKey** Keys;
  vtkObjectBase** Values;
  unsigned short TableSize;
  unsigned short HashKey;
  
  vtkInformationInternals()
    {
      this->ComputeHashKey(33);
      this->Keys = new vtkInformationKey* [this->TableSize];
      this->Values = new vtkObjectBase* [this->TableSize];
      int i;
      for (i = 0; i < this->TableSize; ++i)
        {
        this->Keys[i] = 0;
        }
    }
  
  vtkInformationInternals(int size)
    {
      assert(size < 65000 && "information cannot grow to more than 65000 entries");
      this->ComputeHashKey(size);
      this->Keys = new vtkInformationKey* [this->TableSize];
      this->Values = new vtkObjectBase* [this->TableSize];
      int i;
      for (i = 0; i < this->TableSize; ++i)
        {
        this->Keys[i] = 0;
        }
    }

  ~vtkInformationInternals()
    {
      unsigned short i;
      for (i = 0; i < this->TableSize; ++i)
        {
        vtkObjectBase *value = this->Values[i];
        if (this->Keys[i] && value)
          {
          this->Keys[i] = 0;
          this->Values[i] = 0;
          value->UnRegister(0);
          }
        }
      delete [] this->Keys;
      delete [] this->Values;
    }

  void ComputeHashKey(int size)
    {
      // finds the best hash key for the target table size
      // and then adjust table size to fit the hash size
#if USE_MOD
      unsigned short i = 1;
      while(_stl_prime_list[i] + 1 <= size && i < _stl_num_primes)
        {
        i++;
        }
      this->HashKey = _stl_prime_list[i-1];
      this->TableSize = this->HashKey + 1;
#else
      this->HashKey = 1;
      while (this->HashKey + 1 <= size)
        {
        this->HashKey *= 2;
        }
      this->HashKey = this->HashKey/2-1;
      this->TableSize = this->HashKey + 2;
#endif      
    }

  unsigned short Hash(unsigned long hv)
    {
#if USE_MOD
      return hv % this->HashKey;
#else      
      return (hv >> 2 & this->HashKey);
#endif
    }
};

#endif
