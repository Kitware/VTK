
#ifndef TreeRingView_h
#define TreeRingView_h

#include "QVTKGraphicsItem.h"
class vtkTreeRingView;

class TreeRingViewItem : public QVTKGraphicsItem
{
public:
  TreeRingViewItem(QGLContext* ctx, QGraphicsItem* p=0);
  ~TreeRingViewItem();

protected:
  vtkSmartPointer<vtkTreeRingView> TreeRingView;
};

#endif
