/*=========================================================================

  Program:   Visualization Library
  Module:    TCoords.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlTCoords - abstract interface to 3D texture coordinates
// .SECTION Description
// vlTCoords provides an abstract interface to 3D texture coordinates. 
// Texture coordinates are 2D (u,v) or 3D (u,v,w) parametric values that
// map geometry into regular 2D or 3D arrays of color and/or transparency
// values. During rendering the array are mapped onto the geometry for
// fast image detailing. The subclasses of vlTCoords are concrete data 
// types (float, int, etc.) that implement the interface of vlTCoords. 

#ifndef __vlTCoords_h
#define __vlTCoords_h

#include "RefCount.hh"

class vlIdList;
class vlFloatTCoords;

class vlTCoords : public vlRefCount
{
public:
  vlTCoords(int dim=2);
  virtual ~vlTCoords() {};
  char *GetClassName() {return "vlTCoords";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Create a copy of this object.
  virtual vlTCoords *MakeObject(int sze, int d=2, int ext=1000) = 0;

  // Description:
  // Return number of texture coordinates in array.
  virtual int GetNumberOfTCoords() = 0;

  // Description:
  // Return a float texture coordinate tc[2/3] for a particular point id.
  virtual float *GetTCoord(int i) = 0;

  // Description:
  // Insert texture coordinate into object. No range checking performed (fast!).
  virtual void SetTCoord(int i,float *tc) = 0;

  // Description:
  // Insert texture coordinate into object. Range checking performed and 
  // memory allocated as necessary.
  virtual void InsertTCoord(int i, float *tc) = 0;

  // Description:
  // Insert texture coordinate into next available slot. Returns point
  // id of slot.
  virtual int InsertNextTCoord(float *tc) = 0;

  // Description:
  // Reclaim any extra memory.
  virtual void Squeeze() = 0;

  void GetTCoords(vlIdList& ptId, vlFloatTCoords& fp);

  vlSetClampMacro(Dimension,int,1,3);
  vlGetMacro(Dimension,int);

protected:
  int Dimension;

};

#endif
