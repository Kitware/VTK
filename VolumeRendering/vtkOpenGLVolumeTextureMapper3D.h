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
# include "vtkOpenGL.h"
#endif

class vtkRenderWindow;
class vtkVolumeProperty;

#ifndef  GL_REPLACE_EXT
#define  GL_REPLACE_EXT                    0x8062 
#endif
#ifndef  GL_TEXTURE_3D_EXT
#define  GL_TEXTURE_3D_EXT                 0x806F
#endif
#ifndef  GL_PROXY_TEXTURE_3D_EXT
#define  GL_PROXY_TEXTURE_3D_EXT          0x8070
#endif
#ifndef  GL_TEXTURE0_ARB
#define  GL_TEXTURE0_ARB                   0x84C0 
#endif
#ifndef  GL_TEXTURE1_ARB
#define  GL_TEXTURE1_ARB                   0x84C1 
#endif
#ifndef  GL_TEXTURE2_ARB
#define  GL_TEXTURE2_ARB                   0x84C2 
#endif
#ifndef  GL_TEXTURE3_ARB
#define  GL_TEXTURE3_ARB                   0x84C3 
#endif
#ifndef  GL_MAX_TEXTURE_UNITS_ARB 
#define  GL_MAX_TEXTURE_UNITS_ARB          0x84E2
#endif
#ifndef  GL_REGISTER_COMBINERS_NV
#define  GL_REGISTER_COMBINERS_NV          0x8522
#endif
#ifndef  GL_VARIABLE_A_NV
#define  GL_VARIABLE_A_NV                  0x8523 
#endif
#ifndef  GL_VARIABLE_B_NV
#define  GL_VARIABLE_B_NV                  0x8524 
#endif
#ifndef  GL_VARIABLE_C_NV
#define  GL_VARIABLE_C_NV                  0x8525 
#endif
#ifndef  GL_VARIABLE_D_NV
#define  GL_VARIABLE_D_NV                  0x8526 
#endif
#ifndef  GL_VARIABLE_E_NV 
#define  GL_VARIABLE_E_NV                  0x8527 
#endif
#ifndef  GL_VARIABLE_F_NV
#define  GL_VARIABLE_F_NV                  0x8528 
#endif
#ifndef  GL_VARIABLE_G_NV 
#define  GL_VARIABLE_G_NV                  0x8529
#endif
#ifndef  GL_CONSTANT_COLOR0_NV 
#define  GL_CONSTANT_COLOR0_NV             0x852A
#endif
#ifndef  GL_CONSTANT_COLOR1_NV 
#define  GL_CONSTANT_COLOR1_NV             0x852B
#endif
#ifndef  GL_PRIMARY_COLOR_NV 
#define  GL_PRIMARY_COLOR_NV               0x852C
#endif
#ifndef  GL_SPARE0_NV
#define  GL_SPARE0_NV                      0x852D
#endif
#ifndef  GL_SPARE1_NV 
#define  GL_SPARE1_NV                      0x852E
#endif
#ifndef  GL_DISCARD_NV
#define  GL_DISCARD_NV                     0x8530
#endif
#ifndef  GL_SPARE0_PLUS_SECONDARY_COLOR_NV
#define  GL_SPARE0_PLUS_SECONDARY_COLOR_NV 0x8532
#endif
#ifndef  GL_PER_STAGE_CONSTANTS_NV  
#define  GL_PER_STAGE_CONSTANTS_NV         0x8535
#endif
#ifndef  GL_UNSIGNED_IDENTITY_NV 
#define  GL_UNSIGNED_IDENTITY_NV           0x8536 
#endif
#ifndef  GL_EXPAND_NORMAL_NV 
#define  GL_EXPAND_NORMAL_NV               0x8538 
#endif
#ifndef  GL_EXPAND_NEGATE_NV 
#define  GL_EXPAND_NEGATE_NV               0x8539 
#endif
#ifndef  GL_NUM_GENERAL_COMBINERS_NV
#define  GL_NUM_GENERAL_COMBINERS_NV       0x854E
#endif
#ifndef  GL_COLOR_SUM_CLAMP_NV  
#define  GL_COLOR_SUM_CLAMP_NV             0x854F
#endif
#ifndef  GL_COMBINER0_NV 
#define  GL_COMBINER0_NV                   0x8550
#endif
#ifndef  GL_COMBINER1_NV
#define  GL_COMBINER1_NV                   0x8551
#endif
#ifndef  GL_COMBINER2_NV
#define  GL_COMBINER2_NV                   0x8552
#endif
#ifndef  GL_COMBINER3_NV
#define  GL_COMBINER3_NV                   0x8553
#endif
#ifndef  GL_COMBINER4_NV
#define  GL_COMBINER4_NV                   0x8554
#endif
#ifndef  GL_COMBINER5_NV 
#define  GL_COMBINER5_NV                   0x8555
#endif
#ifndef  GL_COMBINER6_NV
#define  GL_COMBINER6_NV                   0x8556
#endif
#ifndef  GL_COMBINER7_NV
#define  GL_COMBINER7_NV                   0x8557
#endif
#ifndef  GL_COMBINE_EXT 
#define  GL_COMBINE_EXT                    0x8570 
#endif
#ifndef  GL_COMBINE_RGB_EXT
#define  GL_COMBINE_RGB_EXT                0x8571
#endif
#ifndef  GL_TEXTURE_SHADER_NV   
#define  GL_TEXTURE_SHADER_NV              0x86DE
#endif
#ifndef  GL_SHADER_OPERATION_NV   
#define  GL_SHADER_OPERATION_NV            0x86DF
#endif
#ifndef  GL_PREVIOUS_TEXTURE_INPUT_NV 
#define  GL_PREVIOUS_TEXTURE_INPUT_NV      0x86E4
#endif
#ifndef  GL_DEPENDENT_AR_TEXTURE_2D_NV 
#define  GL_DEPENDENT_AR_TEXTURE_2D_NV     0x86E9
#endif
#ifndef  GL_DEPENDENT_GB_TEXTURE_2D_NV   
#define  GL_DEPENDENT_GB_TEXTURE_2D_NV     0x86EA
#endif
#ifndef  GL_TEXTURE_3D
#define  GL_TEXTURE_3D                     0x86EF 
#endif
#ifndef  GL_FRAGMENT_PROGRAM_ARB
#define  GL_FRAGMENT_PROGRAM_ARB           0x8804
#endif
#ifndef  GL_PROGRAM_FORMAT_ASCII_ARB
#define  GL_PROGRAM_FORMAT_ASCII_ARB       0x8875
#endif

typedef void (APIENTRY *PFNGLTEX3DEXT) 
  (GLenum target, GLint level, GLenum internalformat,
   GLsizei width, GLsizei height, GLsizei depth, GLint border, 
   GLenum format, GLenum type, const void* pixels);

typedef void (APIENTRY *PFNGLMULTITEXCOORD3FVARB)(GLenum texture, float *coords);
typedef void (APIENTRY *PFNGLACTIVETEXTUREARB)(GLenum texture);
typedef void (APIENTRY *PFNGLCOMBINERPARAMETERINV)(GLenum pname, const GLint params);
typedef void (APIENTRY *PFNGLCOMBINERSTAGEPARAMETERFVNV)(GLenum stage, GLenum pname, const GLfloat *params);
typedef void (APIENTRY *PFNGLCOMBINERINPUTNV)(GLenum stage, GLenum portion,
                                              GLenum variable, GLenum input,
                                              GLenum mapping, GLenum componentUsage);
typedef void (APIENTRY *PFNGLCOMBINEROUTPUTNV)(GLenum stage, GLenum portion,
                                              GLenum abOutput, GLenum cdOutput,
                                              GLenum sumOutput, GLenum scale,
                                              GLenum bias, GLenum abDotProduct,
                                              GLenum cdDotProduct, GLenum muxSum);
typedef void (APIENTRY *PFNGLFINALCOMBINERINPUTNV)(GLenum variable, GLenum input,
                                                   GLenum mapping, GLenum componentUsage);

typedef void (APIENTRY *PFNGLGENPROGRAMSARB)(GLsizei n, GLuint *programs);
typedef void (APIENTRY *PFNGLDELETEPROGRAMSARB)(GLsizei n, const GLuint *programs);
typedef void (APIENTRY *PFNGLBINDPROGRAMARB)(GLenum target, GLuint program);
typedef void (APIENTRY *PFNGLPROGRAMSTRINGARB)(GLenum target, GLenum format, 
                 GLsizei len, const void *string );
typedef void (APIENTRY *PFNGLPROGRAMLOCALPARAMETER4FARB)(GLenum target, GLuint index,
               GLfloat x, GLfloat y, GLfloat z, GLfloat w );

typedef void (*KWVTMFuncPtr)();

class VTK_VOLUMERENDERING_EXPORT vtkOpenGLVolumeTextureMapper3D : public vtkVolumeTextureMapper3D
{
public:
  vtkTypeRevisionMacro(vtkOpenGLVolumeTextureMapper3D,vtkVolumeTextureMapper3D);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkOpenGLVolumeTextureMapper3D *New();

  // Description:
  // Is hardware rendering supported? No if the input data is
  // more than one independent component, or if the hardware does
  // not support the required extensions
  int IsRenderSupported(vtkVolumeProperty *);
  
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
  int IsExtensionSupported(const char *extension);

  void vtkOpenGLVolumeTextureMapper3D::GetLightInformation( vtkRenderer *ren,
                                                              vtkVolume *vol,
                                                              GLfloat lightDirection[2][4],
                                                              GLfloat lightDiffuseColor[2][4],
                                                              GLfloat lightSpecularColor[2][4],
                                                              GLfloat halfwayVector[2][4],
                                                              GLfloat *ambient );
  KWVTMFuncPtr GetProcAddress(char *);
  
//ETX
    
  int              Initialized;
  GLuint           Volume1Index;
  GLuint           Volume2Index;
  GLuint           Volume3Index;
  GLuint           ColorLookupIndex;
  GLuint           AlphaLookupIndex;
  vtkRenderWindow *RenderWindow;
  
  void Initialize();

  virtual void RenderNV(vtkRenderer *ren, vtkVolume *vol);
  virtual void RenderFP(vtkRenderer *ren, vtkVolume *vol);

  void RenderOneIndependentNoShadeFP( vtkRenderer *ren,
                                      vtkVolume *vol );
  void RenderOneIndependentShadeFP( vtkRenderer *ren,
            vtkVolume *vol );
  void RenderTwoDependentNoShadeFP( vtkRenderer *ren,
            vtkVolume *vol );
  void RenderTwoDependentShadeFP( vtkRenderer *ren,
          vtkVolume *vol );
  void RenderFourDependentNoShadeFP( vtkRenderer *ren,
             vtkVolume *vol );
  void RenderFourDependentShadeFP( vtkRenderer *ren,
           vtkVolume *vol );

  void RenderOneIndependentNoShadeNV( vtkRenderer *ren,
                                      vtkVolume *vol );
  void RenderOneIndependentShadeNV( vtkRenderer *ren,
                                    vtkVolume *vol );
  void RenderTwoDependentNoShadeNV( vtkRenderer *ren,
                                    vtkVolume *vol );
  void RenderTwoDependentShadeNV( vtkRenderer *ren,
                                  vtkVolume *vol );
  void RenderFourDependentNoShadeNV( vtkRenderer *ren,
                                     vtkVolume *vol );
  void RenderFourDependentShadeNV( vtkRenderer *ren,
                                   vtkVolume *vol );
  
  void SetupOneIndependentTextures( vtkRenderer *ren,
            vtkVolume *vol );
  void SetupTwoDependentTextures( vtkRenderer *ren,
          vtkVolume *vol );
  void SetupFourDependentTextures( vtkRenderer *ren,
           vtkVolume *vol );

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

  void SetupProgramLocalsForShadingFP( vtkRenderer *ren,
               vtkVolume *vol );
                        
  PFNGLTEX3DEXT                    glTexImage3DEXT;
  PFNGLACTIVETEXTUREARB            glActiveTextureARB;
  PFNGLMULTITEXCOORD3FVARB         glMultiTexCoord3fvARB;
  PFNGLCOMBINERPARAMETERINV        glCombinerParameteriNV;
  PFNGLCOMBINERSTAGEPARAMETERFVNV  glCombinerStageParameterfvNV;
  PFNGLCOMBINERINPUTNV             glCombinerInputNV;
  PFNGLCOMBINEROUTPUTNV            glCombinerOutputNV;
  PFNGLFINALCOMBINERINPUTNV        glFinalCombinerInputNV;
  PFNGLGENPROGRAMSARB              glGenProgramsARB;
  PFNGLPROGRAMLOCALPARAMETER4FARB  glProgramLocalParameter4fARB;
  PFNGLDELETEPROGRAMSARB           glDeleteProgramsARB;
  PFNGLBINDPROGRAMARB              glBindProgramARB;
  PFNGLPROGRAMSTRINGARB            glProgramStringARB;
  
  // Description:
  // Check if we can support this texture size.
  int IsTextureSizeSupported( int size[3] );

  // Description:
  // Common code for setting up interpolation / clamping on 3D textures
  void Setup3DTextureParameters( vtkVolumeProperty *property );

private:
  vtkOpenGLVolumeTextureMapper3D(const vtkOpenGLVolumeTextureMapper3D&);  // Not implemented.
  void operator=(const vtkOpenGLVolumeTextureMapper3D&);  // Not implemented.
};


#endif



