/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextActor.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTextActor - An actor that displays text. Scaled or unscaled
// .SECTION Description
// vtkTextActor can be used to place text annotation into a window.
// When ScaledText is false, the text is fixed font and operation is
// the same as a vtkTextMapper/vtkActor2D pair.
// When ScaledText is true, the font resizes such that the text fits inside the
// box defined by the position 1 & 2 coordinates. This class replaces the
// deprecated vtkScaledTextActor and acts as a convenient wrapper for
// a vtkTextMapper/vtkActor2D pair.
// Set the text property/attributes through the vtkTextProperty associated to
// this actor.
//
// .SECTION See Also
// vtkActor2D vtkTextMapper vtkTextProperty

#ifndef __vtkTextActor_h
#define __vtkTextActor_h

#include "vtkActor2D.h"

// We need to include vtkTextMapper here otherwise we have an ambiguous
// case of vtkMapper2D or vtkTextMapper in SetMapper(vtkTextMapper *mapper);
// - two members with identical prototypes!
#include "vtkTextMapper.h"

class vtkTextProperty;

class VTK_RENDERING_EXPORT vtkTextActor : public vtkActor2D
{
public:
  vtkTypeRevisionMacro(vtkTextActor,vtkActor2D);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Instantiate object with a rectangle in normaled view coordinates
  // of (0.2,0.85, 0.8, 0.95).
  static vtkTextActor *New();

  // Description:
  // Shallow copy of this text actor. Overloads the virtual
  // vtkProp method.
  void ShallowCopy(vtkProp *prop);

  // Description:
  // Override the vtkTextMapper that defines the text to be drawn.
  // One will be created by default if none is supplied
  void SetMapper(vtkTextMapper *mapper);

  // Description:
  // Set the text string to be displayed. "\n" is recognized
  // as a carriage return/linefeed (line separator).
  // Convenience method to the underlying mapper
  void SetInput(const char *inputString);
  char *GetInput();

  // Description:
  // Set/Get the minimum size in pixels for this actor.
  // Defaults to 10,10.
  // Not valid when ScaledText = false
  vtkSetVector2Macro(MinimumSize,int);
  vtkGetVector2Macro(MinimumSize,int);

  // Description:
  // Set/Get the maximum height of a line of text as a
  // percentage of the vertical area allocated to this
  // scaled text actor. Defaults to 1.0.
  // Not valid when ScaledText = false
  vtkSetMacro(MaximumLineHeight,float);
  vtkGetMacro(MaximumLineHeight,float);

  // Description:
  // Turn on or off the ScaledText option.
  // When text is scaled, the bounding rectangle is used to fit the text
  // When ScaledText is off, the text is rendered at a fixed font size
  vtkSetMacro(ScaledText,int);
  vtkGetMacro(ScaledText,int);
  vtkBooleanMacro(ScaledText,int);

  // Description:
  // Set/Get the Alignment point for unscaled (fixed fontsize) text
  // if zero (default), the text aligns itself to the bottom left corner
  // (which is defined by the PositionCoordinate)
  // otherwise the text aligns itself to corner/midpoint or centre
  //      6   7   8    of the box defined by the position 1 & 2 coordinates
  //      3   4   5    according to the diagram on the left.
  //      0   1   2
  vtkSetClampMacro(AlignmentPoint,int,0,8)
  vtkGetMacro(AlignmentPoint,int);

  // Description:
  // Return the actual vtkCoordinate reference that the mapper should use
  // to position the actor. This is used internally by the mappers and should
  // be overridden in specialized subclasses and otherwise ignored.
  vtkCoordinate *GetActualPositionCoordinate(void)
    { return this->AdjustedPositionCoordinate; }

  // Description:
  // Set/Get the text property.
  virtual void SetTextProperty(vtkTextProperty *p);
  vtkGetObjectMacro(TextProperty,vtkTextProperty);
  
//BTX
  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS.
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS.
  // Draw the text actor to the screen.
  int RenderOpaqueGeometry(vtkViewport* viewport);
  int RenderTranslucentGeometry(vtkViewport* ) {return 0;};
  int RenderOverlay(vtkViewport* viewport);
//ETX

protected:
  // Description:
  // Hide access methods which use superclass vtkMapper2D and not vtkTextMapper
  void SetMapper(vtkMapper2D *mapper);

   vtkTextActor();
  ~vtkTextActor();

  int   MinimumSize[2];
  float MaximumLineHeight;
  int   ScaledText;
  int   AlignmentPoint;

  vtkCoordinate *AdjustedPositionCoordinate;
  vtkTextProperty *TextProperty;

  vtkTimeStamp  BuildTime;
  int LastSize[2];
  int LastOrigin[2];

private:
  vtkTextActor(const vtkTextActor&);  // Not implemented.
  void operator=(const vtkTextActor&);  // Not implemented.
};


#endif

