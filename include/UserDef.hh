/*=========================================================================

  Program:   Visualization Library
  Module:    UserDef.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlUserDefined - interface to user defined data
// .SECTION Description
// vlUserDefined provides an abstract interface to user defined data. User 
// defined data are manipulated using void* pointers. These pointers are
// accessed via point id, so information can be represented on a per vertex 
// basis.

#ifndef __vlUserDefined_h
#define __vlUserDefined_h

#include "RefCount.hh"
#include "VArray.hh"
#include "IdList.hh"

class vlUserDefined : public vlRefCount 
{
public:
  vlUserDefined() {};
  vlUserDefined(const vlUserDefined& ud) {this->UD = ud.UD;};
  vlUserDefined(const int sz, const int ext=1000):UD(sz,ext){};
  ~vlUserDefined() {};
  int Allocate(const int sz, const int ext=1000) {return this->UD.Allocate(sz,ext);};
  void Initialize() {this->UD.Initialize();};
  char *GetClassName() {return "vlUserDefined";};
  void PrintSelf(ostream& os, vlIndent indent);

  vlUserDefined *MakeObject(int sze, int ext=1000);
  int GetNumberOfUserDefined() {return (this->UD.GetMaxId()+1);};
  void Squeeze() {this->UD.Squeeze();};
  void* GetUserDefined(int i) {return this->UD[i];};
  void SetUserDefined(int i, void *ud) {this->UD[i] = ud;};
  void InsertUserDefined(int i, void *ud) {UD.InsertValue(i,ud);};
  int InsertNextUserDefined(void *ud) {return UD.InsertNextValue(ud);};
  void GetUserDefined(vlIdList& ptId, vlUserDefined& ud);

  vlUserDefined &operator=(const vlUserDefined& ud);
  void operator+=(const vlUserDefined& ud) {this->UD += ud.UD;};
  void Reset() {this->UD.Reset();};

  // Description:
  // Interpolate user defined data. Method must be supplied by user. Return
  // value must be non-static void* pointer to data.
  virtual void* Interpolate(float *weights) {return NULL;};

protected:
  vlVoidArray UD;
};

#endif
