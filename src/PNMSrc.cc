/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PNMSrc.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "PNMSrc.hh"

vtkPNMSource::vtkPNMSource()
{
  this->Filename = NULL;
  this->ImageRange[0] = this->ImageRange[1] = -1;

  this->DataOrigin[0] = this->DataOrigin[1] = this->DataOrigin[2] = 0.0;
  this->DataAspectRatio[0] = this->DataAspectRatio[1] = this->DataAspectRatio[2] = 1.0;
}

void vtkPNMSource::Execute()
{
  vtkColorScalars *newScalars;
  int dim[3];

  this->Initialize();

  if ( this->Filename == NULL )
    {
    vtkErrorMacro(<<"Please specify a filename!");
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

vtkColorScalars *vtkPNMSource::ReadImage(int dim[3])
{
  char magic[80];
  int max;
  vtkPixmap *pixmap;
  vtkGraymap *graymap;
  vtkBitmap *bitmap;
  vtkColorScalars *s=NULL;
  FILE *fp;
  int numPts;

  dim[2] = 1;

  if ( !(fp = fopen(this->Filename,"r")) )
    {
    vtkErrorMacro(<<"Can't find file: " << this->Filename);
    return NULL;
    }

  fscanf(fp,"%s %d %d", magic, dim, dim+1);

  // check input
  if ( (numPts = dim[0]*dim[1]) < 1 )
    {
    vtkErrorMacro(<<"Bad input data!");
    return NULL;
    }

  // compare magic number to see proper file type
  if ( ! strcmp(magic,"P4") ) //pbm file
    {
    bitmap = new vtkBitmap(numPts);
    }

  else if ( ! strcmp(magic,"P5") ) //pgm file
    {
    graymap = new vtkGraymap(numPts);
    if ( this->ReadBinaryPGM(fp,graymap,numPts,dim[0],dim[1]) )
      {
      s = (vtkColorScalars *) graymap;
      }
    else
      {
      delete graymap;
      }
    }

  else if ( ! strcmp(magic,"P6") ) //ppm file
    {
    pixmap = new vtkPixmap(numPts);
    if ( this->ReadBinaryPPM(fp,pixmap,numPts,dim[0],dim[1]) )
      {
      s = (vtkColorScalars *) pixmap;
      }
    else
      {
      delete pixmap;
      }
    }
  else
    {
    vtkErrorMacro(<<"Unknown file type!");
    return NULL;
    }

  return s;
}

vtkColorScalars *vtkPNMSource::ReadVolume(int dim[3])
{
  vtkColorScalars *s=NULL;

  return s;
}


int vtkPNMSource::ReadBinaryPBM(FILE *fp, vtkBitmap* bitmap, int numPts,
                               int xsize, int ysize)
{
  int max, j, packedXSize=xsize/8;
  unsigned char *cptr;

  fscanf(fp, "%d", &max);
//
// Since pnm coordinate system is at upper left of image, need to convert
// to lower rh corner origin by reading a row at a time.
//
  for (j=0; j<ysize; j++)
    {
    cptr = bitmap->WritePtr(numPts-(ysize-(j+1))*packedXSize,packedXSize);
    if ( ! fread(cptr,1,packedXSize,fp) )
      {
      vtkErrorMacro(<<"Error reaading raw pbm data!");
      return 0;
      }
    }

  return 1;
}

int vtkPNMSource::ReadBinaryPGM(FILE *fp, vtkGraymap* graymap, int numPts,
                               int xsize, int ysize)
{
  int max, j;
  unsigned char *cptr;

  fscanf(fp, "%d", &max);
//
// Since pnm coordinate system is at upper left of image, need to convert
// to lower rh corner origin by reading a row at a time.
//
  for (j=0; j<ysize; j++)
    {
    cptr = graymap->WritePtr(numPts-(ysize-(j+1))*xsize,xsize);
    if ( ! fread(cptr,1,xsize,fp) )
      {
      vtkErrorMacro(<<"Error reaading raw pgm data!");
      return 0;
      }
    }

  return 1;
}

int vtkPNMSource::ReadBinaryPPM(FILE *fp, vtkPixmap* pixmap, int numPts,
                               int xsize, int ysize)
{
  int max, j;
  unsigned char *cptr;

  fscanf(fp, "%d", &max);
//
// Since pnm coordinate system is at upper left of image, need to convert
// to lower rh corner origin by reading a row at a time.
//
  for (j=0; j<ysize; j++)
    {
    cptr = pixmap->WritePtr(numPts-(ysize-(j+1))*xsize,xsize);
    if ( ! fread(cptr,3,xsize,fp) )
      {
      vtkErrorMacro(<<"Error reaading raw ppm data!");
      return 0;
      }
    }

  return 1;
}

void vtkPNMSource::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStructuredPointsSource::PrintSelf(os,indent);

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
