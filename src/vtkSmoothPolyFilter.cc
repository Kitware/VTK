/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSmoothPolyFilter.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkSmoothPolyFilter.hh"
#include "vtkMath.hh"
#include "vtkTriangleFilter.hh"
#include "vtkPolygon.hh"

// Description:
// Construct object with number of iterations 20; expansion factor 0.34;
// contraction factor -0.33; feature edge smoothing turned on; feature 
// angle 45 degrees; edge angle 15 degrees; and boundary smoothing turned 
// on.
vtkSmoothPolyFilter::vtkSmoothPolyFilter()
{
  this->NumberOfIterations = 20;
  this->ExpansionFactor = 0.34;
  this->ContractionFactor = -0.33;
  this->FeatureEdgeSmoothing = 1;
  this->FeatureAngle = 45.0;
  this->EdgeAngle = 15.0;
  this->BoundarySmoothing = 1;
}

#define VTK_SIMPLE_VERTEX 0
#define VTK_FIXED_VERTEX 1
#define VTK_EDGE_VERTEX 1

// Special structure for marking vertices
typedef struct _vtkLocalVertex 
  {
  char      type;
  vtkIdList *edges; // connected edges (list of connected point ids)
  } vtkMeshVertex, *vtkMeshVertexPtr;
    
void vtkSmoothPolyFilter::Execute()
{
  static vtkMath math; // avoid constructor overhead
  int numPts, numCells;
  int i, j, k, numPolys, numStrips;
  int npts, *pts;
  int p1, p2;
  float *x, *y, deltaX[3], xNew[3];
  float x1[3], x2[3], x3[3], l1[3], l2[3], lenl1, lenl2;
  float CosFeatureAngle; //Cosine of angle between adjacent polys
  float CosEdgeAngle; // Cosine of angle between adjacent edges
  vtkPolyData *inMesh, *Mesh;
  vtkPoints *inPts;
  vtkTriangleFilter *toTris=NULL;
  vtkCellArray *inVerts, *inLines, *inPolys, *inStrips;
  vtkFloatPoints *newPts;
  vtkMeshVertexPtr Verts;
  vtkPolyData *input=(vtkPolyData *)this->Input;
  vtkPolyData *output=(vtkPolyData *)this->Output;
//
// Check input
//
  if ( (numPts=input->GetNumberOfPoints()) < 1 || 
  (numCells=input->GetNumberOfCells()) < 1 )
    {
    vtkErrorMacro(<<"No data to smooth!");
    return;
    }

  CosFeatureAngle = cos((double) math.DegreesToRadians() * this->FeatureAngle);
  CosEdgeAngle = cos((double) math.DegreesToRadians() * this->EdgeAngle);

  vtkDebugMacro(<<"Smoothing " << numPts << " vertices, " << numCells 
               << " cells with:\n"
               << "\tIterations= " << this->NumberOfIterations << "\n"
               << "\tExpansion Factor= " << this->ExpansionFactor << "\n"
               << "\tContraction Factor= " << this->ContractionFactor << "\n"
               << "\tEdge Angle= " << this->EdgeAngle << "\n"
               << "\tBoundary Smoothing " << (this->BoundarySmoothing ? "On\n" : "Off\n")
               << "\tFeature Edge Smoothing " << (this->FeatureEdgeSmoothing ? "On\n" : "Off\n"));

//
// Peform topological analysis. What we're gonna do is build a connectivity
// array of connected vertices. The outcome will be one of three
// classifications for a vertex: VTK_SIMPLE_VERTEX, VTK_FIXED_VERTEX. or
// VTK_EDGE_VERTEX. Simple vertices are smoothed using all connected 
// vertices. FIXED vertices are never smoothed. Edge vertices are smoothed
// using a subset of the attached vertices.
//
  vtkDebugMacro(<<"Analyzing topology...");
  Verts = new vtkMeshVertex[numPts];
  for (i=0; i<numPts; i++)
    {
    Verts[i].type = VTK_SIMPLE_VERTEX; //can smooth
    Verts[i].edges = NULL;
    }

  inPts = input->GetPoints();
  
  // check vertices first. Vertices are never smoothed_--------------
  for (inVerts=input->GetVerts(), inVerts->InitTraversal(); 
  inVerts->GetNextCell(npts,pts); )
    {
    for (j=0; j<npts; j++)
      {
      Verts[pts[j]].type = VTK_FIXED_VERTEX;
      }
    }

  // now check lines. Only manifold lines can be smoothed------------
  for (inLines=input->GetLines(), inLines->InitTraversal(); 
  inLines->GetNextCell(npts,pts); )
    {
    for (j=0; j<npts; j++)
      {
      if ( Verts[pts[j]].type == VTK_SIMPLE_VERTEX )
        {
        if ( j == (npts-1) ) //end-of-line marked FIXED
          {
          Verts[pts[j]].type == VTK_FIXED_VERTEX;
          }
        else if ( j == 0 ) //beginning-of-line marked FIXED
          {
          Verts[pts[0]].type == VTK_FIXED_VERTEX;
          inPts->GetPoint(pts[0],x2);
          inPts->GetPoint(pts[1],x3);
          }
        else //is edge vertex (unless already edge vertex!)
          {
          Verts[pts[j]].type = VTK_EDGE_VERTEX;
          Verts[pts[j]].edges = new vtkIdList(2);
          Verts[pts[j]].edges->SetId(0,pts[j-1]);
          Verts[pts[j]].edges->SetId(1,pts[j+1]);
          }
        } //if simple vertex

      else if ( Verts[pts[j]].type == VTK_EDGE_VERTEX )
        { //multiply connected, becomes fixed!
        Verts[pts[j]].type = VTK_FIXED_VERTEX;
        delete Verts[pts[j]].edges;
        }

      } //for all points in this line
    } //for all lines


  // now polygons and triangle strips-------------------------------
  inPolys=input->GetPolys();
  numPolys = inPolys->GetNumberOfCells();
  inStrips=input->GetStrips();
  numStrips = inStrips->GetNumberOfCells();

  if ( numPolys > 0 || numStrips > 0 )
    { //build cell structure
    vtkCellArray *polys;
    int numNei, cellId, nei, edge;
    int *neiPts, numNeiPts;
    float normal[3], neiNormal[3];
    vtkIdList neighbors(VTK_CELL_SIZE);
    vtkPolygon poly;

    inMesh = new vtkPolyData;
    inMesh->SetPoints(inPts);
    inMesh->SetPolys(inPolys);
    Mesh = inMesh;

    if ( (numStrips = inStrips->GetNumberOfCells()) > 0 )
      { // convert data to triangles
      inMesh->SetStrips(inStrips);
      toTris = new vtkTriangleFilter;
      toTris->SetInput(inMesh);
      toTris->Update();
      Mesh = toTris->GetOutput();
      }

    Mesh->BuildLinks(); //to do neighborhood searching
    polys = Mesh->GetPolys();

    for (cellId=0, polys->InitTraversal(); polys->GetNextCell(npts,pts); 
    cellId++)
      {
      for (i=0; i < npts; i++) 
        {
        p1 = pts[i];
        p2 = pts[(i+1)%npts];

        if ( Verts[p1].edges == NULL ) Verts[p1].edges = new vtkIdList;
        if ( Verts[p2].edges == NULL ) Verts[p2].edges = new vtkIdList;

        Mesh->GetCellEdgeNeighbors(cellId,p1,p2,neighbors);
        numNei = neighbors.GetNumberOfIds();

        edge = 0;
        if ( numNei == 0 )
          {
          edge = 1;
          }

        else if ( numNei >= 2 )
          {
          // check to make sure that this edge hasn't been marked already
          for (j=0; j < numNei; j++)
            if ( neighbors.GetId(j) < cellId )
              break;
          if ( j >= numNei )
            {
            edge = 1;
            }
          }

        else if ( numNei == 1 && (nei=neighbors.GetId(0)) > cellId ) 
          {
          poly.ComputeNormal(inPts,npts,pts,normal);
          Mesh->GetCellPoints(nei,numNeiPts,neiPts);
          poly.ComputeNormal(inPts,numNeiPts,neiPts,neiNormal);

          if ( math.Dot(normal,neiNormal) <= CosFeatureAngle ) 
            {
            edge = 1;
            }
          }//see whether it's an edge

        if ( edge && Verts[p1].type == VTK_SIMPLE_VERTEX )
          {
          Verts[p1].edges->Reset();
          Verts[p1].edges->InsertNextId(p2);
          Verts[p1].type = VTK_EDGE_VERTEX;
          }
        else if ( (edge && Verts[p1].type == VTK_EDGE_VERTEX) ||
        (!edge && Verts[p1].type == VTK_SIMPLE_VERTEX ) )
          {
          Verts[p1].edges->InsertNextId(p2);
          }

        if ( edge && Verts[p2].type == VTK_SIMPLE_VERTEX )
          {
          Verts[p2].edges->Reset();
          Verts[p2].edges->InsertNextId(p1);
          Verts[p2].type = VTK_EDGE_VERTEX;
          }
        else if ( (edge && Verts[p2].type == VTK_EDGE_VERTEX ) ||
        (!edge && Verts[p2].type == VTK_SIMPLE_VERTEX ) )
          {
          Verts[p2].edges->InsertNextId(p1);
          }
        }
      }

    delete inMesh;
    if (toTris) delete toTris;

    }//if strips or polys

  //post-process edge vertices to make sure we can smooth them
  for (i=0; i<numPts; i++)
    {
    if ( Verts[i].type == VTK_EDGE_VERTEX )
      { //see how many edges; if two, what the angle is
      if ( (npts = Verts[i].edges->GetNumberOfIds()) != 2 )
        {
        Verts[i].type = VTK_FIXED_VERTEX;
        }

      else //check angle between edges
        {
        inPts->GetPoint(Verts[i].edges->GetId(0),x1);
        inPts->GetPoint(i,x2);
        inPts->GetPoint(Verts[i].edges->GetId(1),x3);

        for (k=0; k<3; k++)
          {
          l1[k] = x2[k] - x1[k];
          l2[k] = x3[k] - x2[k];
          }
        if ( (lenl1 = math.Normalize(l1)) >= 0.0 &&
        (lenl2 = math.Normalize(l2)) >= 0.0 &&
        math.Dot(l1,l2) < CosEdgeAngle)
          {
          Verts[pts[j]].type = VTK_FIXED_VERTEX;
          }
        }//if along edge
      }//if edge vertex
    }//for all points
//
// Perform Laplacian smoothing
//
  vtkDebugMacro(<<"Beginning smoothing iterations...");

  newPts = new vtkFloatPoints(numPts);
  for (i=0; i<numPts; i++) //initialize to old coordinates
    {
    newPts->SetPoint(i,inPts->GetPoint(i));
    }

  if ( this->ContractionFactor != 0.0 ) //contraction pass-----------
    {
    for (i=0; i<numPts; i++) 
      {
      if ( Verts[i].type != VTK_FIXED_VERTEX && Verts[i].edges != NULL &&
      (npts = Verts[i].edges->GetNumberOfIds()) > 0 )
        {
        x = newPts->GetPoint(i); //use current points
        deltaX[0] = deltaX[1] = deltaX[2] = 0.0;
        for (j=0; j<npts; j++)
          {
          y = newPts->GetPoint(Verts[i].edges->GetId(j));
          for (k=0; k<3; k++) deltaX[k] += (y[k] - x[k]) / npts;
          }//for all connected points

        for (k=0;k<3;k++) xNew[k] = x[k] + this->ContractionFactor*deltaX[k];
        newPts->SetPoint(i,xNew);
        }//if can move point
      }//for all points
    }//if non-zero contraction


  if ( this->ExpansionFactor != 0.0 ) //expansion pass---------------
    {
    for (i=0; i<numPts; i++) 
      {
      if ( Verts[i].type != VTK_FIXED_VERTEX && Verts[i].edges != NULL &&
      (npts = Verts[i].edges->GetNumberOfIds()) > 0 )
        {
        x = newPts->GetPoint(i); //use current points
        deltaX[0] = deltaX[1] = deltaX[2] = 0.0;
        for (j=0; j<npts; j++)
          {
          y = newPts->GetPoint(Verts[i].edges->GetId(j));
          for (k=0; k<3; k++) deltaX[k] += (y[k] - x[k]) / npts;
          }//for all connected points

        for (k=0; k<3; k++) xNew[k] = x[k] + this->ExpansionFactor*deltaX[k];
        newPts->SetPoint(i,xNew);
        }//if can move point
      }//for all points
    }//if non-zero expansion
//
// Update output. Only point coordinates have changed.
//
  output->GetPointData()->PassData(input->GetPointData());

  output->SetPoints(newPts);
  newPts->Delete();

  //free up connectivity storage
  for (i=0; i<numPts; i++)
    {
    if ( Verts[i].edges != NULL ) delete Verts[i].edges;
    }
  delete [] Verts;
}

void vtkSmoothPolyFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyToPolyFilter::PrintSelf(os,indent);

  os << indent << "Number of Iterations: " << this->NumberOfIterations << "\n";
  os << indent << "Expansion Factor: " << this->ExpansionFactor << "\n";
  os << indent << "Contraction Factor: " << this->ContractionFactor << "\n";
  os << indent << "Feature Edge Smoothing: " << (this->FeatureEdgeSmoothing ? "On\n" : "Off\n");
  os << indent << "Feature Angle: " << this->FeatureAngle << "\n";
  os << indent << "Edge Angle: " << this->EdgeAngle << "\n";
  os << indent << "Boundary Smoothing: " << (this->BoundarySmoothing ? "On\n" : "Off\n");

}
