/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlaybackRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPlaybackRepresentation - represent the vtkPlaybackWidget
// .SECTION Description
// This class is used to represent the vtkPlaybackWidget. Besides defining
// geometry, this class defines a series of virtual method stubs that are
// meant to be subclassed by applications for controlling playback.

// .SECTION See Also
// vtkPlaybackWidget


#ifndef __vtkPlaybackRepresentation_h
#define __vtkPlaybackRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkBorderRepresentation.h"

class vtkRenderer;
class vtkRenderWindowInteractor;
class vtkPoints;
class vtkPolyData;
class vtkTransformPolyDataFilter;
class vtkPolyDataMapper2D;
class vtkProperty2D;
class vtkActor2D;

class VTKINTERACTIONWIDGETS_EXPORT vtkPlaybackRepresentation : public vtkBorderRepresentation
{
public:
  // Description:
  // Instantiate this class.
  static vtkPlaybackRepresentation *New();

  // Description:
  // Standard VTK class methods.
  vtkTypeMacro(vtkPlaybackRepresentation,vtkBorderRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // By obtaining this property you can specify the properties of the
  // representation.
  vtkGetObjectMacro(Property,vtkProperty2D);

  // Description:
  // Virtual callbacks that subclasses should implement.
  virtual void Play() {}
  virtual void Stop() {}
  virtual void ForwardOneFrame() {}
  virtual void BackwardOneFrame() {}
  virtual void JumpToBeginning() {}
  virtual void JumpToEnd() {}

  // Description:
  // Satisfy the superclasses' API.
  virtual void BuildRepresentation();
  virtual void GetSize(double size[2])
    {size[0]=12.0; size[1]=2.0;}

  // Description:
  // These methods are necessary to make this representation behave as
  // a vtkProp.
  virtual void GetActors2D(vtkPropCollection*);
  virtual void ReleaseGraphicsResources(vtkWindow*);
  virtual int RenderOverlay(vtkViewport*);
  virtual int RenderOpaqueGeometry(vtkViewport*);
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport*);
  virtual int HasTranslucentPolygonalGeometry();

protected:
  vtkPlaybackRepresentation();
  ~vtkPlaybackRepresentation();

  // representation geometry
  vtkPoints                  *Points;
  vtkPolyData                *PolyData;
  vtkTransformPolyDataFilter *TransformFilter;
  vtkPolyDataMapper2D        *Mapper;
  vtkProperty2D              *Property;
  vtkActor2D                 *Actor;

private:
  vtkPlaybackRepresentation(const vtkPlaybackRepresentation&);  //Not implemented
  void operator=(const vtkPlaybackRepresentation&);  //Not implemented
};

#endif
