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

#include "vtkMathTextModule.h" // For export macro
#include "vtkNew.h" // For smart pointer
#include "vtkSmartPointer.h" // For smart pointer
#include "vtkProp3D.h"

class vtkImageActor;
class vtkImageData;
class vtkTextProperty;

class VTKMATHTEXT_EXPORT vtkMathTextActor3D : public vtkProp3D
{
public:
  static vtkMathTextActor3D *New();
  vtkTypeMacro(vtkMathTextActor3D,vtkProp3D)
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns true if MathText is available on this build of VTK. If false,
  // this actor will not function.
  bool IsSupported();

  // Description:
  // Set the text string to be displayed.
  vtkSetStringMacro(Input)
  vtkGetStringMacro(Input)

  // Description:
  // Set/Get the text property.
  virtual void SetTextProperty(vtkTextProperty *p);
  vtkTextProperty *GetTextProperty();

  // Description:
  // Shallow copy of this text actor. Overloads the virtual
  // vtkProp method.
  void ShallowCopy(vtkProp *prop);

  // Description:
  // Get the bounds for this Prop3D as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
  virtual double *GetBounds();

  // Description:
  // Get the dimensions of the underlying image:
  virtual int *GetImageDimensions();
  virtual void GetImageDimensions(int dims[3]);

  // Description:
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // Draw the text actor to the screen.
  int RenderOpaqueGeometry(vtkViewport* viewport);
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport* viewport);
  int RenderOverlay(vtkViewport* viewport);

  // Description:
  // Does this prop have some translucent polygonal geometry?
  virtual int HasTranslucentPolygonalGeometry();

protected:
   vtkMathTextActor3D();
  ~vtkMathTextActor3D();

  char *Input;

  vtkNew<vtkImageActor> ImageActor;
  vtkNew<vtkImageData> ImageData;
  vtkSmartPointer<vtkTextProperty> TextProperty;

  virtual int UpdateImageActor();

private:
  vtkMathTextActor3D(const vtkMathTextActor3D&);  // Not implemented.
  void operator=(const vtkMathTextActor3D&);  // Not implemented.
};

#endif
