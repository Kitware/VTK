/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderWindow.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkRenderWindow - create a window for renderers to draw into
// .SECTION Description
// vtkRenderWindow is an abstract object to specify the behavior of a
// rendering window. A rendering window is a window in a graphical user
// interface where renderers draw their images. Methods are provided to 
// synchronize the rendering process, set window size, and control double
// buffering.  The window also allows rendering in stereo.  The interlaced
// render stereo type is for output to a VRex stereo projector.  All of the
// odd horizontal lines are from the left eye, and the even lines are from
// the right eye.  The user has to make the render window aligned with the 
// VRex projector, or the eye will be swapped.

// .SECTION see also
// vtkRenderer vtkRenderWindowInteractor

#ifndef __vtkRenderWindow_h
#define __vtkRenderWindow_h

#include "vtkWindow.h"
#include "vtkRendererCollection.h"
#include <stdio.h>
#include "vtkGraphicsFactory.h"

class vtkRenderWindowInteractor;



// lets define the different types of stereo
#define VTK_STEREO_CRYSTAL_EYES 1
#define VTK_STEREO_RED_BLUE     2
#define VTK_STEREO_INTERLACED   3
#define VTK_STEREO_LEFT         4
#define VTK_STEREO_RIGHT        5
#define VTK_STEREO_DRESDEN      6

class VTK_EXPORT vtkRenderWindow : public vtkWindow
{
public:
  vtkTypeMacro(vtkRenderWindow,vtkWindow);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct an instance of  vtkRenderWindow with its screen size 
  // set to 300x300, borders turned on, positioned at (0,0), double 
  // buffering turned on.
  static vtkRenderWindow *New();

  // Description:
  // Add a renderer to the list of renderers.
  void AddRenderer(vtkRenderer *);

  // Description:
  // Remove a renderer from the list of renderers.
  void RemoveRenderer(vtkRenderer *);

  // Description:
  // What rendering library has the user requested
  static const char *GetRenderLibrary() {
    return vtkGraphicsFactory::GetRenderLibrary();};

  // Description:
  // Return the collection of renderers in the render window.
  vtkRendererCollection *GetRenderers() {return this->Renderers;};

  // Description:
  // Ask each renderer owned by this RenderWindow to render its image and 
  // synchronize this process.
  virtual void Render();

  // Description:
  // Initialize the rendering process.
  virtual void Start() {};
  
  // Description:
  // A termination method performed at the end of the rendering process
  // to do things like swapping buffers (if necessary) or similar actions.
  virtual void Frame() {};

  // Description:
  // Performed at the end of the rendering process to generate image.
  // This is typically done right before swapping buffers.
  virtual void CopyResultFrame();

  // Description:
  // Create an interactor to control renderers in this window. We need
  // to know what type of interactor to create, because we might be in
  // X Windows or MS Windows. 
  virtual vtkRenderWindowInteractor *MakeRenderWindowInteractor();

  // Description:
  // Hide or Show the mouse cursor, it is nice to be able to hide the
  // default cursor if you want VTK to display a 3D cursor instead.
  virtual void HideCursor() {};
  virtual void ShowCursor() {};

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
  // Prescribe that the window be created in a stereo-capable mode. This
  // method must be called before the window is realized. Default if off.
  vtkGetMacro(StereoCapableWindow,int);
  vtkBooleanMacro(StereoCapableWindow,int);
  virtual void SetStereoCapableWindow(int capable);

  // Description:
  // Turn on/off stereo rendering.
  vtkGetMacro(StereoRender,int);
  void SetStereoRender(int stereo);
  vtkBooleanMacro(StereoRender,int);

  // Description:
  // Turn on/off point smoothing. Default is off.
  // This must be applied before the first Render.
  vtkSetMacro(PointSmoothing,int);
  vtkGetMacro(PointSmoothing,int);
  vtkBooleanMacro(PointSmoothing,int);

  // Description:
  // Turn on/off line smoothing. Default is off.
  // This must be applied before the first Render.
  vtkSetMacro(LineSmoothing,int);
  vtkGetMacro(LineSmoothing,int);
  vtkBooleanMacro(LineSmoothing,int);

  // Description:
  // Turn on/off polygon smoothing. Default is off.
  // This must be applied before the first Render.
  vtkSetMacro(PolygonSmoothing,int);
  vtkGetMacro(PolygonSmoothing,int);
  vtkBooleanMacro(PolygonSmoothing,int);

  // Description:
  // Set/Get what type of stereo rendering to use.
  vtkGetMacro(StereoType,int);
  vtkSetMacro(StereoType,int);
  void SetStereoTypeToCrystalEyes() 
    {this->SetStereoType(VTK_STEREO_CRYSTAL_EYES);};
  void SetStereoTypeToRedBlue() 
    {this->SetStereoType(VTK_STEREO_RED_BLUE);};
  void SetStereoTypeToInterlaced() 
    {this->SetStereoType(VTK_STEREO_INTERLACED);};
  void SetStereoTypeToLeft() 
    {this->SetStereoType(VTK_STEREO_LEFT);};
  void SetStereoTypeToRight() 
    {this->SetStereoType(VTK_STEREO_RIGHT);};
  void SetStereoTypeToDresden() 
    {this->SetStereoType(VTK_STEREO_DRESDEN);};  
  char *GetStereoTypeAsString();

  // Description:
  // Update the system, if needed, due to stereo rendering. For some stereo 
  // methods, subclasses might need to switch some hardware settings here.
  virtual void StereoUpdate();

  // Description:
  // Intermediate method performs operations required between the rendering
  // of the left and right eye.
  virtual void StereoMidpoint();

  // Description:
  // Handles work required once both views have been rendered when using
  // stereo rendering.
  virtual void StereoRenderComplete();

  // Description:
  // Remap the rendering window. This probably only works on UNIX right now.
  // It is useful for changing properties that can't normally be changed
  // once the window is up.
  virtual void WindowRemap() {};
  
  // Description:
  // Turn on/off buffer swapping between images. 
  vtkSetMacro(SwapBuffers,int);
  vtkGetMacro(SwapBuffers,int);
  vtkBooleanMacro(SwapBuffers,int);
  
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
  virtual void SetPixelData(int, int, int, int, unsigned char *,int) {};

  // Description:
  // Same as Get/SetPixelData except that the image also contains an alpha
  // component. The image is transmitted as RGBARGBARGBA... each of which is a
  // float value. The "blend" parameter controls whether the SetRGBAPixelData
  // method blends the data with the previous contents of the frame buffer
  // or completely replaces the frame buffer data.
  virtual float *GetRGBAPixelData(int ,int ,int ,int ,int ) {
    return (float *)NULL;};
  virtual void SetRGBAPixelData(int ,int ,int ,int ,float *,int,
                                int blend=0) { blend = blend;}
  virtual unsigned char *GetRGBACharPixelData(int ,int ,int ,int ,int ) {
    return (unsigned char*)NULL;}
  virtual void SetRGBACharPixelData(int ,int ,int ,int ,unsigned char *, int,
				    int blend=0) { blend = blend;}
  

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
  // This flag is set if the window hasn't rendered since it was created
  vtkGetMacro(NeverRendered,int);

  // Description:
  // This is a flag that can be set to interrupt a rendering that is in
  // progress.
  vtkGetMacro(AbortRender,int);
  vtkSetMacro(AbortRender,int);
  vtkGetMacro(InAbortCheck,int);
  vtkSetMacro(InAbortCheck,int);
  virtual int CheckAbortStatus();
  virtual int GetEventPending() { return 0;};
  
  // Description:
  // Specify a function to be called to check and see if an abort
  // of the rendering in progress is desired.
  void SetAbortCheckMethod(void (*f)(void *), void *arg);

  // Description:
  // Set the arg delete method. This is used to free user memory.
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
  // Get the number of layers for renderers.  Each renderer should have
  // its layer set individually.  Some algorithms iterate through all layers,
  // so it is not wise to set the number of layers to be exorbitantly large
  // (say bigger than 100).
  vtkGetMacro(NumLayers, int);
  vtkSetClampMacro(NumLayers, int, 1, VTK_LARGE_INTEGER);

  // Description:
  // Get the interactor associated with this render window
  vtkGetObjectMacro(Interactor,vtkRenderWindowInteractor);
  void SetInteractor(vtkRenderWindowInteractor *);

  // Description:
  // This Method detects loops of RenderWindow<->Interactor,
  // so objects are freed properly.
  void UnRegister(vtkObject *o);
  
  // Description:
  // Dummy stubs for vtkWindow API.
  virtual void SetDisplayId(void *) {}
  virtual void SetWindowId(void *)  {}
  virtual void SetParentId(void *)  {}
  virtual void *GetGenericDisplayId() {return NULL;}
  virtual void *GetGenericWindowId() {return NULL;}
  virtual void *GetGenericParentId() {return NULL;}
  virtual void *GetGenericContext() {return NULL;}
  virtual void *GetGenericDrawable() {return NULL;}
  virtual void SetWindowInfo(char *) {}
  virtual void SetParentInfo(char *) {};

  // Description:
  // Make this the current window. 
  virtual void MakeCurrent() {};

  // Description:
  // This method should be defined by the subclass. How many bits of
  // precision are there in the zbuffer?
  virtual int GetDepthBufferSize() {return -1;}

protected:
  vtkRenderWindow();
  ~vtkRenderWindow();
  vtkRenderWindow(const vtkRenderWindow&);
  void operator=(const vtkRenderWindow&);

  virtual void DoStereoRender();
  virtual void DoFDRender();
  virtual void DoAARender();

  vtkRendererCollection *Renderers;
  int Borders;
  int FullScreen;
  int OldScreen[5];
  int PointSmoothing;
  int LineSmoothing;
  int PolygonSmoothing;
  int StereoRender;
  int StereoType;
  int StereoStatus; // used for keeping track of what's going on
  int StereoCapableWindow;
  vtkRenderWindowInteractor *Interactor;
  char *FileName;
  unsigned char* StereoBuffer; // used for red blue stereo
  float *AccumulationBuffer;   // used for many techniques
  unsigned int AccumulationBufferSize;
  int AAFrames;
  int FDFrames;
  int SubFrames;               // number of sub frames
  int CurrentSubFrame;         // what one are we on
  unsigned char *ResultFrame;  // used for any non immediate rendering
  int   SwapBuffers;
  float DesiredUpdateRate;
  FILE* PPMImageFilePtr;
  int   AbortRender;
  int   InAbortCheck;
  int   InRender;
  int   NeverRendered;
  int   NumLayers;
  void (*AbortCheckMethod)(void *);
  void (*AbortCheckMethodArgDelete)(void *);
  void *AbortCheckMethodArg;
};

// Description:
// Return the stereo type as a character string.
inline char *vtkRenderWindow::GetStereoTypeAsString(void)
{
  switch ( this->StereoType )
    {
    case VTK_STEREO_CRYSTAL_EYES:
      return (char *)"CrystalEyes";
    case VTK_STEREO_RED_BLUE:
      return (char *)"RedBlue";
    case VTK_STEREO_LEFT:
      return (char *)"Left";
    case VTK_STEREO_RIGHT:
      return (char *)"Right";
    case VTK_STEREO_DRESDEN:
      return (char *)"DresdenDisplay";
    default:
      return (char *)"";
    }
}

#endif


