/*=========================================================================

  Program:   OSCAR 
  Module:    LightC.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the vis library

- Ken Martin

=========================================================================*/
#ifndef __vlLightC_hh
#define __vlLightC_hh

#include "Light.hh"

class vlLightListElement
{
 public:
  vlLight *Light;
  vlLightListElement *Next;

};

class vlLightCollection : public vlObject
{
 public:
  int NumberOfItems;

 private:
  vlLightListElement *Top;
  vlLightListElement *Bottom;

 public:
  vlLightCollection();
  virtual char *GetClassName() {return "vlLightCollection";};
  void AddMember(vlLight *);
  int  GetNumberOfMembers();
  vlLight *GetMember(int num);
};

#endif
