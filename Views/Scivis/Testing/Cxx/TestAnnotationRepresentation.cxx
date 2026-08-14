// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// A representation with no data behind it -- an annotation -- derives from
// vtkScivisRepresentation and implements two methods.  It is not asked for an
// array, a range, a color map or bounds, because it has none of them and the
// contract that requires those lives one level down, on
// vtkScivisDataRepresentation.

#include "vtkNew.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkScivisDataRepresentation.h"
#include "vtkScivisRepresentation.h"
#include "vtkScivisView.h"
#include "vtkTextActor.h"

#include <iostream>

#define CHECK(expr, msg)                                                                           \
  do                                                                                               \
  {                                                                                                \
    if (!(expr))                                                                                   \
    {                                                                                              \
      std::cerr << "FAILED: " << msg << "\n";                                                      \
      return EXIT_FAILURE;                                                                         \
    }                                                                                              \
  } while (false)

namespace
{

// The whole of an annotation: what it draws, and how it comes and goes.
class TextAnnotation : public vtkScivisRepresentation
{
public:
  static TextAnnotation* New() { return new TextAnnotation; }
  vtkTypeMacro(TextAnnotation, vtkScivisRepresentation);

  void SetVisibility(bool val) override { this->Text->SetVisibility(val); }
  bool GetVisibility() override { return this->Text->GetVisibility() != 0; }

  vtkTextActor* GetTextActor() { return this->Text; }

protected:
  TextAnnotation() { this->SetNumberOfInputPorts(0); }
  ~TextAnnotation() override = default;

  bool AddToView(vtkScivisView* view) override
  {
    view->GetRenderer()->AddViewProp(this->Text);
    return true;
  }
  bool RemoveFromView(vtkScivisView* view) override
  {
    view->GetRenderer()->RemoveViewProp(this->Text);
    return true;
  }

private:
  vtkNew<vtkTextActor> Text;
};

}

int TestAnnotationRepresentation(int, char*[])
{
  vtkNew<vtkScivisView> view;
  view->GetRenderWindow()->SetOffScreenRendering(true);
  view->SetWindowSize(300, 300);

  vtkNew<TextAnnotation> title;
  title->GetTextActor()->SetInput("Frame 12");

  view->AddRepresentation(title);
  CHECK(view->GetNumberOfRepresentations() == 1, "the annotation was not accepted by the view");
  CHECK(view->GetRepresentation(0) == title.Get(), "the view is holding something else");

  // It is a representation, but not one with data behind it -- so nothing asks
  // it for an array or a range.
  CHECK(vtkScivisDataRepresentation::SafeDownCast(title) == nullptr,
    "an annotation should not be a data representation");

  CHECK(title->GetVisibility(), "an annotation starts hidden");
  title->SetVisibility(false);
  CHECK(!title->GetVisibility(), "hiding the annotation did not take");
  title->SetVisibility(true);

  view->ResetCamera();
  view->Render();

  view->RemoveRepresentation(title);
  CHECK(view->GetNumberOfRepresentations() == 0, "the annotation was not removed");

  return EXIT_SUCCESS;
}
