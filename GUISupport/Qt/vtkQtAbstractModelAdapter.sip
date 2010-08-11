
%Import QtCore/QtCoremod.sip
%Import QtGui/QtGuimod.sip
%Import vtkFilteringPythonSIP.sip

class vtkQtAbstractModelAdapter : QAbstractItemModel
{
%TypeHeaderCode
#include "vtkQtAbstractModelAdapter.h"
%End

public:
  enum
  {
    FULL_VIEW,
    DATA_VIEW
  };

  vtkQtAbstractModelAdapter(QObject* p);

  virtual void SetVTKDataObject(vtkDataObject *data) = 0;
  virtual vtkDataObject* GetVTKDataObject() const = 0;

  virtual vtkSelection* QModelIndexListToVTKIndexSelection(const QModelIndexList qmil) const = 0;
  virtual QItemSelection VTKIndexSelectionToQItemSelection(vtkSelection *vtksel) const = 0;

  virtual void SetViewType(int type);
  virtual int GetViewType();

  virtual void SetKeyColumn(int col);
  virtual int GetKeyColumn();
  virtual void SetKeyColumnName(const char* name) = 0;

  virtual void SetColorColumn(int col);
  virtual int GetColorColumn();
  virtual void SetColorColumnName(const char* name) = 0;

  virtual void SetDataColumnRange(int c1, int c2);

  void reset();
};
