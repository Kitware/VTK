/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBYUReader.cxx
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
#include "vtkBYUReader.h"
#include "vtkPoints.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkBYUReader* vtkBYUReader::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkBYUReader");
  if(ret)
    {
    return (vtkBYUReader*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkBYUReader;
}




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
}

vtkBYUReader::~vtkBYUReader()
{
  if ( this->GeometryFileName )
    {
    delete [] this->GeometryFileName;
    }
  if ( this->DisplacementFileName )
    {
    delete [] this->DisplacementFileName;
    }
  if ( this->ScalarFileName )
    {
    delete [] this->ScalarFileName;
    }
  if ( this->TextureFileName )
    {
    delete [] this->TextureFileName;
    }
}

void vtkBYUReader::Execute()
{
  FILE *geomFp;
  int numPts;

  if (this->GeometryFileName == NULL)
    {
    vtkErrorMacro(<< "No GeometryFileName specified!");
    return;
    }
  if ((geomFp = fopen(this->GeometryFileName, "r")) == NULL)
    {
    vtkErrorMacro(<< "Geometry file: " << this->GeometryFileName << " not found");
    return;
    }
  else
    {
    this->ReadGeometryFile(geomFp,numPts);
    fclose(geomFp);
    }

  this->ReadDisplacementFile(numPts);
  this->ReadScalarFile(numPts);
  this->ReadTextureFile(numPts);
  this->UpdateProgress(1.0);
}

void vtkBYUReader::ReadGeometryFile(FILE *geomFile, int &numPts)
{
  int numParts, numPolys, numEdges;
  int partStart, partEnd;
  int i;
  vtkPoints *newPts;
  vtkCellArray *newPolys;
  float x[3];
  vtkIdList *pts;
  int polyId, pt;
  vtkPolyData *output = this->GetOutput();
  
  pts = vtkIdList::New();
  pts->Allocate(VTK_CELL_SIZE);

  //
  // Read header (not using fixed format! - potential problem in some files.)
  //
  fscanf (geomFile, "%d %d %d %d", &numParts, &numPts, &numPolys, &numEdges);

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
      fscanf (geomFile, "%*d %*d");
      }
    fscanf (geomFile, "%d %d", &partStart, &partEnd);
    for (i=this->PartNumber; i < numParts; i++)
      {
      fscanf (geomFile, "%*d %*d");
      }
    }
  else // read all parts
    {
    vtkDebugMacro(<<"Reading all parts.");
    for (i=0; i < numParts; i++)
      {
      fscanf (geomFile, "%*d %*d");
      }
    partStart = 1;
    partEnd = VTK_LARGE_INTEGER;
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
    fscanf(geomFile, "%e %e %e", x, x+1, x+2);
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

void vtkBYUReader::ReadDisplacementFile(int numPts)
{
  FILE *dispFp;
  int i;
  float v[3];
  vtkFloatArray *newVectors;
  vtkPolyData *output = this->GetOutput();
  
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
    fscanf(dispFp, "%e %e %e", v, v+1, v+2);
    newVectors->SetTuple(i,v);
    }

  fclose(dispFp);
  vtkDebugMacro(<<"Read " << numPts << " displacements");

  output->GetPointData()->SetVectors(newVectors);
  newVectors->Delete();
}

void vtkBYUReader::ReadScalarFile(int numPts)
{
  FILE *scalarFp;
  int i;
  float s;
  vtkFloatArray *newScalars;
  vtkPolyData *output = this->GetOutput();
  
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
    fscanf(scalarFp, "%e", &s);
    newScalars->SetTuple(i,&s);
    }

  fclose(scalarFp);
  vtkDebugMacro(<<"Read " << numPts << " scalars");

  output->GetPointData()->SetScalars(newScalars);
  newScalars->Delete();
}

void vtkBYUReader::ReadTextureFile(int numPts)
{
  FILE *textureFp;
  int i;
  float t[2];
  vtkFloatArray *newTCoords;
  vtkPolyData *output = this->GetOutput();

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
    fscanf(textureFp, "%e %e", t, t+1);
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
  vtkPolyDataSource::PrintSelf(os,indent);

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

