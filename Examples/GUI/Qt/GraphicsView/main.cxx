
#include <QApplication>
#include <QPainter>
#include <QGraphicsView>
#include <QResizeEvent>
#include <QCheckBox>
#include <QStyle>
#include <QGraphicsPixmapItem>
#include <QGraphicsProxyWidget>
#include <QDialog>
#include <QVBoxLayout>

#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkConeSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"

#include "QVTKWidget.h"

class MyQGraphicsView : public QGraphicsView
{
public:
  MyQGraphicsView()
  {
  mWidget = new QVTKWidget();
  this->setViewport(mWidget);
  this->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
  this->setCacheMode(CacheNone);

  vtkRenderWindow* renWin = vtkRenderWindow::New();
  mWidget->SetRenderWindow(renWin);
  renWin->Delete();
  }

  QVTKWidget* qvtkWidget() { return mWidget; }

protected:
  void resizeEvent(QResizeEvent* e)
    {
    if(this->scene())
      {
      this->scene()->setSceneRect(QRect(QPoint(0,0), e->size()));
      }
    QGraphicsView::resizeEvent(e);
    }

  void paintEvent(QPaintEvent* e)
    {
    this->mWidget->GetRenderWindow()->SetSwapBuffers(0);
    QGraphicsView::paintEvent(e);
    this->mWidget->GetRenderWindow()->SetSwapBuffers(1);
    this->mWidget->GetRenderWindow()->Frame();
    }
  
  void drawBackground(QPainter*, const QRectF& vtkNotUsed(r))
    {
    mWidget->GetRenderWindow()->Render();
    }
  QVTKWidget* mWidget;

};

class MyScene : public QGraphicsScene
{
public:
  MyScene(QObject* p = NULL) : QGraphicsScene(p)
  {
  QPixmap pix = qApp->style()->standardPixmap(QStyle::SP_ComputerIcon);
  QGraphicsItem* item = this->addPixmap(pix);
  item->setFlag(QGraphicsItem::ItemIsMovable, true);
  item->setPos(10,10);

  QDialog* d = new QDialog(0, Qt::CustomizeWindowHint | Qt::WindowTitleHint);
  d->setWindowOpacity(0.8);
  d->setWindowTitle("My Title");
  d->setLayout(new QVBoxLayout);
  d->layout()->addWidget(new QCheckBox("check me"));

  item = this->addWidget(d);
  item->setFlag(QGraphicsItem::ItemIsMovable, true);
  item->setPos(100,100);
  }
protected:
};

int main(int argc, char** argv)
{
  QApplication app(argc, argv);

  MyQGraphicsView view;
  view.resize(256,256);
  MyScene scene;
  view.setScene(&scene);
  
  QVTKWidget* qvtkwidget = view.qvtkWidget();
  vtkRenderWindow* renWin = qvtkwidget->GetRenderWindow();
  vtkRenderer *ren = vtkRenderer::New();
  renWin->AddRenderer(ren);
  ren->SetBackground( 0, 1, 1 );

  vtkConeSource *cone = vtkConeSource::New();
  cone->SetHeight( 0.5 );
  cone->SetRadius( 0.2 );
  cone->SetResolution( 20 );

  vtkPolyDataMapper *coneMapper = vtkPolyDataMapper::New();
  coneMapper->SetInputConnection( cone->GetOutputPort() );

  vtkActor *coneActor = vtkActor::New();
  coneActor->SetMapper( coneMapper );
  ren->AddActor( coneActor );

  view.show();

  app.exec();

  ren->Delete();
  cone->Delete();
  coneMapper->Delete();
  coneActor->Delete();

  return 0;
}

