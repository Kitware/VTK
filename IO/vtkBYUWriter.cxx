/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBYUWriter.cxx
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
#include "vtkBYUWriter.h"

#include "vtkObjectFactory.h"
#include "vtkPolyData.h"

vtkCxxRevisionMacro(vtkBYUWriter, "1.45");
vtkStandardNewMacro(vtkBYUWriter);

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

  if ( !this->GeometryFileName )
    {
    vtkErrorMacro(<< "Geometry file name was not specified");
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
  vtkIdType npts = 0;
  vtkIdType *pts = 0;
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
    // treating vtkIdType as int
    for (i=0; i < (npts-1); i++)
      {
      fprintf (geomFile, "%d ", (int)(pts[i]+1));
      }
    fprintf (geomFile, "%d\n", (int)(-(pts[npts-1]+1)));
    }

  vtkDebugMacro(<<"Wrote " << numPts << " points, " << numPolys << " polygons");
}

void vtkBYUWriter::WriteDisplacementFile(int numPts)
{
  FILE *dispFp;
  int i;
  float *v;
  vtkDataArray *inVectors;
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
    v = inVectors->GetTuple(i);
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
  vtkDataArray *inScalars;
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
    s = inScalars->GetComponent(i,0);
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
  vtkDataArray *inTCoords;
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
    t = inTCoords->GetTuple(i);
    fprintf(textureFp, "%e %e", t[0], t[1]);
    }

  fclose (textureFp);
  vtkDebugMacro(<<"Wrote " << numPts << " texture coordinates");
}

void vtkBYUWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

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

