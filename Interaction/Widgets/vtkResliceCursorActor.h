/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkResliceCursorActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkResliceCursorActor
 * @brief   Represent a reslice cursor
 *
 * A reslice cursor consists of a pair of lines (cross hairs), thin or thick,
 * that may be interactively manipulated for thin/thick reformats through the
 * data.
 * @sa
 * vtkResliceCursor vtkResliceCursorPolyDataAlgorithm vtkResliceCursorWidget
 * vtkResliceCursorRepresentation vtkResliceCursorLineRepresentation
*/

#ifndef vtkResliceCursorActor_h
#define vtkResliceCursorActor_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkProp3D.h"

class vtkResliceCursor;
class vtkResliceCursorPolyDataAlgorithm;
class vtkPolyDataMapper;
class vtkActor;
class vtkProperty;
class vtkBoundingBox;

class VTKINTERACTIONWIDGETS_EXPORT vtkResliceCursorActor : public vtkProp3D
{

public:

  //@{
  /**
   * Standard VTK methods
   */
  static vtkResliceCursorActor *New();
  vtkTypeMacro(vtkResliceCursorActor,vtkProp3D);
  void PrintSelf(ostream& os, vtkIndent indent);
  //@}

  //@{
  /**
   * Get the cursor algorithm. The cursor must be set on the algorithm
   */
  vtkGetObjectMacro( CursorAlgorithm, vtkResliceCursorPolyDataAlgorithm );
  //@}

  /**
   * Support the standard render methods.
   */
  virtual int RenderOpaqueGeometry(vtkViewport *viewport);

  /**
   * Does this prop have some translucent polygonal geometry? No.
   */
  virtual int HasTranslucentPolygonalGeometry();

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow *);

  /**
   * Get the bounds for this Actor as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
   */
  double *GetBounds();

  /**
   * Get the actors mtime plus consider its algorithm.
   */
  vtkMTimeType GetMTime();

  //@{
  /**
   * Get property of the internal actor.
   */
  vtkProperty *GetCenterlineProperty( int i );
  vtkProperty *GetThickSlabProperty( int i );
  //@}

  /**
   * Get the centerline actor along a particular axis
   */
  vtkActor * GetCenterlineActor(int axis);

  /**
   * Set the user matrix on all the internal actors.
   */
  virtual void SetUserMatrix( vtkMatrix4x4 *matrix);

protected:
  vtkResliceCursorActor();
  ~vtkResliceCursorActor();

  void UpdateViewProps( vtkViewport * v = NULL );
  void UpdateHoleSize( vtkViewport * v );

  vtkResliceCursorPolyDataAlgorithm * CursorAlgorithm;
  vtkPolyDataMapper                 * CursorCenterlineMapper[3];
  vtkActor                          * CursorCenterlineActor[3];
  vtkPolyDataMapper                 * CursorThickSlabMapper[3];
  vtkActor                          * CursorThickSlabActor[3];
  vtkProperty                       * CenterlineProperty[3];
  vtkProperty                       * ThickSlabProperty[3];

private:
  vtkResliceCursorActor(const vtkResliceCursorActor&) VTK_DELETE_FUNCTION;
  void operator=(const vtkResliceCursorActor&) VTK_DELETE_FUNCTION;
};

#endif
