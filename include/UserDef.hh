/*=========================================================================

  Program:   Visualization Toolkit
  Module:    UserDef.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkUserDefined - interface to user defined data
// .SECTION Description
// vtkUserDefined provides an abstract interface to user defined data. User 
// defined data are manipulated using void* pointers. These pointers are
// accessed via point id, so information can be represented on a per vertex 
// basis.

#ifndef __vtkUserDefined_h
#define __vtkUserDefined_h

#include "RefCount.hh"
#include "VArray.hh"
#include "IdList.hh"

class vtkUserDefined : public vtkRefCount 
{
public:
  vtkUserDefined() {};
  vtkUserDefined(const vtkUserDefined& ud) {this->UD = ud.UD;};
  vtkUserDefined(const int sz, const int ext=1000):UD(sz,ext){};
  ~vtkUserDefined() {};
  int Allocate(const int sz, const int ext=1000) {return this->UD.Allocate(sz,ext);};
  void Initialize() {this->UD.Initialize();};
  char *GetClassName() {return "vtkUserDefined";};
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkUserDefined *MakeObject(int sze, int ext=1000);
  int GetNumberOfUserDefined() {return (this->UD.GetMaxId()+1);};
  void Squeeze() {this->UD.Squeeze();};
  void* GetUserDefined(int i) {return this->UD[i];};
  void SetUserDefined(int i, void *ud) {this->UD[i] = ud;};
  void InsertUserDefined(int i, void *ud) {UD.InsertValue(i,ud);};
  int InsertNextUserDefined(void *ud) {return UD.InsertNextValue(ud);};
  void GetUserDefined(vtkIdList& ptId, vtkUserDefined& ud);

  vtkUserDefined &operator=(const vtkUserDefined& ud);
  void operator+=(const vtkUserDefined& ud) {this->UD += ud.UD;};
  void Reset() {this->UD.Reset();};

  // Description:
  // Interpolate user defined data. Method must be supplied by user. Return
  // value must be non-static void* pointer to data.
  virtual void* Interpolate(float *weights) {return NULL;};

protected:
  vtkVoidArray UD;
};

#endif
