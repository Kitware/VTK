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
// .NAME vtkTextRepresentation - represent text for vtkTextWidget
// .SECTION Description
// This class represents text for a vtkTextWidget.  This class provides
// support for interactively placing text on the 2D overlay plane. The text
// is defined by an instance of vtkTextActor.

// .SECTION See Also
// vtkTextRepresentation vtkBorderWidget vtkAbstractWidget vtkWidgetRepresentation


#ifndef vtkTextRepresentation_h
#define vtkTextRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkBorderRepresentation.h"

class vtkRenderer;
class vtkTextActor;
class vtkTextProperty;
class vtkTextRepresentationObserver;

class VTKINTERACTIONWIDGETS_EXPORT vtkTextRepresentation : public vtkBorderRepresentation
{
public:
  // Description:
  // Instantiate class.
  static vtkTextRepresentation *New();

  // Description:
  // Standard VTK methods.
  vtkTypeMacro(vtkTextRepresentation,vtkBorderRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the vtkTextActor to manage. If not specified, then one
  // is automatically created.
  void SetTextActor(vtkTextActor *textActor);
  vtkGetObjectMacro(TextActor,vtkTextActor);

  // Description:
  // Get/Set the text string display by this representation.
  void SetText(const char* text);
  const char* GetText();

  // Description:
  // Satisfy the superclasses API.
  virtual void BuildRepresentation();
  virtual void GetSize(double size[2])
    {size[0]=2.0; size[1]=2.0;}

  // Description:
  // These methods are necessary to make this representation behave as
  // a vtkProp.
  virtual void GetActors2D(vtkPropCollection*);
  virtual void ReleaseGraphicsResources(vtkWindow*);
  virtual int RenderOverlay(vtkViewport*);
  virtual int RenderOpaqueGeometry(vtkViewport*);
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport*);
  virtual int HasTranslucentPolygonalGeometry();

  //BTX
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
  //ETX

  // Description:
  // Set the text position, by enumeration (
    // AnyLocation = 0,
    // LowerLeftCorner,
    // LowerRightCorner,
    // LowerCenter,
    // UpperLeftCorner,
    // UpperRightCorner,
    // UpperCenter)
  // related to the render window
  virtual void SetWindowLocation(int enumLocation);
  vtkGetMacro(WindowLocation, int);

  // Description:
  // Set the text position, by overiding the same function of
  // vtkBorderRepresentation so that the Modified() will be called.
  virtual void SetPosition(double x, double y);
  virtual void SetPosition(double pos[2])
    { this->SetPosition(pos[0], pos[1]);};

  // Description:
  // Internal. Execute events observed by internal observer
  void ExecuteTextPropertyModifiedEvent(vtkObject* obj, unsigned long enumEvent, void* p);
  void ExecuteTextActorModifiedEvent(vtkObject* obj, unsigned long enumEvent, void* p);

protected:
  vtkTextRepresentation();
  ~vtkTextRepresentation();

  // Initialize text actor
  virtual void InitializeTextActor();

  // Check and adjust boundaries according to the size of the text
  virtual void CheckTextBoundary();

  // the text to manage
  vtkTextActor  *TextActor;
  vtkTextProperty *TextProperty;

  // Window location by enumeration
  int WindowLocation;
  virtual void UpdateWindowLocation();

  // observer to observe internal TextActor and TextProperty
  vtkTextRepresentationObserver *Observer;

private:
  vtkTextRepresentation(const vtkTextRepresentation&);  //Not implemented
  void operator=(const vtkTextRepresentation&);  //Not implemented
};

#endif
