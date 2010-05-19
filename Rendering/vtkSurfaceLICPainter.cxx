/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSurfaceLICPainter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSurfaceLICPainter.h"

#include "vtkBase64Utilities.h"
#include "vtkBoundingBox.h"
#include "vtkCellData.h"
#include "vtkGarbageCollector.h"
#include "vtkGenericDataObjectReader.h"
#include "vtkImageData.h"
#include "vtkColorMaterialHelper.h"
#include "vtkDataTransferHelper.h"
#include "vtkFrameBufferObject.h"
#include "vtkLightingHelper.h"
#include "vtkLineIntegralConvolution2D.h"
#include "vtkNoise200x200.h"
#include "vtkShaderProgram2.h"
#include "vtkShader2.h"
#include "vtkShader2Collection.h"
#include "vtkUniformVariables.h"
#include "vtkTextureObject.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkTimerLog.h"
#include "vtkWeakPointer.h"

#include <assert.h>
#include "vtkgl.h"
#include <vtkstd/string>

#define vtkGetIndex(r,c)    (c*4+r)
extern const char* vtkSurfaceLICPainter_fs1;
extern const char* vtkSurfaceLICPainter_vs1;
extern const char* vtkSurfaceLICPainter_fs2;

inline double vtkClamp(double val, const double& min, const double& max)
{
  val = (val < min)? min : val;
  val = (val > max)? max : val;
  return val;
}

class vtkSurfaceLICPainter::vtkInternals
{
public:
  vtkWeakPointer<vtkOpenGLRenderWindow> LastRenderWindow;
  int LastViewportSize[2];

  // Extent relative to the viewport origin.
  unsigned int ViewportExtent[4];

  vtkSmartPointer<vtkFrameBufferObject> FBO;
  vtkSmartPointer<vtkTextureObject> VelocityImage;
  vtkSmartPointer<vtkTextureObject> GeometryImage;
  vtkSmartPointer<vtkTextureObject> NoiseImage;
  vtkSmartPointer<vtkShaderProgram2> PassOne;
  vtkSmartPointer<vtkShaderProgram2> PassTwo;
  vtkSmartPointer<vtkLightingHelper> LightingHelper;
  vtkSmartPointer<vtkColorMaterialHelper> ColorMaterialHelper;
  vtkSmartPointer<vtkImageData> Noise;

  int FieldAssociation;
  int FieldAttributeType;
  vtkstd::string FieldName;
  bool FieldNameSet;

  // Some internal flags.
  bool HasVectors;

  vtkInternals()
    {
    this->LastViewportSize[0] = this->LastViewportSize[1] = 0;
    this->HasVectors = false;
    this->FieldNameSet = false;
    this->FieldAttributeType = 0;
    this->FieldAssociation = 0;
    this->LightingHelper = vtkSmartPointer<vtkLightingHelper>::New();
    this->ColorMaterialHelper = vtkSmartPointer<vtkColorMaterialHelper>::New();
    }

  void ClearTextures()
    {
    this->VelocityImage = 0;
    this->GeometryImage = 0;
    this->NoiseImage = 0;
    if (this->FBO)
      {
      this->FBO->RemoveAllColorBuffers();
      }
    }

  void ClearGraphicsResources()
    {
    this->ClearTextures();
    this->FBO = 0;
    this->VelocityImage = 0;
    this->GeometryImage = 0;
    this->NoiseImage = 0;
    if(this->PassOne!=0)
      {
      this->PassOne->ReleaseGraphicsResources();
      this->PassOne = 0;
      }
    if(this->PassTwo!=0)
      {
      this->PassTwo->ReleaseGraphicsResources();
      this->PassTwo = 0;
      }
    this->LightingHelper->Initialize(0,VTK_SHADER_TYPE_VERTEX);
    this->ColorMaterialHelper->Initialize(0);
    }
};

vtkStandardNewMacro(vtkSurfaceLICPainter);
//----------------------------------------------------------------------------
vtkSurfaceLICPainter::vtkSurfaceLICPainter()
{
  this->Internals     = new vtkInternals();
  this->Output        = 0;
  this->Enable        = 1;
  this->StepSize      = 1;
  this->EnhancedLIC   = 1;
  this->LICIntensity  = 0.8;
  this->NumberOfSteps = 20;
  this->LICSuccess    = 0;
  this->RenderingPreparationSuccess = 0;

  this->SetInputArrayToProcess(vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS,
    vtkDataSetAttributes::VECTORS);
}

//----------------------------------------------------------------------------
vtkSurfaceLICPainter::~vtkSurfaceLICPainter()
{
  this->ReleaseGraphicsResources(this->Internals->LastRenderWindow);
  delete this->Internals;

  if (this->Output)
    {
    this->Output->Delete();
    this->Output = 0;
    }
}

//----------------------------------------------------------------------------
void vtkSurfaceLICPainter::SetInputArrayToProcess(int fieldAssociation,
  const char* name)
{
  if (this->Internals->FieldAssociation != fieldAssociation || 
    !this->Internals->FieldNameSet ||
    this->Internals->FieldName != name)
    {
    this->Internals->FieldAssociation = fieldAssociation;
    this->Internals->FieldName = name;
    this->Internals->FieldNameSet = true;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkSurfaceLICPainter::SetInputArrayToProcess(int fieldAssociation,
  int fieldAttributeType)
{
  if (this->Internals->FieldAssociation != fieldAssociation || 
    this->Internals->FieldNameSet ||
    this->Internals->FieldAttributeType != fieldAttributeType)
    {
    this->Internals->FieldAssociation = fieldAssociation;
    this->Internals->FieldNameSet = false;
    this->Internals->FieldAttributeType = fieldAttributeType;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkSurfaceLICPainter::ReleaseGraphicsResources(vtkWindow* win)
{
  this->Internals->ClearGraphicsResources();
  this->Internals->LastRenderWindow = 0;

  this->Superclass::ReleaseGraphicsResources(win);
}

static vtkImageData* vtkGetNoiseResource()
{
  vtkstd::string base64string;
  for (unsigned int cc=0; cc < file_noise200x200_vtk_nb_sections; cc++)
    {
      base64string += reinterpret_cast<const char*>(file_noise200x200_vtk_sections[cc]);
    }

  unsigned char* binaryInput = new unsigned char[file_noise200x200_vtk_decoded_length + 10];
  unsigned long binarylength = vtkBase64Utilities::Decode(
    reinterpret_cast<const unsigned char*>(base64string.c_str()), static_cast<unsigned long>(base64string.length()),
    binaryInput);
  assert("check valid_length" && binarylength == file_noise200x200_vtk_decoded_length);

  vtkGenericDataObjectReader* reader = vtkGenericDataObjectReader::New();
  reader->ReadFromInputStringOn();
  reader->SetBinaryInputString(reinterpret_cast<char*>(binaryInput), static_cast<int>(binarylength));
  reader->Update();

  vtkImageData* data = vtkImageData::New();
  data->ShallowCopy(reader->GetOutput());

  delete [] binaryInput;
  reader->Delete();
  return data;
}

//----------------------------------------------------------------------------
bool vtkSurfaceLICPainter::CanRenderLIC
   ( vtkRenderer * vtkNotUsed(renderer), vtkActor * actor )
{
  return ( this->Enable && this->Internals->HasVectors &&
           actor->GetProperty()->GetRepresentation() == VTK_SURFACE );
}

//----------------------------------------------------------------------------
bool vtkSurfaceLICPainter::IsSupported( vtkRenderWindow * renWin )
{
  return (  vtkDataTransferHelper::IsSupported( renWin ) &&
            vtkLineIntegralConvolution2D::IsSupported( renWin )  );
}

//----------------------------------------------------------------------------
void vtkSurfaceLICPainter::PrepareForRendering
   ( vtkRenderer * renderer, vtkActor * actor )
{
  if ( !this->PrepareOutput() )
    {
    this->RenderingPreparationSuccess = 0;
    return;
    }

  if (  !this->CanRenderLIC( renderer, actor )  )
    {
    this->ReleaseGraphicsResources( this->Internals->LastRenderWindow );
    this->Superclass::PrepareForRendering( renderer, actor );
    this->RenderingPreparationSuccess = 0;
    return;
    }
    
  vtkOpenGLRenderWindow * renWin = vtkOpenGLRenderWindow::SafeDownCast
                                   ( renderer->GetRenderWindow() );
    
  if (  !this->IsSupported( renWin )  )
    {
    this->RenderingPreparationSuccess = 0;
    renWin = NULL;
    return;
    }

  if ( !this->Internals->Noise )
    {
    vtkImageData * noise = ::vtkGetNoiseResource();
    this->Internals->Noise = noise;
    noise->Delete();
    noise = NULL;
    }

  if ( this->Internals->LastRenderWindow && 
       this->Internals->LastRenderWindow != renWin )
    {
    // Cleanup all graphics resources associated with the old render window.
    this->ReleaseGraphicsResources( this->Internals->LastRenderWindow );
    }

  this->Internals->LastRenderWindow = renWin;

  // we get the view port size (not the renderwindow size).
  int viewsize[2], vieworigin[2];
  renderer->GetTiledSizeAndOrigin( &viewsize[0],   &viewsize[1], 
                                   &vieworigin[0], &vieworigin[1] );

  if ( this->Internals->LastViewportSize[0] != viewsize[0] || 
       this->Internals->LastViewportSize[1] != viewsize[1] )
    {
    // View size has changed, we need to re-generate the textures.
    this->Internals->ClearTextures();
    }
    
  this->Internals->LastViewportSize[0] = viewsize[0];
  this->Internals->LastViewportSize[1] = viewsize[1];

  if ( !this->Internals->FBO )
    {
    vtkFrameBufferObject * fbo = vtkFrameBufferObject::New();
    fbo->SetContext( renWin );
    fbo->SetNumberOfRenderTargets( 2 );
    unsigned int activeTargets[]=  { 0, 1 };
    fbo->SetActiveBuffers( 2, activeTargets );
    this->Internals->FBO = fbo;
    fbo->Delete();
    fbo = NULL;
    }

  if ( !this->Internals->GeometryImage )
    {
    vtkTextureObject * geometryImage = vtkTextureObject::New();
    geometryImage->SetContext( renWin );
    geometryImage->Create2D( viewsize[0], viewsize[1], 4, VTK_FLOAT, false );
    this->Internals->GeometryImage = geometryImage;
    geometryImage->Delete();
    geometryImage = NULL;
    }
  this->Internals->FBO->SetColorBuffer( 0, this->Internals->GeometryImage );

  if ( !this->Internals->VelocityImage )
    {
    vtkTextureObject * velocityImage = vtkTextureObject::New();
    velocityImage->SetContext( renWin );
    velocityImage->Create2D( viewsize[0], viewsize[1], 4, VTK_FLOAT, false ); 
                  // (r,g) == surface vector in image space
                  // (b) == depth.
                  // a == unused.
    this->Internals->VelocityImage = velocityImage;
    velocityImage->Delete();
    velocityImage = NULL;
    }
  this->Internals->FBO->SetColorBuffer( 1, this->Internals->VelocityImage );

  if ( !this->Internals->PassOne )
    {
    vtkShaderProgram2 * pgmPass1 = vtkShaderProgram2::New();
    pgmPass1->SetContext( renWin );
    
    vtkShader2 * s1 = vtkShader2::New();
    s1->SetSourceCode( vtkSurfaceLICPainter_vs1 );
    s1->SetType( VTK_SHADER_TYPE_VERTEX );
    s1->SetContext( pgmPass1->GetContext() );
    
    vtkShader2 * s2 = vtkShader2::New();
    s2->SetSourceCode( vtkSurfaceLICPainter_fs1 );
    s2->SetType( VTK_SHADER_TYPE_FRAGMENT );
    s2->SetContext( pgmPass1->GetContext() );
    
    pgmPass1->GetShaders()->AddItem( s1 );
    pgmPass1->GetShaders()->AddItem( s2 );
    s1->Delete();
    s2->Delete();
    s1 = NULL;
    s2 = NULL;
    
    this->Internals->LightingHelper->Initialize
                                     ( pgmPass1, VTK_SHADER_TYPE_VERTEX );
    this->Internals->ColorMaterialHelper->Initialize( pgmPass1 );
    this->Internals->PassOne = pgmPass1;
    pgmPass1->Delete();
    pgmPass1 = NULL;
    }

  if ( !this->Internals->NoiseImage )
    {
    vtkDataTransferHelper * noiseBus=vtkDataTransferHelper::New();
    noiseBus->SetContext( renWin );
    noiseBus->SetCPUExtent( this->Internals->Noise->GetExtent() );
    noiseBus->SetGPUExtent( this->Internals->Noise->GetExtent() );
    noiseBus->SetTextureExtent( this->Internals->Noise->GetExtent() );
    noiseBus->SetArray( this->Internals->Noise->GetPointData()->GetScalars() );
    noiseBus->Upload( 0, 0 );
    this->Internals->NoiseImage = noiseBus->GetTexture();
    noiseBus->Delete();
    noiseBus = NULL;

    vtkTextureObject * tex = this->Internals->NoiseImage;
    tex->Bind();
    glTexParameteri( tex->GetTarget(), GL_TEXTURE_WRAP_S,     GL_CLAMP   );
    glTexParameteri( tex->GetTarget(), GL_TEXTURE_WRAP_T,     GL_CLAMP   );
    glTexParameteri( tex->GetTarget(), vtkgl::TEXTURE_WRAP_R, GL_CLAMP   );
    glTexParameteri( tex->GetTarget(), GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( tex->GetTarget(), GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    tex->UnBind();
    tex = NULL;
    }

  if ( !this->Internals->PassTwo )
    {
    vtkShaderProgram2 * pgmPass2 = vtkShaderProgram2::New();
    pgmPass2->SetContext( renWin );
    
    vtkShader2 * s3 = vtkShader2::New();
    s3->SetSourceCode( vtkSurfaceLICPainter_fs2 );
    s3->SetType( VTK_SHADER_TYPE_FRAGMENT );
    s3->SetContext( pgmPass2->GetContext() );
    pgmPass2->GetShaders()->AddItem( s3 );
    s3->Delete();
    s3 = NULL;
    
    this->Internals->PassTwo = pgmPass2;
    pgmPass2->Delete();
    pgmPass2 = NULL;
    }

  // Now compute the bounds of the pixels that this dataset is going to occupy
  // on the screen.

  double bounds[6];
  this->GetInputAsPolyData()->GetBounds( bounds );
  double worldPoints[8][4];
  worldPoints[0][0] = bounds[0];
  worldPoints[0][1] = bounds[2];
  worldPoints[0][2] = bounds[4];
  worldPoints[0][3] = 0;

  worldPoints[1][0] = bounds[1];
  worldPoints[1][1] = bounds[2];
  worldPoints[1][2] = bounds[4];
  worldPoints[1][3] = 0;

  worldPoints[2][0] = bounds[1];
  worldPoints[2][1] = bounds[3];
  worldPoints[2][2] = bounds[4];
  worldPoints[2][3] = 0;

  worldPoints[3][0] = bounds[0];
  worldPoints[3][1] = bounds[3];
  worldPoints[3][2] = bounds[4];
  worldPoints[3][3] = 0;

  worldPoints[4][0] = bounds[0];
  worldPoints[4][1] = bounds[2];
  worldPoints[4][2] = bounds[5];
  worldPoints[4][3] = 0;

  worldPoints[5][0] = bounds[1];
  worldPoints[5][1] = bounds[2];
  worldPoints[5][2] = bounds[5];
  worldPoints[4][3] = 0;

  worldPoints[6][0] = bounds[1];
  worldPoints[6][1] = bounds[3];
  worldPoints[6][2] = bounds[5];
  worldPoints[6][3] = 0;

  worldPoints[7][0] = bounds[0];
  worldPoints[7][1] = bounds[3];
  worldPoints[7][2] = bounds[5];
  worldPoints[7][3] = 0;

  // We need to use matrices provided by OpenGL since renderers such as
  // vtkIceTRenderer change the matrices on the fly without updating the vtkCamera
  // tranforms.
  GLdouble projection[16];
  GLdouble modelview[16];
  GLdouble transform[16];
  glGetDoublev( GL_PROJECTION_MATRIX, projection );
  glGetDoublev( GL_MODELVIEW_MATRIX,  modelview  );
  for ( int c = 0; c < 4; c ++ )
    {
    for ( int r = 0; r < 4; r ++ )
      {
      transform[ c * 4 + r ] =
          projection[ vtkGetIndex( r, 0 ) ] * modelview[ vtkGetIndex( 0, c ) ]
        + projection[ vtkGetIndex( r, 1 ) ] * modelview[ vtkGetIndex( 1, c ) ]
        + projection[ vtkGetIndex( r, 2 ) ] * modelview[ vtkGetIndex( 2, c ) ]
        + projection[ vtkGetIndex( r, 3 ) ] * modelview[ vtkGetIndex( 3, c ) ];
      }
    }

  vtkBoundingBox box;
  for (int kk = 0; kk < 8; kk ++ )
    {
    double x = worldPoints[kk][0];
    double y = worldPoints[kk][1];
    double z = worldPoints[kk][2];
    double view[4];
    view[0] = x * transform[vtkGetIndex(0,0)] + y * transform[vtkGetIndex(0,1)] +
              z * transform[vtkGetIndex(0,2)] + transform[vtkGetIndex(0,3)];
    view[1] = x * transform[vtkGetIndex(1,0)] + y * transform[vtkGetIndex(1,1)] +
              z * transform[vtkGetIndex(1,2)] + transform[vtkGetIndex(1,3)];
    view[2] = x * transform[vtkGetIndex(2,0)] + y * transform[vtkGetIndex(2,1)] +
              z * transform[vtkGetIndex(2,2)] + transform[vtkGetIndex(2,3)];
    view[3] = x * transform[vtkGetIndex(3,0)] + y * transform[vtkGetIndex(3,1)] +
              z * transform[vtkGetIndex(3,2)] + transform[vtkGetIndex(3,3)];

    if (view[3] != 0.0)
      {
      view[0] = view[0]/view[3];
      view[1] = view[1]/view[3];
      view[2] = view[2]/view[3];
      }
    double displayPt[2];
    displayPt[0] = ( view[0] + 1.0 ) * viewsize[0] / 2.0/* + vieworigin[0]*/;
    displayPt[1] = ( view[1] + 1.0 ) * viewsize[1] / 2.0/* + vieworigin[1]*/;
    box.AddPoint(
      vtkClamp( displayPt[0]/*-vieworigin[0]*/, 0.0, viewsize[0] - 1.0 ),
      vtkClamp( displayPt[1]/*-vieworigin[1]*/, 0.0, viewsize[1] - 1.0 ), 0.0 );
    }

  this->Internals->ViewportExtent[0] = 
        static_cast<unsigned int>( box.GetMinPoint()[0] );
  this->Internals->ViewportExtent[1] = 
        static_cast<unsigned int>( box.GetMaxPoint()[0] );
  this->Internals->ViewportExtent[2] = 
        static_cast<unsigned int>( box.GetMinPoint()[1] );
  this->Internals->ViewportExtent[3] = 
        static_cast<unsigned int>( box.GetMaxPoint()[1] );

  vtkDebugMacro( << "ViewportExtent: " << this->Internals->ViewportExtent[0]
                 << ", " << this->Internals->ViewportExtent[1]  
                 << ", " << this->Internals->ViewportExtent[2]  
                 << ", " << this->Internals->ViewportExtent[3] << endl );
                 
  this->Superclass::PrepareForRendering( renderer, actor );
  
  this->RenderingPreparationSuccess = 1;
}

//----------------------------------------------------------------------------
void vtkSurfaceLICPainter::RenderInternal
   ( vtkRenderer * renderer,  vtkActor * actor,
     unsigned long typeflags, bool forceCompileOnly )
{
  if (  !this->RenderingPreparationSuccess  ||  
        !this->CanRenderLIC( renderer, actor )  )
    {
    this->Superclass::RenderInternal
        ( renderer, actor, typeflags, forceCompileOnly );
    return;
    }

  vtkTimerLog * timer = vtkTimerLog::New();
  timer->StartTimer();

  // Save context state to be able to restore.
  glPushAttrib(GL_ALL_ATTRIB_BITS);
  // save model-view/projection matrices.
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();

  // TODO: eventually we'll add code to generate the LIC only if the camera
  // position has changed or the input dataset has changed. Currently, we always
  // rebuild the LIC.
  
  // * PASS ONE
  //   * Render geometry
  //   * Outputs:
  //      - shaded geometry rendering -- used to combine with the final LIC
  //        image.
  //      - "velocity image"
  //      - "depth mask" - when putting pixes back into the original scene, we
  //        need to ensure that the depth values match the original rendering.
  //      - model-view and projection matrices.
  // * PASS THREE to N
  //   * Render Quad covering the image-space bounds of the rendered geometry
  //     and perform LIC
  // * PASS (N+1)
  //   * Combine shaded geometry rendering with LIC image and put it back into
  //   the actual render window.

  vtkOpenGLRenderWindow * renWin = vtkOpenGLRenderWindow::SafeDownCast
                                   ( renderer->GetRenderWindow() );

  // we get the view port size (not the renderwindow size).
  int viewsize[2], vieworigin[2];
  renderer->GetTiledSizeAndOrigin(&viewsize[0], &viewsize[1], &vieworigin[0], &vieworigin[1]);

  glViewport(0, 0, viewsize[0], viewsize[1]);
  // Set clear color to black in case user has set some background color.
  glClearColor(0.0, 0.0, 0.0, 0.0);

  // Set scissor to work with on the area covered by the data.
  glEnable(GL_SCISSOR_TEST);
  glScissor(this->Internals->ViewportExtent[0],
    this->Internals->ViewportExtent[2], 
    this->Internals->ViewportExtent[1]-this->Internals->ViewportExtent[0]+1,
    this->Internals->ViewportExtent[3]-this->Internals->ViewportExtent[2]+1);

  if (  !this->Internals->FBO
             ->StartNonOrtho( viewsize[0], viewsize[1], false )  )
    {
    timer->Delete();
    timer  = NULL;
    renWin = NULL;
    this->LICSuccess = 0;
    return;
    }
    
  glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);

  this->Internals->ColorMaterialHelper->PrepareForRendering();
  this->Internals->LightingHelper->PrepareForRendering();

  this->Internals->PassOne->Build();
  if(this->Internals->PassOne->GetLastBuildStatus()!=
     VTK_SHADER_PROGRAM2_LINK_SUCCEEDED)
    {
    vtkErrorMacro("Pass One failed.");
    abort();
    }
  this->Internals->PassOne->Use();
  if(!this->Internals->PassOne->IsValid())
    {
    vtkErrorMacro(<<" validation of the program failed: "<<this->Internals->PassOne->GetLastValidateLog());
    }
  
  this->Internals->ColorMaterialHelper->Render();

  this->Superclass::RenderInternal(renderer, actor, typeflags,
                                   forceCompileOnly);
  glFinish();
  this->Internals->PassOne->Restore();
  this->Internals->FBO->UnBind();

  renWin->MakeCurrent();

  int licSize[2] = {
    this->Internals->ViewportExtent[1]-this->Internals->ViewportExtent[0]+1,
    this->Internals->ViewportExtent[3]-this->Internals->ViewportExtent[2]+1 };

  // vtkLineIntegralConvolution2D needs step size in normalized image space, so we
  // convert this->StepSize to normalized space.
  // (assuming 1 pixel is a unit square):
  double stepsize = this->StepSize * sqrt(2.0) / 
                    sqrt(  static_cast< double > ( licSize[0] * licSize[0] + 
                                                   licSize[1] * licSize[1] 
                                                 )
                        );
  vtkLineIntegralConvolution2D * licer = vtkLineIntegralConvolution2D::New();
  if (  !licer->IsSupported( renWin )  )
    {
    licer->Delete();
    timer->Delete();
    licer  = NULL;
    timer  = NULL;
    renWin = NULL;
    this->LICSuccess = 0;
    return;
    }
    
  licer->SetNumberOfSteps( this->NumberOfSteps );
  licer->SetLICStepSize( stepsize );
  licer->SetEnhancedLIC( this->EnhancedLIC );
  licer->SetLICForSurface( 1 );
  licer->SetNoise( this->Internals->NoiseImage );
  licer->SetVectorField( this->Internals->VelocityImage );
  licer->SetComponentIds( 0, 1 );
  if (  !licer->Execute( this->Internals->ViewportExtent )  )
    {
    licer->Delete();
    timer->Delete();
    licer  = NULL;
    timer  = NULL;
    renWin = NULL;
    this->LICSuccess = 0;
    return;
    }
  this->LICSuccess = 1;

  vtkSmartPointer<vtkTextureObject> lic = licer->GetLIC();
  licer->Delete();

  glFinish();

  // * Now render lic on-to the scene with 
  renWin->MakeCurrent();

  this->Internals->PassTwo->Build();
  if(this->Internals->PassTwo->GetLastBuildStatus()!=
     VTK_SHADER_PROGRAM2_LINK_SUCCEEDED)
    {
    vtkErrorMacro("Pass Two failed.");
    abort();
    }
  this->Internals->PassTwo->Use();

  vtkgl::ActiveTexture(vtkgl::TEXTURE0);
  lic->Bind();
  int value=0;
  this->Internals->PassTwo->GetUniformVariables()->SetUniformi("texLIC",1,&value);
  
  vtkgl::ActiveTexture(vtkgl::TEXTURE1);
  this->Internals->GeometryImage->Bind();
  
  value=1;
  this->Internals->PassTwo->GetUniformVariables()->SetUniformi("texGeometry",1,&value);
  vtkgl::ActiveTexture(vtkgl::TEXTURE2);
  this->Internals->VelocityImage->Bind();
  
  value=2;
  this->Internals->PassTwo->GetUniformVariables()->SetUniformi("texDepth",1,&value);

  float fvalue=static_cast<float>(this->LICIntensity);
  this->Internals->PassTwo->GetUniformVariables()->SetUniformf("uLICIntensity",1,&fvalue);

  // vtkLineIntegralConvolution2D changed the matrices to be orthogonal to the
  // extents we provided. Now we want the view to be orthogonal to the full
  // viewport.
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0, viewsize[0], 0.0, viewsize[1], -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glViewport(vieworigin[0], vieworigin[1], viewsize[0], viewsize[1]);
  glScissor(vieworigin[0], vieworigin[1], viewsize[0], viewsize[1]);

  // vtkFrameBufferObject disables depth-test, we need to enable it.
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_SCISSOR_TEST);
  
  this->Internals->PassTwo->Use();
  if(!this->Internals->PassTwo->IsValid())
    {
    vtkErrorMacro(<<" validation of the program failed: "<<this->Internals->PassTwo->GetLastValidateLog());
    }
  
  glBegin(GL_QUADS);
  glTexCoord2f(0.0, 0.0);
  vtkgl::MultiTexCoord2f(vtkgl::TEXTURE1,
                         static_cast<GLfloat>(this->Internals->ViewportExtent[0]/double(viewsize[0])),
                         static_cast<GLfloat>(this->Internals->ViewportExtent[2]/double(viewsize[1])));
  glVertex2f(static_cast<GLfloat>(this->Internals->ViewportExtent[0]),
             static_cast<GLfloat>(this->Internals->ViewportExtent[2]));

  glTexCoord2f(1.0, 0.0);
  vtkgl::MultiTexCoord2f(vtkgl::TEXTURE1,
                         static_cast<GLfloat>(this->Internals->ViewportExtent[1]/double(viewsize[0])),
                         static_cast<GLfloat>(this->Internals->ViewportExtent[2]/double(viewsize[1])));
  glVertex2f(static_cast<GLfloat>(this->Internals->ViewportExtent[1]),
             static_cast<GLfloat>(this->Internals->ViewportExtent[2]));

  glTexCoord2f(1.0, 1.0);
  vtkgl::MultiTexCoord2f(vtkgl::TEXTURE1,
                         static_cast<GLfloat>(this->Internals->ViewportExtent[1]/double(viewsize[0])),
                         static_cast<GLfloat>(this->Internals->ViewportExtent[3]/double(viewsize[1])));
  glVertex2f(static_cast<GLfloat>(this->Internals->ViewportExtent[1]),
             static_cast<GLfloat>(this->Internals->ViewportExtent[3]));

  glTexCoord2f(0.0, 1.0);
  vtkgl::MultiTexCoord2f(vtkgl::TEXTURE1,
                         static_cast<GLfloat>(this->Internals->ViewportExtent[0]/double(viewsize[0])),
                         static_cast<GLfloat>(this->Internals->ViewportExtent[3]/double(viewsize[1])));
  glVertex2f(static_cast<GLfloat>(this->Internals->ViewportExtent[0]),
             static_cast<GLfloat>(this->Internals->ViewportExtent[3]));
  glEnd();

  lic = 0;
  this->Internals->PassTwo->Restore();

  // Essential to restore the context to what it was before we started messing
  // with it.
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();

  // Pop the attributes.
  glPopAttrib();

  timer->StopTimer();
  vtkDebugMacro( << "Elapsed: " << timer->GetElapsedTime() << endl );
  timer->Delete();
}

//-----------------------------------------------------------------------------
void vtkSurfaceLICPainter::ReportReferences(vtkGarbageCollector *collector)
{
  this->Superclass::ReportReferences(collector);

  vtkGarbageCollectorReport(collector, this->Output, "Output PolyData");
}

//----------------------------------------------------------------------------
vtkDataObject* vtkSurfaceLICPainter::GetOutput()
{
  if (this->Enable)
    {
    return this->Output;
    }

  return this->Superclass::GetOutput();
}

//----------------------------------------------------------------------------
bool vtkSurfaceLICPainter::PrepareOutput()
{
  if ( !this->Enable )
    {
    // Don't bother doing any work, we are simply passing the input as the
    // output.
    return false;
    }

  // TODO: Handle composite datasets.
  vtkPolyData * input = this->GetInputAsPolyData();

  if (  !this->Output || 
        !this->Output->IsA( input->GetClassName() ) ||
       ( this->Output->GetMTime() < this->GetMTime( ) ) || 
       ( this->Output->GetMTime() < input->GetMTime() ) 
     )
    {
    this->Internals->HasVectors = true;
    if ( this->Output )
      {
      this->Output->Delete();
      this->Output = 0;
      }
      
    bool           cell_data = false;
    vtkPolyData  * output = vtkPolyData::New();
    output->ShallowCopy( input );
    vtkDataArray * vectors = NULL;
    
    if ( this->Internals->FieldNameSet )
      {
      vectors = vtkDataArray::SafeDownCast
                (  this->GetInputArrayToProcess
                   ( this->Internals->FieldAssociation,
                     this->Internals->FieldName.c_str(),
                     output,
                     &cell_data
                   )
                );
      }
    else
      {
      vectors = vtkDataArray::SafeDownCast
                (  this->GetInputArrayToProcess
                   ( this->Internals->FieldAssociation,
                     this->Internals->FieldAttributeType, 
                     output,
                     &cell_data
                   )
                );
      }

    if ( vectors )
      {
      if ( cell_data )
        {
        output->GetCellData()->SetTCoords( vectors );
        }
      else
        {
        output->GetPointData()->SetTCoords( vectors );
        }
        
      vectors = NULL;
      }
    else
      {
      vtkErrorMacro( "No vectors available." );
      this->Internals->HasVectors = false;
      }

    this->Output = output;
    this->Output->Modified();
    output = NULL;
    }

  input = NULL;
  return this->Internals->HasVectors;
}

//----------------------------------------------------------------------------
void vtkSurfaceLICPainter::PrintSelf( ostream & os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  
  os << indent << "Enable: "        << this->Enable        << endl;
  os << indent << "StepSize: "      << this->StepSize      << endl;
  os << indent << "EnhancedLIC: "   << this->EnhancedLIC   << endl;
  os << indent << "LICIntensity: "  << this->LICIntensity  << endl;
  os << indent << "NumberOfSteps: " << this->NumberOfSteps << endl;
  os << indent << "RenderingPreparationSuccess: " 
               << this->RenderingPreparationSuccess << endl;
}
