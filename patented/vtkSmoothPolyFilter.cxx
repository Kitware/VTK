/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSmoothPolyFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkSmoothPolyFilter.h"
#include "vtkMath.h"
#include "vtkTriangleFilter.h"
#include "vtkPolygon.h"

// Description:
// Construct object with number of iterations 20; expansion factor -0.34;
// contraction factor 0.33; feature edge smoothing turned off; feature 
// angle 45 degrees; edge angle 15 degrees; and boundary smoothing turned 
// on. Error scalars and vectors are not generated (by default). The 
// convergence criterion is 0.0 of the bounding box diagonal.
vtkSmoothPolyFilter::vtkSmoothPolyFilter()
{
  this->Convergence = 0.0; //goes to number of specied iterations
  this->NumberOfIterations = 20;

  this->ExpansionFactor = -0.34;
  this->ContractionFactor = 0.33;

  this->FeatureAngle = 45.0;
  this->EdgeAngle = 15.0;
  this->FeatureEdgeSmoothing = 0;
  this->BoundarySmoothing = 1;

  this->GenerateErrorScalars = 0;
  this->GenerateErrorVectors = 0;
}

#define VTK_SIMPLE_VERTEX 0
#define VTK_FIXED_VERTEX 1
#define VTK_FEATURE_EDGE_VERTEX 2
#define VTK_BOUNDARY_EDGE_VERTEX 3

// Special structure for marking vertices
typedef struct _vtkMeshVertex 
  {
  char      type;
  vtkIdList *edges; // connected edges (list of connected point ids)
  } vtkMeshVertex, *vtkMeshVertexPtr;
    
void vtkSmoothPolyFilter::Execute()
{
  int numPts, numCells;
  int i, j, k, numPolys, numStrips, pass;
  int npts, *pts;
  int p1, p2;
  float *x, *y, deltaX[3], xNew[3], conv, maxDist, dist, factor;
  float x1[3], x2[3], x3[3], l1[3], l2[3], lenl1, lenl2;
  float CosFeatureAngle; //Cosine of angle between adjacent polys
  float CosEdgeAngle; // Cosine of angle between adjacent edges
  int iterationNumber;
  int numSimple=0, numBEdges=0, numFixed=0, numFEdges=0;
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
  numPts=input->GetNumberOfPoints();
  numCells=input->GetNumberOfCells();
  if (numPts < 1 || numCells < 1)
    {
    vtkErrorMacro(<<"No data to smooth!");
    return;
    }

  CosFeatureAngle = 
    cos((double) vtkMath::DegreesToRadians() * this->FeatureAngle);
  CosEdgeAngle = cos((double) vtkMath::DegreesToRadians() * this->EdgeAngle);

  vtkDebugMacro(<<"Smoothing " << numPts << " vertices, " << numCells 
               << " cells with:\n"
               << "\tConvergence= " << this->Convergence << "\n"
               << "\tIterations= " << this->NumberOfIterations << "\n"
               << "\tExpansion Factor= " << this->ExpansionFactor << "\n"
               << "\tContraction Factor= " << this->ContractionFactor << "\n"
               << "\tEdge Angle= " << this->EdgeAngle << "\n"
               << "\tBoundary Smoothing " << (this->BoundarySmoothing ? "On\n" : "Off\n")
               << "\tFeature Edge Smoothing " << (this->FeatureEdgeSmoothing ? "On\n" : "Off\n")
               << "\tError Scalars " << (this->GenerateErrorScalars ? "On\n" : "Off\n")
               << "\tError Vectors " << (this->GenerateErrorVectors ? "On\n" : "Off\n"));

  if ( this->NumberOfIterations <= 0 ) //don't do anything!
    {
    this->Output->CopyStructure(this->Input);
    this->Output->GetPointData()->PassData(this->Input->GetPointData());
    vtkWarningMacro(<<"Number of iterations == 0: passing data through unchanged");
    return;
    }
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
  conv = this->Convergence * input->GetLength();
  
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
          Verts[pts[j]].type = VTK_FIXED_VERTEX;
          }
        else if ( j == 0 ) //beginning-of-line marked FIXED
          {
          Verts[pts[0]].type = VTK_FIXED_VERTEX;
          inPts->GetPoint(pts[0],x2);
          inPts->GetPoint(pts[1],x3);
          }
        else //is edge vertex (unless already edge vertex!)
          {
          Verts[pts[j]].type = VTK_FEATURE_EDGE_VERTEX;
          Verts[pts[j]].edges = vtkIdList::New();
          Verts[pts[j]].edges->SetNumberOfIds(2);
          Verts[pts[j]].edges->SetId(0,pts[j-1]);
          Verts[pts[j]].edges->SetId(1,pts[j+1]);
          }
        } //if simple vertex

      else if ( Verts[pts[j]].type == VTK_FEATURE_EDGE_VERTEX )
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

    inMesh = vtkPolyData::New();
    inMesh->SetPoints(inPts);
    inMesh->SetPolys(inPolys);
    Mesh = inMesh;

    if ( (numStrips = inStrips->GetNumberOfCells()) > 0 )
      { // convert data to triangles
      inMesh->SetStrips(inStrips);
      toTris = vtkTriangleFilter::New();
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

        if ( Verts[p1].edges == NULL )
          {
          Verts[p1].edges = vtkIdList::New();
          Verts[p1].edges->Allocate(6,6);
          }
        if ( Verts[p2].edges == NULL )
          {
          Verts[p2].edges = vtkIdList::New();
          Verts[p2].edges->Allocate(6,6);
          }

        Mesh->GetCellEdgeNeighbors(cellId,p1,p2,neighbors);
        numNei = neighbors.GetNumberOfIds();

        edge = VTK_SIMPLE_VERTEX;
        if ( numNei == 0 )
          {
          edge = VTK_BOUNDARY_EDGE_VERTEX;
          }

        else if ( numNei >= 2 )
          {
          // check to make sure that this edge hasn't been marked already
          for (j=0; j < numNei; j++)
            if ( neighbors.GetId(j) < cellId )
              break;
          if ( j >= numNei )
            {
            edge = VTK_FEATURE_EDGE_VERTEX;
            }
          }

        else if ( numNei == 1 && (nei=neighbors.GetId(0)) > cellId ) 
          {
          vtkPolygon::ComputeNormal(inPts,npts,pts,normal);
          Mesh->GetCellPoints(nei,numNeiPts,neiPts);
          vtkPolygon::ComputeNormal(inPts,numNeiPts,neiPts,neiNormal);

          if ( this->FeatureEdgeSmoothing &&
          vtkMath::Dot(normal,neiNormal) <= CosFeatureAngle ) 
            {
            edge = VTK_FEATURE_EDGE_VERTEX;
            }
          }
        else // a visited edge; skip rest of analysis
          {
          continue;
          }

        if ( edge && Verts[p1].type == VTK_SIMPLE_VERTEX )
          {
          Verts[p1].edges->Reset();
          Verts[p1].edges->InsertNextId(p2);
          Verts[p1].type = edge;
          }
        else if ( (edge && Verts[p1].type == VTK_BOUNDARY_EDGE_VERTEX) ||
        (edge && Verts[p1].type == VTK_FEATURE_EDGE_VERTEX) ||
        (!edge && Verts[p1].type == VTK_SIMPLE_VERTEX ) )
          {
          Verts[p1].edges->InsertNextId(p2);
          if ( Verts[p1].type && edge == VTK_BOUNDARY_EDGE_VERTEX )
            Verts[p1].type = VTK_BOUNDARY_EDGE_VERTEX;
          }

        if ( edge && Verts[p2].type == VTK_SIMPLE_VERTEX )
          {
          Verts[p2].edges->Reset();
          Verts[p2].edges->InsertNextId(p1);
          Verts[p2].type = edge;
          }
        else if ( (edge && Verts[p2].type == VTK_BOUNDARY_EDGE_VERTEX ) ||
        (edge && Verts[p2].type == VTK_FEATURE_EDGE_VERTEX) ||
        (!edge && Verts[p2].type == VTK_SIMPLE_VERTEX ) )
          {
          Verts[p2].edges->InsertNextId(p1);
          if ( Verts[p2].type && edge == VTK_BOUNDARY_EDGE_VERTEX )
            Verts[p2].type = VTK_BOUNDARY_EDGE_VERTEX;
          }
        }
      }

    delete inMesh;
    if (toTris) delete toTris;

    }//if strips or polys

  //post-process edge vertices to make sure we can smooth them
  for (i=0; i<numPts; i++)
    {
    if ( Verts[i].type == VTK_SIMPLE_VERTEX )
      {
      numSimple++;
      }

    else if ( Verts[i].type == VTK_FIXED_VERTEX )
      {
      numFixed++;
      }

    else if ( Verts[i].type == VTK_FEATURE_EDGE_VERTEX ||
    Verts[i].type == VTK_BOUNDARY_EDGE_VERTEX )
      { //see how many edges; if two, what the angle is

      if ( !this->BoundarySmoothing && 
      Verts[i].type == VTK_BOUNDARY_EDGE_VERTEX )
        {
        Verts[i].type = VTK_FIXED_VERTEX;
        numBEdges++;
        }

      else if ( (npts = Verts[i].edges->GetNumberOfIds()) != 2 )
        {
        Verts[i].type = VTK_FIXED_VERTEX;
        numFixed++;
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
        if ( (lenl1 = vtkMath::Normalize(l1)) >= 0.0 &&
        (lenl2 = vtkMath::Normalize(l2)) >= 0.0 &&
        vtkMath::Dot(l1,l2) < CosEdgeAngle)
          {
          numFixed++;
          Verts[i].type = VTK_FIXED_VERTEX;
          }
        else
          {
          if ( Verts[i].type == VTK_FEATURE_EDGE_VERTEX ) numFEdges++;
          else numBEdges++;
          }
        }//if along edge
      }//if edge vertex
    }//for all points

  vtkDebugMacro(<<"Found\n\t" << numSimple << " simple vertices\n\t"
                << numFEdges << " feature edge vertices\n\t"
                << numBEdges << " boundary edge vertices\n\t"
                << numFixed << " fixed vertices\n\t");
//
// Perform Laplacian smoothing
//
  vtkDebugMacro(<<"Beginning smoothing iterations...");

  newPts = vtkFloatPoints::New();
  newPts->SetNumberOfPoints(numPts);
  for (i=0; i<numPts; i++) //initialize to old coordinates
    {
    newPts->SetPoint(i,inPts->GetPoint(i));
    }

  for ( maxDist=VTK_LARGE_FLOAT, iterationNumber=0; 
  maxDist > conv && iterationNumber < this->NumberOfIterations;
  iterationNumber++ )
    {

    for (maxDist=0.0, pass=0; pass<2; pass++)
      {
      if ( pass == 0 ) factor = this->ContractionFactor;
      else factor = this->ExpansionFactor;

      if ( factor != 0.0 ) //smooth pass
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

            for (k=0;k<3;k++) 
              {
              xNew[k] = x[k] + factor * deltaX[k];
              }
            newPts->SetPoint(i,xNew);
            if ( (dist = vtkMath::Norm(deltaX)) > maxDist )
              {
              maxDist = dist;
              }
            }//if can move point
          }//for all points
        }//if non-zero contraction
      } //for this pass (either contraction or expansion)
    } //for not converged or within iteration count

  vtkDebugMacro(<<"Performed " << iterationNumber << " smoothing passes");
//
// Update output. Only point coordinates have changed.
//
  output->GetPointData()->PassData(input->GetPointData());

  if ( this->GenerateErrorScalars )
    {
    vtkFloatScalars *newScalars = vtkFloatScalars::New();
    newScalars->SetNumberOfScalars(numPts);
    for (i=0; i<numPts; i++)
      {
      inPts->GetPoint(i,x1);
      newPts->GetPoint(i,x2);
      newScalars->SetScalar(i,sqrt(vtkMath::Distance2BetweenPoints(x1,x2)));
      }
    output->GetPointData()->SetScalars(newScalars);
    newScalars->Delete();
    }

  if ( this->GenerateErrorVectors )
    {
    vtkFloatVectors *newVectors = vtkFloatVectors::New();
    newVectors->SetNumberOfVectors(numPts);
    for (i=0; i<numPts; i++)
      {
      inPts->GetPoint(i,x1);
      newPts->GetPoint(i,x2);
      for (j=0; j<3; j++) x3[j] = x2[j] - x1[j];
      newVectors->SetVector(i,x3);
      }
    output->GetPointData()->SetVectors(newVectors);
    newVectors->Delete();
    }

  output->SetPoints(newPts);
  newPts->Delete();

  output->SetVerts(input->GetVerts());
  output->SetLines(input->GetLines());
  output->SetPolys(input->GetPolys());
  output->SetStrips(input->GetStrips());

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

  os << indent << "Convergence: " << this->Convergence << "\n";
  os << indent << "Number of Iterations: " << this->NumberOfIterations << "\n";
  os << indent << "Expansion Factor: " << this->ExpansionFactor << "\n";
  os << indent << "Contraction Factor: " << this->ContractionFactor << "\n";
  os << indent << "Feature Edge Smoothing: " << (this->FeatureEdgeSmoothing ? "On\n" : "Off\n");
  os << indent << "Feature Angle: " << this->FeatureAngle << "\n";
  os << indent << "Edge Angle: " << this->EdgeAngle << "\n";
  os << indent << "Boundary Smoothing: " << (this->BoundarySmoothing ? "On\n" : "Off\n");
  os << indent << "Generate Error Scalars: " << (this->GenerateErrorScalars ? "On\n" : "Off\n");
  os << indent << "Generate Error Vectors: " << (this->GenerateErrorVectors ? "On\n" : "Off\n");

}
