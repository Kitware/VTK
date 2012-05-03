/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTessellatedBoxSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkTessellatedBoxSource - Create a polygonal representation of a box
// with a given level of subdivision.
// .SECTION Description
// vtkTessellatedBoxSource creates a axis-aligned box defined by its bounds
// and a level of subdivision. Connectivity is strong: points of the vertices
// and inside the edges are shared between faces. In other words, faces are
// connected. Each face looks like a grid of quads, each quad is composed of
// 2 triangles.
// Given a level of subdivision `l', each edge has `l'+2 points, `l' of them
// are internal edge points, the 2 other ones are the vertices.
// Each face has a total of (`l'+2)*(`l'+2) points, 4 of them are vertices,
// 4*`l' are internal edge points, it remains `l'^2 internal face points.
//
// This source only generate geometry, no DataArrays like normals or texture
// coordinates.

#ifndef __vtkTessellatedBoxSource_h
#define __vtkTessellatedBoxSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class VTKFILTERSSOURCES_EXPORT vtkTessellatedBoxSource : public vtkPolyDataAlgorithm
{
public:
  static vtkTessellatedBoxSource *New();
  vtkTypeMacro(vtkTessellatedBoxSource,vtkPolyDataAlgorithm);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the bounds of the box. See GetBounds() for a detail description.
  // \pre xmin<=xmax && ymin<=ymax && zmin<zmax
  vtkSetVector6Macro(Bounds, double);

  // Description:
  // Bounds of the box in world coordinates. This a 6-uple of xmin,xmax,ymin,
  // ymax,zmin and zmax. Initial value is (-0.5,0.5,-0.5,0.5,-0.5,0.5), bounds
  // of a cube of length 1 centered at (0,0,0). Bounds are defined such that
  // xmin<=xmax, ymin<=ymax and zmin<zmax.
  // \post xmin<=xmax && ymin<=ymax && zmin<zmax
  vtkGetVector6Macro(Bounds, double);

  // Description:
  // Set the level of subdivision of the faces.
  // \pre positive_level: level>=0
  vtkSetMacro(Level,int);

  // Description:
  // Level of subdivision of the faces. Initial value is 0.
  // \post positive_level: level>=0
  vtkGetMacro(Level,int);

  // Description:
  // Flag to tell the source to duplicate points shared between faces
  // (vertices of the box and internal edge points). Initial value is false.
  // Implementation note: duplicating points is an easier method to implement
  // than a minimal number of points.
  vtkSetMacro(DuplicateSharedPoints, int);
  vtkGetMacro(DuplicateSharedPoints, int);
  vtkBooleanMacro(DuplicateSharedPoints, int);

  // Description:
  // Flag to tell the source to generate either a quad or two triangle for a
  // set of four points. Initial value is false (generate triangles).
  vtkSetMacro(Quads, int);
  vtkGetMacro(Quads, int);
  vtkBooleanMacro(Quads, int);

protected:
   vtkTessellatedBoxSource();
  ~vtkTessellatedBoxSource();

  // Description:
  // Called by the superclass. Send the WHOLE_BOUNDING_BOX key.
  virtual int RequestInformation(vtkInformation *request,
                                 vtkInformationVector **inputVector,
                                 vtkInformationVector *outputVector);

  // Description:
  // Called by the superclass. Actual creation of the points and cells
  // happens here.
  virtual int RequestData(vtkInformation *request,
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outpuVector);


  void DuplicateSharedPointsMethod(double *bounds,
                                   vtkPoints *points,
                                   vtkCellArray *polys);

  void MinimalPointsMethod(double *bounds,
                           vtkPoints *points,
                           vtkCellArray *polys);

  // Description:
  // Compute the pointId of point (i,j) of face f.
  // Used by MinimalPointsMethod().
  // \pre valid_face: f>=0 && f<6
  // \pre valid_i: i>=0 && i<=(this->Level+1)
  // \pre valid_j: j>=0 && j<=(this->Level+1)
  vtkIdType LocalFacePointCoordinatesToPointId(int f,
                                               int i,
                                               int j);

  // Description:
  // Build one of the face of the box with some level of tessellation.
  // facePoints[0] is the lower-left point
  // facePoints[1] is the point along the first axis
  // facePoints[2] is the point along the second axis
  // \pre positive_id: firstPointId>=0
  // \pre points_exists: points!=0
  // \pre polys_exists: polys!=0
  void BuildFace(vtkPoints *points,
                 vtkCellArray *polys,
                 vtkIdType firstPointId,
                 double facePoints[3][3],
                 int changed);

  double Bounds[6];
  int Level;
  int DuplicateSharedPoints;
  int Quads;

private:
  vtkTessellatedBoxSource(const vtkTessellatedBoxSource&);  // Not implemented.
  void operator=(const vtkTessellatedBoxSource&);  // Not implemented.
};

#endif
