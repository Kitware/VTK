/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderedRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .NAME vtkRenderedRepresentation - 
//
// .SECTION Description

#ifndef __vtkRenderedRepresentation_h
#define __vtkRenderedRepresentation_h

#include "vtkDataRepresentation.h"
#include "vtkSmartPointer.h" // for SP ivars
#include "vtkUnicodeString.h" // for string

class vtkApplyColors;
class vtkProp;
class vtkRenderView;
class vtkRenderWindow;
class vtkTextProperty;
class vtkTexture;
class vtkView;

class VTK_VIEWS_EXPORT vtkRenderedRepresentation : public vtkDataRepresentation
{
public:
  static vtkRenderedRepresentation* New();
  vtkTypeMacro(vtkRenderedRepresentation, vtkDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the label render mode.
  // vtkRenderView::QT - Use Qt-based labeler with fitted labeling
  //   and unicode support. Requires VTK_USE_QT to be on.
  // vtkRenderView::FREETYPE - Use standard freetype text rendering.
  vtkSetMacro(LabelRenderMode, int);
  vtkGetMacro(LabelRenderMode, int);

protected:
  vtkRenderedRepresentation();
  ~vtkRenderedRepresentation();

  // Description:
  // Subclasses may call these methods to add or remove props from the representation.
  // Use these if the number of props/actors changes as the result of input connection
  // changes.
  void AddPropOnNextRender(vtkProp* p);
  void RemovePropOnNextRender(vtkProp* p);

  //BTX
  // Description:
  // Obtains the hover text for a particular prop and cell.
  // If the prop is not applicable to the representation, return an empty string.
  // Subclasses should override GetHoverTextInternal, in which the prop and cell
  // are converted to an appropriate selection using ConvertSelection().
  vtkUnicodeString GetHoverText(vtkView* view, vtkProp* prop, vtkIdType cell);

  // Description:
  // Subclasses may override this method to generate the hover text.
  virtual vtkUnicodeString GetHoverTextInternal(vtkSelection*)
    { return vtkUnicodeString(); }
  //ETX

  // Description:
  // The view will call this method before every render.
  // Representations may add their own pre-render logic here.
  virtual void PrepareForRendering(vtkRenderView* view);

  //BTX
  friend class vtkRenderView;
  class Internals;
  Internals* Implementation;
  //ETX

  int LabelRenderMode;

private:
  vtkRenderedRepresentation(const vtkRenderedRepresentation&); // Not implemented
  void operator=(const vtkRenderedRepresentation&);   // Not implemented
};

#endif

