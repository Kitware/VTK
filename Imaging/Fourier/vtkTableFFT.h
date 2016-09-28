// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTableFFT.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

/**
 * @class   vtkTableFFT
 * @brief   FFT for table columns
 *
 *
 *
 * vtkTableFFT performs the Fast Fourier Transform on the columns of a table.
 * Internally, it shoves each column into an image data and then uses
 * vtkImageFFT to perform the actual FFT.
 *
 *
 * @sa
 * vtkImageFFT
 *
*/

#ifndef vtkTableFFT_h
#define vtkTableFFT_h

#include "vtkTableAlgorithm.h"
#include "vtkImagingFourierModule.h" // For export macro
#include "vtkSmartPointer.h"    // For internal method.

class VTKIMAGINGFOURIER_EXPORT vtkTableFFT : public vtkTableAlgorithm
{
public:
  vtkTypeMacro(vtkTableFFT, vtkTableAlgorithm);
  static vtkTableFFT *New();
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkTableFFT();
  ~vtkTableFFT() VTK_OVERRIDE;

  int RequestData(vtkInformation *request,
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector) VTK_OVERRIDE;

  /**
   * Perform the FFT on the given data array.
   */
  virtual vtkSmartPointer<vtkDataArray> DoFFT(vtkDataArray *input);

private:
  vtkTableFFT(const vtkTableFFT &) VTK_DELETE_FUNCTION;
  void operator=(const vtkTableFFT &) VTK_DELETE_FUNCTION;
};


#endif //vtkTableFFT_h
