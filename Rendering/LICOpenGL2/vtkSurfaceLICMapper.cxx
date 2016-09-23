/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSurfaceLICMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSurfaceLICMapper.h"

#include "vtkSurfaceLICInterface.h"


#include "vtkObjectFactory.h"
#include "vtkOpenGLError.h"
#include "vtkPainterCommunicator.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkShaderProgram.h"

#include "vtkOpenGLVertexBufferObject.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkOpenGLIndexBufferObject.h"

// use parallel timer for benchmarks and scaling
// if not defined vtkTimerLOG is used.
// #define vtkSurfaceLICMapperTIME
#if !defined(vtkSurfaceLICMapperTIME)
#include "vtkTimerLog.h"
#endif

//----------------------------------------------------------------------------
vtkObjectFactoryNewMacro(vtkSurfaceLICMapper);

//----------------------------------------------------------------------------
vtkSurfaceLICMapper::vtkSurfaceLICMapper()
{
  this->SetInputArrayToProcess(0,0,0,
    vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS,
    vtkDataSetAttributes::VECTORS);

  this->VectorVBO = vtkOpenGLVertexBufferObject::New();
  this->LICInterface = vtkSurfaceLICInterface::New();
}

//----------------------------------------------------------------------------
vtkSurfaceLICMapper::~vtkSurfaceLICMapper()
{
  #if vtkSurfaceLICMapperDEBUG >= 1
  cerr << "=====vtkSurfaceLICMapper::~vtkSurfaceLICMapper" << endl;
  #endif

  this->VectorVBO->Delete();
  this->VectorVBO = 0;
  this->LICInterface->Delete();
  this->LICInterface = 0;
}

void vtkSurfaceLICMapper::ShallowCopy(vtkAbstractMapper *mapper)
{
  vtkSurfaceLICMapper *m = vtkSurfaceLICMapper::SafeDownCast(mapper);
  this->LICInterface->ShallowCopy(m->GetLICInterface());

  this->SetInputArrayToProcess(0,
    m->GetInputArrayInformation(0));
  this->SetScalarVisibility(m->GetScalarVisibility());

  // Now do superclass
  this->vtkOpenGLPolyDataMapper::ShallowCopy(mapper);
}

//----------------------------------------------------------------------------
void vtkSurfaceLICMapper::ReleaseGraphicsResources(vtkWindow* win)
{
  this->LICInterface->ReleaseGraphicsResources(win);
  this->VectorVBO->ReleaseGraphicsResources();
  this->Superclass::ReleaseGraphicsResources(win);
}

void vtkSurfaceLICMapper::ReplaceShaderValues(
    std::map<vtkShader::Type, vtkShader *> shaders,
    vtkRenderer *ren, vtkActor *actor)
{
  std::string VSSource = shaders[vtkShader::Vertex]->GetSource();
  std::string FSSource = shaders[vtkShader::Fragment]->GetSource();

  // add some code to handle the LIC vectors and mask
  vtkShaderProgram::Substitute(VSSource,
    "//VTK::TCoord::Dec",
    "attribute vec3 vecsMC;\n"
    "varying vec3 tcoordVCVSOutput;\n"
    );

  vtkShaderProgram::Substitute(VSSource, "//VTK::TCoord::Impl",
    "tcoordVCVSOutput = vecsMC;"
    );

  vtkShaderProgram::Substitute(FSSource,
    "//VTK::TCoord::Dec",
    // 0/1, when 1 V is projected to surface for |V| computation.
    "uniform int uMaskOnSurface;\n"
    "uniform mat3 normalMatrix;\n"
    "varying vec3 tcoordVCVSOutput;"
    );

  vtkShaderProgram::Substitute(FSSource,
    "//VTK::TCoord::Impl",
    // projected vectors
    "  vec3 tcoordLIC = normalMatrix * tcoordVCVSOutput;\n"
    "  vec3 normN = normalize(normalVCVSOutput);\n"
    "  float k = dot(tcoordLIC, normN);\n"
    "  tcoordLIC = (tcoordLIC - k*normN);\n"
    "  gl_FragData[1] = vec4(tcoordLIC.x, tcoordLIC.y, 0.0 , gl_FragCoord.z);\n"
 //   "  gl_FragData[1] = vec4(tcoordVC.xyz, gl_FragCoord.z);\n"
    // vectors for fragment masking
    "  if (uMaskOnSurface == 0)\n"
    "    {\n"
    "    gl_FragData[2] = vec4(tcoordVCVSOutput, gl_FragCoord.z);\n"
    "    }\n"
    "  else\n"
    "    {\n"
    "    gl_FragData[2] = vec4(tcoordLIC.x, tcoordLIC.y, 0.0 , gl_FragCoord.z);\n"
    "    }\n"
 //   "  gl_FragData[2] = vec4(19.0, 19.0, tcoordVC.x, gl_FragCoord.z);\n"
    , false);

  shaders[vtkShader::Vertex]->SetSource(VSSource);
  shaders[vtkShader::Fragment]->SetSource(FSSource);

  this->Superclass::ReplaceShaderValues(shaders,ren,actor);
}

void vtkSurfaceLICMapper::SetMapperShaderParameters(
  vtkOpenGLHelper &cellBO,
  vtkRenderer* ren, vtkActor *actor)
{
  if (cellBO.IBO->IndexCount && (this->VBOBuildTime > cellBO.AttributeUpdateTime ||
      cellBO.ShaderSourceTime > cellBO.AttributeUpdateTime))
  {
    cellBO.VAO->Bind();
    if (!cellBO.VAO->AddAttributeArray(cellBO.Program, this->VectorVBO,
        "vecsMC", this->VectorVBO->TCoordOffset,
         this->VectorVBO->Stride, VTK_FLOAT, this->VectorVBO->TCoordComponents,
         false))
    {
      vtkErrorMacro(<< "Error setting 'vecsMC' in shader VAO.");
    }
  }

  this->Superclass::SetMapperShaderParameters(cellBO, ren, actor);
  cellBO.Program->SetUniformi("uMaskOnSurface",
    this->LICInterface->GetMaskOnSurface());
}

//----------------------------------------------------------------------------
void vtkSurfaceLICMapper::RenderPiece(
        vtkRenderer *renderer,
        vtkActor *actor)
{
  #ifdef vtkSurfaceLICMapperTIME
  this->StartTimerEvent("vtkSurfaceLICMapper::RenderInternal");
  #else
  vtkSmartPointer<vtkTimerLog> timer = vtkSmartPointer<vtkTimerLog>::New();
  timer->StartTimer();
  #endif

  vtkOpenGLClearErrorMacro();

  this->LICInterface->ValidateContext(renderer);

  this->LICInterface->UpdateCommunicator(renderer, actor, this->GetInput());

  vtkPainterCommunicator *comm = this->LICInterface->GetCommunicator();

  if (comm->GetIsNull())
  {
    // other rank's may have some visible data but we
    // have none and should not participate further
    return;
  }

  this->CurrentInput = this->GetInput();
  vtkDataArray *vectors = NULL;
  vectors = this->GetInputArrayToProcess(0, this->CurrentInput);
  this->LICInterface->SetHasVectors(vectors != NULL ? true : false);

  if (!this->LICInterface->CanRenderSurfaceLIC(actor))
  {
    // we've determined that there's no work for us, or that the
    // requisite opengl extensions are not available. pass control on
    // to delegate renderer and return.
    this->Superclass::RenderPiece(renderer, actor);
    #ifdef vtkSurfaceLICMapperTIME
    this->EndTimerEvent("vtkSurfaceLICMapper::RenderInternal");
    #endif
    return;
  }

  // allocate rendering resources, initialize or update
  // textures and shaders.
  this->LICInterface->InitializeResources();

  // draw the geometry
  this->LICInterface->PrepareForGeometry();
  this->RenderPieceStart(renderer, actor);
  this->RenderPieceDraw(renderer, actor);
  this->RenderEdges(renderer,actor);
  this->RenderPieceFinish(renderer, actor);
  this->LICInterface->CompletedGeometry();

  // --------------------------------------------- compoiste vectors for parallel LIC
  this->LICInterface->GatherVectors();

  // ------------------------------------------- LIC on screen
  this->LICInterface->ApplyLIC();

  // ------------------------------------------- combine scalar colors + LIC
  this->LICInterface->CombineColorsAndLIC();

  // ----------------------------------------------- depth test and copy to screen
  this->LICInterface->CopyToScreen();

  // clear opengl error flags and be absolutely certain that nothing failed.
  vtkOpenGLCheckErrorMacro("failed during surface lic painter");

  #ifdef vtkSurfaceLICMapperTIME
  this->EndTimerEvent("vtkSurfaceLICMapper::RenderInternal");
  #else
  timer->StopTimer();
  #endif
}

//-------------------------------------------------------------------------
void vtkSurfaceLICMapper::BuildBufferObjects(vtkRenderer *ren, vtkActor *act)
{
  this->Superclass::BuildBufferObjects(ren,act);

  if (!this->LICInterface->GetHasVectors())
  {
    return;
  }

  vtkDataArray *vectors = NULL;
  vectors = this->GetInputArrayToProcess(0, this->CurrentInput);

  int numComp = vectors->GetNumberOfComponents();
  this->VectorVBO->VertexCount = vectors->GetNumberOfTuples();
  this->VectorVBO->TCoordComponents = numComp;
  this->VectorVBO->TCoordOffset = 0;
  this->VectorVBO->Stride = this->VectorVBO->TCoordComponents*sizeof(float);

  if (vectors->GetDataType() != VTK_FLOAT)
  {
    float *data = new float[vectors->GetNumberOfTuples()*numComp];
    double *tuple = new double [numComp];
    for (int i = 0; i < vectors->GetNumberOfTuples(); i++)
    {
      vectors->GetTuple(i,tuple);
      for (int j = 0; j < numComp; j++)
      {
        data[i*numComp+j] = tuple[j];
      }
    }
    this->VectorVBO->Upload(data,
      vectors->GetNumberOfTuples()*numComp,
      vtkOpenGLBufferObject::ArrayBuffer);
    delete [] data;
    delete [] tuple;
  }
  else
  {
    // and add our vector VBO
    this->VectorVBO->Upload(static_cast<float *>(vectors->GetVoidPointer(0)),
      vectors->GetNumberOfTuples()*numComp,
      vtkOpenGLBufferObject::ArrayBuffer);
  }
}


//----------------------------------------------------------------------------
void vtkSurfaceLICMapper::PrintSelf(ostream & os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
