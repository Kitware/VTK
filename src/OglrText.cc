/*=========================================================================

  Program:   Visualization Toolkit
  Module:    OglrText.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#include <math.h>
#include <string.h>
#include "OglrRen.hh"
#include "OglrText.hh"

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
void vtkOglrTexture::Load(vtkTexture *txt, vtkOglrRenderer *ren)
{
  GLenum format;

  // need to reload the texture
  if (txt->GetInput()->GetMTime() > this->LoadTime.GetMTime())
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
      vtkErrorMacro(<< "Cannot do quick coversion to unsigned char.\n");
      return;
      }

    dataPtr = ((vtkColorScalars *)scalars)->GetPtr(0);    

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
