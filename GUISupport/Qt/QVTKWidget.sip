
%Import QtCore/QtCoremod.sip
%Import QtGui/QtGuimod.sip
%Import vtkRenderingPythonSIP.sip
%Include QVTKInteractor.sip

class QVTKWidget : QWidget
{
%TypeHeaderCode
#include "QVTKWidget.h"
%End

public:

  QVTKWidget(QWidget* parent = NULL, Qt::WindowFlags f = 0);
  ~QVTKWidget();

  vtkRenderWindow* GetRenderWindow();
  void SetRenderWindow(vtkRenderWindow*);

  QVTKInteractor* GetInteractor();

protected:
  void resizeEvent(QResizeEvent* event);
  void moveEvent(QMoveEvent* event);
  void paintEvent(QPaintEvent* event);
  void mousePressEvent(QMouseEvent* event);
  void mouseMoveEvent(QMouseEvent* event);
  void mouseReleaseEvent(QMouseEvent* event);
  void keyPressEvent(QKeyEvent* event);
  void keyReleaseEvent(QKeyEvent* event);
  void enterEvent(QEvent*);
  void leaveEvent(QEvent*);
  void wheelEvent(QWheelEvent*);
  void focusInEvent(QFocusEvent*);
  void focusOutEvent(QFocusEvent*);
  bool event( QEvent* e );

  void contextMenuEvent(QContextMenuEvent*);
  void dragEnterEvent(QDragEnterEvent*);
  void dragMoveEvent(QDragMoveEvent*);
  void dragLeaveEvent(QDragLeaveEvent*);
  void dropEvent(QDropEvent*);

};
