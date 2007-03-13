/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTemporalInterpolatedVelocityField.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTemporalInterpolatedVelocityField - Interface for obtaining
// interpolated velocity values
// .SECTION Description
// vtkTemporalInterpolatedVelocityField acts as a continuous velocity field
// by performing cell interpolation on the underlying vtkDataSet.
// This is a concrete sub-class of vtkFunctionSet with 
// NumberOfIndependentVariables = 4 (x,y,z,t) and 
// NumberOfFunctions = 3 (u,v,w). Normally, every time an evaluation
// is performed, the cell which contains the point (x,y,z) has to
// be found by calling FindCell. This is a computationally expansive 
// operation. In certain cases, the cell search can be avoided or shortened 
// by providing a guess for the cell id. For example, in streamline
// integration, the next evaluation is usually in the same or a neighbour
// cell. For this reason, vtkTemporalInterpolatedVelocityField stores the last
// cell id. If caching is turned on, it uses this id as the starting point.

// .SECTION Caveats
// vtkTemporalInterpolatedVelocityField is not thread safe. A new instance should
// be created by each thread.

// .SECTION See Also
// vtkFunctionSet vtkStreamer

#ifndef __vtkTemporalInterpolatedVelocityField_h
#define __vtkTemporalInterpolatedVelocityField_h

#include "vtkFunctionSet.h"

#define ID_INSIDE_ALL  00
#define ID_OUTSIDE_ALL 01
#define ID_OUTSIDE_T1  02
#define ID_OUTSIDE_T2  03

class vtkDataSet;
class vtkGenericCell;
class vtkInterpolatedVelocityField;
class vtkTInterpolatedVelocityFieldDataSetsType;

class VTK_PARALLEL_EXPORT vtkTemporalInterpolatedVelocityField : public vtkFunctionSet
{
public:
  vtkTypeRevisionMacro(vtkTemporalInterpolatedVelocityField,vtkFunctionSet);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct a vtkTemporalInterpolatedVelocityField with no initial data set.
  // Caching is on. LastCellId is set to -1.
  static vtkTemporalInterpolatedVelocityField *New();

  // Description:
  // Evaluate the velocity field, f, at (x, y, z, t).
  // For now, t is ignored.
  virtual int FunctionValues(double* x, double* u);

  // Description:
  // If you want to work with an arbitrary vector array, then set its name 
  // here. By default this in NULL and the filter will use the active vector 
  // array.
  void SelectVectors(const char *fieldName) 
    {this->SetVectorsSelection(fieldName);}

  void AddDataSetAtTime(int N, double T, vtkDataSet* dataset);

  // Description:
  // Allow the algorithm using us to turn on/off caching
  // of CellIds between datasets
  // Don't use a SetGetMacro to avoid calling Modified()
  void SetGeometryFixed(int g) 
  { this->GeometryFixed = g; }
  int GetGeometryFixed() 
  { return this->GeometryFixed; }

  // Description:
  bool GetCachedCellIds(vtkIdType id[2], int ds[2]);
  void SetCachedCellIds(vtkIdType id[2], int ds[2]);

  // Description:
  // Set the last cell id to -1 so that the next search does not
  // start from the previous cell
  void ClearCache();

  int TestPoint(double* x);

  vtkGetVector3Macro(LastGoodVelocity,double);

protected:
  vtkTemporalInterpolatedVelocityField();
  ~vtkTemporalInterpolatedVelocityField();

  vtkInterpolatedVelocityField *ivf[2];

  int FunctionValues(vtkDataSet* ds, double* x, double* f);
  virtual void SetVectorsSelection(const char *v);

  double vals1[3];
  double vals2[3];
  double times[2];
  double LastGoodVelocity[3];
  double scalecoeff;
  int    GeometryFixed;
  //BTX
  // Datasets per time step
  vtkTInterpolatedVelocityFieldDataSetsType* DataSets[2];
  //ETX

private:
  // Hide this since we need multiple time steps and are using a differnt
  // function prototype
  virtual void AddDataSet(vtkDataSet*) {};

private:
  vtkTemporalInterpolatedVelocityField(const vtkTemporalInterpolatedVelocityField&);  // Not implemented.
  void operator=(const vtkTemporalInterpolatedVelocityField&);  // Not implemented.
};

#endif
