// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWebGPUHardwareSelector.h"

#include "vtkDataObject.h"
#include "vtkHardwareSelector.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkWebGPURenderWindow.h"

#include <iterator>

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkWebGPUHardwareSelector);

vtkWebGPUHardwareSelector::vtkWebGPUHardwareSelector() = default;

//------------------------------------------------------------------------------
vtkWebGPUHardwareSelector::~vtkWebGPUHardwareSelector() = default;

//------------------------------------------------------------------------------
bool vtkWebGPUHardwareSelector::CaptureBuffers()
{
  if (!this->Renderer)
  {
    vtkErrorMacro("Renderer must be set before calling Select.");
    return false;
  }
  if (auto* wgpuRenderWindow =
        vtkWebGPURenderWindow::SafeDownCast(this->Renderer->GetRenderWindow()))
  {
    if (auto wgpuConfiguration = wgpuRenderWindow->WGPUConfiguration)
    {
      if (this->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
      {
        // render a second time and draw only points.
        // mappers check if points need to be drawn for selection
        // if the renderer has a selector and selector->GetFieldAssociation() ==
        // vtkDataObject::FIELD_ASSOCIATION_POINTS)
        wgpuRenderWindow->Render();
      }
      this->BeginSelection();

      // Map a subset of the IDs texture into a buffer and copy the values into this->IdBuffer.
      const auto& queryXMin = this->Area[0];
      const auto& queryYMin = this->Area[1];
      const auto& queryXMax = this->Area[2];
      const auto& queryYMax = this->Area[3];
      auto onTextureMapped = [queryYMax, queryYMin](
                               const void* mappedData, int bytesPerRow, void* userData)
      {
        auto* idBuffer = reinterpret_cast<std::vector<Ids>*>(userData);
        const Ids* mappedDataAsIds = reinterpret_cast<const Ids*>(mappedData);
        std::copy(mappedDataAsIds,
          mappedDataAsIds + (queryYMax - queryYMin + 1) * bytesPerRow / sizeof(Ids),
          std::back_inserter(*idBuffer));
      };
      wgpuRenderWindow->ReadTextureFromGPU(wgpuRenderWindow->IdsAttachment.Texture,
        wgpuRenderWindow->IdsAttachment.Format, /*mipLevel=*/0, wgpu::TextureAspect::All,
        wgpu::Origin3D{ queryXMin, queryYMin, 0 },
        wgpu::Extent3D{ queryXMax - queryXMin + 1, queryYMax - queryYMin + 1, 1 }, onTextureMapped,
        reinterpret_cast<void*>(&(this->IdBuffer)));
      // Wait for texture copy to buffer.
      wgpuRenderWindow->WaitForCompletion();
      this->BuildPropHitList(this->PixBuffer[ACTOR_PASS]);
      this->EndSelection();
      return true;
    }
    else
    {
      vtkErrorMacro("This selector's render window is not configured to use webgpu. Please call "
                    "vtkWebGPURenderWindow::SetWGPUConfiguration().");
      return false;
    }
  }
  else
  {
    vtkErrorMacro(
      << "Cannot capture Ids buffer because this selector is not using a webgpu render window");
    return false;
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUHardwareSelector::BeginSelection()
{
  this->Superclass::BeginSelection();
  vtkProp* aProp;
  int propArrayCount = 0;
  if (this->Renderer->GetViewProps()->GetNumberOfItems() > 0)
  {
    this->PropArray = new vtkProp*[this->Renderer->GetViewProps()->GetNumberOfItems()];
  }
  else
  {
    this->PropArray = nullptr;
  }

  propArrayCount = 0;
  vtkCollectionSimpleIterator pit;
  for (this->Renderer->GetViewProps()->InitTraversal(pit);
       (aProp = this->Renderer->GetViewProps()->GetNextProp(pit));)
  {
    if (aProp->GetVisibility())
    {
      this->PropArray[propArrayCount++] = aProp;
    }
  }
  for (int i = MIN_KNOWN_PASS; i < MAX_KNOWN_PASS + 1; ++i)
  {
    unsigned char* iPtr = new unsigned char();
    *iPtr = i;
    this->PixBuffer[i] = iPtr;
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUHardwareSelector::EndSelection()
{
  this->Superclass::EndSelection();
}

//------------------------------------------------------------------------------
vtkProp* vtkWebGPUHardwareSelector::GetPropFromID(int id)
{
  return this->PropArray[id];
}

//------------------------------------------------------------------------------
void vtkWebGPUHardwareSelector::ReleasePixBuffers()
{
  this->IdBuffer.clear();

  for (int i = MIN_KNOWN_PASS; i < MAX_KNOWN_PASS + 1; ++i)
  {
    delete this->PixBuffer[i];
    this->PixBuffer[i] = nullptr;
  }
  delete[] this->PropArray;
  this->PropArray = nullptr;
}

//------------------------------------------------------------------------------
int vtkWebGPUHardwareSelector::Convert(int x, int y, unsigned char* pb)
{
  if (!pb)
  {
    return 0;
  }
  if (this->IdBuffer.empty())
  {
    vtkErrorMacro(<< "Ids are not captured!");
    return 0;
  }
  const auto& queryXMin = this->Area[0];
  const auto& queryXMax = this->Area[2];
  const auto queryWidth = queryXMax - queryXMin + 1;
  std::size_t pixelLinearId = (y * queryWidth + x);
  const auto& ids = this->IdBuffer[pixelLinearId];
  if (*pb == vtkHardwareSelector::ACTOR_PASS)
  {
    return ids.PropId;
  }
  else if (*pb == vtkHardwareSelector::COMPOSITE_INDEX_PASS)
  {
    return ids.CompositeId;
  }
  else if (*pb == vtkHardwareSelector::POINT_ID_HIGH24 ||
    *pb == vtkHardwareSelector::POINT_ID_LOW24)
  {
    return ids.AttributeId;
  }
  else if (*pb == vtkHardwareSelector::PROCESS_PASS)
  {
    return ids.ProcessId;
  }
  else if (*pb == vtkHardwareSelector::CELL_ID_HIGH24 || *pb == vtkHardwareSelector::CELL_ID_LOW24)
  {
    return ids.AttributeId;
  }
  else
  {
    // unimplemented.
    return 0;
  }
}

//------------------------------------------------------------------------------
vtkHardwareSelector::PixelInformation vtkWebGPUHardwareSelector::GetPixelInformation(
  const unsigned int inDisplayPosition[2], int maxDist, unsigned int outSelectedPosition[2])
{
  assert(inDisplayPosition != outSelectedPosition);
  auto* wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(this->Renderer->GetRenderWindow());
  if (wgpuRenderWindow == nullptr)
  {
    return {};
  }
  if (this->IdBuffer.empty())
  {
    return {};
  }

  const auto& queryXMin = this->Area[0];
  const auto& queryYMin = this->Area[1];
  const auto& queryXMax = this->Area[2];
  const auto& queryYMax = this->Area[3];
  // Simple case when maxDist = 0
  const unsigned int maxDistanceClamped = (maxDist < 0) ? 0 : static_cast<unsigned int>(maxDist);
  if (maxDistanceClamped == 0)
  {
    outSelectedPosition[0] = inDisplayPosition[0];
    outSelectedPosition[1] = inDisplayPosition[1];
    if (inDisplayPosition[0] < queryXMin || inDisplayPosition[0] > queryXMax ||
      inDisplayPosition[1] < queryYMin || inDisplayPosition[1] > queryYMax)
    {
      return {};
    }

    unsigned int displayPosition[2] = { inDisplayPosition[0] - this->Area[0],
      inDisplayPosition[1] - this->Area[1] };
    const int actorId =
      this->Convert(displayPosition[0], displayPosition[1], this->PixBuffer[ACTOR_PASS]);
    if (actorId == 0)
    {
      return {};
    }
    PixelInformation info;
    info.Valid = true;
    // undo offset from shader
    info.PropID = actorId - 1;
    info.Prop = this->GetPropFromID(info.PropID);
    if (this->ActorPassOnly)
    {
      return info;
    }
    const int compositeId =
      this->Convert(displayPosition[0], displayPosition[1], this->PixBuffer[COMPOSITE_INDEX_PASS]);
    if (compositeId == 0)
    {
      return {};
    }
    // undo offset from shader
    info.CompositeID = compositeId - 1;

    if (this->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
    {
      const int pointId =
        this->Convert(displayPosition[0], displayPosition[1], this->PixBuffer[POINT_ID_HIGH24]);
      if (pointId > 0)
      {
        info.AttributeID = pointId - 1;
      }
    }
    else
    {
      const int cellId =
        this->Convert(displayPosition[0], displayPosition[1], this->PixBuffer[CELL_ID_HIGH24]);
      if (cellId > 0)
      {
        info.AttributeID = cellId - 1;
      }
    }
    const int processId =
      this->Convert(displayPosition[0], displayPosition[1], this->PixBuffer[PROCESS_PASS]);
    if (processId > 0)
    {
      info.ProcessID = processId - 1;
    }
    return info;
  }
  else
  {
    // Iterate over successively growing boxes.
    // They recursively call the base case to handle single pixels.
    // Iterate over successively growing boxes.
    // They recursively call the base case to handle single pixels.
    unsigned int disp_pos[2] = { inDisplayPosition[0], inDisplayPosition[1] };
    unsigned int cur_pos[2] = { 0, 0 };
    PixelInformation info;
    info = this->GetPixelInformation(inDisplayPosition, 0, outSelectedPosition);
    if (info.Valid)
    {
      return info;
    }
    for (unsigned int dist = 1; dist < maxDistanceClamped; ++dist)
    {
      // Vertical sides of box.
      for (unsigned int y = ((disp_pos[1] > dist) ? (disp_pos[1] - dist) : 0);
           y <= disp_pos[1] + dist; ++y)
      {
        cur_pos[1] = y;
        if (disp_pos[0] >= dist)
        {
          cur_pos[0] = disp_pos[0] - dist;
          info = this->GetPixelInformation(cur_pos, 0, outSelectedPosition);
          if (info.Valid)
          {
            return info;
          }
        }
        cur_pos[0] = disp_pos[0] + dist;
        info = this->GetPixelInformation(cur_pos, 0, outSelectedPosition);
        if (info.Valid)
        {
          return info;
        }
      }
      // Horizontal sides of box.
      for (unsigned int x = ((disp_pos[0] >= dist) ? (disp_pos[0] - (dist - 1)) : 0);
           x <= disp_pos[0] + (dist - 1); ++x)
      {
        cur_pos[0] = x;
        if (disp_pos[1] >= dist)
        {
          cur_pos[1] = disp_pos[1] - dist;
          info = this->GetPixelInformation(cur_pos, 0, outSelectedPosition);
          if (info.Valid)
          {
            return info;
          }
        }
        cur_pos[1] = disp_pos[1] + dist;
        info = this->GetPixelInformation(cur_pos, 0, outSelectedPosition);
        if (info.Valid)
        {
          return info;
        }
      }
    }
  }
  // nothing hit.
  outSelectedPosition[0] = inDisplayPosition[0];
  outSelectedPosition[1] = inDisplayPosition[1];
  return PixelInformation();
}

//------------------------------------------------------------------------------
void vtkWebGPUHardwareSelector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
