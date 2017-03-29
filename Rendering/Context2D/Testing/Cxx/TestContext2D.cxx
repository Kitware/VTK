/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestContext2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkContext2D.h"
#include "vtkContextActor.h"
#include "vtkContextDevice2D.h"
#include "vtkContextItem.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLContextDevice2D.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkTextProperty.h"


//----------------------------------------------------------------------------
namespace {
class ContextItem : public vtkContextItem
{
public:
  ContextItem() : Succeeded(true) {}
  static ContextItem *New();
  vtkTypeMacro(ContextItem, vtkContextItem);

  bool Paint(vtkContext2D* painter) VTK_OVERRIDE;

  bool Succeeded;
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(ContextItem);

//----------------------------------------------------------------------------
bool IsVector4Same(float expected[4], float computed[4])
{

  // The origin should be with 3 px of the expected value. This is because we
  // align to the text data (ie. actual drawn pixels), not the texture image
  // size, which may include a degree of padding.
  const float originEps = 3.f;
  const bool closeOrigin = (fabs(expected[0] - computed[0]) <= originEps &&
                            fabs(expected[1] - computed[1]) <= originEps);

  // The width / height should be the same:
  const float sizeEps = 1e-6f;
  const bool sameSize = (fabs(expected[2] - computed[2]) <= sizeEps &&
                         fabs(expected[3] - computed[3]) <= sizeEps);

  if (!sameSize || !closeOrigin)
  {
    std::cout << "Not the same!\n";
    std::cout << "Expected: (" << expected[0] << ", " << expected[1] << ", "
              << expected[2] << ", " << expected[3] << ")\n";
    std::cout << "Computed: (" << computed[0] << ", " << computed[1] << ", "
              << computed[2] << ", " << computed[3] << ")\n";
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
bool ContextItem::Paint(vtkContext2D* painter)
{
  const char* text = "Test";

  float expectedUnjustifiedBounds[4];
  painter->ComputeStringBounds(text, expectedUnjustifiedBounds);
  float expectedJustifiedBounds[4] = {0, 0, expectedUnjustifiedBounds[2], expectedUnjustifiedBounds[3]};

  float unjustifiedBounds[4];
  float justifiedBounds[4];

  // Left justification
  painter->GetTextProp()->SetJustification(VTK_TEXT_LEFT);
  painter->ComputeStringBounds(text, unjustifiedBounds);
  std::cout << "Left-justified ComputeStringBounds\n";
  this->Succeeded = this->Succeeded && IsVector4Same(expectedUnjustifiedBounds, unjustifiedBounds);

  painter->ComputeJustifiedStringBounds(text, justifiedBounds);
  std::cout << "Left-justified ComputeJustifiedStringBounds\n";
  this->Succeeded = this->Succeeded && IsVector4Same(expectedJustifiedBounds, justifiedBounds);

  // Center justification
  painter->GetTextProp()->SetJustification(VTK_TEXT_CENTERED);
  painter->ComputeStringBounds(text, unjustifiedBounds);
  std::cout << "Center-justified ComputeStringBounds\n";
  this->Succeeded = this->Succeeded && IsVector4Same(expectedUnjustifiedBounds, unjustifiedBounds);

  expectedJustifiedBounds[0] = -0.5*expectedUnjustifiedBounds[2]; // negative half the width
  painter->ComputeJustifiedStringBounds(text, justifiedBounds);
  std::cout << "Center-justified ComputeJustifiedStringBounds\n";
  this->Succeeded = this->Succeeded && IsVector4Same(expectedJustifiedBounds, justifiedBounds);

  // Right justification
  painter->GetTextProp()->SetJustification(VTK_TEXT_RIGHT);
  painter->ComputeStringBounds(text, unjustifiedBounds);
  std::cout << "Right-justified ComputeStringBounds\n";
  this->Succeeded = this->Succeeded && IsVector4Same(expectedUnjustifiedBounds, unjustifiedBounds);

  expectedJustifiedBounds[0] = -expectedUnjustifiedBounds[2]; // negative full width
  painter->ComputeJustifiedStringBounds(text, justifiedBounds);
  std::cout << "Right-justified result from ComputeJustifiedStringBounds\n";
  this->Succeeded = this->Succeeded && IsVector4Same(expectedJustifiedBounds, justifiedBounds);

  return true;
}

} // end anonymous namespace

//----------------------------------------------------------------------------
int TestContext2D(int, char*[])
{
  // Set up a 2D context view, context test object and add it to the scene
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(300, 300);
  vtkNew<ContextItem> test;
  view->GetScene()->AddItem(test.GetPointer());

  // Force the use of the freetype based rendering strategy
  vtkOpenGLContextDevice2D::SafeDownCast(view->GetContext()->GetDevice())
    ->SetStringRendererToFreeType();

  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->Render();

  return test->Succeeded ? EXIT_SUCCESS : EXIT_FAILURE;
}
