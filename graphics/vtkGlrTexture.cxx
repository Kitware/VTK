/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGlrTexture.cxx
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
#include <math.h>
#include <string.h>
#include "vtkGlrRenderer.hh"
#include "vtkGlrTexture.hh"

static float texprops[] 
  = {
  TX_MINFILTER, TX_POINT,
  TX_MAGFILTER, TX_POINT,
  TX_WRAP, TX_REPEAT, TX_NULL
  };

// shared increasing counter
long vtkGlrTexture::GlobalIndex = 0;

// Description:
// Initializes an instance, generates a unique index.
vtkGlrTexture::vtkGlrTexture()
{
  this->GlobalIndex++;
  this->Index = this->GlobalIndex;
}

// Description:
// Implement base class method.
void vtkGlrTexture::Load(vtkTexture *txt, vtkRenderer *ren)
{
  this->Load(txt, (vtkGlrRenderer *)ren);
}

// Description:
// Actual Texture load method.
void vtkGlrTexture::Load(vtkTexture *txt, vtkGlrRenderer *vtkNotUsed(ren))
{
  // make sure it can handle textures
  if (!getgdesc(GD_TEXTURE)) 
    {
    vtkDebugMacro(<< "Texture mapping not supported on this machine\n");
    return;
    }
  
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
    int xsize, ysize, col, row;

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
    
    // format the data so that it can be sent to the gl
    // each row must be a multiple of 4 bytes in length
    // the best idea is to make your size a multiple of 4
    // so that this conversion will never be done.
    rowLength = ((xsize*bytesPerPixel +3 )/4)*4;
    if (rowLength == xsize*bytesPerPixel && bytesPerPixel == 1 )
      {
      resultData = dataPtr;
      }
    else //have to create our own local data
      {
      unsigned char *src,*dest;
      int srcLength;

      srcLength = xsize*bytesPerPixel;
      resultData = new unsigned char [rowLength*ysize];
      
      src = dataPtr;
      dest = resultData;

      for (row = 0; row < ysize; row++)
	{
	memcpy(dest,src,srcLength);
	src += srcLength;
	dest += rowLength;
	}
      }

    // gl orders things as abgr; vtk has reverse order; need to swap
    if ( bytesPerPixel > 1 )
      {
      int i, idx;
      unsigned char b, *b1, *b2;
      for (row=0; row < ysize; row++)
        {
        for (col=0; col < xsize; col++)
          {
          for (i=0; i<(bytesPerPixel/2); i++)
            {
            idx = row*rowLength + col*bytesPerPixel;
            b1 = resultData + idx + i;
            b2 = resultData + idx + (bytesPerPixel-i-1);
            b = *b1;
            *b1 = *b2;
            *b2 = b;
            }
          }
        }
      }

    if (txt->GetInterpolate())
      {
      texprops[1] = TX_MIPMAP_BILINEAR;
      texprops[3] = TX_BILINEAR;
      }
    else
      {
      texprops[1] = TX_POINT;
      texprops[3] = TX_POINT;
      }
    if (txt->GetRepeat())
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
