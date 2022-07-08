/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkmTriangleMeshPointNormals.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkmTriangleMeshPointNormals
 * @brief   compute point normals for triangle mesh
 *
 * vtkmTriangleMeshPointNormals is a filter that computes point normals for
 * a triangle mesh to enable high-performance rendering. It is a fast-path
 * version of the vtkmPolyDataNormals filter in order to be able to compute
 * normals for triangle meshes deforming rapidly.
 *
 * The computed normals (a vtkFloatArray) are set to be the active normals
 * (using SetNormals()) of the PointData. The array name is "Normals".
 *
 * The algorithm works by determining normals for each triangle and adding
 * these vectors to the triangle points. The resulting vectors at each
 * point are then normalized.
 *
 * @warning
 * Normals are computed only for triangular polygons: the filter can not
 * handle meshes with other types of cells (Verts, Lines, Strips) or Polys
 * with the wrong number of components (not equal to 3).
 *
 * @warning
 * Unlike the vtkPolyDataNormals filter, this filter does not apply any
 * splitting nor checks for cell orientation consistency in order to speed
 * up the computation. Moreover, normals are not calculated the exact same
 * way as the vtkPolyDataNormals filter since the triangle normals are not
 * normalized before being added to the point normals: those cell normals
 * are therefore weighted by the triangle area. This is not more nor less
 * correct than normalizing them before adding them, but it is much faster.
 *
 */

#ifndef vtkmTriangleMeshPointNormals_h
#define vtkmTriangleMeshPointNormals_h

#include "vtkAcceleratorsVTKmFiltersModule.h" // for export macro
#include "vtkTriangleMeshPointNormals.h"
#include "vtkmlib/vtkmInitializer.h" // Need for initializing vtk-m

class VTKACCELERATORSVTKMFILTERS_EXPORT vtkmTriangleMeshPointNormals
  : public vtkTriangleMeshPointNormals
{
public:
  vtkTypeMacro(vtkmTriangleMeshPointNormals, vtkTriangleMeshPointNormals);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkmTriangleMeshPointNormals* New();

  ///@{
  /**
   * When this flag is off (the default), then the computation will fall back
   * to the serial VTK version if VTK-m fails to run. When the flag is on,
   * the filter will generate an error if VTK-m fails to run. This is mostly
   * useful in testing to make sure the expected algorithm is run.
   */
  vtkGetMacro(ForceVTKm, vtkTypeBool);
  vtkSetMacro(ForceVTKm, vtkTypeBool);
  vtkBooleanMacro(ForceVTKm, vtkTypeBool);
  ///@}

protected:
  vtkmTriangleMeshPointNormals();
  ~vtkmTriangleMeshPointNormals() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkTypeBool ForceVTKm = false;

private:
  vtkmTriangleMeshPointNormals(const vtkmTriangleMeshPointNormals&) = delete;
  void operator=(const vtkmTriangleMeshPointNormals&) = delete;
  vtkmInitializer Initializer;
};

#endif // vtkmTriangleMeshPointNormals_h
