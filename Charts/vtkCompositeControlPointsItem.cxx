/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeControlPointsItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkBrush.h"
#include "vtkCallbackCommand.h"
#include "vtkContext2D.h"
#include "vtkIdTypeArray.h"
#include "vtkColorTransferFunction.h"
#include "vtkCompositeControlPointsItem.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPoints2D.h"
#include "vtkContextScene.h"
#include "vtkPiecewisePointHandleItem.h"

// to handle mouse.GetButton
#include "vtkContextMouseEvent.h"

#include <cassert>
#include <limits>
#include <algorithm>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkCompositeControlPointsItem);

//-----------------------------------------------------------------------------
vtkCompositeControlPointsItem::vtkCompositeControlPointsItem()
{
  this->PointsFunction = ColorAndOpacityPointsFunction;
  this->OpacityFunction = 0;
  this->ColorFill = true;
  this->OpacityPointHandle = NULL;
  this->UseOpacityPointHandles = false;
}

//-----------------------------------------------------------------------------
vtkCompositeControlPointsItem::~vtkCompositeControlPointsItem()
{
  if (this->OpacityFunction)
    {
    this->OpacityFunction->Delete();
    this->OpacityFunction = 0;
    }
  if (this->OpacityPointHandle)
    {
    this->OpacityPointHandle->Delete();
    this->OpacityPointHandle = 0;
    }
}

//-----------------------------------------------------------------------------
void vtkCompositeControlPointsItem::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "OpacityFunction: ";
  if (this->OpacityFunction)
    {
    os << endl;
    this->OpacityFunction->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
  os << indent << "OpacityFunction: ";
  if (this->OpacityPointHandle)
    {
    os << endl;
    this->OpacityPointHandle->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
  os << indent << "UseOpacityPointHandles: " << this->UseOpacityPointHandles << endl;
}

//-----------------------------------------------------------------------------
void vtkCompositeControlPointsItem::emitEvent(unsigned long event, void* params)
{
  if (this->OpacityFunction)
    {
    this->OpacityFunction->InvokeEvent(event, params);
    }
  this->Superclass::emitEvent(event, params);
}

//-----------------------------------------------------------------------------
unsigned long int vtkCompositeControlPointsItem::GetControlPointsMTime()
{
  unsigned long int mTime = this->Superclass::GetControlPointsMTime();
  if (this->OpacityFunction)
    {
    mTime = std::max(mTime, this->OpacityFunction->GetMTime());
    }
  return mTime;
}

//-----------------------------------------------------------------------------
void vtkCompositeControlPointsItem::SetOpacityFunction(vtkPiecewiseFunction* function)
{
  if (function == this->OpacityFunction)
    {
    return;
    }
  if (this->OpacityFunction)
    {
    this->OpacityFunction->RemoveObserver(this->Callback);
    }
  vtkSetObjectBodyMacro(OpacityFunction, vtkPiecewiseFunction, function);
  if (this->PointsFunction == ColorAndOpacityPointsFunction)
    {
    this->SilentMergeTransferFunctions();
    }
  if (this->OpacityFunction)
    {
    this->OpacityFunction->AddObserver(vtkCommand::StartEvent, this->Callback);
    this->OpacityFunction->AddObserver(vtkCommand::ModifiedEvent, this->Callback);
    this->OpacityFunction->AddObserver(vtkCommand::EndEvent, this->Callback);
    }
  this->ResetBounds();
  this->ComputePoints();
}

//-----------------------------------------------------------------------------
void vtkCompositeControlPointsItem::SetColorTransferFunction(vtkColorTransferFunction* c)
{
  if (c == this->ColorTransferFunction)
    {
    return;
    }
  // We need to set the color transfer function here (before
  // Superclass::SetPiecewiseFunction) to be able to have a valid
  // color transfer function for MergeColorTransferFunction().
  vtkSetObjectBodyMacro(ColorTransferFunction, vtkColorTransferFunction, c);
  if (this->PointsFunction == ColorAndOpacityPointsFunction)
    {
    this->SilentMergeTransferFunctions();
    }
  this->Superclass::SetColorTransferFunction(c);
}

//-----------------------------------------------------------------------------
void vtkCompositeControlPointsItem::DrawPoint(vtkContext2D* painter, vtkIdType index)
{
  if (this->PointsFunction == ColorPointsFunction ||
      this->PointsFunction == ColorAndOpacityPointsFunction)
    {
    this->Superclass::DrawPoint(painter, index);
    return;
    }
  if (this->PointsFunction == OpacityPointsFunction &&
      this->ColorFill && this->ColorTransferFunction)
    {
    double xvms[4];
    this->OpacityFunction->GetNodeValue(index, xvms);
    unsigned char* rgb = this->ColorTransferFunction->MapValue(xvms[0]);
    painter->GetBrush()->SetColorF(
      rgb[0] / 255., rgb[1] / 255., rgb[2] / 255., 0.55);
    }
  this->vtkControlPointsItem::DrawPoint(painter, index);
}

//-----------------------------------------------------------------------------
vtkIdType vtkCompositeControlPointsItem::GetNumberOfPoints()const
{
  if (this->ColorTransferFunction &&
      (this->PointsFunction == ColorPointsFunction ||
       this->PointsFunction == ColorAndOpacityPointsFunction))
    {
    return this->Superclass::GetNumberOfPoints();
    }
  if (this->OpacityFunction &&
      (this->PointsFunction == OpacityPointsFunction ||
       this->PointsFunction == ColorAndOpacityPointsFunction))
    {
    return static_cast<vtkIdType>(this->OpacityFunction->GetSize());
    }
  return 0;
}

//-----------------------------------------------------------------------------
void vtkCompositeControlPointsItem::SetControlPoint(vtkIdType index, double* newPos)
{
  if (this->PointsFunction == ColorPointsFunction ||
      this->PointsFunction == ColorAndOpacityPointsFunction)
    {
    this->Superclass::SetControlPoint(index, newPos);
    }
  if (this->OpacityFunction &&
      (this->PointsFunction == OpacityPointsFunction ||
       this->PointsFunction == ColorAndOpacityPointsFunction))
    {
    this->OpacityFunction->SetNodeValue(index, newPos);
    }
}

//-----------------------------------------------------------------------------
void vtkCompositeControlPointsItem::GetControlPoint(vtkIdType index, double* pos)const
{
  if (!this->OpacityFunction ||
      this->PointsFunction == ColorPointsFunction)
    {
    this->Superclass::GetControlPoint(index, pos);
    if (this->OpacityFunction)
      {
      pos[1] = const_cast<vtkPiecewiseFunction*>(this->OpacityFunction)
        ->GetValue(pos[0]);
      }
    return;
    }
  const_cast<vtkPiecewiseFunction*>(this->OpacityFunction)
    ->GetNodeValue(index, pos);
}

//-----------------------------------------------------------------------------
void vtkCompositeControlPointsItem::EditPoint(float tX, float tY)
{
  if (this->PointsFunction == ColorPointsFunction ||
      this->PointsFunction == ColorAndOpacityPointsFunction)
    {
    this->Superclass::EditPoint(tX, tY);
    }
  if (this->OpacityFunction &&
      (this->PointsFunction == ColorPointsFunction ||
       this->PointsFunction == ColorAndOpacityPointsFunction))
    {
    double xvms[4];
    this->OpacityFunction->GetNodeValue(this->CurrentPoint, xvms);
    xvms[2] += tX;
    xvms[3] += tY;
    this->OpacityFunction->SetNodeValue(this->CurrentPoint, xvms);
    // Not sure why we move the point before too
    if (this->CurrentPoint > 0)
      {
      this->OpacityFunction->GetNodeValue(this->CurrentPoint - 1, xvms);
      xvms[2] += tX;
      xvms[3] += tY;
      this->OpacityFunction->SetNodeValue(this->CurrentPoint - 1, xvms);
      }
    }
}

//-----------------------------------------------------------------------------
vtkIdType vtkCompositeControlPointsItem::AddPoint(double* newPos)
{
  vtkIdType addedPoint = -1;
  this->StartChanges();
  if (this->OpacityFunction &&
      (this->PointsFunction == OpacityPointsFunction ||
       this->PointsFunction == ColorAndOpacityPointsFunction))
    {
    addedPoint = this->OpacityFunction->AddPoint(newPos[0], newPos[1]);
    if (this->PointsFunction == OpacityPointsFunction)
      {
      this->vtkControlPointsItem::AddPointId(addedPoint);
      }
    }
  if (this->PointsFunction == ColorPointsFunction ||
      this->PointsFunction == ColorAndOpacityPointsFunction)
    {
    addedPoint = this->Superclass::AddPoint(newPos);
    }
  this->EndChanges();
  return addedPoint;
}

//-----------------------------------------------------------------------------
vtkIdType vtkCompositeControlPointsItem::RemovePoint(double* currentPoint)
{
  vtkIdType removedPoint = -1;
  if (!this->IsPointRemovable(this->GetControlPointId(currentPoint)))
    {
    return removedPoint;
    }

  this->StartChanges();
  if (this->PointsFunction == ColorPointsFunction ||
      this->PointsFunction == ColorAndOpacityPointsFunction)
    {
    removedPoint = this->Superclass::RemovePoint(currentPoint);
    }
  if (this->OpacityFunction &&
      (this->PointsFunction == OpacityPointsFunction ||
       this->PointsFunction == ColorAndOpacityPointsFunction))
    {
    removedPoint = this->OpacityFunction->RemovePoint(currentPoint[0]);
    }
  this->EndChanges();
  return removedPoint;
}

//-----------------------------------------------------------------------------
void vtkCompositeControlPointsItem::MergeTransferFunctions()
{
  if (!this->ColorTransferFunction || !this->OpacityFunction)
    {
    return;
    }
  // Naive implementation that does the work but can be a bit slow
  // Copy OpacityFunction points into the ColorTransferFunction
  const int piecewiseFunctionCount = this->OpacityFunction->GetSize();
  for (int i = 0; i < piecewiseFunctionCount; ++i)
    {
    double piecewisePoint[4];
    this->OpacityFunction->GetNodeValue(i, piecewisePoint);
    double rgb[3];
    this->ColorTransferFunction->GetColor(piecewisePoint[0], rgb);
    // note that we might lose the midpoint/sharpness of the point if any
    this->ColorTransferFunction->RemovePoint(piecewisePoint[0]);
    this->ColorTransferFunction->AddRGBPoint(piecewisePoint[0], rgb[0], rgb[1], rgb[2], piecewisePoint[2], piecewisePoint[3]);
    }
  // Copy ColorTransferFunction points into the OpacityFunction
  const int colorFunctionCount = this->ColorTransferFunction->GetSize();
  for (int i = 0; i < colorFunctionCount; ++i)
    {
    double xrgbms[6];
    this->ColorTransferFunction->GetNodeValue(i, xrgbms);
    double value = this->OpacityFunction->GetValue(xrgbms[0]);
    // note that we might lose the midpoint/sharpness of the point if any
    this->OpacityFunction->RemovePoint(xrgbms[0]);
    this->OpacityFunction->AddPoint(xrgbms[0], value, xrgbms[4], xrgbms[5]);
    }
}

//-----------------------------------------------------------------------------
void vtkCompositeControlPointsItem::SilentMergeTransferFunctions()
{
  this->StartChanges();
  this->MergeTransferFunctions();
  this->EndChanges();
}

//-----------------------------------------------------------------------------
bool vtkCompositeControlPointsItem::MouseButtonPressEvent(const vtkContextMouseEvent &mouse)
{
  bool result=false;
  if(this->OpacityPointHandle && this->OpacityPointHandle->GetVisible())
    {
    result = this->OpacityPointHandle->MouseButtonPressEvent(mouse);
    }
  if(!result)
    {
    result = this->Superclass::MouseButtonPressEvent(mouse);
    if(result && this->OpacityPointHandle &&
     this->OpacityPointHandle->GetVisible() &&
     this->OpacityPointHandle->GetCurrentPointIndex() !=
     this->GetCurrentPoint())
      {
      this->OpacityPointHandle->SetVisible(false);
      }
    }
  return result;
}

//-----------------------------------------------------------------------------
bool vtkCompositeControlPointsItem::MouseDoubleClickEvent(const vtkContextMouseEvent &mouse)
{
   bool superRes = this->Superclass::MouseDoubleClickEvent(mouse);
   if(superRes)
     {
     vtkIdType curIdx = this->GetCurrentPoint();
     this->EditPointCurve(curIdx);
     }
   return superRes;
}

//-----------------------------------------------------------------------------
bool vtkCompositeControlPointsItem::MouseMoveEvent(const vtkContextMouseEvent &mouse)
{
  bool result=false;
  if(this->OpacityPointHandle && this->OpacityPointHandle->GetVisible())
    {
    result = this->OpacityPointHandle->MouseMoveEvent(mouse);
    }
  if(!result)
    {
    result = this->Superclass::MouseMoveEvent(mouse);
    }
  return result;
}

//-----------------------------------------------------------------------------
void vtkCompositeControlPointsItem::EditPointCurve(vtkIdType index)
{
  if(index<0 || index>=this->GetNumberOfPoints())
    {
    return;
    }
  if(this->GetUseOpacityPointHandles())
    {
    if(!this->OpacityPointHandle)
      {
      this->OpacityPointHandle = vtkPiecewisePointHandleItem::New();
      this->AddItem(this->OpacityPointHandle);
      this->OpacityPointHandle->SetPiecewiseFunction(
        this->GetOpacityFunction());
      }
    else
      {
      this->OpacityPointHandle->SetVisible(
        !this->OpacityPointHandle->GetVisible());
      this->GetScene()->SetDirty(true);
      }
    }
}
