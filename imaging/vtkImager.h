   

#ifndef __vtkImager_h
#define __vtkImager_h

#include "vtkReferenceCount.h"
#include "vtkActor2DCollection.h"
#include "vtkActor2D.h"
#include "vtkViewport.h"


class vtkImageWindow;

class VTK_EXPORT vtkImager : public vtkViewport
{ 
public:

  vtkImager();
  ~vtkImager();

  static vtkImager *New() {return new vtkImager;};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Renders an imager.  Passes Render message on the 
  // the imager's actor2D collection.
  void Render();

  // Description:
  // Set/Get the image window that an imager is attached to.
  void SetImageWindow (vtkImageWindow* win) {this->VTKWindow = (vtkWindow*) win;};
  vtkImageWindow* GetImageWindow () {return (vtkImageWindow*) this->VTKWindow;};
  void SetVTKWindow (vtkWindow* win) {this->VTKWindow = (vtkWindow*) win;};
  vtkWindow *GetVTKWindow() {return (vtkWindow*) this->VTKWindow;};

  // Description:
  // Erase the contents of the imager in the window.
  void Erase(){vtkErrorMacro(<<"vtkImager::Erase - Not implemented!");};


protected:


};


#endif




