/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQWidgetWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
//#include "vtkStdString.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"
#include "vtkWidgetEventTranslator.h"

vtkStandardNewMacro(vtkQWidgetWidget);

//----------------------------------------------------------------------------
vtkQWidgetWidget::vtkQWidgetWidget()
{
  this->Widget = nullptr;
  this->WidgetState = vtkQWidgetWidget::Start;

  {
    vtkNew<vtkEventDataButton3D> ed;
    ed->SetDevice(vtkEventDataDevice::RightController);
    ed->SetInput(vtkEventDataDeviceInput::Trigger);
    ed->SetAction(vtkEventDataAction::Press);
    this->CallbackMapper->SetCallbackMethod(vtkCommand::Button3DEvent, ed, vtkWidgetEvent::Select3D,
      this, vtkQWidgetWidget::SelectAction3D);
  }

  {
    vtkNew<vtkEventDataButton3D> ed;
    ed->SetDevice(vtkEventDataDevice::RightController);
    ed->SetInput(vtkEventDataDeviceInput::Trigger);
    ed->SetAction(vtkEventDataAction::Release);
    this->CallbackMapper->SetCallbackMethod(vtkCommand::Button3DEvent, ed,
      vtkWidgetEvent::EndSelect3D, this, vtkQWidgetWidget::EndSelectAction3D);
  }

  {
    vtkNew<vtkEventDataMove3D> ed;
    ed->SetDevice(vtkEventDataDevice::RightController);
    this->CallbackMapper->SetCallbackMethod(
      vtkCommand::Move3DEvent, ed, vtkWidgetEvent::Move3D, this, vtkQWidgetWidget::MoveAction3D);
  }
}

//----------------------------------------------------------------------------
vtkQWidgetWidget::~vtkQWidgetWidget() {}

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

//-------------------------------------------------------------------------
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
  int widgetCoords[2];
  vtkQWidgetRepresentation* wrep = self->GetQWidgetRepresentation();
  wrep->GetWidgetCoordinates(widgetCoords);

  // if we are not mapped yet return
  QGraphicsScene* scene = wrep->GetQWidgetTexture()->GetScene();
  if (!scene)
  {
    return;
  }

  QPointF mousePos(widgetCoords[0], widgetCoords[1]);
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
  mouseEvent.setModifiers(0);
  mouseEvent.setAccepted(false);

  QApplication::sendEvent(scene, &mouseEvent);

  self->LastWidgetCoordinates = mousePos;

  self->EventCallbackCommand->SetAbortFlag(1);

  // fire a mouse click with the correct coords
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
}

//----------------------------------------------------------------------
void vtkQWidgetWidget::MoveAction3D(vtkAbstractWidget* w)
{
  vtkQWidgetWidget* self = reinterpret_cast<vtkQWidgetWidget*>(w);

  int interactionState = self->WidgetRep->ComputeComplexInteractionState(
    self->Interactor, self, vtkWidgetEvent::Select3D, self->CallData);

  if (interactionState == vtkQWidgetRepresentation::Outside)
  {
    return;
  }

  int widgetCoords[2];
  vtkQWidgetRepresentation* wrep = self->GetQWidgetRepresentation();
  wrep->GetWidgetCoordinates(widgetCoords);

  // if we are not mapped yet return
  QGraphicsScene* scene = wrep->GetQWidgetTexture()->GetScene();
  if (!scene)
  {
    return;
  }

  QPointF mousePos(widgetCoords[0], widgetCoords[1]);
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
  mouseEvent.setModifiers(0);
  mouseEvent.setAccepted(false);

  QApplication::sendEvent(scene, &mouseEvent);
  // OnSceneChanged( QList<QRectF>() );

  self->LastWidgetCoordinates = mousePos;

  self->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
}

//----------------------------------------------------------------------
void vtkQWidgetWidget::EndSelectAction3D(vtkAbstractWidget* w)
{
  vtkQWidgetWidget* self = reinterpret_cast<vtkQWidgetWidget*>(w);

  if (self->WidgetState != vtkQWidgetWidget::Active ||
    self->WidgetRep->GetInteractionState() == vtkQWidgetRepresentation::Outside)
  {
    return;
  }

  self->WidgetRep->ComputeComplexInteractionState(
    self->Interactor, self, vtkWidgetEvent::Select3D, self->CallData);

  // We are definitely selected
  int widgetCoords[2];
  vtkQWidgetRepresentation* wrep = self->GetQWidgetRepresentation();
  wrep->GetWidgetCoordinates(widgetCoords);

  // if we are not mapped yet return
  QGraphicsScene* scene = wrep->GetQWidgetTexture()->GetScene();
  if (!scene)
  {
    return;
  }

  QPointF mousePos(widgetCoords[0], widgetCoords[1]);
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
  mouseEvent.setModifiers(0);
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

//----------------------------------------------------------------------
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

//----------------------------------------------------------------------
void vtkQWidgetWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = vtkQWidgetRepresentation::New();
    this->GetQWidgetRepresentation()->SetWidget(this->Widget);
  }
}

//----------------------------------------------------------------------
void vtkQWidgetWidget::SetRepresentation(vtkQWidgetRepresentation* rep)
{
  this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(rep));
  rep->SetWidget(this->Widget);
}

//----------------------------------------------------------------------------
void vtkQWidgetWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
