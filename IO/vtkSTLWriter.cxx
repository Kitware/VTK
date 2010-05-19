/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSTLWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSTLWriter.h"

#include "vtkByteSwap.h"
#include "vtkCellArray.h"
#include "vtkErrorCode.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkTriangle.h"

#if !defined(_WIN32) || defined(__CYGWIN__)
# include <unistd.h> /* unlink */
#else
# include <io.h> /* unlink */
#endif

vtkStandardNewMacro(vtkSTLWriter);

vtkSTLWriter::vtkSTLWriter()
{
  this->FileType = VTK_ASCII;
}

void vtkSTLWriter::WriteData()
{
  vtkPoints *pts;
  vtkCellArray *polys;
  vtkPolyData *input = this->GetInput();

  polys = input->GetPolys();
  pts = input->GetPoints();
  if (pts == NULL || polys == NULL )
    {
    vtkErrorMacro(<<"No data to write!");
    return;
    }

  if ( this->FileName == NULL)
    {
    vtkErrorMacro(<< "Please specify FileName to write");
    this->SetErrorCode(vtkErrorCode::NoFileNameError);
    return;
    }

  if ( this->FileType == VTK_BINARY )
    {
    this->WriteBinarySTL(pts,polys);
    if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
      {
      vtkErrorMacro("Ran out of disk space; deleting file: "
                    << this->FileName);
      unlink(this->FileName);
      }
    }
  else
    {
    this->WriteAsciiSTL(pts,polys);
    if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
      {
      vtkErrorMacro("Ran out of disk space; deleting file: "
                    << this->FileName);
      unlink(this->FileName);
      }
    }
}

static char header[]="Visualization Toolkit generated SLA File                                        ";

void vtkSTLWriter::WriteAsciiSTL(vtkPoints *pts, vtkCellArray *polys)
{
  FILE *fp;
  double n[3], v1[3], v2[3], v3[3];
  vtkIdType npts = 0;
  vtkIdType *indx = 0;
  
  if ((fp = fopen(this->FileName, "w")) == NULL)
    {
    vtkErrorMacro(<< "Couldn't open file: " << this->FileName);
    this->SetErrorCode(vtkErrorCode::CannotOpenFileError);
    return;
    }
//
//  Write header
//
  vtkDebugMacro("Writing ASCII sla file");
  if (fprintf (fp, "solid ascii\n") < 0)
    {
    fclose(fp);
    this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
    return;
    }
//
//  Write out triangle polygons.  In not a triangle polygon, only first 
//  three vertices are written.
//
  for (polys->InitTraversal(); polys->GetNextCell(npts,indx); )
    {
    pts->GetPoint(indx[0],v1);
    pts->GetPoint(indx[1],v2);
    pts->GetPoint(indx[2],v3);

    vtkTriangle::ComputeNormal(pts, npts, indx, n);

    if (fprintf (fp, " facet normal %.6g %.6g %.6g\n  outer loop\n",
                 n[0], n[1], n[2]) < 0)
      {
      fclose(fp);
      this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
      return;
      }

    if (fprintf (fp, "   vertex %.6g %.6g %.6g\n", v1[0], v1[1], v1[2]) < 0)
      {
      fclose(fp);
      this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
      return;
      }
    if (fprintf (fp, "   vertex %.6g %.6g %.6g\n", v2[0], v2[1], v2[2]) < 0)
      {
      fclose(fp);
      this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
      return;
      }
    if (fprintf (fp, "   vertex %.6g %.6g %.6g\n", v3[0], v3[1], v3[2]) < 0)
      {
      fclose(fp);
      this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
      return;
      }

    if (fprintf (fp, "  endloop\n endfacet\n") < 0)
      {
      fclose(fp);
      this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
      return;
      }
    }
  if (fprintf (fp, "endsolid\n") < 0)
    {
    this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
    }
  fclose (fp);
}

void vtkSTLWriter::WriteBinarySTL(vtkPoints *pts, vtkCellArray *polys)
{
  FILE *fp;
  double dn[3], v1[3], v2[3], v3[3];
  vtkIdType npts = 0;
  vtkIdType *indx = 0;
  unsigned long ulint;
  unsigned short ibuff2=0;

  if ((fp = fopen(this->FileName, "wb")) == NULL)
    {
    vtkErrorMacro(<< "Couldn't open file: " << this->FileName);
    this->SetErrorCode(vtkErrorCode::CannotOpenFileError);
    return;
    }
  
  //  Write header
  //
  vtkDebugMacro("Writing Binary STL file");
  if (fwrite (header, 1, 80, fp) < 80)
    {
    fclose(fp);
    this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
    return;
    }

  ulint = (unsigned long int) polys->GetNumberOfCells();
  vtkByteSwap::Swap4LE(&ulint);
  if (fwrite (&ulint, 1, 4, fp) < 4)
    {
    fclose(fp);
    this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
    return;
    }

  //  Write out triangle polygons.  In not a triangle polygon, only first 
  //  three vertices are written.
  //
  for (polys->InitTraversal(); polys->GetNextCell(npts,indx); )
    {
    pts->GetPoint(indx[0],v1);
    pts->GetPoint(indx[1],v2);
    pts->GetPoint(indx[2],v3);
    
    vtkTriangle::ComputeNormal(pts, npts, indx, dn);
    float n[3];
    n[0] = (float)dn[0];
    n[1] = (float)dn[1];
    n[2] = (float)dn[2];    
    vtkByteSwap::Swap4LE(n); 
    vtkByteSwap::Swap4LE(n+1); 
    vtkByteSwap::Swap4LE(n+2);
    if (fwrite (n, 4, 3, fp) < 3)
      {
      fclose(fp);
      this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
      return;
      }

    n[0] = (float)v1[0];  n[1] = (float)v1[1];  n[2] = (float)v1[2]; 
    vtkByteSwap::Swap4LE(n); 
    vtkByteSwap::Swap4LE(n+1); 
    vtkByteSwap::Swap4LE(n+2);
    if (fwrite (n, 4, 3, fp) < 3)
      {
      fclose(fp);
      this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
      return;
      }

    n[0] = (float)v2[0];  n[1] = (float)v2[1];  n[2] = (float)v2[2]; 
    vtkByteSwap::Swap4LE(n); 
    vtkByteSwap::Swap4LE(n+1); 
    vtkByteSwap::Swap4LE(n+2);
    if (fwrite (n, 4, 3, fp) < 3)
      {
      fclose(fp);
      this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
      return;
      }

    n[0] = (float)v3[0];  n[1] = (float)v3[1];  n[2] = (float)v3[2]; 
    vtkByteSwap::Swap4LE(n); 
    vtkByteSwap::Swap4LE(n+1); 
    vtkByteSwap::Swap4LE(n+2);
    if (fwrite (n, 4, 3, fp) < 3)
      {
      fclose(fp);
      this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
      return;
      }

    if (fwrite (&ibuff2, 2, 1, fp) < 1)
      {
      fclose(fp);
      this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
      return;
      }
    }
  fclose (fp);
}

//----------------------------------------------------------------------------
void vtkSTLWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
