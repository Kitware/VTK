/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLMaterialReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLMaterialReader.h"

#include "vtkObjectFactory.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLMaterial.h"
#include "vtkXMLMaterialParser.h"

vtkStandardNewMacro( vtkXMLMaterialReader );
//-----------------------------------------------------------------------------
vtkXMLMaterialReader::vtkXMLMaterialReader()
  :
  FileName(NULL),
  XMLParser(NULL)
{
  this->CreateXMLParser();
}

//-----------------------------------------------------------------------------
vtkXMLMaterialReader::~vtkXMLMaterialReader()
{
  this->SetFileName(NULL);
  this->DestroyXMLParser();
}

//-----------------------------------------------------------------------------
void vtkXMLMaterialReader::CreateXMLParser()
{
  if( this->XMLParser)
    {
    vtkErrorMacro("vtkXMLMaterialReader::CreateXMLParser() called with \
      an existent XMLParser.");
    this->DestroyXMLParser();
    }
  this->XMLParser = vtkXMLMaterialParser::New();
}

//-----------------------------------------------------------------------------
void vtkXMLMaterialReader::DestroyXMLParser()
{
  if(!this->XMLParser)
    {
    vtkErrorMacro("DestroyXMLParser() called with no current XMLParser.");
    return;
    }
  this->XMLParser->Delete();
  this->XMLParser = 0;
}

//-----------------------------------------------------------------------------
vtkXMLMaterial* vtkXMLMaterialReader::GetMaterial()
{
  if (this->XMLParser)
    {
    return this->XMLParser->GetMaterial();
    }
  return 0;
}

//-----------------------------------------------------------------------------
void vtkXMLMaterialReader::ReadMaterial()
{
  if( this->GetFileName() == NULL )
    {
    vtkErrorMacro( "No material file specified in vtkXMLMaterialReader." );
    }

  if (this->ParseTime < this->MTime )
    {
    if( this->XMLParser )
      {
      this->XMLParser->SetFileName( this->GetFileName() );
      this->XMLParser->Parse();
      this->ParseTime.Modified();
      }
    else
      {
      vtkErrorMacro( "Cannot read the material file without a Parser." );
      }
    }
}

//-----------------------------------------------------------------------------
void vtkXMLMaterialReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "XMLParser: " ;
  if (this->XMLParser)
    {
    os << endl;
    this->XMLParser->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
  os << indent << "FileName: " <<
    (this->FileName? this->FileName : "(null)") << endl;
}
