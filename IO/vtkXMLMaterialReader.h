/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLMaterialReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLMaterialReader - Provide access to elements in Material files
//
// .SECTION Description
// vtkXMLMaterialReader provides access to three types of vtkXMLDataElement
// found in XML Material Files. This class sorts them by type and integer
// id from 0-N for N elements of a specific type starting with the first
// instance found.
//
// .SECTION Design
// This class is basically a Facade for vtkXMLMaterialParser. Currently
// functionality is to only provide access to vtkXMLDataElements but further
// extensions may return higher level data structures.
//
// Having both an vtkXMLMaterialParser and a vtkXMLMaterialReader is consistent with
// VTK's design for handling xml file and provides for future flexibility, that is
// better data handlers and interfacing with a DOM xml parser.
//
// vtkProperty - defines values for some or all data members of vtkProperty
//
// vtkVertexShader - defines vertex shaders
//
// vtkFragmentShader - defines fragment shaders
// .SECTION Thanks
// Shader support in VTK includes key contributions by Gary Templet at 
// Sandia National Labs.

#ifndef __vtkXMLMaterialReader_h
#define __vtkXMLMaterialReader_h

#include "vtkObject.h"

class vtkXMLDataElement;
class vtkXMLMaterial;
class vtkXMLMaterialParser;

class VTK_IO_EXPORT vtkXMLMaterialReader : public vtkObject
{
public:
  vtkTypeMacro(vtkXMLMaterialReader,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkXMLMaterialReader* New();

  // Description:
  // Set and get file name.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Read the material file refered to in FileName.
  // If the Reader hasn't changed since the last ReadMaterial(),
  // it does not read the file again.
  void ReadMaterial();

  // Description:
  // Get the Material representation read by the reader.
  vtkXMLMaterial* GetMaterial();
protected:
  vtkXMLMaterialReader();
  ~vtkXMLMaterialReader();

  // Description:
  // Create and vtkXMLParser to read the file
  virtual void CreateXMLParser();

  // Description:
  // Destroys the vtkXMLParser.
  virtual void DestroyXMLParser();

  char* FileName;
  vtkXMLMaterialParser* XMLParser;
  vtkTimeStamp ParseTime;

private:
  vtkXMLMaterialReader(const vtkXMLMaterialReader&);  // Not implemented.
  void operator=(const vtkXMLMaterialReader&);  // Not implemented.
};
#endif
