
#ifndef __vtkImageWindow_h
#define __vtkImageWindow_h

#include "vtkWindow.h"
#include "vtkActor2D.h"
#include "vtkImageCache.h"
#include "vtkImager.h"
#include "vtkImageMapper.h"
#include "vtkImagerCollection.h"
#include "vtkWindow.h"

#define TOP_LEFT_ORIGIN 0

class VTK_EXPORT vtkImageWindow : public vtkWindow
{
public:
  vtkImageWindow();
  ~vtkImageWindow();

  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkImageWindow *New();

  // Description:
  // Set the position of the window on the screen
  virtual void SetPosition(int x, int y) = 0;
  virtual void SetPosition(int a[2]) { this->SetPosition(a[0],a[1]); };
 
  // Description:
  // Get the position of the window on the screen
  virtual int* GetPosition() = 0;
  virtual void GetPosition(int* x, int* y);

  // Description:
  // Sets the size of a window in pixels.
  virtual void SetSize(int , int ) = 0;
  virtual void SetSize(int a[2]) { this->SetSize(a[0], a[1]); };
  
  // Description:
  // Returns the size of a window in pixels
  virtual int* GetSize() = 0;
  virtual void GetSize(int *x, int *y);

  // Description:
  // These are here for using a tk window.
  virtual void SetDisplayId(void *) = 0;
  virtual void SetWindowId(void *) = 0;
  virtual void SetParentId(void *) = 0;
  virtual void *GetGenericDisplayId() = 0;
  virtual void *GetGenericWindowId() = 0;
  virtual void *GetGenericParentId() = 0;
  virtual void *GetGenericContext()   = 0;
  virtual void *GetGenericDrawable() {return NULL;};

  // Description:
  // Swap the front and back buffers.  This function
  // is used to implement double buffering.  The user
  // shouldn't need to call this function.  To enable
  // double buffering, invoke DoubleBufferOn
  virtual void SwapBuffers() = 0;

  // useful for scripting languages
  virtual void SetWindowInfo(char *) 
      { vtkErrorMacro(<<"vtkImageWindow::SetWindowInfo - Not implemented"); };

  vtkSetMacro(GrayScaleHint, int);
  vtkGetMacro(GrayScaleHint, int);
  vtkBooleanMacro(GrayScaleHint, int);

  // Description:
  // Add an imager to the window's list of imagers
  // to be rendered.
  void AddImager(vtkImager* im);

  // Description:
  // Remove an imager from the window
  void RemoveImager(vtkImager* im);
  
  // Description:
  // Draw the contents of the window
  void Render();

  // Description:
  // Erase the window contents 
  virtual void EraseWindow() = 0;

  // Description:
  // Save the current image as a PPM file.
  virtual void SaveImageAsPPM();

  // Description:
  // Open a PPM file
  virtual  int OpenPPMImageFile();

  // Description:
  // Write a PPM file
  virtual void WritePPMImageFile();

  // Description:
  // Close the PPM file
  virtual void ClosePPMImageFile();

  // Description:
  // Set/Get the pixel data of an image, transmitted as RGBRGBRGB. The
  // front argument indicates if the front buffer should be used or the back 
  // buffer. It is the caller's responsibility to delete the resulting 
  // array. It is very important to realize that the memory in this array
  // is organized from the bottom of the window to the top. The origin
  // of the screen is in the lower left corner. The y axis increases as
  // you go up the screen. So the storage of pixels is from left to right
  // and from bottom to top.
  virtual unsigned char *GetPixelData(int, int, int, int, int) {return (unsigned char *)NULL;};

  // Description:
  // Set/Get the FileName used for saving images. See the SaveImageAsPPM 
  // method.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

protected:
  vtkImagerCollection Imagers;
  int WindowCreated;
  int GrayScaleHint;
  virtual void MakeDefaultWindow() = 0;
  char *FileName;
  FILE* PPMImageFilePtr;

};


#endif




