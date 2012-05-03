
%Import QtCore/QtCoremod.sip
%Import QtGui/QtGuimod.sip
%Import QtOpenGL/QtOpenGLmod.sip
%Import vtkRenderingPythonSIP.sip
%Include QVTKInteractor.sip

class QVTKWidget2 : QGLWidget
{
%TypeHeaderCode
#include "QVTKWidget2.h"
%End

public:

  QVTKWidget2(QWidget* parent = NULL, const QGLWidget* shareWidget=0, Qt::WindowFlags f = 0);
  QVTKWidget2(QGLContext* ctx, QWidget* parent = NULL, const QGLWidget* shareWidget=0, Qt::WindowFlags f = 0);
  QVTKWidget2(const QGLFormat& fmt, QWidget* parent = NULL, const QGLWidget* shareWidget=0, Qt::WindowFlags f = 0);

  ~QVTKWidget2();

  vtkGenericOpenGLRenderWindow* GetRenderWindow();
  void SetRenderWindow(vtkGenericOpenGLRenderWindow*);

  QVTKInteractor* GetInteractor();

protected:
  void resizeGL(int, int);
  void moveEvent(QMoveEvent*);
  void paintGL();
  void mousePressEvent(QMouseEvent*);
  void mouseMoveEvent(QMouseEvent*);
  void mouseReleaseEvent(QMouseEvent*);
  void enterEvent(QEvent*);
  void leaveEvent(QEvent*);
  void keyPressEvent(QKeyEvent*);
  void keyReleaseEvent(QKeyEvent*);
  void wheelEvent(QWheelEvent*);

  void MakeCurrent();
  void Frame();
  void Start();
  void End();

};
