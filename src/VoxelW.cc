/*=========================================================================

  Program:   Visualization Library
  Module:    VoxelW.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#include "VoxelW.hh"
#include "BScalars.hh"

vlVoxelWriter::vlVoxelWriter()
{
  this->Filename = NULL;
}

vlVoxelWriter::~vlVoxelWriter()
{
  if ( this->Filename ) delete [] this->Filename;
}

// Description:
// Write voxel data out.
void vlVoxelWriter::Write()
{
  FILE *fp;
  int i, j, k;
  vlBitScalars *newScalars;
  int numPts, idx;
  int bitcount;
  unsigned char uc;
  int *dim;
  float *origin,*aspect;
  vlStructuredPoints *input=(vlStructuredPoints *)this->Input;

  vlDebugMacro(<< "Writing Voxel model");

  // update the data
  input->Update();
  
  dim = input->GetDimensions();
  origin = input->GetOrigin();
  aspect = input->GetAspectRatio();
  numPts = dim[0]*dim[1]*dim[2];

  newScalars = (vlBitScalars *)input->GetPointData()->GetScalars();
  
  if ( this->Filename == NULL)
    {
    vlErrorMacro(<< "Please specify filename to write");
    return;
    }

  fp = fopen(this->Filename,"w");
  if (!fp) 
    {
    vlErrorMacro(<< "Couldn't open file: " << this->Filename << endl);
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

void vlVoxelWriter::PrintSelf(ostream& os, vlIndent indent)
{
  vlStructuredPointsFilter::_PrintSelf(os,indent);
  vlWriter::PrintSelf(os,indent);

  os << indent << "Filename: " << this->Filename << "\n";
}


