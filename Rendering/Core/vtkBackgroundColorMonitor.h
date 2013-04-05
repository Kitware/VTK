/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBackgroundColorMonitor

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBackgroundColorMonitor -- A helper for painters that
// tracks state of background color(s).
//
// .SECTION Description:
// vtkBackgroundColorMonitor -- A helper for painters that
// tracks state of background color(s). A Painter could use this
// to skip expensive processing that is only needed when
// background color changes. This class queries VTK renderer
// rather than OpenGL state in order to support VTK's gradient
// background.
//
// this is not intended to be shared. each object should use it's
// own instance of this class. it's intended to be called once per
// render.

#ifndef __vtkBackgroundColorMonitor_H
#define __vtkBackgroundColorMonitor_H

#include "vtkRenderingCoreModule.h" // for export macro
#include "vtkObject.h"

class vtkRenderer;

class VTKRENDERINGCORE_EXPORT vtkBackgroundColorMonitor : public vtkObject
{
public:
  static vtkBackgroundColorMonitor* New();
  vtkTypeMacro(vtkBackgroundColorMonitor, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Fetches the current background color state and
  // updates the internal copies of the data. returns
  // true if any of the tracked colors or modes have
  // changed. Typically this is the only function a
  // user needs to call.
  bool StateChanged(vtkRenderer *ren);

  // Description:
  // Update the internal state if anything changed. Note,
  // this is done automatically in SateChanged.
  void Update(vtkRenderer *ren);

protected:
  vtkBackgroundColorMonitor();
  ~vtkBackgroundColorMonitor(){}

private:
  unsigned int UpTime;
  bool Gradient;
  double Color1[3];
  double Color2[3];

private:
 vtkBackgroundColorMonitor(const vtkBackgroundColorMonitor&); // Not implemented
 void operator=(const vtkBackgroundColorMonitor&); // Not implemented
};

#endif
