/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLabelRenderStrategy.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkLabelRenderStrategy - Superclass for label rendering implementations.
//
// .SECTION Description
// These methods should only be called within a mapper.

#ifndef __vtkLabelRenderStrategy_h
#define __vtkLabelRenderStrategy_h

#include "vtkObject.h"

#include "vtkStdString.h" // For string support
#include "vtkUnicodeString.h" // For unicode string support

class vtkRenderer;
class vtkTextProperty;

class VTK_RENDERING_EXPORT vtkLabelRenderStrategy : public vtkObject
{
 public:
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkTypeRevisionMacro(vtkLabelRenderStrategy, vtkObject);

  // Description:
  // Whether the text rendering strategy supports rotation.
  // The superclass returns true. Subclasses should override this to
  // return the appropriate value.
  virtual bool SupportsRotation()
    { return true; }

  // Description:
  // Set the renderer associated with this strategy.
  virtual void SetRenderer(vtkRenderer* ren);
  vtkGetObjectMacro(Renderer, vtkRenderer);

  // Description:
  // Set the default text property for the strategy.
  virtual void SetDefaultTextProperty(vtkTextProperty* tprop);
  vtkGetObjectMacro(DefaultTextProperty, vtkTextProperty);

  //BTX
  // Description:
  // Compute the bounds of a label. Must be performed after the renderer is set.
  // Only the unicode string version must be implemented in subclasses.
  virtual void ComputeLabelBounds(vtkTextProperty* tprop, vtkStdString label, double bds[4])
    { this->ComputeLabelBounds(tprop, vtkUnicodeString::from_utf8(label.c_str()), bds); }
  virtual void ComputeLabelBounds(vtkTextProperty* tprop, vtkUnicodeString label, double bds[4]) = 0;

  // Description:
  // Render a label at a location in world coordinates.
  // Must be performed between StartFrame() and EndFrame() calls.
  // Only the unicode string version must be implemented in subclasses.
  virtual void RenderLabel(double x[3], vtkTextProperty* tprop, vtkStdString label)
    { this->RenderLabel(x, tprop, vtkUnicodeString::from_utf8(label)); }
  virtual void RenderLabel(double x[3], vtkTextProperty* tprop, vtkUnicodeString label) = 0;
  //ETX

  // Description:
  // Start a rendering frame. Renderer must be set.
  virtual void StartFrame() { }

  // Description:
  // End a rendering frame.
  virtual void EndFrame() { }

protected:
  vtkLabelRenderStrategy();
  ~vtkLabelRenderStrategy();

  vtkRenderer* Renderer;
  vtkTextProperty* DefaultTextProperty;

private:
  vtkLabelRenderStrategy(const vtkLabelRenderStrategy&);  // Not implemented.
  void operator=(const vtkLabelRenderStrategy&);  // Not implemented.
};

#endif

