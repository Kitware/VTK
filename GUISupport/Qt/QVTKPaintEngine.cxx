

#include "QVTKPaintEngine.h"
#include "QVTKWidget.h"

#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkImageData.h"
#include "vtkSmartPointer.h"

#include <QCache>
#include <QPainterPath>

class QVTKPaintEngineInternal
{
public:
  // cache of pixmaps
  QCache<qint64, vtkSmartPointer<vtkImageData> > mImageCache;
};

QVTKPaintEngine::QVTKPaintEngine() 
    : QPaintEngine(QPaintEngine::PaintOutsidePaintEvent |
                   QPaintEngine::AlphaBlend)
{
  this->Internal = new QVTKPaintEngineInternal;
}

QVTKPaintEngine::~QVTKPaintEngine()
{
  delete this->Internal;
}

bool QVTKPaintEngine::begin(QPaintDevice* dev)
{
  Widget = static_cast<QVTKWidget*>(dev);
  return true;
}

bool QVTKPaintEngine::end()
{
  Widget = NULL;
  return true;
}

QPaintEngine::Type QVTKPaintEngine::type() const
{
  return QPaintEngine::User;
}

void QVTKPaintEngine::updateState(const QPaintEngineState&)
{
}

  // at a minimum, we only need to re-implement this drawPixmap function.
  // Qt can do all other drawing to create a pixmap and then we draw it here.
void QVTKPaintEngine::drawPixmap(const QRectF& r, const QPixmap& pm, const QRectF& sr)
{
  if(!this->Widget)
    {
    return;
    }
  QRect ri = r.toRect();
  QRect sri = sr.toRect();

  QPixmap pix = pm.copy(sri);
  if(sri.size() != ri.size())
    {
    pix = pix.scaled(ri.size());
    }

  QImage img = pix.toImage().mirrored().rgbSwapped();
  
  // blend the pixels from QImage into the vtkRenderWindow's buffer
  vtkRenderWindow* renWin = this->Widget->GetRenderWindow();
  renWin->SetRGBACharPixelData(ri.left(), this->Widget->height() - ri.top() - ri.height(),
                               ri.left()+img.width() - 1,
                               this->Widget->height() - ri.top() - 1,
                               img.bits(),
                               renWin->GetDoubleBuffer() ? 0 : 1,
                               1);

  // NOTE: this would perform much better if textures were used and caching of those
  // textures was done (probably vtkActor2D and vtkImageMapper)
}

void QVTKPaintEngine::drawPath(const QPainterPath& path)
{
  // drawPath in base class does nothing so here we make it do something
  QRectF box = path.boundingRect();
  QPixmap img((int)box.width(), (int)box.height());
  img.fill(Qt::transparent);
  QPainter p(&img);
  p.translate(-box.left(), -box.right());
  p.drawPath(path);
  this->drawPixmap(QRectF(QPoint(0,0), img.size()), img, box);
}

