/*=========================================================================

  Program:   Visualization Library
  Module:    VoxelR.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#include <ctype.h>
#include "VoxelR.hh"

void vlVoxelReader::Execute()
{
  FILE *fp;
  vlBitScalars *newScalars;
  int i,numPts;
  int bitcount;
  unsigned char uc;
  float f[3];
  int ti[3];

  // Initialize
  this->Initialize();

  if ((fp = fopen(this->Filename, "r")) == NULL)
    {
    vlErrorMacro(<< "File " << this->Filename << " not found");
    return;
    }

  // read the header
  fscanf(fp,"Voxel Data File\n");

  // read in the 
  fscanf(fp,"Origin: %f %f %f\n",f,f+1,f+2);
  this->SetOrigin(f);

  fscanf(fp,"Aspect: %f %f %f\n",f,f+1,f+2);
  this->SetAspectRatio(f);

  fscanf(fp,"Dimensions: %i %i %i\n",ti,ti+1,ti+2);
  this->SetDimensions(ti);

  numPts = this->Dimensions[0] * this->Dimensions[1] * this->Dimensions[2];
  newScalars = new vlBitScalars(numPts);

  bitcount = 0;
  for (i=0; i<numPts;) 
    {
    uc = fgetc(fp);
    newScalars->SetScalar(i++,(int)(uc&0x80));
    newScalars->SetScalar(i++,(int)(uc&0x40));
    newScalars->SetScalar(i++,(int)(uc&0x20));
    newScalars->SetScalar(i++,(int)(uc&0x10));
    newScalars->SetScalar(i++,(int)(uc&0x08));
    newScalars->SetScalar(i++,(int)(uc&0x04));
    newScalars->SetScalar(i++,(int)(uc&0x02));
    newScalars->SetScalar(i++,(int)(uc&0x01));
    }

  vlDebugMacro(<< "Read " << numPts<< " points");

  this->PointData.SetScalars(newScalars);
}

void vlVoxelReader::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlVoxelReader::GetClassName()))
    {
    vlStructuredPointsSource::PrintSelf(os,indent);

    os << indent << "Filename: " << this->Filename << "\n";
    }
}
