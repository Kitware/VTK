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

// .NAME vtkRectilinearGridToPointSet - Converts a vtkRectilinearGrid to a vtkPointSet
//
// .SECTION Description
// vtkRectilinearGridToPointSet takes a vtkRectilinearGrid as an image and
// outputs an equivalent vtkStructuredGrid (which is a subclass of
// vtkPointSet).
//
// .SECTION Thanks
// This class was developed by Kenneth Moreland (kmorel@sandia.gov) from
// Sandia National Laboratories.

#ifndef __vtkRectilinearGridToPointSet_h
#define __vtkRectilinearGridToPointSet_h

#include "vtkStructuredGridAlgorithm.h"

class vtkRectilinearGrid;
class vtkStructuredData;

class VTK_GRAPHICS_EXPORT vtkRectilinearGridToPointSet : public vtkStructuredGridAlgorithm
{
public:
  vtkTypeMacro(vtkRectilinearGridToPointSet, vtkStructuredGridAlgorithm);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  static vtkRectilinearGridToPointSet *New();

protected:
  vtkRectilinearGridToPointSet();
  ~vtkRectilinearGridToPointSet();

  int RequestData(vtkInformation *request,
                  vtkInformationVector **inputVector,
                  vtkInformationVector *outputVector);

  virtual int FillInputPortInformation(int port, vtkInformation *info);

private:
  vtkRectilinearGridToPointSet(const vtkRectilinearGridToPointSet &); // Not implemented
  void operator=(const vtkRectilinearGridToPointSet &);         // Not implemented

  int CopyStructure(vtkStructuredGrid *outData, vtkRectilinearGrid *inData);
};


#endif //__vtkRectilinearGridToPointSet_h
