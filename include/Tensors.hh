/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Tensors.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkTensors - abstract interface to tensors
// .SECTION Description
// vtkTensors provides an abstract interface to n-dimensional tensors. The 
// data model for vtkTensors is a list of arrays of nxn tensor matrices 
// accessible by point id. The subclasses of vtkTensors are concrete data 
// types (float, int, etc.) that implement the interface of vtkTensors.

#ifndef __vtkTensors_h
#define __vtkTensors_h

#include "RefCount.hh"
#include "Tensor.hh"

class vtkIdList;
class vtkFloatTensors;

class vtkTensors : public vtkRefCount 
{
public:
  vtkTensors(int dim=3);
  virtual ~vtkTensors() {};
  char *GetClassName() {return "vtkTensors";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a copy of this object.
  virtual vtkTensors *MakeObject(int sze, int d=3, int ext=1000) = 0;

  // Description:
  // Return data type. One of "bit", "unsigned char", "short", "int", "float", or
  // "double".
  virtual char *GetDataType() = 0;

  // Description:
  // Return number of tensors in array.
  virtual int GetNumberOfTensors() = 0;

  // Description:
  // Return a float tensor t[dim*dim] for a particular point id.
  virtual vtkTensor *GetTensor(int id) = 0;

  // Description:
  // Copy float tensor into user provided tensor
  // for specified point id.
  virtual void GetTensor(int id, vtkTensor& t);

  // Description:
  // Insert tensor into object. No range checking performed (fast!).
  virtual void SetTensor(int id, vtkTensor *t) = 0;

  // Description:
  // Insert tensor into object. Range checking performed and 
  // memory allocated as necessary.
  virtual void InsertTensor(int id, vtkTensor *t) = 0;
  void InsertTensor(int id, float t11, float t12, float t13, 
                    float t21, float t22, float t23, 
                    float t31, float t32, float t33);

  // Description:
  // Insert tensor into next available slot. Returns point
  // id of slot.
  virtual int InsertNextTensor(vtkTensor *t) = 0;
  int InsertNextTensor(float t11, float t12, float t13, 
                       float t21, float t22, float t23, 
                       float t31, float t32, float t33);

  // Description:
  // Reclaim any extra memory.
  virtual void Squeeze() = 0;

  void GetTensors(vtkIdList& ptId, vtkFloatTensors& ft);

  vtkSetClampMacro(Dimension,int,1,3);
  vtkGetMacro(Dimension,int);

protected:
  int Dimension;

};

// These include files are placed here so that if Tensors.hh is included 
// all other classes necessary for compilation are also included. 
#include "IdList.hh"
#include "FTensors.hh"

#endif
