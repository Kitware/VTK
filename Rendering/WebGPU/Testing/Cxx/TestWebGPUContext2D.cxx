// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file TestWebGPUContext2D.cxx
 * @brief Tests basic functionality and text bounding box calculations for the WebGPU 2D context
 * device.
 */

#include "vtkContext2D.h"
#include "vtkContextActor.h"
#include "vtkContextDevice2D.h"
#include "vtkContextItem.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTextProperty.h"

//------------------------------------------------------------------------------
namespace
{
class ContextItem : public vtkContextItem
{
public:
  ContextItem()
    : Succeeded(true)
  {
  }
  static ContextItem* New();
  vtkTypeMacro(ContextItem, vtkContextItem);

  bool Paint(vtkContext2D* painter) override;

  bool Succeeded;
};

//------------------------------------------------------------------------------
vtkStandardNewMacro(ContextItem);

//------------------------------------------------------------------------------
bool IsVector4Same(float expected[4], float computed[4])
{
  // The origin should be within 3 px of the expected value. This is because we
  // align to the text data (ie. actual drawn pixels), not the texture image
  // size, which may include a degree of padding.
  constexpr float originEps = 3.f;
  const bool closeOrigin =
    (fabs(expected[0] - computed[0]) <= originEps && fabs(expected[1] - computed[1]) <= originEps);

  // The width / height should be the same:
  constexpr float sizeEps = 1e-6f;
  const bool sameSize =
    (fabs(expected[2] - computed[2]) <= sizeEps && fabs(expected[3] - computed[3]) <= sizeEps);

  if (!sameSize || !closeOrigin)
  {
    vtkLog(INFO, "Not the same!");
    vtkLog(INFO,
      "Expected: (" << expected[0] << ", " << expected[1] << ", " << expected[2] << ", "
                    << expected[3] << ")");
    vtkLog(INFO,
      "Computed: (" << computed[0] << ", " << computed[1] << ", " << computed[2] << ", "
                    << computed[3] << ")");
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool ContextItem::Paint(vtkContext2D* painter)
{
  const char* text = "Test";

  float expectedUnjustifiedBounds[4];
  painter->ComputeStringBounds(text, expectedUnjustifiedBounds);
  float expectedJustifiedBounds[4] = { 0, 0, expectedUnjustifiedBounds[2],
    expectedUnjustifiedBounds[3] };

  float unjustifiedBounds[4];
  float justifiedBounds[4];

  // Left justification
  painter->GetTextProp()->SetJustification(VTK_TEXT_LEFT);
  painter->ComputeStringBounds(text, unjustifiedBounds);
  vtkLog(INFO, "Left-justified ComputeStringBounds");
  this->Succeeded = this->Succeeded && IsVector4Same(expectedUnjustifiedBounds, unjustifiedBounds);

  painter->ComputeJustifiedStringBounds(text, justifiedBounds);
  vtkLog(INFO, "Left-justified ComputeJustifiedStringBounds");
  this->Succeeded = this->Succeeded && IsVector4Same(expectedJustifiedBounds, justifiedBounds);

  // Center justification
  painter->GetTextProp()->SetJustification(VTK_TEXT_CENTERED);
  painter->ComputeStringBounds(text, unjustifiedBounds);
  vtkLog(INFO, "Center-justified ComputeStringBounds");
  this->Succeeded = this->Succeeded && IsVector4Same(expectedUnjustifiedBounds, unjustifiedBounds);

  expectedJustifiedBounds[0] = -0.5 * expectedUnjustifiedBounds[2]; // negative half the width
  painter->ComputeJustifiedStringBounds(text, justifiedBounds);
  vtkLog(INFO, "Center-justified ComputeJustifiedStringBounds");
  this->Succeeded = this->Succeeded && IsVector4Same(expectedJustifiedBounds, justifiedBounds);

  // Right justification
  painter->GetTextProp()->SetJustification(VTK_TEXT_RIGHT);
  painter->ComputeStringBounds(text, unjustifiedBounds);
  vtkLog(INFO, "Right-justified ComputeStringBounds");
  this->Succeeded = this->Succeeded && IsVector4Same(expectedUnjustifiedBounds, unjustifiedBounds);

  expectedJustifiedBounds[0] = -expectedUnjustifiedBounds[2]; // negative full width
  painter->ComputeJustifiedStringBounds(text, justifiedBounds);
  vtkLog(INFO, "Right-justified result from ComputeJustifiedStringBounds");
  this->Succeeded = this->Succeeded && IsVector4Same(expectedJustifiedBounds, justifiedBounds);

  painter->GetPen()->SetWidth(16.0);
  float markerPoints[] = { 100.0, 100.0, 200.0, 200.0, 100.0, 200.0 };
  painter->DrawMarkers(VTK_MARKER_CIRCLE, false, markerPoints, 1);
  painter->DrawMarkers(VTK_MARKER_DIAMOND, true, markerPoints + 2, 1);
  painter->DrawMarkers(VTK_MARKER_SQUARE, false, markerPoints + 4, 1);

  return true;
}

} // end anonymous namespace

//------------------------------------------------------------------------------
int TestWebGPUContext2D(int, char*[])
{
  // Set up a 2D context view, context test object and add it to the scene
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(300, 300);
  vtkNew<ContextItem> test;
  view->GetScene()->AddItem(test);

  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->Render();

  // Exercise NewInstance for coverage.
  auto dummy = test->NewInstance();
  dummy->Delete();

  return test->Succeeded ? EXIT_SUCCESS : EXIT_FAILURE;
}
