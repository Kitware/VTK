/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContourFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkSMPContourFilterManyPieces_h
#define vtkSMPContourFilterManyPieces_h

#include "vtkContourFilter.h"

class VTK_EXPORT vtkSMPContourFilterManyPieces : public vtkContourFilter
{
public:
  vtkTypeMacro(vtkSMPContourFilterManyPieces,vtkContourFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with initial range (0,1) and single contour value
  // of 0.0.
  static vtkSMPContourFilterManyPieces *New();

protected:
  vtkSMPContourFilterManyPieces();
  ~vtkSMPContourFilterManyPieces();

  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);
  virtual int FillOutputPortInformation(int port, vtkInformation *info);

private:
  vtkSMPContourFilterManyPieces(const vtkSMPContourFilterManyPieces&);  // Not implemented.
  void operator=(const vtkSMPContourFilterManyPieces&);  // Not implemented.
};

#endif
