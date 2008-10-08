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
#include "vtkSelection.h"
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
vtkCxxRevisionMacro(vtkHardwareSelector, "1.1");
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
    return false;
    }

  rwin->SwapBuffersOff();

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
    //cout << "Before Pass: " << this->CurrentPass << endl;
    //glFinish();
    rwin->Render();
    //cout << "Rendered Pass: " << this->CurrentPass << endl;
    //glFinish();
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
      return (this->MaxAttributeId >= 0xffffffffffff);
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
  vtkIdType& attrId, vtkProp*& prop)
{
  if (display_position[0] < this->Area[0] || display_position[0] > this->Area[2] ||
    display_position[1] < this->Area[1] || display_position[1] > this->Area[3])
    {
    vtkErrorMacro("Position out of selected region.");
    processid=-1;
    attrId=-1;
    prop = 0;
    return false;
    }

  int width = this->Area[2] - this->Area[0] + 1;
  unsigned long offset =
    ((display_position[1] - this->Area[1]) * width + display_position[0]-this->Area[0]);

  processid = this->Convert(offset, this->PixBuffer[PROCESS_PASS]);
  processid--;

  int actorid = this->Convert(offset, this->PixBuffer[ACTOR_PASS]);
  if (actorid <= 0)
    {
    processid=-1;
    attrId=-1;
    prop = 0;
    // nothing hit.
    return false;
    }
  actorid--;

  int low24 = this->Convert(offset, this->PixBuffer[ID_LOW24]); 
  int mid24 = this->Convert(offset, this->PixBuffer[ID_MID24]);
  int high16 = this->Convert(offset, this->PixBuffer[ID_HIGH16]);
  // id 0 is reserved for nothing present.
  attrId = this->GetID(low24, mid24, high16);
  attrId -= ID_OFFSET;
  if (attrId < 0)
    {
    processid=-1;
    attrId=-1;
    prop = 0;
    // nothing hit.
    return false;
    }

  prop = this->Internals->Props[actorid];
  return true;
}

//----------------------------------------------------------------------------
vtkSelection* vtkHardwareSelector::GenerateSelection()
{
  typedef vtkstd::map<int, vtkstd::map<int, vtkstd::set<vtkIdType> > > MapType;
  MapType dataMap;

  typedef vtkstd::map<int, vtkstd::map<int, vtkIdType> > PixelCountType;
  PixelCountType pixelCounts;

  for (int yy=0; yy <= static_cast<int>(this->Area[3]-this->Area[1]); yy++)
    {
    for (int xx=0; xx <= static_cast<int>(this->Area[2]-this->Area[0]); xx++)
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
      vtkIdType id = this->GetID(low24, mid24, high16);
      id -= ID_OFFSET;
      if (id < 0)
        {
        continue;
        }
      dataMap[processid][actorid].insert(id);
      pixelCounts[processid][actorid]++;
      }
    }

  vtkSelection* sel = vtkSelection::New();
  sel->SetContentType(vtkSelection::SELECTIONS);

  MapType::iterator procIter;
  for (procIter = dataMap.begin(); procIter != dataMap.end(); ++procIter)
    {
    int processid = procIter->first;
    vtkstd::map<int, vtkstd::set<vtkIdType> >::iterator iter;
    for (iter = procIter->second.begin(); iter != procIter->second.end(); ++iter)
      {
      vtkSelection* child = vtkSelection::New();
      child->SetContentType(vtkSelection::INDICES);
      switch (this->FieldAssociation)
        {
      case vtkDataObject::FIELD_ASSOCIATION_CELLS:
        child->SetFieldType(vtkSelection::CELL);
        break;

      case vtkDataObject::FIELD_ASSOCIATION_POINTS:
        child->SetFieldType(vtkSelection::POINT);
        break;
        }
      child->GetProperties()->Set(vtkSelection::PROP_ID(), iter->first);
      child->GetProperties()->Set(vtkSelection::PROP(),
        this->Internals->Props[iter->first]);
      child->GetProperties()->Set(vtkSelection::PIXEL_COUNT(),
        pixelCounts[processid][iter->first]);
      if (processid >= 0)
        {
        child->GetProperties()->Set(vtkSelection::PROCESS_ID(), processid);
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
      sel->AddChild(child);
      child->Delete();
      }
    }

  return sel;
}

//----------------------------------------------------------------------------
void vtkHardwareSelector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

