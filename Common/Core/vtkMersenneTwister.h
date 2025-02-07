// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (C) 2001-2009 Makoto Matsumoto and Takuji Nishimura
// SPDX-FileCopyrightText: Copyright (C) 2009 Mutsuo Saito
// SPDX-License-Identifier: BSD-3-Clause AND BSD-2-Clause
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
 */

#ifndef vtkMersenneTwister_h
#define vtkMersenneTwister_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkRandomSequence.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkMersenneTwisterInternals;

class VTKCOMMONCORE_EXPORT vtkMersenneTwister : public vtkRandomSequence
{
public:
  typedef vtkTypeUInt32 SequenceId;

  ///@{
  /**
   * Standard methods for instantiation, type information, and printing.
   */
  static vtkMersenneTwister* New();
  vtkTypeMacro(vtkMersenneTwister, vtkRandomSequence);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Satisfy general API of vtkRandomSequence superclass. Initialize the
   * sequence with a seed.
   */
  void Initialize(vtkTypeUInt32 seed) override { this->InitializeSequence(0, seed); }

  /**
   * Initialize a new Mersenne Twister sequence, given a) a \c seed and b) a
   * Mersenne exponent (p s.t. 2^p-1 is a Mersenne prime). If \c p is not a
   * usable Mersenne exponent, its value is used to pick one from a list.
   * The return value is the id for the generated sequence, which is used as a
   * key to access values of the sequence.
   */
  SequenceId InitializeNewSequence(vtkTypeUInt32 seed, int p = 521);

  /**
   * Initialize a sequence as in InitializeNewSequence(), but additionally pass
   * an id to associate with the new sequence. If a sequence is already
   * associated with this id, a warning is given and the sequence is reset using
   * the given parameters.
   */
  void InitializeSequence(SequenceId id, vtkTypeUInt32 seed, int p = 521);

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
   * Move to the next number in random sequence \c id. If no sequence is
   * associated with this id, a warning is given and a sequence is generated
   * with default values.
   */
  virtual void Next(SequenceId id);

  /**
   * Move to the next number in random sequence <0>. If no sequence is
   * associated with this id, a warning is given and a sequence is generated
   * with default values.
   */
  void Next() override { this->Next(0); }

protected:
  vtkMersenneTwister();
  ~vtkMersenneTwister() override;

  vtkMersenneTwisterInternals* Internal;

private:
  vtkMersenneTwister(const vtkMersenneTwister&) = delete;
  void operator=(const vtkMersenneTwister&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // #ifndef vtkMersenneTwister_h
