/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOpenVRHardwarePicker.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenVRHardwarePicker.h"

#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkDataObject.h"
#include "vtkHardwareSelector.h"
#include "vtkObjectFactory.h"
#include "vtkOpenVRRenderWindow.h"
#include "vtkOpenVRRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSelection.h"
#include "vtkTransform.h"

vtkStandardNewMacro(vtkOpenVRHardwarePicker);

vtkOpenVRHardwarePicker::vtkOpenVRHardwarePicker()
{
  this->Selection = nullptr;
}

vtkOpenVRHardwarePicker::~vtkOpenVRHardwarePicker()
{
  if (this->Selection)
  {
    this->Selection->Delete();
  }
}

// set up for a pick
void vtkOpenVRHardwarePicker::Initialize()
{
  this->vtkAbstractPropPicker::Initialize();
}

// Pick from the given collection
int vtkOpenVRHardwarePicker::PickProp(
  double p0[3], double wxyz[4], vtkRenderer* renderer, vtkPropCollection*, bool actorPassOnly)
{
  //  Initialize picking process
  this->Initialize();
  this->Renderer = renderer;

  // Invoke start pick method if defined
  this->InvokeEvent(vtkCommand::StartPickEvent, nullptr);

  vtkOpenVRRenderWindow* renWin = vtkOpenVRRenderWindow::SafeDownCast(renderer->GetRenderWindow());
  if (!renWin)
  {
    return 0;
  }

  vtkNew<vtkHardwareSelector> sel;
  sel->SetFieldAssociation(vtkDataObject::FIELD_ASSOCIATION_CELLS);
  sel->SetRenderer(renderer);
  sel->SetActorPassOnly(actorPassOnly);
  vtkCamera* oldcam = renderer->GetActiveCamera();
  renWin->SetTrackHMD(false);

  vtkNew<vtkTransform> tran;
  tran->RotateWXYZ(wxyz[0], wxyz[1], wxyz[2], wxyz[3]);
  double pin[4] = { 0.0, 0.0, -1.0, 1.0 };
  double dop[4];
  tran->MultiplyPoint(pin, dop);
  double distance = oldcam->GetDistance();
  oldcam->SetPosition(p0);
  oldcam->SetFocalPoint(
    p0[0] + dop[0] * distance, p0[1] + dop[1] * distance, p0[2] + dop[2] * distance);
  oldcam->OrthogonalizeViewUp();

  const int* size = renderer->GetSize();

  sel->SetArea(size[0] / 2 - 5, size[1] / 2 - 5, size[0] / 2 + 5, size[1] / 2 + 5);

  if (this->Selection)
  {
    this->Selection->Delete();
  }

  this->Selection = nullptr;
  if (sel->CaptureBuffers())
  {
    unsigned int outPos[2];
    unsigned int inPos[2] = { static_cast<unsigned int>(size[0] / 2),
      static_cast<unsigned int>(size[1] / 2) };
    // find the data closest to the center
    vtkHardwareSelector::PixelInformation pinfo = sel->GetPixelInformation(inPos, 5, outPos);
    if (pinfo.Valid)
    {
      this->Selection = sel->GenerateSelection(outPos[0], outPos[1], outPos[0], outPos[1]);
    }
  }

  // this->Selection = sel->Select();
  // sel->SetArea(0, 0, size[0]-1, size[1]-1);

  renWin->SetTrackHMD(true);

  this->InvokeEvent(vtkCommand::EndPickEvent, this->Selection);

  if (this->Selection && this->Selection->GetNode(0))
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

void vtkOpenVRHardwarePicker::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
