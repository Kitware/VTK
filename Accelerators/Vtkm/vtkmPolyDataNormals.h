/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkmPolyDataNormals.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkmPolyDataNormals
 * @brief   compute normals for polygonal mesh
 *
 * vtkmPolyDataNormals is a filter that computes point and/or cell normals
 * for a polygonal mesh. The user specifies if they would like the point
 * and/or cell normals to be computed by setting the ComputeCellNormals
 * and ComputePointNormals flags.
 *
 * The computed normals (a vtkFloatArray) are set to be the active normals
 * (using SetNormals()) of the PointData and/or the CellData (respectively)
 * of the output PolyData. The name of these arrays is "Normals".
 *
 * The algorithm works by determining normals for each polygon and then
 * averaging them at shared points.
 *
 * @warning
 * Normals are computed only for polygons and triangles. Normals are
 * not computed for lines, vertices, or triangle strips.
 *
 * @sa
 * For high-performance rendering, you could use vtkmTriangleMeshPointNormals
 * if you know that you have a triangle mesh which does not require splitting
 * nor consistency check on the cell orientations.
 *
 */

#ifndef vtkmPolyDataNormals_h
#define vtkmPolyDataNormals_h

#include "vtkAcceleratorsVTKmModule.h" // for export macro
#include "vtkPolyDataNormals.h"

class VTKACCELERATORSVTKM_EXPORT vtkmPolyDataNormals : public vtkPolyDataNormals
{
public:
  vtkTypeMacro(vtkmPolyDataNormals, vtkPolyDataNormals);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkmPolyDataNormals* New();

protected:
  vtkmPolyDataNormals();
  ~vtkmPolyDataNormals() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkmPolyDataNormals(const vtkmPolyDataNormals&) = delete;
  void operator=(const vtkmPolyDataNormals&) = delete;
};

#endif // vtkmPolyDataNormals_h
// VTK-HeaderTest-Exclude: vtkmPolyDataNormals.h
