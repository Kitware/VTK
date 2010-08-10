
%Import QtCore/QtCoremod.sip
%Import QtGui/QtGuimod.sip
%Import vtkCommonPythonSIP.sip
%Import vtkRenderingPythonSIP.sip
%Include QVTKInteractor.sip

class QVTKGraphicsItem : QGraphicsWidget
{
%TypeHeaderCode
#include "QVTKGraphicsItem.h"
%End

public:
  QVTKGraphicsItem(QGLContext* ctx, QGraphicsItem* p = 0);

  ~QVTKGraphicsItem();

  void SetRenderWindow(vtkGenericOpenGLRenderWindow* win);

  vtkGenericOpenGLRenderWindow* GetRenderWindow() const;

  QVTKInteractor* GetInteractor() const;

  virtual void Update();

protected:
  void MakeCurrent();
  void Start();
  void End();
  void IsCurrent(vtkObject* caller, unsigned long vtk_event, void* client_data, void* call_data);

  void keyPressEvent(QKeyEvent* e);
  void keyReleaseEvent(QKeyEvent* e);

  void mousePressEvent(QGraphicsSceneMouseEvent* e);
  void mouseReleaseEvent(QGraphicsSceneMouseEvent* e);
  void mouseMoveEvent(QGraphicsSceneMouseEvent* e);
  void resizeEvent(QGraphicsSceneResizeEvent* e);
  void moveEvent(QGraphicsSceneMoveEvent* e);
  void wheelEvent(QGraphicsSceneWheelEvent* e);
  void hoverEnterEvent(QGraphicsSceneHoverEvent* e);
  void hoverLeaveEvent(QGraphicsSceneHoverEvent* e);
  void hoverMoveEvent(QGraphicsSceneHoverEvent* e);

  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
};
