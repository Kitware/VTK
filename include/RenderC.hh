/*=========================================================================

  Program:   Visualization Library
  Module:    RenderC.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#ifndef __vlRendererCollection_hh
#define __vlRendererColleciton_hh

#include "Renderer.hh"

class vlRendererListElement
{
 public:
  vlRenderer *Renderer;
  vlRendererListElement *Next;

};

class vlRendererCollection : public vlObject
{
 public:
  int NumberOfItems;

 private:
  vlRendererListElement *Top;
  vlRendererListElement *Bottom;

 public:
  void Render();
  char *GetClassName() {return "vlRendererCollection";};
  void PrintSelf(ostream& os, vlIndent indent);
  vlRendererCollection();
  void AddMember(vlRenderer *);
  int  GetNumberOfMembers();
  vlRenderer *GetMember(int num);
};

#endif
