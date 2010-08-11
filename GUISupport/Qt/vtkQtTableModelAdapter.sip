
%Import QtCore/QtCoremod.sip
%Import QtGui/QtGuimod.sip
%Import vtkFilteringPythonSIP.sip
%Include vtkQtAbstractModelAdapter.sip

class vtkQtTableModelAdapter : vtkQtAbstractModelAdapter
{
%TypeHeaderCode
#include "vtkQtTableModelAdapter.h"
%End

public:
  vtkQtTableModelAdapter(QObject *parent = 0);
  vtkQtTableModelAdapter(vtkTable* table, QObject *parent = 0);
  ~vtkQtTableModelAdapter();

  virtual void SetVTKDataObject(vtkDataObject *data);
  virtual vtkDataObject* GetVTKDataObject() const;

  virtual vtkSelection* QModelIndexListToVTKIndexSelection(
    const QModelIndexList qmil) const;
  virtual QItemSelection VTKIndexSelectionToQItemSelection(
    vtkSelection *vtksel) const;

  virtual void SetKeyColumnName(const char* name);
  virtual void SetColorColumnName(const char* name);
  void SetIconIndexColumnName(const char* name);

  enum
  {
    HEADER = 0,
    ITEM = 1
  };

  enum
  {
    COLORS = 0,
    ICONS = 1,
    NONE = 2
  };

  void SetDecorationLocation(int s);

  void SetDecorationStrategy(int s);

  bool GetSplitMultiComponentColumns() const;
  void SetSplitMultiComponentColumns(bool value);

  void setTable(vtkTable* table);
  vtkTable* table() const;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
  Qt::ItemFlags flags(const QModelIndex &index) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
  QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
  QModelIndex parent(const QModelIndex &index) const;
  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  int columnCount(const QModelIndex &parent = QModelIndex()) const;

  virtual bool dropMimeData(const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent) ;
  virtual QMimeData * mimeData ( const QModelIndexList & indexes ) const;
  virtual QStringList mimeTypes () const ;
  Qt::DropActions supportedDropActions() const;

  void SetIconSheet(QImage sheet);
  void SetIconSize(int w, int h);
  void SetIconSheetSize(int w, int h);

};
