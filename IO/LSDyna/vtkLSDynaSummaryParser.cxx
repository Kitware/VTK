/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLSDynaSummaryParser.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkLSDynaSummaryParser.h"
#include "LSDynaMetaData.h"
#include "vtkObjectFactory.h"

#include <string>

vtkStandardNewMacro(vtkLSDynaSummaryParser);

namespace
{
static void vtkLSTrimWhitespace( std::string& line )
{
  std::string::size_type llen = line.length();
  while ( llen &&
    ( line[llen - 1] == ' ' ||
      line[llen - 1] == '\t' ||
      line[llen - 1] == '\r' ||
      line[llen - 1] == '\n' ) )
    {
    --llen;
    }

  std::string::size_type nameStart = 0;
  while ( nameStart < llen &&
    ( line[nameStart] == ' ' ||
      line[nameStart] == '\t' ) )
    {
    ++nameStart;
    }

  line = line.substr( nameStart, llen - nameStart );
}

}

//-----------------------------------------------------------------------------
vtkLSDynaSummaryParser::vtkLSDynaSummaryParser()
  :MetaData(NULL),
  PartId(-1),
  PartStatus(0),
  PartMaterial(0),
  InPart(0),
  InDyna(0),
  InName(0)
  {
  }

//-----------------------------------------------------------------------------
void vtkLSDynaSummaryParser::StartElement(const char* name, const char** atts)
{
  int i;
  if ( ! strcmp( name, "part" ) )
    {
    if ( ! this->InDyna || this->InPart )
      { // can't have loner parts or parts that contain parts
      this->ReportUnknownElement( name );
      }
    else
      {
      this->InPart = 1;
      this->PartName = "";

      this->PartId = -1;
      this->PartStatus = 1;
      this->PartMaterial = -1;
      for ( i = 0; atts[i] != 0; i += 2 )
        {
        if ( ! strcmp( atts[i], "id" ) )
          {
          if ( sscanf( atts[i+1], "%d", &this->PartId ) <= 0 )
            {
            this->PartId = -1;
            this->ReportBadAttribute( name, atts[i], atts[i+1] );
            }
          }
        else if ( ! strcmp( atts[i], "material" ) )
          {
          if ( sscanf( atts[i+1], "%d", &this->PartMaterial ) <= 0 )
            {
            this->PartMaterial = -1;
            this->ReportBadAttribute( name, atts[i], atts[i+1] );
            }
          }
        else if ( ! strcmp( atts[i], "status" ) )
          {
          if ( sscanf( atts[i+1], "%d", &this->PartStatus ) <= 0 )
            {
            this->PartStatus = 1;
            this->ReportBadAttribute( name, atts[i], atts[i+1] );
            }
          }
        }
      if ( this->PartId < 0 )
        {
        this->ReportMissingAttribute( name, "id" );
        }
      }
    }
  else if ( ! strcmp( name, "name" ) )
    {
    if ( ! this->InDyna || ! this->InPart )
      { // name must be inside a part
      this->ReportUnknownElement( name );
      }
    else
      {
      this->InName = 1;
      this->PartName = "";
      }
    }
  else if ( ! strcmp( name, "database" ) )
    { // database must be inside the lsdyna tag, but not inside a part or name
    if ( ! this->InDyna || this->InPart || this->InName )
      {
      this->ReportUnknownElement( name );
      }
    else
      {
      const char* dbpath = 0;
      const char* dbname = 0;
      for ( i = 0; atts[i] != 0; i += 2 )
        {
        if ( ! strcmp( atts[i], "path" ) )
          {
          dbpath = atts[i+1];
          }
        else if ( ! strcmp( atts[i], "name" ) )
          {
          dbname = atts[i+1];
          }
        }
      if ( dbpath && dbname )
        {
        this->MetaData->Fam.SetDatabaseDirectory( dbpath );
        this->MetaData->Fam.SetDatabaseBaseName( dbname );
        }
      else
        {
        this->ReportXmlParseError();
        }
      }
    }
  else if ( ! strcmp( name, "lsdyna" ) )
    {
    if ( this->InPart || this->InName || this->InDyna )
      { // dyna must be outermost tag
      this->ReportUnknownElement( name );
      }
    else
      {
      this->InDyna = 1;
      }
    }
}

//-----------------------------------------------------------------------------
void vtkLSDynaSummaryParser::EndElement(const char* name)
{
  if ( ! strcmp( name, "part" ) )
    {
    this->InPart = this->InName = 0;
    if ( this->PartName.empty() || this->PartId <= 0 ||
         this->PartId > (int) this->MetaData->PartNames.size() )
      { // missing a name or an id
      this->ReportXmlParseError();
      }
    else
      {
      vtkLSTrimWhitespace( this->PartName );
      this->MetaData->PartNames[this->PartId - 1] = this->PartName;
      this->MetaData->PartIds[this->PartId - 1] = this->PartId;
      this->MetaData->PartMaterials[this->PartId - 1] = this->PartMaterial;
      this->MetaData->PartStatus[this->PartId - 1] = this->PartStatus;
      }
    }
  else if ( ! strcmp( name, "name" ) )
    {
    this->InName = 0;
    }
  else if ( ! strcmp( name, "lsdyna" ) )
    {
    this->InDyna = this->InPart = this->InName = 0;
    }
}
//-----------------------------------------------------------------------------
void vtkLSDynaSummaryParser::CharacterDataHandler(const char* data, int length)
{
  if ( ! this->InName )
    {
    return;
    }
  // skip leading whitespace
  int i = 0;
  while ( this->PartName.empty() && i < length && this->IsSpace( data[i] ) )
    ++i;

  if ( i < length )
    this->PartName.append( data + i, length - i );
}

//-----------------------------------------------------------------------------
void vtkLSDynaSummaryParser::PrintSelf( ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );

  os << indent << "PartName: \"" << this->PartName << "\"" << endl;
  os << indent << "PartId: " << this->PartId << endl;
  os << indent << "PartStatus: " << this->PartStatus << endl;
  os << indent << "PartMaterial: " << this->PartMaterial << endl;
  os << indent << "InPart: " << this->InPart << endl;
  os << indent << "InDyna: " << this->InDyna << endl;
  os << indent << "InName: " << this->InName << endl;
}
