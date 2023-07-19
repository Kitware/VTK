/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLBatchedPolyDataMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLBatchedPolyDataMapper.h"
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
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLTexture.h"
#include "vtkOpenGLVertexBufferObject.h"
#include "vtkOpenGLVertexBufferObjectGroup.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
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

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLBatchedPolyDataMapper);

//------------------------------------------------------------------------------
vtkOpenGLBatchedPolyDataMapper::vtkOpenGLBatchedPolyDataMapper()
{
  // force static
  this->Static = true;
}

//------------------------------------------------------------------------------
vtkOpenGLBatchedPolyDataMapper::~vtkOpenGLBatchedPolyDataMapper() = default;

//------------------------------------------------------------------------------
void vtkOpenGLBatchedPolyDataMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Primitive ID Used: " << this->PrimIDUsed << endl;
  os << indent << "Override Color Used: " << this->OverideColorUsed << endl;
}

//------------------------------------------------------------------------------
void vtkOpenGLBatchedPolyDataMapper::AddBatchElement(unsigned int flatIndex, BatchElement&& element)
{
  auto address = reinterpret_cast<std::uintptr_t>(element.PolyData);
  auto found = this->VTKPolyDataToGLBatchElement.find(address);
  if (found == this->VTKPolyDataToGLBatchElement.end())
  {
    GLBatchElement glBatchElement;
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
vtkCompositePolyDataMapperDelegator::BatchElement* vtkOpenGLBatchedPolyDataMapper::GetBatchElement(
  vtkPolyData* polydata)
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
void vtkOpenGLBatchedPolyDataMapper::ClearBatchElements()
{
  this->VTKPolyDataToGLBatchElement.clear();
}

//------------------------------------------------------------------------------
std::vector<vtkPolyData*> vtkOpenGLBatchedPolyDataMapper::GetRenderedList() const
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
void vtkOpenGLBatchedPolyDataMapper::SetParent(vtkCompositePolyDataMapper* parent)
{
  this->Parent = parent;
}

//------------------------------------------------------------------------------
void vtkOpenGLBatchedPolyDataMapper::RenderPiece(vtkRenderer* renderer, vtkActor* actor)
{
  // Make sure that we have been properly initialized.
  if (renderer->GetRenderWindow()->CheckAbortStatus())
  {
    return;
  }

  if (renderer->GetSelector())
  {
    for (auto& iter : this->VTKPolyDataToGLBatchElement)
    {
      this->CurrentInput = iter.second->Parent.PolyData;
      this->UpdateMaximumPointCellIds(renderer, actor);
    }
  }
  this->CurrentInput = this->VTKPolyDataToGLBatchElement.begin()->second->Parent.PolyData;

  this->UpdateCameraShiftScale(renderer, actor);
  this->RenderPieceStart(renderer, actor);
  this->RenderPieceDraw(renderer, actor);
  this->RenderPieceFinish(renderer, actor);
}

//------------------------------------------------------------------------------
void vtkOpenGLBatchedPolyDataMapper::UnmarkBatchElements()
{
  for (auto& iter : this->VTKPolyDataToGLBatchElement)
  {
    auto& glBatchElement = iter.second;
    glBatchElement->Parent.Marked = false;
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLBatchedPolyDataMapper::ClearUnmarkedBatchElements()
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
void vtkOpenGLBatchedPolyDataMapper::RenderPieceDraw(vtkRenderer* renderer, vtkActor* actor)
#ifndef GL_ES_VERSION_3_0
{
  int representation = actor->GetProperty()->GetRepresentation();

  // render points for point picking in a special way
  // all cell types should be rendered as points
  this->CurrentSelector = renderer->GetSelector();
  bool pointPicking = false;
  if (this->CurrentSelector && this->PopulateSelectionSettings &&
    this->CurrentSelector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    representation = VTK_POINTS;
    pointPicking = true;
  }

  this->PrimitiveIDOffset = 0;

  // draw IBOs
  for (int i = vtkOpenGLPolyDataMapper::PrimitiveStart;
       i < (this->CurrentSelector ? vtkOpenGLPolyDataMapper::PrimitiveTriStrips + 1
                                  : vtkOpenGLPolyDataMapper::PrimitiveEnd);
       i++)
  {
    this->DrawingVertices = i > vtkOpenGLPolyDataMapper::PrimitiveTriStrips;
    this->DrawingSelection = false;
    GLenum mode = this->GetOpenGLMode(representation, i);
    this->DrawIBO(renderer, actor, i, this->Primitives[i], mode,
      pointPicking ? this->GetPointPickingPrimitiveSize(i) : 0);
  }

  if (!this->CurrentSelector)
  {
    vtkSelection* sel = this->Parent->GetSelection();

    if (sel && sel->GetNumberOfNodes() > 0)
    {
      // draw selection IBOs
      for (int i = vtkOpenGLPolyDataMapper::PrimitiveStart;
           i <= vtkOpenGLPolyDataMapper::PrimitiveTriStrips; i++)
      {
        this->DrawingSelection = true;
        GLenum mode = this->GetOpenGLMode(this->SelectionType, i);
        this->DrawIBO(renderer, actor, i, this->SelectionPrimitives[i], mode, 5);
      }
    }
  }

  if (this->CurrentSelector &&
    (this->CurrentSelector->GetCurrentPass() == vtkHardwareSelector::CELL_ID_LOW24 ||
      this->CurrentSelector->GetCurrentPass() == vtkHardwareSelector::CELL_ID_HIGH24))
  {
    this->CurrentSelector->SetPropColorValue(this->PrimitiveIDOffset);
  }
}
#else
{
  int representation = actor->GetProperty()->GetRepresentation();
  // render points for point picking in a special way
  // all cell types should be rendered as points
  this->CurrentSelector = renderer->GetSelector();
  if (this->CurrentSelector && this->PopulateSelectionSettings &&
    this->CurrentSelector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    representation = VTK_POINTS;
    this->PointPicking = true;
  }

  this->PrimitiveIDOffset = 0;

  for (int primType = vtkOpenGLPolyDataMapper::PrimitiveStart;
       primType < (this->CurrentSelector ? vtkOpenGLPolyDataMapper::PrimitiveTriStrips + 1
                                         : vtkOpenGLPolyDataMapper::PrimitiveEnd);
       primType++)
  {
    this->DrawingVertices = primType > PrimitiveTriStrips;
    this->DrawingSelection = false;
    const auto numVerts = this->PrimitiveIndexArrays[primType].size();
    if (!numVerts)
    {
      continue;
    }
    // set index count and vbos so that UpdateShaders and everyone else can function correctly.
    ScopedValueRollback<vtkOpenGLVertexBufferObjectGroup*> vbogBkp(
      this->VBOs, this->PrimitiveVBOGroup[primType].Get());
    ScopedValueRollback<std::size_t> indexCountBkp(
      this->Primitives[primType].IBO->IndexCount, numVerts);
    this->UpdateShaders(this->Primitives[primType], renderer, actor);

    bool selecting = this->CurrentSelector != nullptr;
    bool tpass = actor->IsRenderingTranslucentPolygonalGeometry();
    vtkShaderProgram* prog = this->Primitives[primType].Program;
    this->PrimIDUsed = prog->IsUniformUsed("PrimitiveIDOffset");
    this->OverideColorUsed = prog->IsUniformUsed("OverridesColor");

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

      const unsigned int first = glBatchElement->StartIndex[primType];
      const GLsizei count = glBatchElement->NextIndex[primType] - first;

      if (shouldDraw && glBatchElement->NextIndex[primType] > glBatchElement->StartIndex[primType])
      {
        if (primType <= vtkOpenGLPolyDataMapper::PrimitiveTriStrips)
        {
          this->SetShaderValues(
            prog, glBatchElement, glBatchElement->CellCellMap->GetPrimitiveOffsets()[primType]);
        }
        GLenum mode = this->GetOpenGLMode(representation, primType);
        if (mode == GL_LINES && this->HaveWideLines(renderer, actor))
        {
          glDrawArraysInstanced(
            mode, first, count, 2 * vtkMath::Ceil(actor->GetProperty()->GetLineWidth()));
        }
        else
        {
          glDrawArrays(mode, first, count);
        }
      }
    }
  }

  if (this->CurrentSelector &&
    (this->CurrentSelector->GetCurrentPass() == vtkHardwareSelector::CELL_ID_LOW24 ||
      this->CurrentSelector->GetCurrentPass() == vtkHardwareSelector::CELL_ID_HIGH24))
  {
    this->CurrentSelector->SetPropColorValue(this->PrimitiveIDOffset);
  }
}
#endif

//------------------------------------------------------------------------------
void vtkOpenGLBatchedPolyDataMapper::ProcessSelectorPixelBuffers(
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

  if (PickPixels.empty() && !pixeloffsets.empty())
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
void vtkOpenGLBatchedPolyDataMapper::ProcessCompositePixelBuffers(vtkHardwareSelector* sel,
  vtkProp* prop, GLBatchElement* glBatchElement, std::vector<unsigned int>& mypixels)
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
  bool pointPicking = sel->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS;
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
      processArray = this->ProcessIdArrayName
        ? vtkArrayDownCast<vtkUnsignedIntArray>(pd->GetArray(this->ProcessIdArrayName))
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
        // the computed value should be higher than StartVertex
        inval -= glBatchElement->StartVertex;
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
    vtkIdTypeArray* pointArrayId = this->PointIdArrayName
      ? vtkArrayDownCast<vtkIdTypeArray>(pd->GetArray(this->PointIdArrayName))
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
          inval -= glBatchElement->StartVertex;
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
    vtkIdTypeArray* pointArrayId = this->PointIdArrayName
      ? vtkArrayDownCast<vtkIdTypeArray>(pd->GetArray(this->PointIdArrayName))
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
        inval -= glBatchElement->StartVertex;
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

  // vars for cell based indexing
  vtkCellArray* prims[4];
  prims[0] = poly->GetVerts();
  prims[1] = poly->GetLines();
  prims[2] = poly->GetPolys();
  prims[3] = poly->GetStrips();

  int representation = static_cast<vtkActor*>(prop)->GetProperty()->GetRepresentation();

  unsigned char* rawclowdata = sel->GetRawPixelBuffer(vtkHardwareSelector::CELL_ID_LOW24);
  unsigned char* rawchighdata = sel->GetRawPixelBuffer(vtkHardwareSelector::CELL_ID_HIGH24);

  // do we need to do anything to the composite pass data?
  if (currPass == vtkHardwareSelector::COMPOSITE_INDEX_PASS)
  {
    unsigned char* compositedata = sel->GetPixelBuffer(vtkHardwareSelector::COMPOSITE_INDEX_PASS);

    vtkUnsignedIntArray* compositeArray = this->CompositeIdArrayName
      ? vtkArrayDownCast<vtkUnsignedIntArray>(cd->GetArray(this->CompositeIdArrayName))
      : nullptr;

    if (compositedata && compositeArray && rawclowdata)
    {
      glBatchElement->CellCellMap->Update(prims, representation, poly->GetPoints());

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
        vtkIdType vtkCellId =
          glBatchElement->CellCellMap->ConvertOpenGLCellIdToVTKCellId(pointPicking, inval);
        unsigned int outval = compositeArray->GetValue(vtkCellId);
        compositedata[pos] = outval & 0xff;
        compositedata[pos + 1] = (outval & 0xff00) >> 8;
        compositedata[pos + 2] = (outval & 0xff0000) >> 16;
      }
    }
  }

  if (currPass == vtkHardwareSelector::CELL_ID_LOW24)
  {
    vtkIdTypeArray* cellArrayId = this->CellIdArrayName
      ? vtkArrayDownCast<vtkIdTypeArray>(cd->GetArray(this->CellIdArrayName))
      : nullptr;
    unsigned char* clowdata = sel->GetPixelBuffer(vtkHardwareSelector::CELL_ID_LOW24);
    bool hasHighCellIds = sel->HasHighCellIds();

    if (rawclowdata)
    {
      glBatchElement->CellCellMap->Update(prims, representation, poly->GetPoints());

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
          vtkIdType outval =
            glBatchElement->CellCellMap->ConvertOpenGLCellIdToVTKCellId(pointPicking, inval);
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
    vtkIdTypeArray* cellArrayId = this->CellIdArrayName
      ? vtkArrayDownCast<vtkIdTypeArray>(cd->GetArray(this->CellIdArrayName))
      : nullptr;
    unsigned char* chighdata = sel->GetPixelBuffer(vtkHardwareSelector::CELL_ID_HIGH24);

    if (rawchighdata)
    {
      glBatchElement->CellCellMap->Update(prims, representation, poly->GetPoints());

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
        vtkIdType outval =
          glBatchElement->CellCellMap->ConvertOpenGLCellIdToVTKCellId(pointPicking, inval);
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
vtkUnsignedCharArray* vtkOpenGLBatchedPolyDataMapper::MapScalars(vtkDataSet* input, double alpha)
{
  int cellFlag = 0;
  bool restoreLookupTable = false;
  // Can't use ScopedValueRollback here because vtkMapper::SetLookupTable affects the refcount.
  auto oldLut = this->LookupTable;
  auto scalars = vtkAbstractMapper::GetAbstractScalars(
    input, this->ScalarMode, this->ArrayAccessMode, this->ArrayId, this->ArrayName, cellFlag);
  // Get the lookup table.
  vtkDataArray* dataArray = vtkArrayDownCast<vtkDataArray>(scalars);
  if (dataArray && dataArray->GetLookupTable())
  {
    this->SetLookupTable(dataArray->GetLookupTable());
    restoreLookupTable = true;
  }
  // let superclass use the new lookup table specified on the array.
  auto result = this->Superclass::MapScalars(input, alpha, cellFlag);
  // restore original lookup table.
  if (restoreLookupTable)
  {
    this->SetLookupTable(oldLut);
  }
  return result;
}

//------------------------------------------------------------------------------
void vtkOpenGLBatchedPolyDataMapper::UpdateCameraShiftScale(vtkRenderer* renderer, vtkActor* actor)
{
  if (this->PauseShiftScale)
  {
    return;
  }

  // handle camera shift scale
  if (this->ShiftScaleMethod == ShiftScaleMethodType::NEAR_PLANE_SHIFT_SCALE ||
    this->ShiftScaleMethod == ShiftScaleMethodType::FOCAL_POINT_SHIFT_SCALE)
  {
    // get ideal shift scale from camera
    auto posVBO = this->VBOs->GetVBO("vertexMC");
    if (posVBO)
    {
      posVBO->SetCamera(renderer->GetActiveCamera());
      posVBO->SetProp3D(actor);
      posVBO->UpdateShiftScale(this->CurrentInput->GetPoints()->GetData());
      // force a rebuild if needed
      if (posVBO->GetMTime() > posVBO->GetUploadTime())
      {
        this->Modified();
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLBatchedPolyDataMapper::DrawIBO(vtkRenderer* renderer, vtkActor* actor, int primType,
  vtkOpenGLHelper& CellBO, GLenum mode, int pointSize)
{
  if (CellBO.IBO->IndexCount)
  {
    vtkOpenGLRenderWindow* renWin =
      static_cast<vtkOpenGLRenderWindow*>(renderer->GetRenderWindow());
    vtkOpenGLState* ostate = renWin->GetState();

    if (pointSize > 0)
    {
      ostate->vtkglPointSize(pointSize); // need to use shader value
    }
    // First we do the triangles, update the shader, set uniforms, etc.
    this->UpdateShaders(CellBO, renderer, actor);
    vtkShaderProgram* prog = CellBO.Program;
    if (!prog)
    {
      return;
    }
    this->PrimIDUsed = prog->IsUniformUsed("PrimitiveIDOffset");
    this->OverideColorUsed = prog->IsUniformUsed("OverridesColor");
    CellBO.IBO->Bind();

    if (!this->HaveWideLines(renderer, actor) && mode == GL_LINES)
    {
      ostate->vtkglLineWidth(actor->GetProperty()->GetLineWidth());
    }

    // if (this->DrawingEdgesOrVetices && !this->DrawingTubes(CellBO, actor))
    // {
    //   vtkProperty *ppty = actor->GetProperty();
    //   float diffuseColor[3] = {0.0, 0.0, 0.0};
    //   float ambientColor[3];
    //   double *acol = ppty->GetEdgeColor();
    //   ambientColor[0] = acol[0];
    //   ambientColor[1] = acol[1];
    //   ambientColor[2] = acol[2];
    //   prog->SetUniform3f("diffuseColorUniform", diffuseColor);
    //   prog->SetUniform3f("ambientColorUniform", ambientColor);
    // }

    bool selecting = this->CurrentSelector != nullptr;
    bool tpass = actor->IsRenderingTranslucentPolygonalGeometry();

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
      if (shouldDraw && glBatchElement->NextIndex[primType] > glBatchElement->StartIndex[primType])
      {
        // compilers think this can exceed the bounds so we also
        // test against primType even though we should not need to
        if (primType <= vtkOpenGLPolyDataMapper::PrimitiveTriStrips)
        {
          this->SetShaderValues(
            prog, glBatchElement, glBatchElement->CellCellMap->GetPrimitiveOffsets()[primType]);
        }

        unsigned int count = this->DrawingSelection
          ? static_cast<unsigned int>(CellBO.IBO->IndexCount)
          : glBatchElement->NextIndex[primType] - glBatchElement->StartIndex[primType];

        glDrawRangeElements(mode, static_cast<GLuint>(glBatchElement->StartVertex),
          static_cast<GLuint>(glBatchElement->NextVertex > 0 ? glBatchElement->NextVertex - 1 : 0),
          count, GL_UNSIGNED_INT,
          reinterpret_cast<const GLvoid*>(glBatchElement->StartIndex[primType] * sizeof(GLuint)));
      }
    }
    CellBO.IBO->Release();
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLBatchedPolyDataMapper::SetShaderValues(
  vtkShaderProgram* prog, GLBatchElement* glBatchElement, size_t primOffset)
{
  if (this->PrimIDUsed)
  {
    prog->SetUniformi("PrimitiveIDOffset", static_cast<int>(primOffset));
  }

  auto& batchElement = glBatchElement->Parent;
  if (this->CurrentSelector)
  {
    if (this->CurrentSelector->GetCurrentPass() == vtkHardwareSelector::COMPOSITE_INDEX_PASS &&
      prog->IsUniformUsed("mapperIndex"))
    {
      this->CurrentSelector->RenderCompositeIndex(batchElement.FlatIndex);
      prog->SetUniform3f("mapperIndex", this->CurrentSelector->GetPropColorValue());
    }
    return;
  }

  ScopedValueRollback<int> scalarModeSaver(this->ScalarMode, batchElement.ScalarMode);
  ScopedValueRollback<int> accessModeSaver(this->ArrayAccessMode, batchElement.ArrayAccessMode);
  ScopedValueRollback<int> arrayComponentSaver(this->ArrayComponent, batchElement.ArrayComponent);
  ScopedValueRollback<int> arrayIdSaver(this->ArrayId, batchElement.ArrayId);
  char* newArrayName = batchElement.ArrayName.empty() ? nullptr : &batchElement.ArrayName.front();
  ScopedValueRollback<char*> arrayNameSaver(this->ArrayName, newArrayName);
  ScopedValueRollback<vtkIdType> fieldDataTupleIdSaver(
    this->FieldDataTupleId, batchElement.FieldDataTupleId);
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
  prog->SetUniformf("opacityUniform", batchElement.Opacity);

  if (useNanColor)
  {
    float fnancolor[3] = { static_cast<float>(nanColor[0]), static_cast<float>(nanColor[1]),
      static_cast<float>(nanColor[2]) };
    prog->SetUniform3f("ambientColorUniform", fnancolor);
    prog->SetUniform3f("diffuseColorUniform", fnancolor);
  }
  else
  {
    if (this->DrawingSelection)
    {
      vtkColor3d& sColor = batchElement.SelectionColor;
      float selectionColor[3] = { static_cast<float>(sColor[0]), static_cast<float>(sColor[1]),
        static_cast<float>(sColor[2]) };
      prog->SetUniform3f("ambientColorUniform", selectionColor);
      prog->SetUniform3f("diffuseColorUniform", selectionColor);
      prog->SetUniformf("opacityUniform", batchElement.SelectionOpacity);
    }
    else
    {
      vtkColor3d& aColor = batchElement.AmbientColor;
      float ambientColor[3] = { static_cast<float>(aColor[0]), static_cast<float>(aColor[1]),
        static_cast<float>(aColor[2]) };
      vtkColor3d& dColor = batchElement.DiffuseColor;
      float diffuseColor[3] = { static_cast<float>(dColor[0]), static_cast<float>(dColor[1]),
        static_cast<float>(dColor[2]) };
      prog->SetUniform3f("ambientColorUniform", ambientColor);
      prog->SetUniform3f("diffuseColorUniform", diffuseColor);
    }
    if (this->OverideColorUsed)
    {
      prog->SetUniformi("OverridesColor", batchElement.OverridesColor);
    }
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLBatchedPolyDataMapper::UpdateShaders(
  vtkOpenGLHelper& cellBO, vtkRenderer* renderer, vtkActor* actor)
{
  this->Superclass::UpdateShaders(cellBO, renderer, actor);
  if (cellBO.Program && this->Parent)
  {
    // allow the program to set what it wants
    this->Parent->InvokeEvent(vtkCommand::UpdateShaderEvent, cellBO.Program);
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLBatchedPolyDataMapper::ReplaceShaderColor(
  std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* renderer, vtkActor* actor)
{
  if (!this->CurrentSelector)
  {
    std::string FSSource = shaders[vtkShader::Fragment]->GetSource();

    vtkShaderProgram::Substitute(FSSource, "//VTK::Color::Dec",
      "uniform bool OverridesColor;\n"
      "//VTK::Color::Dec",
      false);

    vtkShaderProgram::Substitute(FSSource, "//VTK::Color::Impl",
      "//VTK::Color::Impl\n"
      "  if (OverridesColor) {\n"
      "    ambientColor = ambientColorUniform * ambientIntensity;\n"
      "    diffuseColor = diffuseColorUniform * diffuseIntensity; }\n",
      false);

    shaders[vtkShader::Fragment]->SetSource(FSSource);
  }

  this->Superclass::ReplaceShaderColor(shaders, renderer, actor);
}

//------------------------------------------------------------------------------
bool vtkOpenGLBatchedPolyDataMapper::GetNeedToRebuildBufferObjects(vtkRenderer*, vtkActor* actor)
{
  // Same as vtkOpenGLPolyDataMapper::GetNeedToRebuildBufferObjects(), but
  // we need to check all inputs, not just this->CurrentInput
  this->TempState.Clear();
  this->TempState.Append(actor->GetProperty()->GetMTime(), "actor mtime");
  for (const auto& iter : this->VTKPolyDataToGLBatchElement)
  {
    auto polydata = iter.second->Parent.PolyData;
    this->TempState.Append(polydata ? polydata->GetMTime() : 0, "input mtime");
  }
  this->TempState.Append(
    actor->GetTexture() ? actor->GetTexture()->GetMTime() : 0, "texture mtime");

  if (this->VBOBuildState != this->TempState || this->VBOBuildTime < this->GetMTime())
  {
    this->VBOBuildState = this->TempState;
    return true;
  }

  return false;
}

//------------------------------------------------------------------------------
void vtkOpenGLBatchedPolyDataMapper::BuildBufferObjects(vtkRenderer* renderer, vtkActor* actor)
#ifndef GL_ES_VERSION_3_0
{
  // render using the composite data attributes

  // create the cell scalar array adjusted for ogl Cells
  std::vector<unsigned char> newColors;
  std::vector<float> newNorms;

  this->VBOs->ClearAllVBOs();

  if (this->VTKPolyDataToGLBatchElement.empty())
  {
    this->VBOBuildTime.Modified();
    return;
  }

  this->EdgeValues.clear();

  vtkBoundingBox bbox;
  double bounds[6];
  this->VTKPolyDataToGLBatchElement.begin()->second->Parent.PolyData->GetPoints()->GetBounds(
    bounds);
  bbox.SetBounds(bounds);
  {
    GLBatchElement* prevGLBatchElement = nullptr;
    for (auto& iter : this->VTKPolyDataToGLBatchElement)
    {
      auto glBatchElement = iter.second.get();
      auto& batchElement = glBatchElement->Parent;

      batchElement.PolyData->GetPoints()->GetBounds(bounds);
      bbox.AddBounds(bounds);

      for (int i = 0; i < vtkOpenGLPolyDataMapper::PrimitiveEnd; i++)
      {
        glBatchElement->StartIndex[i] = static_cast<unsigned int>(this->IndexArray[i].size());
      }

      ScopedValueRollback<int> scalarModeSaver(this->ScalarMode, batchElement.ScalarMode);
      ScopedValueRollback<int> accessModeSaver(this->ArrayAccessMode, batchElement.ArrayAccessMode);
      ScopedValueRollback<int> arrayComponentSaver(
        this->ArrayComponent, batchElement.ArrayComponent);
      ScopedValueRollback<int> arrayIdSaver(this->ArrayId, batchElement.ArrayId);
      char* newArrayName =
        batchElement.ArrayName.empty() ? nullptr : &batchElement.ArrayName.front();
      ScopedValueRollback<char*> arrayNameSaver(this->ArrayName, newArrayName);
      ScopedValueRollback<vtkIdType> fieldDataTupleIdSaver(
        this->FieldDataTupleId, batchElement.FieldDataTupleId);
      vtkIdType vertexOffset = 0;
      // vert cell offset starts at the end of the last block
      glBatchElement->CellCellMap->SetStartOffset(
        prevGLBatchElement ? prevGLBatchElement->CellCellMap->GetFinalOffset() : 0);
      this->AppendOneBufferObject(
        renderer, actor, glBatchElement, vertexOffset, newColors, newNorms);
      glBatchElement->StartVertex = static_cast<unsigned int>(vertexOffset);
      glBatchElement->NextVertex =
        glBatchElement->StartVertex + batchElement.PolyData->GetPoints()->GetNumberOfPoints();
      for (int i = 0; i < vtkOpenGLPolyDataMapper::PrimitiveEnd; i++)
      {
        glBatchElement->NextIndex[i] = static_cast<unsigned int>(this->IndexArray[i].size());
      }
      prevGLBatchElement = glBatchElement;
    }
  }

  // clear color cache
  for (auto& iter : this->ColorArrayMap)
  {
    iter.second->Delete();
  }
  this->ColorArrayMap.clear();

  vtkOpenGLVertexBufferObject* posVBO = this->VBOs->GetVBO("vertexMC");
  if (posVBO)
  {
    if (this->ShiftScaleMethod == ShiftScaleMethodType::AUTO_SHIFT_SCALE)
    {
      posVBO->SetCoordShiftAndScaleMethod(ShiftScaleMethodType::MANUAL_SHIFT_SCALE);
      bbox.GetBounds(bounds);
      std::vector<double> shift;
      std::vector<double> scale;
      for (int i = 0; i < 3; i++)
      {
        shift.push_back(0.5 * (bounds[i * 2] + bounds[i * 2 + 1]));
        scale.push_back(
          (bounds[i * 2 + 1] - bounds[i * 2]) ? 1.0 / (bounds[i * 2 + 1] - bounds[i * 2]) : 1.0);
      }
      posVBO->SetShift(shift);
      posVBO->SetScale(scale);
    }
    else
    {
      posVBO->SetCoordShiftAndScaleMethod(
        static_cast<ShiftScaleMethodType>(this->ShiftScaleMethod));
      posVBO->SetProp3D(actor);
      posVBO->SetCamera(renderer->GetActiveCamera());
    }
  }

  this->VBOs->BuildAllVBOs(renderer);

  // refetch as it may have been deleted
  posVBO = this->VBOs->GetVBO("vertexMC");
  if (posVBO)
  {
    // If the VBO coordinates were shifted and scaled, prepare the inverse transform
    // for application to the model->view matrix:
    if (posVBO->GetCoordShiftAndScaleEnabled())
    {
      std::vector<double> const& shift = posVBO->GetShift();
      std::vector<double> const& scale = posVBO->GetScale();
      this->VBOInverseTransform->Identity();
      this->VBOInverseTransform->Translate(shift[0], shift[1], shift[2]);
      this->VBOInverseTransform->Scale(1.0 / scale[0], 1.0 / scale[1], 1.0 / scale[2]);
      this->VBOInverseTransform->GetTranspose(this->VBOShiftScale);
    }
  }

  for (int i = vtkOpenGLPolyDataMapper::PrimitiveStart; i < vtkOpenGLPolyDataMapper::PrimitiveEnd;
       i++)
  {
    this->Primitives[i].IBO->IndexCount = this->IndexArray[i].size();
    if (this->Primitives[i].IBO->IndexCount)
    {
      this->Primitives[i].IBO->Upload(
        this->IndexArray[i], vtkOpenGLBufferObject::ElementArrayBuffer);
      this->IndexArray[i].resize(0);
    }
  }

  if (!this->EdgeValues.empty())
  {
    if (!this->EdgeTexture)
    {
      this->EdgeTexture = vtkTextureObject::New();
      this->EdgeBuffer = vtkOpenGLBufferObject::New();
      this->EdgeBuffer->SetType(vtkOpenGLBufferObject::TextureBuffer);
    }
    this->EdgeTexture->SetContext(static_cast<vtkOpenGLRenderWindow*>(renderer->GetVTKWindow()));
    this->EdgeBuffer->Upload(this->EdgeValues, vtkOpenGLBufferObject::TextureBuffer);
    this->EdgeTexture->CreateTextureBuffer(
      static_cast<unsigned int>(this->EdgeValues.size()), 1, VTK_UNSIGNED_CHAR, this->EdgeBuffer);
  }

  // allocate as needed
  if (this->HaveCellScalars)
  {
    if (!this->CellScalarTexture)
    {
      this->CellScalarTexture = vtkTextureObject::New();
      this->CellScalarBuffer = vtkOpenGLBufferObject::New();
    }
    this->CellScalarTexture->SetContext(
      static_cast<vtkOpenGLRenderWindow*>(renderer->GetVTKWindow()));
    this->CellScalarBuffer->Upload(newColors, vtkOpenGLBufferObject::TextureBuffer);
    this->CellScalarTexture->CreateTextureBuffer(static_cast<unsigned int>(newColors.size() / 4), 4,
      VTK_UNSIGNED_CHAR, this->CellScalarBuffer);
  }

  if (this->HaveCellNormals)
  {
    if (!this->CellNormalTexture)
    {
      this->CellNormalTexture = vtkTextureObject::New();
      this->CellNormalBuffer = vtkOpenGLBufferObject::New();
      this->CellNormalBuffer->SetType(vtkOpenGLBufferObject::TextureBuffer);
    }
    this->CellNormalTexture->SetContext(
      static_cast<vtkOpenGLRenderWindow*>(renderer->GetVTKWindow()));

    // do we have float texture support ?
    int ftex = static_cast<vtkOpenGLRenderWindow*>(renderer->GetRenderWindow())
                 ->GetDefaultTextureInternalFormat(VTK_FLOAT, 4, false, true, false);

    if (ftex)
    {
      this->CellNormalBuffer->Upload(newNorms, vtkOpenGLBufferObject::TextureBuffer);
      this->CellNormalTexture->CreateTextureBuffer(
        static_cast<unsigned int>(newNorms.size() / 4), 4, VTK_FLOAT, this->CellNormalBuffer);
    }
    else
    {
      // have to convert to unsigned char if no float support
      std::vector<unsigned char> ucNewNorms;
      ucNewNorms.resize(newNorms.size());
      for (size_t i = 0; i < newNorms.size(); i++)
      {
        ucNewNorms[i] = 127.0 * (newNorms[i] + 1.0);
      }
      this->CellNormalBuffer->Upload(ucNewNorms, vtkOpenGLBufferObject::TextureBuffer);
      this->CellNormalTexture->CreateTextureBuffer(static_cast<unsigned int>(newNorms.size() / 4),
        4, VTK_UNSIGNED_CHAR, this->CellNormalBuffer);
    }
  }

  this->VBOBuildTime.Modified();
}
#else
{
  // render using the composite data attributes
  // this class keeps a member `IndexArrays` but that's not accessible from
  // `vtkOpenGLES30PolyDataMapper`. work with `vtkOpenGLES30PolyDataMapper::PrimitiveIndexArrays`
  for (int i = 0; i < PrimitiveEnd; ++i)
  {
    this->PrimitiveVBOGroup[i]->ClearAllVBOs();
    this->PrimitiveIndexArrays[i].clear();
  }
  if (this->VTKPolyDataToGLBatchElement.empty())
  {
    this->VBOBuildTime.Modified();
    return;
  }
  this->EdgeValues.clear();

  vtkBoundingBox bbox;
  double bounds[6] = {};
  this->VTKPolyDataToGLBatchElement.begin()->second->Parent.PolyData->GetPoints()->GetBounds(
    bounds);
  bbox.SetBounds(bounds);
  vtkIdType vOffset = 0;
  // these are normals and colors of all polydata
  std::vector<unsigned char> newColors;
  std::vector<float> newNormals;
  {
    GLBatchElement* prevGLBatchElement = nullptr;
    for (auto& iter : this->VTKPolyDataToGLBatchElement)
    {
      auto glBatchElement = iter.second.get();
      auto& batchElement = glBatchElement->Parent;

      vtkPolyData* geometry = batchElement.PolyData;

      geometry->GetPoints()->GetBounds(bounds);
      bbox.AddBounds(bounds);
      for (int i = 0; i < PrimitiveEnd; ++i)
      {
        glBatchElement->StartIndex[i] = this->PrimitiveIndexArrays[i].size();
      }

      ScopedValueRollback<int> scalarModeSaver(this->ScalarMode, batchElement.ScalarMode);
      ScopedValueRollback<int> accessModeSaver(this->ArrayAccessMode, batchElement.ArrayAccessMode);
      ScopedValueRollback<int> arrayComponentSaver(
        this->ArrayComponent, batchElement.ArrayComponent);
      ScopedValueRollback<int> arrayIdSaver(this->ArrayId, batchElement.ArrayId);
      char* newArrayName =
        batchElement.ArrayName.empty() ? nullptr : &batchElement.ArrayName.front();
      ScopedValueRollback<char*> arrayNameSaver(this->ArrayName, newArrayName);
      ScopedValueRollback<vtkIdType> fieldDataTupleIdSaver(
        this->FieldDataTupleId, batchElement.FieldDataTupleId);
      glBatchElement->StartVertex = 0;
      glBatchElement->CellCellMap->SetStartOffset(
        prevGLBatchElement ? prevGLBatchElement->CellCellMap->GetFinalOffset() : 0);
      this->AppendOneBufferObject(renderer, actor, glBatchElement, vOffset, newColors, newNormals);
      for (int i = 0; i < PrimitiveEnd; i++)
      {
        glBatchElement->NextIndex[i] = this->PrimitiveIndexArrays[i].size();
      }
      prevGLBatchElement = glBatchElement;
    }
    prevGLBatchElement = nullptr;
  }

  bool draw_surface_with_edges = (actor->GetProperty()->GetEdgeVisibility() &&
    actor->GetProperty()->GetRepresentation() == VTK_SURFACE);

  for (int primType = 0; primType < PrimitiveEnd; ++primType)
  {
    auto& vbos = this->PrimitiveVBOGroup[primType];
    if (draw_surface_with_edges && (primType == PrimitiveTris))
    {
      vtkNew<vtkFloatArray> edgeValuesArray;
      edgeValuesArray->SetNumberOfComponents(1);
      for (const auto& val : this->EdgeValues)
      {
        edgeValuesArray->InsertNextValue(val);
        edgeValuesArray->InsertNextValue(val);
        edgeValuesArray->InsertNextValue(val);
      }
      vbos->CacheDataArray("edgeValue", edgeValuesArray, renderer, VTK_FLOAT);
    }

    // upload vtk vertex IDs that span 0 .. polydata->GetNumberOfPoints()
    const auto& indices = this->PrimitiveIndexArrays[primType];
    vtkNew<vtkFloatArray> vertexIDs;
    vertexIDs->SetNumberOfComponents(1);
    vertexIDs->SetNumberOfValues(this->PrimitiveIndexArrays[primType].size());
    std::copy(indices.begin(), indices.end(), vertexIDs->Begin());
    vbos->CacheDataArray("vtkVertexID", vertexIDs, renderer, VTK_FLOAT);

    for (auto name : { "vertexMC", "prevVertexMC", "nextVertexMC" })
    {
      vtkOpenGLVertexBufferObject* posVBO = vbos->GetVBO(name);
      if (posVBO)
      {
        if (this->ShiftScaleMethod == ShiftScaleMethodType::AUTO_SHIFT_SCALE)
        {
          posVBO->SetCoordShiftAndScaleMethod(ShiftScaleMethodType::MANUAL_SHIFT_SCALE);
          bbox.GetBounds(bounds);
          std::vector<double> shift;
          std::vector<double> scale;
          for (int i = 0; i < 3; i++)
          {
            shift.push_back(0.5 * (bounds[i * 2] + bounds[i * 2 + 1]));
            scale.push_back((bounds[i * 2 + 1] - bounds[i * 2])
                ? 1.0 / (bounds[i * 2 + 1] - bounds[i * 2])
                : 1.0);
          }
          posVBO->SetShift(shift);
          posVBO->SetScale(scale);
        }
        else
        {
          posVBO->SetCoordShiftAndScaleMethod(
            static_cast<vtkOpenGLVertexBufferObject::ShiftScaleMethod>(this->ShiftScaleMethod));
          posVBO->SetProp3D(actor);
          posVBO->SetCamera(renderer->GetActiveCamera());
        }
      }
    }

    vbos->BuildAllVBOs(renderer);

    auto posVBO = vbos->GetVBO("vertexMC");
    if (posVBO)
    {
      if (posVBO->GetCoordShiftAndScaleEnabled())
      {
        std::vector<double> const& shift = posVBO->GetShift();
        std::vector<double> const& scale = posVBO->GetScale();
        this->VBOInverseTransform->Identity();
        this->VBOInverseTransform->Translate(shift[0], shift[1], shift[2]);
        this->VBOInverseTransform->Scale(1.0 / scale[0], 1.0 / scale[1], 1.0 / scale[2]);
        this->VBOInverseTransform->GetTranspose(this->VBOShiftScale);
      }
    }
  }
  this->VBOBuildTime.Modified();
}
#endif

//------------------------------------------------------------------------------
void vtkOpenGLBatchedPolyDataMapper::AppendOneBufferObject(vtkRenderer* renderer, vtkActor* actor,
  GLBatchElement* glBatchElement, vtkIdType& vertexOffset, std::vector<unsigned char>& newColors,
  std::vector<float>& newNorms)
#ifndef GL_ES_VERSION_3_0
{
  auto& batchElement = glBatchElement->Parent;
  vtkPolyData* poly = batchElement.PolyData;

  // if there are no points then skip this piece
  if (!poly->GetPoints() || poly->GetPoints()->GetNumberOfPoints() == 0)
  {
    return;
  }

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

  // For vertex coloring, this sets this->Colors as side effect.
  // For texture map coloring, this sets ColorCoordinates
  // and ColorTextureMap as a side effect.
  // I moved this out of the conditional because it is fast.
  // Color arrays are cached. If nothing has changed,
  // then the scalars do not have to be regenerted.
  this->MapScalars(poly, 1.0);

  // If we are coloring by texture, then load the texture map.
  if (this->ColorTextureMap)
  {
    if (this->InternalColorTexture == nullptr)
    {
      this->InternalColorTexture = vtkOpenGLTexture::New();
      this->InternalColorTexture->RepeatOff();
    }
    this->InternalColorTexture->SetInputData(this->ColorTextureMap);
  }

  this->HaveCellScalars = false;
  vtkDataArray* c = this->Colors;
  if (this->ScalarVisibility)
  {
    // We must figure out how the scalars should be mapped to the polydata.
    if ((this->ScalarMode == VTK_SCALAR_MODE_USE_CELL_DATA ||
          this->ScalarMode == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA ||
          this->ScalarMode == VTK_SCALAR_MODE_USE_FIELD_DATA ||
          !poly->GetPointData()->GetScalars()) &&
      this->ScalarMode != VTK_SCALAR_MODE_USE_POINT_FIELD_DATA && this->Colors)
    {
      this->HaveCellScalars = true;
      c = nullptr;
    }
  }

  this->HaveCellNormals = false;
  // Do we have cell normals?
  vtkDataArray* n = (actor->GetProperty()->GetInterpolation() != VTK_FLAT)
    ? poly->GetPointData()->GetNormals()
    : nullptr;
  if (n == nullptr && poly->GetCellData()->GetNormals())
  {
    this->HaveCellNormals = true;
    n = nullptr;
  }

  int representation = actor->GetProperty()->GetRepresentation();
  vtkHardwareSelector* selector = renderer->GetSelector();

  if (selector && this->PopulateSelectionSettings &&
    selector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    representation = VTK_POINTS;
  }

  // if we have cell scalars then we have to
  // explode the data
  vtkCellArray* prims[4];
  prims[0] = poly->GetVerts();
  prims[1] = poly->GetLines();
  prims[2] = poly->GetPolys();
  prims[3] = poly->GetStrips();

  // needs to get a cell call map passed in
  this->AppendCellTextures(
    renderer, actor, prims, representation, newColors, newNorms, poly, glBatchElement->CellCellMap);

  glBatchElement->CellCellMap->BuildPrimitiveOffsetsIfNeeded(
    prims, representation, poly->GetPoints());

  // Set the texture coordinate attribute if we are going to use texture for coloring
  vtkDataArray* tcoords = nullptr;
  if (this->HaveTCoords(poly))
  {
    tcoords = poly->GetPointData()->GetTCoords();
  }

  // Set specific texture coordinates if we are going to use texture for scalar coloring
  vtkDataArray* colorTCoords = nullptr;
  if (this->InterpolateScalarsBeforeMapping && this->ColorCoordinates)
  {
    colorTCoords = this->ColorCoordinates;
  }

  // Check if color array is already computed for the current array.
  // This step is mandatory otherwise the test ArrayExists will fail for "scalarColor" even if
  // the array used to map the color has already been added.
  if (c)
  {
    int cellFlag = 0; // not used
    vtkAbstractArray* abstractArray = vtkOpenGLBatchedPolyDataMapper::GetAbstractScalars(
      poly, this->ScalarMode, this->ArrayAccessMode, this->ArrayId, this->ArrayName, cellFlag);

    auto iter = this->ColorArrayMap.find(abstractArray);
    if (iter != this->ColorArrayMap.end())
    {
      c = iter->second;
    }
    else
    {
      this->ColorArrayMap[abstractArray] = c;
      c->Register(this);
    }
  }

  vtkFloatArray* tangents = vtkFloatArray::SafeDownCast(poly->GetPointData()->GetTangents());

  // Build the VBO
  vtkIdType offsetPos = 0;
  vtkIdType offsetNorm = 0;
  vtkIdType offsetColor = 0;
  vtkIdType offsetTex = 0;
  vtkIdType offsetColorTex = 0;
  vtkIdType offsetTangents = 0;
  vtkIdType totalOffset = 0;
  vtkIdType dummy = 0;
  bool exists =
    this->VBOs->ArrayExists("vertexMC", poly->GetPoints()->GetData(), offsetPos, totalOffset) &&
    this->VBOs->ArrayExists("normalMC", n, offsetNorm, dummy) &&
    this->VBOs->ArrayExists("scalarColor", c, offsetColor, dummy) &&
    this->VBOs->ArrayExists("tcoord", tcoords, offsetTex, dummy) &&
    this->VBOs->ArrayExists("colorTCoord", colorTCoords, offsetColorTex, dummy) &&
    this->VBOs->ArrayExists("tangentMC", tangents, offsetTangents, dummy);

  // if all used arrays have the same offset and have already been added,
  // we can reuse them and save memory
  if (exists && (offsetNorm == 0 || offsetPos == offsetNorm) &&
    (offsetColor == 0 || offsetPos == offsetColor) && (offsetTex == 0 || offsetPos == offsetTex) &&
    (offsetColorTex == 0 || offsetPos == offsetColorTex) &&
    (offsetTangents == 0 || offsetPos == offsetTangents))
  {
    vertexOffset = offsetPos;
  }
  else
  {
    this->VBOs->AppendDataArray("vertexMC", poly->GetPoints()->GetData(), VTK_FLOAT);
    this->VBOs->AppendDataArray("normalMC", n, VTK_FLOAT);
    this->VBOs->AppendDataArray("scalarColor", c, VTK_UNSIGNED_CHAR);
    this->VBOs->AppendDataArray("tcoord", tcoords, VTK_FLOAT);
    this->VBOs->AppendDataArray("colorTCoord", colorTCoords, VTK_FLOAT);
    this->VBOs->AppendDataArray("tangentMC", tangents, VTK_FLOAT);

    vertexOffset = totalOffset;
  }

  // now create the IBOs
  vtkOpenGLIndexBufferObject::AppendPointIndexBuffer(this->IndexArray[0], prims[0], vertexOffset);

  vtkDataArray* ef = poly->GetPointData()->GetAttribute(vtkDataSetAttributes::EDGEFLAG);
  if (ef)
  {
    if (ef->GetNumberOfComponents() != 1)
    {
      vtkDebugMacro(<< "Currently only 1d edge flags are supported.");
      ef = nullptr;
    }
    if (ef && !ef->IsA("vtkUnsignedCharArray"))
    {
      vtkDebugMacro(<< "Currently only unsigned char edge flags are supported.");
      ef = nullptr;
    }
  }

  vtkProperty* prop = actor->GetProperty();
  bool draw_surface_with_edges =
    (prop->GetEdgeVisibility() && prop->GetRepresentation() == VTK_SURFACE);

  if (representation == VTK_POINTS)
  {
    vtkOpenGLIndexBufferObject::AppendPointIndexBuffer(this->IndexArray[1], prims[1], vertexOffset);

    vtkOpenGLIndexBufferObject::AppendPointIndexBuffer(this->IndexArray[2], prims[2], vertexOffset);

    vtkOpenGLIndexBufferObject::AppendPointIndexBuffer(this->IndexArray[3], prims[3], vertexOffset);
  }
  else // WIREFRAME OR SURFACE
  {
    vtkOpenGLIndexBufferObject::AppendLineIndexBuffer(this->IndexArray[1], prims[1], vertexOffset);

    if (representation == VTK_WIREFRAME)
    {
      if (ef)
      {
        vtkOpenGLIndexBufferObject::AppendEdgeFlagIndexBuffer(
          this->IndexArray[2], prims[2], vertexOffset, ef);
      }
      else
      {
        vtkOpenGLIndexBufferObject::AppendTriangleLineIndexBuffer(
          this->IndexArray[2], prims[2], vertexOffset);
      }
      vtkOpenGLIndexBufferObject::AppendStripIndexBuffer(
        this->IndexArray[3], prims[3], vertexOffset, true);
    }
    else // SURFACE
    {
      if (draw_surface_with_edges)
      {
        // have to insert dummy values for points and lines
        vtkIdType* offsets = glBatchElement->CellCellMap->GetPrimitiveOffsets();
        this->EdgeValues.resize(offsets[2], 0);
        vtkOpenGLIndexBufferObject::AppendTriangleIndexBuffer(
          this->IndexArray[2], prims[2], poly->GetPoints(), vertexOffset, &this->EdgeValues, ef);
      }
      else
      {
        vtkOpenGLIndexBufferObject::AppendTriangleIndexBuffer(
          this->IndexArray[2], prims[2], poly->GetPoints(), vertexOffset, nullptr, nullptr);
      }
      vtkOpenGLIndexBufferObject::AppendStripIndexBuffer(
        this->IndexArray[3], prims[3], vertexOffset, false);
    }
  }

  if (prop->GetVertexVisibility())
  {
    vtkOpenGLIndexBufferObject::AppendVertexIndexBuffer(
      this->IndexArray[vtkOpenGLPolyDataMapper::PrimitiveVertices], prims, vertexOffset);
  }
}
#else
{
  (void)newColors;
  (void)newNorms;
  auto& batchElement = glBatchElement->Parent;
  vtkPolyData* poly = batchElement.PolyData;
  this->Superclass::AppendOneBufferObject(
    renderer, actor, poly, glBatchElement->CellCellMap, vertexOffset);
}
#endif

//------------------------------------------------------------------------------
void vtkOpenGLBatchedPolyDataMapper::BuildSelectionIBO(
  vtkPolyData*, std::vector<unsigned int> (&indices)[4], vtkIdType)
{
  for (auto& iter : this->VTKPolyDataToGLBatchElement)
  {
    auto glBatchElement = iter.second.get();
    auto& batchElement = glBatchElement->Parent;
    vtkPolyData* poly = batchElement.PolyData;
    this->Superclass::BuildSelectionIBO(poly, indices, glBatchElement->StartVertex);
  }
}

//------------------------------------------------------------------------------
// Returns if we can use texture maps for scalar coloring. Note this doesn't say
// we "will" use scalar coloring. It says, if we do use scalar coloring, we will
// use a texture.
// When rendering multiblock datasets, if any 2 blocks provide different
// lookup tables for the scalars, then also we cannot use textures. This case can
// be handled if required.
int vtkOpenGLBatchedPolyDataMapper::CanUseTextureMapForColoring(vtkDataObject*)
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

VTK_ABI_NAMESPACE_END
