/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPNMReader.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkPNMReader.hh"

char vtkPNMReaderGetChar(FILE *fp)
{
  char c;
  int result;

  if ((result = getc(fp)) == EOF )
    {
    return '\0';
    }
  
  c = (char)result;
  if (c == '#')
    {
    do
      {
      if ((result = getc(fp)) == EOF )
	{
	return '\0';
	}
      c = (char)result;
      }
    while (c != '\n');
    }
  
  return c;
}

int vtkPNMReaderGetInt(FILE *fp)
{
  char c;
  int result = 0;
  
  do
    {
    c = vtkPNMReaderGetChar(fp);
    }
  while ((c < '1')||(c > '9'));
  do
    {
    result = result * 10 + (c - '0');
    c = vtkPNMReaderGetChar(fp);
    }
  while ((c >= '0')&&(c <= '9'));

  return result;
}
  

vtkPNMReader::vtkPNMReader()
{
  this->Filename = NULL;
  this->ImageRange[0] = this->ImageRange[1] = -1;

  this->DataOrigin[0] = this->DataOrigin[1] = this->DataOrigin[2] = 0.0;
  this->DataAspectRatio[0] = this->DataAspectRatio[1] = this->DataAspectRatio[2] = 1.0;
}

void vtkPNMReader::Execute()
{
  vtkColorScalars *newScalars;
  int dim[3];
  vtkStructuredPoints *output = this->GetOutput();
  

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

  output->SetDimensions(dim);
  output->SetAspectRatio(this->DataAspectRatio);
  output->SetOrigin(this->DataOrigin);
  output->GetPointData()->SetScalars(newScalars);
  newScalars->Delete();
}

vtkColorScalars *vtkPNMReader::ReadImage(int dim[3])
{
  char magic[80];
  vtkPixmap *pixmap;
  vtkGraymap *graymap;
  vtkBitmap *bitmap;
  vtkColorScalars *s=NULL;
  FILE *fp;
  int numPts;
  char c;
  
  dim[2] = 1;

  if ( !(fp = fopen(this->Filename,"r")) )
    {
    vtkErrorMacro(<<"Can't find file: " << this->Filename);
    return NULL;
    }

  // get the magic number
  do
    {
    c = vtkPNMReaderGetChar(fp);
    }
  while (c != 'P');
  magic[0] = c;
  magic[1] = vtkPNMReaderGetChar(fp);
  magic[2] = '\0';

  // now get the dimensions
  dim[0] = vtkPNMReaderGetInt(fp);
  dim[1] = vtkPNMReaderGetInt(fp);

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
      graymap->Delete();
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
      pixmap->Delete();
      }
    }
  else
    {
    vtkErrorMacro(<<"Unknown file type!");
    return NULL;
    }

  return s;
}

vtkColorScalars *vtkPNMReader::ReadVolume(int dim[3])
{
  vtkColorScalars *s=NULL;

  return s;
}


int vtkPNMReader::ReadBinaryPBM(FILE *fp, vtkBitmap* bitmap, int numPts,
                               int xsize, int ysize)
{
  int max, j, packedXSize=xsize/8;
  unsigned char *cptr;

  max = vtkPNMReaderGetInt(fp);

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

int vtkPNMReader::ReadBinaryPGM(FILE *fp, vtkGraymap* graymap, int numPts,
                               int xsize, int ysize)
{
  int max, j;
  unsigned char *cptr;

  max = vtkPNMReaderGetInt(fp);
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

int vtkPNMReader::ReadBinaryPPM(FILE *fp, vtkPixmap* pixmap, int numPts,
                               int xsize, int ysize)
{
  int max, j;
  unsigned char *cptr;

  max = vtkPNMReaderGetInt(fp);
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

void vtkPNMReader::PrintSelf(ostream& os, vtkIndent indent)
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
