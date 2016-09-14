#include "vtkDataArray.h"
#include "vtkInformation.h"
#include "vtkMapper.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLBufferObject.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLHelper.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkRenderer.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"
#include "vtkValuePass.h"
#include "vtkValuePassHelper.h"


vtkStandardNewMacro(vtkValuePassHelper)

// ----------------------------------------------------------------------------
vtkValuePassHelper::vtkValuePassHelper()
: ValueBuffer(NULL)
, ValuePassArray(NULL)
, CellFloatTexture(NULL)
, CellFloatBuffer(NULL)
, RenderingMode(-1)
, ResourcesAllocated(false)
, CurrentDataArrayMode(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA)
, LastDataArrayMode(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA)
{
}

//-----------------------------------------------------------------------------
vtkValuePassHelper::~vtkValuePassHelper()
{
  // Graphics resources released previously by the parent mapper or after switching
  // to INVERTIBLE_LUT mode
  if (this->ValueBuffer)
    {
    this->ValueBuffer->Delete();
    this->ValueBuffer = NULL;
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

//-----------------------------------------------------------------------------
void vtkValuePassHelper::ReleaseGraphicsResources(vtkWindow *win)
{
  if (this->CellFloatTexture)
    {
    this->CellFloatTexture->ReleaseGraphicsResources(win);
    this->CellFloatTexture->Delete();
    this->CellFloatTexture = NULL;
    }

  if (this->CellFloatBuffer)
    {
    this->CellFloatBuffer->ReleaseGraphicsResources();
    this->CellFloatBuffer->Delete();
    this->CellFloatBuffer = NULL;
    }

  if (this->ValueBuffer)
    {
    this->ValueBuffer->ReleaseGraphicsResources();
    this->ValueBuffer->Delete();
    this->ValueBuffer = NULL;
    }

  this->ValuePassArray = NULL;
  this->ResourcesAllocated = false;
}

//-----------------------------------------------------------------------------
void vtkValuePassHelper::RenderPieceFinish()
{
  if (this->CurrentDataArrayMode == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA)
    {
    if (this->CellFloatTexture)
      {
      this->CellFloatTexture->Deactivate();
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
    this->ValuePassArray = vtkAbstractMapper::GetScalars(input,
      info->Get(vp::SCALAR_MODE()), info->Get(vp::ARRAY_MODE()),
      info->Get(vp::ARRAY_ID()), info->Get(vp::ARRAY_NAME()), cellFlag);

    if (!this->ValuePassArray)
      {
      vtkErrorMacro("Invalid data array from GetScalars()!");
      return;
      }

    // Extract the current component value from the array.
    vtkIdType const numTuples = this->ValuePassArray->GetNumberOfTuples();
    int const compIndex = info->Get(vp::ARRAY_COMPONENT());
    this->Buffer.clear();
    this->Buffer.reserve(numTuples);

    for (vtkIdType id = 0; id < numTuples; id++)
      {
      double* tuple = this->ValuePassArray->GetTuple(id);
      float value = static_cast<float>(tuple[compIndex]);
      this->Buffer.push_back(value);
      }

    // Upload array data
    if (this->CurrentDataArrayMode == VTK_SCALAR_MODE_USE_POINT_FIELD_DATA)
      {
      // Point data
      this->ValueBuffer->Upload(&(this->Buffer.front()), static_cast<size_t>(numTuples),
        vtkOpenGLBufferObject::ArrayBuffer);
      }
    else
      {
      // Cell data
      this->CellFloatBuffer->Upload(&(this->Buffer.front()), numTuples,
        vtkOpenGLBufferObject::TextureBuffer);

      this->CellFloatTexture->CreateTextureBuffer(static_cast<unsigned int>(numTuples),
        1, VTK_FLOAT, this->CellFloatBuffer);
      }
    }

  // Bind textures
  if (this->CurrentDataArrayMode == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA)
    {
    this->CellFloatTexture->Activate();
    }
}

//-----------------------------------------------------------------------------
void vtkValuePassHelper::UpdateConfiguration(vtkRenderer* ren, vtkActor* act,
  vtkMapper* mapper)
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
    vtkInformation *info = act->GetPropertyKeys();
    if (info)
      {
      // Since it has RENDER_VALUES it is assumed it has all the tags from ValuePass
      this->CurrentDataArrayMode = info->Get(vtkValuePass::SCALAR_MODE());
      }

    switch (this->RenderingMode)
      {
      case vtkValuePass::FLOATING_POINT:
        this->AllocateGraphicsResources(ren);
        break;

      case vtkValuePass::INVERTIBLE_LUT:
      default:
        {
        vtkInformation* info = act->GetPropertyKeys();
        mapper->UseInvertibleColorFor(info->Get(vtkValuePass::SCALAR_MODE()),
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
  if (this->ResourcesAllocated)
    {
    return;
    }

  // For point data
  this->ValueBuffer = vtkOpenGLBufferObject::New();
  this->ValueBuffer->SetType(vtkOpenGLBufferObject::ArrayBuffer);

  // For cell data
  this->CellFloatTexture = vtkTextureObject::New();
  this->CellFloatTexture->SetContext
    (static_cast<vtkOpenGLRenderWindow*>(ren->GetVTKWindow()));

  this->CellFloatBuffer = vtkOpenGLBufferObject::New();
  this->CellFloatBuffer->SetType(vtkOpenGLBufferObject::TextureBuffer);

  this->ResourcesAllocated = true;
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

  if (this->CurrentDataArrayMode == VTK_SCALAR_MODE_USE_POINT_FIELD_DATA)
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
  if (this->CurrentDataArrayMode == VTK_SCALAR_MODE_USE_POINT_FIELD_DATA)
    {
    if (this->ValuePassArray)
      {
      if (cellBO.Program->IsAttributeUsed("dataAttribute"))
        {
        size_t const stride = sizeof(float);

        if (!cellBO.VAO->AddAttributeArray(cellBO.Program, this->ValueBuffer,
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
  if (this->CurrentDataArrayMode == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA)
    {
    if (cellBO.Program->IsAttributeUsed("textureF"))
      {
      int tunit = this->CellFloatTexture->GetTextureUnit();
      cellBO.Program->SetUniformi("textureF", tunit);
      }
    }
}

//-----------------------------------------------------------------------------
bool vtkValuePassHelper::RequiresShaderRebuild(vtkActor* act)
{
  if (this->RenderingMode == vtkValuePass::FLOATING_POINT &&
   this->CurrentDataArrayMode != this->LastDataArrayMode)
    {
    this->LastDataArrayMode = this->CurrentDataArrayMode;
    return true;
    }

  return false;
}
