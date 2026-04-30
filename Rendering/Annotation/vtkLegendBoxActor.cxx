// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkLegendBoxActor.h"

#include "Private/vtkLegendBoxEntryInternal.h"
#include "vtkCellArray.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPlaneSource.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkProperty.h"
#include "vtkProperty2D.h"
#include "vtkTextMapper.h"
#include "vtkTextProperty.h"
#include "vtkTexture.h"
#include "vtkTexturedActor2D.h"
#include "vtkViewport.h"

#include <iostream>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkLegendBoxActor);

vtkCxxSetObjectMacro(vtkLegendBoxActor, EntryTextProperty, vtkTextProperty);

//------------------------------------------------------------------------------
vtkLegendBoxActor::vtkLegendBoxActor()
{
  // Positioning information
  this->PositionCoordinate->SetCoordinateSystemToNormalizedViewport();
  this->PositionCoordinate->SetValue(0.75, 0.75);

  this->Position2Coordinate->SetValue(0.2, 0.2);

  this->LockBorder = 0;
  this->ScalarVisibility = 1;

  // Control font properties
  this->EntryTextProperty = vtkTextProperty::New();
  this->EntryTextProperty->SetBold(0);
  this->EntryTextProperty->SetItalic(0);
  this->EntryTextProperty->SetShadow(0);
  this->EntryTextProperty->SetFontFamily(VTK_ARIAL);
  this->EntryTextProperty->SetJustification(VTK_TEXT_LEFT);
  this->EntryTextProperty->SetVerticalJustification(VTK_TEXT_CENTERED);

  this->Border = 1;
  this->Box = 0;
  this->Padding = 3;

  this->NumberOfEntries = 0;

  // Construct the border
  this->BorderPolyData = vtkPolyData::New();
  vtkPoints* points = vtkPoints::New();
  points->SetNumberOfPoints(4);
  this->BorderPolyData->SetPoints(points);
  points->Delete();
  vtkCellArray* lines = vtkCellArray::New();
  lines->InsertNextCell(5); // points will be updated later
  lines->InsertCellPoint(0);
  lines->InsertCellPoint(1);
  lines->InsertCellPoint(2);
  lines->InsertCellPoint(3);
  lines->InsertCellPoint(0);
  this->BorderPolyData->SetLines(lines);
  lines->Delete();

  this->BorderMapper = vtkPolyDataMapper2D::New();
  this->BorderMapper->SetInputData(this->BorderPolyData);

  this->BorderActor = vtkActor2D::New();
  this->BorderActor->SetMapper(this->BorderMapper);

  // Construct the box
  this->BoxPolyData = vtkPolyData::New();
  this->BoxPolyData->SetPoints(this->BorderPolyData->GetPoints());
  vtkCellArray* polys = vtkCellArray::New();
  polys->InsertNextCell(4);
  polys->InsertCellPoint(0);
  polys->InsertCellPoint(1);
  polys->InsertCellPoint(2);
  polys->InsertCellPoint(3);
  this->BoxPolyData->SetPolys(polys);
  polys->Delete();

  this->BoxMapper = vtkPolyDataMapper2D::New();
  this->BoxMapper->SetInputData(this->BoxPolyData);

  this->BoxActor = vtkActor2D::New();
  this->BoxActor->SetMapper(this->BoxMapper);

  // Background.
  this->UseBackground = 0;
  this->BackgroundOpacity = 1.0;
  this->BackgroundColor[0] = this->BackgroundColor[1] = this->BackgroundColor[2] = 0.3;
  this->Background = vtkPlaneSource::New();
  this->BackgroundActor = vtkTexturedActor2D::New();
  this->BackgroundMapper = vtkPolyDataMapper2D::New();
  this->BackgroundActor->SetMapper(this->BackgroundMapper);
}

//------------------------------------------------------------------------------
vtkLegendBoxActor::~vtkLegendBoxActor()
{
  if (this->BorderActor)
  {
    this->BorderActor->Delete();
    this->BorderMapper->Delete();
    this->BorderPolyData->Delete();
  }

  if (this->BoxActor)
  {
    this->BoxActor->Delete();
    this->BoxMapper->Delete();
    this->BoxPolyData->Delete();
  }

  if (this->BackgroundActor)
  {
    this->BackgroundActor->Delete();
    this->BackgroundMapper->Delete();
    this->Background->Delete();
  }

  this->SetEntryTextProperty(nullptr);
}

//------------------------------------------------------------------------------
void vtkLegendBoxActor::InitializeEntries()
{
  this->Entries.clear();
}

//------------------------------------------------------------------------------
void vtkLegendBoxActor::SetNumberOfEntries(int num)
{
  if (num == this->NumberOfEntries)
  {
    return;
  }

  this->Entries.resize(num);
  for (int newIdx = this->NumberOfEntries; newIdx < num; newIdx++)
  {
    this->Entries[newIdx] = std::make_unique<vtkLegendBoxEntryInternal>();
  }

  this->NumberOfEntries = num;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkLegendBoxActor::SetEntry(int i, vtkPolyData* symbol, const char* string, double color[3])
{
  if (i >= 0 && i < this->NumberOfEntries)
  {
    this->SetEntrySymbol(i, symbol);
    this->SetEntryString(i, string);
    this->SetEntryColor(i, color);
  }
}

//------------------------------------------------------------------------------
void vtkLegendBoxActor::SetEntry(int i, vtkImageData* icon, const char* string, double color[3])
{
  if (i >= 0 && i < this->NumberOfEntries)
  {
    this->SetEntryIcon(i, icon);
    this->SetEntryString(i, string);
    this->SetEntryColor(i, color);
  }
}

//------------------------------------------------------------------------------
void vtkLegendBoxActor::SetEntry(
  int i, vtkPolyData* symbol, vtkImageData* icon, const char* string, double color[3])
{
  if (i >= 0 && i < this->NumberOfEntries)
  {
    this->SetEntry(i, symbol, string, color);
    this->SetEntryIcon(i, icon);
  }
}

//------------------------------------------------------------------------------
void vtkLegendBoxActor::SetEntrySymbol(int i, vtkPolyData* symbol)
{
  if (i >= 0 && i < this->NumberOfEntries)
  {
    if (this->Entries[i]->SetSymbol(symbol))
    {
      this->Modified();
    }
  }
}

//------------------------------------------------------------------------------
void vtkLegendBoxActor::SetEntryIcon(int i, vtkImageData* icon)
{
  if (i >= 0 && i < this->NumberOfEntries)
  {
    if (this->Entries[i]->SetIcon(icon))
    {
      this->Modified();
    }
  }
}

//------------------------------------------------------------------------------
void vtkLegendBoxActor::SetEntryString(int i, const char* string)
{
  if (i >= 0 && i < this->NumberOfEntries)
  {
    if (this->Entries[i]->SetText(string))
    {
      this->Modified();
    }
  }
}

//------------------------------------------------------------------------------
void vtkLegendBoxActor::SetEntryColor(int i, double color[3])
{
  if (i >= 0 && i < this->NumberOfEntries)
  {
    if (this->Entries[i]->SetColor(color))
    {
      this->Modified();
    }
  }
}

//------------------------------------------------------------------------------
void vtkLegendBoxActor::SetEntryColor(int i, double r, double g, double b)
{
  double rgb[3];
  rgb[0] = r;
  rgb[1] = g;
  rgb[2] = b;
  this->SetEntryColor(i, rgb);
}

//------------------------------------------------------------------------------
vtkPolyData* vtkLegendBoxActor::GetEntrySymbol(int i)
{
  if (i < 0 || i >= this->NumberOfEntries)
  {
    return nullptr;
  }
  else
  {
    return this->Entries[i]->GetSymbol();
  }
}

//------------------------------------------------------------------------------
vtkImageData* vtkLegendBoxActor::GetEntryIcon(int i)
{
  if (i < 0 || i >= this->NumberOfEntries)
  {
    return nullptr;
  }
  else
  {
    return this->Entries[i]->GetIcon();
  }
}

//------------------------------------------------------------------------------
const char* vtkLegendBoxActor::GetEntryString(int i)
{
  if (i < 0 || i >= this->NumberOfEntries)
  {
    return nullptr;
  }
  else
  {
    return this->Entries[i]->GetText();
  }
}

//------------------------------------------------------------------------------
double* vtkLegendBoxActor::GetEntryColor(int i)
{
  if (i < 0 || i >= this->NumberOfEntries)
  {
    return nullptr;
  }
  else
  {
    return this->Entries[i]->GetColor();
  }
}

//------------------------------------------------------------------------------
// Release any graphics resources that are being consumed by this actor.
// The parameter window could be used to determine which graphic
// resources to release.
void vtkLegendBoxActor::ReleaseGraphicsResources(vtkWindow* win)
{
  if (this->BackgroundActor)
  {
    this->BackgroundActor->ReleaseGraphicsResources(win);
  }

  if (this->BorderActor)
  {
    this->BorderActor->ReleaseGraphicsResources(win);
  }

  if (this->BoxActor)
  {
    this->BoxActor->ReleaseGraphicsResources(win);
  }

  for (int i = 0; i < this->NumberOfEntries; i++)
  {
    this->Entries[i]->ReleaseGraphicsResources(win);
  }
}

//------------------------------------------------------------------------------
int vtkLegendBoxActor::RenderOverlay(vtkViewport* viewport)
{
  if (this->NumberOfEntries <= 0)
  {
    return 0;
  }

  int renderedSomething = 0;
  if (this->BackgroundActor && this->UseBackground)
  {
    this->BackgroundActor->RenderOverlay(viewport);
  }

  if (this->Border)
  {
    renderedSomething += this->BorderActor->RenderOverlay(viewport);
  }

  if (this->Box)
  {
    renderedSomething += this->BoxActor->RenderOverlay(viewport);
  }

  if (this->LegendEntriesVisible)
  {
    for (int i = 0; i < this->NumberOfEntries; i++)
    {
      this->Entries[i]->RenderOverlay(viewport);
    }
  }

  return renderedSomething;
}

//------------------------------------------------------------------------------
int vtkLegendBoxActor::RenderOpaqueGeometry(vtkViewport* viewport)
{
  int i;
  double symbolSize;

  if (this->NumberOfEntries <= 0)
  {
    return 0;
  }

  if (!this->EntryTextProperty)
  {
    vtkErrorMacro(<< "Need entry text property to render legend box actor");
    return 0;
  }

  // Check to see whether we have to rebuild everything
  const int* vsize = viewport->GetSize();
  if (this->GetMTime() > this->BuildTime || this->EntryTextProperty->GetMTime() > this->BuildTime ||
    vsize[0] != this->CachedSize[0] || vsize[1] != this->CachedSize[1])
  {
    vtkDebugMacro(<< "Rebuilding text");
    this->CachedSize[0] = vsize[0];
    this->CachedSize[1] = vsize[1];

    // If text prop has changed, recopy it to all mappers
    // We have to use shallow copy since the color of each text prop
    // can be overridden

    if (this->EntryTextProperty->GetMTime() > this->BuildTime)
    {
      for (i = 0; i < this->NumberOfEntries; i++)
      {
        this->Entries[i]->CopyTextProperty(this->EntryTextProperty);
      }
    }

    // Get position information
    int *x1, *x2;
    double p1[3], p2[3];
    x1 = this->PositionCoordinate->GetComputedViewportValue(viewport);
    x2 = this->Position2Coordinate->GetComputedViewportValue(viewport);
    p1[0] = (double)x1[0];
    p1[1] = (double)x1[1];
    p1[2] = 0.0;
    p2[0] = (double)x2[0];
    p2[1] = (double)x2[1];
    p2[2] = 0.0;

    // Compute spacing...trying to keep things proportional
    //
    // Find the longest string and symbol width ratio
    int length, maxLength;
    int maxTextMapper = 0;
    int tempi[2], fontSize;
    double sf, twr, swr;
    const double* bounds;
    bool iconExists(false);
    bool symbolExists(false);

    for (swr = 0.0, maxLength = i = 0; i < this->NumberOfEntries; i++)
    {
      this->Entries[i]->SetFontSize(12);
      int size[2];
      this->Entries[i]->GetSize(viewport, size);
      length = size[0];
      if (length > maxLength)
      {
        maxLength = length;
        maxTextMapper = i;
      }

      if (this->Entries[i]->HasSymbol()) // if there is a symbol
      {
        symbolExists = true;
        // this->Symbol[i]->Update();
        bounds = this->Entries[i]->GetSymbolBounds();
        if ((bounds[3] - bounds[2]) == 0.0)
        {
          sf = 1.0;
        }
        else
        {
          sf = (bounds[1] - bounds[0]) / (bounds[3] - bounds[2]);
        }
        swr = std::max(sf, swr);
      } // if symbol defined

      // We pick the one with highest ratio if both symbol and icon
      // exists.
      if (this->Entries[i]->HasIcon())
      {
        iconExists = true;

        bounds = this->Entries[i]->GetIconBounds();
        if ((bounds[3] - bounds[2]) == 0.0)
        {
          sf = 1.0;
        }
        else
        {
          sf = (bounds[1] - bounds[0]) / (bounds[3] - bounds[2]);
        }
        swr = std::max(sf, swr);
      } // if icon defined.
    }

    // Compute the final proportion (symbol width to text width)
    fontSize = 12;
    this->Entries[maxTextMapper]->SetFontSize(fontSize);
    this->Entries[maxTextMapper]->GetSize(viewport, tempi);

    if (maxLength > 0) // make sure that tempi is not 0, to avoid a
                       // divide-by-zero floating-point exception.
    {
      twr = (double)tempi[0] / tempi[1];
      symbolSize = swr / (swr + twr);
    }
    else
    {
      symbolSize = 0;
    }

    if (iconExists && symbolExists)
    {
      symbolSize *= 2;
    }

    // Okay, now that the proportions are okay, let's size everything
    // First the text
    int size[2];
    size[0] = (int)((1.0 - symbolSize) * (p2[0] - p1[0] - 2.0 * this->Padding));
    size[1] = (int)((p2[1] - p1[1] - 2.0 * this->Padding) / this->NumberOfEntries);

    fontSize = this->Entries[maxTextMapper]->SetConstrainedFontSize(viewport, size);
    this->Entries[maxTextMapper]->GetSize(viewport, tempi);

    // don't draw anything if it's too small
    if (size[1] > 0 && fontSize > 0)
    {
      this->LegendEntriesVisible = 1;
    }
    else
    {
      this->LegendEntriesVisible = 0;
    }

    // Border and box - may adjust spacing based on font size relationship
    // to the proportions relative to the border
    //
    if (this->Border || this->Box)
    {
      // adjust the border/box placement if too much whitespace
      if (!this->LockBorder && tempi[0] < size[0])
      {
        p2[0] =
          p1[0] + 2. * this->Padding + symbolSize * (p2[0] - p1[0] - 2. * this->Padding) + tempi[0];
      }
      vtkPoints* pts = this->BorderPolyData->GetPoints();
      pts->SetPoint(0, p1);
      pts->SetPoint(1, p2[0], p1[1], 0.);
      pts->SetPoint(2, p2[0], p2[1], 0.);
      pts->SetPoint(3, p1[0], p2[1], 0.);
      pts->Modified();
    }

    if (this->UseBackground)
    {
      this->Background->SetOrigin(p1[0], p1[1], 0.);
      this->Background->SetPoint1(p2[0], p1[1], 0.);
      this->Background->SetPoint2(p1[0], p2[1], 0.);

      this->BackgroundMapper->SetInputConnection(this->Background->GetOutputPort());
      this->BackgroundActor->GetProperty()->SetOpacity(this->BackgroundOpacity);
      this->BackgroundActor->GetProperty()->SetColor(this->BackgroundColor);
    }

    if (this->Border)
    {
      this->BorderActor->SetProperty(this->GetProperty());
    }

    // Place entries
    const double sizeFraction = (symbolExists && iconExists) ? 0.5 : 1.;
    const double symbolsPositionFraction = (symbolExists && iconExists) ? 0.25 : 0.5;
    // NOLINTBEGIN(readability-avoid-nested-conditional-operator)
    const double iconsPositionFraction = (symbolExists && iconExists) ? 0.625
      : iconExists                                                    ? 0.5
                                                                      : 0.0;
    // NOLINTEND(readability-avoid-nested-conditional-operator)

    // Find the x-y bounds of the symbols...we'll be scaling these as well
    int entryHeight = size[1];
    double entryYStart = p2[1] - this->Padding - 0.5 * entryHeight;
    double textPosX = p1[0] + this->Padding + symbolSize * (p2[0] - p1[0] - 2.0 * this->Padding);

    int symbolWidth = (int)(sizeFraction * symbolSize * (p2[0] - p1[0] - 2.0 * this->Padding));
    double symbolX = p1[0] + this->Padding +
      symbolsPositionFraction * symbolSize * (p2[0] - p1[0] - 2.0 * this->Padding);

    // Place icons.
    int iconWidth = (int)(sizeFraction * symbolSize * (p2[0] - p1[0] - 2.0 * this->Padding));
    double iconX = p1[0] + this->Padding +
      iconsPositionFraction * symbolSize * (p2[0] - p1[0] - 2.0 * this->Padding);

    for (i = 0; i < this->NumberOfEntries; i++)
    {
      double textPosY = entryYStart - (double)i * size[1];
      this->Entries[i]->SetTextPosition(textPosX, textPosY);
      this->Entries[i]->SetFontSize(fontSize);

      double entryY = textPosY - 0.25 * tempi[1];
      this->Entries[i]->UpdateIconTransform(iconWidth, entryHeight, iconX, entryY);
      this->Entries[i]->UpdateSymbolTransform(symbolWidth, entryHeight, symbolX, entryY);
      this->Entries[i]->UpdateProperties(this->ScalarVisibility, this->GetProperty());
    }
    this->BuildTime.Modified();
  } // rebuild legend box

  // Okay, now we're ready to render something
  // Border
  int renderedSomething = 0;
  if (this->BackgroundActor && this->UseBackground)
  {
    this->BackgroundActor->RenderOpaqueGeometry(viewport);
  }

  if (this->Border)
  {
    renderedSomething += this->BorderActor->RenderOpaqueGeometry(viewport);
  }

  if (this->Box)
  {
    renderedSomething += this->BoxActor->RenderOpaqueGeometry(viewport);
  }

  if (this->LegendEntriesVisible)
  {
    for (i = 0; i < this->NumberOfEntries; i++)
    {
      this->Entries[i]->RenderOpaqueGeometry(viewport);
    }
  }

  return renderedSomething;
}

//------------------------------------------------------------------------------
// Description:
// Does this prop have some translucent polygonal geometry?
vtkTypeBool vtkLegendBoxActor::HasTranslucentPolygonalGeometry()
{
  return 0;
}

//------------------------------------------------------------------------------
void vtkLegendBoxActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->EntryTextProperty)
  {
    os << indent << "Entry Text Property:\n";
    this->EntryTextProperty->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Entry Text Property: (none)\n";
  }

  os << indent << "Number Of Entries: " << this->NumberOfEntries << "\n";

  os << indent << "Scalar Visibility: " << (this->ScalarVisibility ? "On\n" : "Off\n");
  os << indent << "Padding: " << this->Padding << "\n";
  os << indent << "Border: " << (this->Border ? "On\n" : "Off\n");
  os << indent << "Box: " << (this->Box ? "On\n" : "Off\n");
  os << indent << "LockBorder: " << (this->LockBorder ? "On\n" : "Off\n");

  os << indent << "UseBackgroud: " << (this->UseBackground ? "On\n" : "Off\n");
  os << indent << "BackgroundOpacity: " << this->BackgroundOpacity << "\n";

  os << indent << "BackgroundColor: (" << this->BackgroundColor[0] << ", "
     << this->BackgroundColor[1] << ", " << this->BackgroundColor[2] << ")\n";
}

//------------------------------------------------------------------------------
void vtkLegendBoxActor::ShallowCopy(vtkProp* prop)
{
  vtkLegendBoxActor* a = vtkLegendBoxActor::SafeDownCast(prop);
  if (a != nullptr)
  {
    this->SetPosition2(a->GetPosition2());
    this->SetEntryTextProperty(a->GetEntryTextProperty());
    this->SetBorder(a->GetBorder());
    this->SetLockBorder(a->GetLockBorder());
    this->SetPadding(a->GetPadding());
    this->SetScalarVisibility(a->GetScalarVisibility());
    this->SetNumberOfEntries(a->GetNumberOfEntries());
    for (int i = 0; i < this->NumberOfEntries; i++)
    {
      this->SetEntrySymbol(i, a->GetEntrySymbol(i));
      this->SetEntryString(i, a->GetEntryString(i));
      this->SetEntryColor(i, a->GetEntryColor(i));
    }
  }

  // Now do superclass
  this->vtkActor2D::ShallowCopy(prop);
}
VTK_ABI_NAMESPACE_END
