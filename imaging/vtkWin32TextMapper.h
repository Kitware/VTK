
#ifndef __vtkWin32TextMapper_h
#define __vtkWin32TextMapper_h

#include "vtkMapper2D.h"
#include "vtkWindow.h"
#include "vtkViewport.h"
#include "vtkActor2D.h"
#include "vtkProperty2D.h"
#include "vtkTextMapper.h"

class VTK_EXPORT vtkWin32TextMapper : public vtkTextMapper
{
public:

  vtkWin32TextMapper();
  ~vtkWin32TextMapper();

  static vtkWin32TextMapper *New() {return new vtkWin32TextMapper;};
  int GetCompositingMode(vtkActor2D* actor);

  void Render(vtkViewport* viewport, vtkActor2D* actor);


protected:
  
};


#endif

