/*=========================================================================

  Program:   Visualization Library
  Module:    GlrText.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#include <math.h>
#include <string.h>
#include "GlrRen.hh"
#include "GlrText.hh"

static float texprops[] 
  = {
  TX_MINFILTER, TX_POINT,
  TX_MAGFILTER, TX_POINT,
  TX_WRAP, TX_REPEAT, TX_NULL
  };

// shared increasing counter
long vlGlrTexture::GlobalIndex = 0;

// Description:
// Initializes an instance, generates a unique index.
vlGlrTexture::vlGlrTexture()
{
  this->GlobalIndex++;
  this->Index = this->GlobalIndex;
}

// Description:
// Implement base class method.
void vlGlrTexture::Load(vlRenderer *ren)
{
  this->Load((vlGlrRenderer *)ren);
}

// Description:
// Actual Texture load method.
void vlGlrTexture::Load(vlGlrRenderer *ren)
{
  // make sure it can handle textures
  if (!getgdesc(GD_TEXTURE)) 
    {
    vlDebugMacro(<< "Texture mapping not supported on this machine\n");
    return;
    }
  
  // need to reload the texture
  if (this->Input->GetMTime() > this->LoadTime.GetMTime())
    {
    int bytesPerPixel;
    int *size;
    vlScalars *scalars;
    unsigned char *dataPtr;
    int rowLength;
    unsigned char *resultData;
    int xsize, ysize;

    // get some info
    size = this->Input->GetDimensions();
    scalars = (this->Input->GetPointData())->GetScalars();

    // make sure scalars are non null
    if (!scalars) 
      {
      vlErrorMacro(<< "No scalar values found for texture input!\n");
      return;
      }

    bytesPerPixel = scalars->GetNumberOfValuesPerScalar();

    // make sure using unsigned char data of color scalars type
    if ( strcmp(scalars->GetDataType(),"char") ||
    strcmp(scalars->GetScalarType(),"ColorScalar") )
      {
      vlDebugMacro(<< "Cannot do quick coversion to unsigned char.\n");
      return;
      }

    dataPtr = ((vlColorScalars *)scalars)->GetPtr(0);    

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
	  vlErrorMacro(<< "3D texture maps currently are not supported!\n");
	  return;
	  }
	}
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

    if (this->Interpolate)
      {
      texprops[1] = TX_MIPMAP_BILINEAR;
      texprops[3] = TX_BILINEAR;
      }
    else
      {
      texprops[1] = TX_POINT;
      texprops[3] = TX_POINT;
      }
    if (this->Repeat)
      {
      texprops[5] = TX_REPEAT;
      }
    else
      {
      texprops[5] = TX_CLAMP;
      }
    
    texdef2d(this->Index,bytesPerPixel,xsize,ysize,
	     (unsigned long *)resultData,0,texprops);

    // modify the load time to the current time
    this->LoadTime.Modified();
    
    // free memory
    if (resultData != dataPtr)
      {
      delete [] resultData;
      }
    }

  // now bind it 
  texbind(TX_TEXTURE_0,this->Index);
}
