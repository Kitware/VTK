/*=========================================================================

  Program:   Visualization Library
  Module:    LightC.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

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
  char *GetClassName() {return "vlLightCollection";};
  void AddMember(vlLight *);
  int  GetNumberOfMembers();
  vlLight *GetMember(int num);
  void PrintSelf(ostream& os, vlIndent indent);
};

#endif
