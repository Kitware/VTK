/*=========================================================================

  Program:   Visualization Toolkit
  Module:    VoxelW.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#include "VoxelW.hh"
#include "BScalars.hh"

vtkVoxelWriter::vtkVoxelWriter()
{
  this->Filename = NULL;
}

vtkVoxelWriter::~vtkVoxelWriter()
{
  if ( this->Filename ) delete [] this->Filename;
}

// Description:
// Specify the input data or filter.
void vtkVoxelWriter::SetInput(vtkStructuredPoints *input)
{
  if ( this->Input != input )
    {
    vtkDebugMacro(<<" setting Input to " << (void *)input);
    this->Input = (vtkDataSet *) input;
    this->Modified();
    }
}

// Description:
// Write voxel data out.
void vtkVoxelWriter::WriteData()
{
  FILE *fp;
  int i, j, k;
  vtkBitScalars *newScalars;
  int numPts, idx;
  int bitcount;
  unsigned char uc;
  int *dim;
  float *origin,*aspect;
  vtkStructuredPoints *input=(vtkStructuredPoints *)this->Input;

  vtkDebugMacro(<< "Writing Voxel model");

  dim = input->GetDimensions();
  origin = input->GetOrigin();
  aspect = input->GetAspectRatio();
  numPts = dim[0]*dim[1]*dim[2];

  newScalars = (vtkBitScalars *)input->GetPointData()->GetScalars();
  
  if ( this->Filename == NULL)
    {
    vtkErrorMacro(<< "Please specify filename to write");
    return;
    }

  fp = fopen(this->Filename,"w");
  if (!fp) 
    {
    vtkErrorMacro(<< "Couldn't open file: " << this->Filename << endl);
    return;
    }

  fprintf(fp,"Voxel Data File\n");
  fprintf(fp,"Origin: %f %f %f\n",origin[0],origin[1],origin[2]);
  fprintf(fp,"Aspect: %f %f %f\n",aspect[0],aspect[1],aspect[2]);
  fprintf(fp,"Dimensions: %i %i %i\n",dim[0],dim[1],dim[2]);

  // write out the data
  bitcount = 0;
  idx = 0;
  uc = 0x00;

  for (k = 0; k < dim[2]; k++)
    for (j = 0; j < dim[1]; j++)
      for (i = 0; i < dim[0]; i++)
	{
	if (newScalars->GetScalar(idx))
	  {
	  uc |= (0x80 >> bitcount);
	  }
	bitcount++;
	if (bitcount == 8)
	  {
	  fputc(uc,fp);
	  uc = 0x00;
	  bitcount = 0;
	  }
	idx++;
	}
  if (bitcount)
    {
    fputc(uc,fp);
    }

  fclose(fp);
}

void vtkVoxelWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkWriter::PrintSelf(os,indent);

  os << indent << "Filename: " << this->Filename << "\n";
}


