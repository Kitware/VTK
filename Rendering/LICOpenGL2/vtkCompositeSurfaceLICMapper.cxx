/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeSurfaceLICMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositeSurfaceLICMapper.h"

// #include "vtkBoundingBox.h"
// #include "vtkCommand.h"
// #include "vtkCompositeDataIterator.h"
// #include "vtkCompositeDataPipeline.h"
//#include "vtkCompositeDataSet.h"
// #include "vtkCompositeDataDisplayAttributes.h"
// #include "vtkGarbageCollector.h"
//#include "vtkHardwareSelector.h"
// #include "vtkInformation.h"
// #include "vtkMath.h"
//#include "vtkObjectFactory.h"
// #include "vtkPolyData.h"
// #include "vtkProperty.h"
// #include "vtkRenderer.h"
// #include "vtkRenderWindow.h"
// #include "vtkScalarsToColors.h"
// #include "vtkShaderProgram.h"
// #include "vtkUnsignedCharArray.h"
// #include "vtkMultiBlockDataSet.h"
// #include "vtkMultiPieceDataSet.h"

#include "vtk_glew.h"

#include "vtkCellData.h"
#include "vtkColorTransferFunction.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkFloatArray.h"
#include "vtkHardwareSelector.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkLookupTable.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLIndexBufferObject.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLTexture.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkOpenGLVertexBufferObject.h"
#include "vtkOpenGLVertexBufferObjectGroup.h"
#include "vtkPainterCommunicator.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkScalarsToColors.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"

#include <algorithm>
#include <sstream>

#include "vtkSurfaceLICInterface.h"

#include "vtkCompositePolyDataMapper2Internal.h"

typedef std::map<vtkPolyData *, vtkCompositeMapperHelperData *>::iterator dataIter;

class vtkCompositeLICHelper : public vtkCompositeMapperHelper2
{
public:
  static vtkCompositeLICHelper* New();
  vtkTypeMacro(vtkCompositeLICHelper, vtkCompositeMapperHelper2);

protected:
  vtkCompositeLICHelper();
  ~vtkCompositeLICHelper();

  /**
   * Build the VBO/IBO, called by UpdateBufferObjects
   */
  virtual void AppendOneBufferObject(vtkRenderer *ren,
    vtkActor *act, vtkCompositeMapperHelperData *hdata,
    unsigned int &flat_index,
    std::vector<unsigned char> &colors,
    std::vector<float> &norms) VTK_OVERRIDE;

protected:
  /**
   * Set the shader parameteres related to the mapper/input data, called by UpdateShader
   */
  virtual void SetMapperShaderParameters(vtkOpenGLHelper &cellBO, vtkRenderer *ren, vtkActor *act) VTK_OVERRIDE;

  /**
   * Perform string replacments on the shader templates
   */
  virtual void ReplaceShaderValues(
    std::map<vtkShader::Type, vtkShader *> shaders,
    vtkRenderer *ren, vtkActor *act) VTK_OVERRIDE;

private:
  vtkCompositeLICHelper(const vtkCompositeLICHelper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCompositeLICHelper&) VTK_DELETE_FUNCTION;
};

//----------------------------------------------------------------------------
vtkObjectFactoryNewMacro(vtkCompositeLICHelper);

//----------------------------------------------------------------------------
vtkCompositeLICHelper::vtkCompositeLICHelper()
{
  this->SetInputArrayToProcess(0,0,0,
    vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS,
    vtkDataSetAttributes::VECTORS);
}

//----------------------------------------------------------------------------
vtkCompositeLICHelper::~vtkCompositeLICHelper()
{
}

void vtkCompositeLICHelper::ReplaceShaderValues(
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

void vtkCompositeLICHelper::SetMapperShaderParameters(
  vtkOpenGLHelper &cellBO,
  vtkRenderer* ren, vtkActor *actor)
{
  this->Superclass::SetMapperShaderParameters(cellBO, ren, actor);
  cellBO.Program->SetUniformi("uMaskOnSurface",
    static_cast<vtkCompositeSurfaceLICMapper*>(this->Parent)
      ->GetLICInterface()->GetMaskOnSurface());
}

//-------------------------------------------------------------------------
void vtkCompositeLICHelper::AppendOneBufferObject(
  vtkRenderer *ren,
  vtkActor *act,
  vtkCompositeMapperHelperData *hdata,
  unsigned int &voffset,
  std::vector<unsigned char> &newColors,
  std::vector<float> &newNorms
  )
{
  vtkPolyData *poly = hdata->Data;
  vtkDataArray *vectors = this->GetInputArrayToProcess(0, poly);
  if (vectors)
  {
    this->VBOs->AppendDataArray("vecsMC", vectors, VTK_FLOAT);
  }

  this->Superclass::AppendOneBufferObject(
    ren, act, hdata, voffset, newColors, newNorms);
}

// #include <algorithm>

//===================================================================
// Now the main class methods

vtkStandardNewMacro(vtkCompositeSurfaceLICMapper);
//----------------------------------------------------------------------------
vtkCompositeSurfaceLICMapper::vtkCompositeSurfaceLICMapper()
{
}

//----------------------------------------------------------------------------
vtkCompositeSurfaceLICMapper::~vtkCompositeSurfaceLICMapper()
{
}

vtkCompositeMapperHelper2 *vtkCompositeSurfaceLICMapper::CreateHelper()
{
  return vtkCompositeLICHelper::New();
}

//----------------------------------------------------------------------------
void vtkCompositeSurfaceLICMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkCompositeSurfaceLICMapper::CopyMapperValuesToHelper(vtkCompositeMapperHelper2 *helper)
{
  this->Superclass::CopyMapperValuesToHelper(helper);
  // static_cast<vtkCompositeLICHelper *>(helper)->SetLICInterface(this->LICInterface.Get());
  helper->SetInputArrayToProcess(0,
     this->GetInputArrayInformation(0));
}

// ---------------------------------------------------------------------------
// Description:
// Method initiates the mapping process. Generally sent by the actor
// as each frame is rendered.

void vtkCompositeSurfaceLICMapper::Render(
  vtkRenderer *ren, vtkActor *actor)
{
  this->LICInterface->ValidateContext(ren);

  this->LICInterface->UpdateCommunicator(ren, actor, this->GetInputDataObject(0, 0));

  vtkPainterCommunicator *comm = this->LICInterface->GetCommunicator();

  if (comm->GetIsNull())
  {
    // other rank's may have some visible data but we
    // have none and should not participate further
    return;
  }

  // do we have vectors? Need a leaf node to know
  vtkCompositeDataSet *input = vtkCompositeDataSet::SafeDownCast(
    this->GetInputDataObject(0, 0));
  bool haveVectors = true;
  if (input)
  {
    vtkSmartPointer<vtkDataObjectTreeIterator> iter =
      vtkSmartPointer<vtkDataObjectTreeIterator>::New();
    iter->SetDataSet(input);
    iter->SkipEmptyNodesOn();
    iter->VisitOnlyLeavesOn();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal();
        iter->GoToNextItem())
    {
      vtkDataObject *dso = iter->GetCurrentDataObject();
      vtkPolyData *pd = vtkPolyData::SafeDownCast(dso);
      if (pd && pd->GetPoints())
      {
        haveVectors = haveVectors && (this->GetInputArrayToProcess(0, pd) != NULL);
      }
    }
  }
  else
  {
    vtkPolyData *pd = vtkPolyData::SafeDownCast(
      this->GetInputDataObject(0, 0));
    if (pd && pd->GetPoints())
    {
      haveVectors = (this->GetInputArrayToProcess(0, pd) != NULL);
    }
  }

  this->LICInterface->SetHasVectors(haveVectors);

  if (!this->LICInterface->CanRenderSurfaceLIC(actor))
  {
    // we've determined that there's no work for us, or that the
    // requisite opengl extensions are not available. pass control on
    // to delegate renderer and return.
    this->Superclass::Render(ren, actor);
    return;
  }

  // Before start rendering LIC, capture some essential state so we can restore
  // it.
  bool blendEnabled = (glIsEnabled(GL_BLEND) == GL_TRUE);

  vtkNew<vtkOpenGLFramebufferObject> fbo;
  fbo->SetContext(vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow()));
  fbo->SaveCurrentBindingsAndBuffers();

  // allocate rendering resources, initialize or update
  // textures and shaders.
  this->LICInterface->InitializeResources();

  // draw the geometry
  this->LICInterface->PrepareForGeometry();

  this->Superclass::Render(ren, actor);

  this->LICInterface->CompletedGeometry();

  // --------------------------------------------- composite vectors for parallel LIC
  this->LICInterface->GatherVectors();

  // ------------------------------------------- LIC on screen
  this->LICInterface->ApplyLIC();

  // ------------------------------------------- combine scalar colors + LIC
  this->LICInterface->CombineColorsAndLIC();

  // ----------------------------------------------- depth test and copy to screen
  this->LICInterface->CopyToScreen();

  fbo->RestorePreviousBindingsAndBuffers();

  if (blendEnabled)
  {
    glEnable(GL_BLEND);
  }
  else
  {
    glDisable(GL_BLEND);
  }
}
