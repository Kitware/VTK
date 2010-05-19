/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtSQLQuery.cxx

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

// Check for Qt SQL module before defining this class.
#include <qglobal.h>
#if (QT_EDITION & QT_MODULE_SQL)

#include "vtkQtSQLQuery.h"

#include "vtkCharArray.h"
#include "vtkObjectFactory.h"
#include "vtkQtSQLDatabase.h"
#include "vtkQtTimePointUtility.h"
#include "vtkVariantArray.h"

#include <QtSql/QtSql>
#include <QtSql/QSqlQuery>
#include <QString>
#include <QDateTime>
#include <QDate>
#include <QTime>
#include <vtksys/stl/string>
#include <vtksys/stl/vector>

class vtkQtSQLQueryInternals
{
public:
  QSqlQuery QtQuery;
  vtksys_stl::vector<vtksys_stl::string> FieldNames;
};


vtkStandardNewMacro(vtkQtSQLQuery);

vtkQtSQLQuery::vtkQtSQLQuery()
{
  this->Internals = new vtkQtSQLQueryInternals();
  this->Internals->QtQuery.setForwardOnly(true);
  this->LastErrorText = NULL;
}

vtkQtSQLQuery::~vtkQtSQLQuery()
{
  delete this->Internals;
  this->SetLastErrorText(NULL);
}

void vtkQtSQLQuery::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "LastErrorText: " << (this->LastErrorText ? this->LastErrorText : "NULL") << endl;
}

bool vtkQtSQLQuery::HasError()
{
  return this->Internals->QtQuery.lastError().isValid();
}

const char* vtkQtSQLQuery::GetLastErrorText()
{
  this->SetLastErrorText(this->Internals->QtQuery.lastError().text().toAscii());
  return this->LastErrorText;
}

bool vtkQtSQLQuery::Execute()
{
  if (this->Query == NULL)
    {
    vtkErrorMacro("Query string must be non-null.");
    return false;
    }
  this->Internals->QtQuery = vtkQtSQLDatabase::SafeDownCast(this->Database)->QtDatabase.exec(this->Query);

  QSqlError error = this->Internals->QtQuery.lastError();
  if (error.isValid())
    {
    QString errorString;
    errorString.sprintf("Query execute error: %s (type:%d)\n",
      error.text().toAscii().data(),error.type());
    vtkErrorMacro(<< errorString.toStdString());
    return false;
    }

  // cache the column names
  this->Internals->FieldNames.clear();
  for (int i = 0; i < this->Internals->QtQuery.record().count(); i++)
    {
    this->Internals->FieldNames.push_back(this->Internals->QtQuery.record().fieldName(i).toStdString());
    }
  return true;
}

int vtkQtSQLQuery::GetNumberOfFields()
{
  return this->Internals->QtQuery.record().count();
}

const char* vtkQtSQLQuery::GetFieldName(int col)
{
  return this->Internals->FieldNames[col].c_str();
}

int QVariantTypeToVTKType(QVariant::Type t)
{
  int type = -1;
  switch(t)
    {
    case QVariant::Bool:
      type = VTK_INT;
      break;
    case QVariant::Char:
      type = VTK_CHAR;
      break;
    case QVariant::DateTime:
    case QVariant::Date:
    case QVariant::Time:
      type = VTK_TYPE_UINT64;
      break;
    case QVariant::Double:
      type = VTK_DOUBLE;
      break;
    case QVariant::Int:
      type = VTK_INT;
      break;
    case QVariant::UInt:
      type = VTK_UNSIGNED_INT;
      break;
    case QVariant::LongLong:
      type = VTK_TYPE_INT64;
      break;
    case QVariant::ULongLong:
      type = VTK_TYPE_UINT64;
      break;
    case QVariant::String:
      type = VTK_STRING;
      break;
    case QVariant::ByteArray:
      type = VTK_STRING;
      break;
    case QVariant::Invalid:
    default:
      cerr << "Found unknown variant type: " << t << endl;
      type = -1;
    }
  return type;
}

int vtkQtSQLQuery::GetFieldType(int col)
{
  return QVariantTypeToVTKType(this->Internals->QtQuery.record().field(col).type());
}

bool vtkQtSQLQuery::NextRow()
{
  return this->Internals->QtQuery.next();
}

vtkVariant vtkQtSQLQuery::DataValue(vtkIdType c)
{
  QVariant v = this->Internals->QtQuery.value(c);
  switch (v.type())
    {
    case QVariant::Bool:
      return vtkVariant(v.toInt());
    case QVariant::Char:
      return vtkVariant(v.toChar().toAscii());
    case QVariant::DateTime:
      {
      QDateTime dt = v.toDateTime();
      vtkTypeUInt64 timePoint = vtkQtTimePointUtility::QDateTimeToTimePoint(dt);
      return vtkVariant(timePoint);
      }
    case QVariant::Date:
      {
      QDate date = v.toDate();
      vtkTypeUInt64 timePoint = vtkQtTimePointUtility::QDateToTimePoint(date);
      return vtkVariant(timePoint);
      }
    case QVariant::Time:
      {
      QTime time = v.toTime();
      vtkTypeUInt64 timePoint = vtkQtTimePointUtility::QTimeToTimePoint(time);
      return vtkVariant(timePoint);
      }
    case QVariant::Double:
      return vtkVariant(v.toDouble());
    case QVariant::Int:
      return vtkVariant(v.toInt());
    case QVariant::LongLong:
      return vtkVariant(v.toLongLong());
    case QVariant::String:
      return vtkVariant(v.toString().toStdString());
    case QVariant::UInt:
      return vtkVariant(v.toUInt());
    case QVariant::ULongLong:
      return vtkVariant(v.toULongLong());
    case QVariant::ByteArray:
      {
      // Carefully storing BLOBs as vtkStrings. This
      // avoids the normal termination problems with
      // zero's in the BLOBs... 
      return vtkVariant(vtkStdString(v.toByteArray().data(), v.toByteArray().length()));
      }
    case QVariant::Invalid:
      return vtkVariant();
    default:
      vtkErrorMacro(<< "Unhandled Qt variant type " 
        << v.type() << " found; returning string variant.");
      return vtkVariant(v.toString().toStdString());
    }
}

#endif // (QT_EDITION & QT_MODULE_SQL)
