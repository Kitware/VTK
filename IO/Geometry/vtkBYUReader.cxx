/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBYUReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBYUReader.h"

#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

vtkStandardNewMacro(vtkBYUReader);

vtkBYUReader::vtkBYUReader()
{
  this->GeometryFileName = NULL;
  this->DisplacementFileName = NULL;
  this->ScalarFileName = NULL;
  this->TextureFileName = NULL;

  this->ReadDisplacement = 1;
  this->ReadScalar = 1;
  this->ReadTexture = 1;

  this->PartNumber = 0;

  this->SetNumberOfInputPorts(0);
}

vtkBYUReader::~vtkBYUReader()
{
  delete [] this->GeometryFileName;
  delete [] this->DisplacementFileName;
  delete [] this->ScalarFileName;
  delete [] this->TextureFileName;
}

int vtkBYUReader::CanReadFile(const char *filename)
{
  int result;
  FILE *fp = fopen(filename, "r");
  if (fp == NULL) return 0;

  int numParts, numPts, numPolys, numEdges;
  result = fscanf(fp, "%d %d %d %d", &numParts, &numPts, &numPolys, &numEdges);
  if ((result < 4) || (numParts < 1) || (numPts < 1) || (numPolys < 1))
    {
    fclose(fp);
    return 0;
    }

  for (int part = 0; part < numParts; part++)
    {
    int partStart, partEnd;
    result = fscanf(fp, "%d %d", &partStart, &partEnd);
    if (   (result < 2)
        || (partStart < 1) || (partStart > numPolys)
        || (partEnd < 1) || (partEnd > numPolys)
        || (partStart >= partEnd) )
      {
      fclose(fp);
      return 0;
      }
    }

  fclose(fp);
  return 1;
}

int vtkBYUReader::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  FILE *geomFp;
  int numPts;

  if (this->GeometryFileName == NULL || this->GeometryFileName[0] == '\0')
    {
    vtkErrorMacro(<< "No GeometryFileName specified!");
    return 0;
    }
  if ((geomFp = fopen(this->GeometryFileName, "r")) == NULL)
    {
    vtkErrorMacro(<< "Geometry file: " << this->GeometryFileName << " not found");
    return 0;
    }
  else
    {
    this->ReadGeometryFile(geomFp,numPts,outInfo);
    fclose(geomFp);
    }

  this->ReadDisplacementFile(numPts, outInfo);
  this->ReadScalarFile(numPts, outInfo);
  this->ReadTextureFile(numPts, outInfo);
  this->UpdateProgress(1.0);

  return 1;
}

void vtkBYUReader::ReadGeometryFile(FILE *geomFile, int &numPts,
                                    vtkInformation *outInfo)
{
  int numParts, numPolys, numEdges;
  int partStart, partEnd;
  int i;
  vtkPoints *newPts;
  vtkCellArray *newPolys;
  float x[3];
  vtkIdList *pts;
  int polyId, pt;
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  pts = vtkIdList::New();
  pts->Allocate(VTK_CELL_SIZE);

  //
  // Read header (not using fixed format! - potential problem in some files.)
  //
  int cnt;
  cnt = fscanf (geomFile, "%d %d %d %d", &numParts, &numPts, &numPolys, &numEdges);
  if (cnt != 4)
    {
    vtkErrorMacro(
      <<"Error reading geometry file: " << this->GeometryFileName
      << "Expected 4 values in header, but got " << cnt);
    return;
    }

  if ( this->PartNumber > numParts )
    {
    vtkWarningMacro(<<"Specified part number > number of parts");
    this->PartNumber = 0;
    }

  if ( this->PartNumber > 0 ) // read just part specified
    {
    vtkDebugMacro(<<"Reading part number: " << this->PartNumber);
    for (i=0; i < (this->PartNumber-1); i++)
      {
      cnt = fscanf (geomFile, "%*d %*d");
      if (cnt != 0)
        {
        vtkErrorMacro(
          <<"Error reading geometry file: " << this->GeometryFileName
          << "Skipping 2 int's, but skipped " << cnt);
        return;
        }
      }
    cnt = fscanf (geomFile, "%d %d", &partStart, &partEnd);
    if (cnt != 2)
      {
      vtkErrorMacro(
        <<"Error reading geometry file: " << this->GeometryFileName
        << "Expected 2 values for partStart and partEnd, but got " << cnt);
      return;
      }
    for (i=this->PartNumber; i < numParts; i++)
      {
      cnt = fscanf (geomFile, "%*d %*d");
      if (cnt != 0)
        {
        vtkErrorMacro(
          <<"Error reading geometry file: " << this->GeometryFileName
          << "Skipping 2 int's, but skipped " << cnt);
        return;
        }
      }
    }
  else // read all parts
    {
    vtkDebugMacro(<<"Reading all parts.");
    for (i=0; i < numParts; i++)
      {
      cnt = fscanf (geomFile, "%*d %*d");
      if (cnt != 0)
        {
        vtkErrorMacro(
          <<"Error reading geometry file: " << this->GeometryFileName
        << "Skipping 2 int's, but skipped " << cnt);
        return;
        }
      }
    partStart = 1;
    partEnd = VTK_INT_MAX;
    }

  if ( numParts < 1 || numPts < 1 || numPolys < 1 )
    {
    vtkErrorMacro(<<"Bad MOVIE.BYU file");
    pts->Delete();
    return;
    }
  //
  // Allocate data objects
  //
  newPts = vtkPoints::New();
  newPts->Allocate(numPts);
  newPolys = vtkCellArray::New();
  newPolys->Allocate(numPolys+numEdges);
  //
  // Read data
  //
  // read point coordinates
  for (i=0; i<numPts; i++)
    {
    cnt = fscanf(geomFile, "%e %e %e", x, x+1, x+2);
    if (cnt != 3)
      {
      vtkErrorMacro(
        <<"Error reading geometry file: " << this->GeometryFileName
        << "Expected 3 points, but got " << cnt);
      return;
      }
    newPts->InsertPoint(i,x);
    }
  this->UpdateProgress(0.333);

  // read poly data. Have to fix 1-offset. Only reading part number specified.
  for ( polyId=1; polyId <= numPolys; polyId++ )
    {
    // read this polygon
    for ( pts->Reset(); fscanf(geomFile, "%d", &pt) && pt > 0; )
      {
      pts->InsertNextId(pt-1);//convert to vtk 0-offset
      }
    pts->InsertNextId(-(pt+1));

    // Insert polygon (if in selected part)
    if ( partStart <= polyId && polyId <= partEnd )
      {
      newPolys->InsertNextCell(pts);
      }
    }
  this->UpdateProgress(0.6667);

  vtkDebugMacro(<<"Reading:" << numPts << " points, "
                 << numPolys << " polygons.");

  output->SetPoints(newPts);
  newPts->Delete();

  output->SetPolys(newPolys);
  newPolys->Delete();
  pts->Delete();
}

void vtkBYUReader::ReadDisplacementFile(int numPts, vtkInformation *outInfo)
{
  FILE *dispFp;
  int i;
  float v[3];
  vtkFloatArray *newVectors;
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if ( this->ReadDisplacement && this->DisplacementFileName )
    {
    if ( !(dispFp = fopen(this->DisplacementFileName, "r")) )
      {
      vtkErrorMacro (<<"Couldn't open displacement file");
      return;
      }
    }
  else
    {
    return;
    }
  //
  // Allocate and read data
  //
  newVectors = vtkFloatArray::New();
  newVectors->SetNumberOfComponents(3);
  newVectors->SetNumberOfTuples(numPts);

  for (i=0; i<numPts; i++)
    {
    int cnt = fscanf(dispFp, "%e %e %e", v, v+1, v+2);
    if (cnt != 3)
      {
      vtkErrorMacro(
        <<"Error reading displacement file: " << this->DisplacementFileName
        << "Expected 3 floats, but got " << cnt);
      return;
      }
    newVectors->SetTuple(i,v);
    }

  fclose(dispFp);
  vtkDebugMacro(<<"Read " << numPts << " displacements");

  output->GetPointData()->SetVectors(newVectors);
  newVectors->Delete();
}

void vtkBYUReader::ReadScalarFile(int numPts, vtkInformation *outInfo)
{
  FILE *scalarFp;
  int i;
  float s;
  vtkFloatArray *newScalars;
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if ( this->ReadScalar && this->ScalarFileName )
    {
    if ( !(scalarFp = fopen(this->ScalarFileName, "r")) )
      {
      vtkErrorMacro (<<"Couldn't open scalar file");
      return;
      }
    }
  else
    {
    return;
    }
  //
  // Allocate and read data
  //
  newScalars = vtkFloatArray::New();
  newScalars->SetNumberOfTuples(numPts);

  for (i=0; i<numPts; i++)
    {
    int cnt = fscanf(scalarFp, "%e", &s);
    if (cnt != 1)
      {
      vtkErrorMacro(
        <<"Error reading scalar file: " << this->ScalarFileName
        << "Expected 1 float, but got " << cnt);
      return;
      }

    newScalars->SetTuple(i,&s);
    }

  fclose(scalarFp);
  vtkDebugMacro(<<"Read " << numPts << " scalars");

  output->GetPointData()->SetScalars(newScalars);
  newScalars->Delete();
}

void vtkBYUReader::ReadTextureFile(int numPts, vtkInformation *outInfo)
{
  FILE *textureFp;
  int i;
  float t[2];
  vtkFloatArray *newTCoords;
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if ( this->ReadTexture && this->TextureFileName )
    {
    if ( !(textureFp = fopen(this->TextureFileName, "r")) )
      {
      vtkErrorMacro (<<"Couldn't open texture file");
      return;
      }
    }
  else
    {
    return;
    }
  //
  // Allocate and read data
  //
  newTCoords = vtkFloatArray::New();
  newTCoords->SetNumberOfComponents(3);
  newTCoords->SetNumberOfTuples(numPts);

  for (i=0; i<numPts; i++)
    {
    int cnt = fscanf(textureFp, "%e %e", t, t+1);
    if (cnt != 2)
      {
      vtkErrorMacro(
        <<"Error reading texture file: " << this->TextureFileName
        << "Expected 2 texture coordibates, but got " << cnt);
      return;
      }
    newTCoords->SetTuple(i,t);
    }

  fclose(textureFp);
  vtkDebugMacro(<<"Read " << numPts << " texture coordinates");

  output->GetPointData()->SetTCoords(newTCoords);
  newTCoords->Delete();
}

//----------------------------------------------------------------------------
// This source does not know how to generate pieces yet.
int vtkBYUReader::ComputeDivisionExtents(vtkDataObject *vtkNotUsed(output),
                                         int idx, int numDivisions)
{
  if (idx == 0 && numDivisions == 1)
    {
    // I will give you the whole thing
    return 1;
    }
  else
    {
    // I have nothing to give you for this piece.
    return 0;
    }
}

void vtkBYUReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Geometry File Name: "
     << (this->GeometryFileName ? this->GeometryFileName : "(none)") << "\n";
  os << indent << "Read Displacement: " << (this->ReadDisplacement ? "On\n" : "Off\n");
  os << indent << "Displacement File Name: "
     << (this->DisplacementFileName ? this->DisplacementFileName : "(none)") << "\n";
  os << indent << "Part Number: " << this->PartNumber << "\n";
  os << indent << "Read Scalar: " << (this->ReadScalar ? "On\n" : "Off\n");
  os << indent << "Scalar File Name: "
     << (this->ScalarFileName ? this->ScalarFileName : "(none)") << "\n";
  os << indent << "Read Texture: " << (this->ReadTexture ? "On\n" : "Off\n");
  os << indent << "Texture File Name: "
     << (this->TextureFileName ? this->TextureFileName : "(none)") << "\n";
}
