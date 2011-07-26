
#ifndef GraphicsView_hpp
#define GraphicsView_hpp

#include <QGraphicsView>
#include <QResizeEvent>
#include "QVTKWidget2.h"
#include "OpenGLScene.hpp"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkTextActor3D.h"

class GraphicsView : public QGraphicsView
{
  public:
    GraphicsView()
    {
      mCtx = new QGLContext(QGLFormat());
      mWidget = new QVTKWidget2(mCtx);
      this->setViewport(mWidget);
      this->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
      this->setScene(new OpenGLScene(mCtx, this));
      vtkSmartPointer<vtkRenderer> ren = vtkSmartPointer<vtkRenderer>::New();
      ren->SetBackground(0,0,0);
      ren->SetBackground2(1,1,1);
      ren->SetGradientBackground(1);
      vtkSmartPointer<vtkTextActor3D> textActor = vtkSmartPointer<vtkTextActor3D>::New();
      textActor->SetInput("Qt & VTK!!");
      ren->AddViewProp(textActor);
      ren->ResetCamera();
      mWidget->GetRenderWindow()->AddRenderer(ren);
      mWidget->GetRenderWindow()->SetSwapBuffers(0);  // don't let VTK swap buffers on us
      mWidget->setAutoBufferSwap(true);
    }
    ~GraphicsView()
    {
    }

  protected:

    void drawBackground(QPainter* p, const QRectF& vtkNotUsed(r))
      {
#if QT_VERSION >= 0x040600
      p->beginNativePainting();
#endif
      mWidget->GetRenderWindow()->PushState();
      mWidget->GetRenderWindow()->Render();
      mWidget->GetRenderWindow()->PopState();
#if QT_VERSION >= 0x040600
      p->endNativePainting();
#endif
      }

    void resizeEvent(QResizeEvent *event)
      {
        // give the same size to the scene that his widget has
        if (scene())
            scene()->setSceneRect(QRect(QPoint(0, 0), event->size()));
        QGraphicsView::resizeEvent(event);
        mWidget->GetRenderWindow()->SetSize(event->size().width(), event->size().height());
      }
    QGLContext* mCtx;
    QVTKWidget2* mWidget;
};

#endif
