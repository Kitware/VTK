/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TCoords.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkTCoords - abstract interface to texture coordinates
// .SECTION Description
// vtkTCoords provides an abstract interface to 2D or 3D texture coordinates. 
// Texture coordinates are 2D (s,t) or 3D (r,s,t) parametric values that
// map geometry into regular 2D or 3D arrays of color and/or transparency
// values. During rendering the array are mapped onto the geometry for
// fast image detailing. The subclasses of vtkTCoords are concrete data 
// types (float, int, etc.) that implement the interface of vtkTCoords. 

#ifndef __vtkTCoords_h
#define __vtkTCoords_h

#include "RefCount.hh"

class vtkIdList;
class vtkFloatTCoords;

class vtkTCoords : public vtkRefCount
{
public:
  vtkTCoords(int dim=2);
  virtual ~vtkTCoords() {};
  char *GetClassName() {return "vtkTCoords";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a copy of this object.
  virtual vtkTCoords *MakeObject(int sze, int d=2, int ext=1000) = 0;

  // Description:
  // Return data type. One of "bit", "unsigned char", "short", "int", "float", or
  // "double".
  virtual char *GetDataType() = 0;

  // Description:
  // Return number of texture coordinates in array.
  virtual int GetNumberOfTCoords() = 0;

  // Description:
  // Return a float texture coordinate tc[2/3] for a particular point id.
  virtual float *GetTCoord(int id) = 0;

  // Description:
  // Copy float texture coordinates into user provided array tc[2/3] 
  // for specified point id.
  virtual void GetTCoord(int id, float tc[3]);

  // Description:
  // Insert texture coordinate into object. No range checking performed (fast!).
  virtual void SetTCoord(int id, float *tc) = 0;

  // Description:
  // Insert texture coordinate into object. Range checking performed and 
  // memory allocated as necessary.
  virtual void InsertTCoord(int id, float *tc) = 0;
  void InsertTCoord(int id, float tc1, float tc2, float tc3);

  // Description:
  // Insert texture coordinate into next available slot. Returns point
  // id of slot.
  virtual int InsertNextTCoord(float *tc) = 0;
  int InsertNextTCoord(float tc1, float tc2, float tc3);

  // Description:
  // Reclaim any extra memory.
  virtual void Squeeze() = 0;

  void GetTCoords(vtkIdList& ptId, vtkFloatTCoords& fp);

  vtkSetClampMacro(Dimension,int,1,3);
  vtkGetMacro(Dimension,int);

protected:
  int Dimension;

};

// These include files are placed here so that if TCoords.hh is included 
// all other classes necessary for compilation are also included. 
#include "IdList.hh"
#include "FTCoords.hh"

#endif
