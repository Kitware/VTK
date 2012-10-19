/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMathTextActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMathTextActor - An actor that displays math text
//
// .SECTION Description
// vtkMathTextActor can be used to place mathtext annotation into a window.
// Set the text property/attributes through the vtkTextProperty associated to
// this actor.
//
// .SECTION See Also
// vtkTextActor vtkMathTextUtilities vtkContext2D

#ifndef __vtkMathTextActor_h
#define __vtkMathTextActor_h

#include "vtkRenderingFreeTypeModule.h" // For export macro
#include "vtkTextActor.h"

class vtkTextProperty;
class vtkImageData;

class VTKRENDERINGFREETYPE_EXPORT vtkMathTextActor : public vtkTextActor
{
public:
  vtkTypeMacro(vtkMathTextActor,vtkTextActor);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkMathTextActor *New();

  // Description:
  // Returns true if MathText is available on this build of VTK. If false,
  // this actor will not function.
  bool IsSupported();

  // Description:
  // If there is no MathText implementation available (e.g. IsSupported()
  // return false), the fallback text will be rendered using the FreeType
  // text rendering backend.
  vtkGetStringMacro(FallbackText)
  vtkSetStringMacro(FallbackText)

  // Description:
  // Shallow copy of this actor.
  void ShallowCopy(vtkProp *prop);

protected:
  vtkMathTextActor();
  ~vtkMathTextActor();

  bool RenderImage(vtkTextProperty *tprop, vtkViewport *viewport);
  bool GetImageBoundingBox(
    vtkTextProperty *tprop, vtkViewport *viewport, int bbox[4]);

  // Description:
  // Used when a MathText implementation is unavailable.
  char *FallbackText;

private:
  vtkMathTextActor(const vtkMathTextActor&);  // Not implemented.
  void operator=(const vtkMathTextActor&);  // Not implemented.
};

#endif
