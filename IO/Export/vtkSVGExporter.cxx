/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSVGExporter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSVGExporter.h"

#include "vtkBrush.h"
#include "vtkContext2D.h"
#include "vtkContextActor.h"
#include "vtkContextScene.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkProp.h"
#include "vtkPropCollection.h"
#include "vtkRect.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkSVGContextDevice2D.h"
#include "vtkTexture.h"
#include "vtkXMLDataElement.h"

#include <array>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <string>

namespace
{

std::string ColorToString(const unsigned char* rgb)
{
  std::ostringstream out;
  out << "#";
  for (int i = 0; i < 3; ++i)
  {
    out << std::setw(2) << std::right << std::setfill('0') << std::hex
        << static_cast<unsigned int>(rgb[i]);
  }
  return out.str();
}

} // end anon namespace

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkSVGExporter);

//------------------------------------------------------------------------------
void vtkSVGExporter::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkSVGExporter::vtkSVGExporter()
  : Title(nullptr)
  , Description(nullptr)
  , FileName(nullptr)
  , Device(nullptr)
  , RootNode(nullptr)
  , PageNode(nullptr)
  , DefinitionNode(nullptr)
  , SubdivisionThreshold(1.f)
  , DrawBackground(true)
  , TextAsPath(true)
{
  this->SetTitle("VTK Exported Scene");
  this->SetDescription("VTK Exported Scene");
}

//------------------------------------------------------------------------------
vtkSVGExporter::~vtkSVGExporter()
{
  assert(!this->Device);
  assert(!this->RootNode);
  assert(!this->PageNode);
  this->SetTitle(nullptr);
  this->SetDescription(nullptr);
  this->SetFileName(nullptr);
}

//------------------------------------------------------------------------------
void vtkSVGExporter::WriteData()
{
  if (!this->FileName || !*this->FileName)
  {
    vtkErrorMacro("FileName not specified.");
    return;
  }

  if (!this->RenderWindow)
  {
    vtkErrorMacro("No RenderWindow set -- nothing to export.");
    return;
  }

  // These should be freed at the end of the last export.
  assert(!this->Device);
  assert(!this->RootNode);
  assert(!this->DefinitionNode);
  assert(!this->PageNode);

  this->WriteSVG();

  this->Device->Delete();
  this->Device = nullptr;
  this->RootNode->Delete();
  this->RootNode = nullptr;
  this->DefinitionNode = nullptr;
  this->PageNode = nullptr;
}

//------------------------------------------------------------------------------
void vtkSVGExporter::WriteSVG()
{
  this->PrepareDocument();
  this->RenderContextActors();

  // If we aren't using any defs, remove the node.
  if (this->DefinitionNode->GetNumberOfNestedElements() == 0)
  {
    this->RootNode->RemoveNestedElement(this->DefinitionNode);
    this->DefinitionNode = nullptr; // no delete -- the remove above clears ref
  }
  else
  {
    this->Device->GenerateDefinitions();
  }

  this->RootNode->PrintXML(this->FileName);
}

//------------------------------------------------------------------------------
void vtkSVGExporter::PrepareDocument()
{
  const int* size = this->RenderWindow->GetSize();

  this->RootNode = vtkXMLDataElement::New();
  this->RootNode->SetName("svg");
  this->RootNode->SetAttribute("xmlns", "http://www.w3.org/2000/svg");
  this->RootNode->SetAttribute("xmlns:xlink", "http://www.w3.org/1999/xlink");
  this->RootNode->SetAttribute("version", "1.1");
  this->RootNode->SetIntAttribute("width", size[0]);
  this->RootNode->SetIntAttribute("height", size[1]);

  // Antialias everything by default (we disable this for adjacent polygons
  // when possible, though):
  this->RootNode->SetAttribute("shape-rendering", "geometricPrecision");

  if (this->Title && this->Title[0])
  {
    vtkNew<vtkXMLDataElement> title;
    title->SetName("title");
    title->SetCharacterData(this->Title, static_cast<int>(std::strlen(this->Title)));
    this->RootNode->AddNestedElement(title);
  }

  if (this->Description && this->Description[0])
  {
    vtkNew<vtkXMLDataElement> desc;
    desc->SetName("desc");
    desc->SetCharacterData(this->Description, static_cast<int>(std::strlen(this->Description)));
    this->RootNode->AddNestedElement(desc);
  }

  this->DefinitionNode = vtkXMLDataElement::New();
  this->RootNode->AddNestedElement(this->DefinitionNode);
  this->DefinitionNode->Delete();
  this->DefinitionNode->SetName("defs");

  this->PageNode = vtkXMLDataElement::New();
  this->RootNode->AddNestedElement(this->PageNode);
  this->PageNode->Delete();
  this->PageNode->SetName("g");

  // Setup the page to not fill or stroke anything by default. Otherwise we'd
  // have to have fill="none" or stroke="none" on every primitive, since we
  // never do both at once in the Context2D API.
  this->PageNode->SetAttribute("stroke", "none");
  this->PageNode->SetAttribute("fill", "none");

  this->Device = vtkSVGContextDevice2D::New();
  this->Device->SetSVGContext(this->PageNode, this->DefinitionNode);
  this->Device->SetTextAsPath(this->TextAsPath);
  this->Device->SetSubdivisionThreshold(this->SubdivisionThreshold);
}

//------------------------------------------------------------------------------
void vtkSVGExporter::RenderContextActors()
{
  vtkRendererCollection* renCol = this->RenderWindow->GetRenderers();
  int numLayers = this->RenderWindow->GetNumberOfLayers();

  for (int i = 0; i < numLayers; ++i)
  {
    vtkCollectionSimpleIterator renIt;
    vtkRenderer* ren;
    for (renCol->InitTraversal(renIt); (ren = renCol->GetNextRenderer(renIt));)
    {
      if (this->ActiveRenderer && ren != this->ActiveRenderer)
      {
        // If ActiveRenderer is specified then ignore all other renderers
        continue;
      }
      if (ren->GetLayer() == i)
      {
        if (this->DrawBackground)
        {
          this->RenderBackground(ren);
        }

        vtkPropCollection* props = ren->GetViewProps();
        vtkCollectionSimpleIterator propIt;
        vtkProp* prop;
        for (props->InitTraversal(propIt); (prop = props->GetNextProp(propIt));)
        {
          vtkContextActor* actor = vtkContextActor::SafeDownCast(prop);
          if (actor)
          {
            this->RenderContextActor(actor, ren);
          }
        }
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkSVGExporter::RenderBackground(vtkRenderer* ren)
{
  if (ren->Transparent())
  {
    return;
  }

  int* renOrigin = ren->GetOrigin();
  const int* renSize = ren->GetSize();
  vtkRectf renRect(renOrigin[0], renOrigin[1], renSize[0], renSize[1]);

  vtkNew<vtkContext2D> ctx;
  ctx->Begin(this->Device);
  this->Device->Begin(ren);

  if (ren->GetTexturedBackground())
  {
    vtkTexture* tex = ren->GetBackgroundTexture();
    vtkImageData* image = tex->GetInput();

    ctx->DrawImage(renRect, image);
  }
  else if (ren->GetGradientBackground())
  {
    std::ostringstream gradIdStream;
    gradIdStream << "bgGrad_" << ren;
    const std::string gradId = gradIdStream.str();

    std::array<double, 3> rgb1d;
    std::array<double, 3> rgb2d;
    ren->GetBackground(rgb1d.data());
    ren->GetBackground2(rgb2d.data());
    double a = ren->GetBackgroundAlpha();

    std::array<unsigned char, 3> rgb1;
    std::array<unsigned char, 3> rgb2;
    for (size_t i = 0; i < 3; ++i)
    {
      rgb1[i] = static_cast<unsigned char>(rgb1d[i] * 255);
      rgb2[i] = static_cast<unsigned char>(rgb2d[i] * 255);
    }

    const float canvasHeight = ren->GetVTKWindow()->GetSize()[1];

    vtkNew<vtkXMLDataElement> gradient;
    this->DefinitionNode->AddNestedElement(gradient);
    gradient->SetName("linearGradient");
    gradient->SetAttribute("id", gradId.c_str());
    gradient->SetAttribute("gradientUnits", "objectBoundingBox");
    gradient->SetIntAttribute("x1", 0);
    gradient->SetIntAttribute("y1", 1);
    gradient->SetIntAttribute("x2", 0);
    gradient->SetIntAttribute("y2", 0);

    vtkNew<vtkXMLDataElement> stop1;
    gradient->AddNestedElement(stop1);
    stop1->SetName("stop");
    stop1->SetAttribute("offset", "0%");
    stop1->SetAttribute("stop-color", ColorToString(rgb1.data()).c_str());

    vtkNew<vtkXMLDataElement> stop2;
    gradient->AddNestedElement(stop2);
    stop2->SetName("stop");
    stop2->SetAttribute("offset", "100%");
    stop2->SetAttribute("stop-color", ColorToString(rgb2.data()).c_str());

    vtkNew<vtkXMLDataElement> rect;
    this->PageNode->AddNestedElement(rect);
    rect->SetName("rect");
    rect->SetAttribute("fill", (std::string("url(#") + gradId + ")").c_str());
    rect->SetFloatAttribute("fill-opacity", static_cast<float>(a));
    rect->SetFloatAttribute("x", renRect.GetLeft());
    rect->SetFloatAttribute("y", canvasHeight - renRect.GetTop());
    rect->SetFloatAttribute("width", renRect.GetWidth());
    rect->SetFloatAttribute("height", renRect.GetHeight());
  }
  else
  {
    std::array<double, 3> rgb;
    ren->GetBackground(rgb.data());
    double a = ren->GetBackgroundAlpha();
    ctx->GetBrush()->SetColor(static_cast<unsigned char>(rgb[0] * 255),
      static_cast<unsigned char>(rgb[1] * 255), static_cast<unsigned char>(rgb[2] * 255),
      static_cast<unsigned char>(a * 255));

    // Draw the rect directly on the device. Context2D::DrawRect also strokes
    // the path...
    std::array<float, 8> poly = { { renRect.GetLeft(), renRect.GetBottom(), renRect.GetRight(),
      renRect.GetBottom(), renRect.GetRight(), renRect.GetTop(), renRect.GetLeft(),
      renRect.GetTop() } };
    this->Device->DrawPolygon(poly.data(), 4);
  }

  ctx->End();
}

//------------------------------------------------------------------------------
void vtkSVGExporter::RenderContextActor(vtkContextActor* actor, vtkRenderer* ren)
{
  vtkContextDevice2D* oldForceDevice = actor->GetForceDevice();
  actor->SetForceDevice(this->Device);
  actor->RenderOverlay(ren);
  actor->SetForceDevice(oldForceDevice);
}
