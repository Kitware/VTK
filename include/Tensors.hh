/*=========================================================================

  Program:   Visualization Library
  Module:    Tensors.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlTensors - abstract interface to tensors
// .SECTION Description
// vlTensors provides an abstract interface to n-dimensional tensors. The 
// data model for vlTensors is a list of arrays of nxn tensor matrices 
// accessible by point id. The subclasses of vlTensors are concrete data 
// types (float, int, etc.) that implement the interface of vlTensors.

#ifndef __vlTensors_h
#define __vlTensors_h

#include "RefCount.hh"
#include "Tensor.hh"

class vlIdList;
class vlFloatTensors;

class vlTensors : public vlRefCount 
{
public:
  vlTensors(int dim=3);
  virtual ~vlTensors() {};
  char *GetClassName() {return "vlTensors";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Create a copy of this object.
  virtual vlTensors *MakeObject(int sze, int d=3, int ext=1000) = 0;

  // Description:
  // Return number of tensors in array.
  virtual int GetNumberOfTensors() = 0;

  // Description:
  // Return a float tensor t[dim*dim] for a particular point id.
  virtual vlTensor &GetTensor(int id) = 0;

  // Description:
  // Copy float tensor into user provided tensor
  // for specified point id.
  virtual void GetTensor(int id, vlTensor& t);

  // Description:
  // Insert tensor into object. No range checking performed (fast!).
  virtual void SetTensor(int id, vlTensor& t) = 0;

  // Description:
  // Insert tensor into object. Range checking performed and 
  // memory allocated as necessary.
  virtual void InsertTensor(int id, vlTensor &t) = 0;

  // Description:
  // Insert tensor into next available slot. Returns point
  // id of slot.
  virtual int InsertNextTensor(vlTensor &t) = 0;

  // Description:
  // Reclaim any extra memory.
  virtual void Squeeze() = 0;

  void GetTensors(vlIdList& ptId, vlFloatTensors& ft);

  vlSetClampMacro(Dimension,int,1,3);
  vlGetMacro(Dimension,int);

protected:
  int Dimension;

};

// These include files are placed here so that if Tensors.hh is included 
// all other classes necessary for compilation are also included. 
#include "IdList.hh"
#include "FTensors.hh"

#endif
