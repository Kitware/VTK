/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractMapper.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAbstractMapper - abstract class specifies interface to map data
// .SECTION Description
// vtkAbstractMapper is an abstract class to specify interface between data and 
// graphics primitives or software rendering techniques. Subclasses of 
// vtkAbstractMapper can be used for rendering 2D data, geometry, or volumetric
// data.
//
// .SECTION See Also
// vtkAbstractMapper3D vtkMapper vtkPolyDataMapper vtkVolumeMapper

#ifndef __vtkAbstractMapper_h
#define __vtkAbstractMapper_h

#include "vtkProcessObject.h"
#include "vtkPlaneCollection.h"
#include "vtkPlane.h"
#include "vtkPlanes.h"
#include "vtkTimerLog.h"

#define VTK_SCALAR_MODE_DEFAULT 0
#define VTK_SCALAR_MODE_USE_POINT_DATA 1
#define VTK_SCALAR_MODE_USE_CELL_DATA 2
#define VTK_SCALAR_MODE_USE_POINT_FIELD_DATA 3
#define VTK_SCALAR_MODE_USE_CELL_FIELD_DATA 4

#define VTK_GET_ARRAY_BY_ID 0
#define VTK_GET_ARRAY_BY_NAME 1

class vtkWindow;
class vtkDataSet;

class VTK_COMMON_EXPORT vtkAbstractMapper : public vtkProcessObject
{
public:
  vtkTypeRevisionMacro(vtkAbstractMapper,vtkProcessObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Override Modifiedtime as we have added Clipping planes
  virtual unsigned long GetMTime();

  // Description:
  // Release any graphics resources that are being consumed by this mapper.
  // The parameter window could be used to determine which graphic
  // resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow *) {};

  // Description:
  // Get the time required to draw the geometry last time it was rendered
  vtkGetMacro( TimeToDraw, float );

  // Description:
  // Specify clipping planes to be applied when the data is mapped
  // (at most 6 clipping planes can be specified).
  void AddClippingPlane(vtkPlane *plane);
  void RemoveClippingPlane(vtkPlane *plane);
  void RemoveAllClippingPlanes();

  // Description:
  // Get/Set the vtkPlaneCollection which specifies the
  // clipping planes.
  vtkSetObjectMacro(ClippingPlanes,vtkPlaneCollection);
  vtkGetObjectMacro(ClippingPlanes,vtkPlaneCollection);

  // Description:
  // An alternative way to set clipping planes: use up to six planes found
  // in the supplied instance of the implicit function vtkPlanes.
  void SetClippingPlanes(vtkPlanes *planes);

  // Description:
  // Make a shallow copy of this mapper.
  void ShallowCopy(vtkAbstractMapper *m);

  // Description:
  // Internal helper function for getting the active scalars. The scalar
  // mode indicates where the scalars come from; the arrayAccessMode
  // is used to indicate how to retrieve the scalars from field data (if
  // the scalarMode indicates that). The component is used to indicate which
  // component in the data array to use as the scalars.
  static vtkDataArray *GetScalars(vtkDataSet *input, int scalarMode,
                                  int arrayAccessMode, int arrayId, 
                                  const char *arrayName, int& component);
protected:
  vtkAbstractMapper();
  ~vtkAbstractMapper();

  vtkTimerLog *Timer;
  float TimeToDraw;
  vtkWindow *LastWindow;   // Window used for the previous render
  vtkPlaneCollection *ClippingPlanes;

private:
  vtkAbstractMapper(const vtkAbstractMapper&);  // Not implemented.
  void operator=(const vtkAbstractMapper&);  // Not implemented.
};

#endif


