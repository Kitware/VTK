
#ifndef OpenGLScene_hpp
#define OpenGLScene_hpp

#include <QGraphicsScene>
#include <QtOpenGL/QGLContext>
#include <QStateMachine>
#include "vtkSmartPointer.h"
class vtkGenericOpenGLRenderWindow;
class vtkRenderer;
class QVTKInteractor;
class QVTKInteractorAdapter;
class vtkEventQtSlotConnect;

class OpenGLScene : public QGraphicsScene
{
  Q_OBJECT
  public:
    OpenGLScene(QGLContext* ctx, QObject* p=0);
    ~OpenGLScene();

  Q_SIGNALS:
    void enterState1();
    void enterState2();
    void enterState3();
    void enterState4();

  protected:
    QGLContext* mContext;
    QStateMachine machine;
    QGraphicsWidget* mGraphLayoutView;
    QGraphicsWidget* mTreeRingView;
    QGraphicsWidget* mWebView;
    int CurrentState;

    void mousePressEvent(QGraphicsSceneMouseEvent* e);

};

#endif
