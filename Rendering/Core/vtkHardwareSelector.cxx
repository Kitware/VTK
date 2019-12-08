/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHardwareSelector.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHardwareSelector.h"

#include "vtkCommand.h"
#include "vtkDataObject.h"
#include "vtkDataSetAttributes.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkProp.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkStructuredExtent.h"

#include <algorithm>
#include <cassert>
#include <map>
#include <set>

#define ID_OFFSET 1

//----------------------------------------------------------------------------
namespace
{
class PixelInformationComparator
{
public:
  bool operator()(const vtkHardwareSelector::PixelInformation& a,
    const vtkHardwareSelector::PixelInformation& b) const
  {
    if (a.Valid != b.Valid)
    {
      return a.Valid < b.Valid;
    }
    if (a.ProcessID != b.ProcessID)
    {
      return a.ProcessID < b.ProcessID;
    }
    if (a.Prop != b.Prop)
    {
      return a.Prop < b.Prop;
    }
    if (a.PropID != b.PropID)
    {
      return a.PropID < b.PropID;
    }
    return a.CompositeID < b.CompositeID;

    // We don't consider AttributeID in this comparison
  }
};
}

class vtkHardwareSelector::vtkInternals
{
public:
  // Ids for props that were hit.
  std::set<int> HitProps;
  std::map<int, vtkSmartPointer<vtkProp> > Props;
  std::map<int, std::vector<unsigned int> > PropPixels;
  std::map<int, double> ZValues;

  // state that's managed through the renderer
  double OriginalBackground[3];
  bool OriginalGradient;

  typedef std::map<PixelInformation, std::set<vtkIdType>, PixelInformationComparator>
    MapOfAttributeIds;

  typedef std::map<PixelInformation, vtkIdType, PixelInformationComparator> PixelCountType;

  //-----------------------------------------------------------------------------
  vtkSelection* ConvertSelection(
    int fieldassociation, const MapOfAttributeIds& dataMap, const PixelCountType& pixelCounts)
  {
    vtkSelection* sel = vtkSelection::New();

    MapOfAttributeIds::const_iterator iter;
    for (iter = dataMap.begin(); iter != dataMap.end(); ++iter)
    {
      const PixelInformation& key = iter->first;
      const std::set<vtkIdType>& id_values = iter->second;
      vtkSelectionNode* child = vtkSelectionNode::New();
      child->SetContentType(vtkSelectionNode::INDICES);
      switch (fieldassociation)
      {
        case vtkDataObject::FIELD_ASSOCIATION_CELLS:
          child->SetFieldType(vtkSelectionNode::CELL);
          break;

        case vtkDataObject::FIELD_ASSOCIATION_POINTS:
          child->SetFieldType(vtkSelectionNode::POINT);
          break;
      }
      child->GetProperties()->Set(vtkSelectionNode::PROP_ID(), key.PropID);
      child->GetProperties()->Set(vtkSelectionNode::PROP(), key.Prop);

      if (this->ZValues.find(key.PropID) != this->ZValues.end())
      {
        child->GetProperties()->Set(vtkSelectionNode::ZBUFFER_VALUE(), this->ZValues[key.PropID]);
      }

      PixelCountType::const_iterator pit = pixelCounts.find(key);
      child->GetProperties()->Set(vtkSelectionNode::PIXEL_COUNT(), pit->second);
      if (key.ProcessID >= 0)
      {
        child->GetProperties()->Set(vtkSelectionNode::PROCESS_ID(), key.ProcessID);
      }

      child->GetProperties()->Set(vtkSelectionNode::COMPOSITE_INDEX(), key.CompositeID);

      vtkIdTypeArray* ids = vtkIdTypeArray::New();
      ids->SetName("SelectedIds");
      ids->SetNumberOfComponents(1);
      ids->SetNumberOfTuples(static_cast<vtkIdType>(iter->second.size()));
      vtkIdType* ptr = ids->GetPointer(0);
      std::set<vtkIdType>::const_iterator idIter;
      vtkIdType cc = 0;
      for (idIter = id_values.begin(); idIter != id_values.end(); ++idIter, ++cc)
      {
        ptr[cc] = *idIter;
      }
      child->SetSelectionList(ids);
      ids->FastDelete();
      sel->AddNode(child);
      child->FastDelete();
    }

    return sel;
  }

  //-----------------------------------------------------------------------------
  bool PixelInsidePolygon(float x, float y, int* polygonPoints, vtkIdType count)
  {
    // http://en.wikipedia.org/wiki/Point_in_polygon
    // RayCasting method shooting the ray along the x axis, using float
    bool inside = false;
    float xintersection;
    for (vtkIdType i = 0; i < count; i += 2)
    {
      float p1X = polygonPoints[i];
      float p1Y = polygonPoints[i + 1];
      float p2X = polygonPoints[(i + 2) % count];
      float p2Y = polygonPoints[(i + 3) % count];

      if (y > std::min(p1Y, p2Y) && y <= std::max(p1Y, p2Y) && p1Y != p2Y)
      {
        if (x <= std::max(p1X, p2X))
        {
          xintersection = (y - p1Y) * (p2X - p1X) / (p2Y - p1Y) + p1X;
          if (p1X == p2X || x <= xintersection)
          {
            // each time intersect, toggle inside
            inside = !inside;
          }
        }
      }
    }

    return inside;
  }
};

//----------------------------------------------------------------------------
vtkAbstractObjectFactoryNewMacro(vtkHardwareSelector);

//----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkHardwareSelector, Renderer, vtkRenderer);

//----------------------------------------------------------------------------
vtkHardwareSelector::vtkHardwareSelector()
{
  this->Internals = new vtkInternals();
  this->Renderer = nullptr;
  this->Area[0] = this->Area[1] = this->Area[2] = this->Area[3] = 0;
  this->FieldAssociation = vtkDataObject::FIELD_ASSOCIATION_CELLS;
  this->MaximumPointId = 0;
  this->MaximumCellId = 0;
  for (int cc = 0; cc < 10; cc++)
  {
    this->RawPixBuffer[cc] = nullptr;
    this->PixBuffer[cc] = nullptr;
  }
  this->CurrentPass = -1;
  this->ProcessID = -1;
  this->PropColorValue[0] = this->PropColorValue[1] = this->PropColorValue[2] = 0;
  this->InPropRender = 0;
  this->UseProcessIdFromData = false;
  this->ActorPassOnly = false;
  this->CaptureZValues = false;
}

//----------------------------------------------------------------------------
vtkHardwareSelector::~vtkHardwareSelector()
{
  this->SetRenderer(nullptr);
  this->ReleasePixBuffers();
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkHardwareSelector::ReleasePixBuffers()
{
  for (int cc = 0; cc < 10; cc++)
  {
    delete[] this->PixBuffer[cc];
    this->PixBuffer[cc] = nullptr;
    delete[] this->RawPixBuffer[cc];
    this->RawPixBuffer[cc] = nullptr;
  }
  // this->Internals->Props.clear();
}

//----------------------------------------------------------------------------
void vtkHardwareSelector::BeginSelection()
{
  this->MaximumPointId = 0;
  this->MaximumCellId = 0;
  this->Renderer->Clear();
  this->Renderer->SetSelector(this);
  this->Internals->HitProps.clear();
  this->Internals->ZValues.clear();
  this->Internals->Props.clear();
  this->Internals->PropPixels.clear();
  this->ReleasePixBuffers();
}

//----------------------------------------------------------------------------
void vtkHardwareSelector::EndSelection()
{
  this->Internals->HitProps.clear();
  this->Renderer->SetSelector(nullptr);
}

//----------------------------------------------------------------------------
vtkSelection* vtkHardwareSelector::Select()
{
  vtkSelection* sel = nullptr;
  if (this->CaptureBuffers())
  {
    sel = this->GenerateSelection();
    this->ReleasePixBuffers();
  }
  return sel;
}

//----------------------------------------------------------------------------
bool vtkHardwareSelector::CaptureBuffers()
{
  if (!this->Renderer)
  {
    vtkErrorMacro("Renderer must be set before calling Select.");
    return false;
  }

  vtkRenderWindow* rwin = this->Renderer->GetRenderWindow();
  rwin->MakeCurrent();

  int rgba[4];
  rwin->GetColorBufferSizes(rgba);
  if (rgba[0] < 8 || rgba[1] < 8 || rgba[2] < 8)
  {
    vtkErrorMacro("Color buffer depth must be at least 8 bit. "
                  "Currently: "
      << rgba[0] << ", " << rgba[1] << ", " << rgba[2]);
    return false;
  }
  this->InvokeEvent(vtkCommand::StartEvent);

  rwin->SwapBuffersOff();

  // Initialize renderer for selection.
  // change the renderer's background to black, which will indicate a miss
  this->Renderer->GetBackground(this->Internals->OriginalBackground);
  this->Renderer->SetBackground(0.0, 0.0, 0.0);
  this->Internals->OriginalGradient = this->Renderer->GetGradientBackground();
  this->Renderer->GradientBackgroundOff();

  int preserveDepth = this->Renderer->GetPreserveDepthBuffer();
  int preserveColor = this->Renderer->GetPreserveColorBuffer();
  this->Renderer->SetPreserveDepthBuffer(0);
  this->Renderer->SetPreserveColorBuffer(0);

  this->BeginSelection();
  for (this->Iteration = 0; this->Iteration < 2; this->Iteration++)
  {
    for (this->CurrentPass = MIN_KNOWN_PASS; this->CurrentPass < MAX_KNOWN_PASS;
         this->CurrentPass++)
    {
      if (!this->PassRequired(this->CurrentPass))
      {
        continue;
      }

      this->PreCapturePass(this->CurrentPass);
      rwin->Render();
      this->PostCapturePass(this->CurrentPass);
    }
  }
  this->EndSelection();

  this->Renderer->SetPreserveDepthBuffer(preserveDepth);
  this->Renderer->SetPreserveColorBuffer(preserveColor);

  // restore original background
  this->Renderer->SetBackground(this->Internals->OriginalBackground);
  this->Renderer->SetGradientBackground(this->Internals->OriginalGradient);
  this->Renderer->GetRenderWindow()->SwapBuffersOn();
  this->InvokeEvent(vtkCommand::EndEvent);
  return true;
}

void vtkHardwareSelector::SetPropColorValue(vtkIdType val)
{
  float color[3];
  vtkHardwareSelector::Convert(val + 1, color);
  this->SetPropColorValue(color);
}

//----------------------------------------------------------------------------
bool vtkHardwareSelector::PassRequired(int pass)
{
  if (this->ActorPassOnly)
  {
    return (pass == ACTOR_PASS);
  }

  switch (pass)
  {
    case ACTOR_PASS:
      // only on the first iteration
      return (this->Iteration == 0);

    case PROCESS_PASS:
      // skip process pass if pid < 0 or not the first pass
      return (this->ProcessID >= 0 && this->Iteration == 0);

    case POINT_ID_LOW24:
      return (this->MaximumPointId >= 0xffffff || this->Iteration == 0);

    case POINT_ID_HIGH24:
      return (this->MaximumPointId >= 0xffffff && this->Iteration == 0);

    case CELL_ID_LOW24:
      return (this->MaximumCellId >= 0xffffff || this->Iteration == 0);

    case CELL_ID_HIGH24:
      return (this->MaximumCellId >= 0xffffff && this->Iteration == 0);
  }

  return true;
}

//----------------------------------------------------------------------------
void vtkHardwareSelector::SavePixelBuffer(int passNo)
{
  delete[] this->PixBuffer[passNo];
  this->PixBuffer[passNo] =
    this->Renderer->GetRenderWindow()->GetPixelData(this->Area[0], this->Area[1], this->Area[2],
      this->Area[3], (this->Renderer->GetRenderWindow()->GetSwapBuffers() == 1) ? 1 : 0);

  // we save the raw buffers the first time we see them
  if (!this->RawPixBuffer[passNo])
  {
    size_t numpix = (this->Area[2] - this->Area[0] + 1) * (this->Area[3] - this->Area[1] + 1) * 3;
    this->RawPixBuffer[passNo] = new unsigned char[numpix];
    memcpy(this->RawPixBuffer[passNo], this->PixBuffer[passNo], numpix);
  }
}

//----------------------------------------------------------------------------
void vtkHardwareSelector::ProcessPixelBuffers()
{
  if (this->CurrentPass == ACTOR_PASS)
  {
    this->BuildPropHitList(this->RawPixBuffer[this->CurrentPass]);
  }

  for (int ai : this->Internals->HitProps)
  {
    vtkProp* prop = this->GetPropFromID(ai);
    if (prop)
    {
      prop->ProcessSelectorPixelBuffers(this, this->Internals->PropPixels[ai]);
    }
  }
}

//----------------------------------------------------------------------------
// Also store the prop zvalues here as we traverse the images
void vtkHardwareSelector::BuildPropHitList(unsigned char* pixelbuffer)
{
  // grab the zbuffer if requested
  float* depthBuffer = nullptr;
  if (this->CaptureZValues)
  {
    depthBuffer = this->Renderer->GetRenderWindow()->GetZbufferData(
      this->Area[0], this->Area[1], this->Area[2], this->Area[3]);
  }

  unsigned int offset = 0;
  for (int yy = 0; yy <= static_cast<int>(this->Area[3] - this->Area[1]); yy++)
  {
    for (int xx = 0; xx <= static_cast<int>(this->Area[2] - this->Area[0]); xx++)
    {
      int val = this->Convert(xx, yy, pixelbuffer);
      if (val > 0)
      {
        val--;
        if (this->Internals->HitProps.find(val) == this->Internals->HitProps.end())
        {
          this->Internals->HitProps.insert(val);
          this->Internals->ZValues[val] = 1.0;
        }
        if (depthBuffer)
        {
          if (depthBuffer[offset] < this->Internals->ZValues[val])
          {
            this->Internals->ZValues[val] = depthBuffer[offset];
          }
        }
        this->Internals->PropPixels[val].push_back(offset * 3);
      }
      offset++;
    }
  }

  delete[] depthBuffer;
}

double vtkHardwareSelector::GetZValue(int val)
{
  auto zitr = this->Internals->ZValues.find(val);
  if (zitr != this->Internals->ZValues.end())
  {
    return zitr->second;
  }
  return 1.0;
}

//----------------------------------------------------------------------------
void vtkHardwareSelector::BeginRenderProp()
{
  this->InPropRender++;
  if (this->InPropRender != 1)
  {
    return;
  }

  // device specific prep
  vtkRenderWindow* renWin = this->Renderer->GetRenderWindow();
  this->BeginRenderProp(renWin);
}

//----------------------------------------------------------------------------
void vtkHardwareSelector::EndRenderProp()
{
  if (this->InPropRender)
  {
    this->InPropRender--;

    if (this->InPropRender != 0)
    {
      return;
    }

    // device specific cleanup
    vtkRenderWindow* renWin = this->Renderer->GetRenderWindow();
    this->EndRenderProp(renWin);
  }
}

//----------------------------------------------------------------------------
void vtkHardwareSelector::RenderCompositeIndex(unsigned int index)
{

  if (index > 0xffffff)
  {
    vtkErrorMacro("Indices > 0xffffff are not supported.");
    return;
  }
}

//----------------------------------------------------------------------------
void vtkHardwareSelector::UpdateMaximumPointId(vtkIdType attribid)
{
  if (attribid < 0)
  {
    // negative attribid is valid. It happens when rendering higher order
    // elements where new points are added for rendering smooth surfaces.
    return;
  }

  this->MaximumPointId = (attribid > this->MaximumPointId) ? attribid : this->MaximumPointId;
}

//----------------------------------------------------------------------------
void vtkHardwareSelector::UpdateMaximumCellId(vtkIdType attribid)
{
  if (attribid < 0)
  {
    // negative attribid is valid. It happens when rendering higher order
    // elements where new points are added for rendering smooth surfaces.
    return;
  }

  this->MaximumCellId = (attribid > this->MaximumCellId) ? attribid : this->MaximumCellId;
}

//----------------------------------------------------------------------------
void vtkHardwareSelector::RenderProcessId(unsigned int processid)
{
  if (this->CurrentPass == PROCESS_PASS && this->UseProcessIdFromData)
  {
    if (processid >= 0xffffff)
    {
      vtkErrorMacro("Invalid id: " << processid);
      return;
    }
  }
}

//----------------------------------------------------------------------------
int vtkHardwareSelector::Render(vtkRenderer* renderer, vtkProp** propArray, int propArrayCount)
{
  if (this->Renderer != renderer)
  {
    vtkErrorMacro("Usage error.");
    return 0;
  }

  int propsRendered = 0;
  // loop through props and give them a chance to
  // render themselves as opaque geometry
  for (int i = 0; i < propArrayCount; i++)
  {
    // all props in propArray are already visible, hence no need to check again
    // (vtkRenderer ensures that).
    if (!propArray[i]->GetPickable())
    {
      continue;
    }
    this->PropID = this->GetPropID(i, propArray[i]);
    this->Internals->Props[this->PropID] = propArray[i];
    if (this->IsPropHit(this->PropID))
    {
      propsRendered += propArray[i]->RenderOpaqueGeometry(renderer);
    }
  }

  // Render props as volumetric data.
  for (int i = 0; i < propArrayCount; i++)
  {
    if (!propArray[i]->GetPickable())
    {
      continue;
    }
    this->PropID = this->GetPropID(i, propArray[i]);
    this->Internals->Props[this->PropID] = propArray[i];
    if (this->IsPropHit(this->PropID))
    {
      propsRendered += propArray[i]->RenderVolumetricGeometry(renderer);
    }
  }

  // loop through props and give them a chance to render themselves as
  // overlay geometry. This allows the overlay geometry
  // also being selected. The props in overlay can use Pickable or
  // SupportsSelection variables to decide whether to be included in this pass.
  for (int i = 0; i < propArrayCount; i++)
  {
    if (!propArray[i]->GetPickable())
    {
      continue;
    }
    this->PropID = this->GetPropID(i, propArray[i]);
    this->Internals->Props[this->PropID] = propArray[i];
    if (this->IsPropHit(this->PropID))
    {
      propsRendered += propArray[i]->RenderOverlay(renderer);
    }
  }

  // loop over hit props and give them a chance to modify the
  // buffer

  this->SavePixelBuffer(this->CurrentPass);
  this->ProcessPixelBuffers();

  return propsRendered;
}

//----------------------------------------------------------------------------
vtkProp* vtkHardwareSelector::GetPropFromID(int id)
{
  std::map<int, vtkSmartPointer<vtkProp> >::iterator iter = this->Internals->Props.find(id);
  if (iter != this->Internals->Props.end())
  {
    return iter->second;
  }
  return nullptr;
}

//----------------------------------------------------------------------------
std::string vtkHardwareSelector::PassTypeToString(PassTypes type)
{
  switch (type)
  {
    case vtkHardwareSelector::PROCESS_PASS:
      return "PROCESS_PASS";
    case vtkHardwareSelector::ACTOR_PASS:
      return "ACTOR_PASS";
    case vtkHardwareSelector::COMPOSITE_INDEX_PASS:
      return "COMPOSITE_INDEX_PASS";
    case vtkHardwareSelector::POINT_ID_LOW24:
      return "POINT_ID_LOW24_PASS";
    case vtkHardwareSelector::POINT_ID_HIGH24:
      return "POINT_ID_HIGH24_PASS";
    case vtkHardwareSelector::CELL_ID_LOW24:
      return "CELL_ID_LOW24_PASS";
    case vtkHardwareSelector::CELL_ID_HIGH24:
      return "CELL_ID_HIGH24_PASS";
    default:
      return "Invalid Enum";
  }
}

//----------------------------------------------------------------------------
bool vtkHardwareSelector::IsPropHit(int id)
{
  return (this->Internals->HitProps.empty() ||
    this->Internals->HitProps.find(id) != this->Internals->HitProps.end());
}

//----------------------------------------------------------------------------
vtkHardwareSelector::PixelInformation vtkHardwareSelector::GetPixelInformation(
  const unsigned int in_display_position[2], int maxDistance, unsigned int out_selected_position[2])
{
  assert(in_display_position != out_selected_position);

  // Base case
  unsigned int maxDist = (maxDistance < 0) ? 0 : static_cast<unsigned int>(maxDistance);
  if (maxDist == 0)
  {
    out_selected_position[0] = in_display_position[0];
    out_selected_position[1] = in_display_position[1];
    if (in_display_position[0] < this->Area[0] || in_display_position[0] > this->Area[2] ||
      in_display_position[1] < this->Area[1] || in_display_position[1] > this->Area[3])
    {
      return PixelInformation();
    }

    // offset in_display_position based on the lower-left-corner of the Area.
    unsigned int display_position[2] = { in_display_position[0] - this->Area[0],
      in_display_position[1] - this->Area[1] };

    int actorid = this->Convert(display_position, this->PixBuffer[ACTOR_PASS]);
    if (actorid <= 0)
    {
      // the pixel did not hit any actor.
      return PixelInformation();
    }

    PixelInformation info;
    info.Valid = true;

    actorid--;
    info.PropID = actorid;
    info.Prop = this->GetPropFromID(actorid);
    if (this->ActorPassOnly)
    {
      return info;
    }

    int composite_id =
      this->Convert(display_position, this->PixBuffer[COMPOSITE_INDEX_PASS]) - ID_OFFSET;
    if (composite_id < 0 || composite_id > 0xffffff)
    {
      // the pixel did not hit any composite
      return PixelInformation();
    }
    info.CompositeID = static_cast<unsigned int>(composite_id);

    int low24 = this->Convert(display_position, this->PixBuffer[CELL_ID_LOW24]);
    int high24 = this->Convert(display_position, this->PixBuffer[CELL_ID_HIGH24]);
    if (this->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS)
    {
      low24 = this->Convert(display_position, this->PixBuffer[POINT_ID_LOW24]);
      high24 = this->Convert(display_position, this->PixBuffer[POINT_ID_HIGH24]);
    }

    // id 0 is reserved for nothing present.
    info.AttributeID = (this->GetID(low24, high24, 0) - ID_OFFSET);
    if (info.AttributeID < 0)
    {
      // the pixel did not hit any cell.
      return PixelInformation();
    }

    info.ProcessID =
      this->Convert(display_position[0], display_position[1], this->PixBuffer[PROCESS_PASS]);
    info.ProcessID--;
    return info;
  }

  // Iterate over successively growing boxes.
  // They recursively call the base case to handle single pixels.
  unsigned int disp_pos[2] = { in_display_position[0], in_display_position[1] };
  unsigned int cur_pos[2] = { 0, 0 };
  PixelInformation info;
  info = this->GetPixelInformation(in_display_position, 0, out_selected_position);
  if (info.Valid)
  {
    return info;
  }
  for (unsigned int dist = 1; dist < maxDist; ++dist)
  {
    // Vertical sides of box.
    for (unsigned int y = ((disp_pos[1] > dist) ? (disp_pos[1] - dist) : 0);
         y <= disp_pos[1] + dist; ++y)
    {
      cur_pos[1] = y;
      if (disp_pos[0] >= dist)
      {
        cur_pos[0] = disp_pos[0] - dist;
        info = this->GetPixelInformation(cur_pos, 0, out_selected_position);
        if (info.Valid)
        {
          return info;
        }
      }
      cur_pos[0] = disp_pos[0] + dist;
      info = this->GetPixelInformation(cur_pos, 0, out_selected_position);
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
        info = this->GetPixelInformation(cur_pos, 0, out_selected_position);
        if (info.Valid)
        {
          return info;
        }
      }
      cur_pos[1] = disp_pos[1] + dist;
      info = this->GetPixelInformation(cur_pos, 0, out_selected_position);
      if (info.Valid)
      {
        return info;
      }
    }
  }

  // nothing hit.
  out_selected_position[0] = in_display_position[0];
  out_selected_position[1] = in_display_position[1];
  return PixelInformation();
}

//----------------------------------------------------------------------------
vtkSelection* vtkHardwareSelector::GenerateSelection(
  unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2)
{
  vtkInternals::MapOfAttributeIds dataMap;
  vtkInternals::PixelCountType pixelCounts;

  for (unsigned int yy = y1; yy <= y2; yy++)
  {
    for (unsigned int xx = x1; xx <= x2; xx++)
    {
      unsigned int pos[2] = { xx, yy };
      PixelInformation info = this->GetPixelInformation(pos, 0);
      if (info.Valid)
      {
        dataMap[info].insert(info.AttributeID);
        pixelCounts[info]++;
      }
    }
  }
  return this->Internals->ConvertSelection(this->FieldAssociation, dataMap, pixelCounts);
}

//----------------------------------------------------------------------------
vtkSelection* vtkHardwareSelector::GeneratePolygonSelection(int* polygonPoints, vtkIdType count)
{
  // we need at least three points (x,y) for a polygon selection.
  if (!polygonPoints || count < 6)
  {
    return nullptr;
  }

  int x1 = VTK_INT_MAX, x2 = VTK_INT_MIN, y1 = VTK_INT_MAX, y2 = VTK_INT_MIN;
  // Get polygon bounds, so that we only check pixels within the bounds
  for (vtkIdType i = 0; i < count; i += 2)
  {
    x1 = std::min(polygonPoints[i], x1);
    x2 = std::max(polygonPoints[i], x2);
    y1 = std::min(polygonPoints[i + 1], y1);
    y2 = std::max(polygonPoints[i + 1], y2);
  }

  vtkInternals::MapOfAttributeIds dataMap;
  vtkInternals::PixelCountType pixelCounts;
  for (int yy = y1; yy <= y2; yy++)
  {
    for (int xx = x1; xx <= x2; xx++)
    {
      if (this->Internals->PixelInsidePolygon(xx, yy, polygonPoints, count))
      {
        unsigned int pos[2] = { static_cast<unsigned int>(xx), static_cast<unsigned int>(yy) };
        PixelInformation info = this->GetPixelInformation(pos, 0);
        if (info.Valid)
        {
          dataMap[info].insert(info.AttributeID);
          pixelCounts[info]++;
        }
      }
    }
  }
  return this->Internals->ConvertSelection(this->FieldAssociation, dataMap, pixelCounts);
}

//----------------------------------------------------------------------------
void vtkHardwareSelector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FieldAssociation: ";
  switch (this->FieldAssociation)
  {
    case vtkDataObject::FIELD_ASSOCIATION_POINTS:
      os << "FIELD_ASSOCIATION_POINTS" << endl;
      break;
    case vtkDataObject::FIELD_ASSOCIATION_CELLS:
      os << "FIELD_ASSOCIATION_CELLS" << endl;
      break;
    case vtkDataObject::FIELD_ASSOCIATION_VERTICES:
      os << "FIELD_ASSOCIATION_VERTICES" << endl;
      break;
    case vtkDataObject::FIELD_ASSOCIATION_EDGES:
      os << "FIELD_ASSOCIATION_EDGES" << endl;
      break;
    case vtkDataObject::FIELD_ASSOCIATION_ROWS:
      os << "FIELD_ASSOCIATION_ROWS" << endl;
      break;
    default:
      os << "--unknown--" << endl;
  }
  os << indent << "ProcessID: " << this->ProcessID << endl;
  os << indent << "CurrentPass: " << this->CurrentPass << endl;
  os << indent << "Area: " << this->Area[0] << ", " << this->Area[1] << ", " << this->Area[2]
     << ", " << this->Area[3] << endl;
  os << indent << "Renderer: " << this->Renderer << endl;
  os << indent << "UseProcessIdFromData: " << this->UseProcessIdFromData << endl;
  os << indent << "ActorPassOnly: " << this->ActorPassOnly << endl;
}
