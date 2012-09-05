/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLMaterialParser.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

// .NAME vtkXMLMaterialParser - Parses VTK Material file
//
// .SECTION Description
// vtkXMLMaterialParser parses a VTK Material file and provides that file's
// description of a number of vertex and fragment shaders along with data
// values specified for data members of vtkProperty. This material is to be
// applied to an actor through it's vtkProperty and augments VTK's concept
// of a vtkProperty to include explicitly include vertex and fragment shaders
// and parameter settings for those shaders. This effectively makes reflectance
// models and other shaders  a material property. If no shaders are specified
// VTK should default to standard rendering.
//
// .SECTION Design
// vtkXMLMaterialParser provides access to 3 distinct types of first-level
// vtkXMLDataElements that describe a VTK material. These elements are as
// follows:
//
// vtkProperty - describe values for vtkProperty data members
//
// vtkVertexShader - a vertex shader and enough information to
// install it into the hardware rendering pipeline including values for
// specific shader parameters and structures.
//
// vtkFragmentShader - a fragment shader and enough information to
// install it into the hardware rendering pipeline including values for
// specific shader parameters and structures.
//
// The design of the material file closely follows that of vtk's xml
// descriptions of it's data sets. This allows use of the very handy
// vtkXMLDataElement which provides easy access to an xml element's
// attribute values. Inlined data is currently not handled.
//
// Ideally this class would be a Facade to a DOM parser, but VTK only
// provides access to expat, a SAX parser. Other vtk classes that parse
// xml files are tuned to read vtkDataSets and don't provide the functionality
// to handle generic xml data. As such they are of little use here.
//
// This class may be extended for better data  handling or may become a
// Facade to a DOM parser should on become part of the VTK code base.
// .SECTION Thanks
// Shader support in VTK includes key contributions by Gary Templet at
// Sandia National Labs.

#ifndef __vtkXMLMaterialParser_h
#define __vtkXMLMaterialParser_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkXMLParser.h"

class vtkXMLMaterial;
class vtkXMLMaterialParserInternals;

class VTKRENDERINGCORE_EXPORT vtkXMLMaterialParser : public vtkXMLParser
{
public:
  static vtkXMLMaterialParser *New();
  vtkTypeMacro(vtkXMLMaterialParser,vtkXMLParser);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the vtkXMLMaterial representation of the parsed material.
  vtkGetObjectMacro(Material, vtkXMLMaterial);
  void SetMaterial(vtkXMLMaterial*);

  // Description:
  // Overridden to initialize the internal structures before
  // the parsing begins.
  virtual int Parse();
  virtual int Parse(const char* inputString);
  virtual int Parse(const char* inputString, unsigned int length);

  // Description:
  // Overridden to clean up internal structures before the chunk-parsing
  // begins.
  virtual int InitializeParser();
protected:
  vtkXMLMaterialParser();
  ~vtkXMLMaterialParser();

  // Description:
  // Event for handling the start of an element
  virtual void StartElement(const char* name, const char** atts);

  // Description:
  // Event for handling the end of an element
  virtual void EndElement(const char*);

  // Description:
  // Handle character data, not yet implemented
  virtual void CharacterDataHandler( const char* data, int length );

  vtkXMLMaterial* Material;
  vtkXMLMaterialParserInternals* Internals;

private:
  vtkXMLMaterialParser(const vtkXMLMaterialParser&); // Not implemented
  void operator=(const vtkXMLMaterialParser&); // Not implemented
};
#endif
