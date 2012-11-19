/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPiecewisePointHandleItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPiecewisePointHandleItem.h"

#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkContext2D.h"
#include "vtkContextScene.h"
#include "vtkContextMouseEvent.h"
#include "vtkPen.h"
#include "vtkBrush.h"
#include "vtkTextProperty.h"
#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"
#include "vtkControlPointsItem.h"
#include "vtkTransform2D.h"
#include "vtkNew.h"

enum enumPointHandleType
  {
  enSharpNess=0,
  enMidPoint
  };

class PointHandle
{
public:
  void Init(float x, float y, vtkIdType idx,
    enumPointHandleType type, float val, float distance,
    double sceneOrigin[2])
    {
    this->Position[0]=x;
    this->Position[1]=y;
    this->PointIndex = idx;
    this->enType = type;
    this->fValue = val;
    this->fDistance = distance;
    this->ScenePos[0]=sceneOrigin[0]+x;
    this->ScenePos[1]=sceneOrigin[1]+y;
    };
  void DrawCircle(vtkContext2D* painter, float radius)
    {
    painter->DrawArc(this->Position[0], this->Position[1], radius, 0.f, 360.f);
    };
  float Position[2];
  float ScenePos[2];
  vtkIdType PointIndex;
  enumPointHandleType enType;
  float fValue;
  float fDistance;
};

class vtkPiecewisePointHandleItem::InternalPiecewisePointHandleInfo
{
public:
  PointHandle PointHandles[4];
};
//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPiecewisePointHandleItem);

//-----------------------------------------------------------------------------
vtkPiecewisePointHandleItem::vtkPiecewisePointHandleItem()
{
  this->MouseOverHandleIndex = -1;
  this->PiecewiseFunction = NULL;
  this->Callback = vtkCallbackCommand::New();
  this->Callback->SetClientData(this);
  this->Callback->SetCallback(
    vtkPiecewisePointHandleItem::CallRedraw);
  this->HandleRadius = 3.f;
  this->CurrentPointIndex = -1;
  this->Internal = new InternalPiecewisePointHandleInfo();
}

//-----------------------------------------------------------------------------
vtkPiecewisePointHandleItem::~vtkPiecewisePointHandleItem()
{
  this->SetPiecewiseFunction(0);
  if (this->Callback)
    {
    this->Callback->Delete();
    this->Callback = 0;
    }
  delete this->Internal;
}
// ----------------------------------------------------------------------------
void vtkPiecewisePointHandleItem::SetParent(vtkAbstractContextItem* parent)
{
  if(this->Parent == parent)
    {
    return;
    }
  else if(this->Parent)
    {
    if (this->PiecewiseFunction)
      {
      this->Parent->RemoveObserver(this->Callback);
      }

    }
  this->Superclass::SetParent(parent);
  if(parent)
    {
    this->Parent->AddObserver(vtkControlPointsItem::CurrentPointChangedEvent,
       this->Callback);
    }
}

//-----------------------------------------------------------------------------
bool vtkPiecewisePointHandleItem::Paint(vtkContext2D *painter)
{
  vtkControlPointsItem* parentControl=vtkControlPointsItem::SafeDownCast(
    this->GetParent());
  if(!parentControl || parentControl->GetCurrentPoint()<0 ||
     !this->GetPiecewiseFunction())
    {
    this->CurrentPointIndex = -1;
    return true;
    }
  vtkIdType currentIdx = parentControl->GetCurrentPoint();
  this->CurrentPointIndex = currentIdx;
  double point[4];
  parentControl->GetControlPoint(parentControl->GetCurrentPoint(), point);
  unsigned char brushOpacity = painter->GetBrush()->GetOpacity();
  unsigned char penColor[3];
  painter->GetPen()->GetColor(penColor);
  unsigned char penOpacity = painter->GetPen()->GetOpacity();

  double pointInScene[2];
  vtkTransform2D* sceneTransform = painter->GetTransform();
  sceneTransform->TransformPoints(point, pointInScene, 1);

  vtkNew<vtkTransform2D> translation;
  translation->Translate(pointInScene[0], pointInScene[1]);

  painter->PushMatrix();
  painter->SetTransform(translation.GetPointer());
  painter->GetPen()->SetColor(0, 200, 0);

  float radius = this->HandleRadius;
  vtkIdType preIdx = currentIdx - 1;
  vtkIdType nxtIdx = currentIdx + 1;
  double preMid=0.0, preSharp=0.0, curMid=point[2], curSharp=point[3];
  double prePoint[4], nxtPoint[4];
  if(preIdx>=0)
    {
    this->PiecewiseFunction->GetNodeValue(preIdx,prePoint);
    preMid=prePoint[2]; preSharp=prePoint[3];
    }
  if(nxtIdx<parentControl->GetNumberOfPoints())
    {
    this->PiecewiseFunction->GetNodeValue(nxtIdx,nxtPoint);
    preMid=prePoint[2]; preSharp=prePoint[3];
    }

  // The following calculations are to find out the correct
  // handle positions to draw. The handle positions are relative
  // to the point position and they are in scene units.
  // The distance from the current point to previous and next points
  // are also cached, so that it will be convenient to convert
  // mouse movement to corresponding midpoint/sharpness changes.

  float ptRadius = parentControl->GetScreenPointRadius();
  float fDistance=this->HandleRadius+ptRadius;

  double blPos[2]={prePoint[0],prePoint[1]};
  double trPos[2]={nxtPoint[0],nxtPoint[1]};
  double screenBLPos[2], screenTRPos[2];
  sceneTransform->TransformPoints(blPos, screenBLPos, 1);
  sceneTransform->TransformPoints(trPos, screenTRPos, 1);
  double blxdistance = fabs(pointInScene[0]-screenBLPos[0])-fDistance*2.0;
  double blydistance = fabs(pointInScene[1]-screenBLPos[1])-fDistance*2.0;
  double trxdistance = fabs(pointInScene[0]-screenTRPos[0])-fDistance*2.0;
  double trydistance = fabs(pointInScene[1]-screenTRPos[1])-fDistance*2.0;

  blxdistance = std::max(0.0, blxdistance);
  blydistance = std::max(0.0, blydistance);
  trxdistance = std::max(0.0, trxdistance);
  trydistance = std::max(0.0, trydistance);

  this->Internal->PointHandles[0].Init(0.f, fDistance+trydistance*curSharp,
    currentIdx,enSharpNess, curSharp, trydistance, pointInScene);
  this->Internal->PointHandles[1].Init(fDistance+trxdistance*curMid, 0.f,
    currentIdx,enMidPoint, curMid, trxdistance, pointInScene);
  this->Internal->PointHandles[2].Init(0.f, -(fDistance+blydistance*preSharp),
    preIdx,enSharpNess, preSharp, blydistance, pointInScene);
  this->Internal->PointHandles[3].Init(-(fDistance+blxdistance*(1-preMid)), 0.f,
    preIdx,enMidPoint, preMid, blxdistance, pointInScene);

  painter->DrawLine(0, ptRadius+trydistance*curSharp, 0, ptRadius);
  painter->DrawLine(ptRadius, 0, ptRadius+trxdistance*curMid, 0);
  painter->DrawLine(0, -(ptRadius+blydistance*preSharp), 0, -ptRadius);
  painter->DrawLine(-(ptRadius+blxdistance*(1-preMid)), 0, -ptRadius, 0);

  for(int i=0; i<4; i++)
    {
    if (i==this->MouseOverHandleIndex)
      {
      painter->GetBrush()->SetColor(255, 0, 255);
      }
    else
      {
      painter->GetBrush()->SetColor(0, 200, 0);
      }
    this->Internal->PointHandles[i].DrawCircle(painter, radius);
    }

  painter->PopMatrix();
  painter->GetPen()->SetColor(penColor);
  painter->GetPen()->SetOpacity(penOpacity);
  painter->GetBrush()->SetOpacity(brushOpacity);

  this->PaintChildren(painter);
  return true;
}

//-----------------------------------------------------------------------------
bool vtkPiecewisePointHandleItem::Hit(const vtkContextMouseEvent &mouse)
{
  float pos[2] = { mouse.GetScenePos().GetX(), mouse.GetScenePos().GetY() };
  if (this->IsOverHandle(pos) >= 0)
    {
    return true;
    }
  return false;
}

//-----------------------------------------------------------------------------
int vtkPiecewisePointHandleItem::IsOverHandle(
  float* scenePos)
{
  vtkControlPointsItem* parentControl=vtkControlPointsItem::SafeDownCast(
    this->GetParent());
  if(!parentControl || parentControl->GetCurrentPoint()<0 ||
     !this->GetPiecewiseFunction() || !this->Scene->GetLastPainter())
    {
    return -1;
    }

  // we have four screen handles to check
  for(int i=0; i<4; ++i)
    {
    double sceneHandlePoint[2]={
      this->Internal->PointHandles[i].ScenePos[0],
      this->Internal->PointHandles[i].ScenePos[1]};
    double distance2 =
      (sceneHandlePoint[0] - scenePos[0]) *
        (sceneHandlePoint[0] - scenePos[0]) +
      (sceneHandlePoint[1] - scenePos[1]) *
        (sceneHandlePoint[1] - scenePos[1]);
    double tolerance = 1.5;
    double radius2 = this->HandleRadius * this->HandleRadius
      * tolerance * tolerance;
    if(distance2 <= radius2)
      {
      return i;
      }
    }

  return -1;
}

//-----------------------------------------------------------------------------
bool vtkPiecewisePointHandleItem::MouseMoveEvent(const vtkContextMouseEvent &mouse)
{
  if (mouse.GetButton() == vtkContextMouseEvent::LEFT_BUTTON)
    {
    if(this->MouseOverHandleIndex >= 0)
      {
      PointHandle* activeHandle =
        &this->Internal->PointHandles[this->MouseOverHandleIndex];
      float deltaX = mouse.GetScenePos().GetX() - activeHandle->ScenePos[0];
      float deltaY = mouse.GetScenePos().GetY() - activeHandle->ScenePos[1];

      vtkControlPointsItem* parentControl = vtkControlPointsItem::SafeDownCast(
        this->GetParent());
      if(activeHandle->fDistance<=0 || !parentControl ||
        parentControl->GetCurrentPoint()<0 || !this->GetPiecewiseFunction())
        {
        return false;
        }
      vtkIdType curIdx = activeHandle->PointIndex;
      double point[4];
      this->PiecewiseFunction->GetNodeValue(curIdx, point);
      if(activeHandle->enType==enMidPoint)
        {
        double fMid=deltaX/activeHandle->fDistance+activeHandle->fValue;
        fMid = std::max(fMid, 0.0);
        fMid = std::min(fMid, 1.0);
        point[2]=fMid;
        }
      else
        {
        if(this->MouseOverHandleIndex==2)
          {
          deltaY = -deltaY;
          }
        double fSharp=deltaY/activeHandle->fDistance+activeHandle->fValue;
        fSharp = std::max(fSharp, 0.0);
        fSharp = std::min(fSharp, 1.0);
        point[3]=fSharp;
        }
      this->GetPiecewiseFunction()->SetNodeValue(curIdx, point);
      return true;
      }
    }
  else if (mouse.GetButton() == vtkContextMouseEvent::NO_BUTTON)
    {
    float mspos[2] = { mouse.GetScenePos().GetX(), mouse.GetScenePos().GetY() };
    int handleIdx = this->IsOverHandle(mspos);
    if (this->MouseOverHandleIndex != handleIdx)
      {
      this->MouseOverHandleIndex = handleIdx;
      this->GetScene()->SetDirty(true);
      return true;
      }
    }

  return false;
}

//-----------------------------------------------------------------------------
bool vtkPiecewisePointHandleItem::MouseButtonPressEvent(const vtkContextMouseEvent &)
{
  if(this->MouseOverHandleIndex>=0)
    {
    return true;
    }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkPiecewisePointHandleItem::MouseButtonReleaseEvent(const vtkContextMouseEvent &)
{
  if(this->MouseOverHandleIndex>=0)
    {
    this->MouseOverHandleIndex = -1;
    this->GetScene()->SetDirty(true);
    return true;
    }
  return false;
}

//-----------------------------------------------------------------------------
vtkWeakPointer<vtkPiecewiseFunction> vtkPiecewisePointHandleItem::
  GetPiecewiseFunction()
{
  return this->PiecewiseFunction;
}

//-----------------------------------------------------------------------------
void vtkPiecewisePointHandleItem::SetPiecewiseFunction(
  vtkPiecewiseFunction* function)
{
  if (function == this->PiecewiseFunction)
    {
    return;
    }
  if (this->PiecewiseFunction)
    {
    this->PiecewiseFunction->RemoveObserver(this->Callback);
    }
  this->PiecewiseFunction = function;
  if (this->PiecewiseFunction)
    {
    this->PiecewiseFunction->AddObserver(vtkCommand::ModifiedEvent, this->Callback);
    this->PiecewiseFunction->AddObserver(vtkCommand::EndEvent, this->Callback);
    }
  this->Redraw();
}
//-----------------------------------------------------------------------------
void vtkPiecewisePointHandleItem::Redraw()
{
  if (this->Scene)
    {
    this->Scene->SetDirty(true);
    }
}
//-----------------------------------------------------------------------------
void vtkPiecewisePointHandleItem::CallRedraw(
  vtkObject* vtkNotUsed(sender), unsigned long event,
  void* receiver, void* vtkNotUsed(params))
{
  vtkPiecewisePointHandleItem* item =
    reinterpret_cast<vtkPiecewisePointHandleItem*>(receiver);
  switch(event)
    {
    case vtkCommand::ModifiedEvent:
    case vtkCommand::EndEvent:
    case vtkControlPointsItem::CurrentPointChangedEvent:
      item->Redraw();
      break;
    default:
      break;
    }
}

//-----------------------------------------------------------------------------
void vtkPiecewisePointHandleItem::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PiecewiseFunction: ";
  if (this->PiecewiseFunction)
    {
    os << endl;
    this->PiecewiseFunction->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
  os << indent << "MouseOverHandleIndex: " << this->MouseOverHandleIndex << endl;
  os << indent << "CurrentPointIndex: " << this->CurrentPointIndex << endl;
}
