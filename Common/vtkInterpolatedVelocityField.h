/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInterpolatedVelocityField.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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

class vtkDataSet;
class vtkGenericCell;

class vtkInterpolatedVelocityFieldDataSetsType;

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
  virtual int FunctionValues(double* x, double* f);

  // Description:
  // Add a dataset used for the implicit function evaluation.
  // If more than one dataset is added, the evaluation point is
  // searched in all until a match is found. THIS FUNCTION
  // DOES NOT CHANGE THE REFERENCE COUNT OF dataset FOR THREAD
  // SAFETY REASONS.
  virtual void AddDataSet(vtkDataSet* dataset);

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
  int GetLastWeights(double* w);
  int GetLastLocalCoordinates(double pcoords[3]);

  // Description:
  // Turn caching on/off.
  vtkGetMacro(Caching, int);
  vtkSetMacro(Caching, int);
  vtkBooleanMacro(Caching, int);

  // Description:
  // Caching statistics.
  vtkGetMacro(CacheHit, int);
  vtkGetMacro(CacheMiss, int);

  // Description:
  // If you want to work with an arbitrary vector array, then set its name 
  // here. By default this in NULL and the filter will use the active vector 
  // array.
  vtkGetStringMacro(VectorsSelection);
  void SelectVectors(const char *fieldName) 
    {this->SetVectorsSelection(fieldName);}
  
  // Description:
  vtkGetObjectMacro(LastDataSet, vtkDataSet);

protected:
  vtkInterpolatedVelocityField();
  ~vtkInterpolatedVelocityField();

  vtkGenericCell* GenCell; // last cell
  vtkGenericCell* Cell;
  double* Weights; // last weights
  int WeightsSize;
  double LastPCoords[3]; // last local coordinates
  vtkIdType LastCellId;
  int CacheHit;
  int CacheMiss;
  int Caching;

  vtkDataSet* LastDataSet;

  vtkSetStringMacro(VectorsSelection);
  char *VectorsSelection;

  vtkInterpolatedVelocityFieldDataSetsType* DataSets;

  int FunctionValues(vtkDataSet* ds, double* x, double* f);

  static const double TOLERANCE_SCALE;

private:
  vtkInterpolatedVelocityField(const vtkInterpolatedVelocityField&);  // Not implemented.
  void operator=(const vtkInterpolatedVelocityField&);  // Not implemented.
};

#endif








