/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLVolumeTextureMapper3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkWindows.h"
#include "vtkOpenGLVolumeTextureMapper3D.h"

#include "vtkImageData.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPlaneCollection.h"
#include "vtkPointData.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkTimerLog.h"
#include "vtkVolumeProperty.h"
#include "vtkTransform.h"
#include "vtkLightCollection.h"
#include "vtkLight.h"
#include "vtkCamera.h"
#include "vtkMath.h"

#include "vtkVolumeTextureMapper3D_OneComponentShadeFP.h"
#include "vtkVolumeTextureMapper3D_OneComponentNoShadeFP.h"
#include "vtkVolumeTextureMapper3D_TwoDependentNoShadeFP.h"
#include "vtkVolumeTextureMapper3D_TwoDependentShadeFP.h"
#include "vtkVolumeTextureMapper3D_FourDependentNoShadeFP.h"
#include "vtkVolumeTextureMapper3D_FourDependentShadeFP.h"

extern "C" void (*glXGetProcAddressARB(const GLubyte *procName))( void );

#ifdef _WIN32

#endif

#define PrintError(S)                                                           \
  {                                                                             \
  GLenum errorCode;                                                             \
  if ( (errorCode = glGetError()) != GL_NO_ERROR )                              \
    {                                                                           \
      cout << S << endl;                                                        \
    cout << "ERROR" << endl;                                                    \
    switch (errorCode)                                                          \
      {                                                                         \
      case GL_INVALID_ENUM: cout << "invalid enum" << endl; break;              \
      case GL_INVALID_VALUE: cout << "invalid value" << endl; break;            \
      case GL_INVALID_OPERATION: cout << "invalid operation" << endl; break;    \
      case GL_STACK_OVERFLOW: cout << "stack overflow" << endl; break;          \
      case GL_STACK_UNDERFLOW: cout << "stack underflow" << endl; break;        \
      case GL_OUT_OF_MEMORY: cout << "out of memory" << endl; break;            \
      default: cout << "unknown error" << endl;                                 \
      }                                                                         \
    }}

//#ifndef VTK_IMPLEMENT_MESA_CXX
vtkCxxRevisionMacro(vtkOpenGLVolumeTextureMapper3D, "1.3");
vtkStandardNewMacro(vtkOpenGLVolumeTextureMapper3D);
//#endif



vtkOpenGLVolumeTextureMapper3D::vtkOpenGLVolumeTextureMapper3D()
{
  this->Initialized                  =  0;
  this->Volume1Index                 =  0;
  this->Volume2Index                 =  0;
  this->Volume3Index                 =  0;
  this->ColorLookupIndex             =  0;
  this->RenderWindow                 = NULL;

  this->glTexImage3DEXT              = NULL;
  this->glActiveTextureARB           = NULL;
  this->glMultiTexCoord3fvARB        = NULL;  
  this->glCombinerParameteriNV       = NULL;
  this->glCombinerStageParameterfvNV = NULL;
  this->glCombinerInputNV            = NULL;
  this->glCombinerOutputNV           = NULL;
  this->glFinalCombinerInputNV       = NULL;
  this->glGenProgramsARB             = NULL;
  this->glDeleteProgramsARB          = NULL;
  this->glBindProgramARB             = NULL;
  this->glProgramStringARB           = NULL;
  this->glProgramLocalParameter4fARB = NULL;
}

vtkOpenGLVolumeTextureMapper3D::~vtkOpenGLVolumeTextureMapper3D()
{
}

KWVTMFuncPtr vtkOpenGLVolumeTextureMapper3D::GetProcAddress(char *name)
{
#ifdef _WIN32
  KWVTMFuncPtr t = (KWVTMFuncPtr)wglGetProcAddress(name);
  return t;
#else
  KWVTMFuncPtr t = (KWVTMFuncPtr)glXGetProcAddressARB((const GLubyte*)name);
  return t;
#endif
}

// Release the graphics resources used by this texture.  
void vtkOpenGLVolumeTextureMapper3D::ReleaseGraphicsResources(vtkWindow 
                                                                *renWin)
{
  if (( this->Volume1Index || this->Volume2Index || 
        this->Volume3Index || this->ColorLookupIndex) && renWin)
    {
    ((vtkRenderWindow *) renWin)->MakeCurrent();
#ifdef GL_VERSION_1_1
    // free any textures
    this->DeleteTextureIndex( &this->Volume1Index );
    this->DeleteTextureIndex( &this->Volume2Index );
    this->DeleteTextureIndex( &this->Volume3Index );
    this->DeleteTextureIndex( &this->ColorLookupIndex );
    this->DeleteTextureIndex( &this->AlphaLookupIndex );    
#endif
    }
  this->Volume1Index     = 0;
  this->Volume2Index     = 0;
  this->Volume3Index     = 0;
  this->ColorLookupIndex = 0;
  this->RenderWindow     = NULL;
  this->Modified();
}

void vtkOpenGLVolumeTextureMapper3D::Render(vtkRenderer *ren, vtkVolume *vol)
{  

  ren->GetRenderWindow()->MakeCurrent();
  
  if ( !this->Initialized )
    {
    this->Initialize();
    }
  
  if ( this->RenderMethod == vtkVolumeTextureMapper3D::NO_METHOD )
    {
    cout << "required extensions not supported" << endl;
    return;
    }

    
  vtkMatrix4x4       *matrix = vtkMatrix4x4::New();
  vtkPlaneCollection *clipPlanes;
  vtkPlane           *plane;
  int                numClipPlanes = 0;
  double             planeEquation[4];


  // build transformation 
  vol->GetMatrix(matrix);
  matrix->Transpose();

  glPushAttrib(GL_ENABLE_BIT   | 
               GL_COLOR_BUFFER_BIT   |
               GL_STENCIL_BUFFER_BIT |
               GL_DEPTH_BUFFER_BIT   | 
               GL_POLYGON_BIT        | 
               GL_TEXTURE_BIT);
  
  int i;
  
  // Use the OpenGL clip planes
  clipPlanes = this->ClippingPlanes;
  if ( clipPlanes )
    {
    numClipPlanes = clipPlanes->GetNumberOfItems();
    if (numClipPlanes > 6)
      {
      vtkErrorMacro(<< "OpenGL guarantees only 6 additional clipping planes");
      }

    for (i = 0; i < numClipPlanes; i++)
      {
      glEnable((GLenum)(GL_CLIP_PLANE0+i));

      plane = (vtkPlane *)clipPlanes->GetItemAsObject(i);

      planeEquation[0] = plane->GetNormal()[0]; 
      planeEquation[1] = plane->GetNormal()[1]; 
      planeEquation[2] = plane->GetNormal()[2];
      planeEquation[3] = -(planeEquation[0]*plane->GetOrigin()[0]+
                           planeEquation[1]*plane->GetOrigin()[1]+
                           planeEquation[2]*plane->GetOrigin()[2]);
      glClipPlane((GLenum)(GL_CLIP_PLANE0+i),planeEquation);
      }
    }


  
  // insert model transformation 
  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();
  glMultMatrixd(matrix->Element[0]);

  glColor4f( 1.0, 1.0, 1.0, 1.0 );

  // Turn lighting off - the polygon textures already have illumination
  glDisable( GL_LIGHTING );

  switch ( this->RenderMethod )
    {
    case vtkVolumeTextureMapper3D::NVIDIA_METHOD:
      this->RenderNV(ren,vol);
      break;
    case vtkVolumeTextureMapper3D::FRAGMENT_PROGRAM_METHOD:
      this->RenderFP(ren,vol);
      break;
    }

  // pop transformation matrix
  glMatrixMode( GL_MODELVIEW );
  glPopMatrix();

  matrix->Delete();
  glPopAttrib();


  glFlush();
  glFinish();
  
  
  this->Timer->StopTimer();      

  this->TimeToDraw = (float)this->Timer->GetElapsedTime();

  // If the timer is not accurate enough, set it to a small
  // time so that it is not zero
  if ( this->TimeToDraw == 0.0 )
    {
    this->TimeToDraw = 0.0001;
    }   
}

void vtkOpenGLVolumeTextureMapper3D::RenderFP( vtkRenderer *ren, vtkVolume *vol )
{
  glAlphaFunc (GL_GREATER, (GLclampf) 0);
  glEnable (GL_ALPHA_TEST);
  
  glEnable( GL_BLEND );
  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

  int components = this->GetInput()->GetNumberOfScalarComponents();   
  switch ( components )
    {
    case 1:
      if ( !vol->GetProperty()->GetShade() )
        {
        this->RenderOneIndependentNoShadeFP(ren,vol);
        }
      else
        {
  this->RenderOneIndependentShadeFP(ren,vol);
        }
      break;
      
    case 2:
      if ( !vol->GetProperty()->GetShade() )
        {
  this->RenderTwoDependentNoShadeFP(ren,vol);
        }
      else
        {
  this->RenderTwoDependentShadeFP(ren,vol);
        }
      break;
      
    case 3:
    case 4:
      if ( !vol->GetProperty()->GetShade() )
        {
  this->RenderFourDependentNoShadeFP(ren,vol);
        }
      else
        {
  this->RenderFourDependentShadeFP(ren,vol);
        }
    }
  
  glActiveTextureARB( GL_TEXTURE2_ARB );
  glDisable( GL_TEXTURE_2D );
  glDisable( GL_TEXTURE_3D_EXT );
  
  glActiveTextureARB( GL_TEXTURE1_ARB );
  glDisable( GL_TEXTURE_2D );
  glDisable( GL_TEXTURE_3D_EXT );
  
  glActiveTextureARB( GL_TEXTURE0_ARB );
  glDisable( GL_TEXTURE_2D );
  glDisable( GL_TEXTURE_3D_EXT );  
}

void vtkOpenGLVolumeTextureMapper3D::RenderNV( vtkRenderer *ren, vtkVolume *vol )
{
  glAlphaFunc (GL_GREATER, (GLclampf) 0);
  glEnable (GL_ALPHA_TEST);
  
  glEnable( GL_BLEND );
  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

  int components = this->GetInput()->GetNumberOfScalarComponents();   
  switch ( components )
    {
    case 1:
      if ( !vol->GetProperty()->GetShade() )
        {
        this->RenderOneIndependentNoShadeNV(ren,vol);
        }
      else
        {
        this->RenderOneIndependentShadeNV(ren,vol);
        }
      break;
      
    case 2:
      if ( !vol->GetProperty()->GetShade() )
        {
        this->RenderTwoDependentNoShadeNV(ren,vol);
        }
      else
        {
        this->RenderTwoDependentShadeNV(ren,vol);
        }
      break;
      
    case 3:
    case 4:
      if ( !vol->GetProperty()->GetShade() )
        {
        this->RenderFourDependentNoShadeNV(ren,vol);
        }
      else
        {
        this->RenderFourDependentShadeNV(ren,vol);
        }
    }
  
  glActiveTextureARB( GL_TEXTURE2_ARB );
  glDisable( GL_TEXTURE_2D );
  glDisable( GL_TEXTURE_3D_EXT );
  
  glActiveTextureARB( GL_TEXTURE1_ARB );
  glDisable( GL_TEXTURE_2D );
  glDisable( GL_TEXTURE_3D_EXT );
  
  glActiveTextureARB( GL_TEXTURE0_ARB );
  glDisable( GL_TEXTURE_2D );
  glDisable( GL_TEXTURE_3D_EXT );
  
  glDisable( GL_TEXTURE_SHADER_NV );

  glDisable(GL_REGISTER_COMBINERS_NV);    
}

void vtkOpenGLVolumeTextureMapper3D::DeleteTextureIndex( GLuint *index )
{
  if (glIsTexture(*index))
    {
    GLuint tempIndex;
    tempIndex = *index;
    glDeleteTextures(1, &tempIndex);
    *index = 0;
    }
}

void vtkOpenGLVolumeTextureMapper3D::CreateTextureIndex( GLuint *index )
{
  GLuint tempIndex=0;    
  glGenTextures(1, &tempIndex);
  *index = (long) tempIndex;
}

void vtkOpenGLVolumeTextureMapper3D::RenderPolygons( vtkRenderer *ren,
                                                       vtkVolume *vol,
                                                       int stages[4] )
{
  vtkRenderWindow *renWin = ren->GetRenderWindow();
  
  if ( renWin->CheckAbortStatus() )
    {
    return;
    }
  
  double bounds[27][6];
  float distance2[27];
  
  int   numIterations;
  int i, j, k;
  
  // No cropping case - render the whole thing
  if ( !this->Cropping )
    {
    // Use the input data bounds - we'll take care of the volume's
    // matrix during rendering
    this->GetInput()->GetBounds(bounds[0]);
    numIterations = 1;
    }
  // Simple cropping case - render the subvolume
  else if ( this->CroppingRegionFlags == 0x2000 )
    {
    this->GetCroppingRegionPlanes(bounds[0]);
    numIterations = 1;
    }
  // Complex cropping case - render each region in back-to-front order
  else
    {
    // Get the camera position
    double camPos[4];
    ren->GetActiveCamera()->GetPosition(camPos);
    
    double volBounds[6];
    this->GetInput()->GetBounds(volBounds);
    
    // Pass camera through inverse volume matrix
    // so that we are in the same coordinate system
    vtkMatrix4x4 *volMatrix = vtkMatrix4x4::New();
    vol->GetMatrix( volMatrix );
    camPos[3] = 1.0;
    volMatrix->Invert();
    volMatrix->MultiplyPoint( camPos, camPos );
    volMatrix->Delete();
    if ( camPos[3] )
      {
      camPos[0] /= camPos[3];
      camPos[1] /= camPos[3];
      camPos[2] /= camPos[3];
      }
    
    // These are the region limits for x (first four), y (next four) and
    // z (last four). The first region limit is the lower bound for
    // that axis, the next two are the region planes along that axis, and
    // the final one in the upper bound for that axis.
    float limit[12];    
    for ( i = 0; i < 3; i++ )
      {
      limit[i*4  ] = volBounds[i*2];
      limit[i*4+1] = this->CroppingRegionPlanes[i*2];
      limit[i*4+2] = this->CroppingRegionPlanes[i*2+1];
      limit[i*4+3] = volBounds[i*2+1];
      }
    
    // For each of the 27 possible regions, find out if it is enabled,
    // and if so, compute the bounds and the distance from the camera
    // to the center of the region.
    int numRegions = 0;
    int region;
    for ( region = 0; region < 27; region++ )
      {
      int regionFlag = 1<<region;
      
      if ( this->CroppingRegionFlags & regionFlag )
        {
        // what is the coordinate in the 3x3x3 grid
        int loc[3];
        loc[0] = region%3;
        loc[1] = (region/3)%3;
        loc[2] = (region/9)%3;

        // compute the bounds and center
        float center[3];
        for ( i = 0; i < 3; i++ )
          {
          bounds[numRegions][i*2  ] = limit[4*i+loc[i]];
          bounds[numRegions][i*2+1] = limit[4*i+loc[i]+1];
          center[i] = 
            (bounds[numRegions][i*2  ] + 
             bounds[numRegions][i*2+1])/2.0;
          }
        
        // compute the distance squared to the center
        distance2[numRegions] = 
          (camPos[0]-center[0])*(camPos[0]-center[0]) +
          (camPos[1]-center[1])*(camPos[1]-center[1]) +
          (camPos[2]-center[2])*(camPos[2]-center[2]);
        
        // we've added one region
        numRegions++;
        }
      }
    
    // Do a quick bubble sort on distance
    for ( i = 1; i < numRegions; i++ )
      {
      for ( j = i; j > 0 && distance2[j] > distance2[j-1]; j-- )
        {
        float tmpBounds[6];
        float tmpDistance2;
        
        for ( k = 0; k < 6; k++ )
          {
          tmpBounds[k] = bounds[j][k];
          }
        tmpDistance2 = distance2[j];

        for ( k = 0; k < 6; k++ )
          {
          bounds[j][k] = bounds[j-1][k];
          }
        distance2[j] = distance2[j-1];

        for ( k = 0; k < 6; k++ )
          {
          bounds[j-1][k] = tmpBounds[k];
          }
        distance2[j-1] = tmpDistance2;
        
        }
      }
    
    numIterations = numRegions;
    }

  // loop over all regions we need to render
  for ( int loop = 0; 
        loop < numIterations;
        loop++ )
    {
    // Compute the set of polygons for this region 
    // according to the bounds
    this->ComputePolygons( ren, vol, bounds[loop] );

    // Loop over the polygons
    for ( i = 0; i < this->NumberOfPolygons; i++ )
      {
      if ( i%64 == 1 )
        {
        glFlush();
        glFinish();
        }
      
      if ( renWin->CheckAbortStatus() )
        {
        return;
        }
      
      float *ptr = this->PolygonBuffer + 36*i;
      
      glBegin( GL_TRIANGLE_FAN );
      
      for ( j = 0; j < 6; j++ )
        {
        if ( ptr[0] < 0.0 )
          {
          break;
          }

        for ( k = 0; k < 4; k++ )
          {
          if ( stages[k] )
            {
            this->glMultiTexCoord3fvARB( GL_TEXTURE0_ARB + k, ptr );
            }
          }
        glVertex3fv( ptr+3 );
        
        ptr += 6;
        }
      glEnd();         
      }
    }
}

void vtkOpenGLVolumeTextureMapper3D::Setup3DTextureParameters( vtkVolumeProperty *property )
{
  if ( property->GetInterpolationType() == VTK_NEAREST_INTERPOLATION )
    {
    glTexParameterf( GL_TEXTURE_3D_EXT, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameterf( GL_TEXTURE_3D_EXT, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    }
  else
    {
    glTexParameterf( GL_TEXTURE_3D_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameterf( GL_TEXTURE_3D_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    }
  glTexParameterf( GL_TEXTURE_3D_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP );
  glTexParameterf( GL_TEXTURE_3D_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP );
}

void vtkOpenGLVolumeTextureMapper3D::SetupOneIndependentTextures( vtkRenderer *vtkNotUsed(ren),
                    vtkVolume *vol )
{
  this->glActiveTextureARB( GL_TEXTURE0_ARB );
  glDisable( GL_TEXTURE_2D );
  glEnable( GL_TEXTURE_3D_EXT );
  if ( this->RenderMethod == vtkVolumeTextureMapper3D::NVIDIA_METHOD )
    {
    glEnable( GL_TEXTURE_SHADER_NV );
    glTexEnvi(GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, GL_TEXTURE_3D_EXT);
    }

  this->glActiveTextureARB( GL_TEXTURE2_ARB );
  glDisable( GL_TEXTURE_2D );
  glEnable( GL_TEXTURE_3D_EXT );
  if ( this->RenderMethod == vtkVolumeTextureMapper3D::NVIDIA_METHOD )
    {
    glEnable( GL_TEXTURE_SHADER_NV );  
    glTexEnvi(GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, GL_TEXTURE_3D_EXT);
    }

  // Update the volume containing the 2 byte scalar / gradient magnitude
  if ( this->UpdateVolumes( vol ) || !this->Volume1Index || !this->Volume2Index )
    {    
    int dim[3];
    this->GetVolumeDimensions(dim);
    this->DeleteTextureIndex(&this->Volume3Index);
    
    this->glActiveTextureARB( GL_TEXTURE0_ARB );
    glBindTexture(GL_TEXTURE_3D_EXT, (GLuint)NULL);
    this->DeleteTextureIndex(&this->Volume1Index);
    this->CreateTextureIndex(&this->Volume1Index);
    glBindTexture(GL_TEXTURE_3D_EXT, this->Volume1Index);
    this->glTexImage3DEXT( GL_TEXTURE_3D_EXT, 0, GL_LUMINANCE8_ALPHA8, dim[0], dim[1], dim[2], 0,
                           GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, this->Volume1 );
    

    this->glActiveTextureARB( GL_TEXTURE2_ARB );
    glBindTexture(GL_TEXTURE_3D_EXT, (GLuint)NULL);
    this->DeleteTextureIndex(&this->Volume2Index);
    this->CreateTextureIndex(&this->Volume2Index);
    glBindTexture(GL_TEXTURE_3D_EXT, this->Volume2Index);
    this->glTexImage3DEXT( GL_TEXTURE_3D_EXT, 0, GL_RGBA8, dim[0], dim[1], dim[2], 0,
                           GL_RGB, GL_UNSIGNED_BYTE, this->Volume2 );
    }
  
  this->glActiveTextureARB( GL_TEXTURE0_ARB );
  glBindTexture(GL_TEXTURE_3D_EXT, this->Volume1Index);   
  this->Setup3DTextureParameters( vol->GetProperty() );

  this->glActiveTextureARB( GL_TEXTURE2_ARB );
  glBindTexture(GL_TEXTURE_3D_EXT, this->Volume2Index);   
  this->Setup3DTextureParameters( vol->GetProperty() );

  this->glActiveTextureARB( GL_TEXTURE1_ARB );
  glEnable( GL_TEXTURE_2D );
  glDisable( GL_TEXTURE_3D_EXT );
  if ( this->RenderMethod == vtkVolumeTextureMapper3D::NVIDIA_METHOD )
    {
    glTexEnvf ( GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, 
                GL_DEPENDENT_AR_TEXTURE_2D_NV );  
    glTexEnvi(GL_TEXTURE_SHADER_NV, GL_PREVIOUS_TEXTURE_INPUT_NV, GL_TEXTURE0_ARB);
    }

  // Update the dependent 2D color table mapping scalar value and
  // gradient magnitude to RGBA
  if ( this->UpdateColorLookup( vol ) || !this->ColorLookupIndex )
    {
    this->DeleteTextureIndex( &this->ColorLookupIndex );
    this->DeleteTextureIndex( &this->AlphaLookupIndex );
    
    this->CreateTextureIndex( &this->ColorLookupIndex );
    glBindTexture(GL_TEXTURE_2D, this->ColorLookupIndex);   

    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );    
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );

    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, 256, 256, 0,
                  GL_RGBA, GL_UNSIGNED_BYTE, this->ColorLookup );    
    }
  
  glBindTexture(GL_TEXTURE_2D, this->ColorLookupIndex);
}

void vtkOpenGLVolumeTextureMapper3D::SetupRegisterCombinersNoShadeNV( vtkRenderer *vtkNotUsed(ren),
                  vtkVolume *vtkNotUsed(vol), 
                  int components )
{
  if ( components < 3 )
    {
    this->glActiveTextureARB(GL_TEXTURE2_ARB);
    glTexEnvi(GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, GL_NONE);
  
    if ( components == 1 )
      {
      this->glActiveTextureARB(GL_TEXTURE3_ARB);
      glTexEnvi(GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, GL_NONE);
      }
    }
  
  
  glEnable(GL_REGISTER_COMBINERS_NV);    
  this->glCombinerParameteriNV(GL_NUM_GENERAL_COMBINERS_NV, 1);
  this->glCombinerParameteriNV(GL_COLOR_SUM_CLAMP_NV, GL_TRUE);
    
  this->glFinalCombinerInputNV(GL_VARIABLE_A_NV, GL_ZERO,         GL_UNSIGNED_IDENTITY_NV, GL_RGB  );
  this->glFinalCombinerInputNV(GL_VARIABLE_B_NV, GL_ZERO,         GL_UNSIGNED_IDENTITY_NV, GL_RGB  );
  this->glFinalCombinerInputNV(GL_VARIABLE_C_NV, GL_ZERO,         GL_UNSIGNED_IDENTITY_NV, GL_RGB  );
  if ( components < 3 )
    {
    this->glFinalCombinerInputNV(GL_VARIABLE_D_NV, GL_TEXTURE1_ARB, GL_UNSIGNED_IDENTITY_NV, GL_RGB  );
    }
  else
    {
    this->glFinalCombinerInputNV(GL_VARIABLE_D_NV, GL_TEXTURE0_ARB, GL_UNSIGNED_IDENTITY_NV, GL_RGB  );
    }
  
  if ( components == 1 )
    {
    this->glFinalCombinerInputNV(GL_VARIABLE_G_NV, GL_TEXTURE1_ARB, GL_UNSIGNED_IDENTITY_NV, GL_ALPHA);
    }
  else
    {
    this->glFinalCombinerInputNV(GL_VARIABLE_G_NV, GL_TEXTURE3_ARB, GL_UNSIGNED_IDENTITY_NV, GL_ALPHA);
    }
}

void vtkOpenGLVolumeTextureMapper3D::SetupRegisterCombinersShadeNV( vtkRenderer *ren,
                      vtkVolume *vol,
                      int components )
{
  if ( components == 1 )
    {
    this->glActiveTextureARB(GL_TEXTURE3_ARB);
    glTexEnvi(GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, GL_NONE);
    }
    
  GLfloat white[4] = {1,1,1,1};
    
  GLfloat lightDirection[2][4];
  GLfloat lightDiffuseColor[2][4];
  GLfloat lightSpecularColor[2][4];
  GLfloat halfwayVector[2][4];
  GLfloat ambientColor[4];

  // Gather information about the light sources. Although we gather info for multiple light sources,
  // in this case we will only use the first one, and will duplicate it (in opposite direction) to
  // approximate two-sided lighting.
  this->GetLightInformation( ren, vol, lightDirection, lightDiffuseColor, 
                             lightSpecularColor, halfwayVector, ambientColor );
  
  float specularPower = vol->GetProperty()->GetSpecularPower();
  
  glEnable(GL_REGISTER_COMBINERS_NV);    
  glEnable( GL_PER_STAGE_CONSTANTS_NV );
  this->glCombinerParameteriNV(GL_NUM_GENERAL_COMBINERS_NV, 8);
  this->glCombinerParameteriNV(GL_COLOR_SUM_CLAMP_NV, GL_TRUE);
  
  // Stage 0
  //
  //  N dot L is computed into GL_SPARE0_NV
  // -N dot L is computed into GL_SPARE1_NV
  //
  this->glCombinerStageParameterfvNV( GL_COMBINER0_NV, GL_CONSTANT_COLOR0_NV, lightDirection[0] );
  
  this->glCombinerInputNV( GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_A_NV, GL_CONSTANT_COLOR0_NV, GL_EXPAND_NORMAL_NV, GL_RGB );
  this->glCombinerInputNV( GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_B_NV, GL_TEXTURE2_ARB,       GL_EXPAND_NORMAL_NV, GL_RGB );
  this->glCombinerInputNV( GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_C_NV, GL_CONSTANT_COLOR0_NV, GL_EXPAND_NORMAL_NV, GL_RGB );
  this->glCombinerInputNV( GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_D_NV, GL_TEXTURE2_ARB,       GL_EXPAND_NEGATE_NV, GL_RGB );
  
  this->glCombinerOutputNV( GL_COMBINER0_NV, GL_RGB, GL_SPARE0_NV, GL_SPARE1_NV, GL_DISCARD_NV, 
                            GL_NONE, GL_NONE, GL_TRUE, GL_TRUE, GL_FALSE );
  
  // Stage 1
  //
  // lightColor * max( 0, (N dot L)) + lightColor * max( 0, (-N dot L)) is computed into GL_SPARE0_NV
  // 
  this->glCombinerStageParameterfvNV( GL_COMBINER1_NV, GL_CONSTANT_COLOR0_NV, lightDiffuseColor[0] );
  
  this->glCombinerInputNV( GL_COMBINER1_NV, GL_RGB, GL_VARIABLE_A_NV, GL_SPARE0_NV,          GL_UNSIGNED_IDENTITY_NV, GL_RGB );
  this->glCombinerInputNV( GL_COMBINER1_NV, GL_RGB, GL_VARIABLE_B_NV, GL_CONSTANT_COLOR0_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB );
  this->glCombinerInputNV( GL_COMBINER1_NV, GL_RGB, GL_VARIABLE_C_NV, GL_SPARE1_NV,          GL_UNSIGNED_IDENTITY_NV, GL_RGB );
  this->glCombinerInputNV( GL_COMBINER1_NV, GL_RGB, GL_VARIABLE_D_NV, GL_CONSTANT_COLOR0_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB );
  
  this->glCombinerOutputNV( GL_COMBINER1_NV, GL_RGB, GL_DISCARD_NV, GL_DISCARD_NV, GL_SPARE0_NV, 
                            GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE );
  
  // Stage 2
  //
  // result from Stage 1 is added to the ambient color and stored in GL_PRIMARY_COLOR_NV
  //
  this->glCombinerStageParameterfvNV( GL_COMBINER2_NV, GL_CONSTANT_COLOR0_NV, white );
  this->glCombinerStageParameterfvNV( GL_COMBINER2_NV, GL_CONSTANT_COLOR1_NV, ambientColor );    
  
  this->glCombinerInputNV( GL_COMBINER2_NV, GL_RGB, GL_VARIABLE_A_NV, GL_SPARE0_NV,          GL_UNSIGNED_IDENTITY_NV, GL_RGB );
  this->glCombinerInputNV( GL_COMBINER2_NV, GL_RGB, GL_VARIABLE_B_NV, GL_CONSTANT_COLOR0_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB );
  this->glCombinerInputNV( GL_COMBINER2_NV, GL_RGB, GL_VARIABLE_C_NV, GL_CONSTANT_COLOR0_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB );
  this->glCombinerInputNV( GL_COMBINER2_NV, GL_RGB, GL_VARIABLE_D_NV, GL_CONSTANT_COLOR1_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB );
  
  this->glCombinerOutputNV( GL_COMBINER2_NV, GL_RGB, GL_DISCARD_NV, GL_DISCARD_NV, GL_PRIMARY_COLOR_NV, 
                            GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE );
  
  // Stage 3
  // 
  //  N dot H is computed into GL_SPARE0_NV
  // -N dot H is computed into GL_SPARE1_NV
  //
  this->glCombinerStageParameterfvNV( GL_COMBINER3_NV, GL_CONSTANT_COLOR0_NV, halfwayVector[0] );
  
  this->glCombinerInputNV( GL_COMBINER3_NV, GL_RGB, GL_VARIABLE_A_NV, GL_CONSTANT_COLOR0_NV, GL_EXPAND_NORMAL_NV, GL_RGB );
  this->glCombinerInputNV( GL_COMBINER3_NV, GL_RGB, GL_VARIABLE_B_NV, GL_TEXTURE2_ARB,       GL_EXPAND_NORMAL_NV, GL_RGB );
  this->glCombinerInputNV( GL_COMBINER3_NV, GL_RGB, GL_VARIABLE_C_NV, GL_CONSTANT_COLOR0_NV, GL_EXPAND_NORMAL_NV, GL_RGB );
  this->glCombinerInputNV( GL_COMBINER3_NV, GL_RGB, GL_VARIABLE_D_NV, GL_TEXTURE2_ARB,       GL_EXPAND_NEGATE_NV, GL_RGB );
  
  this->glCombinerOutputNV( GL_COMBINER3_NV, GL_RGB, GL_SPARE0_NV, GL_SPARE1_NV, GL_DISCARD_NV, 
                            GL_NONE, GL_NONE, GL_TRUE, GL_TRUE, GL_FALSE );
  
  // Stage 4
  //
  // if the specular power is greater than 1, then
  //
  //  N dot H squared is computed into GL_SPARE0_NV
  // -N dot H squared is computed into GL_SPARE1_NV
  //
  // otherwise these registers are simply multiplied by white
  this->glCombinerStageParameterfvNV( GL_COMBINER4_NV, GL_CONSTANT_COLOR0_NV, white );
  
  this->glCombinerInputNV( GL_COMBINER4_NV, GL_RGB, GL_VARIABLE_A_NV, GL_SPARE0_NV,          GL_UNSIGNED_IDENTITY_NV, GL_RGB );
  this->glCombinerInputNV( GL_COMBINER4_NV, GL_RGB, GL_VARIABLE_C_NV, GL_SPARE1_NV,          GL_UNSIGNED_IDENTITY_NV, GL_RGB );
  if ( specularPower > 1.0 )
    {
    this->glCombinerInputNV( GL_COMBINER4_NV, GL_RGB, GL_VARIABLE_B_NV, GL_SPARE0_NV,          GL_UNSIGNED_IDENTITY_NV, GL_RGB );
    this->glCombinerInputNV( GL_COMBINER4_NV, GL_RGB, GL_VARIABLE_D_NV, GL_SPARE1_NV,          GL_UNSIGNED_IDENTITY_NV, GL_RGB );
    }
  else
    {
    this->glCombinerInputNV( GL_COMBINER4_NV, GL_RGB, GL_VARIABLE_B_NV, GL_CONSTANT_COLOR0_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB );
    this->glCombinerInputNV( GL_COMBINER4_NV, GL_RGB, GL_VARIABLE_D_NV, GL_CONSTANT_COLOR0_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB );
    }
  
  this->glCombinerOutputNV( GL_COMBINER4_NV, GL_RGB, GL_SPARE0_NV, GL_SPARE1_NV, GL_DISCARD_NV, 
                            GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE );

  // Stage 5
  //
  // if the specular power is greater than 3, then
  //
  //  N dot H to the fourth is computed into GL_SPARE0_NV
  // -N dot H to the fourth is computed into GL_SPARE1_NV
  //
  // otherwise these registers are simply multiplied by white
  this->glCombinerStageParameterfvNV( GL_COMBINER5_NV, GL_CONSTANT_COLOR0_NV, white );
  
  this->glCombinerInputNV( GL_COMBINER5_NV, GL_RGB, GL_VARIABLE_A_NV, GL_SPARE0_NV,          GL_UNSIGNED_IDENTITY_NV, GL_RGB );
  this->glCombinerInputNV( GL_COMBINER5_NV, GL_RGB, GL_VARIABLE_C_NV, GL_SPARE1_NV,          GL_UNSIGNED_IDENTITY_NV, GL_RGB );
  if ( specularPower > 3.0 )
    {
    this->glCombinerInputNV( GL_COMBINER5_NV, GL_RGB, GL_VARIABLE_B_NV, GL_SPARE0_NV,          GL_UNSIGNED_IDENTITY_NV, GL_RGB );
    this->glCombinerInputNV( GL_COMBINER5_NV, GL_RGB, GL_VARIABLE_D_NV, GL_SPARE1_NV,          GL_UNSIGNED_IDENTITY_NV, GL_RGB );
    }
  else
    {
    this->glCombinerInputNV( GL_COMBINER5_NV, GL_RGB, GL_VARIABLE_B_NV, GL_CONSTANT_COLOR0_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB );
    this->glCombinerInputNV( GL_COMBINER5_NV, GL_RGB, GL_VARIABLE_D_NV, GL_CONSTANT_COLOR0_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB );
    }
  
  this->glCombinerOutputNV( GL_COMBINER5_NV, GL_RGB, GL_SPARE0_NV, GL_SPARE1_NV, GL_DISCARD_NV, 
                            GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE );

  // Stage 6
  //
  // if the specular power is greater than 6, then
  //
  //  N dot H to the eighth is computed into GL_SPARE0_NV
  // -N dot H to the eighth is computed into GL_SPARE1_NV
  //
  // otherwise these registers are simply multiplied by white
  this->glCombinerStageParameterfvNV( GL_COMBINER6_NV, GL_CONSTANT_COLOR0_NV, white );
  
  this->glCombinerInputNV( GL_COMBINER6_NV, GL_RGB, GL_VARIABLE_A_NV, GL_SPARE0_NV,          GL_UNSIGNED_IDENTITY_NV, GL_RGB );
  this->glCombinerInputNV( GL_COMBINER6_NV, GL_RGB, GL_VARIABLE_C_NV, GL_SPARE1_NV,          GL_UNSIGNED_IDENTITY_NV, GL_RGB );

  if ( specularPower > 6.0 )
    {
    this->glCombinerInputNV( GL_COMBINER6_NV, GL_RGB, GL_VARIABLE_B_NV, GL_SPARE0_NV,          GL_UNSIGNED_IDENTITY_NV, GL_RGB );
    this->glCombinerInputNV( GL_COMBINER6_NV, GL_RGB, GL_VARIABLE_D_NV, GL_SPARE1_NV,          GL_UNSIGNED_IDENTITY_NV, GL_RGB );
    }
  else
    {
    this->glCombinerInputNV( GL_COMBINER6_NV, GL_RGB, GL_VARIABLE_B_NV, GL_CONSTANT_COLOR0_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB );
    this->glCombinerInputNV( GL_COMBINER6_NV, GL_RGB, GL_VARIABLE_D_NV, GL_CONSTANT_COLOR0_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB );
    }
  
  this->glCombinerOutputNV( GL_COMBINER6_NV, GL_RGB, GL_SPARE0_NV, GL_SPARE1_NV, GL_DISCARD_NV, 
                            GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE );

  
  // Stage 7
  //
  // Add the two specular contributions and multiply each by the specular color.
  this->glCombinerStageParameterfvNV( GL_COMBINER7_NV, GL_CONSTANT_COLOR0_NV, lightSpecularColor[0] );
  this->glCombinerStageParameterfvNV( GL_COMBINER7_NV, GL_CONSTANT_COLOR1_NV, lightSpecularColor[1] );    
  
  this->glCombinerInputNV( GL_COMBINER7_NV, GL_RGB, GL_VARIABLE_A_NV, GL_SPARE0_NV,          GL_UNSIGNED_IDENTITY_NV, GL_RGB );
  this->glCombinerInputNV( GL_COMBINER7_NV, GL_RGB, GL_VARIABLE_B_NV, GL_CONSTANT_COLOR0_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB );
  this->glCombinerInputNV( GL_COMBINER7_NV, GL_RGB, GL_VARIABLE_C_NV, GL_SPARE1_NV,          GL_UNSIGNED_IDENTITY_NV, GL_RGB );
  this->glCombinerInputNV( GL_COMBINER7_NV, GL_RGB, GL_VARIABLE_D_NV, GL_CONSTANT_COLOR1_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB );
  
  this->glCombinerOutputNV( GL_COMBINER7_NV, GL_RGB, GL_DISCARD_NV, GL_DISCARD_NV, GL_SPARE0_NV, 
                            GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE );

  this->glFinalCombinerInputNV(GL_VARIABLE_A_NV, GL_PRIMARY_COLOR_NV,               GL_UNSIGNED_IDENTITY_NV, GL_RGB  );
  if ( components < 3 )
    {
    this->glFinalCombinerInputNV(GL_VARIABLE_B_NV, GL_TEXTURE1_ARB,                   GL_UNSIGNED_IDENTITY_NV, GL_RGB  );
    }
  else
    {
    this->glFinalCombinerInputNV(GL_VARIABLE_B_NV, GL_TEXTURE0_ARB,                   GL_UNSIGNED_IDENTITY_NV, GL_RGB  );
    }
  this->glFinalCombinerInputNV(GL_VARIABLE_C_NV, GL_ZERO,                           GL_UNSIGNED_IDENTITY_NV, GL_RGB  );
  this->glFinalCombinerInputNV(GL_VARIABLE_D_NV, GL_SPARE0_NV,                      GL_UNSIGNED_IDENTITY_NV, GL_RGB  );
  
  if ( components == 1 )
    {
    this->glFinalCombinerInputNV(GL_VARIABLE_G_NV, GL_TEXTURE1_ARB, GL_UNSIGNED_IDENTITY_NV, GL_ALPHA);
    }
  else
    {
    this->glFinalCombinerInputNV(GL_VARIABLE_G_NV, GL_TEXTURE3_ARB, GL_UNSIGNED_IDENTITY_NV, GL_ALPHA);
    }
  
}

void vtkOpenGLVolumeTextureMapper3D::RenderOneIndependentNoShadeNV( vtkRenderer *ren,
                                                                      vtkVolume *vol )
{
  this->SetupOneIndependentTextures( ren, vol );

  // Start the timer now
  this->Timer->StartTimer();
  
  this->SetupRegisterCombinersNoShadeNV( ren, vol, 1 );
  
  int stages[4] = {1,0,0,0};
  this->RenderPolygons( ren, vol, stages );  
}


void vtkOpenGLVolumeTextureMapper3D::RenderOneIndependentShadeNV( vtkRenderer *ren,
                                                                    vtkVolume *vol )
{
  this->SetupOneIndependentTextures( ren, vol );
  
  // Start the timer now
  this->Timer->StartTimer();
  
  this->SetupRegisterCombinersShadeNV( ren, vol, 1 );
  
  int stages[4] = {1,0,1,0};
  this->RenderPolygons( ren, vol, stages );  
}


void vtkOpenGLVolumeTextureMapper3D::SetupTwoDependentTextures( vtkRenderer *vtkNotUsed(ren),
                  vtkVolume *vol )
{
  this->glActiveTextureARB( GL_TEXTURE0_ARB );
  glDisable( GL_TEXTURE_2D );
  glEnable( GL_TEXTURE_3D_EXT );
  if ( this->RenderMethod == vtkVolumeTextureMapper3D::NVIDIA_METHOD )
    {
    glEnable( GL_TEXTURE_SHADER_NV );
    glTexEnvi(GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, GL_TEXTURE_3D_EXT);
    }

  this->glActiveTextureARB( GL_TEXTURE2_ARB );
  glDisable( GL_TEXTURE_2D );
  glEnable( GL_TEXTURE_3D_EXT );
  if ( this->RenderMethod == vtkVolumeTextureMapper3D::NVIDIA_METHOD )
    {
    glEnable( GL_TEXTURE_SHADER_NV );  
    glTexEnvi(GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, GL_TEXTURE_3D_EXT);
    }

  // Update the volume containing the 3 byte scalars / gradient magnitude
  if ( this->UpdateVolumes( vol ) || !this->Volume1Index || !this->Volume2Index )
    {    
    int dim[3];
    this->GetVolumeDimensions(dim);
    this->DeleteTextureIndex(&this->Volume3Index);
    
    this->glActiveTextureARB( GL_TEXTURE0_ARB );
    glBindTexture(GL_TEXTURE_3D_EXT, (GLuint)NULL);
    this->DeleteTextureIndex(&this->Volume1Index);
    this->CreateTextureIndex(&this->Volume1Index);
    glBindTexture(GL_TEXTURE_3D_EXT, this->Volume1Index);
    this->glTexImage3DEXT( GL_TEXTURE_3D_EXT, 0, GL_RGB8, dim[0], dim[1], dim[2], 0,
                           GL_RGB, GL_UNSIGNED_BYTE, this->Volume1 );
    
    this->glActiveTextureARB( GL_TEXTURE2_ARB );
    glBindTexture(GL_TEXTURE_3D_EXT, (GLuint)NULL);
    this->DeleteTextureIndex(&this->Volume2Index);
    this->CreateTextureIndex(&this->Volume2Index);
    glBindTexture(GL_TEXTURE_3D_EXT, this->Volume2Index);
    this->glTexImage3DEXT( GL_TEXTURE_3D_EXT, 0, GL_RGBA8, dim[0], dim[1], dim[2], 0,
                           GL_RGB, GL_UNSIGNED_BYTE, this->Volume2 );
    }
  
  this->glActiveTextureARB( GL_TEXTURE0_ARB );
  glBindTexture(GL_TEXTURE_3D_EXT, this->Volume1Index);   
  this->Setup3DTextureParameters( vol->GetProperty() );

  this->glActiveTextureARB( GL_TEXTURE2_ARB );
  glBindTexture(GL_TEXTURE_3D_EXT, this->Volume2Index);   
  this->Setup3DTextureParameters( vol->GetProperty() );
    
  this->glActiveTextureARB( GL_TEXTURE1_ARB );
  glEnable( GL_TEXTURE_2D );
  glDisable( GL_TEXTURE_3D_EXT );
  if ( this->RenderMethod == vtkVolumeTextureMapper3D::NVIDIA_METHOD )
    {
    glTexEnvf ( GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, 
                GL_DEPENDENT_AR_TEXTURE_2D_NV );
    glTexEnvi(GL_TEXTURE_SHADER_NV, GL_PREVIOUS_TEXTURE_INPUT_NV, GL_TEXTURE0_ARB);
    }

  this->glActiveTextureARB( GL_TEXTURE3_ARB );
  glEnable( GL_TEXTURE_2D );
  glDisable( GL_TEXTURE_3D_EXT );
  if ( this->RenderMethod == vtkVolumeTextureMapper3D::NVIDIA_METHOD )
    {
    glTexEnvf ( GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, 
                GL_DEPENDENT_GB_TEXTURE_2D_NV );
    glTexEnvi(GL_TEXTURE_SHADER_NV, GL_PREVIOUS_TEXTURE_INPUT_NV, GL_TEXTURE0_ARB);
    }

  // Update the dependent 2D color table mapping scalar value and
  // gradient magnitude to RGBA
  if ( this->UpdateColorLookup( vol ) || 
       !this->ColorLookupIndex || !this->AlphaLookupIndex )
    {    
    this->glActiveTextureARB( GL_TEXTURE1_ARB );
    glBindTexture(GL_TEXTURE_2D, (GLuint)NULL);
    this->DeleteTextureIndex(&this->ColorLookupIndex);
    this->CreateTextureIndex(&this->ColorLookupIndex);
    glBindTexture(GL_TEXTURE_2D, this->ColorLookupIndex);   

    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );

    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB8, 256, 256, 0,
                  GL_RGB, GL_UNSIGNED_BYTE, this->ColorLookup );    
      
    this->glActiveTextureARB( GL_TEXTURE3_ARB );
    glBindTexture(GL_TEXTURE_2D, (GLuint)NULL);
    this->DeleteTextureIndex(&this->AlphaLookupIndex);
    this->CreateTextureIndex(&this->AlphaLookupIndex);
    glBindTexture(GL_TEXTURE_2D, this->AlphaLookupIndex);   

    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );

    glTexImage2D( GL_TEXTURE_2D, 0, GL_ALPHA8, 256, 256, 0,
                  GL_ALPHA, GL_UNSIGNED_BYTE, this->AlphaLookup );      
    }
  
  this->glActiveTextureARB( GL_TEXTURE1_ARB );
  glBindTexture(GL_TEXTURE_2D, this->ColorLookupIndex);   

  this->glActiveTextureARB( GL_TEXTURE3_ARB );
  glBindTexture(GL_TEXTURE_2D, this->AlphaLookupIndex);   
}

void vtkOpenGLVolumeTextureMapper3D::RenderTwoDependentNoShadeNV( vtkRenderer *ren,
                                                                    vtkVolume *vol )
{
  this->SetupTwoDependentTextures(ren, vol);
  
  // Start the timer now
  this->Timer->StartTimer();
  
  this->SetupRegisterCombinersNoShadeNV( ren, vol, 2 );
  
  int stages[4] = {1,0,0,0};
  this->RenderPolygons( ren, vol, stages );  
}

void vtkOpenGLVolumeTextureMapper3D::RenderTwoDependentShadeNV( vtkRenderer *ren,
                                                                  vtkVolume *vol )
{
  this->SetupTwoDependentTextures( ren, vol );
  
  // Start the timer now
  this->Timer->StartTimer();
  
  this->SetupRegisterCombinersShadeNV( ren, vol, 2 );
  
  int stages[4] = {1,0,1,0};
  this->RenderPolygons( ren, vol, stages );  
}

void vtkOpenGLVolumeTextureMapper3D::SetupFourDependentTextures( vtkRenderer *vtkNotUsed(ren),
                   vtkVolume *vol )
{
  this->glActiveTextureARB( GL_TEXTURE0_ARB );
  glDisable( GL_TEXTURE_2D );
  glEnable( GL_TEXTURE_3D_EXT );
  if ( this->RenderMethod == vtkVolumeTextureMapper3D::NVIDIA_METHOD )
    {
    glEnable( GL_TEXTURE_SHADER_NV );
    glTexEnvi(GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, GL_TEXTURE_3D_EXT);
    }

  this->glActiveTextureARB( GL_TEXTURE1_ARB );
  glDisable( GL_TEXTURE_2D );
  glEnable( GL_TEXTURE_3D_EXT );
  if ( this->RenderMethod == vtkVolumeTextureMapper3D::NVIDIA_METHOD )
    {
    glEnable( GL_TEXTURE_SHADER_NV );
    glTexEnvi(GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, GL_TEXTURE_3D_EXT);
    }

  this->glActiveTextureARB( GL_TEXTURE2_ARB );
  glDisable( GL_TEXTURE_2D );
  glEnable( GL_TEXTURE_3D_EXT );
  if ( this->RenderMethod == vtkVolumeTextureMapper3D::NVIDIA_METHOD )
    {
    glEnable( GL_TEXTURE_SHADER_NV );  
    glTexEnvi(GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, GL_TEXTURE_3D_EXT);
    }

  // Update the volume containing the 3 byte scalars / gradient magnitude
  if ( this->UpdateVolumes( vol ) || !this->Volume1Index || 
       !this->Volume2Index || !this->Volume3Index )
    {    
    int dim[3];
    this->GetVolumeDimensions(dim);
    
    this->glActiveTextureARB( GL_TEXTURE0_ARB );
    glBindTexture(GL_TEXTURE_3D_EXT, (GLuint)NULL);
    this->DeleteTextureIndex(&this->Volume1Index);
    this->CreateTextureIndex(&this->Volume1Index);
    glBindTexture(GL_TEXTURE_3D_EXT, this->Volume1Index);
    this->glTexImage3DEXT( GL_TEXTURE_3D_EXT, 0, GL_RGB8, dim[0], dim[1], dim[2], 0,
                           GL_RGB, GL_UNSIGNED_BYTE, this->Volume1 );

    this->glActiveTextureARB( GL_TEXTURE1_ARB );
    glBindTexture(GL_TEXTURE_3D_EXT, (GLuint)NULL);
    this->DeleteTextureIndex(&this->Volume2Index);
    this->CreateTextureIndex(&this->Volume2Index);
    glBindTexture(GL_TEXTURE_3D_EXT, this->Volume2Index);   
    this->glTexImage3DEXT( GL_TEXTURE_3D_EXT, 0, GL_LUMINANCE8_ALPHA8, dim[0], dim[1], dim[2], 0,
                           GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, this->Volume2 );

    this->glActiveTextureARB( GL_TEXTURE2_ARB );
    glBindTexture(GL_TEXTURE_3D_EXT, (GLuint)NULL);
    this->DeleteTextureIndex(&this->Volume3Index);
    this->CreateTextureIndex(&this->Volume3Index);
    glBindTexture(GL_TEXTURE_3D_EXT, this->Volume3Index);
    this->glTexImage3DEXT( GL_TEXTURE_3D_EXT, 0, GL_RGB8, dim[0], dim[1], dim[2], 0,
                           GL_RGB, GL_UNSIGNED_BYTE, this->Volume3 );
    }
  
  this->glActiveTextureARB( GL_TEXTURE0_ARB );
  glBindTexture(GL_TEXTURE_3D_EXT, this->Volume1Index);   
  this->Setup3DTextureParameters( vol->GetProperty() );

  this->glActiveTextureARB( GL_TEXTURE1_ARB );
  glBindTexture(GL_TEXTURE_3D_EXT, this->Volume2Index);   
  this->Setup3DTextureParameters( vol->GetProperty() );

  this->glActiveTextureARB( GL_TEXTURE2_ARB );
  glBindTexture(GL_TEXTURE_3D_EXT, this->Volume3Index);   
  this->Setup3DTextureParameters( vol->GetProperty() );

  this->glActiveTextureARB( GL_TEXTURE3_ARB );
  glEnable( GL_TEXTURE_2D );
  glDisable( GL_TEXTURE_3D_EXT );
  if ( this->RenderMethod == vtkVolumeTextureMapper3D::NVIDIA_METHOD )
    {
    glTexEnvf ( GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, 
                GL_DEPENDENT_AR_TEXTURE_2D_NV );
    glTexEnvi(GL_TEXTURE_SHADER_NV, GL_PREVIOUS_TEXTURE_INPUT_NV, GL_TEXTURE1_ARB);
    }

  // Update the dependent 2D table mapping scalar value and
  // gradient magnitude to opacity
  if ( this->UpdateColorLookup( vol ) || !this->AlphaLookupIndex )
    {    
    this->DeleteTextureIndex(&this->ColorLookupIndex);
    
    this->glActiveTextureARB( GL_TEXTURE3_ARB );
    glBindTexture(GL_TEXTURE_2D, (GLuint)NULL);
    this->DeleteTextureIndex(&this->AlphaLookupIndex);
    this->CreateTextureIndex(&this->AlphaLookupIndex);
    glBindTexture(GL_TEXTURE_2D, this->AlphaLookupIndex);   

    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );

    glTexImage2D( GL_TEXTURE_2D, 0, GL_ALPHA8, 256, 256, 0,
                  GL_ALPHA, GL_UNSIGNED_BYTE, this->AlphaLookup );      
    }

  this->glActiveTextureARB( GL_TEXTURE3_ARB );
  glBindTexture(GL_TEXTURE_2D, this->AlphaLookupIndex);   
}

void vtkOpenGLVolumeTextureMapper3D::RenderFourDependentNoShadeNV( vtkRenderer *ren,
                                                                     vtkVolume *vol )
{
  this->SetupFourDependentTextures(ren, vol);
  
  // Start the timer now
  this->Timer->StartTimer();
  
  this->SetupRegisterCombinersNoShadeNV( ren, vol, 4 );
  
  int stages[4] = {1,1,0,0};
  this->RenderPolygons( ren, vol, stages );  
}

void vtkOpenGLVolumeTextureMapper3D::RenderFourDependentShadeNV( vtkRenderer *ren,
                                                                   vtkVolume *vol )
{
  this->SetupFourDependentTextures( ren, vol );
  
  // Start the timer now
  this->Timer->StartTimer();
  
  this->SetupRegisterCombinersShadeNV( ren, vol, 4 );
  
  int stages[4] = {1,1,1,0};
  this->RenderPolygons( ren, vol, stages );  
}

void vtkOpenGLVolumeTextureMapper3D::RenderOneIndependentNoShadeFP( vtkRenderer *ren,
                                                                      vtkVolume *vol )
{
  glEnable( GL_FRAGMENT_PROGRAM_ARB );

  GLuint fragmentProgram;
  this->glGenProgramsARB( 1, &fragmentProgram );


  this->glBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, fragmentProgram );

  this->glProgramStringARB( GL_FRAGMENT_PROGRAM_ARB,
          GL_PROGRAM_FORMAT_ASCII_ARB, 
          strlen(vtkVolumeTextureMapper3D_OneComponentNoShadeFP),
          vtkVolumeTextureMapper3D_OneComponentNoShadeFP );

  this->SetupOneIndependentTextures( ren, vol );

  // Start the timer now
  this->Timer->StartTimer();
  
  int stages[4] = {1,0,0,0};
  this->RenderPolygons( ren, vol, stages );  

  glDisable( GL_FRAGMENT_PROGRAM_ARB );
  
  this->glDeleteProgramsARB( 1, &fragmentProgram );
}

void vtkOpenGLVolumeTextureMapper3D::RenderOneIndependentShadeFP( vtkRenderer *ren,
                                                                    vtkVolume *vol )
{
  glEnable( GL_FRAGMENT_PROGRAM_ARB );

  GLuint fragmentProgram;
  this->glGenProgramsARB( 1, &fragmentProgram );


  this->glBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, fragmentProgram );

  this->glProgramStringARB( GL_FRAGMENT_PROGRAM_ARB,
          GL_PROGRAM_FORMAT_ASCII_ARB, 
          strlen(vtkVolumeTextureMapper3D_OneComponentShadeFP),
          vtkVolumeTextureMapper3D_OneComponentShadeFP );
           
  this->SetupOneIndependentTextures( ren, vol );
  this->SetupProgramLocalsForShadingFP( ren, vol );

  // Start the timer now
  this->Timer->StartTimer();
  
  int stages[4] = {1,1,1,0};
  this->RenderPolygons( ren, vol, stages );  

  glDisable( GL_FRAGMENT_PROGRAM_ARB );
  
  this->glDeleteProgramsARB( 1, &fragmentProgram );
}

void vtkOpenGLVolumeTextureMapper3D::RenderTwoDependentNoShadeFP( vtkRenderer *ren,
                                                                    vtkVolume *vol )
{
  glEnable( GL_FRAGMENT_PROGRAM_ARB );

  GLuint fragmentProgram;
  this->glGenProgramsARB( 1, &fragmentProgram );

  this->glBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, fragmentProgram );

  this->glProgramStringARB( GL_FRAGMENT_PROGRAM_ARB,
          GL_PROGRAM_FORMAT_ASCII_ARB, 
          strlen(vtkVolumeTextureMapper3D_TwoDependentNoShadeFP),
          vtkVolumeTextureMapper3D_TwoDependentNoShadeFP );

  this->SetupTwoDependentTextures(ren, vol);

  // Start the timer now
  this->Timer->StartTimer();
  
  int stages[4] = {1,0,0,0};
  this->RenderPolygons( ren, vol, stages );  

  glDisable( GL_FRAGMENT_PROGRAM_ARB );
  
  this->glDeleteProgramsARB( 1, &fragmentProgram );
}


void vtkOpenGLVolumeTextureMapper3D::RenderTwoDependentShadeFP( vtkRenderer *ren,
                  vtkVolume *vol )
{
  glEnable( GL_FRAGMENT_PROGRAM_ARB );

  GLuint fragmentProgram;
  this->glGenProgramsARB( 1, &fragmentProgram );

  this->glBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, fragmentProgram );

  this->glProgramStringARB( GL_FRAGMENT_PROGRAM_ARB,
          GL_PROGRAM_FORMAT_ASCII_ARB, 
          strlen(vtkVolumeTextureMapper3D_TwoDependentShadeFP),
          vtkVolumeTextureMapper3D_TwoDependentShadeFP );

  this->SetupTwoDependentTextures(ren, vol);
  this->SetupProgramLocalsForShadingFP( ren, vol );

  // Start the timer now
  this->Timer->StartTimer();
  
  int stages[4] = {1,0,1,0};
  this->RenderPolygons( ren, vol, stages );  

  glDisable( GL_FRAGMENT_PROGRAM_ARB );
  
  this->glDeleteProgramsARB( 1, &fragmentProgram );
}

void vtkOpenGLVolumeTextureMapper3D::RenderFourDependentNoShadeFP( vtkRenderer *ren,
                                                                     vtkVolume *vol )
{
  glEnable( GL_FRAGMENT_PROGRAM_ARB );

  GLuint fragmentProgram;
  this->glGenProgramsARB( 1, &fragmentProgram );

  this->glBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, fragmentProgram );

  this->glProgramStringARB( GL_FRAGMENT_PROGRAM_ARB,
          GL_PROGRAM_FORMAT_ASCII_ARB, 
          strlen(vtkVolumeTextureMapper3D_FourDependentNoShadeFP),
          vtkVolumeTextureMapper3D_FourDependentNoShadeFP );

  this->SetupFourDependentTextures(ren, vol);

  // Start the timer now
  this->Timer->StartTimer();
  
  int stages[4] = {1,1,0,0};
  this->RenderPolygons( ren, vol, stages );  

  glDisable( GL_FRAGMENT_PROGRAM_ARB );
  
  this->glDeleteProgramsARB( 1, &fragmentProgram );
}

void vtkOpenGLVolumeTextureMapper3D::RenderFourDependentShadeFP( vtkRenderer *ren,
                   vtkVolume *vol )
{
  glEnable( GL_FRAGMENT_PROGRAM_ARB );

  GLuint fragmentProgram;
  this->glGenProgramsARB( 1, &fragmentProgram );

  this->glBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, fragmentProgram );

  this->glProgramStringARB( GL_FRAGMENT_PROGRAM_ARB,
          GL_PROGRAM_FORMAT_ASCII_ARB, 
          strlen(vtkVolumeTextureMapper3D_FourDependentShadeFP),
          vtkVolumeTextureMapper3D_FourDependentShadeFP );

  this->SetupFourDependentTextures(ren, vol);
  this->SetupProgramLocalsForShadingFP( ren, vol );

  // Start the timer now
  this->Timer->StartTimer();
  
  int stages[4] = {1,1,1,0};
  this->RenderPolygons( ren, vol, stages );  

  glDisable( GL_FRAGMENT_PROGRAM_ARB );
  
  this->glDeleteProgramsARB( 1, &fragmentProgram );
}


void vtkOpenGLVolumeTextureMapper3D::GetLightInformation( vtkRenderer *ren,
                                                            vtkVolume *vol,
                                                            GLfloat lightDirection[2][4],
                                                            GLfloat lightDiffuseColor[2][4],
                                                            GLfloat lightSpecularColor[2][4],
                                                            GLfloat halfwayVector[2][4],
                                                            GLfloat ambientColor[4] )
{
  float ambient = vol->GetProperty()->GetAmbient();
  float diffuse  = vol->GetProperty()->GetDiffuse();
  float specular = vol->GetProperty()->GetSpecular();
  
  vtkTransform *volumeTransform = vtkTransform::New();
  
  volumeTransform->SetMatrix( vol->GetMatrix() );
  volumeTransform->Inverse();
  
  vtkLightCollection *lights = ren->GetLights();
  lights->InitTraversal();
  
  vtkLight *light[2];
  light[0] = lights->GetNextItem();
  light[1] = lights->GetNextItem();
  
  int lightIndex = 0;
  
  double cameraPosition[3];
  double cameraFocalPoint[3];
  
  ren->GetActiveCamera()->GetPosition( cameraPosition );
  ren->GetActiveCamera()->GetFocalPoint( cameraFocalPoint );
  
  double viewDirection[3];
  
  volumeTransform->TransformPoint( cameraPosition, cameraPosition );
  volumeTransform->TransformPoint( cameraFocalPoint, cameraFocalPoint );
  
  viewDirection[0] = cameraFocalPoint[0] - cameraPosition[0];
  viewDirection[1] = cameraFocalPoint[1] - cameraPosition[1];
  viewDirection[2] = cameraFocalPoint[2] - cameraPosition[2];
  
  vtkMath::Normalize( viewDirection );
  

  ambientColor[0] = 0.0;
  ambientColor[1] = 0.0;
  ambientColor[2] = 0.0;  
  ambientColor[3] = 0.0;  
  
  for ( lightIndex = 0; lightIndex < 2; lightIndex++ )
    {
    float dir[3] = {0,0,0};
    float half[3] = {0,0,0};
    
    if ( light[lightIndex] == NULL ||
         light[lightIndex]->GetSwitch() == 0 )
      {
      lightDiffuseColor[lightIndex][0] = 0.0;
      lightDiffuseColor[lightIndex][1] = 0.0;
      lightDiffuseColor[lightIndex][2] = 0.0;
      lightDiffuseColor[lightIndex][3] = 0.0;

      lightSpecularColor[lightIndex][0] = 0.0;
      lightSpecularColor[lightIndex][1] = 0.0;
      lightSpecularColor[lightIndex][2] = 0.0;
      lightSpecularColor[lightIndex][3] = 0.0;
      }
    else
      {
      float lightIntensity = light[lightIndex]->GetIntensity();
      double lightColor[3];
      
      light[lightIndex]->GetColor( lightColor );
      
      double lightPosition[3];
      double lightFocalPoint[3];
      light[lightIndex]->GetTransformedPosition( lightPosition );
      light[lightIndex]->GetTransformedFocalPoint( lightFocalPoint );

      volumeTransform->TransformPoint( lightPosition, lightPosition );
      volumeTransform->TransformPoint( lightFocalPoint, lightFocalPoint );      
      
      dir[0] = lightPosition[0] - lightFocalPoint[0];
      dir[1] = lightPosition[1] - lightFocalPoint[1];
      dir[2] = lightPosition[2] - lightFocalPoint[2];
      
      vtkMath::Normalize( dir );
      
      
      lightDiffuseColor[lightIndex][0] = lightColor[0]*diffuse*lightIntensity;
      lightDiffuseColor[lightIndex][1] = lightColor[1]*diffuse*lightIntensity;
      lightDiffuseColor[lightIndex][2] = lightColor[2]*diffuse*lightIntensity;
      lightDiffuseColor[lightIndex][3] = 1.0;
      
      lightSpecularColor[lightIndex][0] = lightColor[0]*specular*lightIntensity;
      lightSpecularColor[lightIndex][1] = lightColor[1]*specular*lightIntensity;
      lightSpecularColor[lightIndex][2] = lightColor[2]*specular*lightIntensity;
      lightSpecularColor[lightIndex][3] = 0.0;

      half[0] = dir[0] - viewDirection[0];
      half[1] = dir[1] - viewDirection[1];
      half[2] = dir[2] - viewDirection[2];
      
      vtkMath::Normalize( half );
      
      ambientColor[0] += ambient*lightColor[0];
      ambientColor[1] += ambient*lightColor[1];
      ambientColor[2] += ambient*lightColor[2];      
      }

    lightDirection[lightIndex][0] = (dir[0]+1.0)/2.0;
    lightDirection[lightIndex][1] = (dir[1]+1.0)/2.0;
    lightDirection[lightIndex][2] = (dir[2]+1.0)/2.0;
    lightDirection[lightIndex][3] = 0.0;
    
    halfwayVector[lightIndex][0] = (half[0]+1.0)/2.0;
    halfwayVector[lightIndex][1] = (half[1]+1.0)/2.0;
    halfwayVector[lightIndex][2] = (half[2]+1.0)/2.0;
    halfwayVector[lightIndex][3] = 0.0;    
    }
  
  volumeTransform->Delete();
  
}

void vtkOpenGLVolumeTextureMapper3D::SetupProgramLocalsForShadingFP( vtkRenderer *ren,
                  vtkVolume *vol )
{
  GLfloat lightDirection[2][4];
  GLfloat lightDiffuseColor[2][4];
  GLfloat lightSpecularColor[2][4];
  GLfloat halfwayVector[2][4];
  GLfloat ambientColor[4];

  float ambient       = vol->GetProperty()->GetAmbient();
  float diffuse       = vol->GetProperty()->GetDiffuse();
  float specular      = vol->GetProperty()->GetSpecular();
  float specularPower = vol->GetProperty()->GetSpecularPower();
  
  vtkTransform *volumeTransform = vtkTransform::New();
  
  volumeTransform->SetMatrix( vol->GetMatrix() );
  volumeTransform->Inverse();
  
  vtkLightCollection *lights = ren->GetLights();
  lights->InitTraversal();
  
  vtkLight *light[2];
  light[0] = lights->GetNextItem();
  light[1] = lights->GetNextItem();
  
  int lightIndex = 0;
  
  double cameraPosition[3];
  double cameraFocalPoint[3];
  
  ren->GetActiveCamera()->GetPosition( cameraPosition );
  ren->GetActiveCamera()->GetFocalPoint( cameraFocalPoint );
  
  double viewDirection[3];
  
  viewDirection[0] = cameraFocalPoint[0] - cameraPosition[0];
  viewDirection[1] = cameraFocalPoint[1] - cameraPosition[1];
  viewDirection[2] = cameraFocalPoint[2] - cameraPosition[2];
  
  vtkMath::Normalize( viewDirection );
  
  volumeTransform->TransformPoint( viewDirection, viewDirection );

  ambientColor[0] = 0.0;
  ambientColor[1] = 0.0;
  ambientColor[2] = 0.0;  
  ambientColor[3] = 0.0;  
  
  for ( lightIndex = 0; lightIndex < 2; lightIndex++ )
    {
    float dir[3] = {0,0,0};
    float half[3] = {0,0,0};
    
    if ( light[lightIndex] == NULL ||
         light[lightIndex]->GetSwitch() == 0 )
      {
      lightDiffuseColor[lightIndex][0] = 0.0;
      lightDiffuseColor[lightIndex][1] = 0.0;
      lightDiffuseColor[lightIndex][2] = 0.0;
      lightDiffuseColor[lightIndex][3] = 0.0;

      lightSpecularColor[lightIndex][0] = 0.0;
      lightSpecularColor[lightIndex][1] = 0.0;
      lightSpecularColor[lightIndex][2] = 0.0;
      lightSpecularColor[lightIndex][3] = 0.0;
      }
    else
      {
      float lightIntensity = light[lightIndex]->GetIntensity();
      double lightColor[3];
      
      light[lightIndex]->GetColor( lightColor );
      
      double lightPosition[3];
      double lightFocalPoint[3];
      light[lightIndex]->GetTransformedPosition( lightPosition );
      light[lightIndex]->GetTransformedFocalPoint( lightFocalPoint );
      
      dir[0] = lightPosition[0] - lightFocalPoint[0];
      dir[1] = lightPosition[1] - lightFocalPoint[1];
      dir[2] = lightPosition[2] - lightFocalPoint[2];
      
      vtkMath::Normalize( dir );
      
      volumeTransform->TransformPoint( dir, dir );
      
      lightDiffuseColor[lightIndex][0] = lightColor[0]*diffuse*lightIntensity;
      lightDiffuseColor[lightIndex][1] = lightColor[1]*diffuse*lightIntensity;
      lightDiffuseColor[lightIndex][2] = lightColor[2]*diffuse*lightIntensity;
      lightDiffuseColor[lightIndex][3] = 0.0;
      
      lightSpecularColor[lightIndex][0] = lightColor[0]*specular*lightIntensity;
      lightSpecularColor[lightIndex][1] = lightColor[1]*specular*lightIntensity;
      lightSpecularColor[lightIndex][2] = lightColor[2]*specular*lightIntensity;
      lightSpecularColor[lightIndex][3] = 0.0;

      half[0] = dir[0] - viewDirection[0];
      half[1] = dir[1] - viewDirection[1];
      half[2] = dir[2] - viewDirection[2];
      
      vtkMath::Normalize( half );
      
      ambientColor[0] += ambient*lightColor[0];
      ambientColor[1] += ambient*lightColor[1];
      ambientColor[2] += ambient*lightColor[2];      
      }

    lightDirection[lightIndex][0] = dir[0];
    lightDirection[lightIndex][1] = dir[1];
    lightDirection[lightIndex][2] = dir[2];
    lightDirection[lightIndex][3] = 0.0;
    
    halfwayVector[lightIndex][0] = half[0];
    halfwayVector[lightIndex][1] = half[1];
    halfwayVector[lightIndex][2] = half[2];
    halfwayVector[lightIndex][3] = 0.0;    
    }
  
  volumeTransform->Delete();

  this->glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, 0, 
              lightDirection[0][0],
              lightDirection[0][1],
              lightDirection[0][2],
              lightDirection[0][3] );

  this->glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, 1, 
              halfwayVector[0][0],
              halfwayVector[0][1],
              halfwayVector[0][2],
              halfwayVector[0][3] );

  this->glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, 2,
              ambient, diffuse, specular, specularPower );

  this->glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, 3,
              lightDiffuseColor[0][0],
              lightDiffuseColor[0][1],
              lightDiffuseColor[0][2],
              lightDiffuseColor[0][3] );

  this->glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, 4,
              lightSpecularColor[0][0],
              lightSpecularColor[0][1],
              lightSpecularColor[0][2],
              lightSpecularColor[0][3] );

  this->glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, 5,
              viewDirection[0],
              viewDirection[1],
              viewDirection[2],
              viewDirection[3] );

  this->glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, 6, 2.0, -1.0, 0.0, 0.0 );
}

int  vtkOpenGLVolumeTextureMapper3D::IsRenderSupported( vtkVolumeProperty *property )
{
  if ( !this->Initialized )
    {
    this->Initialize();
    }
  
  if ( this->RenderMethod == vtkVolumeTextureMapper3D::NO_METHOD )
    {
    return 0;
    }
  
  if ( !this->GetInput() )
  {
    return 0;
  }

  if ( this->GetInput()->GetNumberOfScalarComponents() > 1 &&
       property->GetIndependentComponents() )
    {
    return 0;
    }
  
  return 1;
}

void vtkOpenGLVolumeTextureMapper3D::Initialize()
{
  this->Initialized = 1;

  int supports_GL_EXT_texture3D          = this->IsExtensionSupported( "GL_EXT_texture3D" );
  int supports_GL_ARB_multitexture       = this->IsExtensionSupported( "GL_ARB_multitexture" );
  int supports_GL_NV_texture_shader2     = this->IsExtensionSupported( "GL_NV_texture_shader2" );
  int supports_GL_NV_register_combiners2 = this->IsExtensionSupported( "GL_NV_register_combiners2" );
  int supports_GL_ATI_fragment_shader    = this->IsExtensionSupported( "GL_ATI_fragment_shader" );
  int supports_GL_ARB_fragment_program   = this->IsExtensionSupported( "GL_ARB_fragment_program" );

  this->glTexImage3DEXT              = (PFNGLTEX3DEXT) this->GetProcAddress("glTexImage3DEXT");
  this->glActiveTextureARB           = (PFNGLACTIVETEXTUREARB) this->GetProcAddress("glActiveTextureARB");
  this->glMultiTexCoord3fvARB        = (PFNGLMULTITEXCOORD3FVARB) this->GetProcAddress("glMultiTexCoord3fvARB");
  this->glCombinerParameteriNV       = (PFNGLCOMBINERPARAMETERINV) this->GetProcAddress("glCombinerParameteriNV");
  this->glCombinerStageParameterfvNV = (PFNGLCOMBINERSTAGEPARAMETERFVNV) this->GetProcAddress("glCombinerStageParameterfvNV");
  this->glCombinerInputNV            = (PFNGLCOMBINERINPUTNV) this->GetProcAddress("glCombinerInputNV");
  this->glCombinerOutputNV           = (PFNGLCOMBINEROUTPUTNV) this->GetProcAddress("glCombinerOutputNV");
  this->glFinalCombinerInputNV       = (PFNGLFINALCOMBINERINPUTNV) this->GetProcAddress("glFinalCombinerInputNV");
  this->glGenProgramsARB             = (PFNGLGENPROGRAMSARB) this->GetProcAddress("glGenProgramsARB");
  this->glDeleteProgramsARB          = (PFNGLDELETEPROGRAMSARB) this->GetProcAddress("glDeleteProgramsARB");
  this->glBindProgramARB             = (PFNGLBINDPROGRAMARB) this->GetProcAddress("glBindProgramARB");
  this->glProgramStringARB           = (PFNGLPROGRAMSTRINGARB) this->GetProcAddress("glProgramStringARB");
  this->glProgramLocalParameter4fARB = (PFNGLPROGRAMLOCALPARAMETER4FARB) this->GetProcAddress("glProgramLocalParameter4fARB");

  if ( supports_GL_EXT_texture3D          && 
       supports_GL_ARB_multitexture       &&
       supports_GL_ARB_fragment_program   &&
       this->glTexImage3DEXT              &&
       this->glActiveTextureARB           &&
       this->glMultiTexCoord3fvARB        &&
       this->glGenProgramsARB             &&
       this->glDeleteProgramsARB          &&
       this->glBindProgramARB             &&
       this->glProgramStringARB           &&
       this->glProgramLocalParameter4fARB )
    {
    this->RenderMethod = vtkVolumeTextureMapper3D::FRAGMENT_PROGRAM_METHOD;  
    }
  else if ( supports_GL_EXT_texture3D    &&
      supports_GL_ARB_multitexture       &&
      supports_GL_NV_texture_shader2     &&
      supports_GL_NV_register_combiners2 &&
      this->glTexImage3DEXT              &&
      this->glActiveTextureARB           &&
      this->glMultiTexCoord3fvARB        &&
      this->glCombinerParameteriNV       &&
      this->glCombinerStageParameterfvNV &&
      this->glCombinerInputNV            &&
      this->glCombinerOutputNV           &&
      this->glFinalCombinerInputNV )
    {
    this->RenderMethod = vtkVolumeTextureMapper3D::NVIDIA_METHOD;
    }
  else if ( supports_GL_EXT_texture3D         &&
            supports_GL_ARB_multitexture      &&
            supports_GL_ATI_fragment_shader )
    {
    // this should be the older ATI support but is not filled in yet so
    // returning no method instead of:
    //    this->RenderMethod = vtkVolumeTextureMapper3D::ATI_METHOD;
    this->RenderMethod = vtkVolumeTextureMapper3D::NO_METHOD;
    }
  else
    {
    this->RenderMethod = vtkVolumeTextureMapper3D::NO_METHOD;
    }
  
  
}

int vtkOpenGLVolumeTextureMapper3D::IsTextureSizeSupported( int size[3] )
{
  if ( this->GetInput()->GetNumberOfScalarComponents() < 4 )
    {
    if ( size[0]*size[1]*size[2] > 128*256*256 )
      {
      return 0;
      }
    
    this->glTexImage3DEXT( GL_PROXY_TEXTURE_3D_EXT, 0, GL_RGBA8, size[0]*2, 
                           size[1]*2, size[2], 0, GL_RGBA, GL_UNSIGNED_BYTE, this->Volume2 );
    }
  else
    {
    if ( size[0]*size[1]*size[2] > 128*128*128 )
      {
      return 0;
      }
    
    this->glTexImage3DEXT( GL_PROXY_TEXTURE_3D_EXT, 0, GL_RGBA8, size[0]*2, 
                           size[1]*2, size[2]*2, 0, GL_RGBA, GL_UNSIGNED_BYTE, this->Volume2 );
    }
  

  GLint params[1];
  glGetTexLevelParameteriv ( GL_PROXY_TEXTURE_3D_EXT, 0, GL_TEXTURE_WIDTH, params ); 

  if ( params[0] != 0 ) 
    {
    return 1;
    }
  else
    {
    return 0;
    }
}

int vtkOpenGLVolumeTextureMapper3D::IsExtensionSupported(const char *extension)
{
  const GLubyte *extensions = NULL;
  const GLubyte *start;
  
  GLubyte *where, *terminator;

  where = (GLubyte *) strchr(extension, ' ');
  if (where || *extension == '\0')
    return 0;
  
  extensions = glGetString(GL_EXTENSIONS);
  
  start = extensions;
  while (1)
    {
    where = (GLubyte *) strstr((const char *) start, extension);
    if (!where)
      {
      break;
      }
    
    terminator = where + strlen(extension);
    if (where == start || *(where - 1) == ' ')
      {
      if (*terminator == ' ' || *terminator == '\0')
        {
        return true;
        }
      }
    
    start = terminator;
    }
  return false;
}

// Print the vtkOpenGLVolumeTextureMapper3D
void vtkOpenGLVolumeTextureMapper3D::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Initialized " << this->Initialized << endl;
  if ( this->Initialized )
    {
    os << indent << "Supports GL_EXT_texture3D:" 
       << this->IsExtensionSupported( "GL_EXT_texture3D" ) << endl;
    os << indent << "Supports GL_ARB_multitexture: " 
       << this->IsExtensionSupported( "GL_ARB_multitexture" ) << endl;
    os << indent << "Supports GL_NV_texture_shader2: " 
       << this->IsExtensionSupported( "GL_NV_texture_shader2" ) << endl;
    os << indent << "Supports GL_NV_register_combiners2: " 
       << this->IsExtensionSupported( "GL_NV_register_combiners2" ) << endl;
    os << indent << "Supports GL_ATI_fragment_shader: " 
       << this->IsExtensionSupported( "GL_ATI_fragment_shader" ) << endl;
    os << indent << "Supports GL_ARB_fragment_program: "
       << this->IsExtensionSupported( "GL_ARB_fragment_program" ) << endl;
    }

  this->Superclass::PrintSelf(os,indent);
}



