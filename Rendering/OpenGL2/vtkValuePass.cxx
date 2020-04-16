/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkValuePass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <cassert>

#include "vtkCompositeDataSet.h"
#include "vtkCompositePolyDataMapper2.h"
#include "vtkDataSet.h"
#include "vtkExecutive.h"
#include "vtkFloatArray.h"
#include "vtkLookupTable.h"
#include "vtkMapper.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLBufferObject.h"
#include "vtkOpenGLCellToVTKCellMap.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkPolyData.h"
#include "vtkProp.h"
#include "vtkProperty.h"
#include "vtkRenderState.h"
#include "vtkRenderWindow.h"
#include "vtkRenderbuffer.h"
#include "vtkShaderProgram.h"
#include "vtkSmartPointer.h"
#include "vtkTextureObject.h"
#include "vtkTimeStamp.h"
#include "vtkValuePass.h"
#include <vector>

struct vtkValuePass::Parameters
{
  Parameters()
  {
    ArrayMode = VTK_SCALAR_MODE_USE_POINT_FIELD_DATA;
    ArrayAccessMode = VTK_GET_ARRAY_BY_ID;
    ArrayId = 0;
    ArrayName = "";
    ArrayComponent = 0;
    // (min > max) means it is not initialized
    ScalarRange[0] = 1.0;
    ScalarRange[1] = -1.0;
    LookupTable = nullptr;
  };

  int ArrayMode;
  int ArrayAccessMode;
  int ArrayId;
  std::string ArrayName;
  int ArrayComponent;

  // Only useful for invertible mode.
  double ScalarRange[2];
  vtkScalarsToColors* LookupTable;
  bool Lighting;
};

////////////////////////////////////////////////////////////////////////////////
class vtkValuePass::vtkInternalsFloat
{
public:
  vtkInternalsFloat()
    : ValueFBO(nullptr)
    , ValueRBO(nullptr)
    , DepthRBO(nullptr)
    , FBOAllocated(false)
    , PointBuffer(nullptr)
    , BuffersAllocated(false)
    , CellFloatTexture(nullptr)
    , CellFloatBuffer(nullptr)
    , OutputFloatArray(vtkFloatArray::New())
  {
    this->FloatImageExt[0] = 0;
    this->FloatImageExt[1] = 0;
    this->FloatImageExt[2] = 0;
    this->FloatImageExt[3] = 0;
    this->FloatImageExt[4] = 0;
    this->FloatImageExt[5] = 0;

    this->ComponentBuffer->SetNumberOfComponents(1);
    this->OutputFloatArray->SetNumberOfComponents(1); /* GL_RED */
    this->CCMapTime = 0;
  }

  ~vtkInternalsFloat()
  {
    if (this->ValueFBO)
    {
      this->ValueFBO->Delete();
      this->ValueFBO = nullptr;
    }
    if (this->ValueRBO)
    {
      this->ValueRBO->Delete();
      this->ValueRBO = nullptr;
    }
    if (this->DepthRBO)
    {
      this->DepthRBO->Delete();
      this->DepthRBO = nullptr;
    }

    // Graphics resources released previously by the pass's parent
    if (this->PointBuffer)
    {
      this->PointBuffer->Delete();
      this->PointBuffer = nullptr;
    }

    if (this->CellFloatTexture)
    {
      this->CellFloatTexture->Delete();
      this->CellFloatTexture = nullptr;
    }

    if (this->CellFloatBuffer)
    {
      this->CellFloatBuffer->Delete();
      this->CellFloatBuffer = nullptr;
    }

    if (this->OutputFloatArray)
    {
      this->OutputFloatArray->Delete();
      this->OutputFloatArray = nullptr;
    }
  }

  vtkOpenGLFramebufferObject* ValueFBO;
  vtkRenderbuffer* ValueRBO;
  vtkRenderbuffer* DepthRBO;
  bool FBOAllocated;
  int FloatImageExt[6];

  vtkOpenGLBufferObject* PointBuffer;
  vtkNew<vtkFloatArray> ComponentBuffer;
  vtkTimeStamp DataUploadTime;
  bool BuffersAllocated;
  vtkTextureObject* CellFloatTexture;
  vtkOpenGLBufferObject* CellFloatBuffer;
  vtkFloatArray* OutputFloatArray;
  std::vector<vtkIdType> CellCellMap;
  vtkMTimeType CCMapTime;

private:
  vtkInternalsFloat(const vtkInternalsFloat&) = delete;
  void operator=(const vtkInternalsFloat&) = delete;
};

////////////////////////////////////////////////////////////////////////////////
class vtkValuePass::vtkInternalsInvertible
{
public:
  vtkInternalsInvertible(vtkValuePass* pass)
    : Pass(pass)
    , InvertibleLookupTable(nullptr)
  {
    this->CreateInvertibleLookupTable();
  };

  ~vtkInternalsInvertible()
  {
    if (this->InvertibleLookupTable)
    {
      this->InvertibleLookupTable->Delete();
    }
  };

  //-------------------------------------------------------------------
  void ClearInvertibleColor(vtkMapper* mapper, vtkProperty* property)
  {
    this->SetStateInMapper(this->OriginalState, mapper);
    property->SetLighting(this->OriginalState.Lighting);

    if (this->OriginalState.LookupTable != nullptr)
      this->OriginalState.LookupTable->UnRegister(Pass);

    this->OriginalState = Parameters();
  };

  /**
   * Makes a lookup table that can be used for deferred colormaps.
   */
  //----------------------------------------------------------------------------
  void CreateInvertibleLookupTable()
  {
    if (!this->InvertibleLookupTable)
    {
      vtkLookupTable* table = vtkLookupTable::New();
      const int MML = 0x1000;
      table->SetNumberOfTableValues(MML);
      table->SetBelowRangeColor(0.0, 0.0, 0.0, 1.0);
      table->SetAboveRangeColor(0.0, 0.0, 0.0, 1.0);
      table->SetNanColor(0.0, 0.0, 0.0, 1.0);
      unsigned char color[3] = { 0 };
      for (int i = 0; i < MML; ++i)
      {
        this->ValueToColor(i, 0, MML, color);
        table->SetTableValue(
          i, (double)color[0] / 255.0, (double)color[1] / 255.0, (double)color[2] / 255.0, 1.0);
      }
      this->InvertibleLookupTable = table;
    }
  };

  /**
   * Floating point value to an RGB triplet.
   */
  //----------------------------------------------------------------------------
  void ValueToColor(double const value, double const min, double const scale, unsigned char* color)
  {
    double valueS = (value - min) / scale;
    valueS = (valueS < 0.0 ? 0.0 : valueS);     // prevent underflow
    valueS = (valueS > 1.0 ? 1.0 : valueS);     // prevent overflow
    int const valueI = valueS * 0xfffffe + 0x1; // 0 is reserved as "nothing"

    color[0] = (unsigned char)((valueI & 0xff0000) >> 16);
    color[1] = (unsigned char)((valueI & 0x00ff00) >> 8);
    color[2] = (unsigned char)((valueI & 0x0000ff));
  };

  /**
   * RGB triplet to a floating point value.
   */
  //----------------------------------------------------------------------------
  void ColorToValue(unsigned char const* color, double const min, double const scale, double& value)
  {
    int const valueI =
      ((int)(*(color + 0))) << 16 | ((int)(*(color + 1))) << 8 | ((int)(*(color + 2)));
    double const valueS = (valueI - 0x1) / (double)0xfffffe; // 0 is reserved as "nothing"
    value = valueS * scale + min;
  };

  //-------------------------------------------------------------------
  void UseInvertibleColorFor(
    vtkMapper* mapper, vtkDataArray* dataArray, vtkProperty* property, Parameters* passParams)
  {
    this->CacheMapperState(mapper);
    this->OriginalState.Lighting = property->GetLighting();

    passParams->LookupTable = this->InvertibleLookupTable;
    passParams->Lighting = false;
    property->SetLighting(passParams->Lighting);

    // Ensure the scalar range is initialized
    if (passParams->ScalarRange[0] > passParams->ScalarRange[1])
    {
      double* range = dataArray->GetRange();
      passParams->ScalarRange[0] = range[0];
      passParams->ScalarRange[1] = range[1];
    }

    this->SetStateInMapper((*passParams), mapper);
  };

  //-------------------------------------------------------------------
  void CacheMapperState(vtkMapper* mapper)
  {
    Parameters& state = this->OriginalState;
    state.ArrayMode = mapper->GetScalarMode();
    state.ArrayAccessMode = mapper->GetArrayAccessMode();
    state.ArrayId = mapper->GetArrayId();
    state.ArrayName = std::string(mapper->GetArrayName());
    state.ArrayComponent = mapper->GetArrayComponent();
    mapper->GetScalarRange(state.ScalarRange);
    state.LookupTable = mapper->GetLookupTable();
    state.LookupTable->Register(Pass);
  };

  //-------------------------------------------------------------------
  void SetStateInMapper(Parameters& state, vtkMapper* mapper)
  {
    mapper->SetScalarMode(state.ArrayMode);
    mapper->SetArrayComponent(state.ArrayComponent);
    mapper->SetScalarRange(state.ScalarRange);
    mapper->SetArrayName(state.ArrayName.c_str());
    mapper->SetArrayId(state.ArrayId);
    mapper->SetArrayAccessMode(state.ArrayAccessMode);

    // Range and component should be set in the lut within the mapper, but
    // here are set anyway
    if (state.LookupTable != nullptr)
    {
      state.LookupTable->SetVectorComponent(state.ArrayComponent);
      state.LookupTable->SetRange(state.ScalarRange);
    }

    mapper->SetLookupTable(state.LookupTable);
  };

  vtkValuePass* Pass;

  vtkScalarsToColors* InvertibleLookupTable;

  Parameters OriginalState;

private:
  vtkInternalsInvertible(const vtkInternalsInvertible&) = delete;
  void operator=(const vtkInternalsInvertible&) = delete;
};

////////////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------------
vtkStandardNewMacro(vtkValuePass);

// ----------------------------------------------------------------------------
vtkValuePass::vtkValuePass()
  : ImplFloat(new vtkInternalsFloat())
  , ImplInv(new vtkInternalsInvertible(this))
  , PassState(new Parameters())
  , RenderingMode(FLOATING_POINT)
{
  this->MultiBlocksArray = nullptr;
}

// ----------------------------------------------------------------------------
vtkValuePass::~vtkValuePass()
{
  delete this->ImplFloat;
  delete this->ImplInv;
  delete this->PassState;
}

// ----------------------------------------------------------------------------
void vtkValuePass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------------
void vtkValuePass::SetInputArrayToProcess(int fieldAssociation, const char* name)
{
  if (this->PassState->ArrayAccessMode != VTK_GET_ARRAY_BY_NAME ||
    this->PassState->ArrayMode != fieldAssociation ||
    this->PassState->ArrayName.compare(name) != false)
  {
    this->PassState->ArrayMode = fieldAssociation;
    this->PassState->ArrayName = std::string(name);
    this->PassState->ArrayAccessMode = VTK_GET_ARRAY_BY_NAME;
    this->Modified();
  }
}

// ----------------------------------------------------------------------------
void vtkValuePass::SetInputArrayToProcess(int fieldAssociation, int fieldId)
{
  if (this->PassState->ArrayMode != fieldAssociation || this->PassState->ArrayId != fieldId ||
    this->PassState->ArrayAccessMode != VTK_GET_ARRAY_BY_ID)
  {
    this->PassState->ArrayMode = fieldAssociation;
    this->PassState->ArrayId = fieldId;
    this->PassState->ArrayAccessMode = VTK_GET_ARRAY_BY_ID;
    this->Modified();
  }
}

// ----------------------------------------------------------------------------
void vtkValuePass::SetInputComponentToProcess(int component)
{
  if (this->PassState->ArrayComponent != component)
  {
    this->PassState->ArrayComponent = component;
    this->Modified();
  }
}

#if !defined(VTK_LEGACY_REMOVE)
// ----------------------------------------------------------------------------
void vtkValuePass::SetScalarRange(double min, double max)
{
  VTK_LEGACY_BODY(vtkValuePass::SetScalarRange, "VTK 9.0");
  if ((this->PassState->ScalarRange[0] != min || this->PassState->ScalarRange[1] != max) &&
    min <= max)
  {
    this->PassState->ScalarRange[0] = min;
    this->PassState->ScalarRange[1] = max;
    this->Modified();
  }
}
#endif

// ----------------------------------------------------------------------------
void vtkValuePass::PopulateCellCellMap(const vtkRenderState* s)
{
  int const count = s->GetPropArrayCount();
  for (int i = 0; i < count; ++i)
  {
    vtkProp* prop = s->GetPropArray()[i];
    vtkActor* actor = vtkActor::SafeDownCast(prop);
    if (!actor)
    {
      continue;
    }
    vtkProperty* property = actor->GetProperty();
    vtkMapper* mapper = actor->GetMapper();

    vtkOpenGLPolyDataMapper* pdm = vtkOpenGLPolyDataMapper::SafeDownCast(mapper);
    if (!pdm)
    {
      continue;
    }

    vtkMTimeType maptime = pdm->GetInputDataObject(0, 0)->GetMTime();
    if (this->ImplFloat->CCMapTime >= maptime)
    {
      // reuse
      return;
    }
    this->ImplFloat->CellCellMap.clear();
    this->ImplFloat->CCMapTime = maptime;

    vtkCompositePolyDataMapper2* cpdm = vtkCompositePolyDataMapper2::SafeDownCast(mapper);
    if (cpdm)
    {
      vtkIdType offset = 0;
      std::vector<vtkPolyData*> pdl = cpdm->GetRenderedList();
      std::vector<vtkPolyData*>::iterator it;
      for (it = pdl.begin(); it != pdl.end(); ++it)
      {
        vtkPolyData* poly = *it;
        vtkCellArray* prims[4];
        prims[0] = poly->GetVerts();
        prims[1] = poly->GetLines();
        prims[2] = poly->GetPolys();
        prims[3] = poly->GetStrips();
        int representation = property->GetRepresentation();
        vtkPoints* points = poly->GetPoints();
        vtkNew<vtkOpenGLCellToVTKCellMap> aCellCellMap;
        aCellCellMap->Update(prims, representation, points);
        for (size_t c = 0; c < aCellCellMap->GetSize(); ++c)
        {
          this->ImplFloat->CellCellMap.push_back(aCellCellMap->GetValue(c) + offset);
        }
        offset += poly->GetNumberOfCells();
      }
    }
    else if (pdm)
    {
      vtkPolyData* poly = pdm->CurrentInput;
      vtkCellArray* prims[4];
      prims[0] = poly->GetVerts();
      prims[1] = poly->GetLines();
      prims[2] = poly->GetPolys();
      prims[3] = poly->GetStrips();
      int representation = property->GetRepresentation();
      vtkPoints* points = poly->GetPoints();
      vtkNew<vtkOpenGLCellToVTKCellMap> aCellCellMap;
      aCellCellMap->Update(prims, representation, points);
      for (size_t c = 0; c < aCellCellMap->GetSize(); ++c)
      {
        this->ImplFloat->CellCellMap.push_back(aCellCellMap->GetValue(c));
      }
    }

    break; // only ever draw one actor at a time in value mode so OK
  }
}

// ----------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void vtkValuePass::Render(const vtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);

  // GLRenderPass
  this->PreRender(s);

  if (this->RenderingMode == vtkValuePass::FLOATING_POINT &&
    this->PassState->ArrayMode == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA)
  {
    this->PopulateCellCellMap(s);
  }
  this->BeginPass(s->GetRenderer());
  this->NumberOfRenderedProps = 0;
  this->RenderOpaqueGeometry(s);
  this->EndPass();

  this->PostRender(s);
}

// ----------------------------------------------------------------------------
// Description:
// Opaque pass with key checking.
// \pre s_exists: s!=0
void vtkValuePass::RenderOpaqueGeometry(const vtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);

  int const count = s->GetPropArrayCount();
  for (int i = 0; i < count; i++)
  {
    vtkProp* prop = s->GetPropArray()[i];
    vtkActor* actor = vtkActor::SafeDownCast(prop);
    if (!actor)
    {
      continue;
    }

    vtkProperty* property = actor->GetProperty();
    vtkMapper* mapper = actor->GetMapper();

    vtkDataArray* dataArray = this->GetCurrentArray(mapper, this->PassState);
    if (!dataArray)
    {
      // this is OK, happens on internal nodes of multiblock for example
      continue;
    }

    this->BeginMapperRender(mapper, dataArray, property);

    // Cache scalar visibility state and turn it on
    int const currentVis = mapper->GetScalarVisibility();
    mapper->ScalarVisibilityOn();

    int const rendered = prop->RenderOpaqueGeometry(s->GetRenderer());
    this->NumberOfRenderedProps += rendered;

    mapper->SetScalarVisibility(currentVis);

    this->EndMapperRender(mapper, property);
  }
}

//------------------------------------------------------------------------------
void vtkValuePass::BeginPass(vtkRenderer* ren)
{
  vtkOpenGLState* ostate = static_cast<vtkOpenGLRenderer*>(ren)->GetState();

  switch (this->RenderingMode)
  {
    case vtkValuePass::FLOATING_POINT:
      // Allocate if necessary and bind frame buffer.
      if (this->HasWindowSizeChanged(ren))
      {
        this->ReleaseFBO(ren->GetRenderWindow());
      }

      if (this->InitializeFBO(ren))
      {
        ostate->PushDrawFramebufferBinding();
        this->ImplFloat->ValueFBO->Bind(GL_DRAW_FRAMEBUFFER);
        this->ImplFloat->ValueFBO->ActivateDrawBuffer(0);
      }

      this->InitializeBuffers(ren);
      break;

    case vtkValuePass::INVERTIBLE_LUT:
    default:
      // Cleanup in case FLOATING_POINT was active.
      this->ReleaseGraphicsResources(ren->GetRenderWindow());
      break;
  }

  // Clear buffers
  ostate->vtkglClearDepth(1.0);
  if (this->RenderingMode == vtkValuePass::FLOATING_POINT)
  {
    ostate->vtkglClearColor(vtkMath::Nan(), vtkMath::Nan(), vtkMath::Nan(), 0.0);
  }
  else
  {
    ostate->vtkglClearColor(0.0, 0.0, 0.0, 0.0);
  }

  ostate->vtkglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

//------------------------------------------------------------------------------
void vtkValuePass::EndPass()
{
  switch (this->RenderingMode)
  {
    case vtkValuePass::FLOATING_POINT:
      // Unbind the float FBO and glReadPixels to host side.
      this->ImplFloat->ValueFBO->GetContext()->GetState()->PopDrawFramebufferBinding();
      break;

    case vtkValuePass::INVERTIBLE_LUT:
    default:
      // Nothing to do in this mode.
      break;
  }
}

//------------------------------------------------------------------------------
bool vtkValuePass::HasWindowSizeChanged(vtkRenderer* ren)
{
  if (!this->ImplFloat->ValueFBO)
  {
    return true;
  }

  const int* size = ren->GetSize();
  int* fboSize = this->ImplFloat->ValueFBO->GetLastSize();

  return (fboSize[0] != size[0] || fboSize[1] != size[1]);
}

//------------------------------------------------------------------------------
bool vtkValuePass::InitializeFBO(vtkRenderer* ren)
{
  if (this->ImplFloat->FBOAllocated)
  {
    return true;
  }

  vtkOpenGLRenderWindow* renWin = vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());

  const int* size = ren->GetSize();
  // Allocate FBO's Color attachment target
  this->ImplFloat->ValueRBO = vtkRenderbuffer::New();
  this->ImplFloat->ValueRBO->SetContext(renWin);
  // CreateColorAttachment formats the attachment RGBA32F by
  // default, this is what vtkValuePass expects.
  this->ImplFloat->ValueRBO->CreateColorAttachment(size[0], size[1]);

  // Allocate FBO's depth attachment target
  this->ImplFloat->DepthRBO = vtkRenderbuffer::New();
  this->ImplFloat->DepthRBO->SetContext(renWin);
  this->ImplFloat->DepthRBO->CreateDepthAttachment(size[0], size[1]);

  // Initialize the FBO into which the float value pass is rendered.
  this->ImplFloat->ValueFBO = vtkOpenGLFramebufferObject::New();
  this->ImplFloat->ValueFBO->SetContext(renWin);
  renWin->GetState()->PushFramebufferBindings();
  this->ImplFloat->ValueFBO->Bind();
  this->ImplFloat->ValueFBO->InitializeViewport(size[0], size[1]);
  /* GL_COLOR_ATTACHMENT0 */
  this->ImplFloat->ValueFBO->AddColorAttachment(0, this->ImplFloat->ValueRBO);
  this->ImplFloat->ValueFBO->AddDepthAttachment(this->ImplFloat->DepthRBO);

  // Verify FBO
  if (!this->ImplFloat->ValueFBO->CheckFrameBufferStatus(GL_FRAMEBUFFER))
  {
    vtkErrorMacro("Failed to attach FBO.");
    this->ReleaseFBO(ren->GetRenderWindow());
    return false;
  }

  renWin->GetState()->PopFramebufferBindings();
  this->ImplFloat->FBOAllocated = true;

  return true;
}

//-----------------------------------------------------------------------------
void vtkValuePass::ReleaseFBO(vtkWindow* win)
{
  if (!this->ImplFloat->FBOAllocated)
  {
    return;
  }

  win->MakeCurrent();

  // Cleanup FBO (grahpics resources cleaned internally)
  this->ImplFloat->ValueFBO->Delete();
  this->ImplFloat->ValueFBO = nullptr;

  this->ImplFloat->ValueRBO->Delete();
  this->ImplFloat->ValueRBO = nullptr;

  this->ImplFloat->DepthRBO->Delete();
  this->ImplFloat->DepthRBO = nullptr;

  this->ImplFloat->FBOAllocated = false;
}

//-----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
bool vtkValuePass::IsFloatingPointModeSupported()
{
  VTK_LEGACY_BODY(vtkValuePass::IsFloatihngPointModeSupported, "VTK 9.0");
#ifdef GL_ES_VERSION_3_0
  return true;
#else
  return true;
#endif
}
#endif

//------------------------------------------------------------------------------
vtkFloatArray* vtkValuePass::GetFloatImageDataArray(vtkRenderer* ren)
{
  if (!this->ImplFloat->FBOAllocated)
  {
    return this->ImplFloat->OutputFloatArray;
  }

  int* size = this->ImplFloat->ValueFBO->GetLastSize();
  this->ImplFloat->OutputFloatArray->SetNumberOfTuples(size[0] * size[1]);

  // RGB channels are all equal in the FBO (they all contain the same rendered
  // values), by default RED is copied.
  vtkRenderWindow* renWin = ren->GetRenderWindow();
  renWin->MakeCurrent();
  this->GetFloatImageData(
    GL_RED, size[0], size[1], this->ImplFloat->OutputFloatArray->GetVoidPointer(0));

  return this->ImplFloat->OutputFloatArray;
}

//-------------------------------------------------------------------------------
void vtkValuePass::GetFloatImageData(
  int const format, int const width, int const height, void* data)
{
  // Prepare and bind value texture and FBO.
  this->ImplFloat->ValueFBO->GetContext()->GetState()->PushReadFramebufferBinding();
  this->ImplFloat->ValueFBO->Bind(GL_READ_FRAMEBUFFER);
  this->ImplFloat->ValueFBO->ActivateReadBuffer(0);

  // Calling pack alignment ensures any window size can be grabbed.
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
#ifndef GL_ES_VERSION_3_0
  glClampColor(GL_CLAMP_READ_COLOR, GL_FALSE);
#endif

  glReadPixels(0, 0, width, height, format, GL_FLOAT, data);

  this->ImplFloat->ValueFBO->GetContext()->GetState()->PopReadFramebufferBinding();

  vtkOpenGLCheckErrorMacro("Failed to read pixels from OpenGL buffer!");
}

//-------------------------------------------------------------------------------
int* vtkValuePass::GetFloatImageExtents()
{
  int* size = this->ImplFloat->ValueFBO->GetLastSize();

  this->ImplFloat->FloatImageExt[0] = 0;
  this->ImplFloat->FloatImageExt[1] = size[0] - 1;
  this->ImplFloat->FloatImageExt[2] = 0;
  this->ImplFloat->FloatImageExt[3] = size[1] - 1;
  this->ImplFloat->FloatImageExt[4] = 0;
  this->ImplFloat->FloatImageExt[5] = 0;

  return this->ImplFloat->FloatImageExt;
}

//-------------------------------------------------------------------------------
bool vtkValuePass::PostReplaceShaderValues(std::string& vertexShader,
  std::string& vtkNotUsed(geometryShader), std::string& fragmentShader,
  vtkAbstractMapper* vtkNotUsed(mapper), vtkProp* vtkNotUsed(prop))
{
  bool success = true;
  if (this->RenderingMode == vtkValuePass::FLOATING_POINT)
  {
    success = this->UpdateShaders(vertexShader, fragmentShader);
  }

  return success;
}

//-------------------------------------------------------------------------------
bool vtkValuePass::SetShaderParameters(vtkShaderProgram* program,
  vtkAbstractMapper* vtkNotUsed(mapper), vtkProp* vtkNotUsed(prop), vtkOpenGLVertexArrayObject* VAO)
{
  if (this->RenderingMode == vtkValuePass::FLOATING_POINT)
  {
    this->BindAttributes(program, VAO);
    this->BindUniforms(program);
  }

  return true;
}

//-------------------------------------------------------------------------------
vtkMTimeType vtkValuePass::GetShaderStageMTime()
{
  return this->GetMTime();
}

//-----------------------------------------------------------------------------
void vtkValuePass::ReleaseGraphicsResources(vtkWindow* win)
{
  // Release buffers
  if (this->ImplFloat->CellFloatTexture)
  {
    this->ImplFloat->CellFloatTexture->ReleaseGraphicsResources(win);
    this->ImplFloat->CellFloatTexture->Delete();
    this->ImplFloat->CellFloatTexture = nullptr;
  }

  if (this->ImplFloat->CellFloatBuffer)
  {
    this->ImplFloat->CellFloatBuffer->ReleaseGraphicsResources();
    this->ImplFloat->CellFloatBuffer->Delete();
    this->ImplFloat->CellFloatBuffer = nullptr;
  }

  if (this->ImplFloat->PointBuffer)
  {
    this->ImplFloat->PointBuffer->ReleaseGraphicsResources();
    this->ImplFloat->PointBuffer->Delete();
    this->ImplFloat->PointBuffer = nullptr;
  }
  this->ImplFloat->BuffersAllocated = false;

  this->ReleaseFBO(win);
}

//-----------------------------------------------------------------------------
void vtkValuePass::RenderPieceFinish()
{
  if (this->PassState->ArrayMode == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA)
  {
    if (this->ImplFloat->CellFloatTexture)
    {
      this->ImplFloat->CellFloatTexture->Deactivate();
    }
  }
}

//-----------------------------------------------------------------------------
void vtkValuePass::RenderPieceStart(vtkDataArray* dataArr, vtkMapper* mapper)
{
  // TODO It should only be necessary to upload the data if something has changed.
  // In the parallel case however (ParaView with IceT), the solution below causes
  // data not to be uploaded at all (leading to empty images). Because of this, data
  // is uploaded on every render pass.
  vtkOpenGLPolyDataMapper* pdm = vtkOpenGLPolyDataMapper::SafeDownCast(mapper);
  if (!pdm)
  {
    return;
  }
  vtkMTimeType maptime = pdm->GetInputDataObject(0, 0)->GetMTime();

  if (this->GetMTime() > this->ImplFloat->DataUploadTime ||
    maptime > this->ImplFloat->DataUploadTime)
  {
    // Copy the selected component into a buffer for uploading
    vtkIdType const numTuples = dataArr->GetNumberOfTuples();
    int const comp = this->PassState->ArrayComponent;
    this->ImplFloat->ComponentBuffer->SetNumberOfTuples(numTuples);
    this->ImplFloat->ComponentBuffer->CopyComponent(0, dataArr, comp);
    this->ImplFloat->ComponentBuffer->Modified();
    float const* data = static_cast<float*>(this->ImplFloat->ComponentBuffer->GetVoidPointer(0));

    // Upload array data
    if (this->PassState->ArrayMode == VTK_SCALAR_MODE_USE_POINT_FIELD_DATA)
    {
      this->ImplFloat->PointBuffer->Upload(
        data, static_cast<size_t>(numTuples), vtkOpenGLBufferObject::ArrayBuffer);
    }
    else if (this->PassState->ArrayMode == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA)
    {
      // unroll the cell values such that every drawn triangle
      // gets a copy of the value from its parent cell
      // todo: cache and reuse if are stuck with uploading always
      size_t len = this->ImplFloat->CellCellMap.size();
      float* unrolled_data = new float[len];
      for (size_t i = 0; i < len; ++i)
      {
        unrolled_data[i] = data[this->ImplFloat->CellCellMap[i]];
      }
      this->ImplFloat->CellFloatBuffer->Upload(
        unrolled_data, len, vtkOpenGLBufferObject::TextureBuffer);
      delete[] unrolled_data;

      this->ImplFloat->CellFloatTexture->CreateTextureBuffer(
        static_cast<unsigned int>(numTuples), 1, VTK_FLOAT, this->ImplFloat->CellFloatBuffer);
    }
    else
    {
      vtkErrorMacro("Scalar mode " << this->PassState->ArrayMode << " is not supported!");
    }
    this->ImplFloat->DataUploadTime.Modified();
  }

  // Bind textures
  if (this->PassState->ArrayMode == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA)
  {
    this->ImplFloat->CellFloatTexture->Activate();
  }
}

//-----------------------------------------------------------------------------
void vtkValuePass::BeginMapperRender(
  vtkMapper* mapper, vtkDataArray* dataArray, vtkProperty* property)
{
  switch (this->RenderingMode)
  {
    case vtkValuePass::INVERTIBLE_LUT:
      this->ImplInv->UseInvertibleColorFor(mapper, dataArray, property, this->PassState);
      break;

    case vtkValuePass::FLOATING_POINT:
      this->RenderPieceStart(dataArray, mapper);
      break;

    default:
      vtkErrorMacro("Unsupported rendering mode!");
      break;
  }
}

//------------------------------------------------------------------------------
void vtkValuePass::EndMapperRender(vtkMapper* mapper, vtkProperty* property)
{
  switch (this->RenderingMode)
  {
    case vtkValuePass::INVERTIBLE_LUT:
      this->ImplInv->ClearInvertibleColor(mapper, property);
      break;

    case vtkValuePass::FLOATING_POINT:
      this->RenderPieceFinish();
      break;

    default:
      vtkErrorMacro("Unsupported rendering mode!");
      break;
  }
}

//-----------------------------------------------------------------------------
void vtkValuePass::InitializeBuffers(vtkRenderer* ren)
{
  if (this->ImplFloat->BuffersAllocated)
  {
    return;
  }

  // For point data
  this->ImplFloat->PointBuffer = vtkOpenGLBufferObject::New();
  this->ImplFloat->PointBuffer->SetType(vtkOpenGLBufferObject::ArrayBuffer);

  // For cell data
  this->ImplFloat->CellFloatTexture = vtkTextureObject::New();
  this->ImplFloat->CellFloatTexture->SetContext(
    static_cast<vtkOpenGLRenderWindow*>(ren->GetVTKWindow()));

  this->ImplFloat->CellFloatBuffer = vtkOpenGLBufferObject::New();
  this->ImplFloat->CellFloatBuffer->SetType(vtkOpenGLBufferObject::TextureBuffer);

  this->ImplFloat->BuffersAllocated = true;
}

//-----------------------------------------------------------------------------
bool vtkValuePass::UpdateShaders(std::string& VSSource, std::string& FSSource)
{
  vtkShaderProgram::Substitute(VSSource, "//VTK::ValuePass::Dec",
    "in float dataAttribute;\n"
    "out float dataValue;\n"
    "uniform samplerBuffer textureF;\n");

  vtkShaderProgram::Substitute(VSSource, "//VTK::ValuePass::Impl",
    "  // Pass the 'value' attribute to the fragment shader varying\n"
    "  dataValue = dataAttribute;\n");

  vtkShaderProgram::Substitute(FSSource, "//VTK::ValuePass::Dec",
    "in float dataValue;\n"
    "uniform samplerBuffer textureF;\n");

  std::string fragImpl;
  if (this->PassState->ArrayMode == VTK_SCALAR_MODE_USE_POINT_FIELD_DATA)
  {
    fragImpl =
      std::string("  gl_FragData[0] = vec4(vec3(dataValue), 1.0);\n"
                  "  // Return right away since vtkValuePass::FLOATING_POINT mode is attached\n"
                  "  return;");
  }
  else if (this->PassState->ArrayMode == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA)
  {
    fragImpl =
      std::string("  gl_FragData[0] = texelFetchBuffer(textureF, gl_PrimitiveID +\n"
                  "    PrimitiveIDOffset);\n"
                  "  // Return right away since vtkValuePass::FLOATING_POINT mode is attached\n"
                  "  return;");
  }

  return vtkShaderProgram::Substitute(FSSource, "//VTK::ValuePass::Impl", fragImpl);
}

//-----------------------------------------------------------------------------
void vtkValuePass::BindAttributes(vtkShaderProgram* prog, vtkOpenGLVertexArrayObject* VAO)
{
  if (this->PassState->ArrayMode == VTK_SCALAR_MODE_USE_POINT_FIELD_DATA)
  {
    if (prog->IsAttributeUsed("dataAttribute"))
    {
      size_t const stride = sizeof(float);

      if (!VAO->AddAttributeArray(
            prog, this->ImplFloat->PointBuffer, "dataAttribute", 0, stride, VTK_FLOAT, 1, false))
      {
        vtkErrorMacro(<< "Error setting 'dataAttribute' in shader VAO.");
      }
    }
  }
}

//-----------------------------------------------------------------------------
void vtkValuePass::BindUniforms(vtkShaderProgram* prog)
{
  if (this->PassState->ArrayMode == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA)
  {
    if (prog->IsAttributeUsed("textureF"))
    {
      int tunit = this->ImplFloat->CellFloatTexture->GetTextureUnit();
      prog->SetUniformi("textureF", tunit);
    }
  }
}

//-------------------------------------------------------------------
vtkDataArray* vtkValuePass::GetCurrentArray(vtkMapper* mapper, Parameters* arrayPar)
{
  // Check for a regular data set
  vtkAbstractArray* abstractArray = nullptr;
  vtkDataObject* dataObject = mapper->GetExecutive()->GetInputData(0, 0);
  vtkDataSet* input = vtkDataSet::SafeDownCast(dataObject);
  if (input)
  {
    int cellFlag;
    abstractArray = vtkAbstractMapper::GetAbstractScalars(input, arrayPar->ArrayMode,
      arrayPar->ArrayAccessMode, arrayPar->ArrayId, arrayPar->ArrayName.c_str(), cellFlag);
  }

  // Check for a composite data set
  if (!abstractArray)
  {
    abstractArray = this->GetArrayFromCompositeData(mapper, arrayPar);
    this->MultiBlocksArray = abstractArray;
    if (abstractArray)
    {
      abstractArray->Delete();
    }
  }

  vtkDataArray* dataArray = vtkArrayDownCast<vtkDataArray>(abstractArray);
  return dataArray;
}

//-------------------------------------------------------------------
vtkAbstractArray* vtkValuePass::GetArrayFromCompositeData(vtkMapper* mapper, Parameters* arrayPar)
{
  vtkAbstractArray* abstractArray = nullptr;
  vtkCompositePolyDataMapper2* cpdm = vtkCompositePolyDataMapper2::SafeDownCast(mapper);
  if (cpdm)
  {
    std::vector<vtkPolyData*> pdl = cpdm->GetRenderedList();
    std::vector<vtkPolyData*>::iterator it;
    for (it = pdl.begin(); it != pdl.end(); ++it)
    {
      vtkPolyData* pd = *it;
      int cellFlag;
      vtkAbstractArray* blocksArray = vtkAbstractMapper::GetAbstractScalars(pd, arrayPar->ArrayMode,
        arrayPar->ArrayAccessMode, arrayPar->ArrayId, arrayPar->ArrayName.c_str(), cellFlag);

      if (blocksArray)
      {
        if (!abstractArray)
        {
          abstractArray = blocksArray->NewInstance();
          abstractArray->DeepCopy(blocksArray);
        }
        else
        {
          abstractArray->InsertTuples(
            abstractArray->GetNumberOfTuples(), blocksArray->GetNumberOfTuples(), 0, blocksArray);
        }
      }
    }
  }

  return abstractArray;
}

//-------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
void vtkValuePass::ColorToValue(
  unsigned char const* color, double const min, double const scale, double& value)
{
  VTK_LEGACY_BODY(vtkValuePass::ColorToValue, "VTK 9.0");
  this->ImplInv->ColorToValue(color, min, scale, value);
}
#endif
