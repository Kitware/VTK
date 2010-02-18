/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClipConvexPolyData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/


#ifndef __vtkClipConvexPolyData_h
#define __vtkClipConvexPolyData_h

#include "vtkPolyDataAlgorithm.h"

class vtkPlaneCollection;
class vtkPlane;
class vtkClipConvexPolyDataInternals;

class VTK_GRAPHICS_EXPORT vtkClipConvexPolyData : public vtkPolyDataAlgorithm
{
public:
  static vtkClipConvexPolyData *New();
  vtkTypeRevisionMacro(vtkClipConvexPolyData,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set all the planes at once using a vtkPlanes implicit function.
  // This also sets the D value, so it can be used with GenerateClipConvexPolyData().
  void SetPlanes( vtkPlaneCollection *planes );
  vtkGetObjectMacro( Planes, vtkPlaneCollection );
  
  // Description:
  // Redefines this method, as this filter depends on time of its components
  // (planes)
  virtual unsigned long int GetMTime();
  
protected:
  vtkClipConvexPolyData();
  ~vtkClipConvexPolyData();

  vtkPlaneCollection *Planes;

  vtkClipConvexPolyDataInternals *Internal;

  void ClipWithPlane( vtkPlane *, double tolerance );
  
  int HasDegeneracies( vtkPlane * );

  // The method that does it all...
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  void ClearInternals();
  void ClearNewVertices();
  void RemoveEmptyPolygons();
  
private:
  vtkClipConvexPolyData(const vtkClipConvexPolyData&);  // Not implemented.
  void operator=(const vtkClipConvexPolyData&);  // Not implemented.
};

#endif
