/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSplineFilter.cxx
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
#include "vtkSplineFilter.h"
#include "vtkObjectFactory.h"
#include "vtkCardinalSpline.h"
#include "vtkFloatArray.h"
#include "vtkMath.h"

vtkCxxRevisionMacro(vtkSplineFilter, "1.7");
vtkStandardNewMacro(vtkSplineFilter);

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
  this->Spline->Delete();
  this->TCoordMap->Delete();
}

void vtkSplineFilter::Execute()
{
  vtkPolyData *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();
  vtkPointData *pd=input->GetPointData();
  vtkPointData *outPD=output->GetPointData();
  vtkCellData *cd=input->GetCellData();
  vtkCellData *outCD=output->GetCellData();
  vtkCellArray *inLines = NULL;
  
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
    vtkWarningMacro(<< " No input data!");
    return;
    }

  if ( this->Spline == NULL )
    {
    vtkWarningMacro(<< "Need to specify a spline!");
    return;
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
  this->XSpline = this->Spline->MakeObject();
  this->XSpline->DeepCopy(this->Spline);
  this->YSpline = this->Spline->MakeObject();
  this->YSpline->DeepCopy(this->Spline);
  this->ZSpline = this->Spline->MakeObject();
  this->ZSpline->DeepCopy(this->Spline);

  //  Create points along each polyline.
  //
  for (inCellId=0, inLines->InitTraversal(); 
       inLines->GetNextCell(npts,pts) && !abort; inCellId++)
    {
    this->UpdateProgress((float)inCellId/numLines);
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
      vtkWarningMacro(<< "Could not generate points!");
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
  float xPrev[3], x[3], length=0.0, len, t, tc;
  inPts->GetPoint(pts[0],xPrev);
  for (i=1; i < npts; i++)
    {
    inPts->GetPoint(pts[i],x);
    len = sqrt(vtkMath::Distance2BetweenPoints(x,xPrev));
    if ( len <= 0.0 )
      {
      return 0; //failure
      }
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
    len += sqrt(vtkMath::Distance2BetweenPoints(x,xPrev));
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
    numDivs = (int) (length / this->Length);
    }
  numDivs = ( numDivs < 1 ? 1 : (numDivs > this->MaximumNumberOfSubdivisions ? 
                                 this->MaximumNumberOfSubdivisions : numDivs));

  // Now compute the new points
  numNewPts = numDivs + 1;
  vtkIdType idx;
  float s, s0=0.0;
  if ( genTCoords == VTK_TCOORDS_FROM_SCALARS )
    {
    s0=pd->GetScalars()->GetTuple1(pts[0]);
    }
  float tLo = this->TCoordMap->GetValue(0);
  float tHi = this->TCoordMap->GetValue(1);
  for (idx=0, i=0; i < numNewPts; i++)
    {
    t = (float) i / numDivs;
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

