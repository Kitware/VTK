/*=========================================================================

  Program:   Visualization Toolkit
  Module:    DatabaseSchemaWith2Tables.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
// .SECTION Thanks
// Thanks to Philippe Pebay from Sandia National Laboratories for implementing
// this example of a database schema.

#include "DatabaseSchemaWith2Tables.h"

#include <stdexcept>

#include "vtkSQLDatabaseSchema.h"

DatabaseSchemaWith2Tables::DatabaseSchemaWith2Tables()
{
  this->Create();
}

DatabaseSchemaWith2Tables::~DatabaseSchemaWith2Tables()
{

  if(this->Schema)
  {
    this->Schema->Delete();
  }
}

void DatabaseSchemaWith2Tables::Create()
{
  cerr << "@@ Creating a schema...";

  this->Schema = vtkSQLDatabaseSchema::New();
  this->Schema->SetName( "TestSchema" );

  // Create PostgreSQL-specific preambles to load the PL/PGSQL language and create a function
  // with this language. These will be ignored by other backends.
  this->Schema->AddPreamble( "dropplpgsql", "DROP EXTENSION IF EXISTS PLPGSQL", VTK_SQL_POSTGRESQL );
  this->Schema->AddPreamble( "loadplpgsql", "CREATE LANGUAGE PLPGSQL", VTK_SQL_POSTGRESQL );
  this->Schema->AddPreamble( "createsomefunction",
    "CREATE OR REPLACE FUNCTION somefunction() RETURNS TRIGGER AS $btable$ "
    "BEGIN "
    "INSERT INTO btable (somevalue) VALUES (NEW.somenmbr); "
    "RETURN NEW; "
    "END; $btable$ LANGUAGE PLPGSQL",
     VTK_SQL_POSTGRESQL );

  // Insert in alphabetical order so that SHOW TABLES does not mix handles
  this->TableAHandle = this->Schema->AddTableMultipleArguments( "atable",
    vtkSQLDatabaseSchema::COLUMN_TOKEN, vtkSQLDatabaseSchema::SERIAL,  "tablekey",  0, "",
    vtkSQLDatabaseSchema::COLUMN_TOKEN, vtkSQLDatabaseSchema::VARCHAR, "somename", 64, "NOT NULL",
    vtkSQLDatabaseSchema::COLUMN_TOKEN, vtkSQLDatabaseSchema::BIGINT,  "somenmbr", 17, "DEFAULT 0",
    vtkSQLDatabaseSchema::INDEX_TOKEN,  vtkSQLDatabaseSchema::PRIMARY_KEY, "bigkey",
    vtkSQLDatabaseSchema::INDEX_COLUMN_TOKEN, "tablekey",
    vtkSQLDatabaseSchema::END_INDEX_TOKEN,
    vtkSQLDatabaseSchema::INDEX_TOKEN,  vtkSQLDatabaseSchema::UNIQUE, "reverselookup",
    vtkSQLDatabaseSchema::INDEX_COLUMN_TOKEN, "somename",
    vtkSQLDatabaseSchema::INDEX_COLUMN_TOKEN, "somenmbr",
    vtkSQLDatabaseSchema::END_INDEX_TOKEN,
    vtkSQLDatabaseSchema::TRIGGER_TOKEN,  vtkSQLDatabaseSchema::AFTER_INSERT,
      "inserttrigger", "DO NOTHING",
      VTK_SQL_SQLITE,
    vtkSQLDatabaseSchema::TRIGGER_TOKEN,  vtkSQLDatabaseSchema::AFTER_INSERT,
      "inserttrigger", "FOR EACH ROW EXECUTE PROCEDURE somefunction ()",
      VTK_SQL_POSTGRESQL,
    vtkSQLDatabaseSchema::TRIGGER_TOKEN,  vtkSQLDatabaseSchema::AFTER_INSERT,
      "inserttrigger", "FOR EACH ROW INSERT INTO btable SET somevalue = NEW.somenmbr",
      VTK_SQL_MYSQL,
    vtkSQLDatabaseSchema::END_TABLE_TOKEN
  );

  if(this->TableAHandle < 0 )
  {
   throw std::runtime_error("Could not create test schema: Failed to create atable");
  }

  this->TableBHandle = this->Schema->AddTableMultipleArguments( "btable",
    vtkSQLDatabaseSchema::COLUMN_TOKEN, vtkSQLDatabaseSchema::SERIAL,  "tablekey",  0, "",
    vtkSQLDatabaseSchema::COLUMN_TOKEN, vtkSQLDatabaseSchema::BIGINT,  "somevalue", 12, "DEFAULT 0",
    vtkSQLDatabaseSchema::INDEX_TOKEN,  vtkSQLDatabaseSchema::PRIMARY_KEY, "",
    vtkSQLDatabaseSchema::INDEX_COLUMN_TOKEN, "tablekey",
    vtkSQLDatabaseSchema::END_INDEX_TOKEN,
    vtkSQLDatabaseSchema::END_TABLE_TOKEN
  );

  if ( this->TableBHandle < 0 )
  {
    throw std::runtime_error("Could not create test schema: Failed to create btable");
  }
  cerr << " done." << endl;

}
