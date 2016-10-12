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

/**
 * @class   vtkImageDataToPointSet
 * @brief   Converts a vtkImageData to a vtkPointSet
 *
 *
 * vtkImageDataToPointSet takes a vtkImageData as an image and outputs an
 * equivalent vtkStructuredGrid (which is a subclass of vtkPointSet).
 *
 * @par Thanks:
 * This class was developed by Kenneth Moreland (kmorel@sandia.gov) from
 * Sandia National Laboratories.
*/

#ifndef vtkImageDataToPointSet_h
#define vtkImageDataToPointSet_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkStructuredGridAlgorithm.h"

class vtkImageData;
class vtkStructuredData;

class VTKFILTERSGENERAL_EXPORT vtkImageDataToPointSet : public vtkStructuredGridAlgorithm
{
public:
  vtkTypeMacro(vtkImageDataToPointSet, vtkStructuredGridAlgorithm);
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

  static vtkImageDataToPointSet *New();

protected:
  vtkImageDataToPointSet();
  ~vtkImageDataToPointSet() VTK_OVERRIDE;

  int RequestData(vtkInformation *request,
                  vtkInformationVector **inputVector,
                  vtkInformationVector *outputVector) VTK_OVERRIDE;

  int FillInputPortInformation(int port, vtkInformation *info) VTK_OVERRIDE;

private:
  vtkImageDataToPointSet(const vtkImageDataToPointSet &) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageDataToPointSet &) VTK_DELETE_FUNCTION;

  int CopyStructure(vtkStructuredGrid *outData, vtkImageData *inData);
};


#endif //vtkImageDataToPointSet_h
