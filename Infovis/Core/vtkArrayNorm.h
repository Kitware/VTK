/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayNorm.h

-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkArrayNorm
 * @brief   Computes L-norms along one dimension of an array.
 *
 *
 * Given an input matrix (vtkTypedArray<double>), computes the L-norm for each
 * vector along either dimension, storing the results in a dense output
 * vector (1D vtkDenseArray<double>).  The caller may optionally request the
 * inverse norm as output (useful for subsequent normalization), and may limit
 * the computation to a "window" of vector elements, to avoid data copying.
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.
*/

#ifndef vtkArrayNorm_h
#define vtkArrayNorm_h

#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkArrayDataAlgorithm.h"
#include "vtkArrayRange.h"

class VTKINFOVISCORE_EXPORT vtkArrayNorm : public vtkArrayDataAlgorithm
{
public:
  static vtkArrayNorm* New();
  vtkTypeMacro(vtkArrayNorm, vtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Controls the dimension along which norms will be computed.  For input matrices,
   * For input matrices, use "0" (rows) or "1" (columns). Default: 0
   */
  vtkGetMacro(Dimension, int);
  vtkSetMacro(Dimension, int);
  //@}

  //@{
  /**
   * Controls the L-value.  Default: 2
   */
  vtkGetMacro(L, int);
  void SetL(int value);
  //@}

  //@{
  /**
   * Controls whether to invert output values.  Default: false
   */
  vtkSetMacro(Invert, int);
  vtkGetMacro(Invert, int);
  //@}

  //@{
  /**
   * Defines an optional "window" used to compute the norm on a subset of the elements
   * in a vector.
   */
  void SetWindow(const vtkArrayRange& window);
  vtkArrayRange GetWindow();
  //@}

protected:
  vtkArrayNorm();
  ~vtkArrayNorm() VTK_OVERRIDE;

  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*) VTK_OVERRIDE;

private:
  vtkArrayNorm(const vtkArrayNorm&) VTK_DELETE_FUNCTION;
  void operator=(const vtkArrayNorm&) VTK_DELETE_FUNCTION;

  int Dimension;
  int L;
  int Invert;
  vtkArrayRange Window;

};

#endif

// VTK-HeaderTest-Exclude: vtkArrayNorm.h
