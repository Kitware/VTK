// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkSQLDatabaseSchema.h"

#include "vtkObjectFactory.h"

#include <cstdarg> // va_list

#include <vector>

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkSQLDatabaseSchema);

//------------------------------------------------------------------------------
class vtkSQLDatabaseSchemaInternals
{
public: // NB: use of string instead of char* here to avoid leaks on destruction.
  struct Statement
  {
    std::string Name;
    std::string Action;  // may have backend-specific stuff
    std::string Backend; // only active for this backend, if != ""
  };

  struct Column
  {
    vtkSQLDatabaseSchema::DatabaseColumnType Type;
    int Size; // used when required, ignored otherwise (e.g. varchar)
    std::string Name;
    std::string Attributes; // may have backend-specific stuff
  };

  struct Index
  {
    vtkSQLDatabaseSchema::DatabaseIndexType Type;
    std::string Name;
    std::vector<std::string> ColumnNames;
  };

  struct Trigger
  {
    vtkSQLDatabaseSchema::DatabaseTriggerType Type;
    std::string Name;
    std::string Action;  // may have backend-specific stuff
    std::string Backend; // only active for this backend, if != ""
  };

  struct Option
  {
    std::string Text;
    std::string Backend;
  };

  struct Table
  {
    std::string Name;
    std::vector<Column> Columns;
    std::vector<Index> Indices;
    std::vector<Trigger> Triggers;
    std::vector<Option> Options;
  };

  std::vector<Statement> Preambles;
  std::vector<Table> Tables;
};

//------------------------------------------------------------------------------
vtkSQLDatabaseSchema::vtkSQLDatabaseSchema()
{
  this->Name = nullptr;
  this->Internals = new vtkSQLDatabaseSchemaInternals;
}

//------------------------------------------------------------------------------
vtkSQLDatabaseSchema::~vtkSQLDatabaseSchema()
{
  this->SetName(nullptr);
  delete this->Internals;
}

//------------------------------------------------------------------------------
void vtkSQLDatabaseSchema::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Name: ";
  if (this->Name)
  {
    os << this->Name << "\n";
  }
  else
  {
    os << "(null)"
       << "\n";
  }
  os << indent << "Internals: " << this->Internals << "\n";
}

//------------------------------------------------------------------------------
int vtkSQLDatabaseSchema::AddPreamble(
  const char* preName, const char* preAction, const char* preBackend)
{
  if (!preName)
  {
    vtkErrorMacro("Cannot add preamble with empty name");
    return -1;
  }

  vtkSQLDatabaseSchemaInternals::Statement newPre;
  int preHandle = static_cast<int>(this->Internals->Preambles.size());
  newPre.Name = preName;
  newPre.Action = preAction;
  newPre.Backend = preBackend;
  this->Internals->Preambles.push_back(newPre);
  return preHandle;
}

//------------------------------------------------------------------------------
int vtkSQLDatabaseSchema::AddTable(const char* tblName)
{
  if (!tblName)
  {
    vtkErrorMacro("Cannot add table with empty name");
    return -1;
  }

  vtkSQLDatabaseSchemaInternals::Table newTbl;
  int tblHandle = static_cast<int>(this->Internals->Tables.size());
  newTbl.Name = tblName;
  this->Internals->Tables.push_back(newTbl);
  return tblHandle;
}

//------------------------------------------------------------------------------
int vtkSQLDatabaseSchema::AddColumnToIndex(int tblHandle, int idxHandle, int colHandle)
{
  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    vtkErrorMacro("Cannot add column to index of non-existent table " << tblHandle);
    return -1;
  }

  vtkSQLDatabaseSchemaInternals::Table* table = &this->Internals->Tables[tblHandle];
  if (colHandle < 0 || colHandle >= static_cast<int>(table->Columns.size()))
  {
    vtkErrorMacro("Cannot add non-existent column " << colHandle << " in table " << tblHandle);
    return -1;
  }

  if (idxHandle < 0 || idxHandle >= static_cast<int>(table->Indices.size()))
  {
    vtkErrorMacro(
      "Cannot add column to non-existent index " << idxHandle << " of table " << tblHandle);
    return -1;
  }

  table->Indices[idxHandle].ColumnNames.push_back(table->Columns[colHandle].Name);
  return static_cast<int>(table->Indices[idxHandle].ColumnNames.size() - 1);
}

//------------------------------------------------------------------------------
int vtkSQLDatabaseSchema::AddColumnToTable(
  int tblHandle, int colType, const char* colName, int colSize, const char* colOpts)
{
  if (!colName)
  {
    vtkErrorMacro("Cannot add column with empty name to table " << tblHandle);
    return -1;
  }

  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    vtkErrorMacro("Cannot add column to non-existent table " << tblHandle);
    return -1;
  }

  // DCT: This trick avoids copying a Column structure the way push_back would:
  int colHandle = static_cast<int>(this->Internals->Tables[tblHandle].Columns.size());
  this->Internals->Tables[tblHandle].Columns.resize(colHandle + 1);
  vtkSQLDatabaseSchemaInternals::Column* column =
    &this->Internals->Tables[tblHandle].Columns[colHandle];
  column->Type = static_cast<DatabaseColumnType>(colType);
  column->Size = colSize;
  column->Name = colName;
  column->Attributes = colOpts;
  return colHandle;
}

//------------------------------------------------------------------------------
int vtkSQLDatabaseSchema::AddIndexToTable(int tblHandle, int idxType, const char* idxName)
{
  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    vtkErrorMacro("Cannot add index to non-existent table " << tblHandle);
    return -1;
  }

  int idxHandle = static_cast<int>(this->Internals->Tables[tblHandle].Indices.size());
  this->Internals->Tables[tblHandle].Indices.resize(idxHandle + 1);
  vtkSQLDatabaseSchemaInternals::Index* index =
    &this->Internals->Tables[tblHandle].Indices[idxHandle];
  index->Type = static_cast<DatabaseIndexType>(idxType);
  index->Name = idxName;
  return idxHandle;
}

//------------------------------------------------------------------------------
int vtkSQLDatabaseSchema::AddTriggerToTable(
  int tblHandle, int trgType, const char* trgName, const char* trgAction, const char* trgBackend)
{
  if (!trgName)
  {
    vtkErrorMacro("Cannot add trigger with empty name to table " << tblHandle);
    return -1;
  }

  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    vtkErrorMacro("Cannot add trigger to non-existent table " << tblHandle);
    return -1;
  }

  int trgHandle = static_cast<int>(this->Internals->Tables[tblHandle].Triggers.size());
  this->Internals->Tables[tblHandle].Triggers.resize(trgHandle + 1);
  vtkSQLDatabaseSchemaInternals::Trigger* trigger =
    &this->Internals->Tables[tblHandle].Triggers[trgHandle];
  trigger->Type = static_cast<DatabaseTriggerType>(trgType);
  trigger->Name = trgName;
  trigger->Action = trgAction;
  trigger->Backend = trgBackend;
  return trgHandle;
}

//------------------------------------------------------------------------------
int vtkSQLDatabaseSchema::AddOptionToTable(
  int tblHandle, const char* optText, const char* optBackend)
{
  if (!optText)
  {
    vtkErrorMacro("Cannot add nullptr option to table " << tblHandle);
    return -1;
  }

  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    vtkErrorMacro("Cannot add option to non-existent table " << tblHandle);
    return -1;
  }

  int optHandle = static_cast<int>(this->Internals->Tables[tblHandle].Options.size());
  this->Internals->Tables[tblHandle].Options.resize(optHandle + 1);
  vtkSQLDatabaseSchemaInternals::Option* optn =
    &this->Internals->Tables[tblHandle].Options[optHandle];
  optn->Text = optText;
  optn->Backend = optBackend ? optBackend : VTK_SQL_ALLBACKENDS;
  return optHandle;
}

//------------------------------------------------------------------------------
int vtkSQLDatabaseSchema::GetPreambleHandleFromName(const char* preName)
{
  int i;
  int ntab = static_cast<int>(this->Internals->Preambles.size());
  std::string preNameStr(preName);
  for (i = 0; i < ntab; ++i)
  {
    if (this->Internals->Preambles[i].Name == preNameStr)
    {
      return i;
    }
  }
  return -1;
}

//------------------------------------------------------------------------------
const char* vtkSQLDatabaseSchema::GetPreambleNameFromHandle(int preHandle)
{
  if (preHandle < 0 || preHandle >= this->GetNumberOfPreambles())
  {
    vtkErrorMacro("Cannot get name of non-existent preamble " << preHandle);
    return nullptr;
  }

  return this->Internals->Preambles[preHandle].Name.c_str();
}

//------------------------------------------------------------------------------
const char* vtkSQLDatabaseSchema::GetPreambleActionFromHandle(int preHandle)
{
  if (preHandle < 0 || preHandle >= this->GetNumberOfPreambles())
  {
    vtkErrorMacro("Cannot get action of non-existent preamble " << preHandle);
    return nullptr;
  }

  return this->Internals->Preambles[preHandle].Action.c_str();
}

//------------------------------------------------------------------------------
const char* vtkSQLDatabaseSchema::GetPreambleBackendFromHandle(int preHandle)
{
  if (preHandle < 0 || preHandle >= this->GetNumberOfPreambles())
  {
    vtkErrorMacro("Cannot get backend of non-existent preamble " << preHandle);
    return nullptr;
  }

  return this->Internals->Preambles[preHandle].Backend.c_str();
}

//------------------------------------------------------------------------------
int vtkSQLDatabaseSchema::GetTableHandleFromName(const char* tblName)
{
  int i;
  int ntab = static_cast<int>(this->Internals->Tables.size());
  std::string tblNameStr(tblName);
  for (i = 0; i < ntab; ++i)
  {
    if (this->Internals->Tables[i].Name == tblNameStr)
    {
      return i;
    }
  }
  return -1;
}

//------------------------------------------------------------------------------
const char* vtkSQLDatabaseSchema::GetTableNameFromHandle(int tblHandle)
{
  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    vtkErrorMacro("Cannot get name of non-existent table " << tblHandle);
    return nullptr;
  }

  return this->Internals->Tables[tblHandle].Name.c_str();
}

//------------------------------------------------------------------------------
int vtkSQLDatabaseSchema::GetIndexHandleFromName(const char* tblName, const char* idxName)
{
  int tblHandle = this->GetTableHandleFromName(tblName);
  if (tblHandle < 0)
  {
    return -1;
  }

  int i;
  int nidx = static_cast<int>(this->Internals->Tables[tblHandle].Indices.size());
  std::string idxNameStr(idxName);
  for (i = 0; i < nidx; ++i)
  {
    if (this->Internals->Tables[tblHandle].Indices[i].Name == idxNameStr)
    {
      return i;
    }
  }
  return -1;
}

//------------------------------------------------------------------------------
const char* vtkSQLDatabaseSchema::GetIndexNameFromHandle(int tblHandle, int idxHandle)
{
  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    vtkErrorMacro("Cannot get name of an index in non-existent table " << tblHandle);
    return nullptr;
  }

  if (idxHandle < 0 ||
    idxHandle >= static_cast<int>(this->Internals->Tables[tblHandle].Indices.size()))
  {
    vtkErrorMacro(
      "Cannot get name of non-existent index " << idxHandle << " in table " << tblHandle);
    return nullptr;
  }

  return this->Internals->Tables[tblHandle].Indices[idxHandle].Name.c_str();
}

//------------------------------------------------------------------------------
int vtkSQLDatabaseSchema::GetIndexTypeFromHandle(int tblHandle, int idxHandle)
{
  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    vtkErrorMacro("Cannot get type of an index in non-existent table " << tblHandle);
    return -1;
  }

  if (idxHandle < 0 ||
    idxHandle >= static_cast<int>(this->Internals->Tables[tblHandle].Indices.size()))
  {
    vtkErrorMacro(
      "Cannot get type of non-existent index " << idxHandle << " in table " << tblHandle);
    return -1;
  }

  return static_cast<int>(this->Internals->Tables[tblHandle].Indices[idxHandle].Type);
}

//------------------------------------------------------------------------------
const char* vtkSQLDatabaseSchema::GetIndexColumnNameFromHandle(
  int tblHandle, int idxHandle, int cnmHandle)
{
  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    vtkErrorMacro("Cannot get column name of an index in non-existent table " << tblHandle);
    return nullptr;
  }

  if (idxHandle < 0 ||
    idxHandle >= static_cast<int>(this->Internals->Tables[tblHandle].Indices.size()))
  {
    vtkErrorMacro(
      "Cannot get column name of non-existent index " << idxHandle << " in table " << tblHandle);
    return nullptr;
  }

  if (cnmHandle < 0 ||
    cnmHandle >=
      static_cast<int>(this->Internals->Tables[tblHandle].Indices[idxHandle].ColumnNames.size()))
  {
    vtkErrorMacro("Cannot get column name of non-existent column "
      << cnmHandle << " of index " << idxHandle << " in table " << tblHandle);
    return nullptr;
  }

  return this->Internals->Tables[tblHandle].Indices[idxHandle].ColumnNames[cnmHandle].c_str();
}

//------------------------------------------------------------------------------
int vtkSQLDatabaseSchema::GetColumnHandleFromName(const char* tblName, const char* colName)
{
  int tblHandle = this->GetTableHandleFromName(tblName);
  if (tblHandle < 0)
  {
    return -1;
  }

  int i;
  int ncol = static_cast<int>(this->Internals->Tables[tblHandle].Columns.size());
  std::string colNameStr(colName);
  for (i = 0; i < ncol; ++i)
  {
    if (this->Internals->Tables[tblHandle].Columns[i].Name == colNameStr)
    {
      return i;
    }
  }
  return -1;
}

//------------------------------------------------------------------------------
const char* vtkSQLDatabaseSchema::GetColumnNameFromHandle(int tblHandle, int colHandle)
{
  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    vtkErrorMacro("Cannot get name of a column in non-existent table " << tblHandle);
    return nullptr;
  }

  if (colHandle < 0 ||
    colHandle >= static_cast<int>(this->Internals->Tables[tblHandle].Columns.size()))
  {
    vtkErrorMacro(
      "Cannot get name of non-existent column " << colHandle << " in table " << tblHandle);
    return nullptr;
  }

  return this->Internals->Tables[tblHandle].Columns[colHandle].Name.c_str();
}

//------------------------------------------------------------------------------
int vtkSQLDatabaseSchema::GetColumnTypeFromHandle(int tblHandle, int colHandle)
{
  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    vtkErrorMacro("Cannot get type of a column in non-existent table " << tblHandle);
    return -1;
  }

  if (colHandle < 0 ||
    colHandle >= static_cast<int>(this->Internals->Tables[tblHandle].Columns.size()))
  {
    vtkErrorMacro(
      "Cannot get type of non-existent column " << colHandle << " in table " << tblHandle);
    return -1;
  }

  return static_cast<int>(this->Internals->Tables[tblHandle].Columns[colHandle].Type);
}

//------------------------------------------------------------------------------
int vtkSQLDatabaseSchema::GetColumnSizeFromHandle(int tblHandle, int colHandle)
{
  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    vtkErrorMacro("Cannot get size of a column in non-existent table " << tblHandle);
    return -1;
  }

  if (colHandle < 0 ||
    colHandle >= static_cast<int>(this->Internals->Tables[tblHandle].Columns.size()))
  {
    vtkErrorMacro(
      "Cannot get size of non-existent column " << colHandle << " in table " << tblHandle);
    return -1;
  }

  return this->Internals->Tables[tblHandle].Columns[colHandle].Size;
}

//------------------------------------------------------------------------------
const char* vtkSQLDatabaseSchema::GetColumnAttributesFromHandle(int tblHandle, int colHandle)
{
  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    vtkErrorMacro("Cannot get attributes of a column in non-existent table " << tblHandle);
    return nullptr;
  }

  if (colHandle < 0 ||
    colHandle >= static_cast<int>(this->Internals->Tables[tblHandle].Columns.size()))
  {
    vtkErrorMacro(
      "Cannot get attributes of non-existent column " << colHandle << " in table " << tblHandle);
    return nullptr;
  }

  return this->Internals->Tables[tblHandle].Columns[colHandle].Attributes.c_str();
}

//------------------------------------------------------------------------------
int vtkSQLDatabaseSchema::GetTriggerHandleFromName(const char* tblName, const char* trgName)
{
  int tblHandle = this->GetTableHandleFromName(tblName);
  if (tblHandle < 0)
  {
    return -1;
  }

  int i;
  int ntrg = static_cast<int>(this->Internals->Tables[tblHandle].Triggers.size());
  std::string trgNameStr(trgName);
  for (i = 0; i < ntrg; ++i)
  {
    if (this->Internals->Tables[tblHandle].Triggers[i].Name == trgNameStr)
    {
      return i;
    }
  }
  return -1;
}

//------------------------------------------------------------------------------
const char* vtkSQLDatabaseSchema::GetTriggerNameFromHandle(int tblHandle, int trgHandle)
{
  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    vtkErrorMacro("Cannot get name of a trigger in non-existent table " << tblHandle);
    return nullptr;
  }

  if (trgHandle < 0 ||
    trgHandle >= static_cast<int>(this->Internals->Tables[tblHandle].Triggers.size()))
  {
    vtkErrorMacro(
      "Cannot get name of non-existent trigger " << trgHandle << " in table " << tblHandle);
    return nullptr;
  }

  return this->Internals->Tables[tblHandle].Triggers[trgHandle].Name.c_str();
}

//------------------------------------------------------------------------------
int vtkSQLDatabaseSchema::GetTriggerTypeFromHandle(int tblHandle, int trgHandle)
{
  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    vtkErrorMacro("Cannot get type of a trigger in non-existent table " << tblHandle);
    return -1;
  }

  if (trgHandle < 0 ||
    trgHandle >= static_cast<int>(this->Internals->Tables[tblHandle].Triggers.size()))
  {
    vtkErrorMacro(
      "Cannot get type of non-existent trigger " << trgHandle << " in table " << tblHandle);
    return -1;
  }

  return this->Internals->Tables[tblHandle].Triggers[trgHandle].Type;
}

//------------------------------------------------------------------------------
const char* vtkSQLDatabaseSchema::GetTriggerActionFromHandle(int tblHandle, int trgHandle)
{
  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    vtkErrorMacro("Cannot get action of a trigger in non-existent table " << tblHandle);
    return nullptr;
  }

  if (trgHandle < 0 ||
    trgHandle >= static_cast<int>(this->Internals->Tables[tblHandle].Triggers.size()))
  {
    vtkErrorMacro(
      "Cannot get action of non-existent trigger " << trgHandle << " in table " << tblHandle);
    return nullptr;
  }

  return this->Internals->Tables[tblHandle].Triggers[trgHandle].Action.c_str();
}

//------------------------------------------------------------------------------
const char* vtkSQLDatabaseSchema::GetTriggerBackendFromHandle(int tblHandle, int trgHandle)
{
  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    vtkErrorMacro("Cannot get backend of a trigger in non-existent table " << tblHandle);
    return nullptr;
  }

  if (trgHandle < 0 ||
    trgHandle >= static_cast<int>(this->Internals->Tables[tblHandle].Triggers.size()))
  {
    vtkErrorMacro(
      "Cannot get backend of non-existent trigger " << trgHandle << " in table " << tblHandle);
    return nullptr;
  }

  return this->Internals->Tables[tblHandle].Triggers[trgHandle].Backend.c_str();
}

//------------------------------------------------------------------------------
const char* vtkSQLDatabaseSchema::GetOptionTextFromHandle(int tblHandle, int optHandle)
{
  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    vtkErrorMacro("Cannot get text of an option in non-existent table " << tblHandle);
    return nullptr;
  }

  if (optHandle < 0 ||
    optHandle >= static_cast<int>(this->Internals->Tables[tblHandle].Options.size()))
  {
    vtkErrorMacro(
      "Cannot get text of non-existent option " << optHandle << " in table " << tblHandle);
    return nullptr;
  }

  return this->Internals->Tables[tblHandle].Options[optHandle].Text.c_str();
}

//------------------------------------------------------------------------------
const char* vtkSQLDatabaseSchema::GetOptionBackendFromHandle(int tblHandle, int optHandle)
{
  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    vtkErrorMacro("Cannot get backend of an option in non-existent table " << tblHandle);
    return nullptr;
  }

  if (optHandle < 0 ||
    optHandle >= static_cast<int>(this->Internals->Tables[tblHandle].Options.size()))
  {
    vtkErrorMacro(
      "Cannot get backend of non-existent option " << optHandle << " in table " << tblHandle);
    return nullptr;
  }

  return this->Internals->Tables[tblHandle].Options[optHandle].Backend.c_str();
}

//------------------------------------------------------------------------------
int vtkSQLDatabaseSchema::AddTableMultipleArguments(const char* tblName, ...)
{
  int tblHandle = this->AddTable(tblName);
  int token;
  int dtyp;
  int size;
  int curIndexHandle;
  const char* name;
  const char* attr;
  const char* bcke;

  va_list args;
  va_start(args, tblName);
  while ((token = va_arg(args, int)) != END_TABLE_TOKEN)
  {
    switch (token)
    {
      case COLUMN_TOKEN:
        dtyp = va_arg(args, int);
        name = va_arg(args, const char*);
        size = va_arg(args, int);
        attr = va_arg(args, const char*);
        this->AddColumnToTable(tblHandle, dtyp, name, size, attr);
        break;
      case INDEX_TOKEN:
        dtyp = va_arg(args, int);
        name = va_arg(args, const char*);
        curIndexHandle = this->AddIndexToTable(tblHandle, dtyp, name);
        while ((token = va_arg(args, int)) != END_INDEX_TOKEN)
        {
          name = va_arg(args, const char*);
          dtyp = this->GetColumnHandleFromName(tblName, name);
          this->AddColumnToIndex(tblHandle, curIndexHandle, dtyp);
        }
        break;
      case TRIGGER_TOKEN:
        dtyp = va_arg(args, int);
        name = va_arg(args, const char*);
        attr = va_arg(args, const char*);
        bcke = va_arg(args, const char*);
        this->AddTriggerToTable(tblHandle, dtyp, name, attr, bcke);
        break;
      case OPTION_TOKEN:
        attr = va_arg(args, const char*);
        bcke = va_arg(args, const char*);
        this->AddOptionToTable(tblHandle, attr, bcke);
        break;
      default:
      {
        vtkErrorMacro("Bad token " << token << " passed to AddTable");
        va_end(args);
        return -1;
      }
    }
  }
  va_end(args);
  return tblHandle;
}

//------------------------------------------------------------------------------
void vtkSQLDatabaseSchema::Reset()
{
  this->Internals->Tables.clear();
}

//------------------------------------------------------------------------------
int vtkSQLDatabaseSchema::GetNumberOfPreambles()
{
  return static_cast<int>(this->Internals->Preambles.size());
}

//------------------------------------------------------------------------------
int vtkSQLDatabaseSchema::GetNumberOfTables()
{
  return static_cast<int>(this->Internals->Tables.size());
}

//------------------------------------------------------------------------------
int vtkSQLDatabaseSchema::GetNumberOfColumnsInTable(int tblHandle)
{
  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    vtkErrorMacro("Cannot get the number of columns of non-existent table " << tblHandle);
    return -1;
  }

  return static_cast<int>(this->Internals->Tables[tblHandle].Columns.size());
}

//------------------------------------------------------------------------------
int vtkSQLDatabaseSchema::GetNumberOfIndicesInTable(int tblHandle)
{
  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    vtkErrorMacro("Cannot get the number of indices of non-existent table " << tblHandle);
    return -1;
  }

  return static_cast<int>(this->Internals->Tables[tblHandle].Indices.size());
}

//------------------------------------------------------------------------------
int vtkSQLDatabaseSchema::GetNumberOfColumnNamesInIndex(int tblHandle, int idxHandle)
{
  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    vtkErrorMacro(
      "Cannot get the number of column names in index of non-existent table " << tblHandle);
    return -1;
  }

  if (idxHandle < 0 ||
    idxHandle >= static_cast<int>(this->Internals->Tables[tblHandle].Indices.size()))
  {
    vtkErrorMacro("Cannot get the number of column names of non-existent index "
      << idxHandle << " in table " << tblHandle);
    return -1;
  }

  return static_cast<int>(this->Internals->Tables[tblHandle].Indices[idxHandle].ColumnNames.size());
}

//------------------------------------------------------------------------------
int vtkSQLDatabaseSchema::GetNumberOfTriggersInTable(int tblHandle)
{
  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    vtkErrorMacro("Cannot get the number of triggers of non-existent table " << tblHandle);
    return -1;
  }

  return static_cast<int>(this->Internals->Tables[tblHandle].Triggers.size());
}

//------------------------------------------------------------------------------
int vtkSQLDatabaseSchema::GetNumberOfOptionsInTable(int tblHandle)
{
  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    vtkErrorMacro("Cannot get the number of options of non-existent table " << tblHandle);
    return -1;
  }

  return static_cast<int>(this->Internals->Tables[tblHandle].Options.size());
}
VTK_ABI_NAMESPACE_END
