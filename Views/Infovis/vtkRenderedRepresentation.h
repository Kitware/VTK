// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkRenderedRepresentation
 *
 *
 */

#ifndef vtkRenderedRepresentation_h
#define vtkRenderedRepresentation_h

#include "vtkDataRepresentation.h"
#include "vtkSmartPointer.h"       // for SP ivars
#include "vtkViewsInfovisModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkApplyColors;
class vtkProp;
class vtkRenderView;
class vtkRenderWindow;
class vtkTextProperty;
class vtkTexture;
class vtkView;

class VTKVIEWSINFOVIS_EXPORT vtkRenderedRepresentation : public vtkDataRepresentation
{
public:
  static vtkRenderedRepresentation* New();
  vtkTypeMacro(vtkRenderedRepresentation, vtkDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set the label render mode.
   * vtkRenderView::QT - Use Qt-based labeler with fitted labeling
   * and unicode support. Requires VTK_USE_QT to be on.
   * vtkRenderView::FREETYPE - Use standard freetype text rendering.
   */
  vtkSetMacro(LabelRenderMode, int);
  vtkGetMacro(LabelRenderMode, int);
  ///@}

protected:
  vtkRenderedRepresentation();
  ~vtkRenderedRepresentation() override;

  ///@{
  /**
   * Subclasses may call these methods to add or remove props from the representation.
   * Use these if the number of props/actors changes as the result of input connection
   * changes.
   */
  void AddPropOnNextRender(vtkProp* p);
  void RemovePropOnNextRender(vtkProp* p);
  ///@}

  /**
   * Obtains the hover text for a particular prop and cell.
   * If the prop is not applicable to the representation, return an empty string.
   * Subclasses should override GetHoverStringInternal, in which the prop and cell
   * are converted to an appropriate selection using ConvertSelection().
   */
  std::string GetHoverString(vtkView* view, vtkProp* prop, vtkIdType cell);

  /**
   * Subclasses may override this method to generate the hover text.
   */
  virtual std::string GetHoverStringInternal(vtkSelection*) { return ""; }

  /**
   * The view will call this method before every render.
   * Representations may add their own pre-render logic here.
   */
  virtual void PrepareForRendering(vtkRenderView* view);

  friend class vtkRenderView;

  int LabelRenderMode;

private:
  vtkRenderedRepresentation(const vtkRenderedRepresentation&) = delete;
  void operator=(const vtkRenderedRepresentation&) = delete;

  class Internals;
  Internals* Implementation;
};

VTK_ABI_NAMESPACE_END
#endif
