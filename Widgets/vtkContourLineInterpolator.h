/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContourLineInterpolator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkContourLineInterpolator - Defines API for interpolating/modifying nodes from a vtkContourRepresentation
// .SECTION Description 
// vtkContourLineInterpolator is an abstract base class for interpolators
// that work are used by the contour representation class to interpolate 
// and/or modify nodes in a contour. Subclasses must override the virtual
// method: \c InterpolateLine. This is used by the contour representation 
// to give the interpolator a chance to define an interpolation scheme
// between nodes. See vtkBezierContourLineInterpolator for a concrete 
// implementation. Subclasses may also override, \c UpdateNode. This provides
// a way for the representation to give the interpolator a chance to modify
// the nodes, as the user constructs the contours. For instance a sticky
// contour widget may be implemented that moves nodes to nearby regions of
// high gradient, to be used in contour guided segmentation.
//
// .SECTION See Also

#ifndef __vtkContourLineInterpolator_h
#define __vtkContourLineInterpolator_h

#include "vtkObject.h"

class vtkRenderer;
class vtkContourRepresentation;

class VTK_WIDGETS_EXPORT vtkContourLineInterpolator : public vtkObject
{
public:
  // Description:
  // Standard methods for instances of this class.
  vtkTypeRevisionMacro(vtkContourLineInterpolator,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Subclasses that wish to interpolate a line segment must implement this.
  // For instance vtkBezierContourLineInterpolator adds nodes between idx1
  // and idx2, that allow the contour to adhere to a bezier curve.
  virtual int InterpolateLine( vtkRenderer *ren, 
                               vtkContourRepresentation *rep,
                               int idx1, int idx2 ) = 0;
  
  // Description:
  // The interpolator is given a chance to update the node. For instance, the
  // vtkImageContourLineInterpolator updates the idx'th node in the contour, 
  // so it automatically sticks to edges in the vicinity as the user 
  // constructs the contour. 
  // Returns 0 if the node (world position) is unchanged.
  virtual int UpdateNode( vtkRenderer *ren, 
                          vtkContourRepresentation *rep,
                          double *node, int idx );
  
 protected:
  vtkContourLineInterpolator();
  ~vtkContourLineInterpolator();

private:
  vtkContourLineInterpolator(const vtkContourLineInterpolator&);  //Not implemented
  void operator=(const vtkContourLineInterpolator&);  //Not implemented
};

#endif
