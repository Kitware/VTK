/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeTextureMapper2D.cxx
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
				       int directionFlag,
				       vtkVolumeTextureMapper2D *me )
{
  int              i, j, k;
  int              istart, iend, iinc;
  unsigned char    *tptr;
  T                *dptr;
  unsigned short   *nptr;
  unsigned char    *gptr = NULL;
  float            *v, *t;
  unsigned char    *rgbaArray = me->GetRGBAArray();
  float            *gradientOpacityArray;
  unsigned char    *gradientMagnitudes;
  unsigned short   *encodedNormals = NULL;
  float            *redDiffuseShadingTable = NULL;
  float            *greenDiffuseShadingTable = NULL;
  float            *blueDiffuseShadingTable = NULL;
  float            *redSpecularShadingTable = NULL;
  float            *greenSpecularShadingTable = NULL;
  float            *blueSpecularShadingTable = NULL;
  int              shade;
  float            tmpval;
  int              cropping, croppingFlags;
  float            *croppingBounds;
  int              flag[3], tmpFlag, index;
  int              clipLow = 0, clipHigh = 0;
  vtkRenderWindow  *renWin = me->GetRenderWindow();
  float            spacing[3], origin[3];
  unsigned char    zero[4];
  unsigned char    *texture;
  int              targetSize[2];
  int              textureSize[2];
  int              xTile, yTile, xTotal, yTotal, tile, numTiles;
  
  
  // How big should the texture be?
  // Start with the target size
  me->GetTargetTextureSize( targetSize );
  
  // Increase the x dimension of the texture if the y dimension of the data
  // is bigger than it (because these are y by z textures)
  if ( size[1] > targetSize[0] )
    {
    targetSize[0] = size[1];
    }

  // Increase the y dimension of the texture if the z dimension of the data
  // is bigger than it (because these are y by z textures)
  if ( size[2] > targetSize[1] )
    {
    targetSize[1] = size[2];
    }

  // Make sure the x dimension of the texture is a power of 2
  textureSize[0] = 32;
  while( textureSize[0] < targetSize[0] ) 
    {
    textureSize[0] *= 2;
    }
  
  // Make sure the y dimension of the texture is a power of 2
  textureSize[1] = 32;
  while( textureSize[1] < targetSize[1] )
    {
    textureSize[1] *= 2;
    }

  // Our texture might be too big - shrink it carefully making
  // sure that it is still big enough in the right dimensions to
  // handle oddly shaped volumes
  int volSize = size[0]*size[1]*size[2];
  int done = volSize > textureSize[0]*textureSize[1];
  int minSize[2];
  // What is the minumum size the texture could be in X (along the Y
  // axis of the volume)?
  minSize[0] = 32;
  while ( minSize[0] < size[1] )
    {
    minSize[0] *= 2;
    }
  // What is the minumum size the texture could be in Y (along the Z
  // axis of the volume)?
  minSize[1] = 32;
  while ( minSize[1] < size[2] )
    {
    minSize[1] *= 2;
    }
  // Keep reducing the texture size until it is just big enough
  while (!done)
    {
    // Set done to 1. Reset to 0 if we make any changes.
    done = 1;

    // If the texture is bigger in some dimension that it needs to be
    // and chopping that dimension in half would still fit the whole
    // volume, then chop it in half.
    if ( textureSize[0] > minSize[0] &&
         ( ((textureSize[0]/2)/size[1]) * 
           (textureSize[1] / size[2]) >= size[0] ) )
      {
      textureSize[0] /= 2;
      done = 0;
      }
    if ( textureSize[1] > minSize[1] &&
         ( (textureSize[0] / size[1]) * 
           ((textureSize[1]/2) / size[2]) >= size[0] ) )
      {
      textureSize[1] /= 2;
      done = 0;
      }    
    }
  
  
  // Create space for the texture
  texture = new unsigned char[4*textureSize[0]*textureSize[1]];

  // How many tiles are there in X? in Y? total?
  xTotal = textureSize[0] / size[1];
  yTotal = textureSize[1] / size[2];
  numTiles = xTotal * yTotal;
  
  // Create space for the vertices and texture coordinates. You need four vertices with
  // three components each for each tile, and four texture coordinates with three
  // components each for each texture coordinate
  v = new float [12*numTiles];
  t = new float [ 8*numTiles];
  
  // Convenient for filling in the empty regions (due to clipping)
  zero[0] = 0;
  zero[1] = 0;
  zero[2] = 0;
  zero[3] = 0;

  // We need to know the spacing and origin of the data to set up the coordinates
  // correctly
  me->GetDataSpacing( spacing );
  me->GetDataOrigin( origin );

  // What is the first plane, the increment to move to the next plane, and the plane 
  // that is just past the end?
  if ( directionFlag )
    {
    istart  = 0;
    iend    = ((int)( (size[0]-1) / 
                      me->GetInternalSkipFactor())+1)*me->GetInternalSkipFactor();
    
    // Offset the slices so that if we take just one it is in the middle
    istart += (size[0]-1-iend+me->GetInternalSkipFactor())/2;
    iend   += (size[0]-1-iend+me->GetInternalSkipFactor())/2;
    
    iinc    = me->GetInternalSkipFactor();
    }
  else 
    {
    istart  = (int)((size[0]-1) / 
                    me->GetInternalSkipFactor()) * me->GetInternalSkipFactor();
    iend    = -me->GetInternalSkipFactor();

    // Offset the slices so that if we take just one it is in the middle
    iend   += (size[0]-1-istart)/2;
    istart += (size[0]-1-istart)/2;

    iinc    = -me->GetInternalSkipFactor();
    }

  // Fill in the texture coordinates and most of the vertex information in advance
  float offset[2];
  offset[0] = 0.5 / (float)textureSize[0];
  offset[1] = 0.5 / (float)textureSize[1];
  
  for ( i = 0; i < numTiles; i++ )
    {
    yTile = i / xTotal;
    xTile = i % xTotal;
    
    t[i*8 + 0] = (float)((size[1]*(xTile  ))  )/(float)textureSize[0] + offset[0];
    t[i*8 + 1] = (float)((size[2]*(yTile  ))  )/(float)textureSize[1] + offset[1];
    t[i*8 + 2] = (float)((size[1]*(xTile  ))  )/(float)textureSize[0] + offset[0];
    t[i*8 + 3] = (float)((size[2]*(yTile+1))-1)/(float)textureSize[1] - offset[1];
    t[i*8 + 4] = (float)((size[1]*(xTile+1))-1)/(float)textureSize[0] - offset[0];
    t[i*8 + 5] = (float)((size[2]*(yTile+1))-1)/(float)textureSize[1] - offset[1];
    t[i*8 + 6] = (float)((size[1]*(xTile+1))-1)/(float)textureSize[0] - offset[0];
    t[i*8 + 7] = (float)((size[2]*(yTile  ))  )/(float)textureSize[1] + offset[1];
    
    v[i*12 + 1] = origin[1];
    v[i*12 + 2] = origin[2];
    
    v[i*12 + 4] = origin[1];
    v[i*12 + 5] = spacing[2] * (float)(size[2]-1) + origin[2];
    
    v[i*12 + 7] = spacing[1] * (float)(size[1]-1) + origin[1];
    v[i*12 + 8] = spacing[2] * (float)(size[2]-1) + origin[2];
    
    v[i*12 + 10] = spacing[1] * (float)(size[1]-1) + origin[1];
    v[i*12 + 11] = origin[2];
    }
  
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

  tile = 0;
  
  for ( i = istart; i != iend; i+=iinc )
    {
    yTile = tile / xTotal;
    xTile = tile % xTotal;
    
    for ( k = 0; k < size[2]; k++ )
      {
      tptr = texture + 4 * ( yTile*size[2]*textureSize[0]+
                             k*textureSize[0] +
                             xTile*size[1] );
                         
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
      break;
      }

    v[12*tile + 0] = 
      v[12*tile + 3] = 
      v[12*tile + 6] = 
      v[12*tile + 9] = (float)i * spacing[0] + origin[0];
    
    tile++;
    
    if ( tile == numTiles  || (i+iinc == iend) )
      { 
      me->RenderQuads( tile, v, t, texture, textureSize);
      tile = 0;
      }
    
    }
  
  delete [] texture;
  delete [] v;
  delete [] t;
}

template <class T>
static void 
VolumeTextureMapper2D_YMajorDirection( T *data_ptr,
				       int size[3],
				       int directionFlag,
				       vtkVolumeTextureMapper2D *me )
{
  int            i, j, k;
  int            jstart, jend, jinc;
  unsigned char  *tptr;
  T              *dptr;
  unsigned short *nptr;
  unsigned char  *gptr = NULL;
  float          *v, *t;
  unsigned char  *rgbaArray = me->GetRGBAArray();
  unsigned short *encodedNormals = NULL;
  float          *gradientOpacityArray;
  unsigned char  *gradientMagnitudes;
  float          *redDiffuseShadingTable = NULL;
  float          *greenDiffuseShadingTable = NULL;
  float          *blueDiffuseShadingTable = NULL;
  float          *redSpecularShadingTable = NULL;
  float          *greenSpecularShadingTable = NULL;
  float          *blueSpecularShadingTable = NULL;
  int            shade;
  float          tmpval;
  int            cropping, croppingFlags;
  float          *croppingBounds;
  int            flag[3], tmpFlag, index;
  int            clipLow = 0, clipHigh = 0;
  vtkRenderWindow  *renWin = me->GetRenderWindow();
  float            spacing[3], origin[3];
  unsigned char    zero[4];
  unsigned char    *texture;
  int              targetSize[2];
  int              textureSize[2];
  int              xTile, yTile, xTotal, yTotal, tile, numTiles;

  // How big should the texture be?
  // Start with the target size
  me->GetTargetTextureSize( targetSize );
  
  // Increase the x dimension of the texture if the x dimension of the data
  // is bigger than it (because these are x by z textures)
  if ( size[0] > targetSize[0] )
    {
    targetSize[0] = size[0];
    }

  // Increase the y dimension of the texture if the z dimension of the data
  // is bigger than it (because these are x by z textures)
  if ( size[2] > targetSize[1] )
    {
    targetSize[1] = size[2];
    }

  // Make sure the x dimension of the texture is a power of 2
  textureSize[0] = 32;
  while( textureSize[0] < targetSize[0] ) 
    {
    textureSize[0] *= 2;
    }
  
  // Make sure the y dimension of the texture is a power of 2
  textureSize[1] = 32;
  while( textureSize[1] < targetSize[1] )
    {
    textureSize[1] *= 2;
    }

  // Our texture might be too big - shrink it carefully making
  // sure that it is still big enough in the right dimensions to
  // handle oddly shaped volumes
  int volSize = size[0]*size[1]*size[2];
  int done = (volSize > textureSize[0]*textureSize[1]);
  int minSize[2];
  // What is the minumum size the texture could be in X (along the X
  // axis of the volume)?
  minSize[0] = 32;
  while ( minSize[0] < size[0] )
    {
    minSize[0] *= 2;
    }
  
  // What is the minumum size the texture could be in Y (along the Z
  // axis of the volume)?
  minSize[1] = 32;
  while ( minSize[1] < size[2] )
    {
    minSize[1] *= 2;
    }

  // Keep reducing the texture size until it is just big enough
  while (!done)
    {
    // Set done to 1. Reset to 0 if we make any changes.
    done = 1;

    // If the texture is bigger in some dimension that it needs to be
    // and chopping that dimension in half would still fit the whole
    // volume, then chop it in half.
    if ( textureSize[0] > minSize[0] &&
         ( ((textureSize[0]/2) / size[0]) * 
           (textureSize[1] / size[2]) >= size[1] ) )
      {
      textureSize[0] /= 2;
      done = 0;
      }
    if ( textureSize[1] > minSize[1] &&
         ( (textureSize[0] / size[0]) * 
           ((textureSize[1]/2) / size[2]) >= size[1] ) )
      {
      textureSize[1] /= 2;
      done = 0;
      }    
    }

  // Create space for the texture
  texture = new unsigned char[4*textureSize[0]*textureSize[1]];

  // How many tiles are there in X? in Y? total?
  xTotal = textureSize[0] / size[0];
  yTotal = textureSize[1] / size[2];
  numTiles = xTotal * yTotal;
  
  // Create space for the vertices and texture coordinates. You need four vertices with
  // three components each for each tile, and four texture coordinates with three
  // components each for each texture coordinate
  v = new float [12*numTiles];
  t = new float [ 8*numTiles];
  
  // Convenient for filling in the empty regions (due to clipping)
  zero[0] = 0;
  zero[1] = 0;
  zero[2] = 0;
  zero[3] = 0;

  // We need to know the spacing and origin of the data to set up the coordinates
  // correctly
  me->GetDataSpacing( spacing );
  me->GetDataOrigin( origin );

  // What is the first plane, the increment to move to the next plane, and the plane 
  // that is just past the end?
  if ( directionFlag )
    {
    jstart  = 0;
    jend    = ((int)( (size[1]-1) / 
                      me->GetInternalSkipFactor())+1)*me->GetInternalSkipFactor();
    
    // Offset the slices so that if we take just one it is in the middle
    jstart += (size[1]-1-jend+me->GetInternalSkipFactor())/2;
    jend   += (size[1]-1-jend+me->GetInternalSkipFactor())/2;

    jinc    = me->GetInternalSkipFactor();
    }
  else 
    {
    jstart  = (int)((size[1]-1) /
                    me->GetInternalSkipFactor()) * me->GetInternalSkipFactor();
    jend    = -me->GetInternalSkipFactor();

    // Offset the slices so that if we take just one it is in the middle
    jend   += (size[1]-1-jstart)/2;
    jstart += (size[1]-1-jstart)/2;

    jinc    = -me->GetInternalSkipFactor();
    }

  // Fill in the texture coordinates and most of the vertex information in advance
  float offset[2];
  offset[0] = 0.5 / (float)textureSize[0];
  offset[1] = 0.5 / (float)textureSize[1];
  
  for ( i = 0; i < numTiles; i++ )
    {
    yTile = i / xTotal;
    xTile = i % xTotal;
    
    t[i*8 + 0] = (float)((size[0]*(xTile  ))  )/(float)textureSize[0] + offset[0];
    t[i*8 + 1] = (float)((size[2]*(yTile  ))  )/(float)textureSize[1] + offset[1];
    t[i*8 + 2] = (float)((size[0]*(xTile  ))  )/(float)textureSize[0] + offset[0];
    t[i*8 + 3] = (float)((size[2]*(yTile+1))-1)/(float)textureSize[1] - offset[1];
    t[i*8 + 4] = (float)((size[0]*(xTile+1))-1)/(float)textureSize[0] - offset[0];
    t[i*8 + 5] = (float)((size[2]*(yTile+1))-1)/(float)textureSize[1] - offset[1];
    t[i*8 + 6] = (float)((size[0]*(xTile+1))-1)/(float)textureSize[0] - offset[0];
    t[i*8 + 7] = (float)((size[2]*(yTile  ))  )/(float)textureSize[1] + offset[1];
    
    v[i*12 + 0] = origin[0];
    v[i*12 + 2] = origin[2];
    
    v[i*12 + 3] = origin[0];
    v[i*12 + 5] = spacing[2] * (float)(size[2]-1) + origin[2];
    
    v[i*12 + 6] = spacing[0] * (float)(size[0]-1) + origin[0];
    v[i*12 + 8] = spacing[2] * (float)(size[2]-1) + origin[2];
    
    v[i*12 +  9] = spacing[0] * (float)(size[0]-1) + origin[0];
    v[i*12 + 11] = origin[2];
    }
  
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
  
  tile = 0;
  
  for ( j = jstart; j != jend; j+=jinc )
    {
    yTile = tile / xTotal;
    xTile = tile % xTotal;

    for ( k = 0; k < size[2]; k++ )
      {
      tptr = texture + 4 * ( yTile*size[2]*textureSize[0]+
                             k*textureSize[0] +
                             xTile*size[0] );
      
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
      break;
      }

    v[12*tile + 1] = 
      v[12*tile +4] = 
      v[12*tile +7] = 
      v[12*tile +10] = spacing[1] * (float)j + origin[1];

    tile++;
    
    if ( tile == numTiles  || (j+jinc == jend) )
      { 
      me->RenderQuads( tile, v, t, texture, textureSize);
      tile = 0;
      }
    
    }
  
  delete [] texture;
  delete [] v;
  delete [] t;
}

template <class T>
static void 
VolumeTextureMapper2D_ZMajorDirection( T *data_ptr,
				       int size[3],
				       int directionFlag,
				       vtkVolumeTextureMapper2D *me )
{
  int            i, j, k;
  int            kstart, kend, kinc;
  unsigned char  *tptr;
  T              *dptr;
  unsigned short *nptr;
  unsigned char  *gptr = NULL;
  float          *v, *t;
  unsigned char  *rgbaArray = me->GetRGBAArray();
  unsigned short *encodedNormals = NULL;
  float          *gradientOpacityArray;
  unsigned char  *gradientMagnitudes;
  float          *redDiffuseShadingTable = NULL;
  float          *greenDiffuseShadingTable = NULL;
  float          *blueDiffuseShadingTable = NULL;
  float          *redSpecularShadingTable = NULL;
  float          *greenSpecularShadingTable = NULL;
  float          *blueSpecularShadingTable = NULL;
  int            shade;
  float          tmpval;
  int            cropping, croppingFlags;
  float          *croppingBounds;
  int            flag[3], tmpFlag, index;
  int            clipLow = 0, clipHigh = 0;
  vtkRenderWindow  *renWin = me->GetRenderWindow();
  float            spacing[3], origin[3];
  unsigned char    zero[4];
  unsigned char    *texture;
  int              targetSize[2];
  int              textureSize[2];
  int              xTile, yTile, xTotal, yTotal, tile, numTiles;
  
  
  // How big should the texture be?
  // Start with the target size
  me->GetTargetTextureSize( targetSize );
  
  // Increase the x dimension of the texture if the x dimension of the data
  // is bigger than it (because these are x by y textures)
  if ( size[0] > targetSize[0] )
    {
    targetSize[0] = size[0];
    }

  // Increase the y dimension of the texture if the y dimension of the data
  // is bigger than it (because these are x by y textures)
  if ( size[1] > targetSize[1] )
    {
    targetSize[1] = size[1];
    }

  // Make sure the x dimension of the texture is a power of 2
  textureSize[0] = 32;
  while( textureSize[0] < targetSize[0] ) 
    {
    textureSize[0] *= 2;
    }
  
  // Make sure the y dimension of the texture is a power of 2
  textureSize[1] = 32;
  while( textureSize[1] < targetSize[1] )
    {
    textureSize[1] *= 2;
    }

  // Our texture might be too big - shrink it carefully making
  // sure that it is still big enough in the right dimensions to
  // handle oddly shaped volumes
  int volSize = size[0]*size[1]*size[2];
  int done = (volSize > textureSize[0]*textureSize[1]);
  int minSize[2];
  // What is the minumum size the texture could be in X (along the X
  // axis of the volume)?
  minSize[0] = 32;
  while ( minSize[0] < size[0] )
    {
    minSize[0] *= 2;
    }
  
  // What is the minumum size the texture could be in Y (along the Y
  // axis of the volume)?
  minSize[1] = 32;
  while ( minSize[1] < size[1] )
    {
    minSize[1] *= 2;
    }

  // Keep reducing the texture size until it is just big enough
  while (!done)
    {
    // Set done to 1. Reset to 0 if we make any changes.
    done = 1;

    // If the texture is bigger in some dimension that it needs to be
    // and chopping that dimension in half would still fit the whole
    // volume, then chop it in half.
    if ( textureSize[0] > minSize[0] &&
         ( ((textureSize[0]/2) / size[0]) * 
           (textureSize[1] / size[1]) >= size[2] ) )
      {
      textureSize[0] /= 2;
      done = 0;
      }
    if ( textureSize[1] > minSize[1] &&
         ( (textureSize[0] / size[0]) * 
           ((textureSize[1]/2) / size[1]) >= size[2] ) )
      {
      textureSize[1] /= 2;
      done = 0;
      }    
    }

  // Create space for the texture
  texture = new unsigned char[4*textureSize[0]*textureSize[1]];

  // How many tiles are there in X? in Y? total?
  xTotal = textureSize[0] / size[0];
  yTotal = textureSize[1] / size[1];
  numTiles = xTotal * yTotal;
  
  // Create space for the vertices and texture coordinates. You need four vertices with
  // three components each for each tile, and four texture coordinates with three
  // components each for each texture coordinate
  v = new float [12*numTiles];
  t = new float [ 8*numTiles];
  
  // Convenient for filling in the empty regions (due to clipping)
  zero[0] = 0;
  zero[1] = 0;
  zero[2] = 0;
  zero[3] = 0;

  // We need to know the spacing and origin of the data to set up the coordinates
  // correctly
  me->GetDataSpacing( spacing );
  me->GetDataOrigin( origin );

  // What is the first plane, the increment to move to the next plane, and the plane 
  // that is just past the end?
  if ( directionFlag )
    {
    kstart  = 0;
    kend    = ((int)( (size[2]-1) / 
                      me->GetInternalSkipFactor())+1)*me->GetInternalSkipFactor();
    
    // Offset the slices so that if we take just one it is in the middle
    kstart += (size[2]-1-kend+me->GetInternalSkipFactor())/2;
    kend   += (size[2]-1-kend+me->GetInternalSkipFactor())/2;
    
    kinc    = me->GetInternalSkipFactor();
    }
  else 
    {
    kstart  = (int)((size[2]-1) /
                    me->GetInternalSkipFactor()) * me->GetInternalSkipFactor();
    kend    = -me->GetInternalSkipFactor();
    
    // Offset the slices so that if we take just one it is in the middle
    kend   += (size[2]-1-kstart)/2;
    kstart += (size[2]-1-kstart)/2;
    
    kinc    = -me->GetInternalSkipFactor();
    }

  // Fill in the texture coordinates and most of the vertex information in advance
  float offset[2];
  offset[0] = 0.5 / (float)textureSize[0];
  offset[1] = 0.5 / (float)textureSize[1];
  
  for ( i = 0; i < numTiles; i++ )
    {
    yTile = i / xTotal;
    xTile = i % xTotal;
    
    t[i*8 + 0] = (float)((size[0]*(xTile  ))  )/(float)textureSize[0] + offset[0];
    t[i*8 + 1] = (float)((size[1]*(yTile  ))  )/(float)textureSize[1] + offset[1];
    t[i*8 + 2] = (float)((size[0]*(xTile  ))  )/(float)textureSize[0] + offset[0];
    t[i*8 + 3] = (float)((size[1]*(yTile+1))-1)/(float)textureSize[1] - offset[1];
    t[i*8 + 4] = (float)((size[0]*(xTile+1))-1)/(float)textureSize[0] - offset[0];
    t[i*8 + 5] = (float)((size[1]*(yTile+1))-1)/(float)textureSize[1] - offset[1];
    t[i*8 + 6] = (float)((size[0]*(xTile+1))-1)/(float)textureSize[0] - offset[0];
    t[i*8 + 7] = (float)((size[1]*(yTile  ))  )/(float)textureSize[1] + offset[1];
    
    v[i*12 + 0] = origin[0];
    v[i*12 + 1] = origin[1];
    
    v[i*12 + 3] = origin[0];
    v[i*12 + 4] = spacing[1] * (float)(size[1]-1) + origin[1];
    
    v[i*12 + 6] = spacing[0] * (float)(size[0]-1) + origin[0];
    v[i*12 + 7] = spacing[1] * (float)(size[1]-1) + origin[1];
    
    v[i*12 +  9] = spacing[0] * (float)(size[0]-1) + origin[0];
    v[i*12 + 10] = origin[1];
    }

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

  tile = 0; 
  
  for ( k = kstart; k != kend; k+=kinc )
    {
    yTile = tile / xTotal;
    xTile = tile % xTotal;

    for ( j = 0; j < size[1]; j++ )
      {
      tptr = texture + 4 * ( yTile*size[1]*textureSize[0]+
                             j*textureSize[0] +
                             xTile*size[0] );

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
      break;
      }

    v[12*tile + 2] = 
      v[12*tile +5] = 
      v[12*tile +8] = 
      v[12*tile +11] = spacing[2] * (float)k + origin[2];

    tile++;
    
    if ( tile == numTiles  || (k+kinc == kend) )
      { 
      me->RenderQuads( tile, v, t, texture, textureSize);
      tile = 0;
      }
    
    }
  
  delete [] texture;
  delete [] v;
  delete [] t;

}

vtkVolumeTextureMapper2D::vtkVolumeTextureMapper2D()
{
  this->TargetTextureSize[0]  = 512;
  this->TargetTextureSize[1]  = 512;
  this->MaximumNumberOfPlanes =   0;
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


void vtkVolumeTextureMapper2D::GenerateTexturesAndRenderQuads()
{
  vtkImageData           *input = this->GetInput();
  int                    size[3];
  void                   *inputPointer;
  int                    inputType;

  inputPointer = 
    input->GetPointData()->GetScalars()->GetVoidPointer(0);
  inputType = 
    input->GetPointData()->GetScalars()->GetDataType();

  input->GetDimensions( size );


  switch ( inputType )
    {
    case VTK_UNSIGNED_CHAR:
      switch ( this->MajorDirection )
	{
	case VTK_PLUS_X_MAJOR_DIRECTION:
	  VolumeTextureMapper2D_XMajorDirection
	    ( (unsigned char *)inputPointer, size, 1, this );
	  break;

	case VTK_MINUS_X_MAJOR_DIRECTION:
	  VolumeTextureMapper2D_XMajorDirection
	    ( (unsigned char *)inputPointer, size, 0, this );
	  break;

	case VTK_PLUS_Y_MAJOR_DIRECTION:
	  VolumeTextureMapper2D_YMajorDirection
	    ( (unsigned char *)inputPointer, size, 1, this );
	  break;

	case VTK_MINUS_Y_MAJOR_DIRECTION:
	  VolumeTextureMapper2D_YMajorDirection
	    ( (unsigned char *)inputPointer, size, 0, this );
	  break;

	case VTK_PLUS_Z_MAJOR_DIRECTION:
	  VolumeTextureMapper2D_ZMajorDirection
	    ( (unsigned char *)inputPointer, size, 1, this );
	  break;

	case VTK_MINUS_Z_MAJOR_DIRECTION:
	  VolumeTextureMapper2D_ZMajorDirection
	    ( (unsigned char *)inputPointer, size, 0, this );
	  break;
	}
      break;
    case VTK_UNSIGNED_SHORT:
      switch ( this->MajorDirection )
	{
	case VTK_PLUS_X_MAJOR_DIRECTION:
	  VolumeTextureMapper2D_XMajorDirection
	    ( (unsigned short *)inputPointer, size, 1, this );
	  break;

	case VTK_MINUS_X_MAJOR_DIRECTION:
	  VolumeTextureMapper2D_XMajorDirection
	    ( (unsigned short *)inputPointer, size, 0, this );
	  break;

	case VTK_PLUS_Y_MAJOR_DIRECTION:
	  VolumeTextureMapper2D_YMajorDirection
	    ( (unsigned short *)inputPointer, size, 1, this );
	  break;

	case VTK_MINUS_Y_MAJOR_DIRECTION:
	  VolumeTextureMapper2D_YMajorDirection
	    ( (unsigned short *)inputPointer, size, 0, this );
	  break;

	case VTK_PLUS_Z_MAJOR_DIRECTION:
	  VolumeTextureMapper2D_ZMajorDirection
	    ( (unsigned short *)inputPointer, size, 1, this );
	  break;

	case VTK_MINUS_Z_MAJOR_DIRECTION:
	  VolumeTextureMapper2D_ZMajorDirection
	    ( (unsigned short *)inputPointer, size, 0, this );
	  break;
	}
      break;
    default:
      vtkErrorMacro(
        "vtkVolumeTextureMapper2D only works with short or char data.\n" << 
        "Input type: " << inputType << " given.");
    }

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

  // Determine the internal skip factor - if there is a limit on the number of planes
  // we can have (the MaximumNumberOfPlanes value is greater than 0) then increase
  // this skip factor until we ensure the maximum condition.
  this->InternalSkipFactor = 1;
  if ( this->MaximumNumberOfPlanes > 0 )
    {
    int size[3];
    this->GetInput()->GetDimensions( size );
    while ( (float)size[this->MajorDirection/2] / (float)this->InternalSkipFactor > 
            (float)this->MaximumNumberOfPlanes )
      {
      this->InternalSkipFactor++;
      }
    }
  // Assume that the spacing between samples is 1/2 of the maximum - this could be
  // computed accurately for parallel (but isn't right now). For perspective, this
  // spacing changes across the image so no one number will be accurate. 1/2 the
  // maximum is (1 + sqrt(2)) / 2 = 1.2071
  this->GetInput()->GetSpacing( this->DataSpacing );
  this->SampleDistance = 
    this->DataSpacing[this->MajorDirection/2]*this->InternalSkipFactor*1.2071;
  this->vtkVolumeTextureMapper::InitializeRender( ren, vol );
}


// Print the vtkVolumeTextureMapper2D
void vtkVolumeTextureMapper2D::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "TargetTextureSize: "
     << this->TargetTextureSize[0] << ", "
     << this->TargetTextureSize[1] << endl;
  
  os << indent << "MaximumNumberOfPlanes: ";
  
  if ( this->MaximumNumberOfPlanes > 0 )
    {
    os << this->MaximumNumberOfPlanes << endl;
    }
  else
    {
    os << "<unlimited>" << endl;
    }
  
  this->vtkVolumeTextureMapper::PrintSelf(os,indent);
}
