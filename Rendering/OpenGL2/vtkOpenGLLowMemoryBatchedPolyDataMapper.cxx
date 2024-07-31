// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkOpenGLLowMemoryBatchedPolyDataMapper.h"
#include "vtkCellData.h"
#include "vtkColorTransferFunction.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkFloatArray.h"
#include "vtkHardwareSelector.h"
#include "vtkImageData.h"
#include "vtkLookupTable.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLCellToVTKCellMap.h"
#include "vtkOpenGLCompositePolyDataMapperDelegator.h"
#include "vtkOpenGLIndexBufferObject.h"
#include "vtkOpenGLLowMemoryCellTypeAgent.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLTexture.h"
#include "vtkOpenGLVertexBufferObject.h"
#include "vtkOpenGLVertexBufferObjectGroup.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSetGet.h"
#include "vtkShaderProgram.h"
#include "vtkTexture.h"
#include "vtkTextureObject.h"
#include "vtkTransform.h"
#include "vtkUnsignedIntArray.h"

#include <sstream>

namespace
{
template <typename T>
class ScopedValueRollback
{
public:
  ScopedValueRollback(T& value, T newValue)
  {
    Value = value;
    Pointer = &value;
    *Pointer = newValue;
  }
  ~ScopedValueRollback() { *Pointer = Value; }

private:
  T* Pointer = nullptr;
  T Value;
};

} // end anonymous namespace

VTK_ABI_NAMESPACE_BEGIN

#define SCOPED_ROLLBACK(type, varName)                                                             \
  ScopedValueRollback<type> saver_##varName(this->varName, batchElement.varName)

#define SCOPED_ROLLBACK_CUSTOM_VARIABLE(type, varName, newVarName)                                 \
  ScopedValueRollback<type> saver_##varName(this->varName, newVarName)

#define SCOPED_ROLLBACK_ARRAY_ELEMENT(type, varName, idx)                                          \
  ScopedValueRollback<type> saver_##varName##idx(this->varName[idx], batchElement.varName[idx])

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLLowMemoryBatchedPolyDataMapper);

//------------------------------------------------------------------------------
vtkOpenGLLowMemoryBatchedPolyDataMapper::vtkOpenGLLowMemoryBatchedPolyDataMapper()
{
  // force static
  this->Static = true;
}

//------------------------------------------------------------------------------
vtkOpenGLLowMemoryBatchedPolyDataMapper::~vtkOpenGLLowMemoryBatchedPolyDataMapper() = default;

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryBatchedPolyDataMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Override Color Used: " << this->OverideColorUsed << endl;
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryBatchedPolyDataMapper::AddBatchElement(
  unsigned int flatIndex, BatchElement&& element)
{
  auto address = reinterpret_cast<std::uintptr_t>(element.PolyData);
  auto found = this->VTKPolyDataToGLBatchElement.find(address);
  if (found == this->VTKPolyDataToGLBatchElement.end())
  {
    GLBatchElement glBatchElement;
    glBatchElement.CellGroupId = 0;
    glBatchElement.Parent = std::move(element);
    glBatchElement.Parent.Marked = true;
    this->VTKPolyDataToGLBatchElement[address] =
      std::unique_ptr<GLBatchElement>(new GLBatchElement(std::move(glBatchElement)));
  }
  else
  {
    auto& glBatchElement = found->second;
    glBatchElement->Parent.FlatIndex = flatIndex;
    glBatchElement->Parent.Marked = true;
  }
}

//------------------------------------------------------------------------------
vtkCompositePolyDataMapperDelegator::BatchElement*
vtkOpenGLLowMemoryBatchedPolyDataMapper::GetBatchElement(vtkPolyData* polydata)
{
  auto address = reinterpret_cast<std::uintptr_t>(polydata);
  auto found = this->VTKPolyDataToGLBatchElement.find(address);
  if (found != this->VTKPolyDataToGLBatchElement.end())
  {
    auto& glBatchElement = found->second;
    return &(glBatchElement->Parent);
  }
  return nullptr;
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryBatchedPolyDataMapper::ClearBatchElements()
{
  this->VTKPolyDataToGLBatchElement.clear();
}

//------------------------------------------------------------------------------
std::vector<vtkPolyData*> vtkOpenGLLowMemoryBatchedPolyDataMapper::GetRenderedList() const
{
  std::vector<vtkPolyData*> result;
  result.reserve(this->VTKPolyDataToGLBatchElement.size());
  for (const auto& iter : this->VTKPolyDataToGLBatchElement)
  {
    result.emplace_back(iter.second->Parent.PolyData);
  }
  return result;
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryBatchedPolyDataMapper::SetParent(vtkCompositePolyDataMapper* parent)
{
  this->Parent = parent;
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryBatchedPolyDataMapper::RenderPiece(vtkRenderer* renderer, vtkActor* actor)
{
  // Make sure that we have been properly initialized.
  if (renderer->GetRenderWindow()->CheckAbortStatus())
  {
    return;
  }

  this->CurrentSelector = renderer->GetSelector();
  if (this->CurrentSelector)
  {
    for (auto& iter : this->VTKPolyDataToGLBatchElement)
    {
      this->CurrentInput = iter.second->Parent.PolyData;
      this->UpdateMaximumPointCellIds(renderer, actor);
    }
  }
  // Cache the bounding box of all points.
  if (this->ShiftScaleMethod == ShiftScaleMethodType::AUTO_SHIFT_SCALE)
  {
    double bounds[6] = {};
    vtkMath::UninitializeBounds(bounds);
    auto firstPolyData = this->VTKPolyDataToGLBatchElement.begin()->second->Parent.PolyData;
    if (firstPolyData->GetNumberOfPoints() > 0)
    {
      firstPolyData->GetPoints()->GetBounds(bounds);
    }
    this->PointsBBox.SetBounds(bounds);
    for (auto& iter : this->VTKPolyDataToGLBatchElement)
    {
      this->CurrentInput = iter.second->Parent.PolyData;
      if (this->CurrentInput->GetNumberOfPoints() > 0)
      {
        this->CurrentInput->GetPoints()->GetBounds(bounds);
        this->PointsBBox.AddBounds(bounds);
      }
    }
  }
  if (this->VTKPolyDataToGLBatchElement.empty())
  {
    return;
  }
  this->CurrentInput = this->VTKPolyDataToGLBatchElement.begin()->second->Parent.PolyData;
  this->ComputeCameraBasedShiftScale(renderer, actor, this->CurrentInput);
  this->RenderPieceStart(renderer, actor);
  this->RenderPieceDraw(renderer, actor);
  this->RenderPieceFinish(renderer, actor);
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryBatchedPolyDataMapper::UnmarkBatchElements()
{
  for (auto& iter : this->VTKPolyDataToGLBatchElement)
  {
    auto& glBatchElement = iter.second;
    glBatchElement->Parent.Marked = false;
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryBatchedPolyDataMapper::ClearUnmarkedBatchElements()
{
  for (auto iter = this->VTKPolyDataToGLBatchElement.begin();
       iter != this->VTKPolyDataToGLBatchElement.end();)
  {
    if (!iter->second->Parent.Marked)
    {
      this->VTKPolyDataToGLBatchElement.erase(iter++);
      this->Modified();
    }
    else
    {
      ++iter;
    }
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryBatchedPolyDataMapper::DrawPrimitives(
  vtkRenderer* renderer, vtkActor* actor, PrimitiveInformation& primitive)
{
  bool selecting = this->CurrentSelector != nullptr;
  bool tpass = actor->IsRenderingTranslucentPolygonalGeometry();
  auto& agent = primitive.Agent;
  agent->PreDraw(renderer, actor, this);
  for (auto& iter : this->VTKPolyDataToGLBatchElement)
  {
    auto glBatchElement = iter.second.get();
    auto& batchElement = glBatchElement->Parent;
    bool shouldDraw = batchElement.Visibility     // must be visible
      && (!selecting || batchElement.Pickability) // and pickable when selecting
      && (((selecting || batchElement.IsOpaque || actor->GetForceOpaque()) &&
            !tpass) // opaque during opaque or when selecting
           || ((!batchElement.IsOpaque || actor->GetForceTranslucent()) && tpass &&
                !selecting)); // translucent during translucent and never selecting
    if (!shouldDraw)
    {
      continue;
    }
    this->SetShaderValues(glBatchElement);
    agent->Draw(renderer, actor, this, primitive.CellGroups, glBatchElement->CellGroupId);
  }
  agent->PostDraw(renderer, actor, this);
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryBatchedPolyDataMapper::RenderPieceDraw(
  vtkRenderer* renderer, vtkActor* actor)
{
  this->ReadyShaderProgram(renderer);
  this->SetShaderParameters(renderer, actor);
  this->OverideColorUsed = this->ShaderProgram->IsUniformUsed("overridesColor");

  bool pointPicking = false;
  if (this->CurrentSelector && this->PopulateSelectionSettings &&
    this->CurrentSelector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    pointPicking = true;
  }

  for (auto& primitive : this->Primitives)
  {
    if (!pointPicking)
    {
      this->DrawPrimitives(renderer, actor, primitive);
    }
    // vertex visibility pass
    if ((actor->GetProperty()->GetVertexVisibility() &&
          primitive.Agent->ImplementsVertexVisibilityPass()) ||
      pointPicking)
    {
      primitive.Agent->BeginVertexVisibilityPass();
      this->DrawPrimitives(renderer, actor, primitive);
      primitive.Agent->EndVertexVisibilityPass();
    }
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryBatchedPolyDataMapper::ProcessSelectorPixelBuffers(
  vtkHardwareSelector* sel, std::vector<unsigned int>& pixeloffsets, vtkProp* prop)
{
  if (!this->PopulateSelectionSettings)
  {
    return;
  }

  if (sel->GetCurrentPass() == vtkHardwareSelector::ACTOR_PASS)
  {
    this->PickPixels.clear();
    return;
  }

  if (this->PickPixels.empty() && !pixeloffsets.empty())
  {
    // preprocess the image to find matching pixels and
    // store them in a map of vectors based on flat index
    // this makes the block processing far faster as we just
    // loop over the pixels for our block
    unsigned char* compositedata =
      sel->GetRawPixelBuffer(vtkHardwareSelector::COMPOSITE_INDEX_PASS);

    if (!compositedata)
    {
      return;
    }

    size_t maxFlatIndex = 0;
    for (auto& iter : this->VTKPolyDataToGLBatchElement)
    {
      auto& glBatchElement = iter.second;
      auto& batchElement = glBatchElement->Parent;
      maxFlatIndex =
        (batchElement.FlatIndex > maxFlatIndex) ? batchElement.FlatIndex : maxFlatIndex;
    }

    this->PickPixels.resize(maxFlatIndex + 1);

    for (auto pos : pixeloffsets)
    {
      unsigned int compval = compositedata[pos + 2];
      compval = compval << 8;
      compval |= compositedata[pos + 1];
      compval = compval << 8;
      compval |= compositedata[pos];
      if (compval <= maxFlatIndex)
      {
        this->PickPixels[compval].push_back(pos);
      }
    }
  }

  // for each block update the image
  for (auto& iter : this->VTKPolyDataToGLBatchElement)
  {
    auto glBatchElement = iter.second.get();
    auto& batchElement = glBatchElement->Parent;
    if (!this->PickPixels[batchElement.FlatIndex].empty())
    {
      this->ProcessCompositePixelBuffers(
        sel, prop, glBatchElement, this->PickPixels[batchElement.FlatIndex]);
    }
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryBatchedPolyDataMapper::ProcessCompositePixelBuffers(vtkHardwareSelector* sel,
  vtkProp*, GLBatchElement* glBatchElement, std::vector<unsigned int>& mypixels)
{
  auto& batchElement = glBatchElement->Parent;
  vtkPolyData* poly = batchElement.PolyData;

  if (!poly)
  {
    return;
  }

  // which pass are we processing ?
  int currPass = sel->GetCurrentPass();

  // get some common useful values
  vtkPointData* pd = poly->GetPointData();
  vtkCellData* cd = poly->GetCellData();

  // get some values
  unsigned char* rawplowdata = sel->GetRawPixelBuffer(vtkHardwareSelector::POINT_ID_LOW24);
  unsigned char* rawphighdata = sel->GetRawPixelBuffer(vtkHardwareSelector::POINT_ID_HIGH24);

  // do we need to do anything to the process pass data?
  if (currPass == vtkHardwareSelector::PROCESS_PASS)
  {
    unsigned char* processdata = sel->GetPixelBuffer(vtkHardwareSelector::PROCESS_PASS);
    vtkUnsignedIntArray* processArray = nullptr;

    if (sel->GetUseProcessIdFromData())
    {
      processArray = !this->ProcessIdArrayName.empty()
        ? vtkArrayDownCast<vtkUnsignedIntArray>(pd->GetArray(this->ProcessIdArrayName.c_str()))
        : nullptr;
    }

    if (processArray && processdata && rawplowdata)
    {
      for (auto pos : mypixels)
      {
        unsigned int inval = 0;
        if (rawphighdata)
        {
          inval = rawphighdata[pos];
          inval = inval << 8;
        }
        inval |= rawplowdata[pos + 2];
        inval = inval << 8;
        inval |= rawplowdata[pos + 1];
        inval = inval << 8;
        inval |= rawplowdata[pos];
        // as this pass happens after both low and high point passes
        unsigned int outval = processArray->GetValue(inval) + 1;
        processdata[pos] = outval & 0xff;
        processdata[pos + 1] = (outval & 0xff00) >> 8;
        processdata[pos + 2] = (outval & 0xff0000) >> 16;
      }
    }
  }

  // do we need to do anything to the point id data?
  if (currPass == vtkHardwareSelector::POINT_ID_LOW24)
  {
    vtkIdTypeArray* pointArrayId = !this->PointIdArrayName.empty()
      ? vtkArrayDownCast<vtkIdTypeArray>(pd->GetArray(this->PointIdArrayName.c_str()))
      : nullptr;

    // do we need to do anything to the point id data?
    if (rawplowdata)
    {
      unsigned char* plowdata = sel->GetPixelBuffer(vtkHardwareSelector::POINT_ID_LOW24);
      bool hasHighPointIds = sel->HasHighPointIds();

      for (auto pos : mypixels)
      {
        unsigned int inval = 0;
        if (rawphighdata)
        {
          inval = rawphighdata[pos];
          inval = inval << 8;
        }
        inval |= rawplowdata[pos + 2];
        inval = inval << 8;
        inval |= rawplowdata[pos + 1];
        inval = inval << 8;
        inval |= rawplowdata[pos];
        // this pass happens before the high pass which means the value
        // could underflow etc when the high data is not around yet and high
        // data is needed.
        if (rawphighdata || !hasHighPointIds)
        {
          vtkIdType outval = inval;
          if (pointArrayId && static_cast<vtkIdType>(inval) <= pointArrayId->GetMaxId())
          {
            outval = pointArrayId->GetValue(inval);
          }
          plowdata[pos] = outval & 0xff;
          plowdata[pos + 1] = (outval & 0xff00) >> 8;
          plowdata[pos + 2] = (outval & 0xff0000) >> 16;
        }
      }
    }
  }

  if (currPass == vtkHardwareSelector::POINT_ID_HIGH24)
  {
    vtkIdTypeArray* pointArrayId = !this->PointIdArrayName.empty()
      ? vtkArrayDownCast<vtkIdTypeArray>(pd->GetArray(this->PointIdArrayName.c_str()))
      : nullptr;

    // do we need to do anything to the point id data?
    if (rawphighdata)
    {
      unsigned char* phighdata = sel->GetPixelBuffer(vtkHardwareSelector::POINT_ID_HIGH24);

      for (auto pos : mypixels)
      {
        unsigned int inval = 0;
        inval = rawphighdata[pos];
        inval = inval << 8;
        inval |= rawplowdata[pos + 2];
        inval = inval << 8;
        inval |= rawplowdata[pos + 1];
        inval = inval << 8;
        inval |= rawplowdata[pos];
        // always happens after the low pass so we should be safe
        vtkIdType outval = inval;
        if (pointArrayId)
        {
          outval = pointArrayId->GetValue(inval);
        }
        phighdata[pos] = (outval & 0xff000000) >> 24;
        phighdata[pos + 1] = (outval & 0xff00000000) >> 32;
        phighdata[pos + 2] = (outval & 0xff0000000000) >> 40;
      }
    }
  }

  unsigned char* rawclowdata = sel->GetRawPixelBuffer(vtkHardwareSelector::CELL_ID_LOW24);
  unsigned char* rawchighdata = sel->GetRawPixelBuffer(vtkHardwareSelector::CELL_ID_HIGH24);

  // do we need to do anything to the composite pass data?
  if (currPass == vtkHardwareSelector::COMPOSITE_INDEX_PASS)
  {
    unsigned char* compositedata = sel->GetPixelBuffer(vtkHardwareSelector::COMPOSITE_INDEX_PASS);

    vtkUnsignedIntArray* compositeArray = !this->CompositeIdArrayName.empty()
      ? vtkArrayDownCast<vtkUnsignedIntArray>(cd->GetArray(this->CompositeIdArrayName.c_str()))
      : nullptr;

    if (compositedata && compositeArray && rawclowdata)
    {
      for (auto pos : mypixels)
      {
        unsigned int inval = 0;
        if (rawchighdata)
        {
          inval = rawchighdata[pos];
          inval = inval << 8;
        }
        inval |= rawclowdata[pos + 2];
        inval = inval << 8;
        inval |= rawclowdata[pos + 1];
        inval = inval << 8;
        inval |= rawclowdata[pos];

        // always gets called after the cell high and low are available
        // so it is safe
        vtkIdType cellId = inval;
        unsigned int outval = compositeArray->GetValue(cellId);
        compositedata[pos] = outval & 0xff;
        compositedata[pos + 1] = (outval & 0xff00) >> 8;
        compositedata[pos + 2] = (outval & 0xff0000) >> 16;
      }
    }
  }

  if (currPass == vtkHardwareSelector::CELL_ID_LOW24)
  {
    vtkIdTypeArray* cellArrayId = !this->CellIdArrayName.empty()
      ? vtkArrayDownCast<vtkIdTypeArray>(cd->GetArray(this->CellIdArrayName.c_str()))
      : nullptr;
    unsigned char* clowdata = sel->GetPixelBuffer(vtkHardwareSelector::CELL_ID_LOW24);
    bool hasHighCellIds = sel->HasHighCellIds();

    if (rawclowdata)
    {
      for (auto pos : mypixels)
      {
        unsigned int inval = 0;
        if (rawchighdata)
        {
          inval = rawchighdata[pos];
          inval = inval << 8;
        }
        inval |= rawclowdata[pos + 2];
        inval = inval << 8;
        inval |= rawclowdata[pos + 1];
        inval = inval << 8;
        inval |= rawclowdata[pos];
        // this pass happens before the high pass which means the value
        // could underflow etc when the high data is not around yet and high
        // data is needed. This underflow would happen in the ConvertToOpenGLCellId
        // code when passed too low a number
        if (rawchighdata || !hasHighCellIds)
        {
          vtkIdType outval = inval;
          if (cellArrayId && outval <= cellArrayId->GetMaxId())
          {
            outval = cellArrayId->GetValue(outval);
          }
          clowdata[pos] = outval & 0xff;
          clowdata[pos + 1] = (outval & 0xff00) >> 8;
          clowdata[pos + 2] = (outval & 0xff0000) >> 16;
        }
      }
    }
  }

  if (currPass == vtkHardwareSelector::CELL_ID_HIGH24)
  {
    vtkIdTypeArray* cellArrayId = !this->CellIdArrayName.empty()
      ? vtkArrayDownCast<vtkIdTypeArray>(cd->GetArray(this->CellIdArrayName.c_str()))
      : nullptr;
    unsigned char* chighdata = sel->GetPixelBuffer(vtkHardwareSelector::CELL_ID_HIGH24);

    if (rawchighdata)
    {
      for (auto pos : mypixels)
      {
        unsigned int inval = 0;
        inval = rawchighdata[pos];
        inval = inval << 8;
        inval |= rawclowdata[pos + 2];
        inval = inval << 8;
        inval |= rawclowdata[pos + 1];
        inval = inval << 8;
        inval |= rawclowdata[pos];
        // always called after low24 so safe
        vtkIdType outval = inval;
        if (cellArrayId)
        {
          outval = cellArrayId->GetValue(outval);
        }
        chighdata[pos] = (outval & 0xff000000) >> 24;
        chighdata[pos + 1] = (outval & 0xff00000000) >> 32;
        chighdata[pos + 2] = (outval & 0xff0000000000) >> 40;
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryBatchedPolyDataMapper::SetShaderValues(GLBatchElement* glBatchElement)
{
  auto& batchElement = glBatchElement->Parent;
  if (this->CurrentSelector)
  {
    if (this->CurrentSelector->GetCurrentPass() == vtkHardwareSelector::COMPOSITE_INDEX_PASS &&
      this->ShaderProgram->IsUniformUsed("mapperIndex"))
    {
      this->CurrentSelector->RenderCompositeIndex(batchElement.FlatIndex);
      this->ShaderProgram->SetUniform3f("mapperIndex", this->CurrentSelector->GetPropColorValue());
    }
    return;
  }

  SCOPED_ROLLBACK(int, ColorMode);
  SCOPED_ROLLBACK(int, ScalarMode);
  SCOPED_ROLLBACK(int, ArrayAccessMode);
  SCOPED_ROLLBACK(int, ArrayComponent);
  SCOPED_ROLLBACK(int, ArrayId);
  SCOPED_ROLLBACK_CUSTOM_VARIABLE(char*, ArrayName,
    static_cast<char*>(batchElement.ArrayName.empty() ? nullptr : &batchElement.ArrayName.front()));
  SCOPED_ROLLBACK(vtkIdType, FieldDataTupleId);
  SCOPED_ROLLBACK(vtkTypeBool, ScalarVisibility);
  SCOPED_ROLLBACK(vtkTypeBool, UseLookupTableScalarRange);
  SCOPED_ROLLBACK(vtkTypeBool, InterpolateScalarsBeforeMapping);
  SCOPED_ROLLBACK_ARRAY_ELEMENT(double, ScalarRange, 0);
  SCOPED_ROLLBACK_ARRAY_ELEMENT(double, ScalarRange, 1);

  // If requested, color partial / missing arrays with NaN color.
  bool useNanColor = false;
  double nanColor[4] = { -1., -1., -1., -1 };
  if (this->Parent->GetColorMissingArraysWithNanColor() && this->GetScalarVisibility())
  {
    int cellFlag = 0;
    vtkAbstractArray* scalars = vtkAbstractMapper::GetAbstractScalars(batchElement.PolyData,
      this->ScalarMode, this->ArrayAccessMode, this->ArrayId, this->ArrayName, cellFlag);
    if (scalars == nullptr)
    {
      vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->GetLookupTable());
      vtkColorTransferFunction* ctf =
        lut ? nullptr : vtkColorTransferFunction::SafeDownCast(this->GetLookupTable());
      if (lut)
      {
        lut->GetNanColor(nanColor);
        useNanColor = true;
      }
      else if (ctf)
      {
        ctf->GetNanColor(nanColor);
        useNanColor = true;
      }
    }
  }

  // override the opacity and color
  this->ShaderProgram->SetUniformf("intensity_opacity_override", batchElement.Opacity);

  if (useNanColor)
  {
    float fnancolor[3] = { static_cast<float>(nanColor[0]), static_cast<float>(nanColor[1]),
      static_cast<float>(nanColor[2]) };
    this->ShaderProgram->SetUniform3f("color_ambient_override", fnancolor);
    this->ShaderProgram->SetUniform3f("color_diffuse_override", fnancolor);
    this->ShaderProgram->SetUniformi("overridesColor", true);
  }
  else
  {
    // if (this->DrawingSelection)
    // {
    //   vtkColor3d& sColor = batchElement.SelectionColor;
    //   float selectionColor[3] = { static_cast<float>(sColor[0]), static_cast<float>(sColor[1]),
    //     static_cast<float>(sColor[2]) };
    //   this->ShaderProgram->SetUniform3f("color_ambient", selectionColor);
    //   this->ShaderProgram->SetUniform3f("color_diffuse", selectionColor);
    //   this->ShaderProgram->SetUniformf("intensity_opacity_override",
    //   batchElement.SelectionOpacity);
    // }
    // else
    {
      vtkColor3d& aColor = batchElement.AmbientColor;
      float ambientColor[3] = { static_cast<float>(aColor[0]), static_cast<float>(aColor[1]),
        static_cast<float>(aColor[2]) };
      vtkColor3d& dColor = batchElement.DiffuseColor;
      float diffuseColor[3] = { static_cast<float>(dColor[0]), static_cast<float>(dColor[1]),
        static_cast<float>(dColor[2]) };
      this->ShaderProgram->SetUniform3f("color_ambient_override", ambientColor);
      this->ShaderProgram->SetUniform3f("color_diffuse_override", diffuseColor);
    }
    if (this->OverideColorUsed)
    {
      this->ShaderProgram->SetUniformi("overridesColor", batchElement.OverridesColor);
    }
  }
}

//------------------------------------------------------------------------------
bool vtkOpenGLLowMemoryBatchedPolyDataMapper::IsDataObjectUpToDate()
{
  bool uptodate = true;
  for (auto& iter : this->VTKPolyDataToGLBatchElement)
  {
    uptodate &= (this->RenderTimeStamp > iter.second->Parent.PolyData->GetMTime());
  }
  return uptodate;
}

//------------------------------------------------------------------------------
vtkDataArray* vtkOpenGLLowMemoryBatchedPolyDataMapper::GetColors(vtkPolyData* mesh)
{
  // Get rid of old texture color coordinates if any
  if (this->ColorCoordinates)
  {
    this->ColorCoordinates->UnRegister(this);
    this->ColorCoordinates = nullptr;
  }
  // Get rid of old texture color coordinates if any
  if (this->Colors)
  {
    this->Colors->UnRegister(this);
    this->Colors = nullptr;
  }
  return this->Superclass::GetColors(mesh);
}

//------------------------------------------------------------------------------
bool vtkOpenGLLowMemoryBatchedPolyDataMapper::BindArraysToTextureBuffers(vtkRenderer* renderer,
  vtkActor* actor, vtkCellGraphicsPrimitiveMap::CellTypeMapperOffsets& offsets)
{
  std::size_t cellGroupId = 0;
  for (auto& iter : this->VTKPolyDataToGLBatchElement)
  {
    auto glBatchElement = iter.second.get();
    auto& batchElement = glBatchElement->Parent;
    this->CurrentInput = batchElement.PolyData;

    SCOPED_ROLLBACK(int, ColorMode);
    SCOPED_ROLLBACK(int, ScalarMode);
    SCOPED_ROLLBACK(int, ArrayAccessMode);
    SCOPED_ROLLBACK(int, ArrayComponent);
    SCOPED_ROLLBACK(int, ArrayId);
    SCOPED_ROLLBACK_CUSTOM_VARIABLE(char*, ArrayName,
      static_cast<char*>(
        batchElement.ArrayName.empty() ? nullptr : &batchElement.ArrayName.front()));
    SCOPED_ROLLBACK(vtkIdType, FieldDataTupleId);
    SCOPED_ROLLBACK(vtkTypeBool, ScalarVisibility);
    SCOPED_ROLLBACK(vtkTypeBool, UseLookupTableScalarRange);
    SCOPED_ROLLBACK(vtkTypeBool, InterpolateScalarsBeforeMapping);
    SCOPED_ROLLBACK_ARRAY_ELEMENT(double, ScalarRange, 0);
    SCOPED_ROLLBACK_ARRAY_ELEMENT(double, ScalarRange, 1);

    if (this->Superclass::BindArraysToTextureBuffers(renderer, actor, offsets))
    {
      iter.second->CellGroupId = cellGroupId++;
    }
  }
  return true;
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryBatchedPolyDataMapper::UpdateShiftScale(
  vtkRenderer* renderer, vtkActor* actor)
{
  if (this->ShiftScaleMethod == ShiftScaleMethodType::AUTO_SHIFT_SCALE)
  {
    double bounds[6];
    this->PointsBBox.GetBounds(bounds);
    std::vector<double> shift;
    std::vector<double> scale;
    for (int i = 0; i < 3; i++)
    {
      shift.push_back(0.5 * (bounds[i * 2] + bounds[i * 2 + 1]));
      scale.push_back(
        (bounds[i * 2 + 1] - bounds[i * 2]) ? 1.0 / (bounds[i * 2 + 1] - bounds[i * 2]) : 1.0);
    }
    this->SetShiftValues(shift[0], shift[1], shift[2]);
    this->SetScaleValues(scale[0], scale[1], scale[2]);
  }
  else
  {
    // compute shift & scale on first block
    auto firstPolyData = this->VTKPolyDataToGLBatchElement.begin()->second->Parent.PolyData;
    if (firstPolyData->GetNumberOfPoints() > 0)
    {
      this->ComputeShiftScale(renderer, actor, firstPolyData->GetPoints()->GetData());
    }
  }
}

//------------------------------------------------------------------------------
bool vtkOpenGLLowMemoryBatchedPolyDataMapper::IsShaderColorSourceUpToDate(vtkActor* actor)
{
  bool uptodate = true;
  for (auto& iter : this->VTKPolyDataToGLBatchElement)
  {
    auto glBatchElement = iter.second.get();
    auto& batchElement = glBatchElement->Parent;
    this->CurrentInput = batchElement.PolyData;
    SCOPED_ROLLBACK(int, ColorMode);
    SCOPED_ROLLBACK(int, ScalarMode);
    SCOPED_ROLLBACK(int, ArrayAccessMode);
    SCOPED_ROLLBACK(int, ArrayComponent);
    SCOPED_ROLLBACK(int, ArrayId);
    SCOPED_ROLLBACK_CUSTOM_VARIABLE(char*, ArrayName,
      static_cast<char*>(
        batchElement.ArrayName.empty() ? nullptr : &batchElement.ArrayName.front()));
    SCOPED_ROLLBACK(vtkIdType, FieldDataTupleId);
    SCOPED_ROLLBACK(vtkTypeBool, ScalarVisibility);
    SCOPED_ROLLBACK(vtkTypeBool, UseLookupTableScalarRange);
    SCOPED_ROLLBACK(vtkTypeBool, InterpolateScalarsBeforeMapping);
    SCOPED_ROLLBACK_ARRAY_ELEMENT(double, ScalarRange, 0);
    SCOPED_ROLLBACK_ARRAY_ELEMENT(double, ScalarRange, 1);
    uptodate &= this->Superclass::IsShaderColorSourceUpToDate(actor);
  }
  return uptodate;
}

//------------------------------------------------------------------------------
bool vtkOpenGLLowMemoryBatchedPolyDataMapper::IsShaderNormalSourceUpToDate(vtkActor* actor)
{
  bool uptodate = true;
  for (auto& iter : this->VTKPolyDataToGLBatchElement)
  {
    this->CurrentInput = iter.second->Parent.PolyData;
    uptodate &= this->Superclass::IsShaderNormalSourceUpToDate(actor);
  }
  return uptodate;
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryBatchedPolyDataMapper::UpdateShaders(vtkRenderer* renderer, vtkActor* actor)
{
  this->Superclass::UpdateShaders(renderer, actor);
  if (this->ShaderProgram && this->Parent)
  {
    // allow the application to set what it wants on our shader program.
    this->Parent->InvokeEvent(vtkCommand::UpdateShaderEvent, this->ShaderProgram);
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryBatchedPolyDataMapper::ReplaceShaderColor(
  vtkRenderer* renderer, vtkActor* actor, std::string& vsSource, std::string& fsSource)
{
  if (!this->CurrentSelector)
  {
    vtkShaderProgram::Substitute(fsSource, "//VTK::Color::Dec",
      "uniform bool overridesColor;\n"
      "uniform float intensity_opacity_override;\n"
      "uniform vec3 color_ambient_override;\n"
      "uniform vec3 color_diffuse_override;"
      "//VTK::Color::Dec",
      false);

    vtkShaderProgram::Substitute(fsSource, "//VTK::Color::Impl",
      "//VTK::Color::Impl\n"
      "  opacity = intensity_opacity_override;\n"
      "  if (overridesColor && vertex_pass == 0) {\n"
      "    ambientColor = color_ambient_override * intensity_ambient;\n"
      "    diffuseColor = color_diffuse_override * intensity_diffuse; }\n",
      false);
  }

  this->Superclass::ReplaceShaderColor(renderer, actor, vsSource, fsSource);
}

//------------------------------------------------------------------------------
// Returns if we can use texture maps for scalar coloring. Note this doesn't say
// we "will" use scalar coloring. It says, if we do use scalar coloring, we will
// use a texture.
// When rendering multiblock datasets, if any 2 blocks provide different
// lookup tables for the scalars, then also we cannot use textures. This case can
// be handled if required.
int vtkOpenGLLowMemoryBatchedPolyDataMapper::CanUseTextureMapForColoring(vtkDataObject*)
{
  if (!this->InterpolateScalarsBeforeMapping)
  {
    return 0; // user doesn't want us to use texture maps at all.
  }

  int cellFlag = 0;
  vtkScalarsToColors* scalarsLookupTable = nullptr;
  for (auto& iter : this->VTKPolyDataToGLBatchElement)
  {
    auto polydata = iter.second->Parent.PolyData;
    vtkDataArray* scalars = vtkAbstractMapper::GetScalars(
      polydata, this->ScalarMode, this->ArrayAccessMode, this->ArrayId, this->ArrayName, cellFlag);

    if (scalars)
    {
      if (cellFlag)
      {
        return 0;
      }
      // Don't use texture if direct coloring using RGB unsigned chars was requested.
      if ((this->ColorMode == VTK_COLOR_MODE_DEFAULT &&
            vtkArrayDownCast<vtkUnsignedCharArray>(scalars)) ||
        this->ColorMode == VTK_COLOR_MODE_DIRECT_SCALARS)
      {
        // Don't use texture if direct coloring using RGB unsigned chars is
        // requested.
        return 0;
      }

      if (scalarsLookupTable && scalars->GetLookupTable() &&
        (scalarsLookupTable != scalars->GetLookupTable()))
      {
        // Two datasets are requesting different lookup tables to color with.
        // We don't handle this case right now for composite datasets.
        return 0;
      }
      if (scalars->GetLookupTable())
      {
        scalarsLookupTable = scalars->GetLookupTable();
      }
    }
  }

  if ((scalarsLookupTable && scalarsLookupTable->GetIndexedLookup()) ||
    (!scalarsLookupTable && this->LookupTable && this->LookupTable->GetIndexedLookup()))
  {
    return 0;
  }

  return 1;
}

vtkMTimeType vtkOpenGLLowMemoryBatchedPolyDataMapper::GetMTime()
{
  if (this->Parent)
  {
    return std::max(this->Superclass::GetMTime(), this->Parent->GetMTime());
  }
  else
  {
    return this->Superclass::GetMTime();
  }
}
VTK_ABI_NAMESPACE_END
