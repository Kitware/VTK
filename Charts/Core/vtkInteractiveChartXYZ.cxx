/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractiveChartXYZ.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkInteractiveChartXYZ.h"

#include "vtkAnnotationLink.h"
#include "vtkAxis.h"
#include "vtkCommand.h"
#include "vtkContext2D.h"
#include "vtkContext3D.h"
#include "vtkContextKeyEvent.h"
#include "vtkContextMouseEvent.h"
#include "vtkContextScene.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkPoints2D.h"
#include "vtkTable.h"
#include "vtkTextProperty.h"
#include "vtkTransform.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

#include "vtkPen.h"
#include "vtkAnnotationLink.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"

#include "vtkObjectFactory.h"

#include <vector>
#include <cassert>

using std::vector;

vtkStandardNewMacro(vtkInteractiveChartXYZ)

void vtkInteractiveChartXYZ::PrintSelf(ostream &os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}

void vtkInteractiveChartXYZ::Update()
{
  if (this->Link)
    {
    // Copy the row numbers so that we can do the highlight...
    if (!this->points.empty())
      {
      vtkSelection *selection =
          vtkSelection::SafeDownCast(this->Link->GetOutputDataObject(2));
      if (selection->GetNumberOfNodes())
        {
        vtkSelectionNode *node = selection->GetNode(0);
        vtkIdTypeArray *idArray =
            vtkIdTypeArray::SafeDownCast(node->GetSelectionList());
        if (this->selectedPointsBuidTime > idArray->GetMTime() ||
            this->GetMTime() > this->selectedPointsBuidTime)
          {
          this->selectedPoints.resize(idArray->GetNumberOfTuples());
          for (vtkIdType i = 0; i < idArray->GetNumberOfTuples(); ++i)
            {
            this->selectedPoints[i] = this->points[idArray->GetValue(i)];
            }
          this->selectedPointsBuidTime.Modified();
          }
        }
      }
    }
}

bool vtkInteractiveChartXYZ::Paint(vtkContext2D *painter)
{
  if (!this->Visible || this->points.size() == 0)
    return false;

  // Get the 3D context.
  vtkContext3D *context = painter->GetContext3D();

  if (!context)
    return false;

  this->Update();

  // Calculate the transforms required for the current rotation.
  this->CalculateTransforms();

  context->PushMatrix();
  context->AppendTransform(this->ContextTransform.GetPointer());

  // First lets draw the points in 3d.
  context->ApplyPen(this->Pen.GetPointer());
  if (this->NumberOfComponents == 0)
    {
    context->DrawPoints(this->points[0].GetData(), this->points.size());
    }
  else
    {
    context->DrawPoints(this->points[0].GetData(), this->points.size(),
                        this->Colors, this->NumberOfComponents);
    }

  // Now to render the selected points.
  if (!this->selectedPoints.empty())
    {
    context->ApplyPen(this->SelectedPen.GetPointer());
    context->DrawPoints(this->selectedPoints[0].GetData(), this->selectedPoints.size());
    }

  context->PopMatrix();

  // Now to draw the axes - pretty basic for now but could be extended.
  context->PushMatrix();
  context->AppendTransform(this->Box.GetPointer());
  context->ApplyPen(this->AxisPen.GetPointer());

  vtkVector3f box[4];
  box[0] = vtkVector3f(0, 0, 0);
  box[1] = vtkVector3f(0, 1, 0);
  box[2] = vtkVector3f(1, 1, 0);
  box[3] = vtkVector3f(1, 0, 0);
  context->DrawLine(box[0], box[1]);
  context->DrawLine(box[1], box[2]);
  context->DrawLine(box[2], box[3]);
  context->DrawLine(box[3], box[0]);
  for (int i = 0; i < 4; ++i)
    {
    box[i].SetZ(1);
    }
  context->DrawLine(box[0], box[1]);
  context->DrawLine(box[1], box[2]);
  context->DrawLine(box[2], box[3]);
  context->DrawLine(box[3], box[0]);
  context->DrawLine(vtkVector3f(0, 0, 0), vtkVector3f(0, 0, 1));
  context->DrawLine(vtkVector3f(1, 0, 0), vtkVector3f(1, 0, 1));
  context->DrawLine(vtkVector3f(0, 1, 0), vtkVector3f(0, 1, 1));
  context->DrawLine(vtkVector3f(1, 1, 0), vtkVector3f(1, 1, 1));

  // Now draw the axes labels
  vtkNew<vtkTextProperty> textProperties;
  textProperties->SetJustificationToLeft();
  textProperties->SetColor(0.0, 0.0, 0.0);
  textProperties->SetFontFamilyToArial();
  textProperties->SetFontSize(14);
  context->ApplyTextProp(textProperties.GetPointer());

  float bounds[4];
  context->ComputeStringBounds(this->XAxisLabel, bounds); 
  
  float scale[3];
  this->Box->GetScale(scale);

  // X axis first
  float xPos = 0.5 - bounds[2] / (scale[0] * 2);
  float yPos = (-bounds[3] - 5) / scale[1];
  context->DrawString(xPos, yPos, this->XAxisLabel);
  
  // Y axis next
  textProperties->SetOrientation(90);
  context->ApplyTextProp(textProperties.GetPointer());
  context->ComputeStringBounds(this->YAxisLabel, bounds); 
  xPos = -5 / scale[0];
  yPos = 0.5 - bounds[3] / (scale[1] * 2);
  context->DrawString(xPos, yPos, this->YAxisLabel);

  // Last is Z axis
  float pos[2];
  pos[0] = 0;
  pos[1] = 0;
  //context->DrawZAxisLabel(pos, this->ZAxisLabel);

  context->PopMatrix();

  return true;
}

void vtkInteractiveChartXYZ::SetInput(vtkTable *input, const vtkStdString &xName,
                           const vtkStdString &yName, const vtkStdString &zName)
{
  this->Superclass::SetInput(input, xName, yName, zName);
  this->XAxisLabel = xName;
  this->YAxisLabel = yName;
  this->ZAxisLabel = zName;
}

void vtkInteractiveChartXYZ::SetInput(vtkTable *input, const vtkStdString &xName,
                           const vtkStdString &yName, const vtkStdString &zName,
                           const vtkStdString &colorName)
{
  this->Superclass::SetInput(input, xName, yName, zName);
  this->XAxisLabel = xName;
  this->YAxisLabel = yName;
  this->ZAxisLabel = zName;
  
  vtkDataArray *colorArr =
      vtkDataArray::SafeDownCast(input->GetColumnByName(colorName.c_str()));

  assert(colorArr);
  assert(colorArr->GetNumberOfTuples() == this->points.size());

  this->NumberOfComponents = 3;

  //generate a color lookup table
  vtkNew<vtkLookupTable> lookupTable;
  double min = DBL_MAX;
  double max = DBL_MIN;
  for (int i = 0; i < this->points.size(); ++i)
    {
    double value = colorArr->GetComponent(i, 0);
    if (value > max)
      {
      max = value;
      }
    else if (value < min)
      {
      min = value;
      }
    }

  lookupTable->SetNumberOfTableValues(256);
  lookupTable->SetRange(min, max);
  lookupTable->Build();

  double color[3];
  this->Colors =
    new unsigned char[this->points.size() * this->NumberOfComponents];

  for (int i = 0; i < this->points.size(); ++i)
    {
    double value = colorArr->GetComponent(i, 0);
    unsigned char *rgb = lookupTable->MapValue(value);
    this->Colors[i * this->NumberOfComponents] = rgb[0];
    this->Colors[i * this->NumberOfComponents + 1] = rgb[1];
    this->Colors[i * this->NumberOfComponents + 2] = rgb[2];
    }
}

vtkInteractiveChartXYZ::vtkInteractiveChartXYZ()
{
  this->Translation->Identity();
  this->Translation->PostMultiply();
  this->Scale->Identity();
  this->Scale->PostMultiply();
  this->Interactive = true;
  this->Colors = NULL;
  this->NumberOfComponents = 0;
}

vtkInteractiveChartXYZ::~vtkInteractiveChartXYZ()
{
  if(this->Colors != NULL)
    {
    delete [] this->Colors;
    this->Colors = NULL;
    }
}

//-----------------------------------------------------------------------------
bool vtkInteractiveChartXYZ::Hit(const vtkContextMouseEvent &vtkNotUsed(mouse))
{
  // If we are interactive, we want to catch anything that propagates to the
  // background, otherwise we do not want any mouse events.
  return this->Interactive;
}

//-----------------------------------------------------------------------------
bool vtkInteractiveChartXYZ::MouseButtonPressEvent(const vtkContextMouseEvent &mouse)
{
  if (mouse.GetButton() == vtkContextMouseEvent::LEFT_BUTTON)
    {
    return true;
    }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkInteractiveChartXYZ::MouseMoveEvent(const vtkContextMouseEvent &mouse)
{
  if (mouse.GetButton() == vtkContextMouseEvent::LEFT_BUTTON)
    {
    if (mouse.GetModifiers() == vtkContextMouseEvent::SHIFT_MODIFIER)
      {
      return this->Spin(mouse);
      }
    else
      {
      return this->Rotate(mouse);
      }
    }
  if (mouse.GetButton() == vtkContextMouseEvent::RIGHT_BUTTON)
    {
    if (mouse.GetModifiers() == vtkContextMouseEvent::SHIFT_MODIFIER)
      {
      return this->Pan(mouse);
      }
    else
      {
      return this->Zoom(mouse);
      }
    }
  return false;
}
    
//-----------------------------------------------------------------------------
bool vtkInteractiveChartXYZ::MouseWheelEvent(const vtkContextMouseEvent &mouse, int delta)
{
  // Ten "wheels" to double/halve zoom level
  float scaling = pow(2.0f, delta/10.0f);
  this->Scale->Scale(scaling, scaling, scaling);

  // Mark the scene as dirty
  this->Scene->SetDirty(true);

  this->InvokeEvent(vtkCommand::InteractionEvent);
  return true;
}

//-----------------------------------------------------------------------------
bool vtkInteractiveChartXYZ::Rotate(const vtkContextMouseEvent &mouse)
{
  // Figure out how much the mouse has moved in plot coordinates
  vtkVector2d screenPos(mouse.GetScreenPos().Cast<double>().GetData());
  vtkVector2d lastScreenPos(mouse.GetLastScreenPos().Cast<double>().GetData());

  double dx = screenPos[0] - lastScreenPos[0];
  double dy = screenPos[1] - lastScreenPos[1];

  double delta_elevation = -20.0 / this->Scene->GetSceneHeight();
  double delta_azimuth = -20.0 / this->Scene->GetSceneWidth();

  double rxf = dx * delta_azimuth * 10.0;
  double ryf = dy * delta_elevation * 10.0;
  
  this->Rotation->RotateY(-rxf);
  this->Rotation->RotateX(ryf);

  // Mark the scene as dirty
  this->Scene->SetDirty(true);

  this->InvokeEvent(vtkCommand::InteractionEvent);
  return true;
}

//-----------------------------------------------------------------------------
bool vtkInteractiveChartXYZ::Pan(const vtkContextMouseEvent &mouse)
{
  // Figure out how much the mouse has moved in plot coordinates
  vtkVector2d screenPos(mouse.GetScreenPos().Cast<double>().GetData());
  vtkVector2d lastScreenPos(mouse.GetLastScreenPos().Cast<double>().GetData());

  double dx = (screenPos[0] - lastScreenPos[0]);
  double dy = (screenPos[1] - lastScreenPos[1]);

  this->Translation->Translate(dx, dy, 0.0);

  // Mark the scene as dirty
  this->Scene->SetDirty(true);

  this->InvokeEvent(vtkCommand::InteractionEvent);
  return true;
}

//-----------------------------------------------------------------------------
bool vtkInteractiveChartXYZ::Zoom(const vtkContextMouseEvent &mouse)
{
  std::cout << "zooming the natural way" << std::endl;
  // Figure out how much the mouse has moved and scale accordingly
  vtkVector2d screenPos(mouse.GetScreenPos().Cast<double>().GetData());
  vtkVector2d lastScreenPos(mouse.GetLastScreenPos().Cast<double>().GetData());
    
  float delta = 0.0f;
  if (this->Scene->GetSceneHeight() > 0)
    {
    delta = static_cast<float>(mouse.GetLastScreenPos()[1] - mouse.GetScreenPos()[1])/this->Scene->GetSceneHeight();
    }

  // Dragging full screen height zooms 4x.
  float scaling = pow(4.0f, delta);
  this->Scale->Scale(scaling, scaling, scaling);

  // Mark the scene as dirty
  this->Scene->SetDirty(true);

  this->InvokeEvent(vtkCommand::InteractionEvent);
  return true;
}

//-----------------------------------------------------------------------------
bool vtkInteractiveChartXYZ::Spin(const vtkContextMouseEvent &mouse)
{
  // Figure out how much the mouse has moved in plot coordinates
  vtkVector2d screenPos(mouse.GetScreenPos().Cast<double>().GetData());
  vtkVector2d lastScreenPos(mouse.GetLastScreenPos().Cast<double>().GetData());

  double newAngle =
    vtkMath::DegreesFromRadians(atan2(screenPos[1], screenPos[0]));
  double oldAngle =
    vtkMath::DegreesFromRadians(atan2(lastScreenPos[1], lastScreenPos[0]));

  this->Rotation->RotateZ(-(newAngle - oldAngle));

  // Mark the scene as dirty
  this->Scene->SetDirty(true);

  this->InvokeEvent(vtkCommand::InteractionEvent);
  return true;
}

//-----------------------------------------------------------------------------
bool vtkInteractiveChartXYZ::KeyPressEvent(const vtkContextKeyEvent &key)
{
  switch (key.GetKeyCode())
    {
    // Change view to 2D, YZ chart
    case 'x':
      this->LookDownX();
      break;
    case 'X':
      this->LookUpX();
      break;
    // Change view to 2D, XZ chart
    case 'y':
      this->LookDownY();
      break;
    case 'Y':
      this->LookUpY();
      break;
    // Change view to 2D, XY chart
    case 'z':
      this->LookDownZ();
      break;
    case 'Z':
      this->LookUpZ();
      break;
    }

  return true;
}

//-----------------------------------------------------------------------------
void vtkInteractiveChartXYZ::LookDownX()
{
  this->InvokeEvent(vtkCommand::InteractionEvent);
  this->Rotation->Identity();
  this->Rotation->RotateY(90.0);
  this->Scene->SetDirty(true);
}

//-----------------------------------------------------------------------------
void vtkInteractiveChartXYZ::LookDownY()
{
  this->Rotation->Identity();
  this->Rotation->RotateX(90.0);
  this->InvokeEvent(vtkCommand::InteractionEvent);
  this->Scene->SetDirty(true);
}

//-----------------------------------------------------------------------------
void vtkInteractiveChartXYZ::LookDownZ()
{
  this->Rotation->Identity();
  this->InvokeEvent(vtkCommand::InteractionEvent);
  this->Scene->SetDirty(true);
}

//-----------------------------------------------------------------------------
void vtkInteractiveChartXYZ::LookUpX()
{
  this->InvokeEvent(vtkCommand::InteractionEvent);
  this->Rotation->Identity();
  this->Rotation->RotateY(-90.0);
  this->Scene->SetDirty(true);
}

//-----------------------------------------------------------------------------
void vtkInteractiveChartXYZ::LookUpY()
{
  this->Rotation->Identity();
  this->Rotation->RotateX(-90.0);
  this->InvokeEvent(vtkCommand::InteractionEvent);
  this->Scene->SetDirty(true);
}

//-----------------------------------------------------------------------------
void vtkInteractiveChartXYZ::LookUpZ()
{
  this->Rotation->Identity();
  this->Rotation->RotateZ(180.0);
  this->InvokeEvent(vtkCommand::InteractionEvent);
  this->Scene->SetDirty(true);
}

void vtkInteractiveChartXYZ::CalculateTransforms()
{
  // First the rotation transform...
  // Calculate the correct translation vector before the rotation is applied
  vtkVector3f translation(
        (axes[0]->GetPosition2()[0] - axes[0]->GetPosition1()[0]) / 2.0
        + axes[0]->GetPosition1()[0],
        (axes[1]->GetPosition2()[1] - axes[1]->GetPosition1()[1]) / 2.0
        + axes[1]->GetPosition1()[1],
        (axes[2]->GetPosition2()[1] - axes[2]->GetPosition1()[1]) / 2.0
        + axes[2]->GetPosition1()[1]);
  vtkVector3f mtranslation = -1.0 * translation;

  this->ContextTransform->Identity();
  this->ContextTransform->Concatenate(this->Translation.GetPointer());
  this->ContextTransform->Translate(translation.GetData());
  this->ContextTransform->Concatenate(this->Rotation.GetPointer());
  this->ContextTransform->Concatenate(this->Scale.GetPointer());
  this->ContextTransform->Translate(mtranslation.GetData());
  this->ContextTransform->Concatenate(this->Transform.GetPointer());

  // Next the box rotation transform.
  double scale[3] = { 300, 300, 300 };
  for (int i = 0; i < 3; ++i)
    {
    if (i == 0)
      scale[i] = axes[i]->GetPosition2()[0] - axes[i]->GetPosition1()[0];
    else
      scale[i] = axes[i]->GetPosition2()[1] - axes[i]->GetPosition1()[1];
    }

  this->Box->Identity();
  this->Box->PostMultiply();
  this->Box->Translate(-0.5, -0.5, -0.5);
  this->Box->Concatenate(this->Rotation.GetPointer());
  //this->Box->Concatenate(this->Scale.GetPointer());
  this->Box->Translate(0.5, 0.5, 0.5);
  this->Box->Scale(scale);
  this->Box->Translate(axes[0]->GetPosition1()[0],
                       axes[1]->GetPosition1()[1],
                       axes[2]->GetPosition1()[1]);
  //this->Box->Concatenate(this->Translation.GetPointer());
}
