/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScaledTextActor.h
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
// .NAME vtkScaledTextActor - create text that will scale as needed
// .SECTION Description
// vtkScaledTextActor can be used to place text annotation into a window
// and have the font size scale so that the text always bounded by 
// a specified rectangle.
//
// .SECTION See Also
// vtkActor2D vtkTextMapper

#ifndef __vtkScaledTextActor_h
#define __vtkScaledTextActor_h

#include "vtkActor2D.h"
#include "vtkTextMapper.h"

class VTK_RENDERING_EXPORT vtkScaledTextActor : public vtkActor2D
{
public:
  vtkTypeRevisionMacro(vtkScaledTextActor,vtkActor2D);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Instantiate object with a rectangle in normaled view coordinates
  // of (0.2,0.85, 0.8, 0.95).
  static vtkScaledTextActor *New();
  
  // Description:
  // Set/Get the vtkTextMapper that defines the text to be drawn.
  void SetMapper(vtkTextMapper *mapper);

  // Override superclass' SetMapper method and check type. This makes it possible
  // to use the virtual SetMapper call in Actor2D
  void SetMapper(vtkMapper2D *mapper);

  // Description:
  // Get the vtkTextMapper that defines the text to be drawn.
  vtkMapper2D *GetMapper(void);

  // Description:
  // Set/Get the minimum size in pixels for this actor.
  // Defaults to 10,10.
  vtkSetVector2Macro(MinimumSize,int);
  vtkGetVector2Macro(MinimumSize,int);
  
  // Description:
  // Set/Get the maximum height of a line of text as a 
  // percentage of the vertical area allocated to this
  // scaled text actor. Defaults to 1.0.
  vtkSetMacro(MaximumLineHeight,float);
  vtkGetMacro(MaximumLineHeight,float);
  
  // Description:
  // Shallow copy of this scaled text actor. Overloads the virtual 
  // vtkProp method.
  void ShallowCopy(vtkProp *prop);

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
  // Draw the scaled text actor to the screen.
  int RenderOpaqueGeometry(vtkViewport* viewport);
  int RenderTranslucentGeometry(vtkViewport* ) {return 0;};
  int RenderOverlay(vtkViewport* viewport);
//ETX

protected:
  vtkScaledTextActor();
  ~vtkScaledTextActor();

  int MinimumSize[2];
  float MaximumLineHeight;

  vtkActor2D *TextActor;
  vtkTimeStamp  BuildTime;
  int LastSize[2];
  int LastOrigin[2];

private:
  vtkScaledTextActor(const vtkScaledTextActor&);  // Not implemented.
  void operator=(const vtkScaledTextActor&);  // Not implemented.
};


#endif

