/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRenderWidget.h"

#include "vtkAbstractInteractionDevice.h"
#include "vtkAbstractRenderDevice.h"
#include "vtkObjectFactory.h"
#include "vtkRect.h"

vtkStandardNewMacro(vtkRenderWidget);

vtkRenderWidget::vtkRenderWidget()
  : Position(0, 0)
  , Size(300, 300)
  , Name("New VTK RenderWidget!!!")
{
}

vtkRenderWidget::~vtkRenderWidget() = default;

void vtkRenderWidget::SetPosition(const vtkVector2i& pos)
{
  if (this->Position != pos)
  {
    this->Position = pos;
    this->Modified();
  }
}

void vtkRenderWidget::SetSize(const vtkVector2i& size)
{
  if (this->Size != size)
  {
    this->Size = size;
    this->Modified();
  }
}

void vtkRenderWidget::SetName(const std::string& name)
{
  if (this->Name != name)
  {
    this->Name = name;
    this->Modified();
  }
}

void vtkRenderWidget::Render()
{
  assert(this->RenderDevice != nullptr);
  cout << "Render called!!!" << endl;
}

void vtkRenderWidget::MakeCurrent()
{
  assert(this->RenderDevice != nullptr);
  this->RenderDevice->MakeCurrent();
}

void vtkRenderWidget::Initialize()
{
  assert(this->RenderDevice != nullptr && this->InteractionDevice != nullptr);
  this->InteractionDevice->SetRenderWidget(this);
  this->InteractionDevice->SetRenderDevice(this->RenderDevice);
  this->RenderDevice->CreateNewWindow(
    vtkRecti(this->Position.GetX(), this->Position.GetY(), this->Size.GetX(), this->Size.GetY()),
    this->Name);
  this->InteractionDevice->Initialize();
}

void vtkRenderWidget::Start()
{
  assert(this->InteractionDevice != nullptr);
  this->Initialize();
  this->InteractionDevice->Start();
}

void vtkRenderWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  // FIXME: Add methods for this...
  this->Superclass::PrintSelf(os, indent);
}
