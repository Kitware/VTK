/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPNMReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


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
#include "vtkPNMReader.hh"

//tags used to comminicate types in system
#define VTK_UNKNOWN_TYPE 0
#define VTK_PBM_TYPE 1
#define VTK_PGM_TYPE 2
#define VTK_PPM_TYPE 3

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
  

  if (this->FilePrefix)
    {
    this->Filename = strdup(this->FilePrefix);
    }
  if ( this->Filename == NULL )
    {
    vtkErrorMacro(<<"Please specify a filename!");
    return;
    }

  if ( this->ImageRange[0] < 0 )
    {
    dim[2] = 0;
    newScalars = this->ReadImage(dim);
    dim[2] = 1;
    }
  else
    {
    newScalars = this->ReadVolume(dim);
    }

  if ( ! newScalars ) return;

  output->SetDimensions(dim);
  output->SetAspectRatio(this->DataAspectRatio);
  output->SetOrigin(this->DataOrigin);
  output->GetPointData()->SetScalars(newScalars);
  newScalars->Delete();
}

vtkStructuredPoints *vtkPNMReader::GetImage(int ImageNum)
{
  vtkColorScalars *newScalars;
  vtkStructuredPoints *result = new vtkStructuredPoints();
  int dim[3];

  if (this->FilePrefix)
    {
    this->Filename = strdup(this->FilePrefix);
    }
  if ( this->Filename == NULL )
    {
    vtkErrorMacro(<<"Please specify a filename!");
    return NULL;
    }

  dim[2] = ImageNum;
  newScalars = this->ReadImage(dim);
  if ( ! newScalars ) return NULL;

  dim[2] = 1;
  result->SetDimensions(dim);
  result->SetAspectRatio(this->DataAspectRatio);
  result->SetOrigin(this->DataOrigin);
  result->GetPointData()->SetScalars(newScalars);
  newScalars->Delete();
  return result;
}

vtkColorScalars *vtkPNMReader::ReadImage(int dim[3])
{
  int type;
  char filename[1024];
  FILE *fp;

  if (dim[2] > 0)
    {
    sprintf (filename, this->FilePattern, this->Filename, dim[2]);
    }
  else
    {
    sprintf (filename, "%s",this->Filename);
    }

  if ( !(fp = fopen(filename,"rb")) )
    {
      vtkErrorMacro(<<"Can't open file: " << filename);
      return NULL;
    }                                                                          

  type = VTK_UNKNOWN_TYPE;
  return this->ReadBinaryPNM(fp, NULL, type, 0, dim[0], dim[1]);
}

vtkColorScalars *vtkPNMReader::ReadVolume(int dim[3])
{
  vtkColorScalars *s;
  int size, imageSize, imageNum, type;
  int numImages=this->ImageRange[1] - this->ImageRange[0] + 1;
  char filename[1024];
  FILE *fp;

  sprintf (filename, this->FilePattern, this->Filename, this->ImageRange[0]);

  if ( !(fp = fopen(filename,"rb")) )
    {
    vtkErrorMacro(<<"Can't open file: " << filename);
    return NULL;
    }

  //read the first image to initialize reading the volume
  type = VTK_UNKNOWN_TYPE;
  if ( !(s=this->ReadBinaryPNM(fp, NULL, type, 0, dim[0], dim[1])))
    {
    return NULL;
    }

  imageSize = dim[0]*dim[1];
  if ( (size = imageSize*numImages) < 1 )
    {
    vtkErrorMacro(<<"Bad volume dimensions, cannot read data");
    return NULL;
    }

  //loop over remaining images; read them; assemble into volume
  for (imageNum=1; imageNum < numImages; imageNum++)
    {
    sprintf (filename, this->FilePattern, this->Filename, this->ImageRange[0]+imageNum);
    if ( !(fp = fopen(filename,"rb")) ||
    ! this->ReadBinaryPNM(fp, s, type, imageNum*imageSize, dim[0], dim[1]) )
      {
      vtkErrorMacro(<<"Can't read file: " << filename);
      s->Delete();
      return NULL;
      }
    }

  dim[2] = numImages;
  return s;
}


vtkColorScalars *vtkPNMReader::ReadBinaryPNM(FILE *fp, vtkColorScalars *s,
                              int &type, int offset, int  &xsize, int &ysize)
{
  char magic[80];
  vtkPixmap *pixmap;
  vtkGraymap *graymap;
  vtkBitmap *bitmap;
  int numPts, thisType;
  char c;
  
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
  xsize = vtkPNMReaderGetInt(fp);
  ysize = vtkPNMReaderGetInt(fp);

  // check input
  if ( (numPts = xsize*ysize) < 1 )
    {
    vtkErrorMacro(<<"Bad input data!");
    fclose(fp);
    return NULL;
    }

  // compare magic number to determine file type
  if ( ! strcmp(magic,"P4") ) 
    {
    thisType = VTK_PBM_TYPE;
    }
  else if ( ! strcmp(magic,"P5") ) 
    {
    thisType = VTK_PGM_TYPE;
    }
  else if ( ! strcmp(magic,"P6") ) 
    {
    thisType = VTK_PPM_TYPE;
    }
  else
    {
    vtkErrorMacro(<<"Unknown file type!");
    fclose(fp);
    return NULL;
    }

  //if reading multiple files (for volume), make sure each file is consistent
  if ( type == VTK_UNKNOWN_TYPE )
    {
    type = thisType;
    }
  else if ( thisType != type )
    {
    vtkErrorMacro(<<"Incompatible file types");
    fclose(fp);
    return NULL;
    }

  //finally, read file with appropriate color scalar type
  if ( type == VTK_PBM_TYPE )
    {
    if ( !s ) bitmap = new vtkBitmap(numPts);
    else bitmap = (vtkBitmap *)s;

    if ( this->ReadBinaryPBM(fp,bitmap,offset,xsize,ysize) )
      {
      s = (vtkColorScalars *)bitmap;
      }
    else 
      {
      if ( !s ) bitmap->Delete();
      s = NULL;
      }
    }

  else if ( type == VTK_PGM_TYPE )
    {
    if ( !s ) graymap = new vtkGraymap(numPts);
    else graymap = (vtkGraymap *)s;

    if ( this->ReadBinaryPGM(fp,graymap,offset,xsize,ysize) )
      {
      s = (vtkColorScalars *)graymap;
      }
    else 
      {
      if ( !s ) graymap->Delete();
      s = NULL;
      }
    }

  else 
    {
    if ( !s ) pixmap = new vtkPixmap(numPts);
    else pixmap = (vtkPixmap *)s;

    if ( this->ReadBinaryPPM(fp,pixmap,offset,xsize,ysize) )
      {
      s = (vtkColorScalars *)pixmap;
      }
    else 
      {
      if ( !s ) pixmap->Delete();
      s = NULL;
      }
    }

  fclose(fp);
  return s;
}

int vtkPNMReader::ReadBinaryPBM(FILE *fp, vtkBitmap* bitmap, int offset,
                                int xsize, int ysize)
{
  int max, j, packedXSize=xsize/8;
  unsigned char *cptr;

  max = vtkPNMReaderGetInt(fp);
  cptr = bitmap->WritePtr(offset,ysize*packedXSize);
  cptr = cptr + packedXSize*(ysize - 1);

//
// Since pnm coordinate system is at upper left of image, need to convert
// to lower rh corner origin by reading a row at a time.
//
  for (j=0; j<ysize; j++, cptr = cptr - packedXSize)
    {
    if ( ! fread(cptr,1,packedXSize,fp) )
      {
      vtkErrorMacro(<<"Error reaading raw pbm data!");
      return 0;
      }
    }

  return 1;
}

int vtkPNMReader::ReadBinaryPGM(FILE *fp, vtkGraymap* graymap, int offset,
                                int xsize, int ysize)
{
  int max, j;
  unsigned char *cptr;

  max = vtkPNMReaderGetInt(fp);
  cptr = graymap->WritePtr(offset,ysize*xsize);
  cptr = cptr + xsize*(ysize - 1);
//
// Since pnm coordinate system is at upper left of image, need to convert
// to lower rh corner origin by reading a row at a time.
//
  for (j=0; j<ysize; j++, cptr = cptr - xsize)
    {
    if ( ! fread(cptr,1,xsize,fp) )
      {
      vtkErrorMacro(<<"Error reaading raw pgm data!");
      return 0;
      }
    }

  return 1;
}

int vtkPNMReader::ReadBinaryPPM(FILE *fp, vtkPixmap* pixmap, int offset,
                                int xsize, int ysize)
{
  int max, j;
  unsigned char *cptr;

  max = vtkPNMReaderGetInt(fp);
  cptr = pixmap->WritePtr(offset,ysize*xsize);
  cptr = cptr + 3*xsize*(ysize - 1);
  
  //
  // Since pnm coordinate system is at upper left of image, need to convert
  // to lower rh corner origin by reading a row at a time.
  //
  for (j=0; j<ysize; j++, cptr = cptr - 3*xsize)
    {
    if ( ! fread(cptr,1,3*xsize,fp) )
      {
      vtkErrorMacro(<<"Error reaading raw ppm data!");
      return 0;
      }
    }

  return 1;
}

void vtkPNMReader::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkVolumeReader::PrintSelf(os,indent);

  os << indent << "Filename: " 
     << (this->Filename ? this->Filename : "(none)") << "\n";
}
