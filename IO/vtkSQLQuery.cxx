/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSQLQuery.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
#include "vtkSQLQuery.h"

#include "vtkObjectFactory.h"
#include "vtkSQLDatabase.h"
#include "vtkVariantArray.h"

#include "vtksys/SystemTools.hxx"

vtkCxxRevisionMacro(vtkSQLQuery, "1.8");

vtkSQLQuery::vtkSQLQuery()
{
  this->Query = 0;
  this->Database = 0;
  this->Active = false;
}

vtkSQLQuery::~vtkSQLQuery()
{
  this->SetQuery(0);
  if (this->Database)
    {
    this->Database->Delete();
    this->Database = NULL;
    }
}

vtkCxxSetObjectMacro(vtkSQLQuery, Database, vtkSQLDatabase);

void vtkSQLQuery::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Query: " << (this->Query ? this->Query : "NULL") << endl;
  os << indent << "Database: " << (this->Database ? "" : "NULL") << endl;
  if (this->Database)
    {
    this->Database->PrintSelf(os, indent.GetNextIndent());
    }
}

vtkStdString vtkSQLQuery::EscapeString( vtkStdString s, bool addSurroundingQuotes )
{
  vtkStdString d;
  if ( addSurroundingQuotes )
    {
    d += '\'';
    }

  for ( vtkStdString::iterator it = s.begin(); it != s.end(); ++ it )
    {
    if ( *it == '\'' )
      d += '\''; // Single quotes are escaped by repeating them
    d += *it;
    }

  if ( addSurroundingQuotes )
    {
    d += '\'';
    }
  return d;
}

char* vtkSQLQuery::EscapeString( const char* src, bool addSurroundingQuotes )
{
  vtkStdString sstr( src );
  vtkStdString dstr = this->EscapeString( sstr, addSurroundingQuotes );
  return vtksys::SystemTools::DuplicateString( dstr.c_str() );
}

bool vtkSQLQuery::BindParameter(int vtkNotUsed(index), unsigned char vtkNotUsed(value))
{
  vtkErrorMacro(<<"This database driver does not support bound parameters.");
  return false;
}

bool vtkSQLQuery::BindParameter(int vtkNotUsed(index), signed char vtkNotUsed(value))
{
  vtkErrorMacro(<<"This database driver does not support bound parameters.");
  return false;
}


bool vtkSQLQuery::BindParameter(int vtkNotUsed(index), unsigned short vtkNotUsed(value))
{
  vtkErrorMacro(<<"This database driver does not support bound parameters.");
  return false;
}

bool vtkSQLQuery::BindParameter(int vtkNotUsed(index), short vtkNotUsed(value))
{
  vtkErrorMacro(<<"This database driver does not support bound parameters.");
  return false;
}

bool vtkSQLQuery::BindParameter(int vtkNotUsed(index), unsigned int vtkNotUsed(value))
{
  vtkErrorMacro(<<"This database driver does not support bound parameters.");
  return false;
}

bool vtkSQLQuery::BindParameter(int vtkNotUsed(index), int vtkNotUsed(value))
{
  vtkErrorMacro(<<"This database driver does not support bound parameters.");
  return false;
}

bool vtkSQLQuery::BindParameter(int vtkNotUsed(index), unsigned long vtkNotUsed(value))
{
  vtkErrorMacro(<<"This database driver does not support bound parameters.");
  return false;
}

bool vtkSQLQuery::BindParameter(int vtkNotUsed(index), long vtkNotUsed(value))
{
  vtkErrorMacro(<<"This database driver does not support bound parameters.");
  return false;
}

bool vtkSQLQuery::BindParameter(int vtkNotUsed(index), vtkTypeUInt64 vtkNotUsed(value))
{
  vtkErrorMacro(<<"This database driver does not support bound parameters.");
  return false;
}

bool vtkSQLQuery::BindParameter(int vtkNotUsed(index), vtkTypeInt64 vtkNotUsed(value))
{
  vtkErrorMacro(<<"This database driver does not support bound parameters.");
  return false;
}

bool vtkSQLQuery::BindParameter(int vtkNotUsed(index), float vtkNotUsed(value))
{
  vtkErrorMacro(<<"This database driver does not support bound parameters.");
  return false;
}

bool vtkSQLQuery::BindParameter(int vtkNotUsed(index), double vtkNotUsed(value))
{
  vtkErrorMacro(<<"This database driver does not support bound parameters.");
  return false;
}

bool vtkSQLQuery::BindParameter(int vtkNotUsed(index), const char *vtkNotUsed(value))
{
  vtkErrorMacro(<<"This database driver does not support bound parameters.");
  return false;
}

bool vtkSQLQuery::BindParameter(int vtkNotUsed(index), const char *vtkNotUsed(value), size_t vtkNotUsed(length))
{
  vtkErrorMacro(<<"This database driver does not support bound parameters.");
  return false;
}


bool vtkSQLQuery::BindParameter(int vtkNotUsed(index), const vtkStdString &vtkNotUsed(value))
{
  vtkErrorMacro(<<"This database driver does not support bound parameters.");
  return false;
}

bool vtkSQLQuery::BindParameter(int vtkNotUsed(index), const void *vtkNotUsed(value), size_t vtkNotUsed(length))
{
  vtkErrorMacro(<<"This database driver does not support bound parameters.");
  return false;
}

bool vtkSQLQuery::ClearParameterBindings()
{
  vtkErrorMacro(<<"This database driver does not support bound parameters.");
  return false;
}
