/*=========================================================================

  Program:   Visualization Toolkit
  Module:    %M%
  Language:  C++
  Date:      %D%
  Version:   %V%

Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkMCubesWriter.h"
#include "vtkByteSwap.h"

// Description:
// Create object.
vtkMCubesWriter::vtkMCubesWriter()
{
  this->FileName = NULL;
  this->LimitsFileName = NULL;
}

vtkMCubesWriter::~vtkMCubesWriter()
{
  if ( this->FileName ) delete [] this->FileName;
  if ( this->LimitsFileName ) delete [] this->LimitsFileName;
}
static void WriteMCubes(FILE *fp, vtkPoints *pts, vtkNormals *normals, vtkCellArray *polys);
static void WriteLimits(FILE *fp, float *bounds);

// Description:
// Write out data in MOVIE.BYU format.
void vtkMCubesWriter::WriteData()
{
  vtkPoints *pts;
  vtkNormals *normals;
  vtkCellArray *polys;
  vtkPolyData *input=(vtkPolyData *)this->Input;

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
    vtkErrorMacro(<<"No normals to write!: use vtkPolyNormals to generate them");
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

  if (this->LimitsFileName) {
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
  int npts, *indx;

//
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
  vtkPolyWriter::PrintSelf(os,indent);

  os << indent << "File Name: " 
     << (this->FileName ? this->FileName : "(none)") << "\n";

  os << indent << "Limits File Name: " 
     << (this->LimitsFileName ? this->LimitsFileName : "(none)") << "\n";
}

