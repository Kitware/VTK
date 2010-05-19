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
#include "vtkPainterDeviceAdapter.h"
#include "vtkProp.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"

#include <vtkstd/set>
#include <vtkstd/map>

#define TEX_UNIT_ATTRIBID 1
#define ID_OFFSET 1
class vtkHardwareSelector::vtkInternals
{
public:
  // Ids for props that were hit.
  vtkstd::set<int> HitProps;
  vtkstd::map<int, vtkSmartPointer<vtkProp> > Props;
  double OriginalBackground[3];
  bool OriginalGradient;
  int OriginalMultisample;
  int OriginalLighting;
  int OriginalBlending;
};

vtkStandardNewMacro(vtkHardwareSelector);
vtkCxxSetObjectMacro(vtkHardwareSelector, Renderer, vtkRenderer);
//----------------------------------------------------------------------------
vtkHardwareSelector::vtkHardwareSelector()
{
  this->Internals = new vtkInternals();
  this->Renderer = 0;
  this->Area[0] = this->Area[1] = this->Area[2] = this->Area[3] = 0;
  this->FieldAssociation = vtkDataObject::FIELD_ASSOCIATION_CELLS;
  this->MaxAttributeId = 0;
  for (int cc=0; cc < 10; cc++)
    {
    this->PixBuffer[cc] = 0;
    }
  this->CurrentPass = -1;
  this->ProcessID = -1;
}

//----------------------------------------------------------------------------
vtkHardwareSelector::~vtkHardwareSelector()
{
  this->SetRenderer(0);
  this->ReleasePixBuffers();
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkHardwareSelector::ReleasePixBuffers()
{
  for (int cc=0; cc < 10; cc++)
    {
    delete [] this->PixBuffer[cc];
    this->PixBuffer[cc] = 0;
    }
  this->Internals->Props.clear();
}

//----------------------------------------------------------------------------
void vtkHardwareSelector::BeginSelection()
{
  this->MaxAttributeId = 0;
  this->Renderer->Clear();
  this->Renderer->SetSelector(this);
  this->Renderer->PreserveDepthBufferOn();
  this->Internals->HitProps.clear();
  this->ReleasePixBuffers();
}

//----------------------------------------------------------------------------
void vtkHardwareSelector::EndSelection()
{
  this->Internals->HitProps.clear();
  this->Renderer->SetSelector(NULL);
  this->Renderer->PreserveDepthBufferOff();
}

//----------------------------------------------------------------------------
vtkSelection* vtkHardwareSelector::Select()
{
  vtkSelection* sel = 0;
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

  vtkRenderWindow *rwin = this->Renderer->GetRenderWindow();
  int rgba[4];
  rwin->GetColorBufferSizes(rgba);
  if (rgba[0] < 8 || rgba[1] < 8 || rgba[2] < 8)
    {
    vtkErrorMacro("Color buffer depth must be atleast 8 bit. "
      "Currently: " << rgba[0] << ", " << rgba[1] << ", " <<rgba[2]);
    return false;
    }

  rwin->SwapBuffersOff();
  vtkRenderWindowInteractor* iren = rwin->GetInteractor();

  // Initialize renderer for selection.
  //change the renderer's background to black, which will indicate a miss
  this->Renderer->GetBackground(this->Internals->OriginalBackground);
  this->Renderer->SetBackground(0.0,0.0,0.0);
  this->Internals->OriginalGradient = this->Renderer->GetGradientBackground();
  this->Renderer->GradientBackgroundOff();

  this->BeginSelection();
  for (this->CurrentPass = MIN_KNOWN_PASS;
    this->CurrentPass < MAX_KNOWN_PASS; this->CurrentPass++)
    {
    if (!this->PassRequired(this->CurrentPass))
      {
      continue;
      }
    this->InvokeEvent(vtkCommand::StartEvent);
    // We go through render window interactor, if available, since that allows
    // applications, such as ParaView, to do application specific updates etc.
    // before render gets called.
    if (iren)
      {
      iren->Render();
      }
    else
      {
      rwin->Render();
      }
    this->InvokeEvent(vtkCommand::EndEvent);
    this->SavePixelBuffer(this->CurrentPass);
    }
  this->EndSelection();

  //restore original background
  this->Renderer->SetBackground(this->Internals->OriginalBackground);
  this->Renderer->SetGradientBackground(this->Internals->OriginalGradient);
  this->Renderer->GetRenderWindow()->SwapBuffersOn();
  return true;
}

//----------------------------------------------------------------------------
bool vtkHardwareSelector::PassRequired(int pass)
{
  switch (pass)
    {
  case PROCESS_PASS:
    // skip process pass is pid < 0.
    return (this->ProcessID >= 0);

  case ID_MID24:
    return (this->MaxAttributeId >= 0xffffff);

  case ID_HIGH16:
    int upper = (0xffffff & (this->MaxAttributeId >> 24));
    return (upper > 0);
    }
  return true;
}

//----------------------------------------------------------------------------
void vtkHardwareSelector::SavePixelBuffer(int passNo)
{
  if (this->PixBuffer[passNo])
    {
    delete [] this->PixBuffer[passNo];
    this->PixBuffer[passNo] = 0;
    }

  this->PixBuffer[passNo] = this->Renderer->GetRenderWindow()->GetPixelData(
    this->Area[0], this->Area[1], this->Area[2], this->Area[3],
    (this->Renderer->GetRenderWindow()->GetSwapBuffers() == 1)? 1 : 0);

  if (passNo == ACTOR_PASS)
    {
    this->BuildPropHitList(this->PixBuffer[passNo]);
    }
}

//----------------------------------------------------------------------------
void vtkHardwareSelector::BuildPropHitList(unsigned char* pixelbuffer)
{
  for (int yy=0; yy <= static_cast<int>(this->Area[3]-this->Area[1]); yy++)
    {
    for (int xx=0; xx <= static_cast<int>(this->Area[2]-this->Area[0]); xx++)
      {
      int val = this->Convert(xx, yy, pixelbuffer);
      if (val > 0)
        {
        val--;
        this->Internals->HitProps.insert(val);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkHardwareSelector::BeginRenderProp()
{
  // Ensure that blending/lighting/multisampling is off.
  vtkPainterDeviceAdapter* device = this->Renderer->GetRenderWindow()->
    GetPainterDeviceAdapter();
  this->Internals->OriginalMultisample = device->QueryMultisampling();
  this->Internals->OriginalLighting = device->QueryLighting();
  this->Internals->OriginalBlending = device->QueryBlending();

  device->MakeMultisampling(0);
  device->MakeLighting(0);
  device->MakeBlending(0);
  //cout << "In BeginRenderProp" << endl;
  //glFinish(); 
  if (this->CurrentPass == ACTOR_PASS)
    {
    int propid = this->PropID;
    if (propid >= 0xfffffe)
      {
      vtkErrorMacro("Too many props. Currently only " << 0xfffffe
        << " props are supported.");
      return;
      }
    float color[3];
    // Since 0 is reserved for nothing selected, we offset propid by 1.
    propid = propid + 1;
    vtkHardwareSelector::Convert(propid, color);
    this->Renderer->GetRenderWindow()->GetPainterDeviceAdapter()->SendAttribute(
      vtkDataSetAttributes::SCALARS, 3, VTK_FLOAT, color);
    }
  else if (this->CurrentPass == PROCESS_PASS)
    {
    float color[3];
    // Since 0 is reserved for nothing selected, we offset propid by 1.
    vtkHardwareSelector::Convert(this->ProcessID + 1, color);
    this->Renderer->GetRenderWindow()->GetPainterDeviceAdapter()->SendAttribute(
      vtkDataSetAttributes::SCALARS, 3, VTK_FLOAT, color);
    }
}

//----------------------------------------------------------------------------
void vtkHardwareSelector::EndRenderProp()
{
  vtkPainterDeviceAdapter* device = this->Renderer->GetRenderWindow()->
    GetPainterDeviceAdapter();
  device->MakeMultisampling(this->Internals->OriginalMultisample);
  device->MakeLighting(this->Internals->OriginalLighting);
  device->MakeBlending(this->Internals->OriginalBlending);
}

//----------------------------------------------------------------------------
// TODO: make inline
void vtkHardwareSelector::RenderAttributeId(vtkIdType attribid)
{
  if (attribid < 0)
    {
    vtkErrorMacro("Invalid id: " << attribid);
    return;
    }

  this->MaxAttributeId = (attribid > this->MaxAttributeId)? attribid :
    this->MaxAttributeId;

  if (this->CurrentPass < ID_LOW24 || this->CurrentPass > ID_HIGH16)
    {
    return;
    }

  // 0 is reserved.
  attribid += ID_OFFSET;

  for (int cc=0; cc < 3; cc++)
    {
    int words24 = (0xffffff & attribid);
    attribid = attribid >> 24;
    if ((this->CurrentPass - ID_LOW24) == cc)
      {
      float color[3];
      vtkHardwareSelector::Convert(words24, color);
      this->Renderer->GetRenderWindow()->GetPainterDeviceAdapter()->SendAttribute(
        vtkDataSetAttributes::SCALARS, 3, VTK_FLOAT, color);
      break;
      }
    }
}

//----------------------------------------------------------------------------
int vtkHardwareSelector::Render(vtkRenderer* renderer, vtkProp** propArray,
  int propArrayCount)
{
  if (this->Renderer != renderer)
    {
    vtkErrorMacro("Usage error.");
    return 0;
    }

  // I have no idea what this comment means. Preserving from the original code.
  //todo: save off and swap in other renderer/renderwindow settings that
  //could affect colors

  int propsRenderered = 0;
  // loop through props and give them a chance to 
  // render themselves as opaque geometry
  for (int i = 0; i < propArrayCount; i++ )
    {
    // all props in propArray are already visible, hence no need to check again
    // (vtkRenderer ensures that).
    if (!propArray[i]->GetPickable() || !propArray[i]->GetSupportsSelection())
      {
      continue;
      }
    this->PropID = this->GetPropID(i, propArray[i]);
    this->Internals->Props[this->PropID] = propArray[i];
    if (this->IsPropHit(this->PropID))
      {
      propsRenderered += propArray[i]->RenderOpaqueGeometry(renderer);
      }
    }

  return propsRenderered;
}

//----------------------------------------------------------------------------
bool vtkHardwareSelector::IsPropHit(int id)
{
  return (this->Internals->HitProps.size() == 0 ||
    this->Internals->HitProps.find(id) != this->Internals->HitProps.end());
}

//----------------------------------------------------------------------------
bool vtkHardwareSelector::GetPixelInformation(unsigned int display_position[2],
  int& processid,
  vtkIdType& attrId, vtkProp*& prop,
  int maxDist)
{
  // Base case
  if (maxDist == 0)
    {
    if (display_position[0] < this->Area[0] || display_position[0] > this->Area[2] ||
      display_position[1] < this->Area[1] || display_position[1] > this->Area[3])
      {
      processid=-1;
      attrId=-1;
      prop = 0;
      return false;
      }

    int actorid = this->Convert(display_position[0], display_position[1], this->PixBuffer[ACTOR_PASS]);
    if (actorid <= 0)
      {
      // the pixel did not hit any actor.
      processid=-1;
      attrId=-1;
      prop = 0;
      return false;
      }
    actorid--;
    prop = this->Internals->Props[actorid];

    int low24 = this->Convert(display_position[0], display_position[1], this->PixBuffer[ID_LOW24]);
    int mid24 = this->Convert(display_position[0], display_position[1], this->PixBuffer[ID_MID24]);
    int high16 = this->Convert(display_position[0], display_position[1], this->PixBuffer[ID_HIGH16]);
    // id 0 is reserved for nothing present.
    attrId = this->GetID(low24, mid24, high16);
    attrId = (attrId - ID_OFFSET);
    if (attrId < 0)
      {
      // the pixel did not hit any cell.
      processid=-1;
      attrId=-1;
      prop = 0;
      return false;
      }

    processid = this->Convert(display_position[0], display_position[1], this->PixBuffer[PROCESS_PASS]);
    processid--;
    return true;
    }

  // Iterate over successively growing boxes.
  // They recursively call the base case to handle single pixels.
  int disp_pos[2] = {display_position[0], display_position[1]};
  unsigned int cur_pos[2] = {0, 0};
  for (int dist = 0; dist < maxDist; ++dist)
    {
    // Vertical sides of box.
    for (int y = disp_pos[1] - dist; y <= disp_pos[1] + dist; ++y)
      {
      cur_pos[0] = static_cast<unsigned int>(disp_pos[0] - dist);
      cur_pos[1] = static_cast<unsigned int>(y);
      if (this->GetPixelInformation(cur_pos, processid, attrId, prop, 0))
        {
        return true;
        }
      cur_pos[0] = static_cast<unsigned int>(disp_pos[0] + dist);
      if (this->GetPixelInformation(cur_pos, processid, attrId, prop, 0))
        {
        return true;
        }
      }
    // Horizontal sides of box.
    for (int x = disp_pos[0] - (dist-1); x <= disp_pos[0] + (dist-1); ++x)
      {
      cur_pos[0] = static_cast<unsigned int>(x);
      cur_pos[1] = static_cast<unsigned int>(disp_pos[1] - dist);
      if (this->GetPixelInformation(cur_pos, processid, attrId, prop, 0))
        {
        return true;
        }
      cur_pos[1] = static_cast<unsigned int>(disp_pos[1] + dist);
      if (this->GetPixelInformation(cur_pos, processid, attrId, prop, 0))
        {
        return true;
        }
      }
    }

  // nothing hit.
  processid=-1;
  attrId=-1;
  prop = 0;
  return false;
}

//----------------------------------------------------------------------------
vtkSelection* vtkHardwareSelector::GenerateSelection(
  unsigned int x1, unsigned int y1,
  unsigned int x2, unsigned int y2)
{
  // Clamp to saved region
  x1 = x1 < this->Area[0] ? this->Area[0] : x1;
  x1 = x1 > this->Area[2] ? this->Area[2] : x1;
  x2 = x2 < this->Area[0] ? this->Area[0] : x2;
  x2 = x2 > this->Area[2] ? this->Area[2] : x2;
  y1 = y1 < this->Area[1] ? this->Area[1] : y1;
  y1 = y1 > this->Area[3] ? this->Area[3] : y1;
  y2 = y2 < this->Area[1] ? this->Area[1] : y2;
  y2 = y2 > this->Area[3] ? this->Area[3] : y2;

  typedef vtkstd::map<int, vtkstd::map<int, vtkstd::set<vtkIdType> > > MapType;
  MapType dataMap;

  typedef vtkstd::map<int, vtkstd::map<int, vtkIdType> > PixelCountType;
  PixelCountType pixelCounts;

  for (int yy = y1 - Area[1]; yy <= static_cast<int>(y2 - this->Area[1]); yy++)
    {
    for (int xx = x1 - Area[0]; xx <= static_cast<int>(x2 - this->Area[0]); xx++)
      {
      int processid = this->Convert(xx, yy, this->PixBuffer[PROCESS_PASS]);
      processid--;

      int actorid = this->Convert(xx, yy, this->PixBuffer[ACTOR_PASS]);
      if (actorid <= 0)
        {
        // the pixel did not hit any actor.
        continue;
        }
      actorid--;

      int low24 = this->Convert(xx, yy, this->PixBuffer[ID_LOW24]); 
      int mid24 = this->Convert(xx, yy, this->PixBuffer[ID_MID24]);
      int high16 = this->Convert(xx, yy, this->PixBuffer[ID_HIGH16]);
      // id 0 is reserved for nothing present.
      vtkIdType elemid = this->GetID(low24, mid24, high16);
      elemid = (elemid - ID_OFFSET);
      if (elemid < 0)
        {
        continue;
        }
      dataMap[processid][actorid].insert(elemid);
      pixelCounts[processid][actorid]++;
      }
    }

  vtkSelection* sel = vtkSelection::New();

  MapType::iterator procIter;
  for (procIter = dataMap.begin(); procIter != dataMap.end(); ++procIter)
    {
    int processid = procIter->first;
    vtkstd::map<int, vtkstd::set<vtkIdType> >::iterator iter;
    for (iter = procIter->second.begin(); iter != procIter->second.end(); ++iter)
      {
      vtkSelectionNode* child = vtkSelectionNode::New();
      child->SetContentType(vtkSelectionNode::INDICES);
      switch (this->FieldAssociation)
        {
      case vtkDataObject::FIELD_ASSOCIATION_CELLS:
        child->SetFieldType(vtkSelectionNode::CELL);
        break;

      case vtkDataObject::FIELD_ASSOCIATION_POINTS:
        child->SetFieldType(vtkSelectionNode::POINT);
        break;
        }
      child->GetProperties()->Set(vtkSelectionNode::PROP_ID(), iter->first);
      child->GetProperties()->Set(vtkSelectionNode::PROP(),
        this->Internals->Props[iter->first]);
      child->GetProperties()->Set(vtkSelectionNode::PIXEL_COUNT(),
        pixelCounts[processid][iter->first]);
      if (processid >= 0)
        {
        child->GetProperties()->Set(vtkSelectionNode::PROCESS_ID(), processid);
        }

      vtkIdTypeArray* ids = vtkIdTypeArray::New();
      ids->SetName("SelectedIds");
      ids->SetNumberOfComponents(1);
      ids->SetNumberOfTuples(iter->second.size());
      vtkstd::set<vtkIdType>::iterator idIter;
      vtkIdType cc=0;
      for (idIter = iter->second.begin(); idIter != iter->second.end();
        ++idIter, ++cc)
        {
        ids->SetValue(cc, *idIter);
        }
      child->SetSelectionList(ids);
      ids->Delete();
      sel->AddNode(child);
      child->Delete();
      }
    }

  return sel;
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
  os << indent << "Area: " << this->Area[0] << ", " << this->Area[1] << ", "
    << this->Area[2] << ", " << this->Area[3] << endl;
  os << indent << "Renderer: " << this->Renderer << endl;

}

