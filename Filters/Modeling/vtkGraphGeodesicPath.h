// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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

VTK_ABI_NAMESPACE_BEGIN
class vtkIdList;

class VTKFILTERSMODELING_EXPORT vtkGraphGeodesicPath : public vtkGeodesicPath
{
public:
  ///@{
  /**
   * Standard methods for printing and determining type information.
   */
  vtkTypeMacro(vtkGraphGeodesicPath, vtkGeodesicPath);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * The vertex at the start of the shortest path
   */
  vtkGetMacro(StartVertex, vtkIdType);
  vtkSetMacro(StartVertex, vtkIdType);
  ///@}

  ///@{
  /**
   * The vertex at the end of the shortest path
   */
  vtkGetMacro(EndVertex, vtkIdType);
  vtkSetMacro(EndVertex, vtkIdType);
  ///@}

protected:
  vtkGraphGeodesicPath();
  ~vtkGraphGeodesicPath() override;

  vtkIdType StartVertex;
  vtkIdType EndVertex;

private:
  vtkGraphGeodesicPath(const vtkGraphGeodesicPath&) = delete;
  void operator=(const vtkGraphGeodesicPath&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
