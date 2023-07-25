// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkQWidgetWidget.h"

#include <QMouseEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsSceneMouseEvent>
#include <QtWidgets/QWidget>

#include "vtkCallbackCommand.h"
// #include "vtkCommand.h"
#include "vtkEvent.h"
#include "vtkEventData.h"
#include "vtkObjectFactory.h"
#include "vtkQWidgetRepresentation.h"
#include "vtkQWidgetTexture.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTimerLog.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"
#include "vtkWidgetEventTranslator.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkQWidgetWidget);

//------------------------------------------------------------------------------
vtkQWidgetWidget::vtkQWidgetWidget()
{
  this->Widget = nullptr;
  this->WidgetState = vtkQWidgetWidget::Start;

  {
    vtkNew<vtkEventDataDevice3D> ed;
    ed->SetDevice(vtkEventDataDevice::RightController);
    ed->SetInput(vtkEventDataDeviceInput::Any);
    ed->SetAction(vtkEventDataAction::Press);
    this->CallbackMapper->SetCallbackMethod(vtkCommand::Select3DEvent, ed, vtkWidgetEvent::Select3D,
      this, vtkQWidgetWidget::SelectAction3D);
  }

  {
    vtkNew<vtkEventDataDevice3D> ed;
    ed->SetDevice(vtkEventDataDevice::RightController);
    ed->SetInput(vtkEventDataDeviceInput::Any);
    ed->SetAction(vtkEventDataAction::Release);
    this->CallbackMapper->SetCallbackMethod(vtkCommand::Select3DEvent, ed,
      vtkWidgetEvent::EndSelect3D, this, vtkQWidgetWidget::EndSelectAction3D);
  }

  {
    vtkNew<vtkEventDataDevice3D> ed;
    ed->SetDevice(vtkEventDataDevice::RightController);
    ed->SetInput(vtkEventDataDeviceInput::Any);
    this->CallbackMapper->SetCallbackMethod(
      vtkCommand::Move3DEvent, ed, vtkWidgetEvent::Move3D, this, vtkQWidgetWidget::MoveAction3D);
  }

  // start off responding to all move events
  this->LastDevice = static_cast<int>(vtkEventDataDevice::Any);
}

//------------------------------------------------------------------------------
vtkQWidgetWidget::~vtkQWidgetWidget() = default;

vtkQWidgetRepresentation* vtkQWidgetWidget::GetQWidgetRepresentation()
{
  return vtkQWidgetRepresentation::SafeDownCast(this->WidgetRep);
}

void vtkQWidgetWidget::SetWidget(QWidget* w)
{
  if (this->Widget == w)
  {
    return;
  }
  this->Widget = w;

  if (this->GetQWidgetRepresentation())
  {
    this->GetQWidgetRepresentation()->SetWidget(this->Widget);
  }
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkQWidgetWidget::SelectAction3D(vtkAbstractWidget* w)
{
  vtkQWidgetWidget* self = reinterpret_cast<vtkQWidgetWidget*>(w);

  int interactionState = self->WidgetRep->ComputeComplexInteractionState(
    self->Interactor, self, vtkWidgetEvent::Select3D, self->CallData);

  if (interactionState == vtkQWidgetRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->WidgetState = vtkQWidgetWidget::Active;
  float widgetCoords[2];
  vtkQWidgetRepresentation* wrep = self->GetQWidgetRepresentation();
  wrep->GetWidgetCoordinates(widgetCoords);

  // if we are not mapped yet return
  QGraphicsScene* scene = wrep->GetQWidgetTexture()->GetScene();
  if (!scene)
  {
    return;
  }

  vtkEventData* edata = static_cast<vtkEventData*>(self->CallData);
  vtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
  if (!edd || self->LastDevice != static_cast<int>(vtkEventDataDevice::Any))
  {
    return;
  }

  self->LastDevice = static_cast<int>(edd->GetDevice());

  QPointF mousePos(widgetCoords[0], widgetCoords[1]);

  // we store the starting location and time because clicking with a
  // controller can be tricky as people's hands shake. This can make
  // what was intended to be a click turn into a drag select.
  // To mitigate this unwanted behavior we look at the elapsed time
  // of the click and if it is fast enough we set the position of the
  // movement and end events to be the same as the start.
  self->SteadyWidgetCoordinates = mousePos;
  self->SelectStartTime = vtkTimerLog::GetUniversalTime();

  Qt::MouseButton button = Qt::LeftButton;
  QPoint ptGlobal = mousePos.toPoint();
  QGraphicsSceneMouseEvent mouseEvent(QEvent::GraphicsSceneMousePress);
  mouseEvent.setWidget(nullptr);
  mouseEvent.setPos(mousePos);
  mouseEvent.setButtonDownPos(button, mousePos);
  mouseEvent.setButtonDownScenePos(button, ptGlobal);
  mouseEvent.setButtonDownScreenPos(button, ptGlobal);
  mouseEvent.setScenePos(ptGlobal);
  mouseEvent.setScreenPos(ptGlobal);
  mouseEvent.setLastPos(self->LastWidgetCoordinates);
  mouseEvent.setLastScenePos(ptGlobal);
  mouseEvent.setLastScreenPos(ptGlobal);
  mouseEvent.setButtons(button);
  mouseEvent.setButton(button);
  mouseEvent.setModifiers({});
  mouseEvent.setAccepted(false);

  QApplication::sendEvent(scene, &mouseEvent);

  self->LastWidgetCoordinates = mousePos;

  self->EventCallbackCommand->SetAbortFlag(1);

  // fire a mouse click with the correct coords
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
}

//------------------------------------------------------------------------------
void vtkQWidgetWidget::MoveAction3D(vtkAbstractWidget* w)
{
  vtkQWidgetWidget* self = reinterpret_cast<vtkQWidgetWidget*>(w);

  vtkEventData* edata = static_cast<vtkEventData*>(self->CallData);
  vtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
  if (!edd)
  {
    return;
  }

  if (self->LastDevice != static_cast<int>(edd->GetDevice()) &&
    self->LastDevice != static_cast<int>(vtkEventDataDevice::Any))
  {
    return;
  }

  int interactionState = self->WidgetRep->ComputeComplexInteractionState(
    self->Interactor, self, vtkWidgetEvent::Select3D, self->CallData);

  if (interactionState == vtkQWidgetRepresentation::Outside)
  {
    return;
  }

  float widgetCoords[2];
  vtkQWidgetRepresentation* wrep = self->GetQWidgetRepresentation();
  wrep->GetWidgetCoordinates(widgetCoords);

  // if we are not mapped yet return
  QGraphicsScene* scene = wrep->GetQWidgetTexture()->GetScene();
  if (!scene)
  {
    return;
  }

  QPointF mousePos(widgetCoords[0], widgetCoords[1]);
  double elapsedTime = vtkTimerLog::GetUniversalTime() - self->SelectStartTime;
  if (elapsedTime < 1.0)
  {
    mousePos = self->SteadyWidgetCoordinates;
  }

  QPoint ptGlobal = mousePos.toPoint();
  QGraphicsSceneMouseEvent mouseEvent(QEvent::GraphicsSceneMouseMove);
  mouseEvent.setWidget(nullptr);
  mouseEvent.setPos(mousePos);
  mouseEvent.setScenePos(ptGlobal);
  mouseEvent.setScreenPos(ptGlobal);
  mouseEvent.setLastPos(self->LastWidgetCoordinates);
  mouseEvent.setLastScenePos(ptGlobal);
  mouseEvent.setLastScreenPos(ptGlobal);
  mouseEvent.setButtons(
    self->WidgetState == vtkQWidgetWidget::Active ? Qt::LeftButton : Qt::NoButton);
  mouseEvent.setButton(Qt::NoButton);
  mouseEvent.setModifiers({});
  mouseEvent.setAccepted(false);

  QApplication::sendEvent(scene, &mouseEvent);

  self->LastWidgetCoordinates = mousePos;

  self->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
}

//------------------------------------------------------------------------------
void vtkQWidgetWidget::EndSelectAction3D(vtkAbstractWidget* w)
{
  vtkQWidgetWidget* self = reinterpret_cast<vtkQWidgetWidget*>(w);

  if (self->WidgetState != vtkQWidgetWidget::Active ||
    self->WidgetRep->GetInteractionState() == vtkQWidgetRepresentation::Outside)
  {
    return;
  }

  vtkEventData* edata = static_cast<vtkEventData*>(self->CallData);
  vtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
  if (self->LastDevice != static_cast<int>(edd->GetDevice()))
  {
    return;
  }

  // reset back to responding to all move events
  self->LastDevice = static_cast<int>(vtkEventDataDevice::Any);

  self->WidgetRep->ComputeComplexInteractionState(
    self->Interactor, self, vtkWidgetEvent::Select3D, self->CallData);

  // We are definitely selected
  float widgetCoords[2];
  vtkQWidgetRepresentation* wrep = self->GetQWidgetRepresentation();
  wrep->GetWidgetCoordinates(widgetCoords);

  // if we are not mapped yet return
  QGraphicsScene* scene = wrep->GetQWidgetTexture()->GetScene();
  if (!scene)
  {
    return;
  }

  QPointF mousePos(widgetCoords[0], widgetCoords[1]);
  double elapsedTime = vtkTimerLog::GetUniversalTime() - self->SelectStartTime;
  if (elapsedTime < 1.0)
  {
    mousePos = self->SteadyWidgetCoordinates;
  }

  Qt::MouseButton button = Qt::LeftButton;
  QPoint ptGlobal = mousePos.toPoint();
  QGraphicsSceneMouseEvent mouseEvent(QEvent::GraphicsSceneMouseRelease);
  mouseEvent.setWidget(nullptr);
  mouseEvent.setPos(mousePos);
  mouseEvent.setButtonDownPos(button, mousePos);
  mouseEvent.setButtonDownScenePos(button, ptGlobal);
  mouseEvent.setButtonDownScreenPos(button, ptGlobal);
  mouseEvent.setScenePos(ptGlobal);
  mouseEvent.setScreenPos(ptGlobal);
  mouseEvent.setLastPos(self->LastWidgetCoordinates);
  mouseEvent.setLastScenePos(ptGlobal);
  mouseEvent.setLastScreenPos(ptGlobal);
  mouseEvent.setButtons(Qt::NoButton);
  mouseEvent.setButton(button);
  mouseEvent.setModifiers({});
  mouseEvent.setAccepted(false);

  QApplication::sendEvent(scene, &mouseEvent);

  self->LastWidgetCoordinates = mousePos;

  // Return state to not selected
  self->WidgetState = vtkQWidgetWidget::Start;
  if (!self->Parent)
  {
    self->ReleaseFocus();
  }

  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(vtkCommand::EndInteractionEvent, nullptr);
}

//------------------------------------------------------------------------------
void vtkQWidgetWidget::SetEnabled(int enabling)
{
  if (this->Enabled == enabling)
  {
    return;
  }

  if (enabling)
  {
    this->Widget->repaint();
  }
  Superclass::SetEnabled(enabling);
}

//------------------------------------------------------------------------------
void vtkQWidgetWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = vtkQWidgetRepresentation::New();
    this->GetQWidgetRepresentation()->SetWidget(this->Widget);
  }
}

//------------------------------------------------------------------------------
void vtkQWidgetWidget::SetRepresentation(vtkQWidgetRepresentation* rep)
{
  this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(rep));
  rep->SetWidget(this->Widget);
}

//------------------------------------------------------------------------------
void vtkQWidgetWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
