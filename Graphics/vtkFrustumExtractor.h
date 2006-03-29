/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFrustumExtractor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkFrustumExtractor - Returns the portion of the input dataset that 
// lies within a selection frustum.
//
// .SECTION Description
// This class intersects the input DataSet with a frustum and determines which
// cells and points lie within the frustum. The frustum is defined with a 
// vtkPlanes containing six cutting planes. The output is a DataSet, that is 
// either a shallow copy of the input dataset with two new "insidedness" 
// attribute arrays, or a completely new UnstructuredGrid that contains only 
// the cells and points of the input that are inside the frustum. The 
// PassThrough Flag controls which occurs.
//
// You have two choices for what cells are considered to be inside.
// IncludePartialOff treats a cell as inside only if all of its points are 
// inside. This is equivalent to the inside only mode of vtkExtractGeometry 
// filter. IncludePartialOn treats a cell as inside if any part of it is 
// inside. Points are considered to be inside if they are part of an inside 
// cell. An example is a line segment that crosses the frustum but has both
// vertices outside.
//
// .SECTION See Also
// vtkExtractGeometry

#ifndef __vtkFrustumExtractor_h
#define __vtkFrustumExtractor_h

#include "vtkDataSetAlgorithm.h"

class vtkPlanes;
class vtkInformation;
class vtkInformationVector;

class VTK_GRAPHICS_EXPORT vtkFrustumExtractor : public vtkDataSetAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkFrustumExtractor,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkFrustumExtractor *New();

  // Description:
  // Return the MTime taking into account changes to the Frustum
  unsigned long GetMTime();

  // Description:
  // Set the selection frustum. The planes object must contain six planes.
  virtual void SetFrustum(vtkPlanes*);
  vtkGetObjectMacro(Frustum,vtkPlanes);
  
  // Description:
  // Sets/Gets the output data type.
  // If On, the input data set is shallow copied through, and two new
  // "vtkInsidedness" attribute arrays are added. If Off the output is 
  // a new vtkUnstructuredGrid containing only the structure that is inside.
  // Off is the default.
  vtkSetMacro(PassThrough,int);
  vtkGetMacro(PassThrough,int);
  vtkBooleanMacro(PassThrough,int);
  
  // Description:
  // Sets/gets the intersection test type.
  // Off extracts only those points and cells that are completely within the 
  // frustum. On extracts all of the above as well as those cells that cross 
  // the frustum along with all of their points.
  // On is the default.
  vtkSetMacro(IncludePartial,int);
  vtkGetMacro(IncludePartial,int);
  vtkBooleanMacro(IncludePartial,int);

  // Description:
  // Sets/Gets the conditional execution flag.
  // If On (the default) this filter is allowed to process its input.
  // If Off this filter is not allowed to update.
  // The purpose is to allow this possibly expensive algorithm to run only when
  // the faster bounding box test passes. See vtkAreaPicker::AddPropWatcher.
  vtkSetMacro(AllowExecute,int);
  vtkGetMacro(AllowExecute,int);
  vtkBooleanMacro(AllowExecute,int);
  
protected:
  vtkFrustumExtractor(vtkPlanes *f=NULL);
  ~vtkFrustumExtractor();

  //execution
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int ExecutePassThrough(vtkInformationVector **, vtkInformationVector *);
  virtual int ExecuteCreateUGrid(vtkInformationVector **, vtkInformationVector *);

  //sets up output data type
  virtual int RequestDataObject(vtkInformation* request, 
                                vtkInformationVector** inputVector, 
                                vtkInformationVector* outputVector);


  vtkPlanes *Frustum;
  int PassThrough;
  int IncludePartial;
  int AllowExecute;

private:
  vtkFrustumExtractor(const vtkFrustumExtractor&);  // Not implemented.
  void operator=(const vtkFrustumExtractor&);  // Not implemented.
};

#endif


