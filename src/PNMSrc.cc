/*=========================================================================

  Program:   Visualization Library
  Module:    PNMSrc.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "PNMSrc.hh"

vlPNMSource::vlPNMSource()
{
  this->Filename = NULL;
  this->ImageRange[0] = this->ImageRange[1] = -1;

  this->DataOrigin[0] = this->DataOrigin[1] = this->DataOrigin[2] = 0.0;
  this->DataAspectRatio[0] = this->DataAspectRatio[1] = this->DataAspectRatio[2] = 1.0;
}

void vlPNMSource::Execute()
{
  vlColorScalars *newScalars;
  int dim[3];

  this->Initialize();

  if ( this->Filename == NULL )
    {
    vlErrorMacro(<<"Please specify a filename!");
    return;
    }

  if ( this->ImageRange[0] < 0 )
    {
    newScalars = this->ReadImage(dim);
    }
  else
    {
    newScalars = this->ReadVolume(dim);
    }

  this->SetDimensions(dim);
  this->SetAspectRatio(this->DataAspectRatio);
  this->SetOrigin(this->DataOrigin);
  this->PointData.SetScalars(newScalars);
}

vlColorScalars *vlPNMSource::ReadImage(int dim[3])
{
  char magic[80];
  int max;
  vlPixmap *pixmap;
  vlGraymap *graymap;
  vlBitmap *bitmap;
  vlColorScalars *s=NULL;
  FILE *fp;
  int numPts;

  dim[2] = 1;

  if ( !(fp = fopen(this->Filename,"r")) )
    {
    vlErrorMacro(<<"Can't find file: " << this->Filename);
    return NULL;
    }

  fscanf(fp,"%s %d %d", magic, dim, dim+1);

  // check input
  if ( (numPts = dim[0]*dim[1]) < 1 )
    {
    vlErrorMacro(<<"Bad input data!");
    return NULL;
    }

  // compare magic number to see proper file type
  if ( ! strcmp(magic,"P4") ) //pbm file
    {
    bitmap = new vlBitmap(numPts);
    }

  else if ( ! strcmp(magic,"P5") ) //pgm file
    {
    graymap = new vlGraymap(numPts);
    if ( this->ReadBinaryPGM(fp,graymap,numPts) )
      {
      s = (vlColorScalars *) graymap;
      }
    else
      {
      delete graymap;
      }
    }

  else if ( ! strcmp(magic,"P6") ) //ppm file
    {
    pixmap = new vlPixmap(numPts);
    if ( this->ReadBinaryPPM(fp,pixmap,numPts) )
      {
      s = (vlColorScalars *) pixmap;
      }
    else
      {
      delete pixmap;
      }
    }
  else
    {
    vlErrorMacro(<<"Unknown file type!");
    return NULL;
    }

  return s;
}

vlColorScalars *vlPNMSource::ReadVolume(int dim[3])
{
  vlColorScalars *s=NULL;

  return s;
}


int vlPNMSource::ReadBinaryPBM(FILE *fp, vlBitmap* bitmap, int numPts)
{
  return 1;
}

int vlPNMSource::ReadBinaryPGM(FILE *fp, vlGraymap* graymap, int numPts)
{
  int max;
  unsigned char *cptr;

  fscanf(fp, "%d", &max);
  cptr = graymap->WriteInto(graymap->GetNumberOfColors(),numPts);
  if ( ! fread(cptr,1,numPts,fp) )
    {
    vlErrorMacro(<<"Error reaading raw pgm data!");
    return 0;
    }
  return 1;
}

int vlPNMSource::ReadBinaryPPM(FILE *fp, vlPixmap* pixmap, int numPts)
{
  int max;
  unsigned char *cptr;

  fscanf(fp, "%d", &max);
  cptr = pixmap->WriteInto(pixmap->GetNumberOfColors(),numPts);
  if ( ! fread(cptr,3,numPts,fp) )
    {
    vlErrorMacro(<<"Error reaading raw ppm data!");
    return 0;
    }
  return 1;
}

void vlPNMSource::PrintSelf(ostream& os, vlIndent indent)
{
  vlStructuredPointsSource::PrintSelf(os,indent);

  os << indent << "Filename: " << this->Filename << "\n";
  os << indent << "Image Range: (" << this->ImageRange[0] << ", " 
     << this->ImageRange[1] << ")\n";
  os << indent << "Data Origin: (" << this->DataOrigin[0] << ", "
                                   << this->DataOrigin[1] << ", "
                                   << this->DataOrigin[2] << ")\n";
  os << indent << "AspectRatio: (" << this->DataAspectRatio[0] << ", "
                                   << this->DataAspectRatio[1] << ", "
                                   << this->DataAspectRatio[2] << ")\n";
}
