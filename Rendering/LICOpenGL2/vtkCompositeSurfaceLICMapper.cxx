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
#include "vtkSurfaceLICMapper.h"

#include "vtkBoundingBox.h"
#include "vtkCommand.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkGarbageCollector.h"
#include "vtkHardwareSelector.h"
#include "vtkInformation.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkScalarsToColors.h"
#include "vtkShaderProgram.h"
#include "vtkUnsignedCharArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkSurfaceLICInterface.h"

#include <algorithm>

//===================================================================
// We define a helper class that is a subclass of vtkOpenGLPolyDataMapper
// We use this to get some performance improvements over the generic
// mapper case.
class vtkCompositeLICHelper : public vtkSurfaceLICMapper
{
public:
  static vtkCompositeLICHelper* New();
  vtkTypeMacro(vtkCompositeLICHelper, vtkSurfaceLICMapper);

  vtkCompositeSurfaceLICMapper *Parent;

protected:
  vtkCompositeLICHelper() {};
  ~vtkCompositeLICHelper() {};

  // Description:
  // Set the shader parameteres related to the property, called by UpdateShader
  virtual void SetPropertyShaderParameters(vtkOpenGLHelper &cellBO, vtkRenderer *ren, vtkActor *act);

private:
  vtkCompositeLICHelper(const vtkCompositeLICHelper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCompositeLICHelper&) VTK_DELETE_FUNCTION;
};

vtkStandardNewMacro(vtkCompositeLICHelper);

void vtkCompositeLICHelper::SetPropertyShaderParameters(vtkOpenGLHelper &cellBO,
                                                       vtkRenderer *ren, vtkActor *actor)
{
  this->Superclass::SetPropertyShaderParameters(cellBO, ren, actor);

  vtkProperty *ppty = actor->GetProperty();

  // override the opacity
  cellBO.Program->SetUniformf("opacityUniform", this->Parent->BlockState.Opacity.top());
  double aIntensity = this->DrawingEdges ? 1.0 : ppty->GetAmbient();  // ignoring renderer ambient
  double dIntensity = this->DrawingEdges ? 0.0 : ppty->GetDiffuse();

  vtkColor3d &aColor = this->Parent->BlockState.AmbientColor.top();
  float ambientColor[3] = {static_cast<float>(aColor[0] * aIntensity),
    static_cast<float>(aColor[1] * aIntensity),
    static_cast<float>(aColor[2] * aIntensity)};
  vtkColor3d &dColor = this->Parent->BlockState.DiffuseColor.top();
  float diffuseColor[3] = {static_cast<float>(dColor[0] * dIntensity),
    static_cast<float>(dColor[1] * dIntensity),
    static_cast<float>(dColor[2] * dIntensity)};
  cellBO.Program->SetUniform3f("ambientColorUniform", ambientColor);
  cellBO.Program->SetUniform3f("diffuseColorUniform", diffuseColor);
}

//===================================================================
// Now the main class methods

vtkStandardNewMacro(vtkCompositeSurfaceLICMapper);
//----------------------------------------------------------------------------
vtkCompositeSurfaceLICMapper::vtkCompositeSurfaceLICMapper()
{
  this->LastOpaqueCheckTime = 0;
  this->LastOpaqueCheckValue = false;
  this->ColorResult[0] = 0;
  this->ColorResult[1] = 0;
  this->ColorResult[2] = 0;
}

//----------------------------------------------------------------------------
vtkCompositeSurfaceLICMapper::~vtkCompositeSurfaceLICMapper()
{
  std::map<const vtkDataSet*, vtkCompositeLICHelper *>::iterator miter =
    this->Helpers.begin();
  for (;miter != this->Helpers.end(); miter++)
  {
    if (miter->second)
    {
      vtkObjectBase *obj = miter->second;
      miter->second = 0;
      obj->UnRegister(this);
    }
  }
  this->Helpers.clear();
}

//----------------------------------------------------------------------------
int vtkCompositeSurfaceLICMapper::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
}

//----------------------------------------------------------------------------
vtkExecutive* vtkCompositeSurfaceLICMapper::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//-----------------------------------------------------------------------------
//Looks at each DataSet and finds the union of all the bounds
void vtkCompositeSurfaceLICMapper::ComputeBounds()
{
  vtkMath::UninitializeBounds(this->Bounds);
  vtkCompositeDataSet *input = vtkCompositeDataSet::SafeDownCast(
    this->GetInputDataObject(0, 0));

  // If we don't have hierarchical data, test to see if we have
  // plain old polydata. In this case, the bounds are simply
  // the bounds of the input polydata.
  if (!input)
  {
    this->Superclass::ComputeBounds();
    return;
  }

  vtkCompositeDataIterator* iter = input->NewIterator();
  vtkBoundingBox bbox;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    vtkPolyData *pd = vtkPolyData::SafeDownCast(iter->GetCurrentDataObject());
    if (pd)
    {
      double bounds[6];
      pd->GetBounds(bounds);
      bbox.AddBounds(bounds);
    }
  }
  iter->Delete();
  bbox.GetBounds(this->Bounds);
//  this->BoundsMTime.Modified();
}

//-----------------------------------------------------------------------------
bool vtkCompositeSurfaceLICMapper::GetIsOpaque()
{
  vtkCompositeDataSet *input = vtkCompositeDataSet::SafeDownCast(
    this->GetInputDataObject(0, 0));
  vtkMTimeType lastMTime = std::max(input ? input->GetMTime() : 0, this->GetMTime());
  if (lastMTime <= this->LastOpaqueCheckTime)
  {
    return this->LastOpaqueCheckValue;
  }
  this->LastOpaqueCheckTime = lastMTime;
  if (this->ScalarVisibility && input &&
      (this->ColorMode == VTK_COLOR_MODE_DEFAULT ||
       this->ColorMode == VTK_COLOR_MODE_DIRECT_SCALARS))
  {
    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(input->NewIterator());
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      vtkPolyData *pd = vtkPolyData::SafeDownCast(iter->GetCurrentDataObject());
      if (pd)
      {
        int cellFlag;
        vtkDataArray* scalars = this->GetScalars(pd,
          this->ScalarMode, this->ArrayAccessMode, this->ArrayId,
          this->ArrayName, cellFlag);
        if (scalars &&
            (scalars->IsA("vtkUnsignedCharArray")  ||
             this->ColorMode == VTK_COLOR_MODE_DIRECT_SCALARS) &&
            (scalars->GetNumberOfComponents() ==  4 /*(RGBA)*/ ||
             scalars->GetNumberOfComponents() == 2 /*(LuminanceAlpha)*/))
        {
          int opacityIndex = scalars->GetNumberOfComponents() - 1;
          unsigned char opacity = 0;
          switch (scalars->GetDataType())
          {
            vtkTemplateMacro(
              vtkScalarsToColors::ColorToUChar(
                static_cast<VTK_TT>(scalars->GetRange(opacityIndex)[0]),
                &opacity));
          }
          if (opacity < 255)
          {
            // If the opacity is 255, despite the fact that the user specified
            // RGBA, we know that the Alpha is 100% opaque. So treat as opaque.
            this->LastOpaqueCheckValue = false;
            return false;
          }
        }
      }
    }
  }
  else if(this->CompositeAttributes &&
    this->CompositeAttributes->HasBlockOpacities())
  {
    this->LastOpaqueCheckValue = false;
    return false;
  }

  this->LastOpaqueCheckValue = this->Superclass::GetIsOpaque();
  return this->LastOpaqueCheckValue;
}

//----------------------------------------------------------------------------
void vtkCompositeSurfaceLICMapper::SetBlockVisibility(unsigned int index, bool visible)
{
  if(this->CompositeAttributes)
  {
    this->CompositeAttributes->SetBlockVisibility(index, visible);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
bool vtkCompositeSurfaceLICMapper::GetBlockVisibility(unsigned int index) const
{
  if(this->CompositeAttributes)
  {
    return this->CompositeAttributes->GetBlockVisibility(index);
  }
  else
  {
    return true;
  }
}

//----------------------------------------------------------------------------
void vtkCompositeSurfaceLICMapper::RemoveBlockVisibility(unsigned int index)
{
  if(this->CompositeAttributes)
  {
    this->CompositeAttributes->RemoveBlockVisibility(index);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkCompositeSurfaceLICMapper::RemoveBlockVisibilites()
{
  if(this->CompositeAttributes)
  {
    this->CompositeAttributes->RemoveBlockVisibilites();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkCompositeSurfaceLICMapper::SetBlockColor(unsigned int index, double color[3])
{
  if(this->CompositeAttributes)
  {
    this->CompositeAttributes->SetBlockColor(index, color);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
double* vtkCompositeSurfaceLICMapper::GetBlockColor(unsigned int index)
{
  static double white[3] = {1.0,1.0,1.0};

  if(this->CompositeAttributes)
  {
    this->CompositeAttributes->GetBlockColor(index, this->ColorResult);
    return this->ColorResult;
  }
  else
  {
    return white;
  }
}

//----------------------------------------------------------------------------
void vtkCompositeSurfaceLICMapper::RemoveBlockColor(unsigned int index)
{
  if(this->CompositeAttributes)
  {
    this->CompositeAttributes->RemoveBlockColor(index);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkCompositeSurfaceLICMapper::RemoveBlockColors()
{
  if(this->CompositeAttributes)
  {
    this->CompositeAttributes->RemoveBlockColors();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkCompositeSurfaceLICMapper::SetBlockOpacity(unsigned int index, double opacity)
{
  if(this->CompositeAttributes)
  {
    this->CompositeAttributes->SetBlockOpacity(index, opacity);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
double vtkCompositeSurfaceLICMapper::GetBlockOpacity(unsigned int index)
{
  if(this->CompositeAttributes)
  {
    return this->CompositeAttributes->GetBlockOpacity(index);
  }
  return 1.;
}

//----------------------------------------------------------------------------
void vtkCompositeSurfaceLICMapper::RemoveBlockOpacity(unsigned int index)
{
  if(this->CompositeAttributes)
  {
    this->CompositeAttributes->RemoveBlockOpacity(index);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkCompositeSurfaceLICMapper::RemoveBlockOpacities()
{
  if(this->CompositeAttributes)
  {
    this->CompositeAttributes->RemoveBlockOpacities();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkCompositeSurfaceLICMapper::SetCompositeDataDisplayAttributes(
  vtkCompositeDataDisplayAttributes *attributes)
{
  if(this->CompositeAttributes != attributes)
  {
    this->CompositeAttributes = attributes;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
vtkCompositeDataDisplayAttributes*
vtkCompositeSurfaceLICMapper::GetCompositeDataDisplayAttributes()
{
  return this->CompositeAttributes;
}

//----------------------------------------------------------------------------
void vtkCompositeSurfaceLICMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkCompositeSurfaceLICMapper::RenderBlock(vtkRenderer *renderer,
                                              vtkActor *actor,
                                              vtkDataObject *dobj,
                                              unsigned int &flat_index)
{
  vtkHardwareSelector *selector = renderer->GetSelector();
  vtkCompositeDataDisplayAttributes* cda = this->GetCompositeDataDisplayAttributes();

  vtkProperty *prop = actor->GetProperty();
  vtkColor3d ecolor(prop->GetEdgeColor());

  bool overrides_visibility = (cda && cda->HasBlockVisibility(flat_index));
  if (overrides_visibility)
  {
    this->BlockState.Visibility.push(cda->GetBlockVisibility(flat_index));
  }

  bool overrides_opacity = (cda && cda->HasBlockOpacity(flat_index));
  if (overrides_opacity)
  {
    this->BlockState.Opacity.push(cda->GetBlockOpacity(flat_index));
  }

  bool overrides_color = (cda && cda->HasBlockColor(flat_index));
  if (overrides_color)
  {
    vtkColor3d color = cda->GetBlockColor(flat_index);
    this->BlockState.AmbientColor.push(color);
    this->BlockState.DiffuseColor.push(color);
    this->BlockState.SpecularColor.push(color);
  }

  unsigned int my_flat_index = flat_index;
  // Advance flat-index. After this point, flat_index no longer points to this
  // block.
  flat_index++;

  vtkMultiBlockDataSet *mbds = vtkMultiBlockDataSet::SafeDownCast(dobj);
  vtkMultiPieceDataSet *mpds = vtkMultiPieceDataSet::SafeDownCast(dobj);
  if (mbds || mpds)
  {
    unsigned int numChildren = mbds? mbds->GetNumberOfBlocks() :
      mpds->GetNumberOfPieces();
    for (unsigned int cc=0 ; cc < numChildren; cc++)
    {
      vtkDataObject* child = mbds ? mbds->GetBlock(cc) : mpds->GetPiece(cc);
      if (child == NULL)
      {
        // speeds things up when dealing with NULL blocks (which is common with
        // AMRs).
        flat_index++;
        continue;
      }
      this->RenderBlock(renderer, actor, child, flat_index);
    }
  }
  else if (dobj && this->BlockState.Visibility.top() == true && this->BlockState.Opacity.top() > 0.0)
  {
    // Implies that the block is a non-null leaf node.
    // The top of the "stacks" have the state that this block must be rendered
    // with.
    if (selector)
    {
      selector->BeginRenderProp();
      selector->RenderCompositeIndex(my_flat_index);
    }

    // do we have a entry for this dataset?
    // make sure we have an entry for this dataset
    vtkPolyData *ds = vtkPolyData::SafeDownCast(dobj);
    if (ds)
    {
      this->CurrentFlatIndex = my_flat_index;
      vtkCompositeLICHelper *helper;
      typedef std::map<const vtkDataSet *,vtkCompositeLICHelper *>::iterator GVIter;
      GVIter found = this->Helpers.find(ds);
      if (found == this->Helpers.end())
      {
        helper = vtkCompositeLICHelper::New();
        helper->Parent = this;
        this->CopyMapperValuesToHelper(helper);
        this->Helpers.insert(std::make_pair(ds, helper));
        helper->SetInputData(ds);
      }
      else
      {
        helper = found->second;
        helper->SetInputData(ds);
      }
      // the parallel LIC code must get called
      // even if the data is empty to initialize the
      // communicators. Normally we would only call on
      // cases where we have data
      // if (ds && ds->GetPoints())
      {
        helper->RenderPiece(renderer,actor);
      }
    }

    if (selector)
    {
      selector->EndRenderProp();
    }
  }

  if (overrides_color)
  {
    this->BlockState.AmbientColor.pop();
    this->BlockState.DiffuseColor.pop();
    this->BlockState.SpecularColor.pop();
  }
  if (overrides_opacity)
  {
    this->BlockState.Opacity.pop();
  }
  if (overrides_visibility)
  {
    this->BlockState.Visibility.pop();
  }
}

void vtkCompositeSurfaceLICMapper::CopyMapperValuesToHelper(vtkCompositeLICHelper *helper)
{
  helper->vtkSurfaceLICMapper::ShallowCopy(this);
  helper->SetStatic(1);
}

// ---------------------------------------------------------------------------
// Description:
// Method initiates the mapping process. Generally sent by the actor
// as each frame is rendered.
void vtkCompositeSurfaceLICMapper::Render(vtkRenderer *ren, vtkActor *actor)
{
  vtkProperty* prop = actor->GetProperty();

  // Push base-values on the state stack.
  this->BlockState.Visibility.push(true);
  this->BlockState.Opacity.push(prop->GetOpacity());
  this->BlockState.AmbientColor.push(vtkColor3d(prop->GetAmbientColor()));
  this->BlockState.DiffuseColor.push(vtkColor3d(prop->GetDiffuseColor()));
  this->BlockState.SpecularColor.push(vtkColor3d(prop->GetSpecularColor()));

  // if our input has changed then clear out our helpers
  vtkDataObject* inputDO = this->GetInputDataObject(0, 0);
  if (inputDO->GetMTime() > this->HelperMTime)
  {
    std::map<const vtkDataSet*, vtkCompositeLICHelper *>::iterator miter
      = this->Helpers.begin();
    for (;miter != this->Helpers.end(); miter++)
    {
      miter->second->Delete();
    }
    this->Helpers.clear();
    this->HelperMTime.Modified();
  }
  else // otherwise just reinitialize the shaders
  {
    // if we have changed recopy our mapper settings to the helpers
    if (this->GetMTime() > this->HelperMTime ||
        this->LICInterface->GetMTime() > this->HelperMTime)
    {
      std::map<const vtkDataSet*, vtkCompositeLICHelper *>::iterator miter
        = this->Helpers.begin();
      for (;miter != this->Helpers.end(); miter++)
      {
        this->CopyMapperValuesToHelper(miter->second);
      }
      this->HelperMTime.Modified();
    }
  }

  // render using the composite data attributes
  unsigned int flat_index = 0;
  this->RenderBlock(ren, actor, inputDO, flat_index);

  this->BlockState.Visibility.pop();
  this->BlockState.Opacity.pop();
  this->BlockState.AmbientColor.pop();
  this->BlockState.DiffuseColor.pop();
  this->BlockState.SpecularColor.pop();

  this->UpdateProgress(1.0);
}

//-----------------------------------------------------------------------------
void vtkCompositeSurfaceLICMapper::ReleaseGraphicsResources(vtkWindow* win)
{
  std::map<const vtkDataSet*, vtkCompositeLICHelper *>::iterator miter =
    this->Helpers.begin();
  for (;miter != this->Helpers.end(); miter++)
  {
    miter->second->ReleaseGraphicsResources(win);
  }
  this->Superclass::ReleaseGraphicsResources(win);
}

//-----------------------------------------------------------------------------
void vtkCompositeSurfaceLICMapper::ReportReferences(vtkGarbageCollector *collector)
{
  this->Superclass::ReportReferences(collector);

  std::map<const vtkDataSet*, vtkCompositeLICHelper *>::iterator miter =
    this->Helpers.begin();
  for (;miter != this->Helpers.end(); miter++)
  {
    vtkGarbageCollectorReport(collector, miter->second, "Helper Mapper");
  }
}
