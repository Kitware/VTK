
%Import QtCore/QtCoremod.sip
%Import QtGui/QtGuimod.sip
%Import vtkFilteringPythonSIP.sip
%Include vtkQtAbstractModelAdapter.sip

class vtkQtAnnotationLayersModelAdapter : vtkQtAbstractModelAdapter
{
%TypeHeaderCode
#include "vtkQtAnnotationLayersModelAdapter.h"
%End

public:
  vtkQtAnnotationLayersModelAdapter(QObject *parent = 0);
  vtkQtAnnotationLayersModelAdapter(vtkAnnotationLayers* ann, QObject *parent = 0);
  ~vtkQtAnnotationLayersModelAdapter();

  virtual void SetVTKDataObject(vtkDataObject *data);
  virtual vtkDataObject* GetVTKDataObject() const;

  virtual vtkAnnotationLayers* QModelIndexListToVTKAnnotationLayers(const QModelIndexList qmil) const;
  virtual QItemSelection VTKAnnotationLayersToQItemSelection(vtkAnnotationLayers *vtkann) const;
  virtual vtkSelection* QModelIndexListToVTKIndexSelection(const QModelIndexList qmil) const;
  virtual QItemSelection VTKIndexSelectionToQItemSelection(vtkSelection *vtksel) const;

  virtual void SetKeyColumnName(const char* name);
  virtual void SetColorColumnName(const char* name);

  void setAnnotationLayers(vtkAnnotationLayers* annotations);
  vtkAnnotationLayers* annotationLayers() const;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
  Qt::ItemFlags flags(const QModelIndex &index) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
  QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
  QModelIndex parent(const QModelIndex &index) const;
  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  int columnCount(const QModelIndex &parent = QModelIndex()) const;
};
