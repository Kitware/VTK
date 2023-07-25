// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTextRepresentation
 * @brief   represent text for vtkTextWidget
 *
 * This class represents text for a vtkTextWidget.  This class provides
 * support for interactively placing text on the 2D overlay plane. The text
 * is defined by an instance of vtkTextActor.
 *
 * @sa
 * vtkTextRepresentation vtkBorderWidget vtkAbstractWidget vtkWidgetRepresentation
 */

#ifndef vtkTextRepresentation_h
#define vtkTextRepresentation_h

#include "vtkBorderRepresentation.h"
#include "vtkInteractionWidgetsModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkRenderer;
class vtkTextActor;
class vtkTextProperty;
class vtkTextRepresentationObserver;

class VTKINTERACTIONWIDGETS_EXPORT vtkTextRepresentation : public vtkBorderRepresentation
{
public:
  /**
   * Instantiate class.
   */
  static vtkTextRepresentation* New();

  ///@{
  /**
   * Standard VTK methods.
   */
  vtkTypeMacro(vtkTextRepresentation, vtkBorderRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Specify the vtkTextActor to manage. If not specified, then one
   * is automatically created.
   */
  void SetTextActor(vtkTextActor* textActor);
  vtkGetObjectMacro(TextActor, vtkTextActor);
  ///@}

  ///@{
  /**
   * Get/Set the text string display by this representation.
   */
  void SetText(const char* text);
  const char* GetText();
  ///@}

  /**
   * Satisfy the superclasses API.
   */
  void BuildRepresentation() override;
  void GetSize(double size[2]) override
  {
    size[0] = 2.0;
    size[1] = 2.0;
  }

  ///@{
  /**
   * These methods are necessary to make this representation behave as
   * a vtkProp.
   */
  void GetActors2D(vtkPropCollection*) override;
  void ReleaseGraphicsResources(vtkWindow*) override;
  int RenderOverlay(vtkViewport*) override;
  int RenderOpaqueGeometry(vtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(vtkViewport*) override;
  vtkTypeBool HasTranslucentPolygonalGeometry() override;
  ///@}

  /**
   * Set the text position, by enumeration (
   * vtkBorderRepresentation::AnyLocation = 0,
   * vtkBorderRepresentation::LowerLeftCorner,
   * vtkBorderRepresentation::LowerRightCorner,
   * vtkBorderRepresentation::LowerCenter,
   * vtkBorderRepresentation::UpperLeftCorner,
   * vtkBorderRepresentation::UpperRightCorner,
   * vtkBorderRepresentation::UpperCenter)
   * related to the render window
   */
  void SetWindowLocation(int enumLocation) override;

  ///@{
  /**
   * Set the text position, by overriding the same function of
   * vtkBorderRepresentation so that the Modified() will be called.
   */
  void SetPosition(double x, double y) override;
  void SetPosition(double pos[2]) override { this->SetPosition(pos[0], pos[1]); }
  ///@}

  ///@{
  /**
   * Internal. Execute events observed by internal observer
   */
  void ExecuteTextPropertyModifiedEvent(vtkObject* obj, unsigned long enumEvent, void* p);
  void ExecuteTextActorModifiedEvent(vtkObject* obj, unsigned long enumEvent, void* p);
  ///@}

  ///@{
  /**
   * Set/Get the padding between the text and the left border,
   * in pixels unit.
   * Default is 0.
   */
  vtkSetClampMacro(PaddingLeft, int, 0, 4000);
  vtkGetMacro(PaddingLeft, int);
  ///@}

  ///@{
  /**
   * Set/Get the padding between the text and the right border,
   * in pixels unit.
   * Default is 0.
   */
  vtkSetClampMacro(PaddingRight, int, 0, 4000);
  vtkGetMacro(PaddingRight, int);
  ///@}

  ///@{
  /**
   * Set/Get the padding between the text and the top border,
   * in pixels unit.
   * Default is 0.
   */
  vtkSetClampMacro(PaddingTop, int, 0, 4000);
  vtkGetMacro(PaddingTop, int);
  ///@}

  ///@{
  /**
   * Set/Get the padding between the text and the bottom border,
   * in pixels unit.
   * Default is 0.
   */
  vtkSetClampMacro(PaddingBottom, int, 0, 4000);
  vtkGetMacro(PaddingBottom, int);
  ///@}

  ///@{
  /**
   * Set the padding between the text and the left/right/top/bottom
   * border, in pixels unit.
   * Default is 0.
   */
  void SetPadding(int padding);
  ///@}

protected:
  vtkTextRepresentation();
  ~vtkTextRepresentation() override;

  // Initialize text actor
  virtual void InitializeTextActor();

  // Check and adjust boundaries according to the size of the text
  virtual void CheckTextBoundary();

  // the text to manage
  vtkTextActor* TextActor;
  vtkTextProperty* TextProperty;

  // observer to observe internal TextActor and TextProperty
  vtkTextRepresentationObserver* Observer;

  int PaddingLeft = 0;
  int PaddingRight = 0;
  int PaddingTop = 0;
  int PaddingBottom = 0;

private:
  vtkTextRepresentation(const vtkTextRepresentation&) = delete;
  void operator=(const vtkTextRepresentation&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
