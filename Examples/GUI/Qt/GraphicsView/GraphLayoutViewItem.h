
#ifndef GraphLayoutViewItem_h
#define GraphLayoutViewItem_h

#include "QVTKGraphicsItem.h"
class vtkGraphLayoutView;

class GraphLayoutViewItem : public QVTKGraphicsItem
{
public:
  GraphLayoutViewItem(QGLContext* ctx, QGraphicsItem* p=0);
  ~GraphLayoutViewItem();

protected:
  vtkSmartPointer<vtkGraphLayoutView> GraphLayoutView;
};

#endif
