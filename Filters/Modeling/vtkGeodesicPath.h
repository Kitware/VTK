/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeodesicPath.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkGeodesicPath
 * @brief   Abstract base for classes that generate a geodesic path
 *
 * Serves as a base class for algorithms that trace a geodesic path on a
 * polygonal dataset.
*/

#ifndef vtkGeodesicPath_h
#define vtkGeodesicPath_h

#include "vtkFiltersModelingModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkPolyData;

class VTKFILTERSMODELING_EXPORT vtkGeodesicPath : public vtkPolyDataAlgorithm
{
public:

  //@{
  /**
   * Standard methids for printing and determining type information.
   */
  vtkTypeMacro(vtkGeodesicPath,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  //@}

protected:
  vtkGeodesicPath();
  ~vtkGeodesicPath();

  virtual int FillInputPortInformation(int port, vtkInformation *info);

private:
  vtkGeodesicPath(const vtkGeodesicPath&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGeodesicPath&) VTK_DELETE_FUNCTION;
};

#endif

