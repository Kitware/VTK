/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBalloonRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBalloonRepresentation - represent the vtkBalloonWidget
// .SECTION Description
// The vtkBalloonRepresentation is used to represent the vtkBalloonWidget.
// The class provides methods to control the appearance of the text within a 
// rectangular frame.

// .SECTION See Also
// vtkBalloonWidget


#ifndef __vtkBalloonRepresentation_h
#define __vtkBalloonRepresentation_h

#include "vtkWidgetRepresentation.h"

class vtkTextMapper;
class vtkTextActor;
class vtkTextProperty;
class vtkPoints;
class vtkCellArray;
class vtkPolyData;
class vtkPolyDataMapper2D;
class vtkActor2D;
class vtkProperty2D;


class VTK_WIDGETS_EXPORT vtkBalloonRepresentation : public vtkWidgetRepresentation
{
public:
  // Description:
  // Instantiate the class.
  static vtkBalloonRepresentation *New();

  // Description:
  // Standard VTK methods.
  vtkTypeRevisionMacro(vtkBalloonRepresentation,vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify/Retrieve the text to display in the balloon.
  vtkGetStringMacro(BalloonText);
  vtkSetStringMacro(BalloonText);

  // Description:
  // Set/Get the text property.
  virtual void SetTextProperty(vtkTextProperty *p);
  vtkGetObjectMacro(TextProperty,vtkTextProperty);
    
  // Description:
  // Set/Get the frame property.
  virtual void SetFrameProperty(vtkProperty2D *p);
  vtkGetObjectMacro(FrameProperty,vtkProperty2D);
    
  // Description:
  // Set/Get the offset from the mouse pointer from which to place the
  // balloon. The representation will try and honor this offset unless there
  // is a collision with the side of the renderer.
  vtkSetVector2Macro(Offset,int);
  vtkGetVector2Macro(Offset,int);

  // Description:
  // Set/Get the padding (in pixels) that whould be used around the text
  // (i.e., between the frame and the text).
  vtkSetClampMacro(Padding,int,0,100);
  vtkGetMacro(Padding,int);

  // Description:
  // These are methods that satisfy vtkWidgetRepresentation's API.
  virtual void StartWidgetInteraction(double e[2]);
  virtual void EndWidgetInteraction(double e[2]);
  virtual void BuildRepresentation();
  
  // Description:
  // Methods required by vtkProp superclass.
  virtual void ReleaseGraphicsResources(vtkWindow *w);
  virtual int RenderOverlay(vtkViewport *viewport);

protected:
  vtkBalloonRepresentation();
  ~vtkBalloonRepresentation();

  // The balloon text
  char *BalloonText;

  // Controlling placement
  int Padding;
  int Offset[2];

  // The text
  vtkTextMapper   *TextMapper;
  vtkActor2D      *TextActor;
  vtkTextProperty *TextProperty;
  
  // The frame
  vtkPoints           *FramePoints;
  vtkCellArray        *FramePolygon;
  vtkPolyData         *FramePolyData;
  vtkPolyDataMapper2D *FrameMapper;
  vtkActor2D          *FrameActor;
  vtkProperty2D       *FrameProperty;
  
private:
  vtkBalloonRepresentation(const vtkBalloonRepresentation&);  //Not implemented
  void operator=(const vtkBalloonRepresentation&);  //Not implemented
};

#endif
