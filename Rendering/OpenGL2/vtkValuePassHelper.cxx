#include "vtkDataArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkMapper.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLBufferObject.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLHelper.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkPolyData.h"
#include "vtkRenderer.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"
#include "vtkValuePass.h"
#include "vtkValuePassHelper.h"
#include "vtkNew.h"


vtkStandardNewMacro(vtkValuePassHelper)

//------------------------------------------------------------------------------
class vtkValuePassHelper::vtkInternals
{
public:

  vtkInternals()
  : PointBuffer(NULL)
  , ValuePassArray(NULL)
  , CurrentDataArrayMode(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA)
  , LastDataArrayMode(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA)
  , ResourcesAllocated(false)
  , CellFloatTexture(NULL)
  , CellFloatBuffer(NULL)
  {
    this->CurrentValues->SetNumberOfComponents(1);
  }

  ~vtkInternals()
  {
    // Graphics resources released previously by the parent mapper or after switching
    // to INVERTIBLE_LUT mode
    if (this->PointBuffer)
    {
      this->PointBuffer->Delete();
      this->PointBuffer = NULL;
    }

    if (this->CellFloatTexture)
    {
      this->CellFloatTexture->Delete();
      this->CellFloatTexture = NULL;
    }

    if (this->CellFloatBuffer)
    {
      this->CellFloatBuffer->Delete();
      this->CellFloatBuffer = NULL;
    }
  }

  vtkOpenGLBufferObject* PointBuffer;
  vtkDataArray* ValuePassArray;
  vtkNew<vtkFloatArray> CurrentValues;
  int CurrentDataArrayMode;
  int LastDataArrayMode;
  bool ResourcesAllocated;
  vtkTextureObject* CellFloatTexture;
  vtkOpenGLBufferObject* CellFloatBuffer;
};

////////////////////////////////////////////////////////////////////////////////
vtkValuePassHelper::vtkValuePassHelper()
: Impl(new vtkInternals)
, RenderingMode(-1)
{
}

//-----------------------------------------------------------------------------
vtkValuePassHelper::~vtkValuePassHelper()
{
  delete Impl;
}

//-----------------------------------------------------------------------------
void vtkValuePassHelper::ReleaseGraphicsResources(vtkWindow *win)
{
  if (this->Impl->CellFloatTexture)
  {
    this->Impl->CellFloatTexture->ReleaseGraphicsResources(win);
    this->Impl->CellFloatTexture->Delete();
    this->Impl->CellFloatTexture = NULL;
  }

  if (this->Impl->CellFloatBuffer)
  {
    this->Impl->CellFloatBuffer->ReleaseGraphicsResources();
    this->Impl->CellFloatBuffer->Delete();
    this->Impl->CellFloatBuffer = NULL;
  }

  if (this->Impl->PointBuffer)
  {
    this->Impl->PointBuffer->ReleaseGraphicsResources();
    this->Impl->PointBuffer->Delete();
    this->Impl->PointBuffer = NULL;
  }

  this->Impl->ValuePassArray = NULL;
  this->Impl->ResourcesAllocated = false;
}

//-----------------------------------------------------------------------------
void vtkValuePassHelper::RenderPieceFinish()
{
  if (this->Impl->CurrentDataArrayMode == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA)
  {
    if (this->Impl->CellFloatTexture)
    {
      this->Impl->CellFloatTexture->Deactivate();
    }
  }
}

//-----------------------------------------------------------------------------
void vtkValuePassHelper::RenderPieceStart(vtkActor* actor, vtkDataSet* input)
{
  vtkInformation *info = actor->GetPropertyKeys();

  // TODO It should only be necessary to upload the data if something has changed.
  // In the parallel case however (ParaView with IceT), the solution below causes
  // data not to be uploaded at all (leading to empty images). Because of this, data
  // is uploaded on every render pass.
  //
  //if (info && info->Has(vtkValuePass::RELOAD_DATA()))
  if (info)
  {
    int cellFlag = 0;
    typedef vtkValuePass vp;
    this->Impl->ValuePassArray = vtkAbstractMapper::GetScalars(input,
      info->Get(vp::SCALAR_MODE()), info->Get(vp::ARRAY_MODE()),
      info->Get(vp::ARRAY_ID()), info->Get(vp::ARRAY_NAME()), cellFlag);

    if (!this->Impl->ValuePassArray)
    {
      vtkErrorMacro("Invalid data array from GetScalars()!");
      return;
    }

    vtkIdType const numTuples = this->Impl->ValuePassArray->GetNumberOfTuples();
    int const comp = info->Get(vp::ARRAY_COMPONENT());
    this->Impl->CurrentValues->SetNumberOfTuples(numTuples);
    this->Impl->CurrentValues->CopyComponent(0, this->Impl->ValuePassArray, comp);
    float const* data = static_cast<float*>(this->Impl->CurrentValues->GetVoidPointer(0));

    // Upload array data
    if (this->Impl->CurrentDataArrayMode == VTK_SCALAR_MODE_USE_POINT_FIELD_DATA)
    {
      // Point data
      this->Impl->PointBuffer->Upload(data, static_cast<size_t>(numTuples),
        vtkOpenGLBufferObject::ArrayBuffer);
    }
    else
    {
      // Cell data
      this->Impl->CellFloatBuffer->Upload(data, static_cast<size_t>(numTuples),
        vtkOpenGLBufferObject::TextureBuffer);

      this->Impl->CellFloatTexture->CreateTextureBuffer(static_cast<unsigned int>(numTuples),
        1, VTK_FLOAT, this->Impl->CellFloatBuffer);
    }
  }

  // Bind textures
  if (this->Impl->CurrentDataArrayMode == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA)
  {
    this->Impl->CellFloatTexture->Activate();
  }
}

//-----------------------------------------------------------------------------
void vtkValuePassHelper::UpdateConfiguration(vtkRenderer* ren, vtkActor* act,
  vtkMapper* mapper, vtkPolyData* input)
{
  this->RenderingMode = -1;
  vtkInformation *info = act->GetPropertyKeys();
  if (info && info->Has(vtkValuePass::RENDER_VALUES()))
  {
    this->RenderingMode = info->Get(vtkValuePass::RENDER_VALUES());
  }

  // Configure the mapper's behavior if the ValuePass is active.
  if (this->RenderingMode > 0)
  {
    // Since it has RENDER_VALUES it is assumed it has all the tags from ValuePass
    this->Impl->CurrentDataArrayMode = info->Get(vtkValuePass::SCALAR_MODE());

    switch (this->RenderingMode)
    {
      case vtkValuePass::FLOATING_POINT:
        this->AllocateGraphicsResources(ren);
        break;

      case vtkValuePass::INVERTIBLE_LUT:
      default:
      {
        mapper->UseInvertibleColorFor(input,
                                    info->Get(vtkValuePass::SCALAR_MODE()),
                                    info->Get(vtkValuePass::ARRAY_MODE()),
                                    info->Get(vtkValuePass::ARRAY_ID()),
                                    info->Get(vtkValuePass::ARRAY_NAME()),
                                    info->Get(vtkValuePass::ARRAY_COMPONENT()),
                                    info->Get(vtkValuePass::SCALAR_RANGE()));
      }
        break;
    }
  }
  else
  {
    this->ReleaseGraphicsResources(ren->GetRenderWindow());
    mapper->ClearInvertibleColor();
  }
}

//-----------------------------------------------------------------------------
void vtkValuePassHelper::AllocateGraphicsResources(vtkRenderer* ren)
{
  if (this->Impl->ResourcesAllocated)
  {
    return;
  }

  // For point data
  this->Impl->PointBuffer = vtkOpenGLBufferObject::New();
  this->Impl->PointBuffer->SetType(vtkOpenGLBufferObject::ArrayBuffer);

  // For cell data
  this->Impl->CellFloatTexture = vtkTextureObject::New();
  this->Impl->CellFloatTexture->SetContext
    (static_cast<vtkOpenGLRenderWindow*>(ren->GetVTKWindow()));

  this->Impl->CellFloatBuffer = vtkOpenGLBufferObject::New();
  this->Impl->CellFloatBuffer->SetType(vtkOpenGLBufferObject::TextureBuffer);

  this->Impl->ResourcesAllocated = true;
}

//-----------------------------------------------------------------------------
void vtkValuePassHelper::UpdateShaders(std::string & VSSource, std::string & FSSource,
  std::string & required)
{
  // Pass the value pass attribute to the fragment shader.
  vtkShaderProgram::Substitute(VSSource, "//VTK::ValuePass::Dec",
    "attribute float dataAttribute;\n"
    "varying float dataValue;\n"
    "uniform samplerBuffer textureF;\n"
    );

  vtkShaderProgram::Substitute(VSSource, "//VTK::ValuePass::Impl",
    " dataValue = dataAttribute;\n");

  // Render floating point values (variables in 'required' are a requirement in
  // other sections of the fragment shader, so they are included for it to build
  // correctly).
  vtkShaderProgram::Substitute(FSSource, "//VTK::ValuePass::Dec",
    "varying float dataValue;\n"
    "uniform samplerBuffer textureF;\n");

  if (this->Impl->CurrentDataArrayMode == VTK_SCALAR_MODE_USE_POINT_FIELD_DATA)
  {
    required += std::string
      (
       "  vec4 texColor = vec4(vec3(dataValue), 1.0);\n"
       "  gl_FragData[0] = texColor;\n"
       "  // Return right away since vtkValuePass::FLOATING_POINT mode is enabled\n"
       "  return;"
       );
  }
  else
  {
    required += std::string
      (
       "  gl_FragData[0] = texelFetchBuffer(textureF, gl_PrimitiveID + PrimitiveIDOffset);\n"

       "  // Return right away since vtkValuePass::FLOATING_POINT mode is enabled\n"
       "  return;"
       );
  }

  vtkShaderProgram::Substitute(FSSource, "//VTK::Color::Impl", required);
}

//-----------------------------------------------------------------------------
void vtkValuePassHelper::BindAttributes(vtkOpenGLHelper& cellBO)
{
  if (this->Impl->CurrentDataArrayMode == VTK_SCALAR_MODE_USE_POINT_FIELD_DATA)
  {
    if (this->Impl->ValuePassArray)
    {
      if (cellBO.Program->IsAttributeUsed("dataAttribute"))
      {
        size_t const stride = sizeof(float);

        if (!cellBO.VAO->AddAttributeArray(cellBO.Program, this->Impl->PointBuffer,
          "dataAttribute", 0, stride, VTK_FLOAT, 1, false))
        {
          vtkErrorMacro(<< "Error setting 'dataAttribute' in shader VAO.");
        }
      }
    }
  }
}

//-----------------------------------------------------------------------------
void vtkValuePassHelper::BindUniforms(vtkOpenGLHelper& cellBO)
{
  if (this->Impl->CurrentDataArrayMode == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA)
  {
    if (cellBO.Program->IsAttributeUsed("textureF"))
    {
      int tunit = this->Impl->CellFloatTexture->GetTextureUnit();
      cellBO.Program->SetUniformi("textureF", tunit);
    }
  }
}

//-----------------------------------------------------------------------------
bool vtkValuePassHelper::RequiresShaderRebuild()
{
  if (this->RenderingMode == vtkValuePass::FLOATING_POINT &&
   this->Impl->CurrentDataArrayMode != this->Impl->LastDataArrayMode)
  {
    this->Impl->LastDataArrayMode = this->Impl->CurrentDataArrayMode;
    return true;
  }

  return false;
}

// ----------------------------------------------------------------------------
void vtkValuePassHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
