// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef TestAxisActor2DInternal_h
#define TestAxisActor2DInternal_h

#include "vtkTestUtilities.h"

#include "vtkAxisActor2D.h"
#include "vtkCamera.h"
#include "vtkNew.h"
#include "vtkNumberToString.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty2D.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTextMapper.h"
#include "vtkTextProperty.h"

VTK_ABI_NAMESPACE_BEGIN

// starting point in viewport coordinate. X = Y = 30
static constexpr int START_POINT = 30;
static constexpr int END_POINT = 270;

/**
 * Mock vtkAxisActor2D to access inner member for comparison.
 */
class vtkAxisActor2DMock : public vtkAxisActor2D
{
public:
  vtkTypeMacro(vtkAxisActor2DMock, vtkAxisActor2D);
  static vtkAxisActor2DMock* New() { VTK_STANDARD_NEW_BODY(vtkAxisActor2DMock); }

  bool CompareLabelMapperString(const std::vector<std::string>& strings)
  {
    int nbOfLabels = this->NumberOfLabelsBuilt;
    if (nbOfLabels != static_cast<int>(strings.size()))
    {
      vtkErrorMacro(
        "Wrong number of labels strings: has " << nbOfLabels << " instead of " << strings.size());
      return false;
    }
    for (int index = 0; index < nbOfLabels; index++)
    {
      vtkTextMapper* mapper = this->LabelMappers[index];
      std::string label = mapper->GetInput();
      if (label != strings[index])
      {
        vtkErrorMacro("Wrong label: <" << label << "> instead of <" << strings[index] << ">");
        return false;
      }
    }
    return true;
  }
};

/**
 * Compare the provided vtkPoints with TickPositions from axis.
 */
inline bool CompareTicksPosition(
  vtkAxisActor2D* axis, vtkRenderWindow* window, vtkPoints* expectedPoints)
{
  window->Render();
  vtkPoints* points = axis->GetTickPositions();

  vtkNew<vtkPolyData> ticks;
  ticks->SetPoints(points);
  if (ticks->GetNumberOfPoints() == 0 && expectedPoints->GetNumberOfPoints() == 0)
  {
    // ComparePoints raises error with empty vtkPoints, shortcuts it.
    return true;
  }

  vtkNew<vtkPolyData> expectedTicks;
  expectedTicks->SetPoints(expectedPoints);
  return vtkTestUtilities::ComparePoints(ticks, expectedTicks);
}

//------------------------------------------------------------------------------
inline void SetupPipeline(vtkAxisActor2D* axis, vtkRenderWindow* window)
{
  // create a diagonal in render view, with some margins
  axis->SetPoint1(0.1, 0.1);
  axis->SetPoint2(0.9, 0.9);

  vtkNew<vtkSphereSource> sphereSource;
  vtkNew<vtkPolyDataMapper> sphereMapper;
  sphereMapper->SetInputConnection(sphereSource->GetOutputPort());
  vtkNew<vtkActor> sphereActor;
  sphereActor->SetMapper(sphereMapper);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(axis);
  renderer->AddActor(sphereActor);
  renderer->GetActiveCamera()->ParallelProjectionOn();

  window->SetSize(300, 300);
  window->AddRenderer(renderer);
  window->Render();
}

VTK_ABI_NAMESPACE_END
#endif
