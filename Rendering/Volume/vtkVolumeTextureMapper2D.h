/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeTextureMapper2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkVolumeTextureMapper2D
 * @brief   Abstract class for a volume mapper
 *
 *
 * vtkVolumeTextureMapper2D renders a volume using 2D texture mapping.
 *
 * @sa
 * vtkVolumeMapper
 * @deprecated
*/

#ifndef vtkVolumeTextureMapper2D_h
#define vtkVolumeTextureMapper2D_h

#include "vtkRenderingVolumeModule.h" // For export macro
#include "vtkVolumeTextureMapper.h"

#if !defined(VTK_LEGACY_REMOVE)
class VTKRENDERINGVOLUME_EXPORT vtkVolumeTextureMapper2D : public vtkVolumeTextureMapper
{
public:
  vtkTypeMacro(vtkVolumeTextureMapper2D,vtkVolumeTextureMapper);
  void PrintSelf( ostream& os, vtkIndent indent );

  static vtkVolumeTextureMapper2D *New();

  //@{
  /**
   * Target size in pixels of each size of the texture for downloading. Default is
   * 512x512 - so a 512x512 texture will be tiled with as many slices of the volume
   * as possible, then all the quads will be rendered. This can be set to optimize
   * for a particular architecture. This must be set with numbers that are a power
   * of two.
   */
  vtkSetVector2Macro( TargetTextureSize, int );
  vtkGetVector2Macro( TargetTextureSize, int );
  //@}

  //@{
  /**
   * This is the maximum number of planes that will be created for texture mapping
   * the volume. If the volume has more voxels than this along the viewing direction,
   * then planes of the volume will be skipped to ensure that this maximum is not
   * violated. A skip factor is used, and is incremented until the maximum condition
   * is satisfied.
   */
  vtkSetMacro( MaximumNumberOfPlanes, int );
  vtkGetMacro( MaximumNumberOfPlanes, int );
  //@}

  //@{
  /**
   * This is the maximum size of saved textures in bytes. If this size is large
   * enough to hold the RGBA textures for all three directions (XxYxZx3x4 is
   * the approximate value - it is actually a bit larger due to wasted space in
   * the textures) then the textures will be saved.
   */
  vtkSetMacro( MaximumStorageSize, int );
  vtkGetMacro( MaximumStorageSize, int );
  //@}

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   * Render the volume
   */
  virtual void Render(vtkRenderer *, vtkVolume *) {}

  virtual void RenderQuads( int vtkNotUsed(count),
                            float *vtkNotUsed(v), float *vtkNotUsed(t),
                            unsigned char *vtkNotUsed(texture),
                            int vtkNotUsed(size)[2],
                            int vtkNotUsed(reverseFlag)) {}

  /**
   * Made public only for access from the templated method. Not a vtkGetMacro
   * to avoid the PrintSelf defect.
   */
  int GetInternalSkipFactor() {return this->InternalSkipFactor;};

  int *GetAxisTextureSize() {return &(this->AxisTextureSize[0][0]);};

  int GetSaveTextures() {return this->SaveTextures;};

  unsigned char *GetTexture() {return this->Texture;};

protected:
  vtkVolumeTextureMapper2D();
  ~vtkVolumeTextureMapper2D();

  void InitializeRender( vtkRenderer *ren, vtkVolume *vol )
    {this->InitializeRender( ren, vol, -1 );}

  void InitializeRender( vtkRenderer *ren, vtkVolume *vol, int majorDirection );

  void GenerateTexturesAndRenderQuads( vtkRenderer *ren, vtkVolume *vol );

  int  MajorDirection;
  int  TargetTextureSize[2];

  int  MaximumNumberOfPlanes;
  int  InternalSkipFactor;
  int  MaximumStorageSize;

  unsigned char  *Texture;
  int             TextureSize;
  int             SaveTextures;
  vtkTimeStamp    TextureMTime;

  int             AxisTextureSize[3][3];
  void            ComputeAxisTextureSize( int axis, int *size );

  void           RenderSavedTexture();

private:
  vtkVolumeTextureMapper2D(const vtkVolumeTextureMapper2D&) VTK_DELETE_FUNCTION;
  void operator=(const vtkVolumeTextureMapper2D&) VTK_DELETE_FUNCTION;
};

#endif // VTK_LEGACY_REMOVE
#endif


