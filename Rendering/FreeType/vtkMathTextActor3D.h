/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMathTextActor3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMathTextActor3D - An actor that displays mathtext in 3D.
// .SECTION Description
// The input mathtext is rendered into a buffer, which in turn is used as a
// texture applied onto a quad (a vtkImageActor is used under the hood).
// .SECTION Caveats
// This class is experimental at the moment.
// - The TextProperty orientation is not used; instead orient this actor.
// - The TextProperty alignment is not used; instead, position this actor.
// - No checking is done regarding hardware texture size limits.
//
// .SECTION See Also
// vtkProp3D vtkMathTextActor vtkTextActor vtkTextActor3D

#ifndef __vtkMathTextActor3D_h
#define __vtkMathTextActor3D_h

#include "vtkRenderingFreeTypeModule.h" // For export macro
#include "vtkTextActor3D.h"

class VTKRENDERINGFREETYPE_EXPORT vtkMathTextActor3D : public vtkTextActor3D
{
public:
  static vtkMathTextActor3D *New();
  vtkTypeMacro(vtkMathTextActor3D,vtkTextActor3D)
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns true if MathText is available on this build of VTK. If false,
  // this actor will not function.
  bool IsSupported();

  // Description:
  // If there is no MathText implementation available (e.g. IsSupported()
  // return false), the fallback text will be rendered using the FreeType
  // text rendering backend.
  vtkSetStringMacro(FallbackText)
  vtkGetStringMacro(FallbackText)

  // Description:
  // Get the bounding box for the given vtkTextProperty
  // and text string str.  Results are returned in the four element bbox int
  // array.  This call can be used for sizing other elements.
  int GetBoundingBox(int bbox[4]);

  // Shallow copy of this text actor. Overloads the virtual
  // vtkProp method.
  void ShallowCopy(vtkProp *prop);

protected:
   vtkMathTextActor3D();
  ~vtkMathTextActor3D();

  char *FallbackText;

  int UpdateImageActor();

private:
  vtkMathTextActor3D(const vtkMathTextActor3D&);  // Not implemented.
  void operator=(const vtkMathTextActor3D&);  // Not implemented.
};

#endif
