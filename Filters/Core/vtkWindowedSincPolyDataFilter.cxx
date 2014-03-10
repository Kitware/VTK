/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWindowedSincPolyDataFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkWindowedSincPolyDataFilter.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h"
#include "vtkTriangle.h"
#include "vtkTriangleFilter.h"

vtkStandardNewMacro(vtkWindowedSincPolyDataFilter);

// Construct object with number of iterations 20; passband .1;
// feature edge smoothing turned off; feature

// angle 45 degrees; edge angle 15 degrees; and boundary smoothing turned
// on. Error scalars and vectors are not generated (by default). The
// convergence criterion is 0.0 of the bounding box diagonal.
vtkWindowedSincPolyDataFilter::vtkWindowedSincPolyDataFilter()
{
  this->NumberOfIterations = 20;
  this->PassBand = 0.1;

  this->FeatureAngle = 45.0;
  this->EdgeAngle = 15.0;
  this->FeatureEdgeSmoothing = 0;
  this->BoundarySmoothing = 1;
  this->NonManifoldSmoothing = 0;

  this->GenerateErrorScalars = 0;
  this->GenerateErrorVectors = 0;

  this->NormalizeCoordinates = 0;
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

int vtkWindowedSincPolyDataFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData *input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType numPts, numCells, numPolys, numStrips, i;
  int j, k;
  vtkIdType npts = 0;
  vtkIdType *pts = 0;
  vtkIdType p1, p2;
  double x[3], y[3], deltaX[3], xNew[3];
  double x1[3], x2[3], x3[3], l1[3], l2[3];
  double CosFeatureAngle; //Cosine of angle between adjacent polys
  double CosEdgeAngle; // Cosine of angle between adjacent edges
  int iterationNumber;
  vtkIdType numSimple=0, numBEdges=0, numFixed=0, numFEdges=0;
  vtkPolyData *inMesh = NULL, *Mesh;
  vtkPoints *inPts;
  vtkTriangleFilter *toTris=NULL;
  vtkCellArray *inVerts, *inLines, *inPolys, *inStrips;
  vtkPoints *newPts[4];
  vtkMeshVertexPtr Verts;

  // variables specific to windowed sinc interpolation
  double theta_pb, k_pb, sigma, p_x0[3], p_x1[3], p_x3[3];
  double *w, *c, *cprime;
  int zero, one, two, three;

//
// Check input
//
  numPts=input->GetNumberOfPoints();
  numCells=input->GetNumberOfCells();
  if (numPts < 1 || numCells < 1)
    {
    vtkErrorMacro(<<"No data to smooth!");
    return 1;
    }

  CosFeatureAngle = cos( vtkMath::RadiansFromDegrees( this->FeatureAngle) );
  CosEdgeAngle    = cos( vtkMath::RadiansFromDegrees( this->EdgeAngle) );

  vtkDebugMacro(<<"Smoothing " << numPts << " vertices, " << numCells
               << " cells with:\n"
               << "\tIterations= " << this->NumberOfIterations << "\n"
               << "\tPassBand= " << this->PassBand << "\n"
               << "\tEdge Angle= " << this->EdgeAngle << "\n"
               << "\tBoundary Smoothing "
               << (this->BoundarySmoothing ? "On\n" : "Off\n")
               << "\tFeature Edge Smoothing "
               << (this->FeatureEdgeSmoothing ? "On\n" : "Off\n")
               << "\tNonmanifold Smoothing "
               << (this->NonManifoldSmoothing ? "On\n" : "Off\n")
               << "\tError Scalars "
               << (this->GenerateErrorScalars ? "On\n" : "Off\n")
               << "\tError Vectors "
               << (this->GenerateErrorVectors ? "On\n" : "Off\n"));

  if ( this->NumberOfIterations <= 0 ) //don't do anything!
    {
    output->CopyStructure(input);
    output->GetPointData()->PassData(input->GetPointData());
    output->GetCellData()->PassData(input->GetCellData());
    vtkWarningMacro(<<"Number of iterations == 0: passing data through unchanged");
    return 1;
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

  // check vertices first. Vertices are never smoothed_--------------
  for (inVerts=input->GetVerts(), inVerts->InitTraversal();
  inVerts->GetNextCell(npts,pts); )
    {
    for (j=0; j<npts; j++)
      {
      Verts[pts[j]].type = VTK_FIXED_VERTEX;
      }
    }

  this->UpdateProgress(0.10);

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
          //Verts[pts[j]].edges = new vtkIdList(2,2);
          Verts[pts[j]].edges->SetId(0,pts[j-1]);
          Verts[pts[j]].edges->SetId(1,pts[j+1]);
          }
        } //if simple vertex

      else if ( Verts[pts[j]].type == VTK_FEATURE_EDGE_VERTEX )
        { //multiply connected, becomes fixed!
        Verts[pts[j]].type = VTK_FIXED_VERTEX;
        Verts[pts[j]].edges->Delete();
        Verts[pts[j]].edges = NULL;
        }

      } //for all points in this line
    } //for all lines

  this->UpdateProgress(0.25);

  // now polygons and triangle strips-------------------------------
  inPolys=input->GetPolys();
  numPolys = inPolys->GetNumberOfCells();
  inStrips=input->GetStrips();
  numStrips = inStrips->GetNumberOfCells();

  if ( numPolys > 0 || numStrips > 0 )
    { //build cell structure
    vtkCellArray *polys;
    vtkIdType cellId;
    int numNei, nei, edge;
    vtkIdType numNeiPts;
    vtkIdType *neiPts;
    double normal[3], neiNormal[3];
    vtkIdList *neighbors;

    inMesh = vtkPolyData::New();
    inMesh->SetPoints(inPts);
    inMesh->SetPolys(inPolys);
    Mesh = inMesh;
    neighbors = vtkIdList::New();
    neighbors->Allocate(VTK_CELL_SIZE);

    if ( (numStrips = inStrips->GetNumberOfCells()) > 0 )
      { // convert data to triangles
      inMesh->SetStrips(inStrips);
      toTris = vtkTriangleFilter::New();
      toTris->SetInputData(inMesh);
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
          Verts[p1].edges->Allocate(16,6);
          // Verts[p1].edges = new vtkIdList(6,6);
          }
        if ( Verts[p2].edges == NULL )
          {
          Verts[p2].edges = vtkIdList::New();
          Verts[p2].edges->Allocate(16,6);
          // Verts[p2].edges = new vtkIdList(6,6);
          }

        Mesh->GetCellEdgeNeighbors(cellId,p1,p2,neighbors);
        numNei = neighbors->GetNumberOfIds();

        edge = VTK_SIMPLE_VERTEX;
        if ( numNei == 0 )
          {
          edge = VTK_BOUNDARY_EDGE_VERTEX;
          }

        else if ( numNei >= 2 )
          {
          // non-manifold case, check nonmanifold smoothing state
          if (!this->NonManifoldSmoothing)
            {
            // check to make sure that this edge hasn't been marked already
            for (j=0; j < numNei; j++)
              {
              if ( neighbors->GetId(j) < cellId )
                {
                break;
                }
              }
            if ( j >= numNei )
              {
              edge = VTK_FEATURE_EDGE_VERTEX;
              }
            }
          }

        else if ( numNei == 1 && (nei=neighbors->GetId(0)) > cellId )
          {
          if (this->FeatureEdgeSmoothing)
            {
            vtkPolygon::ComputeNormal(inPts,npts,pts,normal);
            Mesh->GetCellPoints(nei,numNeiPts,neiPts);
            vtkPolygon::ComputeNormal(inPts,numNeiPts,neiPts,neiNormal);

            if ( vtkMath::Dot(normal,neiNormal) <= CosFeatureAngle )
              {
              edge = VTK_FEATURE_EDGE_VERTEX;
              }
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
            {
            Verts[p1].type = VTK_BOUNDARY_EDGE_VERTEX;
            }
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
            {
            Verts[p2].type = VTK_BOUNDARY_EDGE_VERTEX;
            }
          }
        }
      }

    //    delete inMesh; // delete this later, windowed sinc smoothing needs it
    if (toTris)
      {
      toTris->Delete();
      }
    neighbors->Delete();
    }//if strips or polys

  this->UpdateProgress(0.50);

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
        // can only smooth edges on 2-manifold surfaces
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
        if ((vtkMath::Normalize(l1) >= 0.0) && (vtkMath::Normalize(l2) >= 0.0)
            && (vtkMath::Dot(l1,l2) < CosEdgeAngle))
          {
          numFixed++;
          Verts[i].type = VTK_FIXED_VERTEX;
          }
        else
          {
          if ( Verts[i].type == VTK_FEATURE_EDGE_VERTEX )
            {
            numFEdges++;
            }
          else
            {
            numBEdges++;
            }
          }
        }//if along edge
      }//if edge vertex
    }//for all points

  vtkDebugMacro(<<"Found\n\t" << numSimple << " simple vertices\n\t"
                << numFEdges << " feature edge vertices\n\t"
                << numBEdges << " boundary edge vertices\n\t"
                << numFixed << " fixed vertices\n\t");
//
// Perform Windowed Sinc function interpolation
//
  vtkDebugMacro(<<"Beginning smoothing iterations...");

  // need 4 vectors of points
  zero=0; one=1; two=2; three=3;

  newPts[0] = vtkPoints::New();
  newPts[0]->SetNumberOfPoints(numPts);
  newPts[1] = vtkPoints::New();
  newPts[1]->SetNumberOfPoints(numPts);
  newPts[2] = vtkPoints::New();
  newPts[2]->SetNumberOfPoints(numPts);
  newPts[3] = vtkPoints::New();
  newPts[3]->SetNumberOfPoints(numPts);

  // Get the center and length of the input dataset
  double *inCenter = input->GetCenter();
  double inLength = input->GetLength();

  if (!this->NormalizeCoordinates)
    {
    for (i=0; i<numPts; i++) //initialize to old coordinates
      {
      newPts[zero]->SetPoint(i,inPts->GetPoint(i));
      }
    }
  else
    {
    // center the data and scale to be within unit cube [-1, 1]
    double normalizedPoint[3];
    for (i=0; i<numPts; i++) //initialize to old coordinates
      {
      inPts->GetPoint(i, normalizedPoint);
      for (j=0; j<3; ++j)
        {
        normalizedPoint[j] = (normalizedPoint[j] - inCenter[j]) / inLength;
        }
      newPts[zero]->SetPoint(i,normalizedPoint);
      }
    }

  // Smooth with a low pass filter defined as a windowed sinc function.
  // Taubin describes this methodology is the IBM tech report RC-20404
  // (#90237, dated 3/12/96) "Optimal Surface Smoothing as Filter Design"
  // G. Taubin, T. Zhang and G. Golub. (Zhang and Golub are at Stanford
  // University)

  // The formulas here follow the notation of Taubin's TR, i.e.
  // newPts[zero], newPts[one], etc.

  // calculate weights and filter coefficients
  k_pb = this->PassBand;   // reasonable default for k_pb in [0, 2] is 0.1
  theta_pb = acos( 1.0 - 0.5 * k_pb ); // theta_pb in [0, M_PI/2]

  //vtkDebugMacro(<< "theta_pb = " << theta_pb);

  w = new double[this->NumberOfIterations+1];
  c = new double[this->NumberOfIterations+1];
  cprime = new double[this->NumberOfIterations+1];

  double zerovector[3];
  zerovector[0] = zerovector[1] = zerovector[2] = 0.0;

  //
  // Calculate the weights and the Chebychev coefficients c.
  //

  // Windowed sinc function weights. This is for a Hamming window. Other
  // windowing function could be implemented here.
  for (i=0; i <= (this->NumberOfIterations); i++)
    {
    w[i] = 0.54 + 0.46*cos(((double)i)*vtkMath::Pi()
                           /(double)(this->NumberOfIterations+1));
    }

  // Calculate the optimal sigma (offset or fudge factor for the filter).
  // This is a Newton-Raphson Search.
  double f_kpb = 0.0, fprime_kpb;
  int done = 0;
  sigma = 0.0;

  for (j=0; !done && (j<500); j++)
    {
    // Chebyshev coefficients
    c[0] = w[0]*(theta_pb + sigma)/vtkMath::Pi();
    for (i=1; i <= this->NumberOfIterations; i++)
      {
      c[i] = 2.0*w[i]*sin(((double)i)*(theta_pb+sigma))/
        (((double)i)*vtkMath::Pi());
      }

    // calculate the Chebyshev coefficients for the derivative of the filter
    cprime[this->NumberOfIterations] = 0.0;
    cprime[this->NumberOfIterations-1] = 0.0;
    if (this->NumberOfIterations > 1)
      {
      cprime[this->NumberOfIterations-2] = 2.0*(this->NumberOfIterations-1)
        * c[this->NumberOfIterations-1];
      }
    for (i=this->NumberOfIterations-3; i>=0; i--)
      {
      cprime[i] = cprime[i+2] + 2.0*(i+1)*c[i+1];
      }
    // Evaluate the filter and its derivative at k_pb (note the discrepancy
    // of calculating the c's based on theta_pb + sigma and evaluating the
    // filter at k_pb (which is equivalent to theta_pb)
    f_kpb = 0.0;
    fprime_kpb = 0.0;
    f_kpb += c[0];
    fprime_kpb += cprime[0];
    for (i=1; i<= this->NumberOfIterations; i++)
      {
      if (i==1)
        {
        f_kpb += c[i]*(1.0 - 0.5*k_pb);
        fprime_kpb += cprime[i]*(1.0 - 0.5*k_pb);
        }
      else
        {
        f_kpb += c[i]*cos(((double) i)*acos(1.0-0.5*k_pb));
        fprime_kpb += cprime[i]*cos(((double) i)*acos(1.0-0.5*k_pb));
        }
      }
    // if f_kpb is not close enough to 1.0, then adjust sigma
    if (this->NumberOfIterations > 1)
      {
      if (fabs(f_kpb - 1.0) >= 1e-3)
        {
        sigma -= (f_kpb - 1.0)/fprime_kpb;   // Newton-Rhapson (want f=1)
        }
      else
        {
        done = 1;
        }
      }
    else
      {
      // Order of Chebyshev is 1. Can't use Newton-Raphson to find an
      // optimal sigma. Object will most likely shrink.
      done = 1;
      sigma = 0.0;
      }
    }
  if (fabs(f_kpb - 1.0) >= 1e-3)
    {
    vtkErrorMacro(<< "An optimal offset for the smoothing filter could not be found.  Unpredictable smoothing/shrinkage may result.");
    }

  // first iteration
  for (i=0; i<numPts; i++)
    {
    if ( Verts[i].edges != NULL &&
         (npts = Verts[i].edges->GetNumberOfIds()) > 0 )
      {
      // point is allowed to move
      newPts[zero]->GetPoint(i, x); //use current points
      deltaX[0] = deltaX[1] = deltaX[2] = 0.0;

      // calculate the negative of the laplacian
      for (j=0; j<npts; j++) //for all connected points
        {
        newPts[zero]->GetPoint(Verts[i].edges->GetId(j), y);
        for (k=0; k<3; k++)
          {
          deltaX[k] += (x[k] - y[k]) / npts;
          }
        }
      // newPts[one] = newPts[zero] - 0.5 newPts[one]
      for (k=0; k<3; k++)
        {
        deltaX[k] = x[k] - 0.5*deltaX[k];
        }
      newPts[one]->SetPoint(i, deltaX);

      // calculate newPts[three] = c0 newPts[zero] + c1 newPts[one]
      for (k=0; k < 3; k++)
        {
        deltaX[k] = c[0]*x[k] + c[1]*deltaX[k];
        }
      if (Verts[i].type == VTK_FIXED_VERTEX)
        {
        newPts[three]->SetPoint(i, newPts[zero]->GetPoint(i));
        }
      else
        {
        newPts[three]->SetPoint(i, deltaX);
        }
      }//if can move point
    else
      {
      // point is not allowed to move, just use the old point...
      // (zero out the Laplacian)
      newPts[one]->SetPoint(i, zerovector);
      newPts[three]->SetPoint(i, newPts[zero]->GetPoint(i));
      }
    }//for all points

  // for the rest of the iterations
  for ( iterationNumber=2;
        iterationNumber <= this->NumberOfIterations;
        iterationNumber++ )
    {
    if ( iterationNumber && !(iterationNumber % 5) )
      {
      this->UpdateProgress (0.5 + 0.5*iterationNumber/this->NumberOfIterations);
      if (this->GetAbortExecute())
        {
        break;
        }
      }

    for (i=0; i<numPts; i++)
      {
      if ( Verts[i].edges != NULL &&
           (npts = Verts[i].edges->GetNumberOfIds()) > 0 )
        {
        // point is allowed to move
        newPts[zero]->GetPoint(i, p_x0); //use current points
        newPts[one]->GetPoint(i, p_x1);

        deltaX[0] = deltaX[1] = deltaX[2] = 0.0;

        // calculate the negative laplacian of x1
        for (j=0; j<npts; j++)
          {
          newPts[one]->GetPoint(Verts[i].edges->GetId(j), y);
          for (k=0; k<3; k++)
            {
            deltaX[k] += (p_x1[k] - y[k]) / npts;
            }
          }//for all connected points

        // Taubin:  x2 = (x1 - x0) + (x1 - x2)
        for (k=0; k<3; k++)
          {
          deltaX[k] = p_x1[k] - p_x0[k] + p_x1[k] - deltaX[k];
          }
        newPts[two]->SetPoint(i, deltaX);

        // smooth the vertex (x3 = x3 + cj x2)
        newPts[three]->GetPoint(i, p_x3);
        for (k=0;k<3;k++)
          {
          xNew[k] = p_x3[k] + c[iterationNumber] * deltaX[k];
          }
        if (Verts[i].type != VTK_FIXED_VERTEX)
          {
          newPts[three]->SetPoint(i,xNew);
          }
        }//if can move point
      else
        {
        // point is not allowed to move, just use the old point...
        // (zero out the Laplacian)
        newPts[one]->SetPoint(i, zerovector);
        newPts[two]->SetPoint(i, zerovector);
        }
      }//for all points

    // update the pointers. three is always three. all other pointers
    // shift by one and wrap.
    zero = (1+zero)%3;
    one = (1+one)%3;
    two = (1+two)%3;

    }//for all iterations or until converge

  // move the iteration count back down so that it matches the
  // actual number of iterations executed
  --iterationNumber;

  // set zero to three so the correct set of positions is outputted
  zero = three;

  delete [] w;
  delete [] c;
  delete [] cprime;

  vtkDebugMacro(<<"Performed " << iterationNumber << " smoothing passes");

  // if we scaled the data down to the unit cube, then scale data back
  // up to the original space
  if (this->NormalizeCoordinates)
    {
    // Re-position the coordinated
    double repositionedPoint[3];
    for (i=0; i<numPts; i++)
      {
      newPts[zero]->GetPoint(i, repositionedPoint);
      for (j=0; j<3; ++j)
        {
        repositionedPoint[j] = repositionedPoint[j] * inLength + inCenter[j];
        }
      newPts[zero]->SetPoint(i,repositionedPoint);
      }
    }

//
// Update output. Only point coordinates have changed.
//
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  if ( this->GenerateErrorScalars )
    {
    vtkFloatArray *newScalars = vtkFloatArray::New();
    newScalars->SetNumberOfTuples(numPts);
    for (i=0; i<numPts; i++)
      {
      inPts->GetPoint(i,x1);
      newPts[zero]->GetPoint(i,x2);
      newScalars->SetComponent(i,0,
                               sqrt(vtkMath::Distance2BetweenPoints(x1,x2)));
      }
    int idx = output->GetPointData()->AddArray(newScalars);
    output->GetPointData()->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
    newScalars->Delete();
    }

  if ( this->GenerateErrorVectors )
    {
    vtkFloatArray *newVectors = vtkFloatArray::New();
    newVectors->SetNumberOfComponents(3);
    newVectors->SetNumberOfTuples(numPts);
    for (i=0; i<numPts; i++)
      {
      inPts->GetPoint(i,x1);
      newPts[zero]->GetPoint(i,x2);
      for (j=0; j<3; j++)
        {
        x3[j] = x2[j] - x1[j];
        }
      newVectors->SetTuple(i,x3);
      }
    output->GetPointData()->SetVectors(newVectors);
    newVectors->Delete();
    }

  output->SetPoints(newPts[zero]);
  newPts[0]->Delete();
  newPts[1]->Delete();
  newPts[2]->Delete();
  newPts[3]->Delete();

  output->SetVerts(input->GetVerts());
  output->SetLines(input->GetLines());
  output->SetPolys(input->GetPolys());
  output->SetStrips(input->GetStrips());

  // finally delete the constructed (local) mesh
  inMesh->Delete();

  //free up connectivity storage
  for (i=0; i<numPts; i++)
    {
    if ( Verts[i].edges != NULL ) {Verts[i].edges->Delete();}
    }
  delete [] Verts;

  return 1;
}

void vtkWindowedSincPolyDataFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Number of Iterations: " << this->NumberOfIterations << "\n";
  os << indent << "Passband: " << this->PassBand << "\n";
  os << indent << "Normalize Coordinates: " << (this->NormalizeCoordinates ? "On\n" : "Off\n");
  os << indent << "Feature Edge Smoothing: " << (this->FeatureEdgeSmoothing ? "On\n" : "Off\n");
  os << indent << "Feature Angle: " << this->FeatureAngle << "\n";
  os << indent << "Edge Angle: " << this->EdgeAngle << "\n";
  os << indent << "Boundary Smoothing: " << (this->BoundarySmoothing ? "On\n" : "Off\n");
  os << indent << "Nonmanifold Smoothing: " << (this->NonManifoldSmoothing ? "On\n" : "Off\n");
  os << indent << "Generate Error Scalars: " << (this->GenerateErrorScalars ? "On\n" : "Off\n");
  os << indent << "Generate Error Vectors: " << (this->GenerateErrorVectors ? "On\n" : "Off\n");
}
