/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataSourceWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPolyDataSourceWidget
 * @brief   abstract PolyDataSource-based 3D widget
 *
 * This abstract class serves as parent to 3D widgets that have simple
 * vtkPolyDataSource instances defining their geometry.
 *
 * In addition to what is offered by the vtk3DWidget parent, this class
 * makes it possible to manipulate the underlying polydatasource and to
 * PlaceWidget() according to that, instead of having to make use of
 * SetInput() or SetProp3D().
 *
 * Implementors of child classes HAVE to implement their PlaceWidget(bounds)
 * to check for the existence of Input and Prop3D FIRST.  If these don't
 * exist, place according to the underlying PolyDataSource.  Child classes
 * also have to imprement UpdatePlacement(), which updates the widget according
 * to the geometry of the underlying PolyDataSource.
 *
 * @sa
 * vtk3DWidget vtkLineWidget vtkPlaneWidget vtkSphereWidget
*/

#ifndef vtkPolyDataSourceWidget_h
#define vtkPolyDataSourceWidget_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtk3DWidget.h"

class vtkPolyDataAlgorithm;

class VTKINTERACTIONWIDGETS_EXPORT vtkPolyDataSourceWidget : public vtk3DWidget
{
 public:
  vtkTypeMacro(vtkPolyDataSourceWidget, vtk3DWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Overrides vtk3DWidget PlaceWidget() so that it doesn't complain if
   * there's no Input and no Prop3D.
   */
  virtual void PlaceWidget();

  /**
   * We have to redeclare this abstract, PlaceWidget() requires it.  You HAVE
   * to override this in your concrete child classes.  If there's no Prop3D
   * and no Input, your PlaceWidget must make use of the underlying
   * PolyDataSource to do its work.
   */
  virtual void PlaceWidget(double bounds[6]) = 0;

  /**
   * Convenience method brought over from vtkPlaneWidget.
   */
  void PlaceWidget(double xmin, double xmax, double ymin, double ymax,
                   double zmin, double zmax)
    {this->Superclass::PlaceWidget(xmin,xmax,ymin,ymax,zmin,zmax);}

  /**
   * Returns underlying vtkPolyDataAlgorithm that determines geometry.  This
   * can be modified after which PlaceWidget() or UpdatePlacement() can be
   * called.  UpdatePlacement() will always update the planewidget according
   * to the geometry of the underlying PolyDataAlgorithm.  PlaceWidget() will
   * only make use of this geometry if there is no Input and no Prop3D set.
   */
  virtual vtkPolyDataAlgorithm* GetPolyDataAlgorithm() = 0;

  /**
   * If you've made changes to the underlying vtkPolyDataSource AFTER your
   * initial call to PlaceWidget(), use this method to realise the changes
   * in the widget.
   */
  virtual void UpdatePlacement() = 0;

protected:
  /**
   * Empty constructor that calls the parent constructor.  Child classes
   * should call this constructor as part of their initialisation.
   */
  vtkPolyDataSourceWidget();

private:
  // this copy constructor and assignment operator are deliberately not
  // implemented so that any "accidental" invocation of a copy (pass by value)
  // or assignment will trigger linker errors; the class is not meant to
  // be used in these ways.  I couldn't resist adding this explanation. :)
  vtkPolyDataSourceWidget(const vtkPolyDataSourceWidget&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPolyDataSourceWidget&) VTK_DELETE_FUNCTION;
};

#endif
