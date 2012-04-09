/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFocalPlaneContourRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkFocalPlaneContourRepresentation - represent a contour drawn on the
// focal plane. 
//
// .SECTION Description
// The contour will stay on the focal plane irrespective of camera 
// position/orientation changes. The class was written in order to be able to 
// draw contours on a volume widget and have the contours overlayed on the 
// focal plane in order to do contour segmentation. The superclass, 
// vtkContourRepresentation handles contours that are drawn in actual world 
// position co-ordinates, so they would rotate with the camera position/
// orientation changes
//
// .SECTION See Also
// vtkContourWidget vtkHandleRepresentation vtkContourRepresentation

#ifndef __vtkFocalPlaneContourRepresentation_h
#define __vtkFocalPlaneContourRepresentation_h

#include "vtkContourRepresentation.h"

class vtkHandleRepresentation;

class VTK_WIDGETS_EXPORT vtkFocalPlaneContourRepresentation : public vtkContourRepresentation
{
public:
  // Description:
  // Standard VTK methods.
  vtkTypeMacro(vtkFocalPlaneContourRepresentation,vtkContourRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the world position of the intermediate point at
  // index idx between nodes n and (n+1) (or n and 0 if
  // n is the last node and the loop is closed). Returns
  // 1 on success or 0 if n or idx are out of range.
  virtual int GetIntermediatePointWorldPosition( int n, 
                                                 int idx, double point[3] );

  // Description:
  // Get the world position of the intermediate point at
  // index idx between nodes n and (n+1) (or n and 0 if
  // n is the last node and the loop is closed). Returns
  // 1 on success or 0 if n or idx are out of range.
  virtual int GetIntermediatePointDisplayPosition( int n, 
                                                 int idx, double point[3] );

  // Description:
  // Get the nth node's display position. Will return
  // 1 on success, or 0 if there are not at least 
  // (n+1) nodes (0 based counting).
  virtual int GetNthNodeDisplayPosition( int n, double pos[2] );
 
  // Description:
  // Get the nth node's world position. Will return
  // 1 on success, or 0 if there are not at least 
  // (n+1) nodes (0 based counting).
  virtual int GetNthNodeWorldPosition( int n, double pos[3] );

  // Description:
  // The class maintains its true contour locations based on display co-ords
  // This method syncs the world co-ords data structure with the display co-ords.
  virtual void UpdateContourWorldPositionsBasedOnDisplayPositions();
  
  // Description:
  // The method must be called whenever the contour needs to be updated, usually
  // from RenderOpaqueGeometry()
  virtual int UpdateContour();

  virtual void UpdateLines( int index );

protected:
  vtkFocalPlaneContourRepresentation();
  ~vtkFocalPlaneContourRepresentation();

private:
  vtkFocalPlaneContourRepresentation(const vtkFocalPlaneContourRepresentation&);  //Not implemented
  void operator=(const vtkFocalPlaneContourRepresentation&);  //Not implemented
};

#endif

