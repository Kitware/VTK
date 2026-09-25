// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Verifies that hardware point selection on vtkOpenGLSphereMapper reports the id of the
// sphere under the selected pixels.

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkHardwareSelector.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkNew.h"
#include "vtkOpenGLSphereMapper.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"

#include <cmath>
#include <iostream>

int TestSphereMapperSelection(int, char*[])
{
  constexpr int numSpheres = 5;
  vtkNew<vtkPoints> points;
  vtkNew<vtkCellArray> verts;
  for (int i = 0; i < numSpheres; ++i)
  {
    const vtkIdType id = points->InsertNextPoint(2.0 * i, 0.0, 0.0);
    verts->InsertNextCell(1, &id);
  }
  vtkNew<vtkPolyData> poly;
  poly->SetPoints(points);
  poly->SetVerts(verts);

  vtkNew<vtkOpenGLSphereMapper> mapper;
  mapper->SetInputData(poly);
  mapper->SetRadius(0.6f);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(500, 200);
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(renderer);

  vtkCamera* camera = renderer->GetActiveCamera();
  camera->SetPosition(4.0, 0.0, 20.0);
  camera->SetFocalPoint(4.0, 0.0, 0.0);
  camera->SetViewUp(0.0, 1.0, 0.0);
  renderer->ResetCamera();
  renWin->Render();

  vtkNew<vtkHardwareSelector> selector;
  selector->SetFieldAssociation(vtkDataObject::FIELD_ASSOCIATION_POINTS);
  selector->SetRenderer(renderer);

  int failures = 0;
  for (int i = 0; i < numSpheres; ++i)
  {
    renderer->SetWorldPoint(2.0 * i, 0.0, 0.0, 1.0);
    renderer->WorldToDisplay();
    const double* display = renderer->GetDisplayPoint();
    const auto x = static_cast<unsigned int>(std::lround(display[0]));
    const auto y = static_cast<unsigned int>(std::lround(display[1]));
    selector->SetArea(x - 2, y - 2, x + 2, y + 2);
    auto result = vtk::TakeSmartPointer(selector->Select());

    bool goodPick = false;
    if (result->GetNumberOfNodes() == 1)
    {
      vtkSelectionNode* node = result->GetNode(0);
      auto* selIds = vtkArrayDownCast<vtkIdTypeArray>(node->GetSelectionList());
      goodPick = node->GetProperties()->Get(vtkSelectionNode::PROP()) == actor.Get() && selIds &&
        selIds->GetNumberOfTuples() == 1 && selIds->GetValue(0) == i;
      if (!goodPick && selIds)
      {
        std::cerr << "sphere " << i << " picked ids:";
        for (vtkIdType j = 0; j < selIds->GetNumberOfTuples(); ++j)
        {
          std::cerr << ' ' << selIds->GetValue(j);
        }
        std::cerr << '\n';
      }
    }
    else
    {
      std::cerr << "sphere " << i << " produced " << result->GetNumberOfNodes()
                << " selection nodes\n";
    }
    failures += goodPick ? 0 : 1;
  }
  return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
