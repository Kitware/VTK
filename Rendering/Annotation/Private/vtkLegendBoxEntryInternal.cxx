// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkLegendBoxEntryInternal.h"

#include "vtkActor2D.h"
#include "vtkImageData.h"
#include "vtkPlaneSource.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkProperty2D.h"
#include "vtkTextMapper.h"
#include "vtkTextProperty.h"
#include "vtkTexture.h"
#include "vtkTexturedActor2D.h"
#include "vtkTransform.h"
#include "vtkTransformFilter.h"
#include "vtkViewport.h"
#include "vtkWindow.h"

//------------------------------------------------------------------------------
vtkLegendBoxEntryInternal::vtkLegendBoxEntryInternal()
{
  this->TextActor->SetMapper(this->TextMapper);
  this->SymbolTransform->SetTransform(this->Transform);
  this->SymbolMapper->SetInputConnection(this->SymbolTransform->GetOutputPort());
  this->SymbolActor->SetMapper(this->SymbolMapper);
  this->TextMapper->GetTextProperty()->SetVerticalJustificationToCentered();
  this->TextMapper->GetTextProperty()->SetJustificationToLeft();

  this->Icon->SetPoint1(1.0, 0.0, 0.0);
  this->Icon->SetPoint2(0, 1.0, 0.0);
  this->Icon->SetOrigin(0.0, 0.0, 0.0);
  this->Icon->SetResolution(1, 1);
  this->IconTransformFilter->SetInputConnection(this->Icon->GetOutputPort());

  this->IconTransformFilter->SetTransform(this->IconTransform);
  this->IconMapper->SetInputConnection(this->IconTransformFilter->GetOutputPort());
  this->IconActor->SetMapper(this->IconMapper);
}

//------------------------------------------------------------------------------
vtkLegendBoxEntryInternal::~vtkLegendBoxEntryInternal() = default;

//------------------------------------------------------------------------------
bool vtkLegendBoxEntryInternal::SetSymbol(vtkPolyData* symbol)
{
  if (this->Symbol == symbol)
  {
    return false;
  }

  this->Symbol = symbol;
  if (symbol)
  {
    this->SymbolTransform->SetInputData(symbol);
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkLegendBoxEntryInternal::SetIcon(vtkImageData* icon)
{
  if (this->IconImage == icon)
  {
    return false;
  }

  this->IconImage = icon;
  vtkNew<vtkTexture> texture;
  texture->SetInputData(icon);
  this->IconActor->SetTexture(texture);

  return true;
}

//------------------------------------------------------------------------------
bool vtkLegendBoxEntryInternal::SetText(const char* text)
{
  if (this->TextMapper->GetInput() && text && (!strcmp(this->TextMapper->GetInput(), text)))
  {
    return false;
  }

  this->TextMapper->SetInput(text);
  return true;
}

//------------------------------------------------------------------------------
bool vtkLegendBoxEntryInternal::SetColor(double color[3])
{
  if (this->Color[0] == color[0] && this->Color[1] == color[1] && this->Color[2] == color[2])
  {
    return false;
  }

  this->Color[0] = color[0];
  this->Color[1] = color[1];
  this->Color[2] = color[2];
  return true;
}

//------------------------------------------------------------------------------
vtkPolyData* vtkLegendBoxEntryInternal::GetSymbol()
{
  return this->Symbol;
}

//------------------------------------------------------------------------------
vtkImageData* vtkLegendBoxEntryInternal::GetIcon()
{
  return this->IconImage;
}

//------------------------------------------------------------------------------
const char* vtkLegendBoxEntryInternal::GetText()
{
  return this->TextMapper->GetInput();
}

//------------------------------------------------------------------------------
double* vtkLegendBoxEntryInternal::GetColor()
{
  return this->Color;
}

//------------------------------------------------------------------------------
void vtkLegendBoxEntryInternal::ReleaseGraphicsResources(vtkWindow* win)
{
  this->TextActor->ReleaseGraphicsResources(win);
  this->SymbolActor->ReleaseGraphicsResources(win);
  this->IconActor->ReleaseGraphicsResources(win);
}

//------------------------------------------------------------------------------
int vtkLegendBoxEntryInternal::RenderOverlay(vtkViewport* viewport)
{
  int renderSomething = 0;
  if (this->HasSymbol())
  {
    renderSomething += this->SymbolActor->RenderOverlay(viewport);
  }
  if (this->HasIcon())
  {
    renderSomething += this->IconActor->RenderOverlay(viewport);
  }
  renderSomething += this->TextActor->RenderOverlay(viewport);

  return renderSomething;
}

//------------------------------------------------------------------------------
int vtkLegendBoxEntryInternal::RenderOpaqueGeometry(vtkViewport* viewport)
{
  int renderedSomething = 0;
  if (this->HasSymbol())
  {
    renderedSomething += this->SymbolActor->RenderOpaqueGeometry(viewport);
  }
  if (this->HasIcon())
  {
    renderedSomething += this->IconActor->RenderOpaqueGeometry(viewport);
  }
  renderedSomething += this->TextActor->RenderOpaqueGeometry(viewport);

  return renderedSomething;
}

//------------------------------------------------------------------------------
double* vtkLegendBoxEntryInternal::GetSymbolBounds()
{
  return this->Symbol->GetBounds();
}

//------------------------------------------------------------------------------
double* vtkLegendBoxEntryInternal::GetIconBounds()
{
  this->IconTransformFilter->Update();
  return this->Icon->GetOutput()->GetBounds();
}

//------------------------------------------------------------------------------
void vtkLegendBoxEntryInternal::UpdateIconTransform(int width, int height, double posX, double posY)
{
  if (!this->HasIcon())
  {
    return;
  }
  double bounds[6];
  this->Icon->Update();
  this->Icon->GetOutput()->GetBounds(bounds);
  this->UpdateTransform(this->IconTransform, posX, posY, bounds, width, height);
}

//------------------------------------------------------------------------------
void vtkLegendBoxEntryInternal::UpdateSymbolTransform(
  int width, int height, double posX, double posY)
{
  if (!this->HasSymbol())
  {
    return;
  }
  double bounds[6];
  this->Symbol->GetBounds(bounds);
  this->UpdateTransform(this->Transform, posX, posY, bounds, width, height);
}

//------------------------------------------------------------------------------
double vtkLegendBoxEntryInternal::GetScale(double bounds[6], int width, int height)
{
  double scaleFactor;
  if ((bounds[1] - bounds[0]) == 0.0)
  {
    scaleFactor = VTK_DOUBLE_MAX;
  }
  else
  {
    scaleFactor = width / (bounds[1] - bounds[0]);
  }

  if ((bounds[3] - bounds[2]) == 0.0)
  {
    if (scaleFactor >= VTK_DOUBLE_MAX)
    {
      scaleFactor = 1.0;
    }
  }
  else if ((height / (bounds[3] - bounds[2])) < scaleFactor)
  {
    scaleFactor = height / (bounds[3] - bounds[2]);
  }

  return scaleFactor;
}

//------------------------------------------------------------------------------
void vtkLegendBoxEntryInternal::UpdateTransform(
  vtkTransform* transform, double posX, double posY, double bounds[6], int width, int height)
{
  double scaleFactor = this->GetScale(bounds, width, height);

  transform->Identity();
  transform->Translate(posX, posY, 0.);
  transform->Scale(0.5 * scaleFactor, 0.5 * scaleFactor, 0);
}

//------------------------------------------------------------------------------
void vtkLegendBoxEntryInternal::UpdateProperties(bool visibility, vtkProperty2D* property)
{
  if (this->HasSymbol())
  {
    this->SymbolMapper->SetScalarVisibility(visibility);
    this->SymbolActor->GetProperty()->DeepCopy(property);
    if (this->Color[0] >= 0.)
    {
      this->SymbolActor->GetProperty()->SetColor(this->Color[0], this->Color[1], this->Color[2]);
    }
  }

  if (this->HasIcon())
  {
    this->IconMapper->SetScalarVisibility(visibility);
  }

  this->TextMapper->GetTextProperty()->SetColor(this->Color);
}

//------------------------------------------------------------------------------
void vtkLegendBoxEntryInternal::CopyTextProperty(vtkTextProperty* prop)
{
  this->TextMapper->GetTextProperty()->ShallowCopy(prop);
}

//------------------------------------------------------------------------------
bool vtkLegendBoxEntryInternal::HasSymbol()
{
  return this->Symbol != nullptr;
}

//------------------------------------------------------------------------------
bool vtkLegendBoxEntryInternal::HasIcon()
{
  return this->IconImage != nullptr;
}

//------------------------------------------------------------------------------
void vtkLegendBoxEntryInternal::SetFontSize(int size)
{
  this->TextMapper->GetTextProperty()->SetFontSize(size);
}

//------------------------------------------------------------------------------
int vtkLegendBoxEntryInternal::SetConstrainedFontSize(vtkViewport* viewport, int size[2])
{
  return this->TextMapper->SetConstrainedFontSize(viewport, size[0], size[1]);
}

//------------------------------------------------------------------------------
void vtkLegendBoxEntryInternal::GetSize(vtkViewport* viewport, int size[2])
{
  this->TextMapper->GetSize(viewport, size);
}

//------------------------------------------------------------------------------
void vtkLegendBoxEntryInternal::SetTextPosition(double X, double Y)
{
  this->TextActor->SetPosition(X, Y);
}
