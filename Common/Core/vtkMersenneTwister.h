/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMersenneTwister.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

/*
  Copyright (C) 2001-2009 Makoto Matsumoto and Takuji Nishimura.
  Copyright (C) 2009 Mutsuo Saito
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/**
 * @class   vtkMersenneTwister
 * @brief   Generator for Mersenne Twister pseudorandom numbers
 *
 * vtkMersenneTwister is an implementation of the Mersenne Twister pseudorandom
 * number generator. The VTK class is simply a wrapper around an implementation
 * written by M. Matsumoto, T. Nishimura and M. Saito, whose source code can be
 * found at http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/DC/dc.html.
 *
 * This implementation of the Mersenne Twister facilitates the generation and
 * query from multiple independent pseudorandom sequences. Independent sequences
 * are identified by a unique vtkMersenneTwister::SequenceId, which is either
 * generated upon request or passed into the initialization method. This id is
 * factored into the initialization of the Mersenne Twister's initial state, so
 * two sequences with the same seed and different sequence ids will produce
 * different results. Once a sequence is initialized with an associated sequence
 * id, this id is used to obtain values from the sequence.
 *
 * This class, besides generating random sequences in sequential order, can
 * also populate a double array of specified size with a random sequence. It
 * will do so using one or more threads depending on the number of values
 * requested to generate.
 *
 * @warning
 * This class has been threaded with vtkMultiThreader. The amount of work
 * each thread performs is controlled by the #define VTK_MERSENNE_CHUNK.
 */

#ifndef vtkMersenneTwister_h
#define vtkMersenneTwister_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkRandomSequence.h"

class vtkMersenneTwisterInternals;

class VTKCOMMONCORE_EXPORT vtkMersenneTwister : public vtkRandomSequence
{
public:
  typedef vtkTypeUInt32 SequenceId;

  //@{
  /**
   * Standard methods for instantiation, type information, and printing.
   */
  static vtkMersenneTwister* New();
  vtkTypeMacro(vtkMersenneTwister,vtkRandomSequence);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  /**
   * Satisfy general API of vtkRandomSequence superclass. Initialize the
   * sequence with a seed.
   */
  void Initialize(vtkTypeUInt32 seed) override
  {this->InitializeSequence(0,seed);}

  /**
   * Initialize a new Mersenne Twister sequence, given a) a <seed> and b) a
   * Mersenne exponent (p s.t. 2^p-1 is a Mersenne prime). If <p> is not a
   * usable Mersenne exponent, its value is used to pick one from a list.
   * The return value is the id for the generated sequence, which is used as a
   * key to access values of the sequence.
   */
  SequenceId InitializeNewSequence(vtkTypeUInt32 seed, int p=521);

  /**
   * Initialize a sequence as in InitializeNewSequence(), but additionally pass
   * an id to associate with the new sequence. If a sequence is already
   * associated with this id, a warning is given and the sequence is reset using
   * the given parameters.
   */
  void InitializeSequence(SequenceId id, vtkTypeUInt32 seed, int p=521);

  /**
   * Current value
   * \post unit_range: result>=0.0 && result<=1.0
   */
  virtual double GetValue(SequenceId id);

  /**
   * Current value
   * \post unit_range: result>=0.0 && result<=1.0
   */
  double GetValue() override { return this->GetValue(0); }

  /**
   * Move to the next number in random sequence <id>. If no sequence is
   * associated with this id, a warning is given and a sequence is generated
   * with default values.
   */
  virtual void Next(SequenceId id);

  /**
   * Move to the next number in random sequence <0>. If no sequence is
   * associated with this id, a warning is given and a sequence is generated
   * with default values.
   */
  void Next() override { return this->Next(0); }

protected:
  vtkMersenneTwister();
  ~vtkMersenneTwister() override;

  vtkMersenneTwisterInternals* Internal;

private:
  vtkMersenneTwister(const vtkMersenneTwister&) = delete;
  void operator=(const vtkMersenneTwister&) = delete;
};

#endif // #ifndef vtkMersenneTwister_h
