/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPNMReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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

#include "vtkPNMReader.h"
#include <stdio.h>

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
  

void vtkPNMReader::UpdateImageInformation()
{
  int xsize, ysize, comp;
  char magic[80];
  char c;
  FILE *fp;

  // if the user has not set the extent, but has set the VOI
  // set the zaxis extent to the VOI z axis
  if (this->DataExtent[4]==0 && this->DataExtent[5] == 0 &&
      (this->DataVOI[4] || this->DataVOI[5]))
    {
    this->DataExtent[4] = this->DataVOI[4];
    this->DataExtent[5] = this->DataVOI[5];
    }

  // Allocate the space for the filename
  this->ComputeInternalFileName(this->DataExtent[4]);
  
  // get the magic number by reading in a file
  fp = fopen(this->InternalFileName,"rb");
  if (!fp)
    {
    vtkErrorMacro("Unable to open file " << this->InternalFileName);
    return;
    }

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

  fclose(fp);

  // compare magic number to determine file type
  if ( ! strcmp(magic,"P5") ) 
    {
    comp = 1;
    }
  else if ( ! strcmp(magic,"P6") ) 
    {
    comp = 3;
    }
  else
    {
    vtkErrorMacro(<<"Unknown file type! Not a binary PGM or PPM");
    return;
    }

  // if the user has set the VOI, just make sure its valid
  if (this->DataVOI[0] || this->DataVOI[1] || 
      this->DataVOI[2] || this->DataVOI[3] ||
      this->DataVOI[4] || this->DataVOI[5])
    { 
    if ((this->DataVOI[0] < 0) ||
	(this->DataVOI[1] >= xsize) ||
	(this->DataVOI[2] < 0) ||
	(this->DataVOI[3] >= ysize))
      {
      vtkWarningMacro("The requested VOI is larger than the file's (" << this->InternalFileName << ") extent ");
      this->DataVOI[0] = 0;
      this->DataVOI[1] = xsize - 1;
      this->DataVOI[2] = 0;
      this->DataVOI[3] = ysize - 1;
      }
    }

  this->DataExtent[0] = 0;
  this->DataExtent[1] = xsize - 1;
  this->DataExtent[2] = 0;
  this->DataExtent[3] = ysize - 1;
  
  this->SetDataScalarTypeToUnsignedChar();
  this->SetNumberOfScalarComponents(comp);
  
  vtkImageReader::UpdateImageInformation();
}

