/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMCubesWriter.cxx
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
#include "vtkMCubesWriter.h"
#include "vtkByteSwap.h"
#include "vtkNormals.h"
#include "vtkObjectFactory.h"

//--------------------------------------------------------------------------
vtkMCubesWriter* vtkMCubesWriter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkMCubesWriter");
  if(ret)
    {
    return (vtkMCubesWriter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkMCubesWriter;
}

// Create object.
vtkMCubesWriter::vtkMCubesWriter()
{
  this->LimitsFileName = NULL;
}

vtkMCubesWriter::~vtkMCubesWriter()
{
  if ( this->LimitsFileName )
    {
    delete [] this->LimitsFileName;
    }
}
static void WriteMCubes(FILE *fp, vtkPoints *pts, vtkNormals *normals, vtkCellArray *polys);
static void WriteLimits(FILE *fp, float *bounds);

// Write out data in MOVIE.BYU format.
void vtkMCubesWriter::WriteData()
{
  vtkPoints *pts;
  vtkNormals *normals;
  vtkCellArray *polys;
  vtkPolyData *input=this->GetInput();

  polys = input->GetPolys();
  pts = input->GetPoints();
  if (pts == NULL || polys == NULL )
    {
    vtkErrorMacro(<<"No data to write!");
    return;
    }

  normals = input->GetPointData()->GetNormals();
  if (normals == NULL )
    {
    vtkErrorMacro(<<"No normals to write!: use vtkPolyDataNormals to generate them");
    return;
    }

  if ( this->FileName == NULL)
    {
    vtkErrorMacro(<< "Please specify FileName to write");
    return;
    }

  vtkDebugMacro("Writing MCubes tri file");
  FILE *fp;
  if ((fp = fopen(this->FileName, "w")) == NULL)
    {
    vtkErrorMacro(<< "Couldn't open file: " << this->FileName);
    return;
    }
  WriteMCubes (fp, pts, normals, polys);
  fclose (fp);

  if (this->LimitsFileName) 
    {
    vtkDebugMacro("Writing MCubes limits file");
    if ((fp = fopen(this->LimitsFileName, "w")) == NULL)
      {
      vtkErrorMacro(<< "Couldn't open file: " << this->LimitsFileName);
      return;
      }
    WriteLimits (fp, input->GetBounds ());
    fclose (fp);
  }
}

void WriteMCubes(FILE *fp, vtkPoints *pts, vtkNormals *normals, vtkCellArray *polys)
{
  typedef struct {float x[3], n[3];} pointType;
  pointType point;
  int i;
  int npts;
  vtkIdType *indx;

  //  Write out triangle polygons.  In not a triangle polygon, create triangles.
  //
  for (polys->InitTraversal(); polys->GetNextCell(npts,indx); )
    {
    for (i=0; i < 3; i++)
      {
      pts->GetPoint(indx[i],&point.x[0]);
      normals->GetNormal(indx[i],&point.n[0]);
      vtkByteSwap::SwapWrite4BERange((float *) (&point),6,fp);
      }
    }
}
void WriteLimits(FILE *fp, float *bounds)
{
  vtkByteSwap::SwapWrite4BERange((float *) bounds,6,fp);
  vtkByteSwap::SwapWrite4BERange((float *) bounds,6,fp);
}

void vtkMCubesWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataWriter::PrintSelf(os,indent);

  os << indent << "Limits File Name: " 
     << (this->LimitsFileName ? this->LimitsFileName : "(none)") << "\n";
}

