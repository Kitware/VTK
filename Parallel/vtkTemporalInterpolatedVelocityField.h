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
// .NAME vtkTemporalInterpolatedVelocityField - A helper class for 
// interpolating between times during particle tracing
// .SECTION Description
// vtkTemporalInterpolatedVelocityField is a temporal version of 
// vtk vtkInterpolatedVelocityField. I maintains two copies of 
// vtkInterpolatedVelocityField internally and uses them to obtain velocity
// values at time T1 and T2. 
// In fact the class does quite a bit more than this because when the geometry
// of the datasets is the same at T1 and T2, we can re-use cached cell Ids and 
// weights used in the cell interpolation routines.
// Additionally, the same weights can be used when interpolating (point) scalar 
// values and computing vorticity etc, so the class acts as a general purpose
// helper for the temporal particle tracing code (vtkTemporalStreamTracer)
// .SECTION Caveats
// vtkTemporalInterpolatedVelocityField is not thread safe. 
// A new instance should be created by each thread.
//
// .SECTION See Also
// vtkFunctionSet vtkStreamer 
// vtkInterpolatedVelocityField vtkTemporalStreamTracer

#ifndef __vtkTemporalInterpolatedVelocityField_h
#define __vtkTemporalInterpolatedVelocityField_h

#include "vtkFunctionSet.h"

#define ID_INSIDE_ALL  00
#define ID_OUTSIDE_ALL 01
#define ID_OUTSIDE_T1  02
#define ID_OUTSIDE_T2  03

class vtkDataSet;
class vtkDataArray;
class vtkPointData;
class vtkGenericCell;
class vtkDoubleArray;
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
  // here. By default this is NULL and the filter will use the active vector 
  // array.
  void SelectVectors(const char *fieldName) 
    {this->SetVectorsSelection(fieldName);}

  // Description:
  // In order to use this class, two sets of data must be supplied, 
  // corresponding to times T1 and T2. Data is added via
  // this function.
  void AddDataSetAtTime(int N, double T, vtkDataSet* dataset);

  // Description:
  // Allow the algorithm using us to turn on/off caching
  // of CellIds between datasets. If true, then calculations
  // on the second set of data (T2) use the weights from T1
  // without any error checking. Use with caution.
  // We don't use a SetGetMacro to avoid calling Modified()
  void SetGeometryFixed(int g) 
  { this->GeometryFixed = g; }
  int GetGeometryFixed() 
  { return this->GeometryFixed; }

  // Description:
  // Between iterations of the PArticle TRacer, Id's of the Cell
  // are stored and then at the start of the next particle the
  // Ids are set to 'pre-fill' the cache.
  bool GetCachedCellIds(vtkIdType id[2], int ds[2]);
  void SetCachedCellIds(vtkIdType id[2], int ds[2]);

  // Description:
  // Set the last cell id to -1 so that the next search does not
  // start from the previous cell
  void ClearCache();

  // Description:
  // A utility function which evaluates the point at T1, T2 to see 
  // if it is inside the data at both times or only one.
  int TestPoint(double* x);

  // Description:
  // If an interpolation was successful, we can retrieve the last computed
  // value from here.
  vtkGetVector3Macro(LastGoodVelocity,double);

  // Description:
  // Get the most recent weight between 0->1 from T1->T2
  vtkGetMacro(CurrentWeight,double);
  
  bool InterpolatePoint(
    vtkPointData *outPD1, vtkPointData *outPD2, vtkIdType outIndex);
  bool InterpolatePointAtT(
    int T, vtkPointData *outPD, vtkIdType outIndex);

  bool GetVorticityData(
    int T, double pcoords[3], double *weights, 
    vtkGenericCell *&cell, vtkDoubleArray *cellVectors);

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

  // The weight (0.0->1.0) of the value of T between the two avaiable 
  // time values for the current computation
  double CurrentWeight;
  // One minus the CurrentWeight 
  double OneMinusWeight;
  // A scaling factor used when calculating the CurrentWeight { 1.0/(T2-T1) }
  double ScaleCoeff;
  // If the two timesteps have the same geometry, then this flag is set and
  // weights can be re-used between them.
  int    GeometryFixed;
  //BTX
  // Datasets per time step
  vtkTInterpolatedVelocityFieldDataSetsType* DataSets[2];
  //ETX

private:
  // Hide this since we need multiple time steps and are using a different
  // function prototype
  virtual void AddDataSet(vtkDataSet*) {};

private:
  vtkTemporalInterpolatedVelocityField(const vtkTemporalInterpolatedVelocityField&);  // Not implemented.
  void operator=(const vtkTemporalInterpolatedVelocityField&);  // Not implemented.
};

#endif
