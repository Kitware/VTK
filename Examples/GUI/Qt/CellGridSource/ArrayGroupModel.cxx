#include "ArrayGroupModel.h"

#include <vtkCellGrid.h>
#include <vtkDataSetAttributes.h>
#include <vtkDoubleArray.h>
#include <vtkStringArray.h>

#include <QVariant>

#include <sstream>

namespace
{

std::string columnName(vtkAbstractArray* array, int component)
{
  std::ostringstream name;
  name << array->GetName() << " " << component;
  return name.str();
}

} // anonymous namespace

ArrayGroupModel::ArrayGroupModel(vtkCellGrid* data, vtkStringToken groupName, QObject* parent)
  : QAbstractTableModel(parent)
  , Data(data)
{
  this->Data->Register(nullptr);
  if (this->Data)
  {
    this->setGroupName(groupName, false);
  }
}

ArrayGroupModel::~ArrayGroupModel()
{
  this->Data->Delete();
  this->CurrentTable = nullptr;
}

bool ArrayGroupModel::setGroupName(vtkStringToken groupName, bool signalChange)
{
  if (groupName == this->GroupName)
  {
    return false;
  }
  if (signalChange)
  {
    Q_EMIT beginResetModel();
  }
  // Clear and rebuild CurrentTable, ArrayColumnStart, ColumnToArrayComponent.
  this->GroupName = groupName;
  this->CurrentTable = this->Data->FindAttributes(this->GroupName);
  this->ColumnToArrayComponent.clear();
  this->ArrayColumnStart.clear();
  if (this->CurrentTable)
  {
    int numArrays = this->CurrentTable->GetNumberOfArrays();
    for (int ii = 0; ii < numArrays; ++ii)
    {
      auto* array = this->CurrentTable->GetAbstractArray(ii);
      this->ArrayColumnStart[array->GetName()] =
        static_cast<int>(this->ColumnToArrayComponent.size());
      int numComps = array->GetNumberOfComponents();
      for (int jj = 0; jj < numComps; ++jj)
      {
        this->ColumnToArrayComponent.emplace_back(array, jj, columnName(array, jj));
      }
    }
  }
  // Notify Qt the table has completely changed.
  if (signalChange)
  {
    Q_EMIT endResetModel();
  }
  return true;
}

int ArrayGroupModel::rowCount(const QModelIndex& parent) const
{
  (void)parent;
  return this->CurrentTable ? this->CurrentTable->GetNumberOfTuples() : 0;
}

int ArrayGroupModel::columnCount(const QModelIndex& parent) const
{
  (void)parent;
  return static_cast<int>(this->ColumnToArrayComponent.size());
}

QVariant ArrayGroupModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (role != Qt::DisplayRole || orientation == Qt::Vertical)
  {
    return QVariant();
  }
  if (section < 0 || section > this->columnCount(QModelIndex()))
  {
    return QVariant();
  }
  return QString::fromStdString(this->ColumnToArrayComponent[section].Label);
}

QVariant ArrayGroupModel::data(const QModelIndex& index, int role) const
{
  if (role != Qt::DisplayRole)
  {
    return QVariant();
  }
  if (index.row() < 0 || index.column() < 0 || index.row() >= this->rowCount(index.parent()) ||
    index.column() >= this->columnCount(index.parent()))
  {
    return QVariant();
  }
  auto& colData(this->ColumnToArrayComponent[index.column()]);
  auto* array = colData.Array;
  if (auto* dataArray = vtkDataArray::SafeDownCast(array))
  {
    std::vector<double> tuple;
    tuple.resize(dataArray->GetNumberOfComponents());
    dataArray->GetTuple(index.row(), tuple.data());
    return tuple[colData.Component];
  }
  else if (auto* stringArray = vtkStringArray::SafeDownCast(array))
  {
    std::string value =
      stringArray->GetValue(stringArray->GetNumberOfComponents() * index.row() + colData.Component);
    return QString::fromStdString(value);
  }
  return QVariant();
}

bool ArrayGroupModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
  if (role != Qt::EditRole)
  {
    return false;
  }
  if (index.row() < 0 || index.column() < 0 || index.row() >= this->rowCount(index.parent()) ||
    index.column() >= this->columnCount(index.parent()))
  {
    return false;
  }
  // TODO: Record edit to a journal of edits, then update the output cell-grid.
  auto& colData(this->ColumnToArrayComponent[index.column()]);
  auto* array = colData.Array;
  if (auto* dataArray = vtkDataArray::SafeDownCast(array))
  {
    if (value.isNull() || value.toString().isEmpty())
    {
      return false;
    }
    std::vector<double> tuple;
    tuple.resize(dataArray->GetNumberOfComponents());
    dataArray->GetTuple(index.row(), tuple.data());
    tuple[colData.Component] = value.toDouble();
    dataArray->SetTuple(index.row(), tuple.data());
    this->Data->Modified();
    Q_EMIT dataChanged(index, index);
    return true;
  }
  else if (auto* stringArray = vtkStringArray::SafeDownCast(array))
  {
    stringArray->SetValue(stringArray->GetNumberOfComponents() * index.row() + colData.Component,
      value.toString().toStdString());
    this->Data->Modified();
    Q_EMIT dataChanged(index, index);
    return true;
  }
  return false;
}

Qt::ItemFlags ArrayGroupModel::flags(const QModelIndex& index) const
{
  (void)index;
  // return Qt::ItemIsEditable;
  return Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren;
}
