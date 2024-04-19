// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkClientServerSynchronizedRenderers.h"

#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"

#include <cassert>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkClientServerSynchronizedRenderers);
//------------------------------------------------------------------------------
vtkClientServerSynchronizedRenderers::vtkClientServerSynchronizedRenderers() = default;

//------------------------------------------------------------------------------
vtkClientServerSynchronizedRenderers::~vtkClientServerSynchronizedRenderers() = default;

//------------------------------------------------------------------------------
void vtkClientServerSynchronizedRenderers::MasterEndRender()
{
  // receive image from slave.
  assert(this->ParallelController->IsA("vtkSocketController"));

  vtkRawImage& rawImage = this->Image;

  int header[4];
  this->ParallelController->Receive(header, 4, 1, 0x023430);
  if (header[0] > 0)
  {
    rawImage.Resize(header[1], header[2], header[3]);
    this->ParallelController->Receive(rawImage.GetRawPtr(), 1, 0x023430);
    rawImage.MarkValid();
  }
}

//------------------------------------------------------------------------------
void vtkClientServerSynchronizedRenderers::SlaveEndRender()
{
  assert(this->ParallelController->IsA("vtkSocketController"));

  vtkRawImage& rawImage = this->CaptureRenderedImage();

  int header[4];
  header[0] = rawImage.IsValid() ? 1 : 0;
  header[1] = rawImage.GetWidth();
  header[2] = rawImage.GetHeight();
  header[3] = rawImage.IsValid() ? rawImage.GetRawPtr()->GetNumberOfComponents() : 0;

  // send the image to the client.
  this->ParallelController->Send(header, 4, 1, 0x023430);
  if (rawImage.IsValid())
  {
    this->ParallelController->Send(rawImage.GetRawPtr(), 1, 0x023430);
  }
}

//------------------------------------------------------------------------------
void vtkClientServerSynchronizedRenderers::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
