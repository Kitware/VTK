/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInterpolatedVelocityField.h
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
// .NAME vtkInterpolatedVelocityField - Interface for obtaining
// interpolated velocity values
// .SECTION Description
// vtkInterpolatedVelocityField acts as a continuous velocity field
// by performing cell interpolation on the underlying vtkDataSet.
// This is a concrete sub-class of vtkFunctionSet with 
// NumberOfIndependentVariables = 4 (x,y,z,t) and 
// NumberOfFunctions = 3 (u,v,w). Normally, every time an evaluation
// is performed, the cell which contains the point (x,y,z) has to
// be found by calling FindCell. This is a computationally expansive 
// operation. In certain cases, the cell search can be avoided or shortened 
// by providing a guess for the cell id. For example, in streamline
// integration, the next evaluation is usually in the same or a neighbour
// cell. For this reason, vtkInterpolatedVelocityField stores the last
// cell id. If caching is turned on, it uses this id as the starting point.

// .SECTION Caveats
// vtkInterpolatedVelocityField is not thread safe. A new instance should
// be created by each thread.

// .SECTION See Also
// vtkFunctionSet vtkStreamer

#ifndef __vtkInterpolatedVelocityField_h
#define __vtkInterpolatedVelocityField_h

#include "vtkFunctionSet.h"
#include "vtkDataSet.h"
#include "vtkGenericCell.h"

class VTK_COMMON_EXPORT vtkInterpolatedVelocityField : public vtkFunctionSet
{
public:
  vtkTypeRevisionMacro(vtkInterpolatedVelocityField,vtkFunctionSet);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct a vtkInterpolatedVelocityField with no initial data set.
  // Caching is on. LastCellId is set to -1.
  static vtkInterpolatedVelocityField *New();

  // Description:
  // Evaluate the velocity field, f, at (x, y, z, t).
  // For now, t is ignored.
  virtual int FunctionValues(float* x, float* f);

  // Description:
  // Set / get the dataset used for the implicit function evaluation.
  virtual void SetDataSet(vtkDataSet* dataset);
  vtkGetObjectMacro(DataSet,vtkDataSet);

  // Description:
  // Return the cell id cached from last evaluation.
  vtkGetMacro(LastCellId, vtkIdType);
  vtkSetMacro(LastCellId, vtkIdType);

  // Description:
  // Set the last cell id to -1 so that the next search does not
  // start from the previous cell
  void ClearLastCellId() { this->LastCellId = -1; }

  // Description: 
  // Returns the interpolation weights cached from last evaluation
  // if the cached cell is valid (returns 1). Otherwise, it does not
  // change w and returns 0.
  int GetLastWeights(float* w);
  int GetLastLocalCoordinates(float pcoords[3]);

  // Description:
  // Turn caching on/off.
  vtkGetMacro(Caching, int);
  vtkSetMacro(Caching, int);
  vtkBooleanMacro(Caching, int);

  // Description:
  // Caching statistics.
  vtkGetMacro(CacheHit, int);
  vtkGetMacro(CacheMiss, int);
  
protected:
  vtkInterpolatedVelocityField();
  ~vtkInterpolatedVelocityField();

  vtkDataSet* DataSet;
  vtkGenericCell* GenCell; // last cell
  vtkGenericCell* Cell;
  float* Weights; // last weights
  float LastPCoords[3]; // last local coordinates
  vtkIdType LastCellId;
  int CacheHit;
  int CacheMiss;
  int Caching;
private:
  vtkInterpolatedVelocityField(const vtkInterpolatedVelocityField&);  // Not implemented.
  void operator=(const vtkInterpolatedVelocityField&);  // Not implemented.
};

#endif








