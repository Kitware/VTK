/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOglrTexture.cc
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
#include <math.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>

#include "vtkOglrRenderer.hh"
#include "vtkOglrTexture.hh"

// shared increasing counter
long vtkOglrTexture::GlobalIndex = 0;

// Description:
// Initializes an instance, generates a unique index.
vtkOglrTexture::vtkOglrTexture()
{
  this->GlobalIndex++;
  this->Index = this->GlobalIndex;
}

// Description:
// Implement base class method.
void vtkOglrTexture::Load(vtkTexture *txt, vtkRenderer *ren)
{
  this->Load(txt, (vtkOglrRenderer *)ren);
}

// Description:
// Actual Texture load method.
void vtkOglrTexture::Load(vtkTexture *txt, vtkOglrRenderer *vtkNotUsed(ren))
{
  GLenum format = GL_LUMINANCE;

  // need to reload the texture
  if (txt->GetInput()->GetMTime() > this->LoadTime.GetMTime() ||
      (txt->GetLookupTable () && txt->GetLookupTable()->GetMTime () >  this->LoadTime.GetMTime()))
    {
    int bytesPerPixel;
    int *size;
    vtkScalars *scalars;
    unsigned char *dataPtr;
    int rowLength;
    unsigned char *resultData;
    int xsize, ysize;
    unsigned short xs,ys;

    // get some info
    size = txt->GetInput()->GetDimensions();
    scalars = (txt->GetInput()->GetPointData())->GetScalars();

    // make sure scalars are non null
    if (!scalars) 
      {
      vtkErrorMacro(<< "No scalar values found for texture input!\n");
      return;
      }

    bytesPerPixel = scalars->GetNumberOfValuesPerScalar();

    // make sure using unsigned char data of color scalars type
    if ( strcmp(scalars->GetDataType(),"unsigned char") ||
    strcmp(scalars->GetScalarType(),"ColorScalar") )
      {
      dataPtr = txt->MapScalarsToColors (scalars);
      bytesPerPixel = 4;
      }
    else
      {
      dataPtr = ((vtkColorScalars *)scalars)->GetPtr(0);    
      }
    // we only support 2d texture maps right now
    // so one of the three sizes must be 1, but it 
    // could be any of them, so lets find it
    if (size[0] == 1)
      {
      xsize = size[1]; ysize = size[2];
      }
    else
      {
      xsize = size[0];
      if (size[1] == 1)
	{
	ysize = size[2];
	}
      else
	{
	ysize = size[1];
	if (size[2] != 1)
	  {
	  vtkErrorMacro(<< "3D texture maps currently are not supported!\n");
	  return;
	  }
	}
      }

    // xsize and ysize must be a power of 2 in OpenGL
    xs = (unsigned short)xsize;
    ys = (unsigned short)ysize;
    while (!(xs & 0x01))
      {
      xs = xs >> 1;
      }
    while (!(ys & 0x01))
      {
      ys = ys >> 1;
      }
    if ((xs > 1)||(ys > 1))
      {
      vtkWarningMacro(<< "Texture map's width and height must be a power of two in OpenGL\n");
      }

    // format the data so that it can be sent to the gl
    // each row must be a multiple of 4 bytes in length
    // the best idea is to make your size a multiple of 4
    // so that this conversion will never be done.
    rowLength = ((xsize*bytesPerPixel +3 )/4)*4;
    if (rowLength == xsize*bytesPerPixel)
      {
      resultData = dataPtr;
      }
    else
      {
      int col;
      unsigned char *src,*dest;
      int srcLength;

      srcLength = xsize*bytesPerPixel;
      resultData = new unsigned char [rowLength*ysize];
      
      src = dataPtr;
      dest = resultData;

      for (col = 0; col < ysize; col++)
	{
	memcpy(dest,src,srcLength);
	src += srcLength;
	dest += rowLength;
	}
      }

    if (txt->GetInterpolate())
      {
      glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
		       GL_LINEAR);
      glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
		       GL_LINEAR );
      }
    else
      {
      glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
      glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
      }
    if (txt->GetRepeat())
      {
      glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_REPEAT );
      glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_REPEAT );
      }
    else
      {
      glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP );
      glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP );
      }
    switch (bytesPerPixel)
      {
      case 1: format = GL_LUMINANCE; break;
      case 2: format = GL_LUMINANCE_ALPHA; break;
      case 3: format = GL_RGB; break;
      case 4: format = GL_RGBA; break;
      }
    glTexImage2D( GL_TEXTURE_2D, 0 , bytesPerPixel,
		  xsize, ysize, 0, format, 
		  GL_UNSIGNED_BYTE, (const GLvoid *)resultData );

    // modify the load time to the current time
    this->LoadTime.Modified();
    
    // free memory
    if (resultData != dataPtr)
      {
      delete [] resultData;
      }
    }

  // now bind it 
  glEnable(GL_TEXTURE_2D);
}
