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
#include "vtkSmartPointer.h"

#include "vtkByteSwap.h"
#include "vtkCellArray.h"
#include "vtkErrorCode.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkTriangle.h"
#include "vtkTriangleStrip.h"
#include <vtksys/SystemTools.hxx>

#if !defined(_WIN32) || defined(__CYGWIN__)
# include <unistd.h> /* unlink */
#else
# include <io.h> /* unlink */
#endif

vtkStandardNewMacro(vtkSTLWriter);

static char header[]="Visualization Toolkit generated SLA File                                        ";

vtkSTLWriter::vtkSTLWriter()
{
  this->FileType = VTK_ASCII;
  this->FileName = NULL;
  this->Header = new char[257];
  strcpy(this->Header, header);
}

void vtkSTLWriter::WriteData()
{
  vtkPoints *pts;
  vtkCellArray *polys;
  vtkCellArray *strips;
  vtkPolyData *input = this->GetInput();

  polys = input->GetPolys();
  strips = input->GetStrips();
  pts = input->GetPoints();
  if (pts == NULL || polys == NULL)
  {
    vtkErrorMacro(<<"No data to write!");
    this->SetErrorCode(vtkErrorCode::UnknownError);
    return;
  }

  if (this->FileName == NULL)
  {
    vtkErrorMacro(<< "Please specify FileName to write");
    this->SetErrorCode(vtkErrorCode::NoFileNameError);
    return;
  }

  if (this->FileType == VTK_BINARY)
  {
    this->WriteBinarySTL(pts,polys,strips);
    if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
    {
      vtkErrorMacro("Ran out of disk space; deleting file: "
                    << this->FileName);
      unlink(this->FileName);
    }
  }
  else
  {
    this->WriteAsciiSTL(pts,polys,strips);
    if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
    {
      vtkErrorMacro("Ran out of disk space; deleting file: "
                    << this->FileName);
      unlink(this->FileName);
    }
  }
}

void vtkSTLWriter::WriteAsciiSTL(
  vtkPoints *pts, vtkCellArray *polys, vtkCellArray *strips)
{
  FILE *fp;
  double n[3], v1[3], v2[3], v3[3];
  vtkIdType npts = 0;
  vtkIdType *indx = 0;

  if ((fp = fopen(this->FileName, "w")) == NULL)
  {
    vtkErrorMacro(<< "Couldn't open file: " << this->FileName << " Reason: "
                  << vtksys::SystemTools::GetLastSystemError());
    this->SetErrorCode(vtkErrorCode::CannotOpenFileError);
    return;
  }
//
//  Write header
//
  vtkDebugMacro("Writing ASCII sla file");
  fprintf (fp, "solid ascii\n");

//
// Decompose any triangle strips into triangles
//
  vtkSmartPointer<vtkCellArray> polyStrips =
    vtkSmartPointer<vtkCellArray>::New();
  if (strips->GetNumberOfCells() > 0)
  {
    vtkIdType *ptIds = 0;
    for (strips->InitTraversal(); strips->GetNextCell(npts,ptIds);)
    {
      vtkTriangleStrip::DecomposeStrip(npts,ptIds,polyStrips);
    }
  }

  //  Write out triangle strips
  //
  for (polyStrips->InitTraversal(); polyStrips->GetNextCell(npts,indx); )
  {
    pts->GetPoint(indx[0],v1);
    pts->GetPoint(indx[1],v2);
    pts->GetPoint(indx[2],v3);

    vtkTriangle::ComputeNormal(pts, npts, indx, n);

    fprintf (fp, " facet normal %.6g %.6g %.6g\n  outer loop\n",
             n[0], n[1], n[2]);
    fprintf (fp, "   vertex %.6g %.6g %.6g\n", v1[0], v1[1], v1[2]);
    fprintf (fp, "   vertex %.6g %.6g %.6g\n", v2[0], v2[1], v2[2]);
    fprintf (fp, "   vertex %.6g %.6g %.6g\n", v3[0], v3[1], v3[2]);
    fprintf (fp, "  endloop\n endfacet\n");
  }

  //  Write out triangle polygons.  If not a triangle polygon, report
  //  an error
  //
  for (polys->InitTraversal(); polys->GetNextCell(npts,indx); )
  {
    if (npts > 3)
    {
      fclose(fp);
      vtkErrorMacro(<<"STL file only supports triangles");
      this->SetErrorCode(vtkErrorCode::FileFormatError);
      return;
    }

    pts->GetPoint(indx[0],v1);
    pts->GetPoint(indx[1],v2);
    pts->GetPoint(indx[2],v3);

    vtkTriangle::ComputeNormal(pts, npts, indx, n);

    fprintf (fp, " facet normal %.6g %.6g %.6g\n  outer loop\n",
             n[0], n[1], n[2]);
    fprintf (fp, "   vertex %.6g %.6g %.6g\n", v1[0], v1[1], v1[2]);
    fprintf (fp, "   vertex %.6g %.6g %.6g\n", v2[0], v2[1], v2[2]);
    fprintf (fp, "   vertex %.6g %.6g %.6g\n", v3[0], v3[1], v3[2]);
    fprintf (fp, "  endloop\n endfacet\n");
  }

  fprintf (fp, "endsolid\n");
  if(fflush(fp))
  {
    fclose(fp);
    this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
    return;
  }
  fclose (fp);
}

void vtkSTLWriter::WriteBinarySTL(
    vtkPoints *pts, vtkCellArray *polys, vtkCellArray *strips)
{
  FILE *fp;
  double dn[3], v1[3], v2[3], v3[3];
  vtkIdType npts = 0;
  vtkIdType *indx = 0;
  unsigned long ulint;
  unsigned short ibuff2=0;

  if ((fp = fopen(this->FileName, "wb")) == NULL)
  {
    vtkErrorMacro(<< "Couldn't open file: " << this->FileName << " Reason: "
                  << vtksys::SystemTools::GetLastSystemError());
    this->SetErrorCode(vtkErrorCode::CannotOpenFileError);
    return;
  }

  //  Write header
  //
  vtkDebugMacro("Writing Binary STL file");

  char szHeader[80+1];

  // Check for STL ASCII format key word 'solid'. According to STL file format
  // only ASCII files can have 'solid' as start key word, so we ignore it and
  // use VTK string instead.
  if (vtksys::SystemTools::StringStartsWith(this->Header, "solid"))
  {
    vtkErrorMacro("Invalid header for Binary STL file. Cannot start with \"solid\". Changing to header to\n" << header);
    strcpy(szHeader, header);
  }
  else
  {
    memset(szHeader, 32, 80);  // fill with space (ASCII=>32)
    snprintf(szHeader, sizeof(szHeader), "%s", this->Header);
  }

  fwrite (szHeader, 1, 80, fp);

  ulint = (unsigned long int) polys->GetNumberOfCells();
  vtkByteSwap::Swap4LE(&ulint);
  fwrite (&ulint, 1, 4, fp);

//
// Decompose any triangle strips into triangles
//
  vtkSmartPointer<vtkCellArray> polyStrips =
    vtkSmartPointer<vtkCellArray>::New();
  if (strips->GetNumberOfCells() > 0)
  {
    vtkIdType *ptIds = 0;
    for (strips->InitTraversal(); strips->GetNextCell(npts,ptIds);)
    {
      vtkTriangleStrip::DecomposeStrip(npts,ptIds,polyStrips);
    }
  }

  //  Write out triangle strips
  //
  for (polyStrips->InitTraversal(); polyStrips->GetNextCell(npts,indx); )
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
    fwrite (n, 4, 3, fp);

    n[0] = (float)v1[0];  n[1] = (float)v1[1];  n[2] = (float)v1[2];
    vtkByteSwap::Swap4LE(n);
    vtkByteSwap::Swap4LE(n+1);
    vtkByteSwap::Swap4LE(n+2);
    fwrite (n, 4, 3, fp);

    n[0] = (float)v2[0];  n[1] = (float)v2[1];  n[2] = (float)v2[2];
    vtkByteSwap::Swap4LE(n);
    vtkByteSwap::Swap4LE(n+1);
    vtkByteSwap::Swap4LE(n+2);
    fwrite (n, 4, 3, fp);

    n[0] = (float)v3[0];  n[1] = (float)v3[1];  n[2] = (float)v3[2];
    vtkByteSwap::Swap4LE(n);
    vtkByteSwap::Swap4LE(n+1);
    vtkByteSwap::Swap4LE(n+2);
    fwrite (n, 4, 3, fp);

    fwrite (&ibuff2, 2, 1, fp);
  }

  //  Write out triangle polygons.  In not a triangle polygon, report
  //  an error
  //
  for (polys->InitTraversal(); polys->GetNextCell(npts,indx); )
  {
    if (npts > 3)
    {
      fclose(fp);
      vtkErrorMacro(<<"STL file only supports triangles");
      this->SetErrorCode(vtkErrorCode::FileFormatError);
      return;
    }

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
    fwrite (n, 4, 3, fp);

    n[0] = (float)v1[0];  n[1] = (float)v1[1];  n[2] = (float)v1[2];
    vtkByteSwap::Swap4LE(n);
    vtkByteSwap::Swap4LE(n+1);
    vtkByteSwap::Swap4LE(n+2);
    fwrite (n, 4, 3, fp);

    n[0] = (float)v2[0];  n[1] = (float)v2[1];  n[2] = (float)v2[2];
    vtkByteSwap::Swap4LE(n);
    vtkByteSwap::Swap4LE(n+1);
    vtkByteSwap::Swap4LE(n+2);
    fwrite (n, 4, 3, fp);

    n[0] = (float)v3[0];  n[1] = (float)v3[1];  n[2] = (float)v3[2];
    vtkByteSwap::Swap4LE(n);
    vtkByteSwap::Swap4LE(n+1);
    vtkByteSwap::Swap4LE(n+2);
    fwrite (n, 4, 3, fp);
    fwrite (&ibuff2, 2, 1, fp);
  }
  if(fflush(fp))
  {
    fclose(fp);
    this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
    return;
  }
  fclose (fp);
}

//----------------------------------------------------------------------------
void vtkSTLWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "FileName: "
     << ((this->GetFileName() == NULL) ?
         "(none)" : this->GetFileName()) << std::endl;
  os << indent << "FileType: "
     << ((this->GetFileType() == VTK_ASCII) ?
         "VTK_ASCII" : "VTK_BINARY") << std::endl;
  os << indent << "Header: " << this->GetHeader() << std::endl;
  os << indent << "Input: " << this->GetInput() << std::endl;
}

//----------------------------------------------------------------------------
vtkPolyData* vtkSTLWriter::GetInput()
{
  return vtkPolyData::SafeDownCast(this->GetInput(0));
}

//----------------------------------------------------------------------------
vtkPolyData* vtkSTLWriter::GetInput(int port)
{
  return vtkPolyData::SafeDownCast(this->Superclass::GetInput(port));
}

//----------------------------------------------------------------------------
int vtkSTLWriter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  return 1;
}
