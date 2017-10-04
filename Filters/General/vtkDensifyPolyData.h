/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDensifyPolyData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkDensifyPolyData
 * @brief   Densify the input by adding points at the
 * centroid
 *
 *
 * The filter takes any polygonal data as input and will tessellate cells that
 * are planar polygons present by fanning out triangles from its centroid.
 * Other cells are simply passed through to the output.  PointData, if present,
 * is interpolated via linear interpolation. CellData for any tessellated cell
 * is simply copied over from its parent cell. Planar polygons are assumed to
 * be convex. Funny things will happen if they are not.
 *
 * The number of subdivisions can be controlled by the parameter
 * NumberOfSubdivisions.
*/

#ifndef vtkDensifyPolyData_h
#define vtkDensifyPolyData_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class VTKFILTERSGENERAL_EXPORT vtkDensifyPolyData : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkDensifyPolyData,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkDensifyPolyData *New();

  //@{
  /**
   * Number of recursive subdivisions. Initial value is 1.
   */
  vtkSetMacro( NumberOfSubdivisions, unsigned int );
  vtkGetMacro( NumberOfSubdivisions, unsigned int );
  //@}

protected:
  vtkDensifyPolyData();
  ~vtkDensifyPolyData() override;

  int RequestData(vtkInformation *,
                  vtkInformationVector **,
                  vtkInformationVector *) override;

  unsigned int NumberOfSubdivisions;

private:
  int FillInputPortInformation(int, vtkInformation*) override;

  vtkDensifyPolyData(const vtkDensifyPolyData&) = delete;
  void operator=(const vtkDensifyPolyData&) = delete;
};

#endif


