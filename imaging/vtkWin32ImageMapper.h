#ifndef __vtkWin32ImageMapper_h
#define __vtkWin32ImageMapper_h


// #include "vtkImageRegion.h"
#include "vtkImageMapper.h"
class vtkImageActor2D;


class VTK_EXPORT vtkWin32ImageMapper : public vtkImageMapper
{
public:

  vtkWin32ImageMapper();
  ~vtkWin32ImageMapper();

  static vtkWin32ImageMapper *New() {return new vtkWin32ImageMapper;};
  
  void RenderData(vtkViewport* viewport, vtkImageData* data, 
		    vtkActor2D* actor);

  unsigned char *DataOut;	// the data in the DIBSection
  HBITMAP HBitmap;			// our handle to the DIBSection

protected:
  int GetCompositingMode(vtkActor2D* actor);

};


#endif









