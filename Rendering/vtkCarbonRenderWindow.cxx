/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkCarbonRenderWindow.cxx
Language:  C++
Date:      $Date$
Version:   $Revision$
Thanks:    to Yves Starreveld for developing this class

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include "vtkCarbonRenderWindow.h"
#include "vtkCarbonRenderWindowInteractor.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLProperty.h"
#include "vtkOpenGLTexture.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLLight.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkObjectFactory.h"


vtkCxxRevisionMacro(vtkCarbonRenderWindow, "1.4");
vtkStandardNewMacro(vtkCarbonRenderWindow);


#define VTK_MAX_LIGHTS 8

//----------------------------------------------------------------------------
// Copy C string to Pascal string
// Some of the Carbon routines require Pascal strings
static void CStrToPStr (StringPtr outString, const char *inString)
{
  unsigned char x = 0;
  do
    {
    *(((char*)outString) + x + 1) = *(inString + x++);
    }  
  while ((*(inString + x) != 0)  && (x < 256));
  *((char*)outString) = (char) x;
}

//----------------------------------------------------------------------------
// Dump agl errors to string, return error code
OSStatus aglReportError (void)
{
  GLenum err = aglGetError();
  if (AGL_NO_ERROR != err)
    cout << ((char *)aglErrorString(err));
  // ensure we are returning an OSStatus noErr if no error condition
  if (err == AGL_NO_ERROR)
    return noErr;
  else
    return (OSStatus) err;
}

//----------------------------------------------------------------------------
// if error dump gl errors, return error
OSStatus glReportError (void)
{
  GLenum err = glGetError();
  switch (err)
  {
  case GL_NO_ERROR:
    break;
  case GL_INVALID_ENUM:
    cout << "GL Error: Invalid enumeration\n";
    break;
  case GL_INVALID_VALUE:
    cout << "GL Error: Invalid value\n";
    break;
  case GL_INVALID_OPERATION:
    cout << "GL Error: Invalid operation\n";
    break;
  case GL_STACK_OVERFLOW:
    cout << "GL Error: Stack overflow\n";
    break;
  case GL_STACK_UNDERFLOW:
    cout << "GL Error: Stack underflow\n";
    break;
  case GL_OUT_OF_MEMORY:
    cout << "GL Error: Out of memory\n";
    break;
  }
  // ensure we are returning an OSStatus noErr if no error condition
  if (err == GL_NO_ERROR)
    return noErr;
  else
    return (OSStatus) err;
}

//----------------------------------------------------------------------------
// CheckRenderer
// looks at renderer attributes it has at least the VRAM is accelerated
// Inputs:   hGD: GDHandle to device to look at
//        pVRAM: pointer to VRAM in bytes required; out is actual VRAM if
//                   a renderer was found, otherwise it is the input parameter
//     pTextureRAM:  pointer to texture RAM in bytes required; out is same
//                   (implementation assume VRAM returned by card is total
//                   so we add texture and VRAM)
//   fAccelMust: do we check for acceleration
// Returns: true if renderer for the requested device complies, false otherwise
static Boolean CheckRenderer (GDHandle hGD, long* pVRAM, long* pTextureRAM,
            GLint* pDepthSizeSupport, Boolean fAccelMust)
{
  AGLRendererInfo info, head_info;
  GLint inum;
  GLint dAccel = 0;
  GLint dVRAM = 0, dMaxVRAM = 0;
  Boolean canAccel = false, found = false;
  head_info = aglQueryRendererInfo(&hGD, 1);
  aglReportError();
  if(!head_info)
    {
    cout << "aglQueryRendererInfo error.\n";
    return false;
    }
  else
    {
    info = head_info;
    inum = 0;
    // Check for accelerated renderer, if so ignore non-accelerated ones
    // Prevents returning info on software renderer when get a hardware one
    while (info)
      {
      aglDescribeRenderer(info, AGL_ACCELERATED, &dAccel);
      aglReportError ();
      if (dAccel)
        canAccel = true;
      info = aglNextRendererInfo(info);
      aglReportError ();
      inum++;
      }

    info = head_info;
    inum = 0;
    while (info)
      {
      aglDescribeRenderer (info, AGL_ACCELERATED, &dAccel);
      aglReportError ();
      // if we can accel then we will choose the accelerated renderer
      // how about compliant renderers???
      if ((canAccel && dAccel) || (!canAccel && (!fAccelMust || dAccel)))
        {
        aglDescribeRenderer (info, AGL_VIDEO_MEMORY, &dVRAM);
        // we assume that VRAM returned is total thus add
        // texture and VRAM required
        aglReportError ();
        if (dVRAM >= (*pVRAM + *pTextureRAM))
          {
          if (dVRAM >= dMaxVRAM) // find card with max VRAM
            {
            aglDescribeRenderer (info, AGL_DEPTH_MODES, pDepthSizeSupport);
            // which depth buffer modes are supported
            aglReportError ();
            dMaxVRAM = dVRAM; // store max
            found = true;
            }
          }
        }
      info = aglNextRendererInfo(info);
      aglReportError ();
      inum++;
      }
    }
  aglDestroyRendererInfo(head_info);
  if (found) // if found a card with enough VRAM and meets the accel criteria
    {
    *pVRAM = dMaxVRAM; // return VRAM
    return true;
    }
  // VRAM will remain to same as it did when sent in
  return false;
}

//----------------------------------------------------------------------------
// CheckAllDeviceRenderers
// looks at renderer attributes and each device must have at least one 
// renderer that fits the profile.
// Inputs:   pVRAM: pointer to VRAM in bytes required; 
//                  out is actual min VRAM of all renderers found,
//                  otherwise it is the input parameter
//     pTextureRAM: pointer to texture RAM in bytes required; 
//                  out is same (implementation assumes VRAM returned 
//                  by card is total so we add texture and VRAM)
//      fAccelMust: do we check fro acceleration
// Returns: true if any renderer on each device complies (not necessarily
//          the same renderer), false otherwise

static Boolean CheckAllDeviceRenderers (long* pVRAM, long* pTextureRAM,
          GLint* pDepthSizeSupport, 
          Boolean fAccelMust)
{
  AGLRendererInfo info, head_info;
  GLint inum;
  GLint dAccel = 0;
  GLint dVRAM = 0, dMaxVRAM = 0;
  Boolean canAccel = false, found = false, goodCheck = true;
  long MinVRAM = 0x8FFFFFFF; // max long
  GDHandle hGD = GetDeviceList (); // get the first screen
  while (hGD && goodCheck)
    {
    head_info = aglQueryRendererInfo(&hGD, 1);
    aglReportError ();
    if(!head_info)
      {
      cout << "aglQueryRendererInfo error";
      return false;
      }
    else
      {
      info = head_info;
      inum = 0;
      // if accelerated renderer, ignore non-accelerated ones
      // prevents returning info on software renderer when get hardware one
      while (info)
        {
        aglDescribeRenderer(info, AGL_ACCELERATED, &dAccel);
        aglReportError ();
        if (dAccel)
          canAccel = true;
        info = aglNextRendererInfo(info);
        aglReportError ();
        inum++;
        }

      info = head_info;
      inum = 0;
      while (info)
        {
        aglDescribeRenderer(info, AGL_ACCELERATED, &dAccel);
        aglReportError ();
        // if we can accel then we will choose the accelerated renderer
        // how about compliant renderers???
        if ((canAccel && dAccel) || (!canAccel && (!fAccelMust || dAccel)))
          {
          aglDescribeRenderer(info, AGL_VIDEO_MEMORY, &dVRAM);
          aglReportError ();
          if (dVRAM >= (*pVRAM + *pTextureRAM))
            {
            if (dVRAM >= dMaxVRAM) // find card with max VRAM
              {// which depth buffer modes are supported
              aglDescribeRenderer(info, AGL_DEPTH_MODES, pDepthSizeSupport);
              aglReportError ();
              dMaxVRAM = dVRAM; // store max
              found = true;
              }
            }
          }
        info = aglNextRendererInfo(info);
        aglReportError ();
        inum++;
        }
      }
    aglDestroyRendererInfo(head_info);
    if (found) // found card with enough VRAM and meets the accel criteria
      {
      if (MinVRAM > dMaxVRAM)
        {
        MinVRAM = dMaxVRAM; // return VRAM
        }
      }
    else
      goodCheck = false; // one device failed thus entire requirement fails
    hGD = GetNextDevice (hGD); // get next device
    } // while
  if (goodCheck) // we check all devices and each was good
    {
    *pVRAM = MinVRAM; // return VRAM
    return true;
    }
  return false; //at least one device failed to have mins
}

//--------------------------------------------------------------------------
// FindGDHandleFromWindow
// Inputs:  a valid WindowPtr
// Outputs:  the GDHandle that that window is mostly on
// returns the number of devices that the windows content touches
short FindGDHandleFromWindow (WindowPtr pWindow, GDHandle * phgdOnThisDevice)
{
  GrafPtr pgpSave;
  Rect rectWind, rectSect;
  long greatestArea, sectArea;
  short numDevices = 0;
  GDHandle hgdNthDevice;

  if (!pWindow || !phgdOnThisDevice)
    return NULL;

  *phgdOnThisDevice = NULL;

  GetPort (&pgpSave);
  SetPortWindowPort (pWindow);

  GetWindowPortBounds (pWindow, &rectWind);
  LocalToGlobal ((Point*)& rectWind.top);
  LocalToGlobal ((Point*)& rectWind.bottom);
  hgdNthDevice = GetDeviceList ();
  greatestArea = 0;
  // check window against all gdRects in gDevice list and remember
  // which gdRect contains largest area of window}
  while (hgdNthDevice)
    {
    if (TestDeviceAttribute (hgdNthDevice, screenDevice))
      if (TestDeviceAttribute (hgdNthDevice, screenActive))
        {
        // The SectRect routine calculates the intersection
        //  of the window rectangle and this gDevice
        //  rectangle and returns TRUE if the rectangles intersect,
        //  FALSE if they don't.
        SectRect (&rectWind, &(**hgdNthDevice).gdRect, &rectSect);
        // determine which screen holds greatest window area
        //  first, calculate area of rectangle on current device
        sectArea = (long) ((rectSect.right - rectSect.left) * 
         (rectSect.bottom - rectSect.top));
        if (sectArea > 0)
          numDevices++;
        if (sectArea > greatestArea)
          {
          greatestArea = sectArea; // set greatest area so far
          *phgdOnThisDevice = hgdNthDevice; // set zoom device
          }
        hgdNthDevice = GetNextDevice(hgdNthDevice);
        }
    }
    SetPort (pgpSave);
    return numDevices;
}

//--------------------------------------------------------------------------
vtkCarbonRenderWindow::vtkCarbonRenderWindow()
{
  this->ApplicationInitialized = 0;
  this->ContextId = 0;
  this->MultiSamples = 8;
  this->WindowId = 0;
  this->ParentId = 0;
  this->StereoType = 0;
  this->SetWindowName("Visualization Toolkit - Carbon");
  this->TextureResourceIds = vtkIdList::New();
  this->CursorHidden = 0;
}

// --------------------------------------------------------------------------
vtkCarbonRenderWindow::~vtkCarbonRenderWindow()
{
  if (this->CursorHidden)
    {
    this->ShowCursor();
    }
  if (this->WindowId && this->OwnWindow)
    {
    this->Clean();
    // can't set WindowId=NULL, needed for DestroyWindow

    DisposeWindow(this->WindowId);
    }
}

//--------------------------------------------------------------------------
void vtkCarbonRenderWindow::Clean()
{
  vtkRenderer *ren;
  GLuint id;

  /* finish OpenGL rendering */
  if (this->ContextId)
    {
    this->MakeCurrent();

    /* now delete all textures */
    glDisable(GL_TEXTURE_2D);
    for (int i = 1; i < this->TextureResourceIds->GetNumberOfIds(); i++)
      {
      id = (GLuint) this->TextureResourceIds->GetId(i);
#ifdef GL_VERSION_1_1
      if (glIsTexture(id))
        {
        glDeleteTextures(1, &id);
        }
#else
      if (glIsList(id))
        {
        glDeleteLists(id,1);
        }
#endif
      }

    // tell each of the renderers that this render window/graphics context
    // is being removed (the RendererCollection is removed by vtkRenderWindow's
    // destructor)
    this->Renderers->InitTraversal();
    for (ren=(vtkOpenGLRenderer *)this->Renderers->GetNextItemAsObject();
         ren != NULL;
         ren = (vtkOpenGLRenderer *)this->Renderers->GetNextItemAsObject())
      {
      ren->SetRenderWindow(NULL);
      }

    aglSetCurrentContext(this->ContextId);
    aglDestroyContext(this->ContextId);
    this->ContextId = NULL;
    }
}

//--------------------------------------------------------------------------
void vtkCarbonRenderWindow::SetWindowName( char * _arg )
{
  vtkWindow::SetWindowName(_arg);
  Str255 newTitle = "\p"; // SetWTitle takes a pascal string

  CStrToPStr (newTitle, _arg);
  if (this->WindowId)
    {
    SetWTitle (this->WindowId, newTitle);
    }
}

//--------------------------------------------------------------------------
int vtkCarbonRenderWindow::GetEventPending()
{
  return 0;
}

//--------------------------------------------------------------------------
// Set the window id to a pre-existing window.
void vtkCarbonRenderWindow::SetParentId(WindowPtr arg)
{
  vtkDebugMacro(<< "Setting ParentId to " << arg << "\n");

  this->ParentId = arg;
}

//--------------------------------------------------------------------------
// Begin the rendering process.
void vtkCarbonRenderWindow::Start(void)
{
  // if the renderer has not been initialized, do so now
  if (!this->ContextId)
    {
    this->Initialize();
    }

  // set the current window
  this->MakeCurrent();
}

// --------------------------------------------------------------------------
void vtkCarbonRenderWindow::MakeCurrent()
{
  if (this->ContextId)
    {
    aglSetCurrentContext(this->ContextId);
    }
}

// --------------------------------------------------------------------------
void vtkCarbonRenderWindow::SetSize(int x, int y)
{
  static int resizing = 0;

  if ((this->Size[0] != x) || (this->Size[1] != y))
    {
    this->Modified();
    this->Size[0] = x;
    this->Size[1] = y;
    if (this->Mapped)
      {
      if (!resizing)
        {
        resizing = 1;
        SizeWindow(this->WindowId, x, y, TRUE);
        resizing = 0;
        }
      }
    }
}

// --------------------------------------------------------------------------
void vtkCarbonRenderWindow::SetPosition(int x, int y)
{
  static int resizing = 0;

  if ((this->Position[0] != x) || (this->Position[1] != y))
    {
    this->Modified();
    this->Position[0] = x;
    this->Position[1] = y;
    if (this->Mapped)
      {
      if (!resizing)
        {
        resizing = 1;
        MoveWindow(this->WindowId, x, y, FALSE);
        resizing = 0;
        }
      }
    }
}


//--------------------------------------------------------------------------
// End the rendering process and display the image.
void vtkCarbonRenderWindow::Frame(void)
{
  if (!this->AbortRender && this->DoubleBuffer)
    {
    aglSwapBuffers(this->ContextId);
    vtkDebugMacro(<< " SwapBuffers\n");
    }
}

//--------------------------------------------------------------------------
// Update system if needed due to stereo rendering.
void vtkCarbonRenderWindow::StereoUpdate(void)
{
  // if stereo is on and it wasn't before
  if (this->StereoRender && (!this->StereoStatus))
    {
    switch (this->StereoType)
      {
      case VTK_STEREO_CRYSTAL_EYES:
        {
        this->StereoStatus = 1;
        }
        break;
      case VTK_STEREO_RED_BLUE:
        {
        this->StereoStatus = 1;
        }
      }
    }
  else if ((!this->StereoRender) && this->StereoStatus)
    {
    switch (this->StereoType)
      {
      case VTK_STEREO_CRYSTAL_EYES:
        {
        this->StereoStatus = 0;
        }
        break;
      case VTK_STEREO_RED_BLUE:
        {
        this->StereoStatus = 0;
        }
      }
    }
}

//--------------------------------------------------------------------------
// Specify various window parameters.
void vtkCarbonRenderWindow::WindowConfigure()
{
  // this is all handled by the desiredVisualInfo method
}

//--------------------------------------------------------------------------
void vtkCarbonRenderWindow::SetupPixelFormat(void *hDC, void *dwFlags,
                                             int debug, int bpp,
                                             int zbpp)
{
  cout << "vtkCarbonRenderWindow::SetupPixelFormat - IMPLEMENT\n";
}

//--------------------------------------------------------------------------
void vtkCarbonRenderWindow::SetupPalette(void *hDC)
{
  cout << "vtkCarbonRenderWindow::SetupPalette - IMPLEMENT\n";
}

//--------------------------------------------------------------------------
// Initialize the window for rendering.
void vtkCarbonRenderWindow::WindowInitialize (void)
{
  int x, y, width, height;
  GDHandle hGD = NULL;
  GLint depthSizeSupport;
  OSStatus err = noErr;
  static int count = 1;
  short i;
  char *windowName;
  short numDevices;     // number of graphics devices our window covers
  WindowAttributes windowAttrs = (kWindowStandardDocumentAttributes | 
                                  kWindowLiveResizeAttribute |
                                  kWindowStandardHandlerAttribute);

  // create our own window if not already set
  this->OwnWindow = 0;
  // Initialize Toolbox Managers if not done already
  if (!this->ApplicationInitialized)
    {
    InitCursor();
    DrawMenuBar();
    this->ApplicationInitialized=1;
    }
  if ((this->Size[0]+this->Size[1])==0)
    {
    this->Size[0]=300;
    this->Size[1]=300;
    }
  if ((this->Position[0]+this->Position[1])==0)
    {
    this->Position[0]=50;
    this->Position[1]=50;
    }

  // Rect is defined as {top, left, bottom, right} (really)
  Rect rectWin = {this->Position[1], this->Position[0],
                  this->Position[1]+this->Size[1],
                  this->Position[0]+this->Size[0]};
  
  if (!this->WindowId)
    {
    if (noErr != CreateNewWindow (kDocumentWindowClass, windowAttrs,
                                  &rectWin, &(this->WindowId)))
      {
      vtkErrorMacro("Could not create window, serious error!");
      return;
      }
    int len = strlen("vtkX - Carbon #")
      + (int)ceil( (double) log10( (double)(count+1) ) )
      + 1;
    windowName = new char [ len ];
    sprintf(windowName,"vtkX - Carbon #%i",count++);
    this->SetWindowName(windowName);
    delete [] windowName;
    this->OwnWindow = 1;
    SetWRefCon(this->WindowId, (long)this);
    ShowWindow(this->WindowId);
    SetPortWindowPort(this->WindowId);
    this->fAcceleratedMust = false;  //must renderer be accelerated?
    this->VRAM = 0 * 1048576;    // minimum VRAM
    this->textureRAM = 0 * 1048576;  // minimum texture RAM
    this->fmt = 0;      // output pixel format
    i = 0;
    this->aglAttributes [i++] = AGL_RGBA;
    this->aglAttributes [i++] = AGL_DOUBLEBUFFER;
    this->aglAttributes [i++] = AGL_ACCELERATED;
    this->aglAttributes [i++] = AGL_NO_RECOVERY;
    this->aglAttributes [i++] = AGL_DEPTH_SIZE;
    this->aglAttributes [i++] = 16;
    this->aglAttributes [i++] = AGL_NONE;
    this->draggable = true;

    numDevices = FindGDHandleFromWindow(this->WindowId, &hGD);
    if (!this->draggable)
      {
      if ((numDevices > 1) || (numDevices == 0)) // multiple or no devices
        {
        // software renderer
        // infinite VRAM, infinite textureRAM, not accelerated
        if (this->fAcceleratedMust)
          {
          vtkErrorMacro ("Window spans multiple devices-no HW accel");
          return;
          }
        }
      else // not draggable on single device
        {
        if (!CheckRenderer (hGD, &(this->VRAM), &(this->textureRAM),
                            &depthSizeSupport, this->fAcceleratedMust))
          {
          vtkErrorMacro ("Renderer check failed");
          return;
          }
        }
    }
    // else if draggable - must check all devices for presence of
    // at least one renderer that meets the requirements
    else if(!CheckAllDeviceRenderers(&(this->VRAM), &(this->textureRAM),
                                     &depthSizeSupport, this->fAcceleratedMust))
      {
      vtkErrorMacro ("Renderer check failed");
      return;
      }

    // do agl
    if ((Ptr) kUnresolvedCFragSymbolAddress == (Ptr) aglChoosePixelFormat)
      {
      vtkErrorMacro ("OpenGL not installed");
      return;
      }
    // we successfully passed the renderer checks!

    if ((!this->draggable && (numDevices == 1)))
      // not draggable on a single device
      this->fmt = aglChoosePixelFormat (&hGD, 1, this->aglAttributes);
    else
      this->fmt = aglChoosePixelFormat (NULL, 0, this->aglAttributes);
    aglReportError (); // cough up any errors encountered
    if (NULL == this->fmt)
      {
      vtkErrorMacro("Could not find valid pixel format");
      return;
      }

    //this->ContextId = aglCreateContext (this->fmt, aglShareContext);
    // Create AGL context
    //if (AGL_BAD_MATCH == aglGetError())
    this->ContextId = aglCreateContext (this->fmt, 0); // create without sharing
    aglReportError (); // cough up errors
    if (NULL == this->ContextId)
      {
      vtkErrorMacro ("Could not create context");
      return;
      }
    // attach the CGrafPtr to the context
    if (!aglSetDrawable (this->ContextId, GetWindowPort (this->WindowId)))
      {
      aglReportError();
      return;
      }

    if(!aglSetCurrentContext (this->ContextId))
      // make the context the current context
      {
      aglReportError();
      return;
      }
    }
  this->OpenGLInit();
  this->Mapped = 1;
}

//--------------------------------------------------------------------------
// Initialize the rendering window.
void vtkCarbonRenderWindow::Initialize (void)
{
  // make sure we havent already been initialized
  if (this->ContextId)
    {
    return;
    }
  this->WindowInitialize();
}

//--------------------------------------------------------------------------
void vtkCarbonRenderWindow::UpdateSizeAndPosition(int xPos, int yPos, 
                                                  int xSize, int ySize)
{
  this->Size[0]=xSize;
  this->Size[1]=ySize;
  this->Position[0]=xPos;
  this->Position[1]=yPos;
  this->Modified();
}

//--------------------------------------------------------------------------
// Get the current size of the window.
int *vtkCarbonRenderWindow::GetSize(void)
{
  // if we aren't mapped then just return the ivar
  if (!this->Mapped)
    {
    return this->Size;
    }

  Rect windowRect;
  GetWindowBounds(this->WindowId, kWindowContentRgn, &windowRect);
  //  Find the current window size
  this->Size[0] = (int) windowRect.right-windowRect.left;
  this->Size[1] = (int) windowRect.bottom-windowRect.top;
  return this->Size;
}

//--------------------------------------------------------------------------
// Get the current size of the screen.
int *vtkCarbonRenderWindow::GetScreenSize(void)
{
  cout << "Inside vtkCarbonRenderWindow::GetScreenSize - MUST IMPLEMENT\n";
  this->Size[0] = 0;
  this->Size[1] = 0;

  return this->Size;
}

//--------------------------------------------------------------------------
// Get the position in screen coordinates of the window.
int *vtkCarbonRenderWindow::GetPosition(void)
{
  // if we aren't mapped then just return the ivar
  if (!this->Mapped)
    {
    return(this->Position);
    }

  //  Find the current window position
  Rect windowRect;
  GetWindowBounds(this->WindowId, kWindowContentRgn, &windowRect);
  this->Position[0] = windowRect.left;
  this->Position[1] = windowRect.top;
  return this->Position;
}

//--------------------------------------------------------------------------
// Change the window to fill the entire screen.
void vtkCarbonRenderWindow::SetFullScreen(int arg)
{
  int *temp;

  if (this->FullScreen == arg)
    {
    return;
    }

  if (!this->Mapped)
    {
    this->PrefFullScreen();
    return;
    }

  // set the mode
  this->FullScreen = arg;
  if (this->FullScreen <= 0)
    {
    this->Position[0] = this->OldScreen[0];
    this->Position[1] = this->OldScreen[1];
    this->Size[0] = this->OldScreen[2];
    this->Size[1] = this->OldScreen[3];
    this->Borders = this->OldScreen[4];
    }
  else
    {
    // if window already up get its values
    if (this->WindowId)
      {
      temp = this->GetPosition();
      this->OldScreen[0] = temp[0];
      this->OldScreen[1] = temp[1];

      this->OldScreen[4] = this->Borders;
      this->PrefFullScreen();
      }
    }

  // remap the window
  this->WindowRemap();

  this->Modified();
}

//--------------------------------------------------------------------------
// Set the variable that indicates that we want a stereo capable window
// be created. This method can only be called before a window is realized.
void vtkCarbonRenderWindow::SetStereoCapableWindow(int capable)
{
  if (this->WindowId == 0)
    {
    vtkRenderWindow::SetStereoCapableWindow(capable);
    }
  else
    {
    vtkWarningMacro(<< "Requesting a StereoCapableWindow must be performed "
                    << "before the window is realized, i.e. before a render.");
    }
}

//--------------------------------------------------------------------------
// Set the preferred window size to full screen.
void vtkCarbonRenderWindow::PrefFullScreen()
{
  vtkWarningMacro(<< "Can't get full screen window.");
}

//--------------------------------------------------------------------------
// Remap the window.
void vtkCarbonRenderWindow::WindowRemap()
{
  vtkWarningMacro(<< "Can't remap the window.");
}

//--------------------------------------------------------------------------
void vtkCarbonRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ContextId: " << this->ContextId << "\n";
  os << indent << "MultiSamples: " << this->MultiSamples << "\n";
}

//--------------------------------------------------------------------------
int vtkCarbonRenderWindow::GetDepthBufferSize()
{
  GLint size;

  if ( this->Mapped )
    {
    size = 0;
    glGetIntegerv( GL_DEPTH_BITS, &size );
    return (int) size;
    }
  else
    {
    vtkDebugMacro(<< "Window is not mapped yet!" );
    return 24;
    }
}

//--------------------------------------------------------------------------
// Get the window id.
WindowPtr vtkCarbonRenderWindow::GetWindowId()
{
  vtkDebugMacro(<< "Returning WindowId of " << this->WindowId << "\n");
  return this->WindowId;
}

//--------------------------------------------------------------------------
// Set the window id to a pre-existing window.
void vtkCarbonRenderWindow::SetWindowId(WindowPtr theWindow)
{
  vtkDebugMacro(<< "Setting WindowId to " << theWindow << "\n");
  this->WindowId = theWindow;
}

//--------------------------------------------------------------------------
// Set this RenderWindow's Carbon window id to a pre-existing window.
void vtkCarbonRenderWindow::SetWindowInfo(void *windowID)
{
  this->WindowId = (WindowPtr)windowID;
  vtkDebugMacro(<< "Setting WindowId to " << this->WindowId << "\n");
}


void vtkCarbonRenderWindow::SetContextId(void *arg)
{
  this->ContextId = (AGLContext)arg;
}

//----------------------------------------------------------------------------
void vtkCarbonRenderWindow::HideCursor()
{
  if (this->CursorHidden)
    {
    return;
    }
  this->CursorHidden = 1;
  HideCursor();
}

//----------------------------------------------------------------------------
void vtkCarbonRenderWindow::ShowCursor()
{
  if (!this->CursorHidden)
    {
    return;
    }
  this->CursorHidden = 0;
  ShowCursor();
}

