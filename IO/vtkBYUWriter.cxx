/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBYUWriter.cxx
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
#include "vtkBYUWriter.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkBYUWriter* vtkBYUWriter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkBYUWriter");
  if(ret)
    {
    return (vtkBYUWriter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkBYUWriter;
}




// Create object so that it writes displacement, scalar, and texture files
// (if data is available).
vtkBYUWriter::vtkBYUWriter()
{
  this->GeometryFileName = NULL;
  this->DisplacementFileName = NULL;
  this->ScalarFileName = NULL;
  this->TextureFileName = NULL;

  this->WriteDisplacement = 1;
  this->WriteScalar = 1;
  this->WriteTexture = 1;
}

vtkBYUWriter::~vtkBYUWriter()
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

// Write out data in MOVIE.BYU format.
void vtkBYUWriter::WriteData()
{
  FILE *geomFp;
  vtkPolyData *input= this->GetInput();
  int numPts=input->GetNumberOfPoints();

  if ( numPts < 1 )
    {
    vtkErrorMacro(<<"No data to write!");
    return;
    }

  if ((geomFp = fopen(this->GeometryFileName, "w")) == NULL)
    {
    vtkErrorMacro(<< "Couldn't open geometry file: " << this->GeometryFileName);
    return;
    }
  else
    {
    this->WriteGeometryFile(geomFp,numPts);
    }

  this->WriteDisplacementFile(numPts);
  this->WriteScalarFile(numPts);
  this->WriteTextureFile(numPts);

  // Close the file
  fclose (geomFp);
}

void vtkBYUWriter::WriteGeometryFile(FILE *geomFile, int numPts)
{
  int numPolys, numEdges;
  int i;
  float *x;
  vtkIdType npts;
  vtkIdType *pts;
  vtkPoints *inPts;
  vtkCellArray *inPolys;
  vtkPolyData *input= this->GetInput();
  //
  // Check input
  //
  inPolys=input->GetPolys();
  if ( (inPts=input->GetPoints()) == NULL || inPolys == NULL )
    {
    vtkErrorMacro(<<"No data to write!");
    return;
    }
//
// Write header (not using fixed format! - potential problem in some files.)
//
  numPolys = input->GetPolys()->GetNumberOfCells();
  for (numEdges=0, inPolys->InitTraversal(); inPolys->GetNextCell(npts,pts); )
    {
    numEdges += npts;
    }

  fprintf (geomFile, "%d %d %d %d\n", 1, numPts, numPolys, numEdges);
  fprintf (geomFile, "%d %d\n", 1, numPolys);
//
// Write data
//
  // write point coordinates
  for (i=0; i < numPts; i++)
    {
    x = inPts->GetPoint(i);
    fprintf(geomFile, "%e %e %e ", x[0], x[1], x[2]);
    if ( (i % 2) )
      {
      fprintf(geomFile, "\n");
      }
    }
  if ( (numPts % 2) )
    {
    fprintf(geomFile, "\n");
    }

  // write poly data. Remember 1-offset.
  for (inPolys->InitTraversal(); inPolys->GetNextCell(npts,pts); )
    {
    // write this polygon
    for (i=0; i < (npts-1); i++)
      {
      fprintf (geomFile, "%d ", pts[i]+1);
      }
    fprintf (geomFile, "%d\n", -(pts[npts-1]+1));
    }

  vtkDebugMacro(<<"Wrote " << numPts << " points, " << numPolys << " polygons");
}

void vtkBYUWriter::WriteDisplacementFile(int numPts)
{
  FILE *dispFp;
  int i;
  float *v;
  vtkVectors *inVectors;
  vtkPolyData *input= this->GetInput();

  if ( this->WriteDisplacement && this->DisplacementFileName &&
  (inVectors = input->GetPointData()->GetVectors()) != NULL )
    {
    if ( !(dispFp = fopen(this->DisplacementFileName, "w")) )
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
  // Write data
  //
  for (i=0; i < numPts; i++)
    {
    v = inVectors->GetVector(i);
    fprintf(dispFp, "%e %e %e", v[0], v[1], v[2]);
    if ( (i % 2) )
      {
      fprintf (dispFp, "\n");
      }
    }

  vtkDebugMacro(<<"Wrote " << numPts << " displacements");
  fclose (dispFp);
}

void vtkBYUWriter::WriteScalarFile(int numPts)
{
  FILE *scalarFp;
  int i;
  float s;
  vtkScalars *inScalars;
  vtkPolyData *input= this->GetInput();

  if ( this->WriteScalar && this->ScalarFileName &&
  (inScalars = input->GetPointData()->GetScalars()) != NULL )
    {
    if ( !(scalarFp = fopen(this->ScalarFileName, "w")) )
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
  // Write data
  //
  for (i=0; i < numPts; i++)
    {
    s = inScalars->GetScalar(i);
    fprintf(scalarFp, "%e ", s);
    if ( i != 0 && !(i % 6) )
      {
      fprintf (scalarFp, "\n");
      }
    }

  fclose (scalarFp);
  vtkDebugMacro(<<"Wrote " << numPts << " scalars");
}

void vtkBYUWriter::WriteTextureFile(int numPts)
{
  FILE *textureFp;
  int i;
  float *t;
  vtkTCoords *inTCoords;
  vtkPolyData *input= this->GetInput();

  if ( this->WriteTexture && this->TextureFileName &&
  (inTCoords = input->GetPointData()->GetTCoords()) != NULL )
    {
    if ( !(textureFp = fopen(this->TextureFileName, "w")) )
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
  // Write data
  //
  for (i=0; i < numPts; i++)
    {
    if ( i != 0 && !(i % 3) )
      {
      fprintf (textureFp, "\n");
      }
    t = inTCoords->GetTCoord(i);
    fprintf(textureFp, "%e %e", t[0], t[1]);
    }

  fclose (textureFp);
  vtkDebugMacro(<<"Wrote " << numPts << " texture coordinates");
}

void vtkBYUWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataWriter::PrintSelf(os,indent);

  os << indent << "Geometry File Name: " 
     << (this->GeometryFileName ? this->GeometryFileName : "(none)") << "\n";

  os << indent << "Write Displacement: " << (this->WriteDisplacement ? "On\n" : "Off\n");
  os << indent << "Displacement File Name: " 
     << (this->DisplacementFileName ? this->DisplacementFileName : "(none)") << "\n";

  os << indent << "Write Scalar: " << (this->WriteScalar ? "On\n" : "Off\n");
  os << indent << "Scalar File Name: " 
     << (this->ScalarFileName ? this->ScalarFileName : "(none)") << "\n";

  os << indent << "Write Texture: " << (this->WriteTexture ? "On\n" : "Off\n");
  os << indent << "Texture File Name: " 
     << (this->TextureFileName ? this->TextureFileName : "(none)") << "\n";
}

