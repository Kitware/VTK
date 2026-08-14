// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkTextOverlayRepresentation
 * @brief   A line of text drawn over the scene.
 *
 * vtkTextOverlayRepresentation draws a string at a fixed position in the render
 * window, on top of whatever else is being drawn -- a title, a frame number, a
 * units label.  Create one, say what it should read and where it goes, and add
 * it to a view:
 *
 * @code
 * vtkNew<vtkTextOverlayRepresentation> title;
 * title->SetText("Frame 12");
 * title->SetPosition(20, 20);
 * title->GetTextProperty()->SetFontSize(24);
 * view->AddRepresentation(title);
 * @endcode
 *
 * @par A representation with no data:
 * There is no input to connect and no array to color by.  A text overlay is
 * shown, hidden, added and removed exactly as a surface or a volume is, which
 * is the whole of what a view asks of the things it shows, so this derives from
 * vtkScivisRepresentation rather than from vtkScivisDataRepresentation and
 * carries none of the machinery -- an input, a selection, a color map, a range
 * -- that would mean nothing here.  A view passes it over when it does
 * something that only data can answer, such as spanning a scalar bar over an
 * array.  It does not move the camera either, being drawn in two dimensions
 * over the scene rather than sitting in it.
 *
 * @par What is here and what is not:
 * The text and where it sits are this representation's own.  How it is drawn --
 * font, size, color, opacity, bold, italic, justification -- belongs to
 * vtkTextProperty, which GetTextProperty() hands out rather than mirroring it
 * one method at a time.
 *
 * @sa vtkScivisRepresentation vtkScivisView vtkTextProperty
 */

#ifndef vtkTextOverlayRepresentation_h
#define vtkTextOverlayRepresentation_h

#include "vtkNew.h" // For ivar
#include "vtkScivisRepresentation.h"
#include "vtkViewsScivisModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkTextActor;
class vtkTextProperty;

class VTKVIEWSSCIVIS_EXPORT vtkTextOverlayRepresentation : public vtkScivisRepresentation
{
public:
  static vtkTextOverlayRepresentation* New();
  vtkTypeMacro(vtkTextOverlayRepresentation, vtkScivisRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Whether the text is drawn.  Default is on.
   */
  void SetVisibility(bool val) override;
  bool GetVisibility() override;
  ///@}

  ///@{
  /**
   * The string to draw.
   */
  void SetText(const char* text);
  const char* GetText();
  ///@}

  ///@{
  /**
   * Where the text sits, in pixels from the lower left corner of the render
   * window.  Which part of the text lands on that point is the text property's
   * justification.
   */
  void SetPosition(int x, int y);
  double* GetPosition() VTK_SIZEHINT(2);
  ///@}

  /**
   * How the text is drawn: its font and size, color and opacity, bold and
   * italic, and its justification about the position above.
   */
  vtkTextProperty* GetTextProperty();

  /**
   * The actor the text is drawn by, for what neither this representation nor
   * the text property covers -- a scaled or fitted text box, or a position that
   * follows a point in the scene.
   */
  vtkTextActor* GetTextActor();

protected:
  vtkTextOverlayRepresentation();
  ~vtkTextOverlayRepresentation() override;

  bool AddToView(vtkScivisView* view) override;
  bool RemoveFromView(vtkScivisView* view) override;

private:
  vtkTextOverlayRepresentation(const vtkTextOverlayRepresentation&) = delete;
  void operator=(const vtkTextOverlayRepresentation&) = delete;

  vtkNew<vtkTextActor> TextActor;
};

VTK_ABI_NAMESPACE_END
#endif
