/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCell3D.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkCell3D.h"
#include "vtkOrderedTriangulator.h"
#include "vtkPointLocator.h"

vtkCell3D::~vtkCell3D()
{
  if ( this->Triangulator )
    {
    this->Triangulator->Delete();
    this->Triangulator = NULL;
    }
}

void vtkCell3D::Clip(float value, vtkScalars *cellScalars, 
                     vtkPointLocator *locator, vtkCellArray *tets,
                     vtkPointData *inPD, vtkPointData *outPD,
                     vtkCellData *vtkNotUsed(inCD), int vtkNotUsed(cellId),
		     vtkCellData *vtkNotUsed(outCD), int insideOut)
{
  vtkCell3D *cell3D = (vtkCell3D *)this; //has to be to be in this method
  int numPts=this->GetNumberOfPoints();
  int numEdges=this->GetNumberOfEdges();
  int *edges;
  int i;
  int type, ptId, id;
  int internalId[VTK_CELL_SIZE];
  float s1, s2, *xPtr, t, p1[3], p2[3], x[3];
  
  // Create one if necessary
  if ( ! this->Triangulator )
    {
    this->Triangulator = vtkOrderedTriangulator::New();
    }

  // Initialize Delaunay insertion process with voxel triangulation.
  // No more than (numPts + numEdges) points can be inserted.
  this->Triangulator->InitTriangulation(this->GetBounds(),
                                        (numPts + numEdges));

  // Inject ordered voxel corner points into triangulation. Recall
  // that the PreSortedOn() flag was set in the triangulator.
  for (i=0; i<numPts; i++)
    {
    ptId = this->PointIds->GetId(i);
      
    // Currently all points are injected because of the possibility 
    // of intersection point merging.
    s1 = cellScalars->GetScalar(i);
    if ( (s1 >= value && !insideOut) || (s1 < value && insideOut) )
      {
      type = 0; //inside
      }
    else
      {
      type = 4; //no insert, but its type might change later
      }

    xPtr = this->Points->GetPoint(i);
    if ( locator->InsertUniquePoint(xPtr, id) )
      {
      outPD->CopyData(inPD,ptId, id);
      }
    internalId[i] = this->Triangulator->InsertPoint(id, xPtr, type);
    }//for eight voxel corner points
  
  // For each edge intersection point, insert into triangulation. Edge
  // intersections come from clipping value. Have to be careful of 
  // intersections near exisiting points (causes bad Delaunay behavior).
  for (int edgeNum=0; edgeNum < numEdges; edgeNum++)
    {
    cell3D->GetEdgePoints(edgeNum, edges);
    s1 = cellScalars->GetScalar(edges[0]);
    s2 = cellScalars->GetScalar(edges[1]);
    if ( (s1 < value && s2 >= value) || (s1 >= value && s2 < value) )
      {
      t = (value - s1) / (s2 - s1);

      // Check to see whether near the intersection is near a voxel corner. 
      // If so,have to merge requiring a change of type to type=boundary.
      if ( t < 0.01 )
        {
        this->Triangulator->UpdatePointType(internalId[edges[0]], 2);
        continue;
        }
      else if ( t > 0.99 )
        {
        this->Triangulator->UpdatePointType(internalId[edges[1]], 2);
        continue;
        }

      // generate edge intersection point
      this->Points->GetPoint(edges[0],p1);
  
    this->Points->GetPoint(edges[1],p2);
      for (i=0; i<3; i++)
        {
        x[i] = p1[i] + t * (p2[i] - p1[i]);
        }
      
      // Incorporate point into output and interpolate edge data as necessary
      if ( locator->InsertUniquePoint(x, ptId) )
        {
        outPD->InterpolateEdge(inPD, ptId, this->PointIds->GetId(edges[0]),
                               this->PointIds->GetId(edges[1]), t);
        }

      //Insert into Delaunay triangulation
      this->Triangulator->InsertPoint(ptId,x,2);

      }//if edge intersects value
    }//for all edges
  
  // triangulate the points
  this->Triangulator->Triangulate();

  // Add the triangulation to the mesh
  this->Triangulator->AddTetras(0,tets);
}

void vtkCell3D::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkCell::PrintSelf(os,indent);
}
