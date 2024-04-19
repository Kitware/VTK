// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkQtSQLQuery.h"

#include "vtkCharArray.h"
#include "vtkObjectFactory.h"
#include "vtkQtSQLDatabase.h"
#include "vtkQtTimePointUtility.h"
#include "vtkVariantArray.h"

#include <QDate>
#include <QDateTime>
#include <QString>
#include <QTime>
#include <QtSql/QSqlQuery>
#include <QtSql/QtSql>
#include <string>
#include <vector>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#define vtk_qSqlFieldMetaType(sqlfield) sqlfield.type()
#define vtk_qVariantType(variant) variant.type()
#define vtk_qMetaType(name) QVariant::name
#define vtk_qMetaType_Q(name) vtk_qMetaType(name)
#define vtk_qMetaType_UnknownType vtk_qMetaType(Invalid)
#else
#define vtk_qSqlFieldMetaType(sqlfield) static_cast<QMetaType::Type>(sqlfield.metaType().id())
#define vtk_qVariantType(variant) variant.typeId()
#define vtk_qMetaType(name) QMetaType::name
#define vtk_qMetaType_Q(name) vtk_qMetaType(Q##name)
#define vtk_qMetaType_UnknownType vtk_qMetaType(UnknownType)
#endif

VTK_ABI_NAMESPACE_BEGIN
class vtkQtSQLQueryInternals
{
public:
  QSqlQuery QtQuery;
  std::vector<std::string> FieldNames;
};

vtkStandardNewMacro(vtkQtSQLQuery);

vtkQtSQLQuery::vtkQtSQLQuery()
{
  this->Internals = new vtkQtSQLQueryInternals();
  this->Internals->QtQuery.setForwardOnly(true);
  this->LastErrorText = nullptr;
}

vtkQtSQLQuery::~vtkQtSQLQuery()
{
  delete this->Internals;
  this->SetLastErrorText(nullptr);
}

void vtkQtSQLQuery::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "LastErrorText: " << (this->LastErrorText ? this->LastErrorText : "nullptr")
     << endl;
}

bool vtkQtSQLQuery::HasError()
{
  return this->Internals->QtQuery.lastError().isValid();
}

const char* vtkQtSQLQuery::GetLastErrorText()
{
  this->SetLastErrorText(this->Internals->QtQuery.lastError().text().toUtf8().data());
  return this->LastErrorText;
}

bool vtkQtSQLQuery::Execute()
{
  if (this->Query == nullptr)
  {
    vtkErrorMacro("Query string must be non-null.");
    return false;
  }
  this->Internals->QtQuery = QSqlQuery(vtkQtSQLDatabase::SafeDownCast(this->Database)->QtDatabase);
  this->Internals->QtQuery.exec(this->Query);

  QSqlError error = this->Internals->QtQuery.lastError();
  if (error.isValid())
  {
    QString errorString = QString("Query execute error: %1 (type:%2)\n")
                            .arg(error.text().toUtf8().data())
                            .arg(error.type());
    vtkErrorMacro(<< errorString.toUtf8().data());
    return false;
  }

  // cache the column names
  this->Internals->FieldNames.clear();
  for (int i = 0; i < this->Internals->QtQuery.record().count(); i++)
  {
    this->Internals->FieldNames.emplace_back(
      this->Internals->QtQuery.record().fieldName(i).toUtf8().data());
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

int QVariantTypeToVTKType(vtk_qMetaType(Type) t)
{
  int type = -1;
  switch (t)
  {
    case vtk_qMetaType(Bool):
      type = VTK_INT;
      break;
    case vtk_qMetaType(Char):
      type = VTK_CHAR;
      break;
    case vtk_qMetaType_Q(DateTime):
    case vtk_qMetaType_Q(Date):
    case vtk_qMetaType_Q(Time):
      type = VTK_TYPE_UINT64;
      break;
    case vtk_qMetaType(Double):
      type = VTK_DOUBLE;
      break;
    case vtk_qMetaType(Int):
      type = VTK_INT;
      break;
    case vtk_qMetaType(UInt):
      type = VTK_UNSIGNED_INT;
      break;
    case vtk_qMetaType(LongLong):
      type = VTK_TYPE_INT64;
      break;
    case vtk_qMetaType(ULongLong):
      type = VTK_TYPE_UINT64;
      break;
    case vtk_qMetaType_Q(String):
      type = VTK_STRING;
      break;
    case vtk_qMetaType_Q(ByteArray):
      type = VTK_STRING;
      break;
    case vtk_qMetaType_UnknownType:
    default:
      cerr << "Found unknown variant type: " << t << endl;
      type = -1;
  }
  return type;
}

int vtkQtSQLQuery::GetFieldType(int col)
{
  return QVariantTypeToVTKType(vtk_qSqlFieldMetaType(this->Internals->QtQuery.record().field(col)));
}

bool vtkQtSQLQuery::NextRow()
{
  return this->Internals->QtQuery.next();
}

vtkVariant vtkQtSQLQuery::DataValue(vtkIdType c)
{
  QVariant v = this->Internals->QtQuery.value(c);
  switch (vtk_qVariantType(v))
  {
    case vtk_qMetaType(Bool):
      return vtkVariant(v.toInt());
    case vtk_qMetaType(Char):
      return vtkVariant(v.toChar().toLatin1());
    case vtk_qMetaType_Q(DateTime):
    {
      QDateTime dt = v.toDateTime();
      vtkTypeUInt64 timePoint = vtkQtTimePointUtility::QDateTimeToTimePoint(dt);
      return vtkVariant(timePoint);
    }
    case vtk_qMetaType_Q(Date):
    {
      QDate date = v.toDate();
      vtkTypeUInt64 timePoint = vtkQtTimePointUtility::QDateToTimePoint(date);
      return vtkVariant(timePoint);
    }
    case vtk_qMetaType_Q(Time):
    {
      QTime time = v.toTime();
      vtkTypeUInt64 timePoint = vtkQtTimePointUtility::QTimeToTimePoint(time);
      return vtkVariant(timePoint);
    }
    case vtk_qMetaType(Double):
      return vtkVariant(v.toDouble());
    case vtk_qMetaType(Int):
      return vtkVariant(v.toInt());
    case vtk_qMetaType(LongLong):
      return vtkVariant(v.toLongLong());
    case vtk_qMetaType_Q(String):
      return vtkVariant(v.toString().toUtf8().data());
    case vtk_qMetaType(UInt):
      return vtkVariant(v.toUInt());
    case vtk_qMetaType(ULongLong):
      return vtkVariant(v.toULongLong());
    case vtk_qMetaType_Q(ByteArray):
    {
      // Carefully storing BLOBs as vtkStrings. This
      // avoids the normal termination problems with
      // zero's in the BLOBs...
      return vtkVariant(std::string(v.toByteArray().data(), v.toByteArray().length()));
    }
    case vtk_qMetaType_UnknownType:
      return vtkVariant();
    default:
      vtkErrorMacro(<< "Unhandled Qt variant type " << vtk_qVariantType(v)
                    << " found; returning string variant.");
      return vtkVariant(v.toString().toUtf8().data());
  }
}
VTK_ABI_NAMESPACE_END
