/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataContourLineInterpolator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPolyDataContourLineInterpolator - Contour interpolator for polygonal data
//
// .SECTION Description
// vtkPolyDataContourLineInterpolator is an abstract base class for contour
// line interpolators that interpolate on polygonal data.
//
// .SECTION See Also

#ifndef vtkPolyDataContourLineInterpolator_h
#define vtkPolyDataContourLineInterpolator_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkContourLineInterpolator.h"

class vtkPolyData;
class vtkPolyDataCollection;

class VTKINTERACTIONWIDGETS_EXPORT vtkPolyDataContourLineInterpolator
                        : public vtkContourLineInterpolator
{
public:
  // Description:
  // Standard methods for instances of this class.
  vtkTypeMacro(vtkPolyDataContourLineInterpolator,
                              vtkContourLineInterpolator);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Subclasses that wish to interpolate a line segment must implement this.
  // For instance vtkBezierContourLineInterpolator adds nodes between idx1
  // and idx2, that allow the contour to adhere to a bezier curve.
  virtual int InterpolateLine( vtkRenderer *ren,
                               vtkContourRepresentation *rep,
                               int idx1, int idx2 ) = 0;

  // Description:
  // The interpolator is given a chance to update the node.
  // vtkImageContourLineInterpolator updates the idx'th node in the contour,
  // so it automatically sticks to edges in the vicinity as the user
  // constructs the contour.
  // Returns 0 if the node (world position) is unchanged.
  virtual int UpdateNode( vtkRenderer *,
                          vtkContourRepresentation *,
                          double * vtkNotUsed(node), int vtkNotUsed(idx) ) = 0;

  // Description:
  // Be sure to add polydata on which you wish to place points to this list
  // or they will not be considered for placement.
  vtkGetObjectMacro( Polys, vtkPolyDataCollection );

protected:
  vtkPolyDataContourLineInterpolator();
  ~vtkPolyDataContourLineInterpolator();

  vtkPolyDataCollection *Polys;

private:
  vtkPolyDataContourLineInterpolator(
      const vtkPolyDataContourLineInterpolator&);  //Not implemented
  void operator=(const vtkPolyDataContourLineInterpolator&);  //Not implemented
};

#endif
