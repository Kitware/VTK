/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSplineFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSplineFilter.h"

#include "vtkCardinalSpline.h"
#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"

vtkStandardNewMacro(vtkSplineFilter);
vtkCxxSetObjectMacro(vtkSplineFilter,Spline,vtkSpline);

vtkSplineFilter::vtkSplineFilter()
{
  this->Subdivide = VTK_SUBDIVIDE_SPECIFIED;
  this->MaximumNumberOfSubdivisions = VTK_LARGE_INTEGER;
  this->NumberOfSubdivisions = 100;
  this->Length = 0.1;
  this->GenerateTCoords = VTK_TCOORDS_FROM_NORMALIZED_LENGTH;
  this->TextureLength = 1.0;

  this->Spline = vtkCardinalSpline::New();
  this->TCoordMap = vtkFloatArray::New();
}

vtkSplineFilter::~vtkSplineFilter()
{
  if (this->Spline)
    {
    this->Spline->Delete();
    this->Spline = 0;
    }

  if (this->TCoordMap)
    {
    this->TCoordMap->Delete();
    this->TCoordMap = 0;
    }
}

int vtkSplineFilter::RequestData(
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

  vtkPointData *pd=input->GetPointData();
  vtkPointData *outPD=output->GetPointData();
  vtkCellData *cd=input->GetCellData();
  vtkCellData *outCD=output->GetCellData();
  vtkCellArray *inLines;
  
  vtkPoints *inPts;
  vtkIdType numLines;
  vtkCellArray *newLines;
  vtkIdType numNewPts, numNewCells;
  vtkPoints *newPts;
  vtkIdType npts=0, *pts=NULL;
  vtkIdType offset=0;
  vtkFloatArray *newTCoords=NULL;
  int abort=0;
  vtkIdType inCellId, numGenPts;
  int genTCoords = VTK_TCOORDS_OFF;

  // Check input and initialize
  //
  vtkDebugMacro(<<"Splining polylines");

  if ( !(inPts=input->GetPoints()) || inPts->GetNumberOfPoints() < 1 ||
      !(inLines = input->GetLines()) || 
       (numLines = inLines->GetNumberOfCells()) < 1 )
    {
    return 1;
    }

  if ( this->Spline == NULL )
    {
    vtkWarningMacro(<< "Need to specify a spline!");
    return 1;
    }

  // Create the geometry and topology
  numNewPts = this->NumberOfSubdivisions * numLines;
  newPts = vtkPoints::New();
  newPts->Allocate(numNewPts);
  newLines = vtkCellArray::New();
  newLines->Allocate(newLines->EstimateSize(1,numNewPts));

  // Point data
  if ( (this->GenerateTCoords == VTK_TCOORDS_FROM_SCALARS &&
        pd->GetScalars() != NULL) ||
       (this->GenerateTCoords == VTK_TCOORDS_FROM_LENGTH ||
        this->GenerateTCoords == VTK_TCOORDS_FROM_NORMALIZED_LENGTH) )
    {
    genTCoords = this->GenerateTCoords;
    newTCoords = vtkFloatArray::New();
    newTCoords->SetNumberOfComponents(2);
    newTCoords->Allocate(numNewPts);
    outPD->CopyTCoordsOff();
    }
  outPD->InterpolateAllocate(pd,numNewPts);
  this->TCoordMap->Allocate(VTK_CELL_SIZE);

  // Copy cell data
  numNewCells = numLines;
  outCD->CopyNormalsOff();
  outCD->CopyAllocate(cd,numNewCells);

  // Set up the splines
  this->XSpline = this->Spline->NewInstance();
  this->XSpline->DeepCopy(this->Spline);
  this->YSpline = this->Spline->NewInstance();
  this->YSpline->DeepCopy(this->Spline);
  this->ZSpline = this->Spline->NewInstance();
  this->ZSpline->DeepCopy(this->Spline);

  //  Create points along each polyline.
  //
  for (inCellId=0, inLines->InitTraversal(); 
       inLines->GetNextCell(npts,pts) && !abort; inCellId++)
    {
      this->UpdateProgress(static_cast<double>(inCellId)/numLines);
    abort = this->GetAbortExecute();

    if (npts < 2)
      {
      vtkWarningMacro(<< "Less than two points in line!");
      continue; //skip tubing this polyline
      }

    // Generate the points around the polyline. The strip is not created
    // if the polyline is bad.
    //
    this->TCoordMap->Reset();
    numGenPts = this->GeneratePoints(offset, npts, pts, inPts, newPts,
                                     pd, outPD, genTCoords, newTCoords);
    if ( ! numGenPts )
      {
      //vtkWarningMacro(<< "Could not generate points!");
      continue; //skip splining 
      }
      
    // Generate the polyline
    //
    this->GenerateLine(offset,numGenPts,inCellId,cd,outCD,newLines);

    // Compute the new offset for the next polyline
    offset += numGenPts;

    }//for all polylines

  // Update ourselves
  //
  this->TCoordMap->Initialize();

  this->XSpline->Delete();
  this->YSpline->Delete();
  this->ZSpline->Delete();

  output->SetPoints(newPts);
  newPts->Delete();

  output->SetLines(newLines);
  newLines->Delete();

  if ( newTCoords )
    {
    outPD->SetTCoords(newTCoords);
    newTCoords->Delete();
    }

  output->Squeeze();

  return 1;
}

int vtkSplineFilter::GeneratePoints(vtkIdType offset, vtkIdType npts, 
                                    vtkIdType *pts, vtkPoints *inPts, 
                                    vtkPoints *newPts, vtkPointData *pd, 
                                    vtkPointData *outPD, int genTCoords,
                                    vtkFloatArray *newTCoords)
{
  vtkIdType i;

  // Initialize the splines
  this->XSpline->RemoveAllPoints();
  this->YSpline->RemoveAllPoints();
  this->ZSpline->RemoveAllPoints();
    
  // Compute the length of the resulting spline
  double xPrev[3], x[3], length=0.0, len, t, tc, dist;
  inPts->GetPoint(pts[0],xPrev);
  for (i=1; i < npts; i++)
    {
    inPts->GetPoint(pts[i],x);
    len = sqrt(vtkMath::Distance2BetweenPoints(x,xPrev));
    length += len;
    xPrev[0]=x[0]; xPrev[1]=x[1]; xPrev[2]=x[2];
    }
  if ( length <= 0.0 )
    {
    return 0; //failure
    }

  // Now we insert points into the splines with the parametric coordinate
  // based on (polyline) length. We keep track of the parametric coordinates
  // of the points for later point interpolation.
  inPts->GetPoint(pts[0],xPrev);
  for (len=0,i=0; i < npts; i++)
    {
    inPts->GetPoint(pts[i],x);
    dist = sqrt(vtkMath::Distance2BetweenPoints(x,xPrev));
    if (i > 0 && dist == 0)
      {
      continue;
      }
    len += dist;
    t = len/length;
    this->TCoordMap->InsertValue(i,t);

    this->XSpline->AddPoint(t,x[0]);
    this->YSpline->AddPoint(t,x[1]);
    this->ZSpline->AddPoint(t,x[2]);
    
    xPrev[0]=x[0]; xPrev[1]=x[1]; xPrev[2]=x[2];
    }
  
  // Compute the number of subdivisions
  vtkIdType numDivs, numNewPts;
  if ( this->Subdivide == VTK_SUBDIVIDE_SPECIFIED )
    {
    numDivs = this->NumberOfSubdivisions;
    }
  else
    {
    numDivs = static_cast<int>(length / this->Length);
    }
  numDivs = ( numDivs < 1 ? 1 : (numDivs > this->MaximumNumberOfSubdivisions ? 
                                 this->MaximumNumberOfSubdivisions : numDivs));

  // Now compute the new points
  numNewPts = numDivs + 1;
  vtkIdType idx;
  double s, s0=0.0;
  if ( genTCoords == VTK_TCOORDS_FROM_SCALARS )
    {
    s0=pd->GetScalars()->GetTuple1(pts[0]);
    }
  double tLo = this->TCoordMap->GetValue(0);
  double tHi = this->TCoordMap->GetValue(1);
  for (idx=0, i=0; i < numNewPts; i++)
    {
      t = static_cast<double>(i) / numDivs;
    x[0] = this->XSpline->Evaluate(t);
    x[1] = this->YSpline->Evaluate(t);
    x[2] = this->ZSpline->Evaluate(t);
    newPts->InsertPoint(offset+i,x);

    // interpolate point data
    while ( t > tHi && idx < (npts-2))
      {
      idx++;
      tLo = this->TCoordMap->GetValue(idx);
      tHi = this->TCoordMap->GetValue(idx+1);
      }
    tc = (t - tLo) / (tHi - tLo);
    outPD->InterpolateEdge(pd,offset+i,pts[idx],pts[idx+1],tc);

    // generate texture coordinates if desired
    if ( genTCoords != VTK_TCOORDS_OFF )
      {
      if ( genTCoords == VTK_TCOORDS_FROM_NORMALIZED_LENGTH )
        {
        tc = t;
        }
      else if ( genTCoords == VTK_TCOORDS_FROM_LENGTH )
        {
        tc = t * length / this->TextureLength;
        }
      else if ( genTCoords == VTK_TCOORDS_FROM_SCALARS )
        {
        s = outPD->GetScalars()->GetTuple1(offset+i); //data just interpolated
        tc = (s - s0) / this->TextureLength;
        }
      newTCoords->InsertTuple2(offset+i,tc,0.0);
      } //if generating tcoords
    } //for all new points

  return numNewPts;
}

void vtkSplineFilter::GenerateLine(vtkIdType offset, vtkIdType npts, 
                                   vtkIdType inCellId,
                                   vtkCellData *cd, vtkCellData *outCD,
                                   vtkCellArray *newLines)
{
  vtkIdType i, outCellId;

  outCellId = newLines->InsertNextCell(npts);
  outCD->CopyData(cd,inCellId,outCellId);
  for (i=0; i < npts; i++) 
    {
    newLines->InsertCellPoint(offset+i);
    }
}


const char *vtkSplineFilter::GetSubdivideAsString()
{
  if ( this->Subdivide == VTK_SUBDIVIDE_SPECIFIED )
    {
    return "Specified by Number of Subdivisions";
    }
  else
    {
    return "Specified by Length";
    }
}

// Return the method of generating the texture coordinates.
const char *vtkSplineFilter::GetGenerateTCoordsAsString(void)
{
  if ( this->GenerateTCoords == VTK_TCOORDS_OFF )
    {
    return "GenerateTCoordsOff";
    }
  else if ( this->GenerateTCoords == VTK_TCOORDS_FROM_SCALARS ) 
    {
    return "GenerateTCoordsFromScalar";
    }
  else if ( this->GenerateTCoords == VTK_TCOORDS_FROM_LENGTH ) 
    {
    return "GenerateTCoordsFromLength";
    }
  else 
    {
    return "GenerateTCoordsFromNormalizedLength";
    }
}

void vtkSplineFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Subdivide: :" << this->GetSubdivideAsString() << "\n";
  os << indent << "Maximum Number of Subdivisions: "
     << this->MaximumNumberOfSubdivisions << "\n";
  os << indent << "Number of Subdivisions: " 
     << this->NumberOfSubdivisions << "\n";
  os << indent << "Length: " << this->Length << "\n";
  os << indent << "Spline: " << this->Spline << "\n";
  os << indent << "Generate TCoords: " 
     << this->GetGenerateTCoordsAsString() << endl;
  os << indent << "Texture Length: " << this->TextureLength << endl;
}

