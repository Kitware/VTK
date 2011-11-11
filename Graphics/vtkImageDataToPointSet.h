/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRectilinearGridToTetrahedra.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

// .NAME vtkImageDataToPointSet - Converts a vtkImageData to a vtkPointSet
//
// .SECTION Description
// vtkImageDataToPointSet takes a vtkImageData as an image and outputs an
// equivalent vtkStructuredGrid (which is a subclass of vtkPointSet).
//
// .SECTION Thanks
// This class was developed by Kenneth Moreland (kmorel@sandia.gov) from
// Sandia National Laboratories.

#ifndef __vtkImageDataToPointSet_h
#define __vtkImageDataToPointSet_h

#include "vtkStructuredGridAlgorithm.h"

class vtkImageData;
class vtkStructuredData;

class VTK_GRAPHICS_EXPORT vtkImageDataToPointSet : public vtkStructuredGridAlgorithm
{
public:
  vtkTypeMacro(vtkImageDataToPointSet, vtkStructuredGridAlgorithm);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  static vtkImageDataToPointSet *New();

protected:
  vtkImageDataToPointSet();
  ~vtkImageDataToPointSet();

  int RequestData(vtkInformation *request,
                  vtkInformationVector **inputVector,
                  vtkInformationVector *outputVector);

  virtual int FillInputPortInformation(int port, vtkInformation *info);

private:
  vtkImageDataToPointSet(const vtkImageDataToPointSet &); // Not implemented
  void operator=(const vtkImageDataToPointSet &);         // Not implemented

  int CopyStructure(vtkStructuredGrid *outData, vtkImageData *inData);
};


#endif //__vtkImageDataToPointSet_h
