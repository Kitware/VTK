/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPNMReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "vtkPNMReader.h"
#include <stdio.h>
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkPNMReader* vtkPNMReader::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPNMReader");
  if(ret)
    {
    return (vtkPNMReader*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPNMReader;
}




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

  // put the CR/LF or whitespace back.....
  ungetc(c, fp);
  return result;
}
  

void vtkPNMReader::ExecuteInformation()
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

  if (!this->FileName && !this->FilePattern)
    {
    vtkErrorMacro(<<"Either a FileName or FilePattern must be specified.");
    return;
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

  // read max pixel value into comp for now
  comp = vtkPNMReaderGetInt(fp);
  // if file is ascii, any amount of whitespace may follow.
  // if file is binary, a single whitespace character will follow.
  // We only support binary ppm and pgm files right now.  So the next
  // character IS always ignored.
  c = getc(fp);

  // if this file was "written" on the PC, then a CR will have been
  // written as a CR/LF combination.  So, if this single whitespace
  // character is a CR and it is followed by a LF, then swallow the
  // linefeed character as well. (Not part of the PPM standard, but a
  // a hard fact of life. 
  if ( c == 0x0d )
     {
     c = getc(fp);
     if ( c != 0x0a )
        {
	ungetc( c, fp );
        }
     }
     
  // Set the header size now that we have parsed it
  this->SetHeaderSize(ftell(fp));

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

  this->vtkImageReader::ExecuteInformation();
}


