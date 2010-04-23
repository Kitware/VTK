/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWinCEOpenGLRenderWindow.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include "vtkWinCEOpenGLRenderWindow.h"
#include "vtkWin32RenderWindowInteractor.h"
#include "vtkOpenGL.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLProperty.h"
#include "vtkOpenGLTexture.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLLight.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkWinCEOpenGLRenderWindow);

#define VTK_MAX_LIGHTS 8

// a couple of routines for offscreen rendering
void vtkOSMesaDestroyWindow(void *Window) 
{
  free(Window);
}

void *vtkOSMesaCreateWindow(int width, int height) 
{
  return malloc(width*height*4);
}

vtkWinCEOpenGLRenderWindow::vtkWinCEOpenGLRenderWindow()
{
  this->OffScreenContextId = NULL;
  this->OffScreenWindow = NULL;
  this->ApplicationInstance =  NULL;
  this->WindowId = 0;
  this->ParentId = 0;
  this->NextWindowId = 0;
  this->DeviceContext = (HDC)0;         // hsr
  this->StereoType = VTK_STEREO_CRYSTAL_EYES;  
  this->CursorHidden = 0;
  this->ForceMakeCurrent = 0;
}

vtkWinCEOpenGLRenderWindow::~vtkWinCEOpenGLRenderWindow()
{
  this->Finalize();
}

void vtkWinCEOpenGLRenderWindow::Clean()
{
  vtkRenderer *ren;
  GLuint id;
  
  /* finish OpenGL rendering */
  if (this->OffScreenContextId) 
    {
    this->MakeCurrent();

    /* first delete all the old lights */
    for (short cur_light = GL_LIGHT0; cur_light < GL_LIGHT0+VTK_MAX_LIGHTS; cur_light++)
      {
      glDisable((GLenum)cur_light);
      }

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
    for ( ren = (vtkOpenGLRenderer *) this->Renderers->GetNextItemAsObject();
          ren != NULL;
          ren = (vtkOpenGLRenderer *) this->Renderers->GetNextItemAsObject() )
      {
      ren->SetRenderWindow(NULL);
      }
    
    OSMesaDestroyContext(this->OffScreenContextId);
    this->OffScreenContextId = NULL;
    vtkOSMesaDestroyWindow(this->OffScreenWindow);
    this->OffScreenWindow = NULL;
    }
}

LRESULT APIENTRY vtkWinCEOpenGLRenderWindow::WndProc(HWND hWnd, UINT message, 
                                                     WPARAM wParam, 
                                                     LPARAM lParam)
{
  vtkWinCEOpenGLRenderWindow *me = 
    (vtkWinCEOpenGLRenderWindow *)vtkGetWindowLong(hWnd,sizeof(vtkLONG));

  // forward to actual object
  if (me)
    {
    return me->MessageProc(hWnd, message, wParam, lParam);
    }

  return DefWindowProc(hWnd, message, wParam, lParam);
}

void vtkWinCEOpenGLRenderWindow::SetWindowName( char * _arg )
{
  vtkWindow::SetWindowName(_arg);
  if (this->WindowId)
    {
//    SetWindowText(this->WindowId,this->WindowName);
    }
}

int vtkWinCEOpenGLRenderWindow::GetEventPending()
{
  MSG msg;
  
  return PeekMessage(&msg,this->WindowId,WM_LBUTTONDOWN,WM_MBUTTONDOWN,PM_NOREMOVE);
}

void vtkWinCEOpenGLRenderWindow::MakeCurrent()
{
  if (this->OffScreenContextId || this->ForceMakeCurrent) 
    {
      if (OSMesaMakeCurrent(this->OffScreenContextId, 
                            this->OffScreenWindow, GL_UNSIGNED_BYTE, 
                            this->Size[0], this->Size[1]) != GL_TRUE) 
        {
         vtkWarningMacro("failed call to OSMesaMakeCurrent");
        }
      this->ForceMakeCurrent = 0;
    }
}

void vtkWinCEOpenGLRenderWindow::SetForceMakeCurrent()
{
  this->ForceMakeCurrent = 1;
}

void vtkWinCEOpenGLRenderWindow::SetSize(int x, int y)
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
           
           if (this->ParentId)
             {
              SetWindowPos(this->WindowId,HWND_TOP,0,0,
                          x, y, SWP_NOMOVE | SWP_NOZORDER);
             }
           else
             {
              SetWindowPos(this->WindowId,HWND_TOP,0,0,
                          x, y,
                          SWP_NOMOVE | SWP_NOZORDER);
             }
           OSMesaDestroyContext(this->OffScreenContextId);
           this->OffScreenContextId = NULL;
           vtkOSMesaDestroyWindow(this->OffScreenWindow);
           this->OffScreenWindow = vtkOSMesaCreateWindow(x,y);
           this->OwnWindow = 1;
           this->OffScreenContextId = OSMesaCreateContext(GL_RGBA, NULL);
           this->MakeCurrent();
           this->OpenGLInit();
           this->Mapped = 1;
           resizing = 0;
         }
      }
    }
}

void vtkWinCEOpenGLRenderWindow::SetPosition(int x, int y)
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
   
        SetWindowPos(this->WindowId,HWND_TOP,x,y,
            0, 0, SWP_NOSIZE | SWP_NOZORDER);
        resizing = 0;
        }
      }
    }
}


// End the rendering process and display the image.
void vtkWinCEOpenGLRenderWindow::Frame(void)
{
  this->MakeCurrent();
  if (!this->AbortRender && this->DoubleBuffer && this->SwapBuffers)
    {
      BITMAPINFO MemoryDataHeader;
      unsigned char *MemoryData;    // the data in the DIBSection
      HDC MemoryHdc;
      
      int dataWidth = ((this->Size[0]*3+3)/4)*4;
      
      MemoryDataHeader.bmiHeader.biSize = 40;
      MemoryDataHeader.bmiHeader.biWidth = this->Size[0];
      MemoryDataHeader.bmiHeader.biHeight = this->Size[1];
      MemoryDataHeader.bmiHeader.biPlanes = 1;
      MemoryDataHeader.bmiHeader.biBitCount = 24;
      MemoryDataHeader.bmiHeader.biCompression = BI_RGB;
      MemoryDataHeader.bmiHeader.biClrUsed = 0;
      MemoryDataHeader.bmiHeader.biClrImportant = 0;
      MemoryDataHeader.bmiHeader.biSizeImage = dataWidth*this->Size[1];
      
      HBITMAP dib = CreateDIBSection(this->DeviceContext,
                                 &MemoryDataHeader, DIB_RGB_COLORS,
                                 (void **)(&(MemoryData)),  NULL, 0);
      
      // copy the data form mesa to dib
      int x, y;
      for (y = 0; y < this->Size[1]; ++y)
       {
         int ytot = y*this->Size[0];
         for (x = 0; x < this->Size[0]; ++x)
           {
             MemoryData[(x+ytot)*3] = ((unsigned char *)this->OffScreenWindow)[(x+ytot)*4+2];
             MemoryData[(x+ytot)*3 + 1] = ((unsigned char *)this->OffScreenWindow)[(x+ytot)*4+1];
             MemoryData[(x+ytot)*3 + 2] = ((unsigned char *)this->OffScreenWindow)[(x+ytot)*4];
           }
       }
      // Create a compatible device context
      MemoryHdc = (HDC)CreateCompatibleDC(this->DeviceContext);
      
      // Put the bitmap into the device context
      SelectObject(MemoryHdc, dib);
      BitBlt(this->DeviceContext, 0, 0, this->Size[0], this->Size[1],
            MemoryHdc, 0, 0,SRCCOPY);
      vtkDebugMacro(<< " SwapBuffers\n");
    }
  else
    {
    glFlush();
    }
}


LRESULT vtkWinCEOpenGLRenderWindow::MessageProc(HWND hWnd, UINT message, 
                                                WPARAM wParam, LPARAM lParam)
{
  switch (message) 
    {
    case WM_CREATE:
      {
      // nothing to be done here, opengl is initilized after the call to
      // create now
      return 0;
      }
    case WM_DESTROY:
      this->Clean();
      if (this->DeviceContext)
        {
        ReleaseDC(this->WindowId, this->DeviceContext);
        this->DeviceContext = NULL;
        this->WindowId = NULL;
        }
      return 0;
    case WM_SIZE:
        /* track window size changes */
        if (this->OffScreenContextId) 
          {
          this->SetSize((int) LOWORD(lParam),(int) HIWORD(lParam));
          return 0;
          }
    case WM_PAINT:
        {
        PAINTSTRUCT ps;
        BeginPaint(hWnd, &ps);
        if (this->OffScreenContextId) 
          {
          this->Render();
          }
        EndPaint(hWnd, &ps);
        return 0;
        }
        break;
    case WM_ERASEBKGND:
      return TRUE;
    default:
        break;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}


void vtkWinCEOpenGLRenderWindow::InitializeApplication()
{
  // get the applicaiton instance if we don't have one already
  if (!this->ApplicationInstance)
    {
      // if we have a parent window get the app instance from it
      this->ApplicationInstance = GetModuleHandle(NULL); /*AfxGetInstanceHandle();*/
    }
}

void vtkWinCEOpenGLRenderWindow::CreateAWindow(int x, int y, int width,
                                              int height)
{
  static int count=1;
  char *windowName;

  if (!this->WindowId)
    {
    WNDCLASS wndClass;
      
    int len = strlen( "Visualization Toolkit - WinCEOpenGL #") 
      + (int)ceil( (double) log10( (double)(count+1) ) )
      + 1; 
    windowName = new char [ len ];
    sprintf(windowName,"Visualization Toolkit - WinCEOpenGL #%i",count++);
    this->SetWindowName(windowName);
    delete [] windowName;
      
    // has the class been registered ?
    if (!GetClassInfo(this->ApplicationInstance,L"vtkOpenGL",&wndClass))
      {
      wndClass.style = CS_HREDRAW | CS_VREDRAW;
      wndClass.lpfnWndProc = vtkWinCEOpenGLRenderWindow::WndProc;
      wndClass.cbClsExtra = 0;
         wndClass.hIcon = NULL;
      wndClass.hInstance = this->ApplicationInstance;
      wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
      wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
      wndClass.lpszMenuName = NULL;
      wndClass.lpszClassName = L"vtkOpenGL";
      // vtk doesn't use the first half of the extra bytes, but app writers
      // may want them, so we provide them. VTK does use the second 
      // half of the extra space.
      wndClass.cbWndExtra = 2 * sizeof(vtkLONG);
      ATOM res = RegisterClass(&wndClass);
      if (!res)
       {
         DWORD lerr = GetLastError();
         *(float *)(0x01) = 1.0;
       }
      }
      
    /* create window */
    if (this->ParentId)
      {
      this->WindowId = CreateWindow(
        L"vtkOpenGL", L"WinCE",
        WS_CHILD | WS_CLIPCHILDREN /*| WS_CLIPSIBLINGS*/,
        x, y, width, height,
        this->ParentId, NULL, this->ApplicationInstance, NULL);
      }
    else
      {
      DWORD style;
      if (this->Borders)
        {
        style = WS_CLIPCHILDREN /*| WS_CLIPSIBLINGS*/;
        }
      else
        {
        style = WS_POPUP | WS_CLIPCHILDREN /*| WS_CLIPSIBLINGS*/;
        }
      this->WindowId = CreateWindow(
        L"vtkOpenGL", L"WinCE", style,
        x,y, width, height,
        NULL, NULL, this->ApplicationInstance, NULL);
      }
    if (!this->WindowId)
      {
      DWORD lerr = GetLastError();
      vtkErrorMacro("Could not create window, error:  " << GetLastError());
      
      *(float *)(0x01) = 1.0;
      return;
      }
    // extract the create info
      
    /* display window */
    ShowWindow(this->WindowId, SW_SHOW);
    //UpdateWindow(this->WindowId);
    this->OwnWindow = 1;
    vtkSetWindowLong(this->WindowId,sizeof(vtkLONG),(vtkLONG)this);
    }
  this->DeviceContext = GetDC(this->WindowId);
  if (!this->OffScreenWindow)
    {
      this->OffScreenWindow = vtkOSMesaCreateWindow(width,height);
      this->Size[0] = width;
      this->Size[1] = height;      
      this->OwnWindow = 1;
    }    
  this->OffScreenContextId = OSMesaCreateContext(GL_RGBA, NULL);
  
  this->MakeCurrent();
  this->OpenGLInit();
  this->Mapped = 1;
}

// Initialize the window for rendering.
void vtkWinCEOpenGLRenderWindow::WindowInitialize()
{
  int x, y, width, height;
  x = ((this->Position[0] >= 0) ? this->Position[0] : 5);
  y = ((this->Position[1] >= 0) ? this->Position[1] : 5);
  height = ((this->Size[1] > 0) ? this->Size[1] : 300);
  width = ((this->Size[0] > 0) ? this->Size[0] : 300);

  
  // create our own window if not already set
  this->OwnWindow = 0;
  this->InitializeApplication();
  this->CreateAWindow(x,y,width,height);
  
  // set the DPI
  this->SetDPI(GetDeviceCaps(this->DeviceContext, LOGPIXELSY));
}

// Initialize the rendering window.
void vtkWinCEOpenGLRenderWindow::Initialize (void)
{
  // make sure we havent already been initialized 
  if (this->OffScreenContextId)
    {
    return;
    }

  // now initialize the window
  this->WindowInitialize();
}

void vtkWinCEOpenGLRenderWindow::Finalize (void)
{
  if (this->CursorHidden)
    {
    this->ShowCursor();
    }

  if (this->WindowId && this->OwnWindow)
    {
    this->Clean();
    ReleaseDC(this->WindowId, this->DeviceContext);
    // can't set WindowId=NULL, needed for DestroyWindow
    this->DeviceContext = NULL;
    
    // clear the extra data before calling destroy
    vtkSetWindowLong(this->WindowId,sizeof(vtkLONG),(vtkLONG)0);
    DestroyWindow(this->WindowId);
    }
}


// Get the current size of the window.
int *vtkWinCEOpenGLRenderWindow::GetSize(void)
{
  RECT rect;

  // if we aren't mapped then just return the ivar 
  if (!this->Mapped)
    {
    return this->Size;
    }

  //  Find the current window size 
  GetClientRect(this->WindowId, &rect);

  this->Size[0] = rect.right;
  this->Size[1] = rect.bottom;
  
  return this->Size;
}

// Get the current size of the window.
int *vtkWinCEOpenGLRenderWindow::GetScreenSize(void)
{
  RECT rect;

  SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
                            
  this->Size[0] = rect.right - rect.left;
  this->Size[1] = rect.bottom - rect.top;
  
  return this->Size;
}

// Get the position in screen coordinates of the window.
int *vtkWinCEOpenGLRenderWindow::GetPosition(void)
{
  // if we aren't mapped then just return the ivar 
  if (!this->Mapped)
    {
    return this->Position;
    }

  //  Find the current window position 
//  x,y,&this->Position[0],&this->Position[1],&child);

  return this->Position;
}

// Change the window to fill the entire screen.
void vtkWinCEOpenGLRenderWindow::SetFullScreen(int arg)
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

//
// Set the variable that indicates that we want a stereo capable window
// be created. This method can only be called before a window is realized.
//
void vtkWinCEOpenGLRenderWindow::SetStereoCapableWindow(int capable)
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


// Set the preferred window size to full screen.
void vtkWinCEOpenGLRenderWindow::PrefFullScreen()
{
  int *size;

  size = this->GetScreenSize();

  // use full screen 
  this->Position[0] = 0;
  this->Position[1] = 0;
  this->Size[0] = size[0];
  this->Size[1] = size[1];
  // don't show borders 
  this->Borders = 0;
}

// Remap the window.
void vtkWinCEOpenGLRenderWindow::WindowRemap()
{
  this->Finalize();

  // set the default windowid 
  this->WindowId = this->NextWindowId;
  this->NextWindowId = 0;

  // configure the window 
  this->Initialize();
}

void vtkWinCEOpenGLRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Next Window Id: " << this->NextWindowId << "\n";
  os << indent << "Window Id: " << this->WindowId << "\n";
}

// Get the window id.
HWND vtkWinCEOpenGLRenderWindow::GetWindowId()
{
  vtkDebugMacro(<< "Returning WindowId of " << this->WindowId << "\n"); 

  return this->WindowId;
}

// Set the window id to a pre-existing window.
void vtkWinCEOpenGLRenderWindow::SetWindowId(HWND arg)
{
  vtkDebugMacro(<< "Setting WindowId to " << arg << "\n"); 

  this->WindowId = arg;
}

// Set this RenderWindow's X window id to a pre-existing window.
void vtkWinCEOpenGLRenderWindow::SetWindowInfo(char *info)
{
  int tmp;
  
  sscanf(info,"%i",&tmp);
 
  this->WindowId = (HWND)tmp;
  vtkDebugMacro(<< "Setting WindowId to " << this->WindowId << "\n"); 
}

void vtkWinCEOpenGLRenderWindow::SetNextWindowInfo(char *info)
{
  int tmp;
  
  sscanf(info,"%i",&tmp);
 
  this->SetNextWindowId((HWND)tmp);
}

// Sets the HWND id of the window that WILL BE created.
void vtkWinCEOpenGLRenderWindow::SetParentInfo(char *info)
{
  int tmp;
  
  sscanf(info,"%i",&tmp);
 
  this->ParentId = (HWND)tmp;
  vtkDebugMacro(<< "Setting ParentId to " << this->ParentId << "\n"); 
}

// Set the window id to a pre-existing window.
void vtkWinCEOpenGLRenderWindow::SetParentId(HWND arg)
{
  vtkDebugMacro(<< "Setting ParentId to " << arg << "\n"); 

  this->ParentId = arg;
}

// Set the window id of the new window once a WindowRemap is done.
void vtkWinCEOpenGLRenderWindow::SetNextWindowId(HWND arg)
{
  vtkDebugMacro(<< "Setting NextWindowId to " << arg << "\n"); 

  this->NextWindowId = arg;
}

void vtkWinCEOpenGLRenderWindow::SetNextWindowId(void *arg)
{
   this->SetNextWindowId((HWND)arg);
}


// Begin the rendering process.
void vtkWinCEOpenGLRenderWindow::Start(void)
{
  // if the renderer has not been initialized, do so now
  if (!this->OffScreenContextId)
    {
    this->Initialize();
    }

  // set the current window 
  this->MakeCurrent();
}


//----------------------------------------------------------------------------
void vtkWinCEOpenGLRenderWindow::HideCursor()
{
  if (this->CursorHidden)
    {
    return;
    }
  this->CursorHidden = 1;

  ::ShowCursor(!this->CursorHidden);
}

//----------------------------------------------------------------------------
void vtkWinCEOpenGLRenderWindow::ShowCursor()
{
  if (!this->CursorHidden)
    {
    return;
    }
  this->CursorHidden = 0;

  ::ShowCursor(!this->CursorHidden);
}                                  

