/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCell3D.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCell3D - abstract class to specify 3D cell interface
// .SECTION Description
// vtkCell3D is an abstract class that extends the interfaces for 3D data 
// cells, and implements methods needed to satisfy the vtkCell API. The 
// 3D cells include hexehedra, tetrahedra, wedge, pyramid, and voxel.

// .SECTION See Also
// vtkTetra vtkHexahedron vtkVoxel vtkWedge vtkPyramid

#ifndef __vtkCell3D_h
#define __vtkCell3D_h

#include "vtkCell.h"

class vtkOrderedTriangulator;

class VTK_COMMON_EXPORT vtkCell3D : public vtkCell
{
public:
  vtkTypeRevisionMacro(vtkCell3D,vtkCell);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the pair of vertices that define an edge. The method returns the
  // number of vertices, along with an array of vertices. Note that the
  // vertices are 0-offset; that is, they refer to the ids of the cell, not
  // the point ids of the mesh that the cell belongs to. The edgeId must
  // range between 0<=edgeId<this->GetNumberOfEdges().
  virtual void GetEdgePoints(int edgeId, int* &pts) = 0;
  
  // Description:
  // Get the list of vertices that define a face.  The list is terminated
  // with a negative number. Note that the vertices are 0-offset; that is,
  // they refer to the ids of the cell, not the point ids of the mesh that
  // the cell belongs to. The faceId must range between
  // 0<=faceId<this->GetNumberOfFaces().
  virtual void GetFacePoints(int faceId, int* &pts) = 0;

  // Description:
  // Cut (or clip) the cell based on the input cellScalars and the specified
  // value. The output of the clip operation will be one or more cells of the
  // same topological dimension as the original cell.  The flag insideOut
  // controls what part of the cell is considered inside - normally cell
  // points whose scalar value is greater than "value" are considered
  // inside. If insideOut is on, this is reversed. Also, if the output cell
  // data is non-NULL, the cell data from the clipped cell is passed to the
  // generated contouring primitives. (Note: the CopyAllocate() method must
  // be invoked on both the output cell and point data. The cellId refers to
  // the cell from which the cell data is copied.)  (Satisfies vtkCell API.)
  virtual void Clip(float value, vtkDataArray *cellScalars, 
                    vtkPointLocator *locator, vtkCellArray *connectivity,
                    vtkPointData *inPd, vtkPointData *outPd,
                    vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd, 
                    int insideOut);

  // Description:
  // The topological dimension of the cell. (Satisfies vtkCell API.)
  virtual int GetCellDimension() {return 3;}

protected:
  vtkCell3D():Triangulator(NULL) {}
  ~vtkCell3D();
  
  vtkOrderedTriangulator *Triangulator;
  
private:
  vtkCell3D(const vtkCell3D&);  // Not implemented.
  void operator=(const vtkCell3D&);  // Not implemented.
};

#endif


