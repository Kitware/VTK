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
/**
 * @class   vtkPlaybackRepresentation
 * @brief   represent the vtkPlaybackWidget
 *
 * This class is used to represent the vtkPlaybackWidget. Besides defining
 * geometry, this class defines a series of virtual method stubs that are
 * meant to be subclassed by applications for controlling playback.
 *
 * @sa
 * vtkPlaybackWidget
*/

#ifndef vtkPlaybackRepresentation_h
#define vtkPlaybackRepresentation_h

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
  /**
   * Instantiate this class.
   */
  static vtkPlaybackRepresentation *New();

  //@{
  /**
   * Standard VTK class methods.
   */
  vtkTypeMacro(vtkPlaybackRepresentation,vtkBorderRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  //@{
  /**
   * By obtaining this property you can specify the properties of the
   * representation.
   */
  vtkGetObjectMacro(Property,vtkProperty2D);
  //@}

  /**
   * Virtual callbacks that subclasses should implement.
   */
  virtual void Play() {}
  virtual void Stop() {}
  virtual void ForwardOneFrame() {}
  virtual void BackwardOneFrame() {}
  virtual void JumpToBeginning() {}
  virtual void JumpToEnd() {}

  /**
   * Satisfy the superclasses' API.
   */
  void BuildRepresentation() override;
  void GetSize(double size[2]) override
    {size[0]=12.0; size[1]=2.0;}

  //@{
  /**
   * These methods are necessary to make this representation behave as
   * a vtkProp.
   */
  void GetActors2D(vtkPropCollection*) override;
  void ReleaseGraphicsResources(vtkWindow*) override;
  int RenderOverlay(vtkViewport*) override;
  int RenderOpaqueGeometry(vtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(vtkViewport*) override;
  vtkTypeBool HasTranslucentPolygonalGeometry() override;
  //@}

protected:
  vtkPlaybackRepresentation();
  ~vtkPlaybackRepresentation() override;

  // representation geometry
  vtkPoints                  *Points;
  vtkPolyData                *PolyData;
  vtkTransformPolyDataFilter *TransformFilter;
  vtkPolyDataMapper2D        *Mapper;
  vtkProperty2D              *Property;
  vtkActor2D                 *Actor;

private:
  vtkPlaybackRepresentation(const vtkPlaybackRepresentation&) = delete;
  void operator=(const vtkPlaybackRepresentation&) = delete;
};

#endif
