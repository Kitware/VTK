/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUncertaintyTubeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkUncertaintyTubeFilter.h"

#include "vtkCellArray.h"
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkPolyData.h"
#include "vtkPolyLine.h"

vtkStandardNewMacro(vtkUncertaintyTubeFilter);

//
// Special classes for manipulating data
//BTX
class vtkTubePoint { //;prevent man page generation
public:
  vtkTubePoint(); // method sets up storage
  vtkTubePoint &operator=(const vtkTubePoint& hp); //for resizing

  double   X[3];    // position
  double   *V[3];   // pointers to eigenvectors
  double   V0[3];   // the line tangent
  double   V1[3];   // the normal to the line
  double   V2[3];   // another, orthogonal normal to the line
  double   Vector[3];//local uncertainty vector
};
//ETX

class vtkTubeArray { //;prevent man page generation
public:
  vtkTubeArray();
  ~vtkTubeArray()
    {
      if (this->Array)
        {
        delete [] this->Array;
        }
    };
  vtkIdType GetNumberOfPoints() {return this->MaxId + 1;};
  vtkTubePoint *GetTubePoint(vtkIdType i) {return this->Array + i;};
  vtkTubePoint *InsertNextTubePoint()
    {
    if ( ++this->MaxId >= this->Size )
      {
      this->Resize(this->MaxId);
      }
    return this->Array + this->MaxId;
    }
  vtkTubePoint *Resize(vtkIdType sz); //reallocates data
  void Reset() {this->MaxId = -1;}

  vtkTubePoint *Array;  // pointer to data
  vtkIdType MaxId;       // maximum index inserted thus far
  vtkIdType Size;        // allocated size of data
  vtkIdType Extend;      // grow array by this amount
};

//-----------------------------------------------------------------------------
vtkTubePoint::vtkTubePoint()
{
  this->V[0] = this->V0;
  this->V[1] = this->V1;
  this->V[2] = this->V2;
}

//-----------------------------------------------------------------------------
vtkTubePoint& vtkTubePoint::operator=(const vtkTubePoint& hp)
{
  int i, j;

  for (i=0; i<3; i++)
    {
    this->X[i] = hp.X[i];
    for (j=0; j<3; j++)
      {
      this->V[j][i] = hp.V[j][i];
      }
    }

  return *this;
}

//-----------------------------------------------------------------------------
vtkTubeArray::vtkTubeArray()
{
  this->MaxId = -1;
  this->Array = new vtkTubePoint[1000];
  this->Size = 1000;
  this->Extend = 5000;
}

//-----------------------------------------------------------------------------
vtkTubePoint *vtkTubeArray::Resize(vtkIdType sz)
{
  vtkTubePoint *newArray;
  vtkIdType newSize, i;

  if (sz >= this->Size)
    {
    newSize = this->Size +
      this->Extend*(((sz-this->Size)/this->Extend)+1);
    }
  else
    {
    newSize = sz;
    }

  newArray = new vtkTubePoint[newSize];

  for (i=0; i<sz; i++)
    {
    newArray[i] = this->Array[i];
    }

  this->Size = newSize;
  delete [] this->Array;
  this->Array = newArray;

  return this->Array;
}

//-----------------------------------------------------------------------------
// Construct object with initial starting position (0,0,0); integration step
// length 0.2; step length 0.01; forward integration; terminal eigenvalue 0.0;
// number of sides 6; radius 0.5; and logarithmic scaling off.
vtkUncertaintyTubeFilter::vtkUncertaintyTubeFilter()
{
  this->Tubes = NULL;

  this->NumberOfSides = 12;
}

//-----------------------------------------------------------------------------
vtkUncertaintyTubeFilter::~vtkUncertaintyTubeFilter()
{
  if ( this->Tubes )
    {
    delete [] this->Tubes;
    }
}

//-----------------------------------------------------------------------------
static double IntersectEllipse(double vector[3], double v[3])
{
  double a = vector[0];
  double b = vector[1];
  double c = vector[2];

  double num = a*a * b*b * c*c;
  double den = v[0]*v[0]*b*b*c*c + v[1]*v[1]*a*a*c*c + v[2]*v[2]*a*a*b*b;

  if ( den <= 0.0 )
    {
    return 0.0;
    }
  else
    {
    return sqrt(num/den);
    }
}

//-----------------------------------------------------------------------------
int vtkUncertaintyTubeFilter::RequestData(vtkInformation *vtkNotUsed(request),
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

  vtkPointData *pd=input->GetPointData();
  vtkPointData *outPD=output->GetPointData();
  vtkCellData *cd=input->GetCellData();
  vtkCellData *outCD=output->GetCellData();
  vtkDataArray *inVectors;
  vtkTubePoint *sPrev, *sPtr, *sNext;
  int k;
  double v0[3], v1[3];

  vtkDebugMacro(<<"Generating uncertainty tubes");

  vtkPoints *inPts = input->GetPoints();
  vtkCellArray *inLines = input->GetLines();
  if ( !inPts || !inLines )
    {
    return 1;
    }

  vtkIdType numPts = inPts->GetNumberOfPoints();
  vtkIdType numLines = inLines->GetNumberOfCells();
  if ( numPts < 1 || numLines < 1 )
    {
    return 1;
    }

  if ( ! (inVectors=pd->GetVectors()) )
    {
    return 1;
    }

  // Initialize the data
  outPD->CopyNormalsOff();
  outPD->CopyAllocate(pd);
  outCD->CopyAllocate(cd);

  this->NumberOfTubes = numLines;
  this->Tubes = new vtkTubeArray[this->NumberOfTubes];

  vtkDoubleArray *newNormals = vtkDoubleArray::New();
  newNormals->SetName("TubeNormals");
  newNormals->SetNumberOfComponents(3);
  newNormals->Allocate(numPts);

  vtkIdType npts, *pts, idx;
  vtkCellArray *singlePolyline = vtkCellArray::New();
  double *normal;
  for (k=0, inLines->InitTraversal();
       inLines->GetNextCell(npts,pts); k++)
    {
    singlePolyline->Reset(); //avoid instantiation
    singlePolyline->InsertNextCell(npts,pts); //avoid crossing confusion
    if ( ! vtkPolyLine::GenerateSlidingNormals(inPts,singlePolyline,newNormals) )
      {
      vtkWarningMacro("Could not generate normals for line. "
                      "Skipping to next.");
      continue; //skip tubing this polyline
      }
    for (idx=0; idx < npts; idx++)
      {
      // This advances to the next point
      sPtr = this->Tubes[k].InsertNextTubePoint();
      inPts->GetPoint(pts[idx],sPtr->X);

      // The normal to the line
      normal = newNormals->GetTuple(idx);
      sPtr->V[1][0] = normal[0];
      sPtr->V[1][1] = normal[1];
      sPtr->V[1][2] = normal[2];
      }

    // Okay build the rest of the coordinate system. We've got to find the
    // vector along the line segment, and then cross the normal/vector to
    // get the third axis.
    for (idx=0; idx<npts; idx++)
      {
      sPtr = this->Tubes[k].GetTubePoint(idx);
      inVectors->GetTuple(pts[idx],sPtr->Vector);
      if ( idx == 0 )
        {
        sNext = this->Tubes[k].GetTubePoint(1);
        sPtr->V[0][0] = sNext->X[0] - sPtr->X[0];
        sPtr->V[0][1] = sNext->X[1] - sPtr->X[1];
        sPtr->V[0][2] = sNext->X[2] - sPtr->X[2];
        }
      else if ( idx == (npts-1) )
        {
        sPrev = this->Tubes[k].GetTubePoint(npts-2);
        sPtr->V[0][0] = sPtr->X[0] - sPrev->X[0];
        sPtr->V[0][1] = sPtr->X[1] - sPrev->X[1];
        sPtr->V[0][2] = sPtr->X[2] - sPrev->X[2];
        }
      else
        {
        sPrev = this->Tubes[k].GetTubePoint(idx-1);
        sNext = this->Tubes[k].GetTubePoint(idx+1);
        v0[0] = sPtr->X[0] - sPrev->X[0];
        v0[1] = sPtr->X[1] - sPrev->X[1];
        v0[2] = sPtr->X[2] - sPrev->X[2];
        vtkMath::Normalize(v0);
        v1[0] = sNext->X[0] - sPtr->X[0];
        v1[1] = sNext->X[1] - sPtr->X[1];
        v1[2] = sNext->X[2] - sPtr->X[2];
        vtkMath::Normalize(v1);
        sPtr->V[0][0] = (v0[0]+v1[0]) / 2.0; //average vector
        sPtr->V[0][1] = (v0[1]+v1[1]) / 2.0;
        sPtr->V[0][2] = (v0[2]+v1[2]) / 2.0;
        }
      vtkMath::Normalize(sPtr->V[0]);

      // Produce orthogonal axis
      vtkMath::Cross(sPtr->V[0], sPtr->V[1], sPtr->V[2]);
      vtkMath::Normalize(sPtr->V[2]);
      vtkMath::Cross(sPtr->V[2],sPtr->V[0],sPtr->V[1]);
      vtkMath::Normalize(sPtr->V[1]);

      }//for all points in polyline
    }//for all polylines
  newNormals->Delete();
  singlePolyline->Delete();

  // Build the uncertainty tubes
  int retval = this->BuildTubes(pd,outPD,cd,outCD,output);

  return retval;
}

//-----------------------------------------------------------------------------
int vtkUncertaintyTubeFilter::BuildTubes(vtkPointData *pd, vtkPointData *outPD,
                                         vtkCellData *cd, vtkCellData *outCD,
                                         vtkPolyData *output)
{
  vtkTubePoint *sPtr;
  vtkPoints *newPts;
  vtkFloatArray *newNormals;
  vtkCellArray *newStrips;
  vtkIdType i, npts, ptOffset=0;
  int cellId, j, id, k, i1, i2;
  double x[3], r1[3], r2[3];
  double xT[3], normal[3], *vector, t;
  double theta=2.0*vtkMath::Pi()/this->NumberOfSides;
  vtkIdType numPts;

  // Initialize
  //
  vtkDebugMacro(<<"Creating uncertainty tubes");
  if ( this->NumberOfTubes <= 0 )
    {
    return 0;
    }


  // Allocate
  //
  newPts  = vtkPoints::New();
  newPts ->Allocate(2500);

  newNormals = vtkFloatArray::New();
  newNormals->SetNumberOfComponents(3);
  newNormals->Allocate(7500);
  newStrips = vtkCellArray::New();
  newStrips->Allocate(newStrips->EstimateSize(3*this->NumberOfTubes,
                                              VTK_CELL_SIZE));
  //
  // Loop over all polylines generating points
  //
  for (cellId=0; cellId < this->NumberOfTubes; cellId++)
    {
    if ( (numPts=this->Tubes[cellId].GetNumberOfPoints()) < 2 )
      {
      continue;
      }

    for ( npts=0, i=0; i < numPts; i++)
      {
      sPtr = this->Tubes[cellId].GetTubePoint(i);
      for (j=0; j<3; j++) //compute point in center of tube
        {
        x[j] = sPtr->X[j];
        r1[j] = sPtr->V[1][j];
        r2[j] = sPtr->V[2][j];
        }
      vector = sPtr->Vector;

      // construct points around tube
      for (k=0; k < this->NumberOfSides; k++)
        {
        for (j=0; j<3; j++)
          {
          normal[j] = r1[j]*cos((double)k*theta) + r2[j]*sin((double)k*theta);
          }
        vtkMath::Normalize(normal);
        t = IntersectEllipse(vector,normal);
        xT[0] = x[0] + 0.5*t*normal[0];
        xT[1] = x[1] + 0.5*t*normal[1];
        xT[2] = x[2] + 0.5*t*normal[2];
        id = newPts->InsertNextPoint(xT);
        outPD->CopyData(pd, i, id);
        vtkMath::Normalize(normal);
        newNormals->InsertTuple(id,normal);
        }
      npts++;
      } //for this polyline

    // Generate the strips for this tube
    //
    for (k=0; k<this->NumberOfSides; k++)
      {
      i1 = (k+1) % this->NumberOfSides;
      id = newStrips->InsertNextCell(npts*2);
      outCD->CopyData(cd, cellId, id);
      for (i=0; i < npts; i++)
        {
        i2 = i*this->NumberOfSides;
        newStrips->InsertCellPoint(ptOffset+i2+k);
        newStrips->InsertCellPoint(ptOffset+i2+i1);
        }
      }//for all tube sides

    ptOffset += this->NumberOfSides*npts;

    } //for all tubes

  // Update ourselves
  //
  output->SetPoints(newPts);
  newPts->Delete();

  output->SetStrips(newStrips);
  newStrips->Delete();

  outPD->SetNormals(newNormals);
  newNormals->Delete();

  output->Squeeze();

  return 1;
}

//-----------------------------------------------------------------------------
void vtkUncertaintyTubeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Number Of Sides: " << this->NumberOfSides << "\n";

}
