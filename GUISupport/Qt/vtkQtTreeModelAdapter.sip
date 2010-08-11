
%Import QtCore/QtCoremod.sip
%Import QtGui/QtGuimod.sip
%Import vtkFilteringPythonSIP.sip
%Include vtkQtAbstractModelAdapter.sip

class vtkQtTreeModelAdapter : vtkQtAbstractModelAdapter
{
%TypeHeaderCode
#include "vtkQtTreeModelAdapter.h"
%End

public:
  vtkQtTreeModelAdapter(QObject *parent = 0, vtkTree* tree = 0);
  ~vtkQtTreeModelAdapter();

  virtual void SetVTKDataObject(vtkDataObject *data);
  virtual vtkDataObject* GetVTKDataObject() const;

  unsigned long GetVTKDataObjectMTime() const;

  virtual vtkSelection* QModelIndexListToVTKIndexSelection(const QModelIndexList qmil) const;
  virtual QItemSelection VTKIndexSelectionToQItemSelection(vtkSelection *vtksel) const;

  virtual void SetKeyColumnName(const char* name);

  virtual void SetColorColumnName(const char* name);

  void setTree(vtkTree* t);
  vtkTree* tree() const;

  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
  Qt::ItemFlags flags(const QModelIndex &index) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
  QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
  QModelIndex parent(const QModelIndex &index) const;
  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  int columnCount(const QModelIndex &parent = QModelIndex()) const;

  Qt::DropActions supportedDragActions() const;
  virtual QMimeData * mimeData ( const QModelIndexList & indexes ) const;
  virtual QStringList mimeTypes () const ;

};
