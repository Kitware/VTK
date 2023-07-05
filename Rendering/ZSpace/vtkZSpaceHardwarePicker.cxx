// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkZSpaceHardwarePicker.h"

#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkHardwareSelector.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkSelection.h"
#include "vtkTransform.h"

#include <chrono> // std::chrono::seconds
#include <thread> // std::this_thread::sleep_for

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkZSpaceHardwarePicker);

//------------------------------------------------------------------------------
vtkSelection* vtkZSpaceHardwarePicker::GetSelection()
{
  return this->Selection;
}

//------------------------------------------------------------------------------
// Perform the picking
int vtkZSpaceHardwarePicker::PickProp(const double pos[3], const double wxyz[4],
  int fieldAssociation, vtkRenderer* renderer, bool actorPassOnly)
{
  // Initialize picking process
  this->Initialize();
  this->Renderer = renderer;

  // Invoke start pick method if defined
  this->InvokeEvent(vtkCommand::StartPickEvent, nullptr);

  // Setup hardware selector
  vtkNew<vtkHardwareSelector> sel;
  sel->SetFieldAssociation(fieldAssociation);
  sel->SetRenderer(renderer);
  sel->SetActorPassOnly(actorPassOnly);

  // Save the current zSpace camera
  vtkSmartPointer<vtkCamera> originalCamera = renderer->GetActiveCamera();

  // Change the camera to disable projection/view matrix of zSpace
  // and to make sure the picked point will be at the middle of the viewport
  vtkNew<vtkCamera> pickingCamera;
  renderer->SetActiveCamera(pickingCamera);

  vtkNew<vtkTransform> transform;
  transform->RotateWXYZ(wxyz[0], wxyz[1], wxyz[2], wxyz[3]);

  double pin[4] = { 0.0, 0.0, -1.0, 1.0 };
  double dop[4];
  transform->MultiplyPoint(pin, dop);

  // Setup the picking camera in order to do the hardware selection
  // at the center of the viewport
  pickingCamera->SetPosition(pos);
  double distance = originalCamera->GetDistance();
  pickingCamera->SetFocalPoint(
    pos[0] + dop[0] * distance, pos[1] + dop[1] * distance, pos[2] + dop[2] * distance);
  pickingCamera->OrthogonalizeViewUp();

  // Set rendering area for capturing buffers
  int pickingRadius = 0;
  if (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    // Picking radius to facilitate point picking
    pickingRadius = this->PointPickingRadius;
  }

  int* size = this->Renderer->GetSize();
  int* origin = this->Renderer->GetOrigin();

  // Clamp radius to renderer size, extreme case (certainly useless)
  int xMin = origin[0] + std::max(0, size[0] / 2 - pickingRadius);
  int ymin = origin[1] + std::max(0, size[1] / 2 - pickingRadius);
  int xMax = origin[0] + std::min(size[0] - 1, size[0] / 2 + pickingRadius);
  int yMax = origin[1] + std::min(size[1] - 1, size[1] / 2 + pickingRadius);

  sel->SetArea(xMin, ymin, xMax, yMax);

  // Generate selection
  this->Selection = nullptr;
  if (sel->CaptureBuffers())
  {
    unsigned int outPos[2];
    unsigned int inPos[2] = { origin[0] + static_cast<unsigned int>(size[0] / 2),
      origin[1] + static_cast<unsigned int>(size[1] / 2) };
    // Pick at the center of the viewport (with a tolerance for point picking)
    vtkHardwareSelector::PixelInformation pinfo =
      sel->GetPixelInformation(inPos, pickingRadius, outPos);
    if (pinfo.Valid)
    {
      this->Selection.TakeReference(
        sel->GenerateSelection(outPos[0], outPos[1], outPos[0], outPos[1]));
    }
  }

  // Restore the original zSpace camera
  renderer->SetActiveCamera(originalCamera);

  // Return 1 if something has been picked, 0 otherwise
  return this->Selection && this->Selection->GetNode(0) ? 1 : 0;
}

//------------------------------------------------------------------------------
void vtkZSpaceHardwarePicker::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->Selection)
  {
    this->Selection->PrintSelf(os, indent);
  }
}
VTK_ABI_NAMESPACE_END
