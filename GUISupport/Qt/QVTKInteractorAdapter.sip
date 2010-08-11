
%Import QtCore/QtCoremod.sip
%Import vtkRenderingPythonSIP.sip

class QVTKInteractorAdapter : QObject
{
%TypeHeaderCode
#include "QVTKInteractorAdapter.h"
%End

public:
  QVTKInteractorAdapter(QObject* parent);

  ~QVTKInteractorAdapter();

  bool ProcessEvent(QEvent* e, vtkRenderWindowInteractor* iren);
};
