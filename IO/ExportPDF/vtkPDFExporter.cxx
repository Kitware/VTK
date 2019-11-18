/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPDFExporter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPDFExporter.h"

#include "vtkContext2D.h"
#include "vtkContextActor.h"
#include "vtkContextScene.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPDFContextDevice2D.h"
#include "vtkProp.h"
#include "vtkPropCollection.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"

#include <vtk_libharu.h>

#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace
{

void HPDF_STDCALL handle_libharu_error(HPDF_STATUS error, HPDF_STATUS detail, void*)
{
  std::ostringstream out;
  out << "LibHaru failed during PDF export. Error=0x" << std::hex << error << " detail=" << std::dec
      << detail;
  throw std::runtime_error(out.str());
}

} // end anon namespace

//------------------------------------------------------------------------------
// vtkPDFExporter::Details
//------------------------------------------------------------------------------

struct vtkPDFExporter::Details
{
  HPDF_Doc Document;
  HPDF_Page Page;
};

//------------------------------------------------------------------------------
// vtkPDFExporter
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkPDFExporter);

//------------------------------------------------------------------------------
void vtkPDFExporter::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkPDFExporter::vtkPDFExporter()
  : Title(nullptr)
  , FileName(nullptr)
  , Impl(new Details)
{
  this->SetTitle("VTK Exported Scene");
}

//------------------------------------------------------------------------------
vtkPDFExporter::~vtkPDFExporter()
{
  this->SetTitle(nullptr);
  this->SetFileName(nullptr);
  delete this->Impl;
}

//------------------------------------------------------------------------------
void vtkPDFExporter::WriteData()
{
  if (!this->FileName || !*this->FileName)
  {
    vtkErrorMacro("FileName not specified.");
    return;
  }

  this->Impl->Document = HPDF_New(handle_libharu_error, nullptr);

  if (!this->Impl->Document)
  {
    vtkErrorMacro("Error initializing LibHaru PDF document: HPDF_New failed.");
    return;
  }

  try
  {
    this->WritePDF();
    HPDF_SaveToFile(this->Impl->Document, this->FileName);
  }
  catch (std::runtime_error& e)
  {
    vtkErrorMacro(<< e.what());
  }

  HPDF_Free(this->Impl->Document);
}

//------------------------------------------------------------------------------
void vtkPDFExporter::WritePDF()
{
  this->PrepareDocument();
  this->RenderContextActors();
}

//------------------------------------------------------------------------------
void vtkPDFExporter::PrepareDocument()
{
  // Compress everything:
  HPDF_SetCompressionMode(this->Impl->Document, HPDF_COMP_ALL);

  // Various metadata:
  HPDF_SetInfoAttr(this->Impl->Document, HPDF_INFO_CREATOR, "The Visualization ToolKit");
  HPDF_SetInfoAttr(this->Impl->Document, HPDF_INFO_TITLE, this->Title);

  this->Impl->Page = HPDF_AddPage(this->Impl->Document);
  HPDF_Page_SetWidth(this->Impl->Page, this->RenderWindow->GetSize()[0]);
  HPDF_Page_SetHeight(this->Impl->Page, this->RenderWindow->GetSize()[1]);
}

//------------------------------------------------------------------------------
void vtkPDFExporter::RenderContextActors()
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
void vtkPDFExporter::RenderContextActor(vtkContextActor* actor, vtkRenderer* ren)
{
  vtkContextDevice2D* oldForceDevice = actor->GetForceDevice();

  vtkNew<vtkPDFContextDevice2D> device;
  device->SetHaruObjects(&this->Impl->Document, &this->Impl->Page);
  device->SetRenderer(ren);
  actor->SetForceDevice(device);

  actor->RenderOverlay(ren);

  actor->SetForceDevice(oldForceDevice);
}
