//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================
/**
 * @class   vtkmLevelOfDetail
 * @brief   reduce the number of triangles in a mesh
 *
 * vtkmLevelOfDetail is a filter to reduce the number of triangles in a
 * triangle mesh, forming a good approximation to the original geometry. The
 * input to vtkmLevelOfDetail is a vtkPolyData or vtkUnstrcutredGrid object,
 * and only triangles are treated. If you desire to decimate polygonal meshes,
 * first triangulate the polygons with vtkTriangleFilter object.
 *
 * The general approach of the algorithm is to cluster vertices in a uniform
 * binning of space, accumulating to an average point within each bin. In
 * more detail, the algorithm first gets the bounds of the input poly data.
 * It then breaks this bounding volume into a user-specified number of
 * spatial bins.  It then reads each triangle from the input and hashes its
 * vertices into these bins. Then, if 2 or more vertices of
 * the triangle fall in the same bin, the triangle is dicarded.  If the
 * triangle is not discarded, it adds the triangle to the list of output
 * triangles as a list of vertex identifiers.  (There is one vertex id per
 * bin.)  After all the triangles have been read, the representative vertex
 * for each bin is computed.  This determines the spatial location of the
 * vertices of each of the triangles in the output.
 *
 * To use this filter, specify the divisions defining the spatial subdivision
 * in the x, y, and z directions. Compared to algorithms such as
 * vtkQuadricClustering, a significantly higher bin count is recommended as it
 * doesn't increase the computation or memory of the algorithm and will produce
 * significantly better results.
 *
*/

#ifndef vtkmLevelOfDetail_h
#define vtkmLevelOfDetail_h

#include "vtkPolyDataAlgorithm.h"
#include "vtkAcceleratorsVTKmModule.h" //required for correct implementation
#include "vtkmConfig.h" //required for general vtkm setup

class VTKACCELERATORSVTKM_EXPORT vtkmLevelOfDetail : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkmLevelOfDetail,vtkPolyDataAlgorithm)
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  static vtkmLevelOfDetail* New();

  // Description:
  // Set/Get the number of divisions along an individual axis for the spatial
  // bins.
  // The number of spatial bins is NumberOfXDivisions*NumberOfYDivisions*
  // NumberOfZDivisions.
  void SetNumberOfXDivisions(int num);
  void SetNumberOfYDivisions(int num);
  void SetNumberOfZDivisions(int num);
  int GetNumberOfXDivisions();
  int GetNumberOfYDivisions();
  int GetNumberOfZDivisions();

  // Description:
  // Set/Get the number of divisions for each axis for the spatial bins.
  // The number of spatial bins is NumberOfXDivisions*NumberOfYDivisions*
  // NumberOfZDivisions.
  void SetNumberOfDivisions(int div[3])
  {
    this->SetNumberOfDivisions(div[0], div[1], div[2]);
  }
  void SetNumberOfDivisions(int div0, int div1, int div2);

  const int* GetNumberOfDivisions();
  void GetNumberOfDivisions(int div[3]);

protected:
  vtkmLevelOfDetail();
  ~vtkmLevelOfDetail();

  virtual int RequestData(vtkInformation*, vtkInformationVector**,
                          vtkInformationVector*) VTK_OVERRIDE;

private:
  int NumberOfDivisions[3];

  vtkmLevelOfDetail(const vtkmLevelOfDetail&) VTK_DELETE_FUNCTION;
  void operator=(const vtkmLevelOfDetail&) VTK_DELETE_FUNCTION;
};

#endif // vtkmLevelOfDetail_h
// VTK-HeaderTest-Exclude: vtkmLevelOfDetail.h
