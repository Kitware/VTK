/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeTextureMapper2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVolumeTextureMapper2D.h"

#include "vtkCamera.h"
#include "vtkDataArray.h"
#include "vtkVolumeRenderingFactory.h"
#include "vtkImageData.h"
#include "vtkLargeInteger.h"
#include "vtkMatrix4x4.h"
#include "vtkPointData.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"
#include "vtkVolumeProperty.h"

#define VTK_PLUS_X_MAJOR_DIRECTION  0
#define VTK_MINUS_X_MAJOR_DIRECTION 1
#define VTK_PLUS_Y_MAJOR_DIRECTION  2
#define VTK_MINUS_Y_MAJOR_DIRECTION 3
#define VTK_PLUS_Z_MAJOR_DIRECTION  4
#define VTK_MINUS_Z_MAJOR_DIRECTION 5

template <class T>
void vtkVolumeTextureMapper2D_TraverseVolume( T *data_ptr,
                                              int size[3],
                                              int axis,
                                              int directionFlag,
                                              vtkVolumeTextureMapper2D *me )
{
  int              i, j, k;
  int              kstart, kend, kinc;
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
  double           *croppingBounds;
  int              flag[3], tmpFlag, index;
  int              clipLow = 0, clipHigh = 0;
  vtkRenderWindow  *renWin = me->GetRenderWindow();
  double           spacing[3], origin[3];
  unsigned char    zero[4];
  unsigned char    *texture;
  int              textureSize[2];
  int              xTile, yTile, xTotal, yTotal, tile, numTiles;
  int              *zAxis=0, *yAxis=0, *xAxis=0;
  int              loc, inc=0;
  int              saveTextures = me->GetSaveTextures();
  int              textureOffset=0;
  
  int a0=0, a1=0, a2=0;
  
  switch ( axis )
    {
    case 0:
      a0 = 1;
      a1 = 2;
      a2 = 0;
      xAxis = &k;
      yAxis = &i;
      zAxis = &j;
      inc = size[0];
      break;
    case 1:
      a0 = 0;
      a1 = 2;
      a2 = 1;
      xAxis = &i;
      yAxis = &k;
      zAxis = &j;
      inc = 1;
      break;
    case 2:
      a0 = 0;
      a1 = 1;
      a2 = 2;
      xAxis = &i;
      yAxis = &j;
      zAxis = &k;
      inc = 1;
      break;
    }

  int *axisTextureSize = me->GetAxisTextureSize();
  textureSize[0] = axisTextureSize[a2*3+0];
  textureSize[1] = axisTextureSize[a2*3+1];

  if ( saveTextures )
    {
    texture = me->GetTexture();
    switch ( axis )
      {
      case 0:
        textureOffset = 0;
        break;
      case 1:
        textureOffset =
          4*axisTextureSize[0]*axisTextureSize[1]*axisTextureSize[2];
        break;
      case 2:
        textureOffset =
          4*axisTextureSize[0]*axisTextureSize[1]*axisTextureSize[2] +
          4*axisTextureSize[3]*axisTextureSize[4]*axisTextureSize[5];
        break;
      }
    }
  else
    {
    // Create space for the texture
    texture = new unsigned char[4*textureSize[0]*textureSize[1]];
    textureOffset = 0;
    }
  
  // How many tiles are there in X? in Y? total?
  xTotal = textureSize[0] / size[a0];
  yTotal = textureSize[1] / size[a1];
  numTiles = xTotal * yTotal;

  // Create space for the vertices and texture coordinates. You need four vertices 
  // with three components each for each tile, and four texture coordinates with 
  // three components each for each texture coordinate
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
    kend    = (static_cast<int>( (size[a2]-1) / 
                      me->GetInternalSkipFactor())+1)*me->GetInternalSkipFactor();
    
    // Offset the slices so that if we take just one it is in the middle
    kstart += (size[a2]-1-kend+me->GetInternalSkipFactor())/2;
    kend   += (size[a2]-1-kend+me->GetInternalSkipFactor())/2;
    
    kinc    = me->GetInternalSkipFactor();
    }
  else 
    {
    kstart  = static_cast<int>((size[a2]-1) /
                    me->GetInternalSkipFactor()) * me->GetInternalSkipFactor();
    kend    = -me->GetInternalSkipFactor();
    
    // Offset the slices so that if we take just one it is in the middle
    kend   += (size[a2]-1-kstart)/2;
    kstart += (size[a2]-1-kstart)/2;
    
    kinc    = -me->GetInternalSkipFactor();
    }

  // Fill in the texture coordinates and most of the vertex information in advance
  float offset[2];
  offset[0] = 0.5 / textureSize[0];
  offset[1] = 0.5 / textureSize[1];
  
  for ( i = 0; i < numTiles; i++ )
    {
    yTile = i / xTotal;
    xTile = i % xTotal;
    
    t[i*8 + 0] = size[a0]*xTile/static_cast<float>(textureSize[0]) + offset[0];
    t[i*8 + 1] = size[a1]*yTile/static_cast<float>(textureSize[1]) + offset[1];
    t[i*8 + 2] = size[a0]*xTile/static_cast<float>(textureSize[0]) + offset[0];
    t[i*8 + 3] = size[a1]*(yTile+1)/static_cast<float>(textureSize[1]) - offset[1];
    t[i*8 + 4] = size[a0]*(xTile+1)/static_cast<float>(textureSize[0]) - offset[0];
    t[i*8 + 5] = size[a1]*(yTile+1)/static_cast<float>(textureSize[1]) - offset[1];
    t[i*8 + 6] = size[a0]*(xTile+1)/static_cast<float>(textureSize[0]) - offset[0];
    t[i*8 + 7] = size[a1]*(yTile  )/static_cast<float>(textureSize[1]) + offset[1];
    
    v[i*12 + a0] = origin[a0];
    v[i*12 + a1] = origin[a1];
    
    v[i*12 + 3+a0] = origin[a0];
    v[i*12 + 3+a1] = spacing[a1] * (size[a1]-1) + origin[a1];
    
    v[i*12 + 6+a0] = spacing[a0] * (size[a0]-1) + origin[a0];
    v[i*12 + 6+a1] = spacing[a1] * (size[a1]-1) + origin[a1];
    
    v[i*12 + 9+a0] = spacing[a0] * (size[a0]-1) + origin[a0];
    v[i*12 + 9+a1] = origin[a1];
    }

  cropping       = me->GetCropping();
  croppingFlags  = me->GetCroppingRegionFlags();
  croppingBounds = me->GetVoxelCroppingRegionPlanes();

  if ( !cropping )
    {
    clipLow    = 0;
    clipHigh   = size[a0];
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

    for ( j = 0; j < size[a1]; j++ )
      {
      i = 0;
      
      tptr = texture + textureOffset + 
        4 * ( yTile*size[a1]*textureSize[0]+
              j*textureSize[0] +
              xTile*size[a0] );
      
      loc = (*zAxis)*size[0]*size[1] + (*yAxis)*size[0] + (*xAxis);
      dptr = data_ptr + loc;

      // Given a Y and Z value, what are the cropping bounds
      // on X.
      if ( cropping )
        {
        switch ( axis )
          {
          case 0:
            clipLow  = static_cast<int>(ceil(croppingBounds[2]));
            clipHigh = static_cast<int>(ceil(croppingBounds[3]));
            tmpFlag =    (*xAxis<croppingBounds[0])?(0):(1+(*xAxis>=croppingBounds[1]));
            tmpFlag+= 9*((*zAxis<croppingBounds[4])?(0):(1+(*zAxis>=croppingBounds[5])));
            flag[0]  = ((croppingFlags >> (tmpFlag)) & 0x1);
            flag[1]  = ((croppingFlags >> (tmpFlag+3)) & 0x1);
            flag[2]  = ((croppingFlags >> (tmpFlag+6)) & 0x1);
            break;
          case 1:
            clipLow  = static_cast<int>(ceil(croppingBounds[0]));
            clipHigh = static_cast<int>(ceil(croppingBounds[1]));
            tmpFlag = 3*((*yAxis<croppingBounds[2])?(0):(1+(*yAxis>=croppingBounds[3])));
            tmpFlag+= 9*((*zAxis<croppingBounds[4])?(0):(1+(*zAxis>=croppingBounds[5])));
            flag[0]  = ((croppingFlags >> (tmpFlag)) & 0x1);
            flag[1]  = ((croppingFlags >> (tmpFlag+1)) & 0x1);
            flag[2]  = ((croppingFlags >> (tmpFlag+2)) & 0x1);
            break;
          case 2:
            clipLow  = static_cast<int>(ceil(croppingBounds[0]));
            clipHigh = static_cast<int>(ceil(croppingBounds[1]));
            tmpFlag = 3*((*yAxis<croppingBounds[2])?(0):(1+(*yAxis>=croppingBounds[3])));
            tmpFlag+= 9*((*zAxis<croppingBounds[4])?(0):(1+(*zAxis>=croppingBounds[5])));
            flag[0]  = ((croppingFlags >> (tmpFlag)) & 0x1);
            flag[1]  = ((croppingFlags >> (tmpFlag+1)) & 0x1);
            flag[2]  = ((croppingFlags >> (tmpFlag+2)) & 0x1);
            break;
          }
        }
      
      if ( shade )
        {
        nptr = encodedNormals + loc;
        
        if ( gradientMagnitudes )
          {
          gptr = gradientMagnitudes + loc;
          }
        for ( i = 0; i < size[a0]; i++ )
          {
          index = 0;
          index += ( i >= clipLow );
          index += ( i >= clipHigh );

          // Add a 1 pixel border to avoid black on edges of visible texture
          if (flag[index] || (i >= (clipLow-1) && i <= clipHigh))
            {
            tmpval = rgbaArray[(*dptr)*4];
            tmpval = tmpval * redDiffuseShadingTable[*nptr] +
              redSpecularShadingTable[*nptr]*255.0;
            if ( tmpval > 255.0 )
              {
              tmpval = 255.0;
              }
            *(tptr++) = static_cast<unsigned char>(tmpval);
            
            tmpval = rgbaArray[(*dptr)*4 + 1];
            tmpval = tmpval * greenDiffuseShadingTable[*nptr] +
              greenSpecularShadingTable[*nptr]*255.0;
            if ( tmpval > 255.0 )
              {
              tmpval = 255.0;
              }
            *(tptr++) = static_cast<unsigned char>(tmpval);
            
            tmpval = rgbaArray[(*dptr)*4 + 2];
            tmpval = tmpval * blueDiffuseShadingTable[*nptr] +
              blueSpecularShadingTable[*nptr]*255.0;
            if ( tmpval > 255.0 )
              {
              tmpval = 255.0;
              }
            *(tptr++) = static_cast<unsigned char>(tmpval);
            
            tmpval = rgbaArray[(*dptr)*4 + 3];
            if ( gradientMagnitudes )
              {
              tmpval *= gradientOpacityArray[*gptr];
              gptr += inc;
              }
            *(tptr++) = static_cast<unsigned char>(tmpval*flag[index]);
            }
          else
            {
            memcpy( tptr, zero, 4 );
            tptr += 4;
            if ( gradientMagnitudes )
              {
              gptr += inc;
              }
            }
          nptr += inc;
          dptr += inc;
          }
        }
      else
        {
        if ( gradientMagnitudes )
          {
          gptr = gradientMagnitudes + loc;
          }

        if ( cropping )
          {
          for ( i = 0; i < size[a0]; i++ )
            {
            index = 0;
            index += ( i >= clipLow );
            index += ( i >= clipHigh );

            memcpy( tptr, rgbaArray + (*dptr)*4, 4 );
            // clear the alpha if flag shows that region is cropped
            *(tptr+3) *= static_cast<unsigned char>(flag[index]);
            if ( gradientMagnitudes )
              {
              *(tptr+3) = static_cast<unsigned char>
                ((*(tptr+3)) * gradientOpacityArray[*gptr]);
              gptr += inc;
              }

            tptr += 4;
            dptr += inc;
            }
          }
        else
          {
          if ( gradientMagnitudes )
            {
            for ( i = 0; i < size[a0]; i++ )
              {
              memcpy( tptr, rgbaArray + (*dptr)*4, 4 );
              *(tptr+3) = static_cast<unsigned char>
                ((*(tptr+3)) * gradientOpacityArray[*gptr]);
              gptr += inc;
              dptr += inc;
              tptr += 4;
              }
            }
          else
            {
            for ( i = 0; i < size[a0]; i++ )
              {
              memcpy( tptr, rgbaArray + (*dptr)*4, 4 );
              tptr += 4;
              dptr += inc;
              }
            }
          }
        }
      }

    if ( renWin->CheckAbortStatus() )
      {
      break;
      }

    v[12*tile + a2] = 
      v[12*tile + 3+a2] = 
      v[12*tile + 6+a2] = 
      v[12*tile + 9+a2] = spacing[a2] * k + origin[a2];

    tile++;
    
    if ( tile == numTiles  || (k+kinc == kend) )
      { 
      if ( saveTextures )
        {
        textureOffset += 4*axisTextureSize[a2*3] * axisTextureSize[a2*3+1];
        }
      else
        {
        me->RenderQuads( tile, v, t, texture, textureSize, 0);
        }      
      tile = 0;
      }
    
    }
  
  
  if ( !saveTextures )
    {
    delete [] texture;
    }
  
  delete [] v;
  delete [] t;

}


//----------------------------------------------------------------------------
// Needed when we don't use the vtkStandardNewMacro.
vtkInstantiatorNewMacro(vtkVolumeTextureMapper2D);
//----------------------------------------------------------------------------

vtkVolumeTextureMapper2D::vtkVolumeTextureMapper2D()
{
  this->TargetTextureSize[0]  = 512;
  this->TargetTextureSize[1]  = 512;
  this->MaximumNumberOfPlanes =   0;
  this->MaximumStorageSize    =   0;
  this->Texture               = NULL;
  this->TextureSize           = 0;
}

vtkVolumeTextureMapper2D::~vtkVolumeTextureMapper2D()
{
  if ( this->Texture )
    {
    delete [] this->Texture;
    }
}


vtkVolumeTextureMapper2D *vtkVolumeTextureMapper2D::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = 
    vtkVolumeRenderingFactory::CreateInstance("vtkVolumeTextureMapper2D");
  return static_cast<vtkVolumeTextureMapper2D *>(ret);
}

void vtkVolumeTextureMapper2D::RenderSavedTexture()
{
  int              i, k;
  int              kstart, kend, kinc;
  unsigned char    *tptr;
  float            *v, *t;
  vtkRenderWindow  *renWin = this->GetRenderWindow();
  double           spacing[3], origin[3];
  unsigned char    *texture;
  int              textureSize[2];
  int              xTile, yTile, xTotal, yTotal, tile, numTiles;
  int              textureOffset=0;
  int              axis=0, directionFlag=0;
  int              size[3];
  
  int a0=0, a1=0, a2=0;

  this->GetInput()->GetDimensions( size );

  switch ( this->MajorDirection )
    {    
    case VTK_PLUS_X_MAJOR_DIRECTION:
      axis = 0;
      directionFlag = 1;
      break;
    case VTK_MINUS_X_MAJOR_DIRECTION:
      axis = 0;
      directionFlag = 0;
      break;
    case VTK_PLUS_Y_MAJOR_DIRECTION:
      axis = 1;
      directionFlag = 1;
      break;
    case VTK_MINUS_Y_MAJOR_DIRECTION:
      axis = 1;
      directionFlag = 0;
      break;
    case VTK_PLUS_Z_MAJOR_DIRECTION:
      axis = 2;
      directionFlag = 1;
      break;
    case VTK_MINUS_Z_MAJOR_DIRECTION:
      axis = 2;
      directionFlag = 0;
      break;
    }
  
  switch ( axis )
    {
    case 0:
      a0 = 1;
      a1 = 2;
      a2 = 0;
      break;
    case 1:
      a0 = 0;
      a1 = 2;
      a2 = 1;
      break;
    case 2:
      a0 = 0;
      a1 = 1;
      a2 = 2;
      break;
    }

  textureSize[0] = this->AxisTextureSize[a2][0];
  textureSize[1] = this->AxisTextureSize[a2][1];

  texture = this->Texture;
  switch ( axis )
    {
    case 0:
      textureOffset = 0;
      break;
    case 1:
      textureOffset =
        4*(this->AxisTextureSize[0][0]*
           this->AxisTextureSize[0][1]*
           this->AxisTextureSize[0][2]);
      break;
    case 2:
      textureOffset =
        4*(this->AxisTextureSize[0][0]*
           this->AxisTextureSize[0][1]*
           this->AxisTextureSize[0][2]) +
        4*(this->AxisTextureSize[1][0]*
           this->AxisTextureSize[1][1]*
           this->AxisTextureSize[1][2]);
      break;
    }

  if ( directionFlag == 0 )
    {
    textureOffset +=
      4*(this->AxisTextureSize[a2][0]*
         this->AxisTextureSize[a2][1]*
         (this->AxisTextureSize[a2][2]-1));
    }

  // How many tiles are there in X? in Y? total?
  xTotal = textureSize[0] / size[a0];
  yTotal = textureSize[1] / size[a1];
  numTiles = xTotal * yTotal;
  
  // Create space for the vertices and texture coordinates. You need four vertices 
  // with three components each for each tile, and four texture coordinates with 
  // three components each for each texture coordinate
  v = new float [12*numTiles];
  t = new float [ 8*numTiles];
  
  // We need to know the spacing and origin of the data to set up the coordinates
  // correctly
  this->GetDataSpacing( spacing );
  this->GetDataOrigin( origin );

  // What is the first plane, the increment to move to the next plane, and the plane 
  // that is just past the end?
  if ( directionFlag )
    {
    kstart  = 0;
    kend    = (static_cast<int>( (size[a2]-1) / 
                                 this->InternalSkipFactor)+1)*this->InternalSkipFactor;
    
    // Offset the slices so that if we take just one it is in the middle
    kstart += (size[a2]-1-kend+this->InternalSkipFactor)/2;
    kend   += (size[a2]-1-kend+this->InternalSkipFactor)/2;
    
    kinc    = this->InternalSkipFactor;
    }
  else 
    {
    kstart  = static_cast<int>((size[a2]-1) /
                    this->InternalSkipFactor) * this->InternalSkipFactor;
    kend    = -this->InternalSkipFactor;
    
    // Offset the slices so that if we take just one it is in the middle
    kend   += (size[a2]-1-kstart)/2;
    kstart += (size[a2]-1-kstart)/2;
    
    kinc    = -this->InternalSkipFactor;
    }

  // Fill in the texture coordinates and most of the vertex information in advance
  float offset[2];
  offset[0] = 0.5 / textureSize[0];
  offset[1] = 0.5 / textureSize[1];
  
  int idx;
  for ( idx = 0; idx < numTiles; idx++ )
    {
    i = ( directionFlag == 1 )?(idx):(numTiles-idx-1);
    
    yTile = i / xTotal;
    xTile = i % xTotal;
    
    t[i*8 + 0] = size[a0]*xTile/static_cast<float>(textureSize[0]) + offset[0];
    t[i*8 + 1] = size[a1]*yTile/static_cast<float>(textureSize[1]) + offset[1];
    t[i*8 + 2] = size[a0]*xTile/static_cast<float>(textureSize[0]) + offset[0];
    t[i*8 + 3] = size[a1]*(yTile+1)/static_cast<float>(textureSize[1]) - offset[1];
    t[i*8 + 4] = size[a0]*(xTile+1)/static_cast<float>(textureSize[0]) - offset[0];
    t[i*8 + 5] = size[a1]*(yTile+1)/static_cast<float>(textureSize[1]) - offset[1];
    t[i*8 + 6] = size[a0]*(xTile+1)/static_cast<float>(textureSize[0]) - offset[0];
    t[i*8 + 7] = size[a1]*yTile/static_cast<float>(textureSize[1]) + offset[1];
    
    v[i*12 + a0] = origin[a0];
    v[i*12 + a1] = origin[a1];
    
    v[i*12 + 3+a0] = origin[a0];
    v[i*12 + 3+a1] = spacing[a1] * (size[a1]-1) + origin[a1];
    
    v[i*12 + 6+a0] = spacing[a0] * (size[a0]-1) + origin[a0];
    v[i*12 + 6+a1] = spacing[a1] * (size[a1]-1) + origin[a1];
    
    v[i*12 + 9+a0] = spacing[a0] * (size[a0]-1) + origin[a0];
    v[i*12 + 9+a1] = origin[a1];
    }

  if ( directionFlag == 1 )
    {
    tile = 0;
    }
  else
    {
    tile = (((kend - kstart)/kinc)-1)%numTiles;
    }

  int tileCount = 0;
  
  for ( k = kstart; k != kend; k+=kinc )
    {
    if ( renWin->CheckAbortStatus() )
      {
      break;
      }

    v[12*tile + a2] = 
      v[12*tile + 3+a2] = 
      v[12*tile + 6+a2] = 
      v[12*tile + 9+a2] = spacing[a2] * k + origin[a2];

    tileCount++;
    
    if ( directionFlag == 1 )
      {
      tile++;
      }
    else
      {
      tile--;
      }
    
    if ( (directionFlag == 1 && tile == numTiles ) ||
         (directionFlag == 0 && tile == -1) || (k+kinc == kend) )
      { 
      tptr = texture + textureOffset;
      if ( directionFlag == 1 )
        {
        textureOffset += 
          4*this->AxisTextureSize[a2][0] * this->AxisTextureSize[a2][1];
        }
      else
        {
        textureOffset -= 
          4*this->AxisTextureSize[a2][0] * this->AxisTextureSize[a2][1];
        }
      
      this->RenderQuads( tileCount, v, t, tptr, textureSize, !directionFlag );
      tile = (directionFlag == 1)?(0):(numTiles-1); 
      tileCount = 0;
      }
    }
  
  delete [] v;
  delete [] t;

}

void vtkVolumeTextureMapper2D::GenerateTexturesAndRenderQuads( vtkRenderer *ren, vtkVolume *vol )
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

  // Do we have a texture already, and nothing has changed? If so
  // just render it.
  if ( this->Texture && !this->Shade &&
       this->GetMTime() < this->TextureMTime &&
       this->GetInput()->GetMTime() < this->TextureMTime &&
       vol->GetProperty()->GetMTime() < this->TextureMTime )
    {
    this->RenderSavedTexture();
    return;
    }

  // Otherwise, we need to generate textures. We can throw away any
  // saved textures
  if ( this->Texture )
    {
    delete [] this->Texture;
        this->Texture = NULL;
    }
  this->TextureSize = 0;
  
  // Will all the textures fit in the allotted storage?
  this->ComputeAxisTextureSize( 0, this->AxisTextureSize[0] );
  this->ComputeAxisTextureSize( 1, this->AxisTextureSize[1] );
  this->ComputeAxisTextureSize( 2, this->AxisTextureSize[2] );

  vtkLargeInteger neededSize;
  vtkLargeInteger tmpInt;


  neededSize = 
    this->AxisTextureSize[0][0];
  neededSize = neededSize *
    this->AxisTextureSize[0][1] *
    this->AxisTextureSize[0][2] ;

  tmpInt =
    this->AxisTextureSize[1][0];
  tmpInt = tmpInt *
    this->AxisTextureSize[1][1] *
    this->AxisTextureSize[1][2];
  neededSize = neededSize + tmpInt;

  tmpInt =  
    this->AxisTextureSize[2][0];
  tmpInt = tmpInt *
    this->AxisTextureSize[2][1] *
    this->AxisTextureSize[2][2];
  neededSize = neededSize + tmpInt;

  neededSize *= 4;

  if ( neededSize.GetLength() > 31 )
    {
    this->SaveTextures = 0;
    }
  else
    {
    this->SaveTextures = 
      ( neededSize.CastToLong() <= this->MaximumStorageSize && 
        !this->Shade );
    }

  if ( this->SaveTextures )
    {
    this->Texture = new unsigned char [neededSize.CastToLong()];
    this->TextureSize = neededSize.CastToLong();
    
    int savedDirection = this->MajorDirection;

    switch ( inputType )
      {
      case VTK_UNSIGNED_CHAR:
        this->InitializeRender( ren, vol, VTK_PLUS_X_MAJOR_DIRECTION );
        vtkVolumeTextureMapper2D_TraverseVolume
          ( static_cast<unsigned char *>(inputPointer), size, 0, 1, this );
        
        this->InitializeRender( ren, vol, VTK_PLUS_Y_MAJOR_DIRECTION );
        vtkVolumeTextureMapper2D_TraverseVolume
          ( static_cast<unsigned char *>(inputPointer), size, 1, 1, this );
        
        this->InitializeRender( ren, vol, VTK_PLUS_Z_MAJOR_DIRECTION );
        vtkVolumeTextureMapper2D_TraverseVolume
          ( static_cast<unsigned char *>(inputPointer), size, 2, 1, this );
        break;
      case VTK_UNSIGNED_SHORT:
        this->InitializeRender( ren, vol, VTK_PLUS_X_MAJOR_DIRECTION );
        vtkVolumeTextureMapper2D_TraverseVolume
          ( static_cast<unsigned short *>(inputPointer), size, 0, 1, this );
        
        this->InitializeRender( ren, vol, VTK_PLUS_Y_MAJOR_DIRECTION );
        vtkVolumeTextureMapper2D_TraverseVolume
          ( static_cast<unsigned short *>(inputPointer), size, 1, 1, this );
        
        this->InitializeRender( ren, vol, VTK_PLUS_Z_MAJOR_DIRECTION );
        vtkVolumeTextureMapper2D_TraverseVolume
          ( static_cast<unsigned short *>(inputPointer), size, 2, 1, this );
        break;
      }
    
    this->MajorDirection = savedDirection;
    if ( !ren->GetRenderWindow()->GetAbortRender() ) 
      {
      this->RenderSavedTexture();
      this->TextureMTime.Modified();
      }
    }
  else
    {
    
    switch ( inputType )
      {
      case VTK_UNSIGNED_CHAR:
        switch ( this->MajorDirection )
          {
          case VTK_PLUS_X_MAJOR_DIRECTION:
            vtkVolumeTextureMapper2D_TraverseVolume
              ( static_cast<unsigned char *>(inputPointer), size, 0, 1, this );
            break;
            
          case VTK_MINUS_X_MAJOR_DIRECTION:
            vtkVolumeTextureMapper2D_TraverseVolume
              ( static_cast<unsigned char *>(inputPointer), size, 0, 0, this );
            break;
            
          case VTK_PLUS_Y_MAJOR_DIRECTION:
            vtkVolumeTextureMapper2D_TraverseVolume
              ( static_cast<unsigned char *>(inputPointer), size, 1, 1, this );
            break;
            
          case VTK_MINUS_Y_MAJOR_DIRECTION:
            vtkVolumeTextureMapper2D_TraverseVolume
              ( static_cast<unsigned char *>(inputPointer), size, 1, 0, this );
            break;
            
          case VTK_PLUS_Z_MAJOR_DIRECTION:
            vtkVolumeTextureMapper2D_TraverseVolume
              ( static_cast<unsigned char *>(inputPointer), size, 2, 1, this );
            break;
            
          case VTK_MINUS_Z_MAJOR_DIRECTION:
            vtkVolumeTextureMapper2D_TraverseVolume
              ( static_cast<unsigned char *>(inputPointer), size, 2, 0, this );
            break;
          }
        break;
      case VTK_UNSIGNED_SHORT:
        switch ( this->MajorDirection )
          {
          case VTK_PLUS_X_MAJOR_DIRECTION:
            vtkVolumeTextureMapper2D_TraverseVolume
              ( static_cast<unsigned short *>(inputPointer), size, 0, 1, this );
            break;
            
          case VTK_MINUS_X_MAJOR_DIRECTION:
            vtkVolumeTextureMapper2D_TraverseVolume
              ( static_cast<unsigned short *>(inputPointer), size, 0, 0, this );
            break;
            
          case VTK_PLUS_Y_MAJOR_DIRECTION:
            vtkVolumeTextureMapper2D_TraverseVolume
              ( static_cast<unsigned short *>(inputPointer), size, 1, 1, this );
            break;
            
          case VTK_MINUS_Y_MAJOR_DIRECTION:
            vtkVolumeTextureMapper2D_TraverseVolume
              ( static_cast<unsigned short *>(inputPointer), size, 1, 0, this );
            break;
            
          case VTK_PLUS_Z_MAJOR_DIRECTION:
            vtkVolumeTextureMapper2D_TraverseVolume
              ( static_cast<unsigned short *>(inputPointer), size, 2, 1, this );
            break;
            
          case VTK_MINUS_Z_MAJOR_DIRECTION:
            vtkVolumeTextureMapper2D_TraverseVolume
              ( static_cast<unsigned short *>(inputPointer), size, 2, 0, this );
            break;
          }
        break;
      default:
        vtkErrorMacro(
          "vtkVolumeTextureMapper2D only works with unsigned short and unsigned char data.\n" << 
          "Input type: " << inputType << " given.");
      }
    }
}

void vtkVolumeTextureMapper2D::InitializeRender( vtkRenderer *ren,
                                                 vtkVolume *vol,
                                                 int majorDirection )
{
  if ( majorDirection >= 0)
    {
    this->MajorDirection = majorDirection;
    }
  else
    {
    double vpn[3];

    // Take the vpn, convert it to volume coordinates, and find the 
    // major direction
    vtkMatrix4x4 *volMatrix = vtkMatrix4x4::New();
    volMatrix->DeepCopy( vol->GetMatrix() );
    vtkTransform *worldToVolumeTransform = vtkTransform::New();
    worldToVolumeTransform->SetMatrix( volMatrix );
     
    // Create a transform that will account for the translation of
    // the scalar data.
    vtkTransform *volumeTransform = vtkTransform::New();
    
    volumeTransform->Identity();
    volumeTransform->Translate(this->GetInput()->GetOrigin());
    
    // Now concatenate the volume's matrix with this scalar data matrix
    worldToVolumeTransform->PreMultiply();
    worldToVolumeTransform->Concatenate( volumeTransform->GetMatrix() );
    worldToVolumeTransform->Inverse();
    
    ren->GetActiveCamera()->GetViewPlaneNormal(vpn);
    worldToVolumeTransform->TransformVector( vpn, vpn );
  
    volMatrix->Delete();
    volumeTransform->Delete();
    worldToVolumeTransform->Delete();
                                                  
    if ( fabs(vpn[0]) >= fabs(vpn[1]) && fabs(vpn[0]) >= fabs(vpn[2]) )
      {
      this->MajorDirection = (vpn[0]<0.0)?
        (VTK_MINUS_X_MAJOR_DIRECTION):(VTK_PLUS_X_MAJOR_DIRECTION);
      }
    else if ( fabs(vpn[1]) >= fabs(vpn[0]) && fabs(vpn[1]) >= fabs(vpn[2]) )
      {
      this->MajorDirection = (vpn[1]<0.0)?
        (VTK_MINUS_Y_MAJOR_DIRECTION):(VTK_PLUS_Y_MAJOR_DIRECTION);
      }
    else
      {
      this->MajorDirection = (vpn[2]<0.0)?
        (VTK_MINUS_Z_MAJOR_DIRECTION):(VTK_PLUS_Z_MAJOR_DIRECTION);
      }
    }

  // Determine the internal skip factor - if there is a limit on the number 
  // of planes we can have (the MaximumNumberOfPlanes value is greater than 
  // 0) then increase this skip factor until we ensure the maximum condition.
  this->InternalSkipFactor = 1;
  if ( this->MaximumNumberOfPlanes > 0 )
    {
    int size[3];
    this->GetInput()->GetDimensions( size );
    while ( size[this->MajorDirection/2] / static_cast<float>(this->InternalSkipFactor) > static_cast<float>(this->MaximumNumberOfPlanes) )
      {
      this->InternalSkipFactor++;
      }
    }
  // Assume that the spacing between samples is 1/2 of the maximum - this 
  // could be computed accurately for parallel (but isn't right now). For 
  // perspective, this spacing changes across the image so no one number will 
  // be accurate. 1/2 the maximum is (1 + sqrt(2)) / 2 = 1.2071
  
  double *dspacing;
  dspacing = this->GetInput()->GetSpacing();
  this->DataSpacing[0] = dspacing[0];
  this->DataSpacing[1] = dspacing[1];
  this->DataSpacing[2] = dspacing[2];
  this->SampleDistance = 
    this->DataSpacing[this->MajorDirection/2]*this->InternalSkipFactor*1.2071;
  this->vtkVolumeTextureMapper::InitializeRender( ren, vol );
}

void vtkVolumeTextureMapper2D::ComputeAxisTextureSize( int axis, int *textureSize )
{ 
  int targetSize[2];
  int a0=0, a1=0, a2=0;
  
  switch ( axis )
    {
    case 0:
      a0 = 1;
      a1 = 2;
      a2 = 0;
      break;
    case 1:
      a0 = 0;
      a1 = 2;
      a2 = 1;
      break;
    case 2:
      a0 = 0;
      a1 = 1;
      a2 = 2;
      break;
    }
  
  
  // How big should the texture be?
  // Start with the target size
  targetSize[0] = this->TargetTextureSize[0];
  targetSize[1] = this->TargetTextureSize[1];
  
  int size[3];
  this->GetInput()->GetDimensions( size );

  // Increase the x dimension of the texture if the x dimension of the data
  // is bigger than it (because these are x by y textures)
  if ( size[a0] > targetSize[0] )
    {
    targetSize[0] = size[a0];
    }

  // Increase the y dimension of the texture if the y dimension of the data
  // is bigger than it (because these are x by y textures)
  if ( size[a1] > targetSize[1] )
    {
    targetSize[1] = size[a1];
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
  while ( minSize[0] < size[a0] )
    {
    minSize[0] *= 2;
    }
  
  // What is the minumum size the texture could be in Y (along the Y
  // axis of the volume)?
  minSize[1] = 32;
  while ( minSize[1] < size[a1] )
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
         ( ((textureSize[0]/2) / size[a0]) * 
           (textureSize[1] / size[a1]) >= size[a2] ) )
      {
      textureSize[0] /= 2;
      done = 0;
      }
    if ( textureSize[1] > minSize[1] &&
         ( (textureSize[0] / size[a0]) * 
           ((textureSize[1]/2) / size[a1]) >= size[a2] ) )
      {
      textureSize[1] /= 2;
      done = 0;
      }    
    }

  // This is how many texture planes would be necessary if one slice fit on a 
  // texture (taking into account the user defined maximum)
  textureSize[2] = 
    (size[a2]<this->MaximumNumberOfPlanes||this->MaximumNumberOfPlanes<=0) ?
    (size[a2]) : (this->MaximumNumberOfPlanes);
  
  // How many slices can fit on a texture in X and Y?
  int xTotal = textureSize[0] / size[a0];
  int yTotal = textureSize[1] / size[a1];

  // The number of textures we need is the number computed above divided by
  // how many fit on a texture (plus one if they don't fit evenly)
  textureSize[2] = (textureSize[2] / (xTotal*yTotal)) + 
    ((textureSize[2] % (xTotal*yTotal))!=0);
}


// Print the vtkVolumeTextureMapper2D
void vtkVolumeTextureMapper2D::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Target Texture Size: "
     << this->TargetTextureSize[0] << ", "
     << this->TargetTextureSize[1] << endl;
  
  os << indent << "Maximum Number Of Planes: ";
  if ( this->MaximumNumberOfPlanes > 0 )
    {
    os << this->MaximumNumberOfPlanes << endl;
    }
  else
    {
    os << "<unlimited>" << endl;
    }
  
  os << indent << "Maximum Storage Size: " 
     << this->MaximumStorageSize << endl;
  
  this->Superclass::PrintSelf(os,indent);
}


