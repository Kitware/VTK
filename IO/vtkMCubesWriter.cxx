/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMCubesWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMCubesWriter.h"

#include "vtkByteSwap.h"
#include "vtkCellArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"

vtkCxxRevisionMacro(vtkMCubesWriter, "1.32");
vtkStandardNewMacro(vtkMCubesWriter);

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
static void WriteMCubes(FILE *fp, vtkPoints *pts, vtkDataArray *normals, 
                        vtkCellArray *polys);
static void WriteLimits(FILE *fp, double *bounds);

// Write out data in MOVIE.BYU format.
void vtkMCubesWriter::WriteData()
{
  vtkPoints *pts;
  vtkDataArray *normals;
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

void WriteMCubes(FILE *fp, vtkPoints *pts, vtkDataArray *normals, 
                 vtkCellArray *polys)
{
  typedef struct {float x[3], n[3];} pointType;
  pointType point;
  int i;
  vtkIdType npts;
  vtkIdType *indx = 0;

  //  Write out triangle polygons.  In not a triangle polygon, create
  //  triangles.
  //
  double p[3], n[3];
  for (polys->InitTraversal(); polys->GetNextCell(npts,indx); )
    {
    for (i=0; i < 3; i++)
      {
      pts->GetPoint(indx[i],p);
      normals->GetTuple(indx[i],n);
      point.x[0] = (float)p[0];
      point.x[1] = (float)p[1];
      point.x[2] = (float)p[2];
      point.n[0] = (float)n[0];
      point.n[1] = (float)n[1];
      point.n[2] = (float)n[2];            
      vtkByteSwap::SwapWrite4BERange((float *) (&point),6,fp);
      }
    }
}
void WriteLimits(FILE *fp, double *bounds)
{
  float fbounds[6];
  fbounds[0] = (float)bounds[0];
  fbounds[1] = (float)bounds[1];
  fbounds[2] = (float)bounds[2];
  fbounds[3] = (float)bounds[3];
  fbounds[4] = (float)bounds[4];
  fbounds[5] = (float)bounds[5];  
  vtkByteSwap::SwapWrite4BERange((float *) fbounds,6,fp);
  vtkByteSwap::SwapWrite4BERange((float *) fbounds,6,fp);
}

void vtkMCubesWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Limits File Name: " 
     << (this->LimitsFileName ? this->LimitsFileName : "(none)") << "\n";
}

