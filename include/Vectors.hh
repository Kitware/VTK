/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Vectors.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkVectors - abstract interface to 3D vectors
// .SECTION Description
// vtkVectors provides an abstract interface to 3D vectors. The data model
// for vtkVectors is an array of vx-vy-vz triplets accessible by point id.
// The subclasses of vtkVectors are concrete data types (float, int, etc.)
// that implement the interface of vtkVectors.

#ifndef __vtkVectors_h
#define __vtkVectors_h

#include "RefCount.hh"

class vtkIdList;
class vtkFloatVectors;

class vtkVectors : public vtkRefCount 
{
public:
  vtkVectors();
  virtual ~vtkVectors() {};
  char *GetClassName() {return "vtkVectors";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a copy of this object.
  virtual vtkVectors *MakeObject(int sze, int ext=1000) = 0;

  // Description:
  // Return data type. One of "bit", "char", "short", "int", "float", or
  // "double".
  virtual char *GetDataType() = 0;

  // Description:
  // Return number of vectors in array.
  virtual int GetNumberOfVectors() = 0;

  // Description:
  // Return a pointer to a float vector v[3] for a specific point id.
  virtual float *GetVector(int id) = 0;

  // Description:
  // Copy vector componenets into user provided array v[3] for specified
  // point id.
  virtual void GetVector(int id, float v[3]);

  // Description:
  // Insert vector into object. No range checking performed (fast!).
  virtual void SetVector(int id, float v[3]) = 0;

  // Description:
  // Insert vector into object. Range checking performed and memory
  // allocated as necessary.
  virtual void InsertVector(int id, float v[3]) = 0;

  // Description:
  // Insert vector into next available slot. Returns point id of slot.
  virtual int InsertNextVector(float v[3]) = 0;

  // Description:
  // Reclaim any extra memory.
  virtual void Squeeze() = 0;

  void GetVectors(vtkIdList& ptId, vtkFloatVectors& fp);
  virtual void ComputeMaxNorm();
  float GetMaxNorm();

protected:
  float MaxNorm;
  vtkTimeStamp ComputeTime; // Time at which MaxNorm computed
};

// These include files are placed here so that if Vectors.hh is included 
// all other classes necessary for compilation are also included. 
#include "IdList.hh"
#include "FVectors.hh"

#endif
