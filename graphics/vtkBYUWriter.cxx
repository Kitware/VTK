/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBYUWriter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkBYUWriter.h"

// Description:
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
  if ( this->GeometryFileName ) delete [] this->GeometryFileName;
  if ( this->DisplacementFileName ) delete [] this->DisplacementFileName;
  if ( this->ScalarFileName ) delete [] this->ScalarFileName;
  if ( this->TextureFileName ) delete [] this->TextureFileName;
}

// Description:
// Write out data in MOVIE.BYU format.
void vtkBYUWriter::WriteData()
{
  FILE *geomFp;
  vtkPolyData *input=(vtkPolyData *)this->Input;
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
  int npts, *pts;
  vtkPoints *inPts;
  vtkCellArray *inPolys;
  vtkPolyData *input=(vtkPolyData *)this->Input;
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
    if ( (i % 2) ) fprintf(geomFile, "\n");
    }
  if ( (numPts % 2) ) fprintf(geomFile, "\n");

  // write poly data. Remember 1-offset.
  for (inPolys->InitTraversal(); inPolys->GetNextCell(npts,pts); )
    {
    // write this polygon
    for (i=0; i < (npts-1); i++) fprintf (geomFile, "%d ", pts[i]+1);
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
  vtkPolyData *input=(vtkPolyData *)this->Input;

  if ( this->WriteDisplacement && this->DisplacementFileName &&
  (inVectors = input->GetPointData()->GetVectors()) != NULL )
    {
    if ( !(dispFp = fopen(this->DisplacementFileName, "r")) )
      {
      vtkErrorMacro (<<"Couldn't open displacement file");
      return;
      }
    }
  else return;
//
// Write data
//
  for (i=0; i < numPts; i++)
    {
    v = inVectors->GetVector(i);
    fprintf(dispFp, "%e %e %e", v[0], v[1], v[2]);
    if ( (i % 2) ) fprintf (dispFp, "\n");
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
  vtkPolyData *input=(vtkPolyData *)this->Input;

  if ( this->WriteScalar && this->ScalarFileName &&
  (inScalars = input->GetPointData()->GetScalars()) != NULL )
    {
    if ( !(scalarFp = fopen(this->ScalarFileName, "r")) )
      {
      vtkErrorMacro (<<"Couldn't open scalar file");
      return;
      }
    }
  else return;
//
// Write data
//
  for (i=0; i < numPts; i++)
    {
    s = inScalars->GetScalar(i);
    fprintf(scalarFp, "%e", s);
    if ( i != 0 && !(i % 6) ) fprintf (scalarFp, "\n");
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
  vtkPolyData *input=(vtkPolyData *)this->Input;

  if ( this->WriteTexture && this->TextureFileName &&
  (inTCoords = input->GetPointData()->GetTCoords()) != NULL )
    {
    if ( !(textureFp = fopen(this->TextureFileName, "r")) )
      {
      vtkErrorMacro (<<"Couldn't open texture file");
      return;
      }
    }
  else return;
//
// Write data
//
  for (i=0; i < numPts; i++)
    {
    if ( i != 0 && !(i % 3) ) fprintf (textureFp, "\n");
    t = inTCoords->GetTCoord(i);
    fprintf(textureFp, "%e %e", t[0], t[1]);
    }

  fclose (textureFp);
  vtkDebugMacro(<<"Wrote " << numPts << " texture coordinates");
}

void vtkBYUWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkWriter::PrintSelf(os,indent);

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

