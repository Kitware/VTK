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

// .NAME vtkTableFFT - FFT for table columns
//
// .SECTION Description
//
// vtkTableFFT performs the Fast Fourier Transform on the columns of a table.
// Internally, it shoves each column into an image data and then uses
// vtkImageFFT to perform the actual FFT.
//
// .SECTION See Also
//
// vtkImageFFT
//

#ifndef __vtkTableFFT_h
#define __vtkTableFFT_h

#include "vtkTableAlgorithm.h"
#include "vtkImagingFourierModule.h" // For export macro
#include "vtkSmartPointer.h"    // For internal method.

class VTKIMAGINGFOURIER_EXPORT vtkTableFFT : public vtkTableAlgorithm
{
public:
  vtkTypeMacro(vtkTableFFT, vtkTableAlgorithm);
  static vtkTableFFT *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);

protected:
  vtkTableFFT();
  ~vtkTableFFT();

  virtual int RequestData(vtkInformation *request,
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector);

//BTX
  // Description:
  // Perform the FFT on the given data array.
  virtual vtkSmartPointer<vtkDataArray> DoFFT(vtkDataArray *input);
//ETX

private:
  vtkTableFFT(const vtkTableFFT &);     // Not implemented
  void operator=(const vtkTableFFT &);  // Not implemented
};


#endif //__vtkTableFFT_h
