/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSTLWriter.cxx
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
#include "vtkSTLWriter.h"
#include "vtkTriangle.h"
#include "vtkByteSwap.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkSTLWriter* vtkSTLWriter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkSTLWriter");
  if(ret)
    {
    return (vtkSTLWriter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkSTLWriter;
}




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
    return;
    }

  if ( this->FileType == VTK_BINARY )
    {
    this->WriteBinarySTL(pts,polys);
    }
  else
    {
    this->WriteAsciiSTL(pts,polys);
    }
}

static char header[]="Visualization Toolkit generated SLA File                                        ";

void vtkSTLWriter::WriteAsciiSTL(vtkPoints *pts, vtkCellArray *polys)
{
  FILE *fp;
  float n[3], *v1, *v2, *v3;
  vtkIdType npts;
  vtkIdType *indx;
  
  if ((fp = fopen(this->FileName, "w")) == NULL)
    {
    vtkErrorMacro(<< "Couldn't open file: " << this->FileName);
    return;
    }
//
//  Write header
//
  vtkDebugMacro("Writing ASCII sla file");
  fprintf (fp, "solid ascii\n");
//
//  Write out triangle polygons.  In not a triangle polygon, only first 
//  three vertices are written.
//
  for (polys->InitTraversal(); polys->GetNextCell(npts,indx); )
    {
    v1 = pts->GetPoint(indx[0]);
    v2 = pts->GetPoint(indx[1]);
    v3 = pts->GetPoint(indx[2]);

    vtkTriangle::ComputeNormal(pts, npts, indx, n);

    fprintf (fp, " facet normal %.6g %.6g %.6g\n  outer loop\n",
            n[0], n[1], n[2]);

    fprintf (fp, "   vertex %.6g %.6g %.6g\n", v1[0], v1[1], v1[2]);
    fprintf (fp, "   vertex %.6g %.6g %.6g\n", v2[0], v2[1], v2[2]);
    fprintf (fp, "   vertex %.6g %.6g %.6g\n", v3[0], v3[1], v3[2]);

    fprintf (fp, "  endloop\n endfacet\n");
    }
  fprintf (fp, "endsolid\n");
  fclose (fp);
}

void vtkSTLWriter::WriteBinarySTL(vtkPoints *pts, vtkCellArray *polys)
{
  FILE *fp;
  float n[3], *v1, *v2, *v3;
  vtkIdType npts;
  vtkIdType *indx;
  unsigned long ulint;
  unsigned short ibuff2=0;

  if ((fp = fopen(this->FileName, "wb")) == NULL)
    {
    vtkErrorMacro(<< "Couldn't open file: " << this->FileName);
    return;
    }
  
  //  Write header
  //
  vtkDebugMacro("Writing Binary STL file");
  fwrite (header, 1, 80, fp);

  ulint = (unsigned long int) polys->GetNumberOfCells();
  vtkByteSwap::Swap4LE(&ulint);
  fwrite (&ulint, 1, 4, fp);

  //  Write out triangle polygons.  In not a triangle polygon, only first 
  //  three vertices are written.
  //
  for (polys->InitTraversal(); polys->GetNextCell(npts,indx); )
    {
    v1 = pts->GetPoint(indx[0]);
    v2 = pts->GetPoint(indx[1]);
    v3 = pts->GetPoint(indx[2]);

    vtkTriangle::ComputeNormal(pts, npts, indx, n);
    vtkByteSwap::Swap4LE(n); 
    vtkByteSwap::Swap4LE(n+1); 
    vtkByteSwap::Swap4LE(n+2);
    fwrite (n, 4, 3, fp);

    n[0] = v1[0];  n[1] = v1[1];  n[2] = v1[2]; 
    vtkByteSwap::Swap4LE(n); 
    vtkByteSwap::Swap4LE(n+1); 
    vtkByteSwap::Swap4LE(n+2);
    fwrite (n, 4, 3, fp);

    n[0] = v2[0];  n[1] = v2[1];  n[2] = v2[2]; 
    vtkByteSwap::Swap4LE(n); 
    vtkByteSwap::Swap4LE(n+1); 
    vtkByteSwap::Swap4LE(n+2);
    fwrite (n, 4, 3, fp);

    n[0] = v3[0];  n[1] = v3[1];  n[2] = v3[2]; 
    vtkByteSwap::Swap4LE(n); 
    vtkByteSwap::Swap4LE(n+1); 
    vtkByteSwap::Swap4LE(n+2);
    fwrite (n, 4, 3, fp);

    fwrite (&ibuff2, 2, 1, fp);
    }
  fclose (fp);
}

