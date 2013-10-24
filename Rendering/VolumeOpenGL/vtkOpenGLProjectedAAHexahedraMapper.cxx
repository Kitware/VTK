/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLProjectedAAHexahedraMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// High quality volume renderer for axis-aligned hexahedra
// Implementation by Stephane Marchesin (stephane.marchesin@gmail.com)
// CEA/DIF - Commissariat a l'Energie Atomique, Centre DAM Ile-De-France
// BP12, F-91297 Arpajon, France.
//
// This file implements the paper
// "High-Quality, Semi-Analytical Volume Rendering for AMR Data",
// Stephane Marchesin and Guillaume Colin de Verdiere, IEEE Vis 2009.


#include "vtkOpenGLProjectedAAHexahedraMapper.h"

#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkCellCenterDepthSort.h"
#include "vtkCellData.h"
#include "vtkCellIterator.h"
#include "vtkColorTransferFunction.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkGarbageCollector.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLExtensionManager.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridPreIntegration.h"
#include "vtkVisibilitySort.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkgl.h"
#include "vtkShaderProgram2.h"
#include "vtkShader2.h"
#include "vtkSmartPointer.h"
#include "vtkUniformVariables.h"
#include "vtkShader2Collection.h"
#include "vtkOpenGLError.h"

#include <math.h>
#include <algorithm>

// Shader code
extern const char *vtkProjectedAAHexahedraMapper_VS;
extern const char *vtkProjectedAAHexahedraMapper_GS;
extern const char *vtkProjectedAAHexahedraMapper_FS;

// ----------------------------------------------------------------------------

vtkStandardNewMacro(vtkOpenGLProjectedAAHexahedraMapper);

// ----------------------------------------------------------------------------
vtkOpenGLProjectedAAHexahedraMapper::vtkOpenGLProjectedAAHexahedraMapper()
{
  this->ConvertedPoints = vtkFloatArray::New();
  this->ConvertedScalars = vtkFloatArray::New();

  this->LastProperty = NULL;

  this->PreintTexture = 0;
  this->MaxCellSize = 0;

  this->GaveError = 0;
  this->Initialized=false;
  this->Shader=0;
}

// ----------------------------------------------------------------------------
vtkOpenGLProjectedAAHexahedraMapper::~vtkOpenGLProjectedAAHexahedraMapper()
{
  this->ConvertedPoints->Delete();
  this->ConvertedScalars->Delete();
  if(this->Shader!=0)
    {
    this->Shader->Delete();
    }
}

// ----------------------------------------------------------------------------
void vtkOpenGLProjectedAAHexahedraMapper::PrintSelf(ostream &os,
                                                    vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------------
bool vtkOpenGLProjectedAAHexahedraMapper::IsRenderSupported(vtkRenderWindow *w)
{
  vtkOpenGLExtensionManager *e=
    static_cast<vtkOpenGLRenderWindow *>(w)->GetExtensionManager();

  bool texture3D=e->ExtensionSupported("GL_VERSION_1_2") ||
    e->ExtensionSupported("GL_EXT_texture3D");

  bool multiTexture=e->ExtensionSupported("GL_VERSION_1_3") ||
    e->ExtensionSupported("GL_ARB_multitexture");

  bool glsl=e->ExtensionSupported("GL_VERSION_2_0") ||
    (e->ExtensionSupported("GL_ARB_shading_language_100") &&
     e->ExtensionSupported("GL_ARB_shader_objects") &&
     e->ExtensionSupported("GL_ARB_vertex_shader") &&
     e->ExtensionSupported("GL_ARB_fragment_shader"));

  bool geometry_shader=e->ExtensionSupported("GL_EXT_geometry_shader4")==1;

  return texture3D && multiTexture && glsl && geometry_shader;
}

// ----------------------------------------------------------------------------
void vtkOpenGLProjectedAAHexahedraMapper::Initialize(
  vtkRenderer *ren,
  vtkVolume *vtkNotUsed(vol))
{
  vtkOpenGLExtensionManager *e=static_cast<vtkOpenGLRenderWindow *>(
    ren->GetRenderWindow())->GetExtensionManager();

  bool gl12=e->ExtensionSupported("GL_VERSION_1_2")==1;
  bool gl13=e->ExtensionSupported("GL_VERSION_1_3")==1;
  bool gl20=e->ExtensionSupported("GL_VERSION_2_0")==1;

  bool texture3D=gl12 || e->ExtensionSupported("GL_EXT_texture3D");
  bool multiTexture=gl13 || e->ExtensionSupported("GL_ARB_multitexture");
  bool glsl=gl20 || (e->ExtensionSupported("GL_ARB_shading_language_100") &&
                     e->ExtensionSupported("GL_ARB_shader_objects") &&
                     e->ExtensionSupported("GL_ARB_vertex_shader") &&
                     e->ExtensionSupported("GL_ARB_fragment_shader"));
  bool geometry_shader=e->ExtensionSupported("GL_EXT_geometry_shader4")==1;

  bool result=texture3D && multiTexture && glsl && geometry_shader;

  if(result)
    {
    if(gl12)
      {
      e->LoadExtension("GL_VERSION_1_2");
      }
    else
      {
      e->LoadCorePromotedExtension("GL_EXT_texture3D");
      }
    if(gl13)
      {
      e->LoadExtension("GL_VERSION_1_3");
      }
    else
      {
      e->LoadCorePromotedExtension("GL_ARB_multitexture");
      }
    if(gl20)
      {
      e->LoadExtension("GL_VERSION_2_0");
      }
    else
      {
      e->LoadCorePromotedExtension("GL_ARB_shading_language_100");
      e->LoadCorePromotedExtension("GL_ARB_shader_objects");
      e->LoadCorePromotedExtension("GL_ARB_vertex_shader");
      e->LoadCorePromotedExtension("GL_ARB_fragment_shader");
      }
    e->LoadExtension("GL_EXT_geometry_shader4");

    this->Initialized=true;
    this->CreateProgram(ren->GetRenderWindow());
    pos_points = new float[3*max_points];
    min_points = new float[3*max_points];
    node_data1 = new float[4*max_points];
    node_data2 = new float[4*max_points];
    }
}

// ----------------------------------------------------------------------------
// sort, iterate the hexahedra and call the rendering function
void vtkOpenGLProjectedAAHexahedraMapper::Render(vtkRenderer *renderer,
                                                 vtkVolume *volume)
{
  vtkOpenGLClearErrorMacro();

  if ( !this->Initialized )
    {
    this->Initialize(renderer, volume);
    }
  vtkUnstructuredGridBase *input = this->GetInput();
  vtkVolumeProperty *property = volume->GetProperty();

  float last_max_cell_size = this->MaxCellSize;

  // Check to see if input changed.
  if (   (this->InputAnalyzedTime < this->MTime)
         || (this->InputAnalyzedTime < input->GetMTime()) )
    {
    this->GaveError = 0;

    if (input->GetNumberOfCells() == 0)
      {
      // Apparently, the input has no cells.  Just do nothing.
      return;
      }

    vtkIdType npts, *pts;
    vtkSmartPointer<vtkCellIterator> cellIter =
        vtkSmartPointer<vtkCellIterator>::Take(input->NewCellIterator());
    for (cellIter->InitTraversal(); !cellIter->IsDoneWithTraversal();
         cellIter->GoToNextCell())
      {
      npts = cellIter->GetNumberOfPoints();
      pts = cellIter->GetPointIds()->GetPointer(0);
      int j;
      if (npts != 8)
        {
        if (!this->GaveError)
          {
          vtkErrorMacro("Encountered non-hexahedral cell!");
          this->GaveError = 1;
          }
        continue;
        }

      double p[3];
      input->GetPoint(pts[0], p);
      double min[3] = {p[0],p[1],p[2]},
        max[3] = {p[0],p[1],p[2]};

        for(j = 1; j < npts; j++)
          {
          input->GetPoint(pts[j], p);

          if (p[0]<min[0])
            {
            min[0] = p[0];
            }
          if (p[1]<min[1])
            {
            min[1] = p[1];
            }
          if (p[2]<min[2])
            {
            min[2] = p[2];
            }
          if (p[0]>max[0])
            {
            max[0] = p[0];
            }
          if (p[1]>max[1])
            {
            max[1] = p[1];
            }
          if (p[2]>max[2])
            {
            max[2] = p[2];
            }
          }

        float size = static_cast<float>(
          vtkMath::Distance2BetweenPoints(min, max));
        if (size > this->MaxCellSize)
          {
          this->MaxCellSize = size;
          }
      }

    this->InputAnalyzedTime.Modified();
    }

  if (renderer->GetRenderWindow()->CheckAbortStatus() || this->GaveError)
    {
    return;
    }

  // Check to see if we need to rebuild preintegartion texture.
  if (   !this->PreintTexture
         || (last_max_cell_size != this->MaxCellSize)
         || (this->LastProperty != property)
         || (this->PreintTextureTime < property->GetMTime()) )
    {
    if (!this->PreintTexture)
      {
      GLuint texid;
      glGenTextures(1, &texid);
      this->PreintTexture = texid;
      }
    vtkDataArray *scalars = this->GetScalars(input, this->ScalarMode,
                                             this->ArrayAccessMode,
                                             this->ArrayId, this->ArrayName,
                                             this->UsingCellColors);
    if (!scalars)
      {
      vtkErrorMacro(<< "Can't use projected tetrahedra without scalars!");
      return;
      }

    this->UpdatePreintegrationTexture(volume, scalars);

    this->PreintTextureTime.Modified();

    this->LastProperty = property;
    }

  if (renderer->GetRenderWindow()->CheckAbortStatus())
    {
    return;
    }

  this->Timer->StartTimer();

  this->ProjectHexahedra(renderer, volume);

  this->Timer->StopTimer();
  this->TimeToDraw = this->Timer->GetElapsedTime();

  vtkOpenGLCheckErrorMacro("failed after Render");
}

// ----------------------------------------------------------------------------
void vtkOpenGLProjectedAAHexahedraMapper::UpdatePreintegrationTexture(
  vtkVolume *volume,
  vtkDataArray *scalars)
{
  vtkOpenGLClearErrorMacro();

  // rebuild the preintegration texture
  vtkUnstructuredGridPreIntegration *pi=
    vtkUnstructuredGridPreIntegration::New();
  pi->Initialize(volume, scalars);
  // We only render the first field
  float *table = pi->GetPreIntegrationTable(0);
  int ScalarSize = pi->GetIntegrationTableScalarResolution();
  int LengthSize = pi->GetIntegrationTableLengthResolution();

  this->ScalarScale =
    static_cast<float>(pi->GetIntegrationTableScalarScale());
  this->ScalarResolution=
    static_cast<float>(pi->GetIntegrationTableScalarResolution());
  this->ScalarShift=
    static_cast<float>(pi->GetIntegrationTableScalarShift());
  this->LengthScale = static_cast<float>(
    (pi->GetIntegrationTableLengthResolution() - 2) /
    pi->GetIntegrationTableLengthScale());

  glBindTexture(vtkgl::TEXTURE_3D, this->PreintTexture);
  glTexParameteri(vtkgl::TEXTURE_3D,vtkgl::TEXTURE_WRAP_R,
                  vtkgl::CLAMP_TO_EDGE);
  glTexParameteri(vtkgl::TEXTURE_3D, GL_TEXTURE_WRAP_S, vtkgl::CLAMP_TO_EDGE);
  glTexParameteri(vtkgl::TEXTURE_3D, GL_TEXTURE_WRAP_T, vtkgl::CLAMP_TO_EDGE);
  glTexParameteri(vtkgl::TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(vtkgl::TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  vtkgl::TexImage3D(vtkgl::TEXTURE_3D, 0, vtkgl::RGBA16_EXT, ScalarSize,
                    ScalarSize, LengthSize, 0, GL_RGBA, GL_FLOAT, table);

  pi->Delete();

  vtkOpenGLCheckErrorMacro("failed after UpdatePreintegrationTexture");
}

// ----------------------------------------------------------------------------
void vtkOpenGLProjectedAAHexahedraMapper::CreateProgram(vtkRenderWindow *w)
{
  this->Shader=vtkShaderProgram2::New();
  this->Shader->SetContext(static_cast<vtkOpenGLRenderWindow *>(w));

  vtkShader2Collection *shaders=this->Shader->GetShaders();

  vtkShader2 *vs=vtkShader2::New();
  vs->SetType(VTK_SHADER_TYPE_VERTEX);
  vs->SetContext(this->Shader->GetContext());
  vs->SetSourceCode(vtkProjectedAAHexahedraMapper_VS);
  shaders->AddItem(vs);
  vs->Delete();

  vtkShader2 *gs=vtkShader2::New();
  gs->SetType(VTK_SHADER_TYPE_GEOMETRY);
  gs->SetContext(this->Shader->GetContext());
  gs->SetSourceCode(vtkProjectedAAHexahedraMapper_GS);
  shaders->AddItem(gs);
  gs->Delete();

  vtkShader2 *fs=vtkShader2::New();
  fs->SetType(VTK_SHADER_TYPE_FRAGMENT);
  fs->SetContext(this->Shader->GetContext());
  fs->SetSourceCode(vtkProjectedAAHexahedraMapper_FS);
  shaders->AddItem(fs);
  fs->Delete();

  this->Shader->SetGeometryVerticesOut(24);
  this->Shader->SetGeometryTypeIn(VTK_GEOMETRY_SHADER_IN_TYPE_POINTS);
  this->Shader->SetGeometryTypeOut(
    VTK_GEOMETRY_SHADER_OUT_TYPE_TRIANGLE_STRIP);

  this->Shader->Build();
}

// ----------------------------------------------------------------------------
void vtkOpenGLProjectedAAHexahedraMapper::SetState(double *observer)
{
  vtkOpenGLClearErrorMacro();

  glDepthMask(GL_FALSE);

  // save the default blend function.
  glPushAttrib(GL_COLOR_BUFFER_BIT);

  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  glEnable(GL_CULL_FACE);
  glFrontFace(GL_CW);
  glCullFace(GL_BACK);
  glDepthFunc( GL_ALWAYS );
  glDisable( GL_DEPTH_TEST );

  vtkgl::ActiveTexture(vtkgl::TEXTURE0);
  glBindTexture(vtkgl::TEXTURE_3D,this->PreintTexture);

  vtkUniformVariables *v=this->Shader->GetUniformVariables();

  // preintegration table
  int ivalue=0;
  v->SetUniformi("preintegration_table",1,&ivalue);

  // observer position
  float fvalue[3];
  fvalue[0]=static_cast<float>(observer[0]);
  fvalue[1]=static_cast<float>(observer[1]);
  fvalue[2]=static_cast<float>(observer[2]);
  v->SetUniformf("observer",3,fvalue);

  // max length of preint table
  v->SetUniformf("length_max",1,&this->LengthScale);

  this->Shader->Use();

  glEnableClientState( GL_VERTEX_ARRAY );
  glVertexPointer( 3, GL_FLOAT, 0, pos_points);

  vtkgl::ActiveTexture( vtkgl::TEXTURE0_ARB );
  vtkgl::ClientActiveTexture(vtkgl::TEXTURE0_ARB);
  glEnableClientState( GL_TEXTURE_COORD_ARRAY );
  glTexCoordPointer( 3, GL_FLOAT, 0, min_points);

  vtkgl::ActiveTexture( vtkgl::TEXTURE1_ARB );
  vtkgl::ClientActiveTexture(vtkgl::TEXTURE1_ARB);
  glEnableClientState( GL_TEXTURE_COORD_ARRAY );
  glTexCoordPointer( 4, GL_FLOAT, 0, node_data1);

  vtkgl::ActiveTexture( vtkgl::TEXTURE2_ARB );
  vtkgl::ClientActiveTexture(vtkgl::TEXTURE2_ARB);
  glEnableClientState( GL_TEXTURE_COORD_ARRAY );
  glTexCoordPointer( 4, GL_FLOAT, 0, node_data2);

  this->num_points = 0;

  vtkOpenGLCheckErrorMacro("failed after SetState");
}

// ----------------------------------------------------------------------------
void vtkOpenGLProjectedAAHexahedraMapper::RenderHexahedron(float vmin[3],
                                                           float vmax[3],
                                                           float scalars[8])
{
  this->pos_points[num_points * 3 + 0] = vmin[0];
  this->pos_points[num_points * 3 + 1] = vmin[1];
  this->pos_points[num_points * 3 + 2] = vmin[2];

  this->min_points[num_points * 3 + 0] = vmax[0];
  this->min_points[num_points * 3 + 1] = vmax[1];
  this->min_points[num_points * 3 + 2] = vmax[2];

  this->node_data1[num_points * 4 + 0] = scalars[0];
  this->node_data1[num_points * 4 + 1] = scalars[1];
  this->node_data1[num_points * 4 + 2] = scalars[2];
  this->node_data1[num_points * 4 + 3] = scalars[3];

  this->node_data2[num_points * 4 + 0] = scalars[4];
  this->node_data2[num_points * 4 + 1] = scalars[5];
  this->node_data2[num_points * 4 + 2] = scalars[6];
  this->node_data2[num_points * 4 + 3] = scalars[7];

  num_points++;

  // need to flush?
  if (num_points >= max_points)
    {
    glDrawArrays(GL_POINTS, 0, num_points);
    num_points=0;
    }
}

// ----------------------------------------------------------------------------
void vtkOpenGLProjectedAAHexahedraMapper::UnsetState()
{
  vtkOpenGLClearErrorMacro();

  // flush what remains of our points
  if (this->num_points>0)
    {
    glDrawArrays(GL_POINTS, 0, this->num_points);
    this->num_points = 0;
    }

  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_COLOR_ARRAY);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);

  this->Shader->Restore();

  // Restore the blend function.
  glPopAttrib();

  glBindTexture(vtkgl::TEXTURE_3D, 0);

  glDepthMask(GL_TRUE);

  vtkOpenGLCheckErrorMacro("failed fater UnsetState");
}

// ----------------------------------------------------------------------------
template<class point_type>
void vtkOpenGLProjectedAAHexahedraMapperConvertScalars(
  const point_type *in_scalars,
  vtkIdType num_scalars,
  float *out_scalars)
{
  for(int i=0;i<num_scalars;i++)
    {
    out_scalars[i] = static_cast<float>(in_scalars[i]);
    }
}

// ----------------------------------------------------------------------------
// convert all our scalars to floating point
float* vtkOpenGLProjectedAAHexahedraMapper::ConvertScalars(
  vtkDataArray* inScalars)
{
  this->ConvertedScalars->SetNumberOfComponents(1);
  this->ConvertedScalars->SetNumberOfTuples(inScalars->GetNumberOfTuples());
  switch (inScalars->GetDataType())
    {
    vtkTemplateMacro(vtkOpenGLProjectedAAHexahedraMapperConvertScalars(
                       static_cast<const VTK_TT *>(
                         inScalars->GetVoidPointer(0)),
                       inScalars->GetNumberOfTuples(),
                       this->ConvertedScalars->GetPointer(0) ) );
    }
  return this->ConvertedScalars->GetPointer(0);
}

// ----------------------------------------------------------------------------
template<class point_type>
void vtkOpenGLProjectedAAHexahedraMapperConvertPoints(
  const point_type *in_points,
  vtkIdType num_points,
  float *out_points)
{
  for(int i=0;i<num_points*3;i++)
    {
    out_points[i] = static_cast<float>(in_points[i]);
    }
}

// ----------------------------------------------------------------------------
// convert all our points to floating point
float* vtkOpenGLProjectedAAHexahedraMapper::ConvertPoints(vtkPoints* inPoints)
{
  this->ConvertedPoints->SetNumberOfComponents(3);
  this->ConvertedPoints->SetNumberOfTuples(inPoints->GetNumberOfPoints());
  switch (inPoints->GetDataType())
    {
    vtkTemplateMacro(vtkOpenGLProjectedAAHexahedraMapperConvertPoints(
                       static_cast<const VTK_TT *>(
                         inPoints->GetVoidPointer(0)),
                       inPoints->GetNumberOfPoints(),
                       this->ConvertedPoints->GetPointer(0) ) );
    }
  return this->ConvertedPoints->GetPointer(0);
}

// ----------------------------------------------------------------------------
void vtkOpenGLProjectedAAHexahedraMapper::ProjectHexahedra(
  vtkRenderer *renderer,
  vtkVolume *volume)
{
  vtkUnstructuredGridBase *input = this->GetInput();

  this->VisibilitySort->SetInput(input);
  this->VisibilitySort->SetDirectionToBackToFront();
  this->VisibilitySort->SetModelTransform(volume->GetMatrix());
  this->VisibilitySort->SetCamera(renderer->GetActiveCamera());
  this->VisibilitySort->SetMaxCellsReturned(1000);

  double* observer = renderer->GetActiveCamera()->GetPosition();

  this->VisibilitySort->InitTraversal();

  float* points = ConvertPoints(input->GetPoints());

  float* scalars = ConvertScalars(this->GetScalars(input, this->ScalarMode,
                                                   this->ArrayAccessMode,
                                                   this->ArrayId,
                                                   this->ArrayName,
                                                   this->UsingCellColors) );

  if (renderer->GetRenderWindow()->CheckAbortStatus())
    {
    return;
    }

  this->SetState(observer);

  vtkIdType totalnumcells = input->GetNumberOfCells();
  vtkIdType numcellsrendered = 0;

  // Let's do it!
  vtkNew<vtkIdList> cellPtIds;
  for (vtkIdTypeArray *sorted_cell_ids = this->VisibilitySort->GetNextCells();
       sorted_cell_ids != NULL;
       sorted_cell_ids = this->VisibilitySort->GetNextCells())
    {
    this->UpdateProgress(static_cast<double>(numcellsrendered)/
                         static_cast<double>(totalnumcells));
    if (renderer->GetRenderWindow()->CheckAbortStatus())
      {
      break;
      }
    vtkIdType *cell_ids = sorted_cell_ids->GetPointer(0);
    vtkIdType num_cell_ids = sorted_cell_ids->GetNumberOfTuples();
    for (vtkIdType i = 0; i < num_cell_ids; i++)
      {
      vtkIdType cell = cell_ids[i];
      input->GetCellPoints(cell, cellPtIds.GetPointer());

      float corner_scalars[8];

      // get the data for the current hexahedron
      vtkIdType index = cellPtIds->GetId(0);
      float* p = points + 3 * index;

      float vmin[3] = {p[0],p[1],p[2]},
        vmax[3] = {p[0],p[1],p[2]};

        int j;
        for(j = 1; j < 8; j++)
          {
          index = cellPtIds->GetId(j);

          p = points + 3 * index;
          if (p[0]<vmin[0])
            {
            vmin[0] = p[0];
            }
          if (p[1]<vmin[1])
            {
            vmin[1] = p[1];
            }
          if (p[2]<vmin[2])
            {
            vmin[2] = p[2];
            }
          if (p[0]>vmax[0])
            {
            vmax[0] = p[0];
            }
          if (p[1]>vmax[1])
            {
            vmax[1] = p[1];
            }
          if (p[2]>vmax[2])
            {
            vmax[2] = p[2];
            }
          }


        float s = static_cast<float>(
          (scalars[index] * this->ScalarScale+this->ScalarShift + 0.5)
          /this->ScalarResolution);
        float mins = s;
        float maxs = s;

        corner_scalars[0] = s;

        for(j = 0; j < 8; j++)
          {
          index = cellPtIds->GetId(j);

          p = points + 3 * index;
          int corner = 0;
          if (p[0]==vmax[0])
            {
            corner += 4;
            }
          if (p[1]==vmax[1])
            {
            corner += 2;
            }
          if (p[2]==vmax[2])
            {
            corner += 1;
            }
          static const int corner_tbl[] = {0, 4, 1, 5, 3, 7, 2, 6};

          s = static_cast<float>(
            (scalars[index] * this->ScalarScale + this->ScalarShift + 0.5)
            /this->ScalarResolution);
          if (s < mins)
            {
            mins = s;
            }
          if (s > maxs)
            {
            maxs = s;
            }

          corner_scalars[corner_tbl[corner]] = s;

          }

        this->RenderHexahedron(vmin,vmax,corner_scalars);

      }

    numcellsrendered += num_cell_ids;
    }

  this->UnsetState();

  this->UpdateProgress(1.0);
}

// ----------------------------------------------------------------------------
void vtkOpenGLProjectedAAHexahedraMapper::ReleaseGraphicsResources(
  vtkWindow *win)
{
  if (this->PreintTexture)
    {
    GLuint texid = this->PreintTexture;
    glDeleteTextures(1, &texid);
    vtkOpenGLCheckErrorMacro("failed at glDeleteTextures");
    this->PreintTexture = 0;
    }
  this->Superclass::ReleaseGraphicsResources(win);
  if(this->Initialized)
    {
    delete[] pos_points;
    delete[] min_points;
    delete[] node_data1;
    delete[] node_data2;
    this->Initialized=false;
    }
  if(this->Shader!=0)
    {
    this->Shader->ReleaseGraphicsResources();
    }
}
