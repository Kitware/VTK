/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLMaterialParser.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2003 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "vtkXMLMaterialParser.h"

#include "vtkXMLMaterial.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkXMLDataElement.h"

#include "vtkXMLUtilities.h"

#include <vtkstd/vector>


//-----------------------------------------------------------------------------
class vtkXMLMaterialParserInternals
{
public:
  typedef vtkstd::vector<vtkSmartPointer<vtkXMLDataElement> > VectorOfElements;
  VectorOfElements Stack;
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkXMLMaterialParser);
vtkCxxSetObjectMacro(vtkXMLMaterialParser, Material, vtkXMLMaterial);

//-----------------------------------------------------------------------------
vtkXMLMaterialParser::vtkXMLMaterialParser()
{
  this->Material = vtkXMLMaterial::New();
  this->Material->Register(this);
  this->Material->Delete();
  this->Internals = new vtkXMLMaterialParserInternals;
}

//-----------------------------------------------------------------------------
vtkXMLMaterialParser::~vtkXMLMaterialParser()
{
  delete this->Internals;
  this->SetMaterial(0);
}

//-----------------------------------------------------------------------------
int vtkXMLMaterialParser::Parse(const char* str)
{
  return this->Superclass::Parse(str);
}

//-----------------------------------------------------------------------------
int vtkXMLMaterialParser::Parse(const char* str, unsigned int length)
{
  return this->Superclass::Parse(str, length);
}

//-----------------------------------------------------------------------------
int vtkXMLMaterialParser::Parse()
{
  this->Internals->Stack.clear();
  return this->Superclass::Parse();
}

//-----------------------------------------------------------------------------
int vtkXMLMaterialParser::InitializeParser()
{
  int ret = this->Superclass::InitializeParser();
  if (ret)
    {
    this->Internals->Stack.clear();
    }
  return ret;
}

//-----------------------------------------------------------------------------
void vtkXMLMaterialParser::StartElement(const char* name, const char** atts)
{
  vtkXMLDataElement* element = vtkXMLDataElement::New();
  element->SetName(name);
  element->SetXMLByteIndex(this->GetXMLByteIndex());
  vtkXMLUtilities::ReadElementFromAttributeArray(element, atts, VTK_ENCODING_NONE);
  const char* id = element->GetAttribute("id");
  if (id)
    {
    element->SetId(id);
    }
  this->Internals->Stack.push_back(element);
  element->Delete();
}

//-----------------------------------------------------------------------------
void vtkXMLMaterialParser::EndElement(const char* vtkNotUsed(name))
{
  vtkXMLDataElement* finished = this->Internals->Stack.back().GetPointer();
  int prev_pos = static_cast<int>(this->Internals->Stack.size()) - 2;
  if (prev_pos >= 0)
    {
    this->Internals->Stack[prev_pos].GetPointer()->AddNestedElement(finished);
    }
  else
    {
    this->Material->SetRootElement(finished);
    }

  this->Internals->Stack.pop_back();
}

//-----------------------------------------------------------------------------
void vtkXMLMaterialParser::CharacterDataHandler( const char* inData, int inLength )
{
  if (this->Internals->Stack.size() > 0)
    {
    vtkXMLDataElement* elem = this->Internals->Stack.back().GetPointer();
    elem->AddCharacterData(inData, inLength);
    }
  /*
  // this wont happen as the XML parser will flag it as an error.
  else
    {
    vtkErrorMacro("Character data not enclosed in XML tags");
    }
  */
}

//-----------------------------------------------------------------------------
void vtkXMLMaterialParser::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Material: " ;
  this->Material->PrintSelf(os, indent.GetNextIndent());
}

