/*=========================================================================

  Program:   OSCAR 
  Module:    RenderC.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the vis library

- Ken Martin

=========================================================================*/
#include <stdlib.h>
#include <iostream.h>
#include "RenderC.hh"

vlRendererCollection::vlRendererCollection()
{
  this->NumberOfItems = 0;
  this->Top = NULL;
  this->Bottom = NULL;
}

void vlRendererCollection::AddMember(vlRenderer *Renderer)
{
  vlRendererListElement *elem;

  elem = new vlRendererListElement;
  
  if (!this->Top)
    {
    this->Top = elem;
    }
  else
    {
    this->Bottom->Next = elem;
    }
  this->Bottom = elem;

  elem->Renderer = Renderer;
  elem->Next = NULL;

  this->NumberOfItems++;
}

int vlRendererCollection::GetNumberOfMembers()
{
  return this->NumberOfItems;
}

vlRenderer *vlRendererCollection::GetMember(int num)
{
  int i;
  vlRendererListElement *elem;

  if (num > this->NumberOfItems)
    {
    cerr << "Renderer: Requesting illegal index\n";
    return this->Top->Renderer;
    }

  elem = this->Top;
  for (i = 1; i < num; i++)
    {
    elem = elem->Next;
    }
  
  return (elem->Renderer);
}

void vlRendererCollection::Render()
{
  int i;
  vlRendererListElement *elem;

  elem = this->Top;
  for (i = 0; i < this->NumberOfItems; i++)
    {
    elem->Renderer->Render();
    elem = elem->Next;
    }
  
}
