/*=========================================================================

  Program:   Visualization Toolkit
  Module:    CoScalar.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkColorScalars - abstract class represents scalar data in color specification
// .SECTION Description
// vtkColorScalars is an abstract class whose subclasses represent scalar
// data using a color specification such as rgb, grayscale, rgba, hsv, etc. 
//    In order to be a vtkScalar subclass, vtkColorScalars must be able to 
// return a single value given a point id. By default, this operation is 
// performed by computing intensity as the single value. Concrete subclasses 
// of vtkColorScalars may have additional methods to convert multi-dimensional
// color information into a single scalar value.
// .SECTION Caveats
// Derived classes of vtkColorScalars treat colors differently. All derived 
// classes will return a rgba (red-green-blue-alpha transparency) array in
// response to "GetColor()" methods. However, when setting colors, the rgba
// data may be converted to internal form. For example, a vtkGrayMap just
// takes the maximum component of rgb and uses that as its gray value.

#ifndef __vtkColorScalars_h
#define __vtkColorScalars_h

#include "Scalars.hh"

class vtkAPixmap;

class vtkColorScalars : public vtkScalars 
{
public:
  vtkColorScalars() {};
  ~vtkColorScalars() {};
  char *GetClassName() {return "vtkColorScalars";};

  // vtkScalars interface
  char *GetScalarType() {return "ColorScalar";};
  char *GetDataType() {return "unsigned char";};
  float GetScalar(int i);
  void SetScalar(int i, float s);
  void InsertScalar(int i, float s);
  int InsertNextScalar(float s);

  // Description:
  // Get pointer to array of data starting at data position "id".
  virtual unsigned char *GetPtr(const int id) = 0;

  // abstract interface for vtkColorScalars
  // Description:
  // Return number of colors (same as number of scalars).
  int GetNumberOfColors() {return this->GetNumberOfScalars();};  

  // Description:
  // Return a unsigned char rgba for a particular point id. No matter
  // what internal representation of color, derived class must convert it
  // to rgba.
  virtual unsigned char *GetColor(int id) = 0;

  // Description:
  // Copy color components into user provided array rgba[4] for specified
  // point id. No matter what internal representation of color, derived 
  // class must convert it to rgba form.
  virtual void GetColor(int id, unsigned char rgba[4]) = 0;

  // Description:
  // Insert color into object. No range checking performed (fast!).
  virtual void SetColor(int id, unsigned char rgba[4]) = 0;

  // Description:
  // Insert color into object. Range checking performed and memory
  // allocated as necessary.
  virtual void InsertColor(int id, unsigned char rgba[4]) = 0;

  // Description:
  // Insert color into next available slot. Returns point id of slot.
  virtual int InsertNextColor(unsigned char rgba[4]) = 0;

  // Description:
  // Allocate space for color data.
  virtual int Allocate(const int sz, const int ext=1000) = 0;

  void GetColors(vtkIdList& ptId, vtkAPixmap& ap);

  void GetComponentRange(unsigned char range[8]);
  unsigned char *GetComponentRange();
};

// These include files are placed here so that if CoScalar.hh is included 
// all other classes necessary for compilation are also included. 
#include "IdList.hh"
#include "APixmap.hh"

#endif
