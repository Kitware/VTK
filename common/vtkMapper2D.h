
#ifndef __vtkMapper2D_h
#define __vtkMapper2D_h

#include "vtkReferenceCount.h"
#include "vtkViewport.h"
#include "vtkWindow.h"
#include "vtkActor2D.h"
// #include "vtkImager.h"

#define XMIN 0
#define YMIN 1
#define XMAX 2
#define YMAX 3

class VTK_EXPORT vtkMapper2D : public vtkReferenceCount
{
public:
  virtual void Render(vtkViewport* viewport, vtkActor2D* actor) = 0;
  void GetActorClipSize(vtkViewport* viewport, vtkActor2D* actor, int* clipWidth, int* clipHeight);
  void GetViewportClipSize(vtkViewport* viewport, int* clipWidth, int* clipHeight);

protected:


};

#endif



