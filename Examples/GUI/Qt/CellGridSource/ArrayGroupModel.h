#ifndef ArrayGroupModel_h
#define ArrayGroupModel_h

#include <QAbstractTableModel>

// #include <vtkDataSetAttributes.h>
// #include <vtkDataArray.h>
#include <vtkStringToken.h>

class vtkAbstractArray;
class vtkCellGrid;
class vtkDataSetAttributes;

class ArrayGroupModel : public QAbstractTableModel
{
public:
  ArrayGroupModel(vtkCellGrid* data, vtkStringToken groupName, QObject* parent = nullptr);
  ~ArrayGroupModel() override;

  bool setGroupName(vtkStringToken groupName, bool signalChange = false);
  vtkStringToken groupName() const { return this->GroupName; }

  int rowCount(const QModelIndex& parent) const override;
  int columnCount(const QModelIndex& parent) const override;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
  QVariant data(const QModelIndex& index, int role) const override;
  bool setData(const QModelIndex& index, const QVariant& value, int role) override;
  Qt::ItemFlags flags(const QModelIndex& index) const override; //  return Qt::ItemIsEditable.

Q_SIGNALS:
  void modelChanged();

protected:
  vtkCellGrid* Data;
  vtkStringToken GroupName;
  vtkDataSetAttributes* CurrentTable{ nullptr };
  std::unordered_map<vtkStringToken, int> ArrayColumnStart;
  struct ColumnData
  {
    ColumnData() = default;
    ColumnData(const ColumnData&) = default;
    ColumnData(vtkAbstractArray* array, int comp, const std::string& label)
      : Array(array)
      , Component(comp)
      , Label(label)
    {
    }

    vtkAbstractArray* Array{ nullptr };
    int Component{ 0 };
    std::string Label;
  };
  std::vector<ColumnData> ColumnToArrayComponent;
};

#endif // ArrayGroupModel_h
