/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PtData.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkPointData - represent and manipulate point attribute data
// .SECTION Description
// vtkPointData is a class that is used to represent and manipulate
// point attribute data (e.g., scalars, vectors, normals, texture 
// coordinates, etc.) Special methods are provided to work with filter
// objects such as passing data through filter, copying data from one 
// point to another, and interpolating data given shape functions.

#ifndef __vtkPointData_h
#define __vtkPointData_h

#include "Object.hh"
#include "Scalars.hh"
#include "Vectors.hh"
#include "Normals.hh"
#include "TCoords.hh"
#include "Tensors.hh"
#include "UserDef.hh"

class vtkPointData : public vtkObject 
{
public:
  vtkPointData();
  void Initialize();
  ~vtkPointData();
  char *GetClassName() {return "vtkPointData";};
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkPointData (const vtkPointData& pd);
  vtkPointData &operator=(vtkPointData& pd);
  virtual void Update() {};

  // pass thru all input data to output
  void PassData(vtkPointData* pd);

  // use to copy data on a point by point basis
  void CopyAllocate(vtkPointData* pd, int sze=0, int ext=1000);
  void CopyData(vtkPointData *fromPd, int fromId, int toId);

  // use to interpolate data
  void InterpolateAllocate(vtkPointData* pd, int sze=0, int ext=1000);
  void InterpolatePoint(vtkPointData *fromPd, int toId, vtkIdList *ptIds, float *weights);

  // Set point data to null values
  void NullPoint(int ptId);

  // Reclaim memory
  void Squeeze();

  // Description:
  // Set scalar data.
  vtkSetRefCountedObjectMacro (Scalars, vtkScalars);
  vtkGetObjectMacro (Scalars, vtkScalars);

  // Description:
  // Set vector data.
  vtkSetRefCountedObjectMacro (Vectors, vtkVectors);
  vtkGetObjectMacro (Vectors, vtkVectors);

  // Description:
  // Set normal data.
  vtkSetRefCountedObjectMacro (Normals, vtkNormals);
  vtkGetObjectMacro (Normals, vtkNormals);

  // Description:
  // Set texture coordinate data.
  vtkSetRefCountedObjectMacro (TCoords, vtkTCoords);
  vtkGetObjectMacro (TCoords, vtkTCoords);

  // Description:
  // Set tensor data.
  vtkSetRefCountedObjectMacro (Tensors, vtkTensors);
  vtkGetObjectMacro (Tensors, vtkTensors);

  // Description:
  // Set user defined data.
  vtkSetRefCountedObjectMacro (UserDefined, vtkUserDefined);
  vtkGetObjectMacro (UserDefined, vtkUserDefined);

  // Description:
  // Turn on/off the copying of scalar data.
  vtkSetMacro(CopyScalars,int);
  vtkGetMacro(CopyScalars,int);
  vtkBooleanMacro(CopyScalars,int);

  // Description:
  // Turn on/off the copying of vector data.
  vtkSetMacro(CopyVectors,int);
  vtkGetMacro(CopyVectors,int);
  vtkBooleanMacro(CopyVectors,int);

  // Description:
  // Turn on/off the copying of normals data.
  vtkSetMacro(CopyNormals,int);
  vtkGetMacro(CopyNormals,int);
  vtkBooleanMacro(CopyNormals,int);

  // Description:
  // Turn on/off the copying of texture coordinates data.
  vtkSetMacro(CopyTCoords,int);
  vtkGetMacro(CopyTCoords,int);
  vtkBooleanMacro(CopyTCoords,int);

  // Description:
  // Turn on/off the copying of tensor data.
  vtkSetMacro(CopyTensors,int);
  vtkGetMacro(CopyTensors,int);
  vtkBooleanMacro(CopyTensors,int);

  // Description:
  // Turn on/off the copying of user defined data.
  vtkSetMacro(CopyUserDefined,int);
  vtkGetMacro(CopyUserDefined,int);
  vtkBooleanMacro(CopyUserDefined,int);

  void CopyAllOn();
  void CopyAllOff();

protected:
  vtkScalars *Scalars;
  vtkVectors *Vectors;
  vtkNormals *Normals;
  vtkTCoords *TCoords;
  vtkTensors *Tensors;
  vtkUserDefined *UserDefined;
  int CopyScalars;
  int CopyVectors;
  int CopyNormals;
  int CopyTCoords;
  int CopyTensors;
  int CopyUserDefined;
};

#endif


