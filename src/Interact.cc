/*=========================================================================

  Program:   Visualization Library
  Module:    Interact.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "Interact.hh"
#include "Actor.hh"

// Description:
// Construct object so that light follows camera motion.
vlInteractiveRenderer::vlInteractiveRenderer()
{
  this->RenderWindow = NULL;
  this->Camera = NULL;
  this->Light = NULL;

  this->LightFollowCamera = 1;
}

vlInteractiveRenderer::~vlInteractiveRenderer()
{
}

void vlInteractiveRenderer::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlInteractiveRenderer::GetClassName()))
    {
    vlObject::PrintSelf(os,indent);

    os << indent << "RenderWindow: " << this->RenderWindow << "\n";
    os << indent << "Camera: " << this->Camera << "\n";
    os << indent << "Light: " << this->Light << "\n";
    os << indent << "LightFollowCamera: " << (this->LightFollowCamera ? "On\n" : "Off\n");
    }
}

 
