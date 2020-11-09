/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

  //@{
  /**
   * Standard VTK methods.
   */
  vtkTypeMacro(vtkTextRepresentation, vtkBorderRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  //@{
  /**
   * Specify the vtkTextActor to manage. If not specified, then one
   * is automatically created.
   */
  void SetTextActor(vtkTextActor* textActor);
  vtkGetObjectMacro(TextActor, vtkTextActor);
  //@}

  //@{
  /**
   * Get/Set the text string display by this representation.
   */
  void SetText(const char* text);
  const char* GetText();
  //@}

  /**
   * Satisfy the superclasses API.
   */
  void BuildRepresentation() override;
  void GetSize(double size[2]) override
  {
    size[0] = 2.0;
    size[1] = 2.0;
  }

  //@{
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
  //@}

  enum
  {
    AnyLocation = 0,
    LowerLeftCorner,
    LowerRightCorner,
    LowerCenter,
    UpperLeftCorner,
    UpperRightCorner,
    UpperCenter
  };

  //@{
  /**
   * Set the text position, by enumeration (
   * AnyLocation = 0,
   * LowerLeftCorner,
   * LowerRightCorner,
   * LowerCenter,
   * UpperLeftCorner,
   * UpperRightCorner,
   * UpperCenter)
   * related to the render window
   */
  virtual void SetWindowLocation(int enumLocation);
  vtkGetMacro(WindowLocation, int);
  //@}

  //@{
  /**
   * Set the text position, by overriding the same function of
   * vtkBorderRepresentation so that the Modified() will be called.
   */
  void SetPosition(double x, double y) override;
  void SetPosition(double pos[2]) override { this->SetPosition(pos[0], pos[1]); }
  //@}

  //@{
  /**
   * Internal. Execute events observed by internal observer
   */
  void ExecuteTextPropertyModifiedEvent(vtkObject* obj, unsigned long enumEvent, void* p);
  void ExecuteTextActorModifiedEvent(vtkObject* obj, unsigned long enumEvent, void* p);
  //@}

  //@{
  /**
   * Set/Get the padding between the text and the top border,
   * in pixels unit.
   * To ensure that padding is even, this value is multiplied by 2, so
   * a right padding of 1 means a 2 pixels padding.
   * Default is 0.
   */
  vtkSetClampMacro(RightPadding, int, 0.0, 4000.0);
  vtkGetMacro(RightPadding, int);
  //@}

  //@{
  /**
   * Set/Get the padding between the text and the right border,
   * in pixels unit.
   * To ensure that padding is even, this value is multiplied by 2, so
   * a top padding of 1 means a 2 pixels padding.
   * Default is 0.
   */
  vtkSetClampMacro(TopPadding, int, 0.0, 4000.0);
  vtkGetMacro(TopPadding, int);
  //@}

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

  // Window location by enumeration
  int WindowLocation;
  virtual void UpdateWindowLocation();

  // observer to observe internal TextActor and TextProperty
  vtkTextRepresentationObserver* Observer;

  int RightPadding = 0.0;
  int TopPadding = 0.0;

private:
  vtkTextRepresentation(const vtkTextRepresentation&) = delete;
  void operator=(const vtkTextRepresentation&) = delete;
};

#endif
