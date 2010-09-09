/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLineIntegralConvolution2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLineIntegralConvolution2D.h"

#include "vtkShader2.h"
#include "vtkTextureObject.h"
#include "vtkShaderProgram2.h"
#include "vtkUniformVariables.h"
#include "vtkShader2Collection.h"
#include "vtkFrameBufferObject.h"
#include "vtkOpenGLExtensionManager.h"

#include "vtkMath.h"
#include "vtkTimerLog.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderWindow.h"

#include <vtkstd/string>

extern const char *vtkLineIntegralConvolution2D_fs;
extern const char *vtkLineIntegralConvolution2D_fs1;
extern const char *vtkLineIntegralConvolution2D_fs2;

#include "vtkgl.h"

static const char * vtkLineIntegralConvolution2DCode = 
// $,$ are replaced with [x,y,z,w]
"vec2 getSelectedComponents(vec4 color)"
"{"
"  return color.$$;"
"}";

//#define VTK_LICDEBUGON

#ifdef VTK_LICDEBUGON
#define vtkLICDebug(x) cout << x << endl;
#else
#define vtkLICDebug(x)
#endif

vtkStandardNewMacro( vtkLineIntegralConvolution2D );

// Given the coordinate range of the vector texture, that of the resulting
// LIC texture, and the size of the output image, this function invokes the
// GLSL vertex and fragment shaders by issuing a command of rendering a quad
//
// vTCoords[4]:   a sub-region of the input vector field that is determined
//                by the view projection
//
// licTCoords[4]: the resulting LIC texture, of which the whole [ 0.0, 1.0 ] 
//                x [ 0.0,  1.0 ], though physically matching only a sub-
//                region of the input vector field, is always rendered 
//                
// width and height: the size (in number of pixels) of the output image
// 
void vtkRenderQuad( double vTCoords[4], double licTCoords[4],
                    unsigned int width, unsigned int height )
{
  // glTexCoord2f( tcoordx, tcoordy ) 
  // == vtkgl::MultiTexCoord2f( vtkgl::TEXTURE0, tcoordx, tcoordy ) 
  
  glBegin( GL_QUADS );
    
    // lower left
    vtkgl::MultiTexCoord2f(  vtkgl::TEXTURE0, 
                             static_cast<GLfloat>( licTCoords[0] ),
                             static_cast<GLfloat>( licTCoords[2] ) );
    vtkgl::MultiTexCoord2f(  vtkgl::TEXTURE1, 
                             static_cast< GLfloat >( vTCoords[0] ),
                             static_cast< GLfloat >( vTCoords[2] )  );
    glVertex2f( 0, 0 );
  
    // lower right  
    vtkgl::MultiTexCoord2f(  vtkgl::TEXTURE0, 
                             static_cast< GLfloat >( licTCoords[1] ),
                             static_cast< GLfloat >( licTCoords[2] )  );
    vtkgl::MultiTexCoord2f(  vtkgl::TEXTURE1, 
                             static_cast< GLfloat >( vTCoords[1] ),
                             static_cast< GLfloat >( vTCoords[2] )  );
    glVertex2f(  static_cast< GLfloat >( width ),  0  );
  
    // upper right
    vtkgl::MultiTexCoord2f(  vtkgl::TEXTURE0, 
                             static_cast< GLfloat >( licTCoords[1] ),
                             static_cast< GLfloat >( licTCoords[3] )  );
    vtkgl::MultiTexCoord2f(  vtkgl::TEXTURE1, 
                             static_cast< GLfloat >( vTCoords[1] ),
                             static_cast< GLfloat >( vTCoords[3] )  );
    glVertex2f(  static_cast< GLfloat >( width  ), 
                 static_cast< GLfloat >( height )  );
  
    // upper left
    vtkgl::MultiTexCoord2f(  vtkgl::TEXTURE0, 
                             static_cast< GLfloat >( licTCoords[0] ),
                             static_cast< GLfloat >( licTCoords[3] )  );
    vtkgl::MultiTexCoord2f(  vtkgl::TEXTURE1, 
                             static_cast< GLfloat >( vTCoords[0] ),
                             static_cast< GLfloat >( vTCoords[3] )  );
    glVertex2f(  0,  static_cast< GLfloat >( height )  );
    
    

  glEnd();
}

#define RENDERQUAD vtkRenderQuad( vTCoords, licTCoords, outWidth, outHeight ); 

#define UNIFORM1i(name, val)\
{\
  int uvar=shaderProg->GetUniformLocation(name);\
  vtkGraphicErrorMacro(context, "Get uniform " name);\
  if(uvar==-1)\
    {\
    vtkErrorMacro(<< name << " is not a uniform variable.");\
    }\
  else\
    {\
    vtkgl::Uniform1i(uvar, val);\
    vtkGraphicErrorMacro(context, "Setting " name);\
    }\
}

#define UNIFORM1f(name, val)\
{\
  int uvar=shaderProg->GetUniformLocation(name);\
  vtkGraphicErrorMacro(context, "Get uniform " name);\
  if(uvar==-1)\
    {\
    vtkErrorMacro(<< name << " is not a uniform variable.");\
    }\
  else\
    {\
    vtkgl::Uniform1f(uvar, val);\
    vtkGraphicErrorMacro(context, "Setting " name);\
    }\
}

#define UNIFORM2f(name, val0, val1)\
{\
  int uvar=shaderProg->GetUniformLocation(name);\
  vtkGraphicErrorMacro(context, "Get uniform " name);\
  if(uvar==-1)\
    {\
    vtkErrorMacro(<< name << " is not a uniform variable.");\
    }\
  else\
    {\
    vtkgl::Uniform2f(uvar, val0, val1);\
    vtkGraphicErrorMacro(context, "Setting " name);\
    }\
}

// ----------------------------------------------------------------------------
vtkLineIntegralConvolution2D::vtkLineIntegralConvolution2D()
{
  this->LIC   = NULL;
  this->Noise = NULL;
  this->VectorField = NULL;
  
  this->VectorShift   = 0.00;
  this->VectorScale   = 1.00;
  this->LICStepSize   = 0.01;
  this->NumberOfSteps = 1;
  
  this->GridSpacings[0]  = 1.0;
  this->GridSpacings[1]  = 1.0;
  this->ComponentIds[0]  = 0;
  this->ComponentIds[1]  = 1;
  
  this->EnhancedLIC      = 1; 
  this->LICForSurface    = 0;
  this->Magnification    = 1;
  this->TransformVectors = 1;
}

// ----------------------------------------------------------------------------
vtkLineIntegralConvolution2D::~vtkLineIntegralConvolution2D()
{
  if ( this->LIC )
    {
    this->LIC->Delete();
    this->LIC = NULL;
    }
    
  if ( this->Noise )
    {
    this->Noise->Delete();
    this->Noise = NULL;
   }
   
  if ( this->VectorField )
    {
    this->VectorField->Delete();
    this->VectorField = NULL;
     }
}

// ----------------------------------------------------------------------------
void vtkLineIntegralConvolution2D::SetLIC( vtkTextureObject * lic )
{
  vtkSetObjectBodyMacro( LIC, vtkTextureObject, lic );
}

// ----------------------------------------------------------------------------
void vtkLineIntegralConvolution2D::SetNoise( vtkTextureObject * noise )
{
  vtkSetObjectBodyMacro( Noise, vtkTextureObject, noise );
}

// ----------------------------------------------------------------------------
void vtkLineIntegralConvolution2D::SetVectorField
  ( vtkTextureObject * vectorField )
{
  vtkSetObjectBodyMacro( VectorField, vtkTextureObject, vectorField );
}

// ----------------------------------------------------------------------------
int vtkLineIntegralConvolution2D::Execute()
{
  unsigned int extent[4] = { 0, 0, 0, 0 };
  extent[1] = this->VectorField->GetWidth()  - 1;
  extent[3] = this->VectorField->GetHeight() - 1;
  
  return this->Execute( extent );
}

// ----------------------------------------------------------------------------
int vtkLineIntegralConvolution2D::Execute( int extent[4] )
{
  unsigned int uiExtent[4];
  
  for ( int i = 0; i < 4; i ++ )
    {
    if ( extent[i] < 0 )
      {
      vtkErrorMacro( "Invalid input extent." );
      return 0;
      }
      
    uiExtent[i] = static_cast< unsigned int >( extent[i] );
    }
    
  return this->Execute( uiExtent );
}

// ----------------------------------------------------------------------------
// checks if the context supports the required extensions
bool vtkLineIntegralConvolution2D::IsSupported(vtkRenderWindow *renWin)
{
  vtkOpenGLRenderWindow *w=static_cast<vtkOpenGLRenderWindow *>(renWin);

  // As we cannot figure out more accurately why the LIC algorithm does not
  // work on OpenGL 2.1/DX9 GPU, we discriminate an OpenGL3.0/DX10 GPU
  // (like a nVidia GeForce 8) against an OpenGL 2.1/DX9 GPU (like an nVidia
  // GeForce 6) by testing for geometry shader support, even if we are not
  // using any geometry shader in the LIC algorithm.

  vtkOpenGLExtensionManager *e=w->GetExtensionManager();
  bool supportGS=e->ExtensionSupported("GL_VERSION_3_0")==1 ||
    e->ExtensionSupported("GL_ARB_geometry_shader4")==1 ||
    e->ExtensionSupported("GL_EXT_geometry_shader4")==1;

  return supportGS && vtkTextureObject::IsSupported(renWin) &&
    vtkFrameBufferObject::IsSupported(renWin) &&
    vtkShaderProgram2::IsSupported(w);
}

// ----------------------------------------------------------------------------
int vtkLineIntegralConvolution2D::Execute( unsigned int extent[4] )
{
  // check the number of steps and step size
  if ( this->NumberOfSteps <= 0 )
    {
      vtkErrorMacro( "Number of integration steps should be positive." );
      return 0;
    }

  if ( this->LICStepSize <= 0.0 )
    {
      vtkErrorMacro( "Streamline integration step size should be positive." );
      return 0;
    }
    
  vtkTimerLog * timer = vtkTimerLog::New();
  timer->StartTimer();

  int components[2];
  components[0] = this->ComponentIds[0];
  components[1] = this->ComponentIds[1];

  if ( this->VectorField->GetComponents() < 2 )
    {
    vtkErrorMacro( "VectorField must have at least 2 components." );
    timer->Delete();
    timer = NULL;
    return 0;
    }

  // check the number of vector components
  if ( this->VectorField->GetComponents() == 2 )
    {
    // for 2 component textures (LA texture)
    components[0] = 0;
    components[1] = 3;
    }

  // given the two specified vector-compoment Ids, modify the source code of
  // the associated fragment shader such that the shader program can extract
  // the two target components from each 3D vector
  const char componentNames[] = { 'x', 0x0, 'y', 0x0, 'z', 0x0, 'w', 0x0 };
  vtkstd::string   additionalKernel = ::vtkLineIntegralConvolution2DCode;
  additionalKernel.replace(  additionalKernel.find( '$' ),  1,  
                            &componentNames[ 2 * components[0] ]  );
  additionalKernel.replace(  additionalKernel.find( '$' ),  1,  
                            &componentNames[ 2 * components[1] ]  );

  // size of the vector field (in number of pixels)
  unsigned int inWidth  = this->VectorField->GetWidth();
  unsigned int inHeight = this->VectorField->GetHeight();

  // Compute the transform for the vector field. This is a 2x2 diagonal matrix.
  // Hence, we only pass the non-NULL diagonal values.
  double vectorTransform[2] = { 1.0, 1.0 };
  if ( this->TransformVectors )
    {
    vectorTransform[0] = 1.0 / ( inWidth  * this->GridSpacings[0] );
    vectorTransform[1] = 1.0 / ( inHeight * this->GridSpacings[1] );
    }
  vtkLICDebug( "vectorTransform: " << vectorTransform[0] << ", " 
                                   << vectorTransform[1] );

  // size of the output LIC image
  unsigned int outWidth  = ( extent[1] - extent[0] + 1 ) 
                           * static_cast<unsigned int>( this->Magnification );
  unsigned int outHeight = ( extent[3] - extent[2] + 1 )
                           * static_cast<unsigned int>( this->Magnification );

  // a sub-region of the input vector field that is determined by projection
  double   vTCoords[4];
  vTCoords[0] = extent[0] / static_cast<double>( inWidth  - 1 ); // xmin
  vTCoords[1] = extent[1] / static_cast<double>( inWidth  - 1 ); // xmax
  vTCoords[2] = extent[2] / static_cast<double>( inHeight - 1 ); // xmin
  vTCoords[3] = extent[3] / static_cast<double>( inHeight - 1 ); // xmax
  
  // the resulting LIC texture, of which the whole [ 0.0, 1.0 ] x [ 0.0,  1.0 ], 
  // though physically matching only a sub-region of the input vector field, is 
  // always rendered 
  double licTCoords[4] = { 0.0, 1.0, 0.0, 1.0 };

  // obtain the rendering context
  vtkOpenGLRenderWindow * context =
  vtkOpenGLRenderWindow::SafeDownCast( this->VectorField->GetContext() );
  if (  !context->GetExtensionManager()->
                  LoadSupportedExtension( "GL_VERSION_1_3" )  )
    {
    vtkErrorMacro( "the required GL_VERSION_1_3 missing" );
    timer->Delete();
    timer   = NULL;
    context = NULL;
    return 0;
    }
  
  // pair #0: a 2D texture that stores the positions where particles released 
  // from the fragments (streamline centers) 'currently' are during integration.
  // Note that this texture is indexed for regular / non-center streamline
  // points only because the fragments' texture coordinates themselves are just
  // the initial positions of the streamline centers.
  // ( r, g ) == ( s, t ) tcoords; ( b ) == not-used.
  vtkTextureObject * tcords0 = vtkTextureObject::New();
  tcords0->SetContext( context );
  tcords0->Create2D( outWidth, outHeight, 3, VTK_FLOAT, false );
  vtkLICDebug( "texture object tcords0 Id = " << tcords0->GetHandle() );
  
  // pair #0: a 2D texture that stores the intermediate accumulated texture
  // values (r, g, b) for the fragments (and it is the output texture upon the
  // completion of the entire LIC process)
  vtkTextureObject * licTex0 = vtkTextureObject::New();
  licTex0->SetContext( context );
  licTex0->Create2D( outWidth, outHeight, 3, VTK_FLOAT, false );
  vtkLICDebug( "texture object licTex0 Id = " << licTex0->GetHandle() );
  
  // pair #1: a 2D texture that stores the positions where particles released 
  // from the fragments (streamline centers) 'currently' are during integration.
  // Note that this texture is indexed for regular / non-center streamline
  // points only because the fragments' texture coordinates themselves are just
  // the initial positions of the streamline centers.
  // ( r, g ) == ( s, t ) tcoords; ( b ) == not-used.
  vtkTextureObject * tcords1 = vtkTextureObject::New();
  tcords1->SetContext( context );
  tcords1->Create2D( outWidth, outHeight, 3, VTK_FLOAT, false );
  vtkLICDebug( "texture object tcords1 Id = " << tcords1->GetHandle() );
  
  // pair #1: a 2D texture that stores the intermediate accumulated texture
  // values (r, g, b) for the fragments (and it is the output texture upon the
  // completion of the entire LIC process)
  vtkTextureObject * licTex1 = vtkTextureObject::New();
  licTex1->SetContext( context );
  licTex1->Create2D( outWidth, outHeight, 3, VTK_FLOAT, false );
  vtkLICDebug( "texture object licTex1 Id = " << licTex1->GetHandle() );
  
  // a 2D texture that stores the output of the high-pass filtering (invoked
  // when enhanced LIC is desired)
  vtkTextureObject * lhpfTex = vtkTextureObject::New();
  lhpfTex->SetContext( context );
  lhpfTex->Create2D( outWidth, outHeight, 3, VTK_FLOAT, false );
  vtkLICDebug( "texture object lhpfTex Id = " << lhpfTex->GetHandle() );

  // frame buffer object that maintains multiple color buffers (texture objects)
  vtkFrameBufferObject * frameBufs = vtkFrameBufferObject::New();
  frameBufs->SetDepthBufferNeeded( false );
  frameBufs->SetContext( context );
  frameBufs->SetColorBuffer( 0, licTex0 );
  frameBufs->SetColorBuffer( 1, tcords0 );
  frameBufs->SetColorBuffer( 2, licTex1 );
  frameBufs->SetColorBuffer( 3, tcords1 );
  frameBufs->SetColorBuffer( 4, lhpfTex );
  frameBufs->SetNumberOfRenderTargets( 5 );
  
  // the four color buffers (texture objects) constitute two pairs (licTex0 with
  // tcords0 and licTex1 with tcords1), which work in a ping-pong fashion, with
  // one pair as the read texture objects, via
  //     vtkgl::ActiveTexture( vtkgl::TEXTURE2 );
  //     frameBufs->GetColorBuffer( pairX[0] )->Bind();
  //     vtkgl::ActiveTexture( vtkgl::TEXTURE3 );
  //     frameBufs->GetColorBuffer( pairX[1] )->Bind();
  //
  //     (note the input vector field and noise texture serve
  //     as vtkgl::TEXTURE0 and vtkgl::TEXTURE1, respectively)
  //
  // and the other pair as the write / render textures / targets, via 
  //     frameBufs->SetActiveBuffers( 2, pairY )
  unsigned int   pair0[2] = { 0, 1 };
  unsigned int   pair1[2] = { 2, 3 };
  unsigned int * pairs[2] = { pair0, pair1 };
 
  // create a shader program invoking the fragment shaders
  vtkShaderProgram2 * shaderProg = vtkShaderProgram2::New();
  shaderProg->SetContext( context );
  context = NULL;
  
  // load the supporting fragment shader that contains utilitiy functions
  vtkShader2 * utilities = vtkShader2::New();
  utilities->SetContext( shaderProg->GetContext() );
  utilities->SetType( VTK_SHADER_TYPE_FRAGMENT );
  utilities->SetSourceCode( vtkLineIntegralConvolution2D_fs );
  shaderProg->GetShaders()->AddItem( utilities );
  utilities->Delete();
  
  // load the supporting fragment shader program that tells which two
  // components are needed from each 3D vector
  vtkShader2 * selectComps = vtkShader2::New();
  selectComps->SetContext( shaderProg->GetContext() );
  selectComps->SetType( VTK_SHADER_TYPE_FRAGMENT );
  selectComps->SetSourceCode( additionalKernel.c_str() );
  shaderProg->GetShaders()->AddItem( selectComps );
  selectComps->Delete();
  
  // load the fragment shader program that implements the LIC process
  vtkShader2 * glslFS1 = vtkShader2::New();
  glslFS1->SetContext( shaderProg->GetContext() );
  glslFS1->SetType( VTK_SHADER_TYPE_FRAGMENT );
  glslFS1->SetSourceCode( vtkLineIntegralConvolution2D_fs1 );
  
  // load the fragment shader program that implements high-pass filtering
  vtkShader2 * glslFS2 = vtkShader2::New();
  glslFS2->SetContext( shaderProg->GetContext() );
  glslFS2->SetType( VTK_SHADER_TYPE_FRAGMENT );
  glslFS2->SetSourceCode( vtkLineIntegralConvolution2D_fs2 );
 
  // build the LIC fragment shader
  vtkLICDebug( "building the LIC fragment shader (pass #1)" );
  shaderProg->GetShaders()->AddItem( glslFS1 );
  shaderProg->Build();
  if ( shaderProg->GetLastBuildStatus() != VTK_SHADER_PROGRAM2_LINK_SUCCEEDED )
    {
    vtkErrorMacro( "error with building the LIC fragment shader (pass #1)" );
    return 0;
    }
  vtkLICDebug( "the LIC fragment shader (pass #1) built" );
  
  // input texture #0: the vector field, bound as TEXTURE0
  vtkgl::ActiveTexture( vtkgl::TEXTURE0 );
  this->VectorField->Bind();
  glTexParameteri( this->VectorField->GetTarget(), 
                   GL_TEXTURE_MIN_FILTER, GL_LINEAR );
  glTexParameteri( this->VectorField->GetTarget(), 
                   GL_TEXTURE_MAG_FILTER, GL_LINEAR );
  vtkLICDebug( "texture object vectorField Id=" 
                  << this->VectorField->GetHandle() );
  
  // input texture #1: the noise texture, bound as TEXTURE1
  vtkgl::ActiveTexture( vtkgl::TEXTURE1 );
  this->Noise->Bind();
  vtkLICDebug( "texture object Noise Id = " << this->Noise->GetHandle() );

  // determine the noise scale factor that allows for the use of a noise texture 
  // smaller than the input vector field and the output image
  double noiseScale[2] = { 1.0, 1.0 };
  noiseScale[0] = this->Magnification * this->VectorField->GetWidth() /
                  static_cast<double>( this->Noise->GetWidth() );
  noiseScale[1] = this->Magnification * this->VectorField->GetHeight() /
                  static_cast<double>( this->Noise->GetHeight() );
  vtkLICDebug( "noiseScale: " << noiseScale[0] << ", " << noiseScale[1] );
  
  // set the parameters for the LIC fragment shader
  int   value;
  float fvalues[2];
  value = this->LICForSurface;
  shaderProg->GetUniformVariables()->SetUniformi( "uSurfaced", 1, &value  );
  value = 1 - this->EnhancedLIC; // it is the last one if EnancedLIC is OFF
  shaderProg->GetUniformVariables()->SetUniformi( "uLastPass",  1,&value );
  value = 0;
  shaderProg->GetUniformVariables()->SetUniformi( "uMaskType", 1, &value  );
  value = this->NumberOfSteps;
  shaderProg->GetUniformVariables()->SetUniformi( "uNumSteps", 1, &value  );
  fvalues[0] = static_cast<float>( this->LICStepSize );
  shaderProg->GetUniformVariables()->SetUniformf( "uStepSize", 1, fvalues );
  fvalues[0] = static_cast<float>( this->VectorShift );
  fvalues[1] = static_cast<float>( this->VectorScale );
  shaderProg->GetUniformVariables()->SetUniformf
                                     ( "uVectorShiftScale", 2, fvalues );
  fvalues[0] = static_cast<float>( noiseScale[0] );
  fvalues[1] = static_cast<float>( noiseScale[1] );
  shaderProg->GetUniformVariables()->SetUniformf
                                     ( "uNoise2VecScaling", 2, fvalues );
  fvalues[0] = static_cast<float>( vectorTransform[0] );
  fvalues[1] = static_cast<float>( vectorTransform[1] );
  shaderProg->GetUniformVariables()->SetUniformf
                                     ( "uVectorTransform2", 2, fvalues );
  
  float vtCordRange[4];
  vtCordRange[0] = static_cast<float> ( vTCoords[0] );
  vtCordRange[1] = static_cast<float> ( vTCoords[1] );
  vtCordRange[2] = static_cast<float> ( vTCoords[2] );
  vtCordRange[3] = static_cast<float> ( vTCoords[3] );
  shaderProg->GetUniformVariables()->SetUniformf
                                     ( "uVTCordRenderBBox", 4, vtCordRange );
  
  value = 0;
  shaderProg->GetUniformVariables()->SetUniformi
                                     ( "uNTCordShiftScale", 1, &value );
  
  // Declare the first two input texture objects of the fragment shader by
  // specifying their names referenced in the shader.
  // These two texture objects correspond to this->VectorField (bound to 
  // vtkgl::TEXTURE0) and this->Noise (bound to vtkgl::TEXTURE1), respectively.
  value = 0;
  shaderProg->GetUniformVariables()->SetUniformi( "texVectorField", 1, &value );
  value = 1;
  shaderProg->GetUniformVariables()->SetUniformi( "texNoise", 1, &value );

  // Declare the last two input texture objects of the fragment shader by
  // specifying their names referenced in the shader. 
  // Note that these two texture objects are dynamically determind and bound
  // (to vtkgl::TEXTURE2 and vtkgl::TEXTURE3, respectively, below) as the two
  // pairs of color buffers (tcords0 with licTex0 and tcords1 with licTex1)
  // work in a ping-pong manner during the LIC process.
  value = 2;
  shaderProg->GetUniformVariables()->SetUniformi( "texLIC", 1, &value );
  value = 3;
  shaderProg->GetUniformVariables()->SetUniformi( "texTCoords", 1,&value );
  
  shaderProg->Use();
  
  int            readIndex = 0; // index of the pair used as the read buffers
  unsigned int * readBuffs = NULL;
  unsigned int * writeBufs = NULL;
  for ( int direction = 0; direction < 2; direction ++ )
    {
    // NOTE: this->NumberOfSteps + 1 is used below beause the streamline center
    //       point is actually visited two times (due to the outer loop), 
    //       one per integration direction. Thus ( this->NumberOfSteps + 1 ) * 
    //       2 visits access ( this->NumberOfSteps + 1 ) * 2 - 1 = 2 * this->
    //       NumberOfSteps + 1 unique streamline points.
    //       
    //       The associated fragment shader addresses this issue by asking
    //       each center-visit to contribute half the texture value.
    for ( int stepIdx = 0; stepIdx < this->NumberOfSteps + 1; stepIdx ++ )
      {
      // determine the pair of color buffers, among the four of the frame 
      // buffer object, used as the input and the one used as the output
      readIndex = ( stepIdx % 2 );
      readBuffs = pairs[     readIndex ];
      writeBufs = pairs[ 1 - readIndex ];
      
      // specify the 2D texture that stores the intermediate accumulated 
      // texture values (r, g, b) for the fragments 
      vtkgl::ActiveTexture( vtkgl::TEXTURE2 );
      vtkTextureObject * accumLIC = frameBufs->GetColorBuffer( readBuffs[0] );
      accumLIC->Bind();
      vtkLICDebug( "accumLIC: " << accumLIC->GetHandle() );
      accumLIC = NULL;

      // Specify the 2D texture that stores the positions where particles
      // released from the fragments (streamline centers) 'currently' are. 
      // Note this texture is indexed for regular / non-center streamline
      // points only because the fragments' texture coordinates themselves
      // are just the initial positions of the streamline centers.
      // ( r, g ) == ( s, t ) tcoords; ( b ) == not-used.
      vtkgl::ActiveTexture( vtkgl::TEXTURE3 );
      vtkTextureObject * dynaTcords = frameBufs->GetColorBuffer( readBuffs[1] );
      dynaTcords->Bind();
      vtkLICDebug( "dynaTcords: " << dynaTcords->GetHandle() );
      dynaTcords = NULL;
      
      // specify the pair of texture objects as the render targets
      frameBufs->SetActiveBuffers( 2, writeBufs );
      if (  !frameBufs->Start( outWidth, outHeight, false )  )
        {
        shaderProg->GetShaders()->RemoveItem( glslFS1 );
        shaderProg->GetShaders()->RemoveItem( utilities );
        shaderProg->GetShaders()->RemoveItem( selectComps );
        
        glslFS1->ReleaseGraphicsResources();
        glslFS2->ReleaseGraphicsResources();
        utilities->ReleaseGraphicsResources();
        selectComps->ReleaseGraphicsResources();
        shaderProg->ReleaseGraphicsResources();
        glslFS1->Delete();
        glslFS2->Delete();
        shaderProg->Delete();
        
        frameBufs->Delete();
        tcords0->Delete();
        tcords1->Delete();
        licTex0->Delete();
        licTex1->Delete();
        lhpfTex->Delete();
        timer->Delete();
        
        glslFS1    = NULL;
        glslFS2    = NULL;
        utilities  = NULL;
        selectComps= NULL;
        shaderProg = NULL;
        frameBufs  = NULL;
        tcords0    = NULL;
        tcords1    = NULL;
        licTex0    = NULL;
        licTex1    = NULL;
        lhpfTex    = NULL;
        timer      = NULL;
        
        readBuffs  = NULL;
        writeBufs  = NULL;
        pairs[0]   = NULL;
        pairs[1]   = NULL;
        
        return 0;
        }
      vtkLICDebug( "active render buffers Ids: " << writeBufs[0] << ", " 
                      << writeBufs[1] << " for step #" << stepIdx );

      // streamline integration direction: negative (-1) and positive (1)
      value = ( direction << 1 ) - 1;
      shaderProg->GetUniformVariables()->SetUniformi( "uStepSign", 1, &value );
      
      // step type (0, 1, 2)
      // 0: first access to the streamline center point
      // 1: access to a regular / non-center streamline point
      // 2: second access to the streamline center point
      //    (due to a change in the streamline integration direction)
      value = 1 + ( !stepIdx ) * (  ( direction << 1 ) - 1  );
      shaderProg->GetUniformVariables()->SetUniformi( "uStepType", 1, &value );
      
      // zero-vector fragment masking
      // 0: retain the white noise texture value by storing the negated version
      // 1: export ( -1.0, -1.0, -1.0, -1.0 ) for use by vtkSurfaceLICPainter
      //    to make this LIC fragment totally transparent to show the underlying
      //    geometry surface
      //
      // a zero-vector fragment is always masked with ( -1.0, -1.0, -1.0, -1.0)
      // IF we need a basic LIC image (instead of an improved one) for display
      value = int(  ( direction == 1 ) && ( stepIdx == this->NumberOfSteps ) && 
                    ( this->EnhancedLIC == 0 )
                 );
      shaderProg->GetUniformVariables()->SetUniformi( "uMaskType", 1, &value );
      
      shaderProg->SendUniforms(); // force resending uniforms
      if( !shaderProg->IsValid() )
        {
        vtkErrorMacro( << " validation of the program failed: "
                       << shaderProg->GetLastValidateLog() );
        }
      
      RENDERQUAD
      }
    }
   
  if ( this->EnhancedLIC )
    {
    // --------------------------------------------- begin high-pass filtering
    // perform Laplacian high-pass filtering using a fragment shader
    shaderProg->Restore();
    vtkLICDebug( "unbinding the LIC fragment shader (pass #1) ... " );
  
    shaderProg->GetShaders()->RemoveItem( glslFS1 );
    shaderProg->GetShaders()->AddItem( glslFS2 );
  
    vtkLICDebug( "building the high-pass filtering shader ... " );
    shaderProg->Build();
    if( shaderProg->GetLastBuildStatus() != VTK_SHADER_PROGRAM2_LINK_SUCCEEDED )
      {
      vtkErrorMacro( "error with bulding the high-pass filtering shader" );
      return 0;
      }
  
    // set parameters for the high-pass filtering shader and declare the only one
    // input texture by specifying its name referenced in the shader 
    value = 0;
    float licWidth  = static_cast<float>( outWidth  );
    float licHeight = static_cast<float>( outHeight );
    shaderProg->GetUniformVariables()->SetUniformi( "licTexture", 1, &value     );
    shaderProg->GetUniformVariables()->SetUniformf( "uLicTexWid", 1, &licWidth  );
    shaderProg->GetUniformVariables()->SetUniformf( "uLicTexHgt", 1, &licHeight );
   
    // determine the read and write / render textures for the high-pass filter
    unsigned int filterReadIdx = writeBufs[0]; // the output of pass #1 LIC
    unsigned int filterWriteId = 4;            // texture object lhpfTex
  
    // bind the input texture to the filter
    vtkgl::ActiveTexture( vtkgl::TEXTURE0 );
    vtkTextureObject * licImage = frameBufs->GetColorBuffer( filterReadIdx );
    licImage->Bind();
    licImage = NULL;
   
    // set the output texture of the filter as the active one of the FBO
    vtkLICDebug( "active render buffer Id: " << filterWriteId );
    frameBufs->SetActiveBuffers( 1, &filterWriteId );
    if (  !frameBufs->Start( outWidth, outHeight, false )  )
      {
      shaderProg->GetShaders()->RemoveItem( glslFS2 );
      shaderProg->GetShaders()->RemoveItem( utilities );
      shaderProg->GetShaders()->RemoveItem( selectComps );
        
      glslFS1->ReleaseGraphicsResources();
      glslFS2->ReleaseGraphicsResources();
      utilities->ReleaseGraphicsResources();
      selectComps->ReleaseGraphicsResources();
      shaderProg->ReleaseGraphicsResources();
      glslFS1->Delete();
      glslFS2->Delete();
      shaderProg->Delete();
      
      frameBufs->Delete();
      tcords0->Delete();
      tcords1->Delete();
      licTex0->Delete();
      licTex1->Delete();
      lhpfTex->Delete();
      timer->Delete();
      
      glslFS1    = NULL;
      glslFS2    = NULL;
      utilities  = NULL;
      selectComps= NULL;
      shaderProg = NULL;
      frameBufs  = NULL;
      tcords0    = NULL;
      tcords1    = NULL;
      licTex0    = NULL;
      licTex1    = NULL;
      lhpfTex    = NULL;
      timer      = NULL;
      
      readBuffs  = NULL;
      writeBufs  = NULL;
      pairs[0]   = NULL;
      pairs[1]   = NULL;
      
      return 0;
      }
  
    shaderProg->Use();
    if( !shaderProg->IsValid() )
      {
      vtkErrorMacro( "error validating the high-pass filtering shader " 
                     << shaderProg->GetLastValidateLog() );
      }
    
    // invoke the high-pass filter by rendering the quad
    RENDERQUAD 
    // --------------------------------------------- end  high-pass  filtering
  
  
    // --------------------------------------------- begin   second-pass   LIC
    shaderProg->Restore();
    vtkLICDebug( "unbinding the high-pass filtering shader ... " );
  
    shaderProg->GetShaders()->RemoveItem( glslFS2 );
    shaderProg->GetShaders()->AddItem( glslFS1 );
  
    vtkLICDebug( "building the LIC fragment shader (pass #2) ... " );
    shaderProg->Build();
    if( shaderProg->GetLastBuildStatus() != VTK_SHADER_PROGRAM2_LINK_SUCCEEDED )
      {
      vtkErrorMacro( "error with bulding the LIC fragment shader (pass #2)" );
      return 0;
      }
      
    // this is the last pass of LIC (for non-suraceLIC, make sure the output
    // pixel values are all positive since neither high-pass filtering nor
    // geometry-LIC compositing is performed at all)
    value = 1;
    shaderProg->GetUniformVariables()->SetUniformi( "uLastPass",  1,&value );
      
    // As pass #1 LIC has constructed the basic flow pattern (the tangential
    // flow streaks have been curved 'out') and then the high-pass filter has
    // even enhanced it, pass #2 LIC can save some integration steps and instead
    // is focused on smoothing away those noisy components (those excessively
    // contrasted fragments).
    int lic2Steps = this->NumberOfSteps / 2;
    shaderProg->GetUniformVariables()->SetUniformi( "uNumSteps", 1, &lic2Steps );
    
    // When the output of pass #1 LIC is high-pass filtered and then forwarded
    // to pass #2 LIC as the input 'noise', the size of this 'noise' texture
    // (uVTCordRenderBBox) is equal to the current extent of the vector 
    // field (vTCoords[4]) times this->Magnification. Since noiseScale (or 
    // uNoise2VecScaling) involves this->Magnification and hence the value of
    // uNoise2VecScaling for pass #2 LIC is just vec2(1.0, 1.0) AS LONG AS we
    // take this 'noise' texture as an extent (uVTCordRenderBBox = vTCoords[4]) 
    // of the virtual full 'noise' texture (for which the out-of-extent part
    // is just not defined / provided by the output of the high-pass filter ---
    // 'virtual'). To compensate for the effect of the 'extent', the vector 
    // field-based noise texture coordinate needs to be shifted and scaled in
    // vtkLineIntegralConvolution2D_fs.glsl::getNoiseColor() to index this
    // 'noise' texture (an extent of the virtual full 'noise' texture) properly.
    fvalues[0] = 1.0;
    fvalues[1] = 1.0;
    shaderProg->GetUniformVariables()->SetUniformf( "uNoise2VecScaling", 2, fvalues );
    value = 1;
    shaderProg->GetUniformVariables()->SetUniformi( "uNTCordShiftScale", 1, &value );
    shaderProg->SendUniforms();
  
    // bind the vector field as an input texture
    vtkgl::ActiveTexture( vtkgl::TEXTURE0 );
    this->VectorField->Bind();
  
    // replace the original white noise texture with a new 'noise' texture (the
    // output generated by high-pass filtering pass #1 LIC image) and bind it
    vtkgl::ActiveTexture( vtkgl::TEXTURE1 );
    this->Noise->UnBind();
    vtkTextureObject * tempTex = frameBufs->GetColorBuffer( 4 );
    tempTex->Bind();
    tempTex = NULL;
  
    shaderProg->Use(); 
    
    for ( int direction = 0; direction < 2; direction ++ )
      {
      // NOTE: lic2Steps + 1 is used below beause the streamline center point
      //       is actually visited two times (due to the outer loop),  one per
      //       integration direction. Thus ( lic2Steps + 1 ) * 2 visits access
      //       ( lic2Steps + 1 ) * 2 - 1 = 2 * lic2Steps + 1 unique streamline
      //       points.
      //       
      //       The associated fragment shader addresses this issue by asking
      //       each center-visit to contribute half the texture value.
      for ( int stepIdx = 0; stepIdx < lic2Steps + 1; stepIdx ++ )
        {
        // determine the pair of color buffers, among the four of the frame 
        // buffer object, used as the input and the one used as the output
        readIndex = ( stepIdx % 2 );
        readBuffs = pairs[     readIndex ];
        writeBufs = pairs[ 1 - readIndex ];
      
        // specify the 2D texture that stores the intermediate accumulated 
        // texture values (r, g, b) for the fragments and bind it as an input
        vtkgl::ActiveTexture( vtkgl::TEXTURE2 );
        vtkTextureObject * accumLIC = frameBufs->GetColorBuffer( readBuffs[0] );
        accumLIC->Bind();
        vtkLICDebug( "accumLIC: " << accumLIC->GetHandle() );
        accumLIC = NULL;

        // Specify the 2D texture that stores the positions where particles
        // released from the fragments (streamline centers) 'currently' are. 
        // Note this texture is indexed for regular / non-center streamline
        // points only because the fragments' texture coordinates themselves
        // are just the initial positions of the streamline centers.
        // (r, g) == (s, t) tcoords; (b) == not-used.
        vtkgl::ActiveTexture( vtkgl::TEXTURE3 );
        vtkTextureObject * dynaTcords = frameBufs->GetColorBuffer( readBuffs[1] );
        dynaTcords->Bind();
        vtkLICDebug( "dynaTcords: " << dynaTcords->GetHandle() );
        dynaTcords = NULL;
      
        // specify the pair of texture objects as the render targets
        frameBufs->SetActiveBuffers( 2, writeBufs );
        if (  !frameBufs->Start( outWidth, outHeight, false )  )
          {
          shaderProg->GetShaders()->RemoveItem( glslFS1 );
          shaderProg->GetShaders()->RemoveItem( utilities );
          shaderProg->GetShaders()->RemoveItem( selectComps );
        
          glslFS1->ReleaseGraphicsResources();
          glslFS2->ReleaseGraphicsResources();
          utilities->ReleaseGraphicsResources();
          selectComps->ReleaseGraphicsResources();
          shaderProg->ReleaseGraphicsResources();
          glslFS1->Delete();
          glslFS2->Delete();
          shaderProg->Delete();
      
          frameBufs->Delete();
          tcords0->Delete();
          tcords1->Delete();
          licTex0->Delete();
          licTex1->Delete();
          lhpfTex->Delete();
          timer->Delete();
      
          glslFS1    = NULL;
          glslFS2    = NULL;
          utilities  = NULL;
          selectComps= NULL;
          shaderProg = NULL;
          frameBufs  = NULL;
          tcords0    = NULL;
          tcords1    = NULL;
          licTex0    = NULL;
          licTex1    = NULL;
          lhpfTex    = NULL;
          timer      = NULL;
      
          readBuffs  = NULL;
          writeBufs  = NULL;
          pairs[0]   = NULL;
          pairs[1]   = NULL;
      
          return 0;
          }
        vtkLICDebug( "active render buffers Ids: " << writeBufs[0] << ", " 
                        << writeBufs[1] << " for step #" << stepIdx );

        // streamline integration direction: negative (-1) and positive (1)
        value = ( direction << 1 ) - 1;
        shaderProg->GetUniformVariables()->SetUniformi( "uStepSign", 1, &value );
        
        // step type (0, 1, 2)
        // 0: first access to the streamline center point
        // 1: access to a regular / non-center streamline point
        // 2: second access to the streamline center point
        //    (due to a change in the streamline integration direction)
        value = 1 + ( !stepIdx ) * (  ( direction << 1 ) - 1  );
        shaderProg->GetUniformVariables()->SetUniformi( "uStepType", 1, &value );
      
        // zero-vector fragment masking
        // 0: retain the white noise texture value by storing the negated version
        // 1: export ( -1.0, -1.0, -1.0, -1.0 ) for use by vtkSurfaceLICPainter
        //    to make this LIC fragment totally transparent to show the underlying
        //    geometry surface 
        value = int(  ( direction == 1 ) && ( stepIdx == lic2Steps )  );
        shaderProg->GetUniformVariables()->SetUniformi( "uMaskType", 1, &value );
     
        shaderProg->SendUniforms(); // force resending uniforms
        if ( !shaderProg->IsValid() )
          {
          vtkErrorMacro( << " validation of the program failed: "
                       << shaderProg->GetLastValidateLog() );
          }
      
        RENDERQUAD
        }
      }
    // --------------------------------------------- end    second-pass    LIC
    }
    
  
  glFinish();
  timer->StopTimer();
  shaderProg->Restore();
  vtkLICDebug( "Exec Time: " <<  timer->GetElapsedTime() );
  timer->Delete();
  timer = NULL;
  
  
  // obtain the LIC image, either basic LIC or enhanced LIC
  this->LIC = frameBufs->GetColorBuffer( writeBufs[0] ); // accept one licTex
  frameBufs->GetColorBuffer( readBuffs[0] )->Delete();   // free other licTex
  
  
  // memory deallocation (NOTE: do not deallocate licTex0 and licTex1 below
  // since one is deallocated above and the other is deallocated via this->
  // LIC upon the destruction of this class)
  glslFS1->ReleaseGraphicsResources();
  glslFS2->ReleaseGraphicsResources();
  utilities->ReleaseGraphicsResources();
  selectComps->ReleaseGraphicsResources();
  shaderProg->ReleaseGraphicsResources();
  glslFS1->Delete();
  glslFS2->Delete();
  shaderProg->Delete();
  frameBufs->Delete();
  tcords0->Delete();
  tcords1->Delete();
  lhpfTex->Delete();
  glslFS1    = NULL;
  glslFS2    = NULL;
  utilities  = NULL;
  selectComps= NULL;
  shaderProg = NULL;
  frameBufs  = NULL;
  tcords0    = NULL;
  tcords1    = NULL;
  licTex0    = NULL;
  licTex1    = NULL;
  lhpfTex    = NULL;
  readBuffs  = NULL;
  writeBufs  = NULL;
  
  return 1;
}

//-----------------------------------------------------------------------------
void vtkLineIntegralConvolution2D::PrintSelf( ostream & os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  
  os << indent << "LIC: "              << this->LIC              << "\n";
  os << indent << "Noise: "            << this->Noise            << "\n";
  os << indent << "VectorField: "      << this->VectorField      << "\n";
  
  os << indent << "EnahncedLIC: "      << this->EnhancedLIC      << "\n";
  os << indent << "LICStepSize: "      << this->LICStepSize      << "\n";
  os << indent << "VectorShift: "      << this->VectorShift      << "\n";
  os << indent << "VectorScale: "      << this->VectorScale      << "\n";
  os << indent << "Magnification: "    << this->Magnification    << "\n";
  os << indent << "NumberOfSteps: "    << this->NumberOfSteps    << "\n";
  os << indent << "ComponentIds: "     << this->ComponentIds[0]  << ", "
                                       << this->ComponentIds[1]  << "\n";
  os << indent << "GridSpacings: "     << this->GridSpacings[0]  << ", "
                                       << this->GridSpacings[1]  << "\n";
  os << indent << "LICForSurface: "    << this->LICForSurface    << "\n";
  os << indent << "TransformVectors: " << this->TransformVectors << "\n";
}
