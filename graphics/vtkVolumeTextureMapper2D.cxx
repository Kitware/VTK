/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeTextureMapper2D.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkVolumeTextureMapper2D.h"
#include "vtkRenderWindow.h"
#include "vtkGraphicsFactory.h"

#define VTK_PLUS_X_MAJOR_DIRECTION  0
#define VTK_MINUS_X_MAJOR_DIRECTION 1
#define VTK_PLUS_Y_MAJOR_DIRECTION  2
#define VTK_MINUS_Y_MAJOR_DIRECTION 3
#define VTK_PLUS_Z_MAJOR_DIRECTION  4
#define VTK_MINUS_Z_MAJOR_DIRECTION 5

template <class T>
static void 
VolumeTextureMapper2D_XMajorDirection( T *data_ptr,
				       int size[3],
				       unsigned char *texture,
				       int tsize[2],
				       int directionFlag,
				       vtkVolumeTextureMapper2D *me )
{
  int              i, j, k;
  int              istart, iend, iinc;
  unsigned char    *tptr;
  T                *dptr;
  unsigned short   *nptr;
  unsigned char    *gptr;
  float            v[12], t[8];
  unsigned char    *rgbaArray = me->GetRGBAArray();
  float            *gradientOpacityArray;
  unsigned char    *gradientMagnitudes;
  unsigned short   *encodedNormals;
  float            *redDiffuseShadingTable;
  float            *greenDiffuseShadingTable;
  float            *blueDiffuseShadingTable;
  float            *redSpecularShadingTable;
  float            *greenSpecularShadingTable;
  float            *blueSpecularShadingTable;
  int              shade;
  float            tmpval;
  int              cropping, croppingFlags;
  float            *croppingBounds;
  int              flag[3], tmpFlag, index;
  int              clipLow, clipHigh;
  vtkRenderWindow  *renWin = me->GetRenderWindow();
  float            spacing[3], origin[3];
  unsigned char    zero[4];

  zero[0] = 0;
  zero[1] = 0;
  zero[2] = 0;
  zero[3] = 0;

  me->GetDataSpacing( spacing );
  me->GetDataOrigin( origin );

  if ( directionFlag )
    {
    istart  = 0;
    iend    = size[0];
    iinc    = 1;
    }
  else 
    {
    istart  = size[0] - 1;
    iend    = -1;
    iinc    = -1;
    }

  float offset[2];
  offset[0] = 0.5 / (float)tsize[0];
  offset[1] = 0.5 / (float)tsize[1];
  t[0] = offset[0];
  t[1] = offset[1];
  t[2] = offset[0];
  t[3] = ((float)size[2] / (float)tsize[1]) - offset[1];
  t[4] = ((float)size[1] / (float)tsize[0]) - offset[0];
  t[5] = ((float)size[2] / (float)tsize[1]) - offset[1];
  t[6] = ((float)size[1] / (float)tsize[0]) - offset[0];
  t[7] = offset[1];

  v[1] = origin[1];
  v[2] = origin[2];

  v[4] = origin[1];
  v[5] = spacing[2] * size[2] + origin[2];

  v[7] = spacing[1] * size[1] + origin[1];
  v[8] = spacing[2] * size[2] + origin[2];

  v[10] = spacing[1] * size[1] + origin[1];
  v[11] = origin[2];

  cropping       = me->GetCropping();
  croppingFlags  = me->GetCroppingRegionFlags();
  croppingBounds = me->GetCroppingRegionPlanes();

  if ( !cropping )
    {
    clipLow    = 0;
    clipHigh   = size[1];
    flag[0]    = 1;
    flag[1]    = 1;
    flag[2]    = 1;
    }

  shade = me->GetShade();
  if ( shade )
    {
    encodedNormals = me->GetEncodedNormals();
    
    redDiffuseShadingTable    = me->GetRedDiffuseShadingTable();
    greenDiffuseShadingTable  = me->GetGreenDiffuseShadingTable();
    blueDiffuseShadingTable   = me->GetBlueDiffuseShadingTable();
    
    redSpecularShadingTable   = me->GetRedSpecularShadingTable();
    greenSpecularShadingTable = me->GetGreenSpecularShadingTable();
    blueSpecularShadingTable =  me->GetBlueSpecularShadingTable(); 
    }

  gradientMagnitudes = me->GetGradientMagnitudes();
  gradientOpacityArray = me->GetGradientOpacityArray();

  renWin = me->GetRenderWindow();

  for ( i = istart; i != iend; i+=iinc )
    {
    for ( k = 0; k < size[2]; k++ )
      {
      tptr = texture + k*4*tsize[0];
      dptr = data_ptr + k*size[0]*size[1] + i;

      // Given an X and Z value, what are the cropping bounds
      // on Y.
      if ( cropping )
	{
	clipLow  = (int) croppingBounds[2];
	clipHigh = (int) croppingBounds[3];
	tmpFlag =    (i<croppingBounds[0])?(0):(1+(i>=croppingBounds[1]));
	tmpFlag+= 9*((k<croppingBounds[4])?(0):(1+(k>=croppingBounds[5])));
	flag[0]  = croppingFlags&(1<<(tmpFlag));
	flag[1]  = croppingFlags&(1<<(tmpFlag+3));
	flag[2]  = croppingFlags&(1<<(tmpFlag+6));
	}

      if ( shade )
	{
	nptr = encodedNormals + k*size[0]*size[1] + i;
	if ( gradientMagnitudes )
	  {
	  gptr = gradientMagnitudes + k*size[0]*size[1] + i;
	  }
	for ( j = 0; j < size[1]; j++ )
	  {
	  index = 0;
	  index += ( j >= clipLow );
	  index += ( j >= clipHigh );
	  if ( flag[index] )
	    {
	    tmpval = rgbaArray[(*dptr)*4];
	    tmpval = tmpval * redDiffuseShadingTable[*nptr] +
	      redSpecularShadingTable[*nptr]*255.0;
	    if ( tmpval > 255.0 )
	      {
	      tmpval = 255.0;
	      }
	    *(tptr++) = (unsigned char) tmpval;

	    tmpval = rgbaArray[(*dptr)*4 + 1];
	    tmpval = tmpval * greenDiffuseShadingTable[*nptr] +
	      greenSpecularShadingTable[*nptr]*255.0;
	    if ( tmpval > 255.0 )
	      {
	      tmpval = 255.0;
	      }
	    *(tptr++) = (unsigned char) tmpval;
	    
	    tmpval = rgbaArray[(*dptr)*4 + 2];
	    tmpval = tmpval * blueDiffuseShadingTable[*nptr] +
	      blueSpecularShadingTable[*nptr]*255.0;
	    if ( tmpval > 255.0 )
	      {
	      tmpval = 255.0;
	      }
	    *(tptr++) = (unsigned char) tmpval;
	    
	    tmpval = rgbaArray[(*dptr)*4 + 3];
	    if ( gradientMagnitudes )
	      {
	      tmpval *= gradientOpacityArray[*gptr];
	      gptr += size[0];
	      }
	    *(tptr++) = (unsigned char) tmpval;
	    }
	  else
	    {
	    memcpy( tptr, zero, 4 );
	    tptr += 4;
	    if ( gradientMagnitudes )
	      {
	      gptr += size[0];
	      }
	    }
	  dptr += size[0];
	  nptr += size[0];
	  }
	}
      else
	{
	if ( gradientMagnitudes )
	  {
	  gptr = gradientMagnitudes + k*size[0]*size[1] + i;
	  }

	if ( cropping )
	  {
	  for ( j = 0; j < size[1]; j++ )
	    {
	    index = 0;
	    index += ( j >= clipLow );
	    index += ( j >= clipHigh );
	    if ( flag[index] )
	      {
	      memcpy( tptr, rgbaArray + (*dptr)*4, 4 );
	      if ( gradientMagnitudes )
		{
		*(tptr+3) = (unsigned char)
		  ((float)(*(tptr+3)) * gradientOpacityArray[*gptr]);
		gptr += size[0];
		}
	      }	  
	    else
	      {
	      memcpy( tptr, zero, 4 );
	      if ( gradientMagnitudes )
		{
		gptr += size[0];
		}	      
	      }
	    tptr += 4;
	    dptr += size[0];
	    }
	  }
	else
	  {
	  if ( gradientMagnitudes )
	    {
	    for ( j = 0; j < size[1]; j++ )
	      {
	      memcpy( tptr, rgbaArray + (*dptr)*4, 4 );
	      *(tptr+3) = (unsigned char)
		((float)(*(tptr+3)) * gradientOpacityArray[*gptr]);
	      gptr += size[0];
	      tptr += 4;
	      dptr += size[0];
	      }
	    }
	  else
	    {
	    for ( j = 0; j < size[1]; j++ )
	      {
	      memcpy( tptr, rgbaArray + (*dptr)*4, 4 );
	      tptr += 4;
	      dptr += size[0];
	      }
	    }
	  }
	}
      }

    if ( renWin->CheckAbortStatus() )
      {
      return;
      }

    v[0] = v[3] = v[6] = v[9] = (float)i * spacing[0] + origin[0];
    me->RenderRectangle( v, t, texture, tsize);
    }
}

template <class T>
static void 
VolumeTextureMapper2D_YMajorDirection( T *data_ptr,
				       int size[3],
				       unsigned char *texture,
				       int tsize[2],
				       int directionFlag,
				       vtkVolumeTextureMapper2D *me )
{
  int            i, j, k;
  int            jstart, jend, jinc;
  unsigned char  *tptr;
  T              *dptr;
  unsigned short *nptr;
  unsigned char  *gptr;
  float          v[12], t[8];
  unsigned char  *rgbaArray = me->GetRGBAArray();
  unsigned short *encodedNormals;
  float          *gradientOpacityArray;
  unsigned char  *gradientMagnitudes;
  float          *redDiffuseShadingTable;
  float          *greenDiffuseShadingTable;
  float          *blueDiffuseShadingTable;
  float          *redSpecularShadingTable;
  float          *greenSpecularShadingTable;
  float          *blueSpecularShadingTable;
  int            shade;
  float          tmpval;
  int            cropping, croppingFlags;
  float          *croppingBounds;
  int            flag[3], tmpFlag, index;
  int            clipLow, clipHigh;
  vtkRenderWindow  *renWin = me->GetRenderWindow();
  float            spacing[3], origin[3];
  unsigned char    zero[4];

  zero[0] = 0;
  zero[1] = 0;
  zero[2] = 0;
  zero[3] = 0;

  me->GetDataSpacing( spacing );
  me->GetDataOrigin( origin );

  if ( directionFlag )
    {
    jstart  = 0;
    jend    = size[1];
    jinc    = 1;
    }
  else 
    {
    jstart  = size[1] - 1;
    jend    = -1;
    jinc    = -1;
    }

  float offset[2];
  offset[0] = 0.5 / (float)tsize[0];
  offset[1] = 0.5 / (float)tsize[1];

  t[0] = offset[0];
  t[1] = offset[1];
  t[2] = ((float)size[0] / (float)tsize[0]) - offset[0];
  t[3] = offset[1];
  t[4] = ((float)size[0] / (float)tsize[0]) - offset[0];
  t[5] = ((float)size[2] / (float)tsize[1]) - offset[1];
  t[6] = offset[0];
  t[7] = ((float)size[2] / (float)tsize[1]) - offset[1];

  v[0] = origin[0];
  v[2] = origin[2];

  v[3] = spacing[0] * size[0] + origin[0];
  v[5] = origin[2];

  v[6] = spacing[0] * size[0] + origin[0];
  v[8] = spacing[2] * size[2] + origin[2];

  v[9] = origin[0];
  v[11] = spacing[2] * size[2] + origin[2];

  cropping       = me->GetCropping();
  croppingFlags  = me->GetCroppingRegionFlags();
  croppingBounds = me->GetCroppingRegionPlanes();

  if ( !cropping )
    {
    clipLow    = 0;
    clipHigh   = size[0];
    flag[0]    = 1;
    flag[1]    = 1;
    flag[2]    = 1;
    }

  shade = me->GetShade();
  if ( shade )
    {
    encodedNormals = me->GetEncodedNormals();
    
    redDiffuseShadingTable    = me->GetRedDiffuseShadingTable();
    greenDiffuseShadingTable  = me->GetGreenDiffuseShadingTable();
    blueDiffuseShadingTable   = me->GetBlueDiffuseShadingTable();
    
    redSpecularShadingTable   = me->GetRedSpecularShadingTable();
    greenSpecularShadingTable = me->GetGreenSpecularShadingTable();
    blueSpecularShadingTable =  me->GetBlueSpecularShadingTable(); 
    }

  gradientMagnitudes = me->GetGradientMagnitudes();
  gradientOpacityArray = me->GetGradientOpacityArray();
  
  for ( j = jstart; j != jend; j+=jinc )
    {
    for ( k = 0; k < size[2]; k++ )
      {
      tptr = texture + k*4*tsize[0];
      dptr = data_ptr + k*size[0]*size[1] + j*size[0];

      // Given a Y and Z value, what are the cropping bounds
      // on X.
      if ( cropping )
	{
	clipLow  = (int)croppingBounds[0];
	clipHigh = (int)croppingBounds[1];
	tmpFlag = 3*((j<croppingBounds[2])?(0):(1+(j>=croppingBounds[3])));
	tmpFlag+= 9*((k<croppingBounds[4])?(0):(1+(k>=croppingBounds[5])));
	flag[0]  = croppingFlags&(1<<(tmpFlag));
	flag[1]  = croppingFlags&(1<<(tmpFlag+1));
	flag[2]  = croppingFlags&(1<<(tmpFlag+2));
	}

      if ( shade )
	{
	nptr = encodedNormals + k*size[0]*size[1] + j*size[0];
	if ( gradientMagnitudes )
	  {
	  gptr = gradientMagnitudes + k*size[0]*size[1] + j*size[0];
	  }
	for ( i = 0; i < size[0]; i++ )
	  {
	  index = 0;
	  index += ( i >= clipLow );
	  index += ( i >= clipHigh );
	  if ( flag[index] )
	    {
	    tmpval = rgbaArray[(*dptr)*4];
	    tmpval = tmpval * redDiffuseShadingTable[*nptr] +
	      redSpecularShadingTable[*nptr]*255.0;
	    if ( tmpval > 255.0 )
	      {
	      tmpval = 255.0;
	      }
	    *(tptr++) = (unsigned char) tmpval;

	    tmpval = rgbaArray[(*dptr)*4 + 1];
	    tmpval = tmpval * greenDiffuseShadingTable[*nptr] +
	      greenSpecularShadingTable[*nptr]*255.0;
	    if ( tmpval > 255.0 )
	      {
	      tmpval = 255.0;
	      }
	    *(tptr++) = (unsigned char) tmpval;

	    tmpval = rgbaArray[(*dptr)*4 + 2];
	    tmpval = tmpval * blueDiffuseShadingTable[*nptr] +
	      blueSpecularShadingTable[*nptr]*255.0;
	    if ( tmpval > 255.0 )
	      {
	      tmpval = 255.0;
	      }
	    *(tptr++) = (unsigned char) tmpval;
	    
	    tmpval = rgbaArray[(*dptr)*4 + 3];
	    if ( gradientMagnitudes )
	      {
	      tmpval *= gradientOpacityArray[*gptr];
	      gptr++;
	      }
	    *(tptr++) = (unsigned char) tmpval;
	    }
	  else
	    {
	    memcpy( tptr, zero, 4 );
	    tptr += 4;
	    if ( gradientMagnitudes )
	      {
	      gptr++;
	      }
	    }
	  dptr++;
	  nptr++;
	  }
	}
      else
	{
	if ( gradientMagnitudes )
	  {
	  gptr = gradientMagnitudes + k*size[0]*size[1] + j*size[0];
	  }

	if ( cropping )
	  {
	  for ( i = 0; i < size[0]; i++ )
	    {
	    index = 0;
	    index += ( i >= clipLow );
	    index += ( i >= clipHigh );
	    if ( flag[index] )
	      {
	      memcpy( tptr, rgbaArray + (*dptr)*4, 4 );
	      if ( gradientMagnitudes )
		{
		*(tptr+3) = (unsigned char)
		  ((float)(*(tptr+3)) * gradientOpacityArray[*gptr]);
		gptr++;
		}
	      }
	    else
	      {
	      memcpy( tptr, zero, 4 );
	      if ( gradientMagnitudes )
		{
		gptr++;
		}	      
	      }
	    tptr += 4;
	    dptr++;
	    }
	  }
	else
	  {
	  if ( gradientMagnitudes )
	    {
	    for ( i = 0; i < size[0]; i++ )
	      {
	      memcpy( tptr, rgbaArray + (*dptr)*4, 4 );
	      *(tptr+3) = (unsigned char)
		((float)(*(tptr+3)) * gradientOpacityArray[*gptr]);
	      gptr++;
	      tptr += 4;
	      dptr++;
	      }
	    }
	  else
	    {
	    for ( i = 0; i < size[0]; i++ )
	      {
	      memcpy( tptr, rgbaArray + (*dptr)*4, 4 );
	      tptr += 4;
	      dptr++;
	      }
	    }
	  }
	}
      }
    if ( renWin->CheckAbortStatus() )
      {
      return;
      }

    v[1] = v[4] = v[7] = v[10] = spacing[1] * (float)j + origin[1];
    me->RenderRectangle( v, t, texture, tsize);
    }
}

template <class T>
static void 
VolumeTextureMapper2D_ZMajorDirection( T *data_ptr,
				       int size[3],
				       unsigned char *texture,
				       int tsize[2],
				       int directionFlag,
				       vtkVolumeTextureMapper2D *me )
{
  int            i, j, k;
  int            kstart, kend, kinc;
  unsigned char  *tptr;
  T              *dptr;
  unsigned short *nptr;
  unsigned char  *gptr;
  float          v[12], t[8];
  unsigned char  *rgbaArray = me->GetRGBAArray();
  unsigned short *encodedNormals;
  float          *gradientOpacityArray;
  unsigned char  *gradientMagnitudes;
  float          *redDiffuseShadingTable;
  float          *greenDiffuseShadingTable;
  float          *blueDiffuseShadingTable;
  float          *redSpecularShadingTable;
  float          *greenSpecularShadingTable;
  float          *blueSpecularShadingTable;
  int            shade;
  float          tmpval;
  int            cropping, croppingFlags;
  float          *croppingBounds;
  int            flag[3], tmpFlag, index;
  int            clipLow, clipHigh;
  vtkRenderWindow  *renWin = me->GetRenderWindow();
  float            spacing[3], origin[3];
  unsigned char    zero[4];

  zero[0] = 0;
  zero[1] = 0;
  zero[2] = 0;
  zero[3] = 0;

  me->GetDataSpacing( spacing );
  me->GetDataOrigin( origin );

  if ( directionFlag )
    {
    kstart  = 0;
    kend    = size[2];
    kinc    = 1;
    }
  else 
    {
    kstart  = size[2] - 1;
    kend    = -1;
    kinc    = -1;
    }

  float offset[2];
  offset[0] = 0.5 / (float)tsize[0];
  offset[1] = 0.5 / (float)tsize[1];

  t[0] = offset[0];
  t[1] = offset[1];
  t[2] = ((float)size[0] / (float)tsize[0]) - offset[0];
  t[3] = offset[1];
  t[4] = ((float)size[0] / (float)tsize[0]) - offset[0];
  t[5] = ((float)size[1] / (float)tsize[1]) - offset[1];
  t[6] = offset[0];
  t[7] = ((float)size[1] / (float)tsize[1]) - offset[1];

  v[0] = origin[0];
  v[1] = origin[1];

  v[3] = spacing[0] * size[0] + origin[0];
  v[4] = origin[1];

  v[6] = spacing[0] * size[0] + origin[0];
  v[7] = spacing[1] * size[1] + origin[1];

  v[9] = origin[0];
  v[10] = spacing[1] * size[1] + origin[1];

  cropping       = me->GetCropping();
  croppingFlags  = me->GetCroppingRegionFlags();
  croppingBounds = me->GetCroppingRegionPlanes();

  if ( !cropping )
    {
    clipLow    = 0;
    clipHigh   = size[0];
    flag[0]    = 1;
    flag[1]    = 1;
    flag[2]    = 1;
    }

  shade = me->GetShade();
  if ( shade )
    {
    encodedNormals = me->GetEncodedNormals();
    
    redDiffuseShadingTable    = me->GetRedDiffuseShadingTable();
    greenDiffuseShadingTable  = me->GetGreenDiffuseShadingTable();
    blueDiffuseShadingTable   = me->GetBlueDiffuseShadingTable();
    
    redSpecularShadingTable   = me->GetRedSpecularShadingTable();
    greenSpecularShadingTable = me->GetGreenSpecularShadingTable();
    blueSpecularShadingTable =  me->GetBlueSpecularShadingTable(); 
    }

  gradientMagnitudes = me->GetGradientMagnitudes();
  gradientOpacityArray = me->GetGradientOpacityArray();

  for ( k = kstart; k != kend; k+=kinc )
    {
    for ( j = 0; j < size[1]; j++ )
      {
      tptr = texture + j*4*tsize[0];
      dptr = data_ptr + k*size[0]*size[1] + j*size[0];

      // Given a Y and Z value, what are the cropping bounds
      // on X.
      if ( cropping )
	{
	clipLow  = (int)croppingBounds[0];
	clipHigh = (int)croppingBounds[1];
	tmpFlag = 3*((j<croppingBounds[2])?(0):(1+(j>=croppingBounds[3])));
	tmpFlag+= 9*((k<croppingBounds[4])?(0):(1+(k>=croppingBounds[5])));
	flag[0]  = croppingFlags&(1<<(tmpFlag));
	flag[1]  = croppingFlags&(1<<(tmpFlag+1));
	flag[2]  = croppingFlags&(1<<(tmpFlag+2));
	}

      if ( shade )
	{
	nptr = encodedNormals + k*size[0]*size[1] + j*size[0];
	if ( gradientMagnitudes )
	  {
	  gptr = gradientMagnitudes + k*size[0]*size[1] + j*size[0];
	  }
	for ( i = 0; i < size[0]; i++ )
	  {
	  index = 0;
	  index += ( i >= clipLow );
	  index += ( i >= clipHigh );
	  if ( flag[index] )
	    {
	    tmpval = rgbaArray[(*dptr)*4];
	    tmpval = tmpval * redDiffuseShadingTable[*nptr] +
	      redSpecularShadingTable[*nptr]*255.0;
	    if ( tmpval > 255.0 )
	      {
	      tmpval = 255.0;
	      }
	    *(tptr++) = (unsigned char) tmpval;
	    
	    tmpval = rgbaArray[(*dptr)*4 + 1];
	    tmpval = tmpval * greenDiffuseShadingTable[*nptr] +
	      greenSpecularShadingTable[*nptr]*255.0;
	    if ( tmpval > 255.0 )
	      {
	      tmpval = 255.0;
	      }
	    *(tptr++) = (unsigned char) tmpval;
	    
	    tmpval = rgbaArray[(*dptr)*4 + 2];
	    tmpval = tmpval * blueDiffuseShadingTable[*nptr] +
	      blueSpecularShadingTable[*nptr]*255.0;
	    if ( tmpval > 255.0 )
	      {
	      tmpval = 255.0;
	      }
	    *(tptr++) = (unsigned char) tmpval;
	    
	    tmpval = rgbaArray[(*dptr)*4 + 3];
	    if ( gradientMagnitudes )
	      {
	      tmpval *= gradientOpacityArray[*gptr];
	      gptr++;
	      }
	    *(tptr++) = (unsigned char) tmpval;
	    }
	  else
	    {
	    memcpy( tptr, zero, 4 );
	    tptr += 4;
	    if ( gradientMagnitudes )
	      {
	      gptr++;
	      }
	    }
	  nptr++;
	  dptr++;
	  }
	}
      else
	{
	if ( gradientMagnitudes )
	  {
	  gptr = gradientMagnitudes + k*size[0]*size[1] + j*size[0];
	  }

	if ( cropping )
	  {
	  for ( i = 0; i < size[0]; i++ )
	    {
	    index = 0;
	    index += ( i >= clipLow );
	    index += ( i >= clipHigh );
	    if ( flag[index] )
	      {
	      memcpy( tptr, rgbaArray + (*dptr)*4, 4 );
	      if ( gradientMagnitudes )
		{
		*(tptr+3) = (unsigned char)
		  ((float)(*(tptr+3)) * gradientOpacityArray[*gptr]);
		gptr++;
		}
	      }
	    else
	      {
	      memcpy( tptr, zero, 4 );
	      if ( gradientMagnitudes )
		{
		gptr++;
		}	      
	      }
	    tptr += 4;
	    dptr++;
	    }
	  }
	else
	  {
	  if ( gradientMagnitudes )
	    {
	    for ( i = 0; i < size[0]; i++ )
	      {
	      memcpy( tptr, rgbaArray + (*dptr)*4, 4 );
	      *(tptr+3) = (unsigned char)
		((float)(*(tptr+3)) * gradientOpacityArray[*gptr]);
	      gptr++;
	      tptr += 4;
	      dptr++;
	      }
	    }
	  else
	    {
	    for ( i = 0; i < size[0]; i++ )
	      {
	      memcpy( tptr, rgbaArray + (*dptr)*4, 4 );
	      tptr += 4;
	      dptr++;
	      }
	    }
	  }
	}
      }

    if ( renWin->CheckAbortStatus() )
      {
      return;
      }

    v[2] = v[5] = v[8] = v[11] = spacing[2] * (float)k + origin[2];
    me->RenderRectangle( v, t, texture, tsize);
    }
}

vtkVolumeTextureMapper2D::vtkVolumeTextureMapper2D()
{
}

vtkVolumeTextureMapper2D::~vtkVolumeTextureMapper2D()
{
}


vtkVolumeTextureMapper2D *vtkVolumeTextureMapper2D::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = 
    vtkGraphicsFactory::CreateInstance("vtkVolumeTextureMapper2D");
  return (vtkVolumeTextureMapper2D*)ret;
}


void vtkVolumeTextureMapper2D::GenerateTexturesAndRenderRectangles()
{
  vtkStructuredPoints    *input = this->GetInput();
  int                    size[3];
  int                    tsize[2];
  int                    targetsize[2];
  unsigned char          *texture;
  void                   *inputPointer;
  int                    inputType;

  inputPointer = 
    input->GetPointData()->GetScalars()->GetVoidPointer(0);
  inputType = 
    input->GetPointData()->GetScalars()->GetDataType();

  input->GetDimensions( size );

  switch ( this->MajorDirection )
    {
    case VTK_PLUS_X_MAJOR_DIRECTION:
    case VTK_MINUS_X_MAJOR_DIRECTION:
      targetsize[0] = size[1];
      targetsize[1] = size[2];
      break;     

    case VTK_PLUS_Y_MAJOR_DIRECTION:
    case VTK_MINUS_Y_MAJOR_DIRECTION:
      targetsize[0] = size[0];
      targetsize[1] = size[2];
      break;     

    case VTK_PLUS_Z_MAJOR_DIRECTION:
    case VTK_MINUS_Z_MAJOR_DIRECTION:
      targetsize[0] = size[0];
      targetsize[1] = size[1];
      break;     
    }

  tsize[0] = 32;
  while( tsize[0] < targetsize[0] ) 
    {
    tsize[0] *= 2;
    }
  
  tsize[1] = 32;
  while( tsize[1] < targetsize[1] )
    {
    tsize[1] *= 2;
    }

  texture = new unsigned char[4*tsize[0]*tsize[1]];

  switch ( inputType )
    {
    case VTK_UNSIGNED_CHAR:
      switch ( this->MajorDirection )
	{
	case VTK_PLUS_X_MAJOR_DIRECTION:
	  VolumeTextureMapper2D_XMajorDirection
	    ( (unsigned char *)inputPointer, size, texture, tsize, 1, this );
	  break;

	case VTK_MINUS_X_MAJOR_DIRECTION:
	  VolumeTextureMapper2D_XMajorDirection
	    ( (unsigned char *)inputPointer, size, texture, tsize, 0, this );
	  break;

	case VTK_PLUS_Y_MAJOR_DIRECTION:
	  VolumeTextureMapper2D_YMajorDirection
	    ( (unsigned char *)inputPointer, size, texture, tsize, 1, this );
	  break;

	case VTK_MINUS_Y_MAJOR_DIRECTION:
	  VolumeTextureMapper2D_YMajorDirection
	    ( (unsigned char *)inputPointer, size, texture, tsize, 0, this );
	  break;

	case VTK_PLUS_Z_MAJOR_DIRECTION:
	  VolumeTextureMapper2D_ZMajorDirection
	    ( (unsigned char *)inputPointer, size, texture, tsize, 1, this );
	  break;

	case VTK_MINUS_Z_MAJOR_DIRECTION:
	  VolumeTextureMapper2D_ZMajorDirection
	    ( (unsigned char *)inputPointer, size, texture, tsize, 0, this );
	  break;
	}
      break;
    case VTK_UNSIGNED_SHORT:
      switch ( this->MajorDirection )
	{
	case VTK_PLUS_X_MAJOR_DIRECTION:
	  VolumeTextureMapper2D_XMajorDirection
	    ( (unsigned short *)inputPointer, size, texture, tsize, 1, this );
	  break;

	case VTK_MINUS_X_MAJOR_DIRECTION:
	  VolumeTextureMapper2D_XMajorDirection
	    ( (unsigned short *)inputPointer, size, texture, tsize, 0, this );
	  break;

	case VTK_PLUS_Y_MAJOR_DIRECTION:
	  VolumeTextureMapper2D_YMajorDirection
	    ( (unsigned short *)inputPointer, size, texture, tsize, 1, this );
	  break;

	case VTK_MINUS_Y_MAJOR_DIRECTION:
	  VolumeTextureMapper2D_YMajorDirection
	    ( (unsigned short *)inputPointer, size, texture, tsize, 0, this );
	  break;

	case VTK_PLUS_Z_MAJOR_DIRECTION:
	  VolumeTextureMapper2D_ZMajorDirection
	    ( (unsigned short *)inputPointer, size, texture, tsize, 1, this );
	  break;

	case VTK_MINUS_Z_MAJOR_DIRECTION:
	  VolumeTextureMapper2D_ZMajorDirection
	    ( (unsigned short *)inputPointer, size, texture, tsize, 0, this );
	  break;
	}
      break;
    }

  delete texture;
}

void vtkVolumeTextureMapper2D::InitializeRender( vtkRenderer *ren,
						 vtkVolume *vol )
{
  float vpn[3];

  ren->GetActiveCamera()->GetViewPlaneNormal( vpn );

  // Fudge this for now - fix later to determine what major direction is
  // in the case of volume movement in perspective.
  if ( fabs(vpn[0]) >= fabs(vpn[1]) && fabs(vpn[0]) >= fabs(vpn[2]) )
    {
    this->MajorDirection = 
      (vpn[0]<0.0)?(VTK_MINUS_X_MAJOR_DIRECTION):(VTK_PLUS_X_MAJOR_DIRECTION);
    }
  else if ( fabs(vpn[1]) >= fabs(vpn[0]) && fabs(vpn[1]) >= fabs(vpn[2]) )
    {
    this->MajorDirection = 
      (vpn[1]<0.0)?(VTK_MINUS_Y_MAJOR_DIRECTION):(VTK_PLUS_Y_MAJOR_DIRECTION);
    }
  else
    {
    this->MajorDirection = 
      (vpn[2]<0.0)?(VTK_MINUS_Z_MAJOR_DIRECTION):(VTK_PLUS_Z_MAJOR_DIRECTION);
    }

  // Fudge this calculation for now - fix later to be accurate
  this->GetInput()->GetSpacing( this->DataSpacing );
  switch ( this->MajorDirection )
    {
    case VTK_PLUS_X_MAJOR_DIRECTION:
    case VTK_MINUS_X_MAJOR_DIRECTION:
      this->SampleDistance = this->DataSpacing[0];
      break;
    case VTK_PLUS_Y_MAJOR_DIRECTION:
    case VTK_MINUS_Y_MAJOR_DIRECTION:
      this->SampleDistance = this->DataSpacing[1];
      break;
    case VTK_PLUS_Z_MAJOR_DIRECTION:
    case VTK_MINUS_Z_MAJOR_DIRECTION:
      this->SampleDistance = this->DataSpacing[2];
      break;
    }

  this->vtkVolumeTextureMapper::InitializeRender( ren, vol );
}


// Print the vtkVolumeTextureMapper2D
void vtkVolumeTextureMapper2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkVolumeTextureMapper::PrintSelf(os,indent);
}
