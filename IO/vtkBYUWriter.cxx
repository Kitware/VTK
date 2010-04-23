/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBYUWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBYUWriter.h"

#include "vtkCellArray.h"
#include "vtkErrorCode.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"

#if !defined(_WIN32) || defined(__CYGWIN__)
# include <unistd.h> /* unlink */
#else
# include <io.h> /* unlink */
#endif

#include <vtkstd/string>

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
    this->SetErrorCode(vtkErrorCode::NoFileNameError);
    return;
    }

  if ((geomFp = fopen(this->GeometryFileName, "w")) == NULL)
    {
    vtkErrorMacro(<< "Couldn't open geometry file: " << this->GeometryFileName);
    this->SetErrorCode(vtkErrorCode::CannotOpenFileError);
    return;
    }
  else
    {
    this->WriteGeometryFile(geomFp,numPts);
    if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
      {
      fclose(geomFp);
      vtkErrorMacro("Ran out of disk space; deleting file: "
                    << this->GeometryFileName);
      unlink(this->GeometryFileName);
      return;
      }
    }

  this->WriteDisplacementFile(numPts);
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
    {
    fclose(geomFp);
    unlink(this->GeometryFileName);
    unlink(this->DisplacementFileName);
    vtkErrorMacro("Ran out of disk space; deleting files: "
                  << this->GeometryFileName << " "
                  << this->DisplacementFileName);
    return;
    }
  this->WriteScalarFile(numPts);
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
    {
    vtkstd::string errorMessage;
    fclose(geomFp);
    unlink(this->GeometryFileName);
    errorMessage = "Ran out of disk space; deleting files: ";
    errorMessage += this->GeometryFileName;
    errorMessage += " ";
    if (this->DisplacementFileName)
      {
      unlink(this->DisplacementFileName);
      errorMessage += this->DisplacementFileName;
      errorMessage += " ";
      }
    unlink(this->ScalarFileName);
    errorMessage += this->ScalarFileName;
    vtkErrorMacro( << errorMessage.c_str());
    return;
    }
  this->WriteTextureFile(numPts);
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
    {
    fclose(geomFp);
    vtkstd::string errorMessage;
    unlink(this->GeometryFileName);
    errorMessage = "Ran out of disk space; deleting files: ";
    errorMessage += this->GeometryFileName;
    errorMessage += " ";
    if (this->DisplacementFileName)
      {
      unlink(this->DisplacementFileName);
      errorMessage += this->DisplacementFileName;
      errorMessage += " ";
      }
    if (this->ScalarFileName)
      {
      unlink(this->ScalarFileName);
      errorMessage += this->ScalarFileName;
      errorMessage += " ";
      }
    unlink(this->TextureFileName);
    errorMessage += this->TextureFileName;
    vtkErrorMacro( << errorMessage.c_str());
    return;
    }

  // Close the file
  fclose (geomFp);
}

void vtkBYUWriter::WriteGeometryFile(FILE *geomFile, int numPts)
{
  int numPolys, numEdges;
  int i;
  double *x;
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

  if (fprintf (geomFile, "%d %d %d %d\n", 1, numPts, numPolys, numEdges) < 0)
    {
    this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
    return;
    }
  if (fprintf (geomFile, "%d %d\n", 1, numPolys) < 0)
    {
    this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
    return;
    }
  
//
// Write data
//
  // write point coordinates
  for (i=0; i < numPts; i++)
    {
    x = inPts->GetPoint(i);
    if (fprintf(geomFile, "%e %e %e ", x[0], x[1], x[2]) < 0)
      {
      this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
      return;
      }
    if ( (i % 2) )
      {
      if (fprintf(geomFile, "\n") < 0)
        {
        this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
        return;
        }
      }
    }
  if ( (numPts % 2) )
    {
    if (fprintf(geomFile, "\n") < 0)
      {
      this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
      return;
      }
    }

  // write poly data. Remember 1-offset.
  for (inPolys->InitTraversal(); inPolys->GetNextCell(npts,pts); )
    {
    // write this polygon
    // treating vtkIdType as int
    for (i=0; i < (npts-1); i++)
      {
      if (fprintf (geomFile, "%d ", static_cast<int>(pts[i]+1)) < 0)
        {
        this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
        return;
        }
      }
    if (fprintf (geomFile, "%d\n", static_cast<int>(-(pts[npts-1]+1))) < 0)
      {
      this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
      return;
      }
    }

  vtkDebugMacro(<<"Wrote " << numPts << " points, " << numPolys << " polygons");
}

void vtkBYUWriter::WriteDisplacementFile(int numPts)
{
  FILE *dispFp;
  int i;
  double *v;
  vtkDataArray *inVectors;
  vtkPolyData *input= this->GetInput();

  if ( this->WriteDisplacement && this->DisplacementFileName &&
  (inVectors = input->GetPointData()->GetVectors()) != NULL )
    {
    if ( !(dispFp = fopen(this->DisplacementFileName, "w")) )
      {
      vtkErrorMacro (<<"Couldn't open displacement file");
      this->SetErrorCode(vtkErrorCode::CannotOpenFileError);
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
    if (fprintf(dispFp, "%e %e %e", v[0], v[1], v[2]) < 0)
      {
      this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
      fclose(dispFp);
      return;
      }
    if ( (i % 2) )
      {
      if (fprintf (dispFp, "\n") < 0)
        {
        this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
        fclose(dispFp);
        return;
        }
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
      this->SetErrorCode(vtkErrorCode::CannotOpenFileError);
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
    if (fprintf(scalarFp, "%e ", s) < 0)
      {
      this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
      fclose(scalarFp);
      return;
      }
    if ( i != 0 && !(i % 6) )
      {
      if (fprintf (scalarFp, "\n") < 0)
        {
        this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
        fclose(scalarFp);
        return;
        }
      }
    }

  fclose (scalarFp);
  vtkDebugMacro(<<"Wrote " << numPts << " scalars");
}

void vtkBYUWriter::WriteTextureFile(int numPts)
{
  FILE *textureFp;
  int i;
  double *t;
  vtkDataArray *inTCoords;
  vtkPolyData *input= this->GetInput();

  if ( this->WriteTexture && this->TextureFileName &&
  (inTCoords = input->GetPointData()->GetTCoords()) != NULL )
    {
    if ( !(textureFp = fopen(this->TextureFileName, "w")) )
      {
      vtkErrorMacro (<<"Couldn't open texture file");
      this->SetErrorCode(vtkErrorCode::CannotOpenFileError);
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
      if (fprintf (textureFp, "\n") < 0)
        {
        this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
        fclose(textureFp);
        return;
        }
      }
    t = inTCoords->GetTuple(i);
    if (fprintf(textureFp, "%e %e", t[0], t[1]) < 0)
      {
      this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
      fclose(textureFp);
      return;
      }
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

