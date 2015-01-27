/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericCompositePolyDataMapper2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGenericCompositePolyDataMapper2.h"

#include "vtkBoundingBox.h"
#include "vtkCommand.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositeDataDisplayAttributes.h"
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

#include <algorithm>

//===================================================================
// We define a helper class that is a subclass of vtkOpenGLPolyDataMapper
// We use this to get some performance improvements over the generic
// mapper case.
class vtkCompositeMapperHelper : public vtkOpenGLPolyDataMapper
{
public:
  static vtkCompositeMapperHelper* New();
  vtkTypeMacro(vtkCompositeMapperHelper, vtkOpenGLPolyDataMapper);

  vtkGenericCompositePolyDataMapper2 *Parent;
  int LastColorCoordinates;
  int LastNormalsOffset;
  int LastTCoordComponents;

protected:
  vtkCompositeMapperHelper() {};
  ~vtkCompositeMapperHelper() {};

  // Description:
  // Set the shader parameteres related to the property, called by UpdateShader
  virtual void SetPropertyShaderParameters(vtkgl::CellBO &cellBO, vtkRenderer *ren, vtkActor *act);

  // Description:
  // Set the shader parameteres related to lighting, called by UpdateShader
  virtual void SetLightingShaderParameters(vtkgl::CellBO &cellBO, vtkRenderer *ren, vtkActor *act);

  // Description:
  // Set the shader parameteres related to the Camera, called by UpdateShader
  virtual void SetCameraShaderParameters(vtkgl::CellBO &cellBO, vtkRenderer *ren, vtkActor *act);

  // Description:
  // Does the shader source need to be recomputed
  virtual bool GetNeedToRebuildShader(vtkgl::CellBO &cellBO, vtkRenderer *ren, vtkActor *act);

  // Description:
  // Make sure an appropriate shader is defined, compiled and bound.  This method
  // orchistrates the process, much of the work is done in other methods
  virtual void UpdateShader(vtkgl::CellBO &cellBO, vtkRenderer *ren, vtkActor *act);

private:
  vtkCompositeMapperHelper(const vtkCompositeMapperHelper&); // Not implemented.
  void operator=(const vtkCompositeMapperHelper&); // Not implemented.
};

vtkStandardNewMacro(vtkCompositeMapperHelper);

void vtkCompositeMapperHelper::SetCameraShaderParameters(vtkgl::CellBO &cellBO,
                                                       vtkRenderer *ren, vtkActor *actor)
{
  if (!this->Parent->GetShaderInitialized(cellBO.Program))
    {
    this->Superclass::SetCameraShaderParameters(cellBO, ren, actor);
    }
}

void vtkCompositeMapperHelper::SetLightingShaderParameters(vtkgl::CellBO &cellBO,
                                                       vtkRenderer *ren, vtkActor *actor)
{
  if (!this->Parent->GetShaderInitialized(cellBO.Program))
    {
    this->Superclass::SetLightingShaderParameters(cellBO, ren, actor);
    }
}

void vtkCompositeMapperHelper::SetPropertyShaderParameters(vtkgl::CellBO &cellBO,
                                                       vtkRenderer *ren, vtkActor *actor)
{
  if (!this->Parent->GetShaderInitialized(cellBO.Program))
    {
    this->Superclass::SetPropertyShaderParameters(cellBO, ren, actor);
    }

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

//-----------------------------------------------------------------------------
void vtkCompositeMapperHelper::UpdateShader(vtkgl::CellBO &cellBO, vtkRenderer* ren, vtkActor *actor)
{
  // invoke superclass
  this->Superclass::UpdateShader(cellBO, ren, actor);
  // mark this shader as initialized
  this->Parent->SetShaderInitialized(cellBO.Program, true);
}

//-----------------------------------------------------------------------------
// smarter version that knows actor/property/camera/lights are not changing
bool vtkCompositeMapperHelper::GetNeedToRebuildShader(vtkgl::CellBO &cellBO, vtkRenderer* ren, vtkActor *actor)
{
  if (!cellBO.Program ||  !this->Parent->GetShaderInitialized(cellBO.Program))
    {
    bool result = this->Superclass::GetNeedToRebuildShader(cellBO, ren, actor);
    this->LastColorCoordinates = this->Layout.ColorComponents;
    this->LastNormalsOffset = this->Layout.NormalOffset;
    this->LastTCoordComponents = this->Layout.TCoordComponents;
    return result;
    }

  // after the first datasedt we only look for changes in pointdata
  if (this->LastColorCoordinates != this->Layout.ColorComponents ||
      this->LastNormalsOffset != this->Layout.NormalOffset ||
      this->LastTCoordComponents != this->Layout.TCoordComponents)
    {
    return true;
    }

  return false;
}

//===================================================================
// Now the main class methods

vtkStandardNewMacro(vtkGenericCompositePolyDataMapper2);
//----------------------------------------------------------------------------
vtkGenericCompositePolyDataMapper2::vtkGenericCompositePolyDataMapper2()
{
  this->LastOpaqueCheckTime = 0;
}

//----------------------------------------------------------------------------
vtkGenericCompositePolyDataMapper2::~vtkGenericCompositePolyDataMapper2()
{
  std::map<const vtkDataSet*, vtkCompositeMapperHelper *>::iterator miter = this->Helpers.begin();
  for (;miter != this->Helpers.end(); miter++)
    {
    miter->second->Delete();
    }
  this->Helpers.clear();
}

bool vtkGenericCompositePolyDataMapper2::GetShaderInitialized(vtkShaderProgram *prog)
{
  typedef std::map<const vtkShaderProgram *, bool>::iterator Iter;
  Iter found = this->ShadersInitialized.find(prog);
  if (found == this->ShadersInitialized.end())
    {
    this->ShadersInitialized.insert(std::make_pair(prog, false));
    return false;
    }
  else
    {
    return found->second;
    }
}

void vtkGenericCompositePolyDataMapper2::SetShaderInitialized(vtkShaderProgram *prog, bool val)
{
  typedef std::map<const vtkShaderProgram *, bool>::iterator Iter;
  Iter found = this->ShadersInitialized.find(prog);
  if (found == this->ShadersInitialized.end())
    {
    this->ShadersInitialized.insert(std::make_pair(prog, val));
    }
  else
    {
    found->second = val;
    }
}

//----------------------------------------------------------------------------
int vtkGenericCompositePolyDataMapper2::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
}

//----------------------------------------------------------------------------
vtkExecutive* vtkGenericCompositePolyDataMapper2::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//-----------------------------------------------------------------------------
//Looks at each DataSet and finds the union of all the bounds
void vtkGenericCompositePolyDataMapper2::ComputeBounds()
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
bool vtkGenericCompositePolyDataMapper2::GetIsOpaque()
{
  vtkCompositeDataSet *input = vtkCompositeDataSet::SafeDownCast(
    this->GetInputDataObject(0, 0));
  unsigned long int lastMTime = std::max(input ? input->GetMTime() : 0, this->GetMTime());
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
void vtkGenericCompositePolyDataMapper2::SetBlockVisibility(unsigned int index, bool visible)
{
  if(this->CompositeAttributes)
    {
    this->CompositeAttributes->SetBlockVisibility(index, visible);
    this->Modified();
    }
}

//----------------------------------------------------------------------------
bool vtkGenericCompositePolyDataMapper2::GetBlockVisibility(unsigned int index) const
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
void vtkGenericCompositePolyDataMapper2::RemoveBlockVisibility(unsigned int index)
{
  if(this->CompositeAttributes)
    {
    this->CompositeAttributes->RemoveBlockVisibility(index);
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkGenericCompositePolyDataMapper2::RemoveBlockVisibilites()
{
  if(this->CompositeAttributes)
    {
    this->CompositeAttributes->RemoveBlockVisibilites();
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkGenericCompositePolyDataMapper2::SetBlockColor(unsigned int index, double color[3])
{
  if(this->CompositeAttributes)
    {
    this->CompositeAttributes->SetBlockColor(index, color);
    this->Modified();
    }
}

//----------------------------------------------------------------------------
double* vtkGenericCompositePolyDataMapper2::GetBlockColor(unsigned int index)
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
void vtkGenericCompositePolyDataMapper2::RemoveBlockColor(unsigned int index)
{
  if(this->CompositeAttributes)
    {
    this->CompositeAttributes->RemoveBlockColor(index);
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkGenericCompositePolyDataMapper2::RemoveBlockColors()
{
  if(this->CompositeAttributes)
    {
    this->CompositeAttributes->RemoveBlockColors();
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkGenericCompositePolyDataMapper2::SetBlockOpacity(unsigned int index, double opacity)
{
  if(this->CompositeAttributes)
    {
    this->CompositeAttributes->SetBlockOpacity(index, opacity);
    this->Modified();
    }
}

//----------------------------------------------------------------------------
double vtkGenericCompositePolyDataMapper2::GetBlockOpacity(unsigned int index)
{
  if(this->CompositeAttributes)
    {
    return this->CompositeAttributes->GetBlockOpacity(index);
    }
  return 1.;
}

//----------------------------------------------------------------------------
void vtkGenericCompositePolyDataMapper2::RemoveBlockOpacity(unsigned int index)
{
  if(this->CompositeAttributes)
    {
    this->CompositeAttributes->RemoveBlockOpacity(index);
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkGenericCompositePolyDataMapper2::RemoveBlockOpacities()
{
  if(this->CompositeAttributes)
    {
    this->CompositeAttributes->RemoveBlockOpacities();
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkGenericCompositePolyDataMapper2::SetCompositeDataDisplayAttributes(
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
vtkGenericCompositePolyDataMapper2::GetCompositeDataDisplayAttributes()
{
  return this->CompositeAttributes;
}

//----------------------------------------------------------------------------
void vtkGenericCompositePolyDataMapper2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkGenericCompositePolyDataMapper2::RenderBlock(vtkRenderer *renderer,
                                              vtkActor *actor,
                                              vtkDataObject *dobj,
                                              unsigned int &flat_index)
{
  vtkHardwareSelector *selector = renderer->GetSelector();
  vtkCompositeDataDisplayAttributes* cda = this->GetCompositeDataDisplayAttributes();

  vtkProperty *prop = actor->GetProperty();
  bool draw_surface_with_edges =
    (prop->GetEdgeVisibility() && prop->GetRepresentation() == VTK_SURFACE);
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
      vtkCompositeMapperHelper *helper;
      typedef std::map<const vtkDataSet *,vtkCompositeMapperHelper *>::iterator GVIter;
      GVIter found = this->Helpers.find(ds);
      if (found == this->Helpers.end())
        {
        helper = vtkCompositeMapperHelper::New();
        helper->Parent = this;
        this->CopyMapperValuesToHelper(helper);
        this->Helpers.insert(std::make_pair(ds, helper));
        helper->SetInputData(ds);
        }
      else
        {
        helper = found->second;
        }
      helper->CurrentInput = ds;
      if (ds && ds->GetPoints())
        {
        helper->RenderPieceStart(renderer,actor);
        helper->RenderPieceDraw(renderer,actor);
        if (draw_surface_with_edges)
          {
          this->BlockState.AmbientColor.push(ecolor);
          helper->RenderEdges(renderer,actor);
          this->BlockState.AmbientColor.pop();
          }
        helper->RenderPieceFinish(renderer,actor);
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

void vtkGenericCompositePolyDataMapper2::CopyMapperValuesToHelper(vtkCompositeMapperHelper *helper)
{
  helper->vtkMapper::ShallowCopy(this);
  helper->SetStatic(1);
}

void vtkGenericCompositePolyDataMapper2::FreeGenericStructures()
{
  std::map<const vtkDataSet*, vtkCompositeMapperHelper *>::iterator miter = this->Helpers.begin();
  for (;miter != this->Helpers.end(); miter++)
    {
    miter->second->Delete();
    }
  this->Helpers.clear();
  this->ShadersInitialized.clear();
}

// ---------------------------------------------------------------------------
// Description:
// Method initiates the mapping process. Generally sent by the actor
// as each frame is rendered.
void vtkGenericCompositePolyDataMapper2::RenderGeneric(vtkRenderer *ren, vtkActor *actor)
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
    std::map<const vtkDataSet*, vtkCompositeMapperHelper *>::iterator miter = this->Helpers.begin();
    for (;miter != this->Helpers.end(); miter++)
      {
      miter->second->Delete();
      }
    this->Helpers.clear();
    this->ShadersInitialized.clear();
    this->HelperMTime.Modified();
    }
  else // otherwise just reinitialize the shaders
    {
    // if we have changed recopy our mapper settings to the helpers
    if (this->GetMTime() > this->HelperMTime)
      {
      std::map<const vtkDataSet*, vtkCompositeMapperHelper *>::iterator miter = this->Helpers.begin();
      for (;miter != this->Helpers.end(); miter++)
        {
        this->CopyMapperValuesToHelper(miter->second);
        }
      }
    // reset initialized flag on the shaders we use
    std::map<const vtkShaderProgram *, bool>::iterator miter = this->ShadersInitialized.begin();
    for (;miter != this->ShadersInitialized.end(); miter++)
      {
      miter->second = false;
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
