/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStarbaseRenderWindow.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkStarbaseRenderWindow - HP starbase rendering window
// .SECTION Description
// vtkStarbaseRenderWindow is a concrete implementation of the abstract class
// vtkRenderWindow. vtkStarbaseRenderer interfaces to the Hewlett-Packard starbase
// graphics library.

#ifndef __vtkStarbaseRenderWindow_h
#define __vtkStarbaseRenderWindow_h

#include <stdlib.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "vtkXRenderWindow.h"

class VTK_EXPORT vtkStarbaseRenderWindow : public vtkXRenderWindow
{
public:
  vtkStarbaseRenderWindow();
  ~vtkStarbaseRenderWindow();
  static vtkStarbaseRenderWindow *New() {return new vtkStarbaseRenderWindow;};
  const char *GetClassName() {return "vtkStarbaseRenderWindow";};
  void PrintSelf(ostream& os, vtkIndent indent);
  

// Description:
// Begin the rendering process.
  void Start(void);


// Description:
// End the rendering process and display the image.
  void Frame(void);


// Description:
// Initialize the rendering window.
  void WindowInitialize(void);


// Description:
// Initialize the rendering window.
  void Initialize(void);


// Description:
// Change the window to fill the entire screen.
  virtual void SetFullScreen(int);


// Description:
// Resize the window.
  void WindowRemap(void);


// Description:
// Set the preferred window size to full screen.
  void PrefFullScreen(void);


// Description:
// Specify the size of the rendering window.
  void SetSize(int,int);


  vtkGetMacro(Fd,int);

  // stereo rendering stuff

// Description:
// Update system if needed due to stereo rendering.
  virtual void StereoUpdate();


  // Description:
  // Set/Get the pixel data of an image, transmitted as RGBRGB... 
  virtual unsigned char *GetPixelData(int x,int y,int x2,int y2, int front);
  virtual void SetPixelData(int x,int y,int x2,int y2,unsigned char *, int front);

  // Description:
  // Set/Get the pixel data of an image, transmitted as RGBARGBA... 
  virtual float *GetRGBAPixelData(int x,int y,int x2,int y2, int front);
  virtual void SetRGBAPixelData(int x,int y,int x2,int y2, float *, int front,
                                int blend=0);

  // Description:
  // Set/Get the zbuffer data from an image
  virtual float *GetZbufferData( int x1, int y1, int x2, int y2 );
  virtual void SetZbufferData( int x1, int y1, int x2, int y2, float *buffer);

  // Xwindow stuff
  int      GetDesiredDepth();

// Description:
// Obtain a colormap from windowing system.
  Colormap GetDesiredColormap();


// Description:
// Get a visual from the windowing system.
  Visual  *GetDesiredVisual();


// Description:
// Create a window for starbase output.
  int      CreateXWindow(Display *,int x,int y,int w,int h,int depth,
			 char name[80]);


protected:
  int      Fd;
  int      Buffer;
  int      NumPlanes;
};

#endif
