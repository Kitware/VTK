/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderWindow.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkRenderWindow - create a window for renderers to draw into
// .SECTION Description
// vtkRenderWindow is an abstract object to specify the behavior of a
// rendering window. A rendering window is a window in a graphical user
// interface where renderers draw their images. Methods are provided to 
// synchronize the rendering process, set window size, and control double
// buffering. 

// .SECTION see also
// vtkRenderer vtkRenderMaster vtkRenderWindowInteractor

#ifndef __vtkRenderWindow_h
#define __vtkRenderWindow_h

#include "vtkObject.h"
#include "vtkRendererCollection.h"
#include <stdio.h>

class vtkRenderWindowInteractor;
class vtkLightDevice;
class vtkCameraDevice;
class vtkActorDevice;
class vtkTextureDevice;
class vtkPropertyDevice;
class vtkPolyDataMapperDevice;

// lets define the diferent types of stereo
#define VTK_STEREO_CRYSTAL_EYES 1
#define VTK_STEREO_RED_BLUE     2

class VTK_EXPORT vtkRenderWindow : public vtkObject
{
public:
  vtkRenderWindow();
  ~vtkRenderWindow();
  static vtkRenderWindow *New();
  char *GetClassName() {return "vtkRenderWindow";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void AddRenderer(vtkRenderer *);
  void RemoveRenderer(vtkRenderer *);
  vtkRendererCollection *GetRenderers() {return &(this->Renderers);};

  virtual void Render();

  // Description:
  // Initialize the rendering process.
  virtual void Start() {};

  // Description:
  // A termination method performed at the end of the rendering process
  // to do things like swapping buffers (if necessary) or similar actions.
  virtual void Frame() {};

  virtual void SetDisplayId(void *) {};
  virtual void SetWindowId(void *) {};
  virtual void SetParentId(void *) {};

  // Description:
  // Performed at the end of the rendering process to generate image.
  // This is typically done right before swapping buffers.
  virtual void CopyResultFrame();

  // Description:
  // Create a device specific renderer. This is the only way to create
  // a renderer that will work. This method is implemented in the
  // subclasses of vtkRenderWindow so that each subclass will return
  // the correct renderer for its graphics library.
  virtual vtkRenderer  *MakeRenderer();

  static char *GetRenderLibrary();
  
  // Description:
  // Create an interactor to control renderers in this window. We need
  // to know what type of interactor to create, because we might be in
  // X Windows or MS Windows. 
  virtual vtkRenderWindowInteractor *MakeRenderWindowInteractor();

  // Description:
  // Set/Get the position in screen coordinates of the rendering window.
  virtual int *GetPosition() {return (int *)NULL;};
  virtual void SetPosition(int,int);
  virtual void SetPosition(int a[2]);

  // Description:
  // Set/Get the size of the window in screen coordinates.
  virtual int *GetSize() {return (int *)NULL;};
  virtual void SetSize(int,int) {};
  virtual void SetSize(int a[2]);

  // Description:
  // Turn on/off rendering full screen window size.
  virtual void SetFullScreen(int) {};
  vtkGetMacro(FullScreen,int);
  vtkBooleanMacro(FullScreen,int);

  // Description:
  // Turn on/off window manager borders. Typically, you shouldn't turn the 
  // borders off, because that bypasses the window manager and can cause
  // undesirable behavior.
  vtkSetMacro(Borders,int);
  vtkGetMacro(Borders,int);
  vtkBooleanMacro(Borders,int);

  // Description:
  // Keep track of whether the rendering window has been mapped to screen.
  vtkSetMacro(Mapped,int);
  vtkGetMacro(Mapped,int);
  vtkBooleanMacro(Mapped,int);

  // Description:
  // Turn on/off double buffering.
  vtkSetMacro(DoubleBuffer,int);
  vtkGetMacro(DoubleBuffer,int);
  vtkBooleanMacro(DoubleBuffer,int);

  // Description:
  // Turn on/off stereo rendering.
  vtkGetMacro(StereoRender,int);
  vtkSetMacro(StereoRender,int);
  vtkBooleanMacro(StereoRender,int);

  // Description:
  // Set/Get what type of stereo rendering to use.
  vtkGetMacro(StereoType,int);
  vtkSetMacro(StereoType,int);
  void SetStereoTypeToCrystalEyes() 
    {this->SetStereoType(VTK_STEREO_CRYSTAL_EYES);};
  void SetStereoTypeToRedBlue() 
    {this->SetStereoType(VTK_STEREO_RED_BLUE);};
  char *GetStereoTypeAsString();

  virtual void StereoUpdate();
  virtual void StereoMidpoint();
  virtual void StereoRenderComplete();

  virtual int  GetRemapWindow();
  virtual void WindowRemap() {};
  
  // Description:
  // Turn on/off erasing the screen between images. This allows multiple 
  // exposure sequences if turned on. You will need to turn double 
  // buffering off or make use of the SwapBuffers methods to prevent
  // you from swapping buffers between exposures.
  vtkSetMacro(Erase,int);
  vtkGetMacro(Erase,int);
  vtkBooleanMacro(Erase,int);

  // Description:
  // Turn on/off buffer swapping between images. 
  vtkSetMacro(SwapBuffers,int);
  vtkGetMacro(SwapBuffers,int);
  vtkBooleanMacro(SwapBuffers,int);
  
  // Description:
  // Get name of rendering window
  vtkGetStringMacro(WindowName);
  virtual void SetWindowName( char * );

  // Description:
  // Set/Get the FileName used for saving images. See the SaveImageAsPPM 
  // method.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Save the current image as a PPM file.
  virtual void SaveImageAsPPM();
  virtual  int OpenPPMImageFile();
  virtual void WritePPMImageFile();
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
  virtual void SetPixelData(int, int, int, int, unsigned char *,int) {};

  // Description:
  // Same as Get/SetPixelData except that the image also contains an alpha
  // component. The image is transmitted as RGBARGBARGBA... each of which is a
  // float value.
  virtual float *GetRGBAPixelData(int ,int ,int ,int ,int ) {return (float *)NULL;};
  virtual void SetRGBAPixelData(int ,int ,int ,int ,float *,int ) {};

  // Description:
  // Set/Get the zbuffer data from the frame buffer.
  virtual float *GetZbufferData(int, int, int, int ) {return (float *)NULL;};
  virtual void SetZbufferData(int, int, int, int, float *) {};

  // Description:
  // Set the number of frames for doing antialiasing. The default is
  // zero. Typically five or six will yield reasonable results without
  // taking too long.
  vtkGetMacro(AAFrames,int);
  vtkSetMacro(AAFrames,int);

  // Description:
  // Set the number of frames for doing focal depth. The default is zero.
  // Depending on how your scene is organized you can get away with as
  // few as four frames for focal depth or you might need thirty.
  // One thing to note is that if you are using focal depth frames,
  // then you will not need many (if any) frames for antialiasing. 
  vtkGetMacro(FDFrames,int);
  vtkSetMacro(FDFrames,int);

  // Description:
  // Set the number of sub frames for doing motion blur. The default is zero.
  // Once this is set greater than one, you will no longer see a new frame
  // for every Render().  If you set this to five, you will need to do 
  // five Render() invocations before seeing the result. This isn't
  // very impressive unless something is changing between the Renders.
  vtkGetMacro(SubFrames,int);
  vtkSetMacro(SubFrames,int);

  // Description:
  // This is a flag that can be set to interrupt a rendering that is in
  // progress.
  vtkGetMacro(AbortRender,int);
  vtkSetMacro(AbortRender,int);
  vtkGetMacro(InAbortCheck,int);
  virtual int CheckAbortStatus();
  virtual int GetEventPending() { return 0;};
  
  
  void SetAbortCheckMethod(void (*f)(void *), void *arg);
  void SetAbortCheckMethodArgDelete(void (*f)(void *));

  // Description:
  // Set/Get the desired update rate. This is used with
  // the vtkLODActor class. When using level of detail actors you
  // need to specify what update rate you require. The LODActors then
  // will pick the correct resolution to meet your desired update rate
  // in frames per second. A value of zero indicates that they can use
  // all the time they want to.
  void SetDesiredUpdateRate(float);
  vtkGetMacro(DesiredUpdateRate,float);

  // Description:
  // Get the interactor associated with this render window
  vtkGetObjectMacro(Interactor,vtkRenderWindowInteractor);
  void SetInteractor(vtkRenderWindowInteractor *);

protected:
  virtual void DoStereoRender();
  virtual void DoFDRender();
  virtual void DoAARender();

  vtkRendererCollection Renderers;
  char *WindowName;
  int Size[2];
  int Position[2];
  int Borders;
  int FullScreen;
  int OldScreen[5];
  int Mapped;
  int DoubleBuffer;
  int StereoRender;
  int StereoType;
  int StereoStatus; // used for keeping track of what's going on
  vtkRenderWindowInteractor *Interactor;
  char *FileName;
  unsigned char* StereoBuffer; // used for red blue stereo
  float *AccumulationBuffer;   // used for many techniques
  int AAFrames;
  int FDFrames;
  int SubFrames;               // number of sub frames
  int CurrentSubFrame;         // what one are we on
  unsigned char* ResultFrame;  // used for any non immediate rendering
  int   Erase;
  int   SwapBuffers;
  float DesiredUpdateRate;
  FILE* PPMImageFilePtr;
  int   AbortRender;
  int   InAbortCheck;
  void (*AbortCheckMethod)(void *);
  void (*AbortCheckMethodArgDelete)(void *);
  void *AbortCheckMethodArg;
};

// Description:
// Return the stereo type as a character string.
inline char *vtkRenderWindow::GetStereoTypeAsString(void)
{
  if ( this->StereoType == VTK_STEREO_CRYSTAL_EYES ) 
    {
    return "CrystalEyes";
    }
  else 
    {
    return "RedBlue";
    }
}

#endif


