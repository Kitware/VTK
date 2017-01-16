/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphGeodesicPath.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkGraphGeodesicPath
 * @brief   Abstract base for classes that generate a geodesic path on a graph (mesh).
 *
 * Serves as a base class for algorithms that trace a geodesic on a
 * polygonal dataset treating it as a graph. ie points connecting the
 * vertices of the graph
*/

#ifndef vtkGraphGeodesicPath_h
#define vtkGraphGeodesicPath_h

#include "vtkFiltersModelingModule.h" // For export macro
#include "vtkGeodesicPath.h"

class vtkIdList;

class VTKFILTERSMODELING_EXPORT vtkGraphGeodesicPath : public vtkGeodesicPath
{
public:
  //@{
  /**
   * Standard methids for printing and determining type information.
   */
  vtkTypeMacro(vtkGraphGeodesicPath,vtkGeodesicPath);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * The vertex at the start of the shortest path
   */
  vtkGetMacro(StartVertex, vtkIdType);
  vtkSetMacro(StartVertex, vtkIdType);
  //@}

  //@{
  /**
   * The vertex at the end of the shortest path
   */
  vtkGetMacro(EndVertex, vtkIdType);
  vtkSetMacro(EndVertex, vtkIdType);
  //@}

protected:
  vtkGraphGeodesicPath();
  ~vtkGraphGeodesicPath() VTK_OVERRIDE;

  vtkIdType StartVertex;
  vtkIdType EndVertex;

private:
  vtkGraphGeodesicPath(const vtkGraphGeodesicPath&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGraphGeodesicPath&) VTK_DELETE_FUNCTION;

};

#endif

