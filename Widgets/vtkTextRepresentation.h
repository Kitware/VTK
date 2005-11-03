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


#ifndef __vtkTextRepresentation_h
#define __vtkTextRepresentation_h

#include "vtkBorderRepresentation.h"

class vtkRenderer;
class vtkTextActor;

class VTK_WIDGETS_EXPORT vtkTextRepresentation : public vtkBorderRepresentation
{
public:
  // Description:
  // Instantiate class.
  static vtkTextRepresentation *New();

  // Description:
  // Standard VTK methods.
  vtkTypeRevisionMacro(vtkTextRepresentation,vtkBorderRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the vtkTextActor to manage. If not specified, then one
  // is automatically created.
  void SetTextActor(vtkTextActor *textActor);
  vtkGetObjectMacro(TextActor,vtkTextActor);

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
  virtual int RenderTranslucentGeometry(vtkViewport*);

protected:
  vtkTextRepresentation();
  ~vtkTextRepresentation();

  // the text to manage
  vtkTextActor  *TextActor;
  
private:
  vtkTextRepresentation(const vtkTextRepresentation&);  //Not implemented
  void operator=(const vtkTextRepresentation&);  //Not implemented
};

#endif
