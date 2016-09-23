/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageProgressIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageProgressIterator
 * @brief   a simple image iterator with progress
 *
 * This is a simple image iterator that can be used to iterate over an
 * image. Typically used to iterate over the output image
 *
 * @sa
 * vtkImageData vtkImageIterator
*/

#ifndef vtkImageProgressIterator_h
#define vtkImageProgressIterator_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkImageIterator.h"
class vtkAlgorithm;

template<class DType>
class VTKCOMMONEXECUTIONMODEL_EXPORT vtkImageProgressIterator : public vtkImageIterator<DType>
{
public:
  typedef vtkImageIterator<DType> Superclass;

  /**
   * Create a progress iterator for the provided image data
   * and extent to iterate over. The passes progress object will
   * receive any UpdateProgress calls if the thread id is zero
   */
  vtkImageProgressIterator(vtkImageData *imgd, int *ext,
                           vtkAlgorithm *po, int id);

  /**
   * Move the iterator to the next span, may call UpdateProgress on the
   * filter (vtkAlgorithm)
   */
  void NextSpan();

  /**
   * Overridden from vtkImageIterator to check AbortExecute on the
   * filter (vtkAlgorithm).
   */
  int IsAtEnd();

protected:
  vtkAlgorithm     *Algorithm;
  unsigned long     Count;
  unsigned long     Count2;
  unsigned long     Target;
  int               ID;
};

#ifndef vtkImageProgressIterator_cxx
vtkExternTemplateMacro(
  extern template class VTKCOMMONEXECUTIONMODEL_EXPORT vtkImageProgressIterator
)
#endif

#endif
// VTK-HeaderTest-Exclude: vtkImageProgressIterator.h
