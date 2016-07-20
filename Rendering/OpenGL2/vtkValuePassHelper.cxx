#include "vtkValuePassHelper.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkShaderProgram.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLBufferObject.h"
#include "vtkOpenGLHelper.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkValuePass.h"
#include "vtkDataArray.h"
#include "vtkInformation.h"
#include "vtkMapper.h"


vtkStandardNewMacro(vtkValuePassHelper)

// ----------------------------------------------------------------------------
vtkValuePassHelper::vtkValuePassHelper()
: ValueBuffer(NULL)
, ValuePassArray(NULL)
, RenderingMode(-1)
{
}

//-----------------------------------------------------------------------------
vtkValuePassHelper::~vtkValuePassHelper()
{
  if (this->ValueBuffer)
    this->ValueBuffer->Delete();
}

//-----------------------------------------------------------------------------
void vtkValuePassHelper::UploadValueData(vtkActor* actor, vtkDataSet* input)
{
  vtkInformation *info = actor->GetPropertyKeys();

  // TODO:  Check the Array number / name instead. info does not seem to  update
  // the timestamp.
  //if (info->GetMTime() < this->ValueBufferTime)
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
    this->ValueBuffer->Upload(&(this->Buffer.front()), numTuples,
      vtkOpenGLBufferObject::ArrayBuffer);

    //this->ValueBufferTime.Modified();
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
    switch (this->RenderingMode)
      {
      case vtkValuePass::FLOATING_POINT:
        this->AllocateBuffer(ren);
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
    this->ReleaseBuffer(ren);
    mapper->ClearInvertibleColor();
    }
}

//-----------------------------------------------------------------------------
void vtkValuePassHelper::AllocateBuffer(vtkRenderer* ren)
{
  if (this->ValueBuffer)
    return;

  this->ValueBuffer = vtkOpenGLBufferObject::New();
  this->ValueBuffer->SetType(vtkOpenGLBufferObject::ArrayBuffer);
}

//-----------------------------------------------------------------------------
void vtkValuePassHelper::ReleaseBuffer(vtkRenderer* ren)
{
  if (!this->ValueBuffer)
    return;

  vtkRenderWindow* renWin = ren->GetRenderWindow();
  vtkOpenGLRenderWindow* glWin = static_cast<vtkOpenGLRenderWindow*>(renWin);
  glWin->MakeCurrent();

  // Cleanup GL buffer
  if (this->ValueBuffer)
    {
    this->ValueBuffer->ReleaseGraphicsResources();
    this->ValueBuffer->Delete();
    this->ValueBuffer = NULL;
    vtkOpenGLCheckErrorMacro("Failed to release ValueBuffer resources.");
    }

  this->ValuePassArray = NULL;
}

//-----------------------------------------------------------------------------
void vtkValuePassHelper::UpdateShaders(std::string & VSSource, std::string & FSSource,
  std::string & required)
{
  // Pass the value pass attribute to the fragment shader.
  vtkShaderProgram::Substitute(VSSource, "//VTK::ValuePass::Dec",
    "attribute float dataAttribute;\n"
    "varying float dataValue;\n");

  vtkShaderProgram::Substitute(VSSource, "//VTK::ValuePass::Impl",
    " dataValue = dataAttribute;\n");

  // Render floating point values (variables in 'required' are a requirement in
  // other sections of the fragment shader, so they are included for it to build
  // correctly).
  vtkShaderProgram::Substitute(FSSource, "//VTK::ValuePass::Dec",
    "varying float dataValue;\n");

  required += std::string(
    "  vec4 texColor = vec4(vec3(dataValue), 1.0);\n"
    "  gl_FragData[0] = texColor;\n"
    "  // Return right away since vtkValuePass::FLOATING_POINT mode is enabled\n"
    "  return;");
  vtkShaderProgram::Substitute(FSSource, "//VTK::Color::Impl", required);
}

//-----------------------------------------------------------------------------
void vtkValuePassHelper::BindValueBuffer(vtkOpenGLHelper& cellBO)
{
  if (this->ValuePassArray)
    {
    if (cellBO.Program->IsAttributeUsed("dataAttribute"))
      {
      int const elementType = this->ValuePassArray->GetDataType();
      size_t const stride = sizeof(float);

      if (!cellBO.VAO->AddAttributeArray(cellBO.Program, this->ValueBuffer,
        "dataAttribute", 0, stride, VTK_FLOAT, 1, false))
        {
        vtkErrorMacro(<< "Error setting 'dataAttribute' in shader VAO.");
        }
      }
    }
  vtkOpenGLCheckErrorMacro("Failed in SetupValueBuffer!");
}
