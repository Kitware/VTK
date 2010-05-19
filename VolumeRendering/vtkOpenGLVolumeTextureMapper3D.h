/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLVolumeTextureMapper3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenGLVolumeTextureMapper3D - concrete implementation of 3D volume texture mapping

// .SECTION Description
// vtkOpenGLVolumeTextureMapper3D renders a volume using 3D texture mapping.
// See vtkVolumeTextureMapper3D for full description.

// .SECTION see also
// vtkVolumeTextureMapper3D vtkVolumeMapper

#ifndef __vtkOpenGLVolumeTextureMapper3D_h
#define __vtkOpenGLVolumeTextureMapper3D_h

#include "vtkVolumeTextureMapper3D.h"

#ifndef VTK_IMPLEMENT_MESA_CXX
# include "vtkOpenGL.h" // GLfloat type is used in some method signatures.
#endif
 

class vtkRenderWindow;
class vtkVolumeProperty;

class VTK_VOLUMERENDERING_EXPORT vtkOpenGLVolumeTextureMapper3D : public vtkVolumeTextureMapper3D
{
public:
  vtkTypeMacro(vtkOpenGLVolumeTextureMapper3D,vtkVolumeTextureMapper3D);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkOpenGLVolumeTextureMapper3D *New();

  // Description:
  // Is hardware rendering supported? No if the input data is
  // more than one independent component, or if the hardware does
  // not support the required extensions
  int IsRenderSupported(vtkVolumeProperty *,
                        vtkRenderer *ren);
  
//BTX

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  // Render the volume
  virtual void Render(vtkRenderer *ren, vtkVolume *vol);

//ETX

  // Desciption:
  // Initialize when we go to render, or go to answer the
  // IsRenderSupported question. Don't call unless we have
  // a valid OpenGL context! 
  vtkGetMacro( Initialized, int );
  
  // Description:
  // Release any graphics resources that are being consumed by this texture.
  // The parameter window could be used to determine which graphic
  // resources to release.
  void ReleaseGraphicsResources(vtkWindow *);
  
protected:
  vtkOpenGLVolumeTextureMapper3D();
  ~vtkOpenGLVolumeTextureMapper3D();

//BTX  

  void GetLightInformation(vtkRenderer *ren,
                           vtkVolume *vol,
                           GLfloat lightDirection[2][4],
                           GLfloat lightDiffuseColor[2][4],
                           GLfloat lightSpecularColor[2][4],
                           GLfloat halfwayVector[2][4],
                           GLfloat *ambient );  
//ETX
    
  int              Initialized;
  GLuint           Volume1Index;
  GLuint           Volume2Index;
  GLuint           Volume3Index;
  GLuint           ColorLookupIndex;
  GLuint           AlphaLookupIndex;
  vtkRenderWindow *RenderWindow;
  
  bool SupportsCompressedTexture;
  bool SupportsNonPowerOfTwoTextures;
  
  // Actual internal texture format (uncompressed vs compressed)
  // Computed in Render()
  int InternalAlpha; // GLint
  int InternalLA; // GLint
  int InternalRGB; // GLint
  int InternalRGBA; // GLint
  
  void Initialize(vtkRenderer *r);

  virtual void RenderNV(vtkRenderer *ren, vtkVolume *vol);
  virtual void RenderFP(vtkRenderer *ren, vtkVolume *vol);

  void RenderOneIndependentNoShadeFP( vtkRenderer *ren,
                                      vtkVolume *vol );
  void RenderOneIndependentShadeFP( vtkRenderer *ren, vtkVolume *vol );
  void RenderTwoDependentNoShadeFP( vtkRenderer *ren, vtkVolume *vol );
  void RenderTwoDependentShadeFP( vtkRenderer *ren, vtkVolume *vol );
  void RenderFourDependentNoShadeFP( vtkRenderer *ren, vtkVolume *vol );
  void RenderFourDependentShadeFP( vtkRenderer *ren, vtkVolume *vol );

  void RenderOneIndependentNoShadeNV( vtkRenderer *ren, vtkVolume *vol );
  void RenderOneIndependentShadeNV( vtkRenderer *ren, vtkVolume *vol );
  void RenderTwoDependentNoShadeNV( vtkRenderer *ren, vtkVolume *vol );
  void RenderTwoDependentShadeNV( vtkRenderer *ren, vtkVolume *vol );
  void RenderFourDependentNoShadeNV( vtkRenderer *ren, vtkVolume *vol );
  void RenderFourDependentShadeNV( vtkRenderer *ren, vtkVolume *vol );
  
  void SetupOneIndependentTextures( vtkRenderer *ren, vtkVolume *vol );
  void SetupTwoDependentTextures( vtkRenderer *ren, vtkVolume *vol );
  void SetupFourDependentTextures( vtkRenderer *ren, vtkVolume *vol );

  void SetupRegisterCombinersNoShadeNV( vtkRenderer *ren,
                                        vtkVolume *vol,
                                        int components );

  void SetupRegisterCombinersShadeNV( vtkRenderer *ren,
                                      vtkVolume *vol,
                                      int components );

  void DeleteTextureIndex( GLuint *index );
  void CreateTextureIndex( GLuint *index );
  
  void RenderPolygons( vtkRenderer *ren,
                       vtkVolume *vol,
                       int stages[4] );

  void SetupProgramLocalsForShadingFP( vtkRenderer *ren, vtkVolume *vol );
  
  // Description:
  // Check if we can support this texture size for the number of components.
  int IsTextureSizeSupported(int size[3],
                             int components);

  // Description:
  // Common code for setting up interpolation / clamping on 3D textures
  void Setup3DTextureParameters( vtkVolumeProperty *property );

private:
  vtkOpenGLVolumeTextureMapper3D(const vtkOpenGLVolumeTextureMapper3D&);  // Not implemented.
  void operator=(const vtkOpenGLVolumeTextureMapper3D&);  // Not implemented.
};


#endif



