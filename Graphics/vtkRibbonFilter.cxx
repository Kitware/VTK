/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRibbonFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRibbonFilter.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyLine.h"

vtkStandardNewMacro(vtkRibbonFilter);

// Construct ribbon so that width is 0.1, the width does 
// not vary with scalar values, and the width factor is 2.0.
vtkRibbonFilter::vtkRibbonFilter()
{
  this->Width = 0.5;
  this->Angle = 0.0;
  this->VaryWidth = 0;
  this->WidthFactor = 2.0;

  this->DefaultNormal[0] = this->DefaultNormal[1] = 0.0;
  this->DefaultNormal[2] = 1.0;
  
  this->UseDefaultNormal = 0;
  
  this->GenerateTCoords = 0;
  this->TextureLength = 1.0;

  // by default process active point scalars
  this->SetInputArrayToProcess(0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,
                               vtkDataSetAttributes::SCALARS);

  // by default process active point vectors
  this->SetInputArrayToProcess(1,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,
                               vtkDataSetAttributes::NORMALS);
}

vtkRibbonFilter::~vtkRibbonFilter()
{
}


int vtkRibbonFilter::RequestData(
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
  vtkDataArray *inNormals;
  vtkDataArray *inScalars = this->GetInputArrayToProcess(0,inputVector);

  vtkPoints *inPts;
  vtkIdType numPts;
  vtkIdType numLines;
  vtkIdType numNewPts, numNewCells;
  vtkPoints *newPts;
  int deleteNormals=0;
  vtkFloatArray *newNormals;
  vtkIdType i;
  double range[2];
  vtkCellArray *newStrips;
  vtkIdType npts=0, *pts=NULL;
  vtkIdType offset=0;
  vtkFloatArray *newTCoords=NULL;
  int abort=0;
  vtkIdType inCellId;

  // Check input and initialize
  //
  vtkDebugMacro(<<"Creating ribbon");

  if ( !(inPts=input->GetPoints()) || 
      (numPts = inPts->GetNumberOfPoints()) < 1 ||
      !(inLines = input->GetLines()) || 
       (numLines = inLines->GetNumberOfCells()) < 1 )
    {
    return 1;
    }

  // Create the geometry and topology
  numNewPts = 2 * numPts;
  newPts = vtkPoints::New();
  newPts->Allocate(numNewPts);
  newNormals = vtkFloatArray::New();
  newNormals->SetNumberOfComponents(3);
  newNormals->Allocate(3*numNewPts);
  newStrips = vtkCellArray::New();
  newStrips->Allocate(newStrips->EstimateSize(1,numNewPts));
  vtkCellArray *singlePolyline = vtkCellArray::New();

  // Point data: copy scalars, vectors, tcoords. Normals may be computed here.
  outPD->CopyNormalsOff();
  if ( (this->GenerateTCoords == VTK_TCOORDS_FROM_SCALARS && inScalars) ||
       this->GenerateTCoords == VTK_TCOORDS_FROM_LENGTH ||
       this->GenerateTCoords == VTK_TCOORDS_FROM_NORMALIZED_LENGTH )
    {
    newTCoords = vtkFloatArray::New();
    newTCoords->SetNumberOfComponents(2);
    newTCoords->Allocate(numNewPts);
    outPD->CopyTCoordsOff();
    }
  outPD->CopyAllocate(pd,numNewPts);

  int generateNormals = 0;
  inNormals = this->GetInputArrayToProcess(1,inputVector);
  if ( !inNormals || this->UseDefaultNormal )
    {
    deleteNormals = 1;
    inNormals = vtkFloatArray::New();
    inNormals->SetNumberOfComponents(3);
    inNormals->SetNumberOfTuples(numPts);

    if ( this->UseDefaultNormal )
      {
      for ( i=0; i < numPts; i++)
        {
        inNormals->SetTuple(i,this->DefaultNormal);
        }
      }
    else
      {
      // Normal generation has been moved to lower in the function.
      // This allows each different polylines to share vertices, but have
      // their normals (and hence their ribbons) calculated independently
      generateNormals = 1;
      }      
    }

  // If varying width, get appropriate info.
  //
  if ( this->VaryWidth && inScalars )
    {
    inScalars->GetRange(range,0);
    if ((range[1] - range[0]) == 0.0)
      {
      vtkWarningMacro(<< "Scalar range is zero!");
      range[1] = range[0] + 1.0;
      }
    }

  // Copy selected parts of cell data; certainly don't want normals
  //
  numNewCells = inLines->GetNumberOfCells();
  outCD->CopyNormalsOff();
  outCD->CopyAllocate(cd,numNewCells);

  //  Create points along each polyline that are connected into NumberOfSides
  //  triangle strips. Texture coordinates are optionally generated.
  //
  this->Theta = vtkMath::RadiansFromDegrees( this->Angle );
  vtkPolyLine *lineNormalGenerator = vtkPolyLine::New();
  for (inCellId=0, inLines->InitTraversal(); 
       inLines->GetNextCell(npts,pts) && !abort; inCellId++)
    {
    this->UpdateProgress((double)inCellId/numLines);
    abort = this->GetAbortExecute();

    if (npts < 2)
      {
      vtkWarningMacro(<< "Less than two points in line!");
      continue; //skip tubing this polyline
      }

    // If necessary calculate normals, each polyline calculates its
    // normals independently, avoiding conflicts at shared vertices.
    if (generateNormals) 
      {
      singlePolyline->Reset(); //avoid instantiation
      singlePolyline->InsertNextCell(npts,pts);
      if ( !lineNormalGenerator->GenerateSlidingNormals(inPts,singlePolyline,
                                                        inNormals) )
        {
        vtkWarningMacro(<< "No normals for line!");
        continue; //skip tubing this polyline
        }
      }

    // Generate the points around the polyline. The strip is not created
    // if the polyline is bad.
    //
    if ( !this->GeneratePoints(offset,npts,pts,inPts,newPts,pd,outPD,
                               newNormals,inScalars,range,inNormals) )
      {
      vtkWarningMacro(<< "Could not generate points!");
      continue; //skip ribboning this polyline
      }
      
    // Generate the strip for this polyline
    //
    this->GenerateStrip(offset,npts,pts,inCellId,cd,outCD,newStrips);

    // Generate the texture coordinates for this polyline
    //
    if ( newTCoords )
      {
      this->GenerateTextureCoords(offset,npts,pts,inPts,inScalars,newTCoords);
      }

    // Compute the new offset for the next polyline
    offset = this->ComputeOffset(offset,npts);

    }//for all polylines

  singlePolyline->Delete();

  // Update ourselves
  //
  if ( deleteNormals )
    {
    inNormals->Delete();
    }

  if ( newTCoords )
    {
    outPD->SetTCoords(newTCoords);
    newTCoords->Delete();
    }

  output->SetPoints(newPts);
  newPts->Delete();

  output->SetStrips(newStrips);
  newStrips->Delete();

  outPD->SetNormals(newNormals);
  newNormals->Delete();
  lineNormalGenerator->Delete();

  output->Squeeze();

  return 1;
}

int vtkRibbonFilter::GeneratePoints(vtkIdType offset, 
                                  vtkIdType npts, vtkIdType *pts,
                                  vtkPoints *inPts, vtkPoints *newPts, 
                                  vtkPointData *pd, vtkPointData *outPD,
                                  vtkFloatArray *newNormals,
                                  vtkDataArray *inScalars, double range[2],
                                  vtkDataArray *inNormals)
{
  vtkIdType j;
  int i;
  double p[3];
  double pNext[3];
  double sNext[3];
  double sPrev[3];
  double n[3];
  double s[3], sp[3], sm[3], v[3];
  //double bevelAngle;
  double w[3];
  double nP[3];
  double sFactor=1.0;
  vtkIdType ptId=offset;

  // Use "averaged" segment to create beveled effect. 
  // Watch out for first and last points.
  //
  for (j=0; j < npts; j++)
    {
    if ( j == 0 ) //first point
      {
      inPts->GetPoint(pts[0],p);
      inPts->GetPoint(pts[1],pNext);
      for (i=0; i<3; i++) 
        {
        sNext[i] = pNext[i] - p[i];
        sPrev[i] = sNext[i];
        }
      }
    else if ( j == (npts-1) ) //last point
      {
      for (i=0; i<3; i++)
        {
        sPrev[i] = sNext[i];
        p[i] = pNext[i];
        }
      }
    else
      {
      for (i=0; i<3; i++)
        {
        p[i] = pNext[i];
        }
      inPts->GetPoint(pts[j+1],pNext);
      for (i=0; i<3; i++)
        {
        sPrev[i] = sNext[i];
        sNext[i] = pNext[i] - p[i];
        }
      }

    inNormals->GetTuple(pts[j], n);

    if ( vtkMath::Normalize(sNext) == 0.0 )
      {
      vtkWarningMacro(<<"Coincident points!");
      return 0;
      }

    for (i=0; i<3; i++)
      {
      s[i] = (sPrev[i] + sNext[i]) / 2.0; //average vector
      }
    // if s is zero then just use sPrev cross n
    if (vtkMath::Normalize(s) == 0.0)
      {
      vtkWarningMacro(<< "Using alternate bevel vector");
      vtkMath::Cross(sPrev,n,s);
      if (vtkMath::Normalize(s) == 0.0)
        {
        vtkWarningMacro(<< "Using alternate bevel vector");
        }
      }
/*
    if ( (bevelAngle = vtkMath::Dot(sNext,sPrev)) > 1.0 )
      {
      bevelAngle = 1.0;
      }
    if ( bevelAngle < -1.0 )
      {
      bevelAngle = -1.0;
      }
    bevelAngle = acos((double)bevelAngle) / 2.0; //(0->90 degrees)
    if ( (bevelAngle = cos(bevelAngle)) == 0.0 )
      {
      bevelAngle = 1.0;
      }

    bevelAngle = this->Width / bevelAngle; //keep ribbon constant width
*/
    vtkMath::Cross(s,n,w);
    if ( vtkMath::Normalize(w) == 0.0)
      {
      vtkWarningMacro(<<"Bad normal s = " <<s[0]<<" "<<s[1]<<" "<< s[2] 
                      << " n = " << n[0] << " " << n[1] << " " << n[2]);
      return 0;
      }

    vtkMath::Cross(w,s,nP); //create orthogonal coordinate system
    vtkMath::Normalize(nP);

    // Compute a scale factor based on scalars or vectors
    if ( inScalars && this->VaryWidth ) // varying by scalar values
      {
      sFactor = 1.0 + ((this->WidthFactor - 1.0) * 
                (inScalars->GetComponent(pts[j],0) - range[0]) 
                       / (range[1]-range[0]));
      }

    for (i=0; i<3; i++) 
      {
      v[i] = (w[i]*cos(this->Theta) + nP[i]*sin(this->Theta));
      sp[i] = p[i] + this->Width * sFactor * v[i];
      sm[i] = p[i] - this->Width * sFactor * v[i];
      }
    newPts->InsertPoint(ptId,sm);
    newNormals->InsertTuple(ptId,nP);
    outPD->CopyData(pd,pts[j],ptId);
    ptId++;
    newPts->InsertPoint(ptId,sp);
    newNormals->InsertTuple(ptId,nP);
    outPD->CopyData(pd,pts[j],ptId);
    ptId++;
    }//for all points in polyline
  
  return 1;
}

void vtkRibbonFilter::GenerateStrip(vtkIdType offset, vtkIdType npts, 
                                    vtkIdType* vtkNotUsed(pts), 
                                    vtkIdType inCellId,
                                    vtkCellData *cd, vtkCellData *outCD,
                                    vtkCellArray *newStrips)
{
  vtkIdType i, idx, outCellId;

  outCellId = newStrips->InsertNextCell(npts*2);
  outCD->CopyData(cd,inCellId,outCellId);
  for (i=0; i < npts; i++) 
    {
    idx = 2*i;
    newStrips->InsertCellPoint(offset+idx);
    newStrips->InsertCellPoint(offset+idx+1);
    }
}

void vtkRibbonFilter::GenerateTextureCoords(vtkIdType offset,
                                            vtkIdType npts, vtkIdType *pts, 
                                            vtkPoints *inPts, 
                                            vtkDataArray *inScalars,
                                            vtkFloatArray *newTCoords)
{
  vtkIdType i;
  int k;
  double tc;

  double s0, s;
  //The first texture coordinate is always 0.
  for ( k=0; k < 2; k++)
    {
    newTCoords->InsertTuple2(offset+k,0.0,0.0);
    }
  if ( this->GenerateTCoords == VTK_TCOORDS_FROM_SCALARS && inScalars)
    {
    s0 = inScalars->GetTuple1(pts[0]);
    for (i=1; i < npts; i++)
      {
      s = inScalars->GetTuple1(pts[i]);
      tc = (s - s0) / this->TextureLength;
      for ( k=0; k < 2; k++)
        {
        newTCoords->InsertTuple2(offset+i*2+k,tc,0.0);
        }
      }
    }
  else if ( this->GenerateTCoords == VTK_TCOORDS_FROM_LENGTH )
    {
    double xPrev[3], x[3], len=0.0;
    inPts->GetPoint(pts[0],xPrev);
    for (i=1; i < npts; i++)
      {
      inPts->GetPoint(pts[i],x);
      len += sqrt(vtkMath::Distance2BetweenPoints(x,xPrev));
      tc = len / this->TextureLength;
      for ( k=0; k < 2; k++)
        {
        newTCoords->InsertTuple2(offset+i*2+k,tc,0.0);
        }
      xPrev[0]=x[0]; xPrev[1]=x[1]; xPrev[2]=x[2];
      }
    }
  else if ( this->GenerateTCoords == VTK_TCOORDS_FROM_NORMALIZED_LENGTH )
    {
    double xPrev[3], x[3], length=0.0, len=0.0;
    inPts->GetPoint(pts[0],xPrev);
    for (i=1; i < npts; i++)
      {
      inPts->GetPoint(pts[i],x);
      length += sqrt(vtkMath::Distance2BetweenPoints(x,xPrev));
      xPrev[0]=x[0]; xPrev[1]=x[1]; xPrev[2]=x[2];
      }

    inPts->GetPoint(pts[0],xPrev);
    for (i=1; i < npts; i++)
      {
      inPts->GetPoint(pts[i],x);
      len += sqrt(vtkMath::Distance2BetweenPoints(x,xPrev));
      tc = len / length;
      for ( k=0; k < 2; k++)
        {
        newTCoords->InsertTuple2(offset+i*2+k,tc,0.0);
        }
      xPrev[0]=x[0]; xPrev[1]=x[1]; xPrev[2]=x[2];
      }
    }
}

// Compute the number of points in this ribbon
vtkIdType vtkRibbonFilter::ComputeOffset(vtkIdType offset, vtkIdType npts)
{
  offset += 2 * npts;
  return offset;
}

// Description:
// Return the method of generating the texture coordinates.
const char *vtkRibbonFilter::GetGenerateTCoordsAsString(void)
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

void vtkRibbonFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Width: " << this->Width << "\n";
  os << indent << "Angle: " << this->Angle << "\n";
  os << indent << "VaryWidth: " << (this->VaryWidth ? "On\n" : "Off\n");
  os << indent << "Width Factor: " << this->WidthFactor << "\n";
  os << indent << "Use Default Normal: " << this->UseDefaultNormal << "\n";
  os << indent << "Default Normal: " << "( " 
     << this->DefaultNormal[0] << ", " 
     << this->DefaultNormal[1] << ", " 
     << this->DefaultNormal[2] << " )\n";

  os << indent << "Generate TCoords: " 
     << this->GetGenerateTCoordsAsString() << endl;
  os << indent << "Texture Length: " << this->TextureLength << endl;
}

