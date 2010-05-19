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
  this->WriteMCubes(fp, pts, normals, polys);
  fclose (fp);

  if (this->LimitsFileName) 
    {
    vtkDebugMacro("Writing MCubes limits file");
    if ((fp = fopen(this->LimitsFileName, "w")) == NULL)
      {
      vtkErrorMacro(<< "Couldn't open file: " << this->LimitsFileName);
      return;
      }
    this->WriteLimits(fp, input->GetBounds ());
    fclose (fp);
  }
}

void vtkMCubesWriter::WriteMCubes(FILE *fp, vtkPoints *pts,
                                  vtkDataArray *normals, 
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
  bool status=true;
  for (polys->InitTraversal(); polys->GetNextCell(npts,indx) && status; )
    {
    for (i=0; i < 3 && status; i++)
      {
      pts->GetPoint(indx[i],p);
      normals->GetTuple(indx[i],n);
      point.x[0] = static_cast<float>(p[0]);
      point.x[1] = static_cast<float>(p[1]);
      point.x[2] = static_cast<float>(p[2]);
      point.n[0] = static_cast<float>(n[0]);
      point.n[1] = static_cast<float>(n[1]);
      point.n[2] = static_cast<float>(n[2]);
      status=vtkByteSwap::SwapWrite4BERange(reinterpret_cast<float *>(&point),
                                            6,fp);
      if(!status)
        {
        vtkErrorMacro(<< "SwapWrite failed.");
        }
      }
    }
}
void vtkMCubesWriter::WriteLimits(FILE *fp, double *bounds)
{
  float fbounds[6];
  fbounds[0] = static_cast<float>(bounds[0]);
  fbounds[1] = static_cast<float>(bounds[1]);
  fbounds[2] = static_cast<float>(bounds[2]);
  fbounds[3] = static_cast<float>(bounds[3]);
  fbounds[4] = static_cast<float>(bounds[4]);
  fbounds[5] = static_cast<float>(bounds[5]);
  bool status=vtkByteSwap::SwapWrite4BERange(fbounds,6,fp);
  if(!status)
    {
    vtkErrorMacro(<< "SwapWrite failed.");
    }
  else
    {
    status=vtkByteSwap::SwapWrite4BERange(fbounds,6,fp);
    if(!status)
      {
      vtkErrorMacro(<< "SwapWrite failed.");
      }
    }
}

void vtkMCubesWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Limits File Name: " 
     << (this->LimitsFileName ? this->LimitsFileName : "(none)") << "\n";
}

