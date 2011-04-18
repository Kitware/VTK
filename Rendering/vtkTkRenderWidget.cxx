/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTkRenderWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTkRenderWidget.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkImageData.h"
#include "vtkTclUtil.h"
#include "vtkTkInternals.h"
#include "vtkToolkits.h"

#ifdef _WIN32
#include "vtkWin32OpenGLRenderWindow.h"
#else
#if defined(VTK_USE_CARBON) || defined(VTK_USE_COCOA)
#ifdef VTK_USE_CARBON
#include "vtkCarbonRenderWindow.h"
#include "tkMacOSXInt.h"// Needed for XEvent.type == UnmapNotify
#else
#include "vtkCocoaRenderWindow.h"
#include "vtkCocoaTkUtilities.h"
#ifndef MAC_OSX_TK
#define MAC_OSX_TK 1
#endif
#include "tkInt.h"
#endif
#else
#include "vtkXOpenGLRenderWindow.h"
#endif
#endif

#include <stdlib.h>

// Silent warning like
// "dereferencing type-punned pointer will break strict-aliasing rules"
// it happens because this kind of expression: (long *)&ptr
// pragma GCC diagnostic is available since gcc>=4.2
#if defined(__GNUG__) && (__GNUC__>4) || (__GNUC__==4 && __GNUC_MINOR__>=2)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

#define VTK_ALL_EVENTS_MASK \
    KeyPressMask|KeyReleaseMask|ButtonPressMask|ButtonReleaseMask|      \
    EnterWindowMask|LeaveWindowMask|PointerMotionMask|ExposureMask|     \
    VisibilityChangeMask|FocusChangeMask|PropertyChangeMask|ColormapChangeMask

#define VTK_MAX(a,b)    (((a)>(b))?(a):(b))

// These are the options that can be set when the widget is created
// or with the command configure.  The only new one is "-rw" which allows
// the uses to set their own render window.
static Tk_ConfigSpec vtkTkRenderWidgetConfigSpecs[] = {
    {TK_CONFIG_PIXELS, (char *) "-height", (char *) "height", (char *) "Height",
     (char *) "400", Tk_Offset(struct vtkTkRenderWidget, Height), 0, NULL},
  
    {TK_CONFIG_PIXELS, (char *) "-width", (char *) "width", (char *) "Width",
     (char *) "400", Tk_Offset(struct vtkTkRenderWidget, Width), 0, NULL},
  
    {TK_CONFIG_STRING, (char *) "-rw", (char *) "rw", (char *) "RW",
     (char *) "", Tk_Offset(struct vtkTkRenderWidget, RW), 0, NULL},

    {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
     (char *) NULL, 0, 0, NULL}
};


// Foward prototypes
extern "C"
{
  void vtkTkRenderWidget_EventProc(ClientData clientData, 
                                   XEvent *eventPtr);
}

static int vtkTkRenderWidget_MakeRenderWindow(struct vtkTkRenderWidget *self);
extern int vtkRenderWindowCommand(ClientData cd, Tcl_Interp *interp,
                                  int argc, char *argv[]);

// Start of vtkImageDataToTkPhoto
template <class T>
void vtkExtractImageData ( unsigned char* buffer, T *inPtr, double shift, double scale, int width, int height, int pitch, int pixelSize, int components )
{
  T* ImagePtr;
  unsigned char* BufferPtr;
  int i, j, c;
  float pixel;
  
  BufferPtr = buffer;
  //ImagePtr = inPtr;
  
  for ( j = 0; j < height; j++ )
    {
    ImagePtr = j * pitch + inPtr;
    for ( i = 0; i < width; i++ )
      {
      for ( c = 0; c < components; c++ )
        {
        // Clamp
        pixel = (*ImagePtr + shift) * scale;
        if ( pixel < 0 )
          {
          pixel = 0;
          }
        else
          {
          if ( pixel > 255 )
            {
            pixel = 255;
            }
          }
        *BufferPtr = (unsigned char) pixel;
        ImagePtr++;
        BufferPtr++;
        }
      ImagePtr += pixelSize - components;
      }
    }
  return;
}


extern "C" {
#define VTKIMAGEDATATOTKPHOTO_CORONAL 0
#define VTKIMAGEDATATOTKPHOTO_SAGITTAL 1
#define VTKIMAGEDATATOTKPHOTO_TRANSVERSE 2
  int vtkImageDataToTkPhoto_Cmd (ClientData vtkNotUsed(clientData), 
                                 Tcl_Interp *interp, 
                                 int argc, 
#if (TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION >= 4)
                                 CONST84
#endif
                                 char **argv)
  {
    int status = 0;
    vtkImageData* image;
    Tk_PhotoHandle photo;
    int slice = 0;
    double window = 256.0;
    double level = window / 2.0;
    int orientation = VTKIMAGEDATATOTKPHOTO_TRANSVERSE;
    
    // Usage: vtkImageDataToTkPhoto vtkImageData photo slice
    if ( argc < 4 || argc > 7 )
      {
      const char m[] = 
        "wrong # args: should be \"vtkImageDataToTkPhoto vtkImageData photo slice [orientation] [window] [level]\"";

      Tcl_SetResult ( interp, const_cast<char*>(m), TCL_VOLATILE );
      return TCL_ERROR;
      }

    // Start with slice, it's fast, etc...
    status = Tcl_GetInt ( interp, argv[3], &slice );
    if ( status != TCL_OK )
      {
      return status;
      }

    // Find the image
#ifdef VTK_PYTHON_BUILD
    char typeCheck[256];
#if VTK_SIZEOF_VOID_P == VTK_SIZEOF_LONG
    union { void *p; unsigned long l; } u;
    sscanf ( argv[1], "_%lx_%s", &u.l, typeCheck);
#elif defined(VTK_TYPE_USE_LONG_LONG)
    union { void *p; unsigned long long l; } u;
    sscanf ( argv[1], "_%llx_%s", &u.l, typeCheck);
#elif defined(VTK_TYPE_USE___INT64)
    union { void *p; unsigned __int64 l; } u;
    sscanf ( argv[1], "_%I64x_%s", &u.l, typeCheck);
#endif
    // Various historical pointer manglings
    if ( strcmp ( "vtkImageData", typeCheck ) != 0 &&
         strcmp ( "vtkImageData_p", typeCheck ) != 0 &&
         strcmp ( "p_vtkImageData", typeCheck ) != 0 &&
         strcmp ( "vtkStructuredPoints", typeCheck ) != 0 &&
         strcmp ( "vtkStructuredPoints_p", typeCheck ) != 0 &&
         strcmp ( "p_vtkStructuredPoints", typeCheck ) != 0 )
      {
      // bad type
      u.p = NULL;
      }
    image = static_cast<vtkImageData*>(u.p);
#else
    image = static_cast<vtkImageData*>(
      vtkTclGetPointerFromObject ( argv[1], "vtkImageData", interp, status ));
#endif
    if ( !image )
      {
      Tcl_AppendResult ( interp, "could not find vtkImageData: ", argv[1], NULL );
      return TCL_ERROR;
      }

    // Find the photo widget
    photo = Tk_FindPhoto ( interp, argv[2] );
    if ( !photo )
      {
      Tcl_AppendResult ( interp, "could not find photo: ", argv[2], NULL );
      return TCL_ERROR;
      }

    int components = image->GetNumberOfScalarComponents();
    if ( components != 1 && components != 3 )
      {
      const char* m = "number of scalar components must be 1, 3, 4";
      Tcl_SetResult ( interp, const_cast<char*>(m), TCL_VOLATILE );
      return TCL_ERROR;
      }

    // Determine the orientation
    if ( argc >= 5 )
      {
      if ( strcmp ( argv[4], "transverse" ) == 0 )
        {
        orientation = VTKIMAGEDATATOTKPHOTO_TRANSVERSE;
        }
      if ( strcmp ( argv[4], "coronal" ) == 0 )
        {
        orientation = VTKIMAGEDATATOTKPHOTO_CORONAL;
        }
      if ( strcmp ( argv[4], "sagittal" ) == 0 )
        {
        orientation = VTKIMAGEDATATOTKPHOTO_SAGITTAL;
        }
      }

    // Get Window/Level
    if ( argc >= 6 )
      {
      if ( ( status =  Tcl_GetDouble ( interp, argv[5], &window ) ) != TCL_OK )
        {
        return status;
        }
      }
    if ( argc >= 7 )
      {
      if ( ( status =  Tcl_GetDouble ( interp, argv[6], &level ) ) != TCL_OK )
        {
        return status;
        }
      }
    
  
    int extent[6];
    // Pass the check?
    int valid = 1;
    image->Update();
    image->GetWholeExtent ( extent );
    // Setup the photo data block, this info will be used later to
    // handle the vtk data types and window/level
    // For reference:
    // pitch - address difference between two vertically adjacent pixels
    // pixelSize - address  difference  between  two
    //             horizontally  adjacent pixels
    Tk_PhotoImageBlock block;
    block.width = 0;
    block.height = 0;
    block.pixelSize = 0;
    block.pitch = 0;
    void *TempPointer = 0;
    switch ( orientation )
      {
      case VTKIMAGEDATATOTKPHOTO_TRANSVERSE:
        {
        valid = ( slice >= extent[4] && slice <= extent[5] );
        if ( valid )
          {
          TempPointer = image->GetScalarPointer ( 0, extent[3], slice );
          block.width = extent[1] - extent[0] + 1;
          block.height = extent[3] - extent[2] + 1;
          block.pixelSize = components;
          block.pitch = -components * ( block.width );
          }      
        break;
        }
      case VTKIMAGEDATATOTKPHOTO_SAGITTAL:
        {
        valid = ( slice >= extent[0] && slice <= extent[1] );
        if ( valid )
          {
          TempPointer = image->GetScalarPointer ( slice, extent[3], 0 );
          block.width = extent[3] - extent[2] + 1;
          block.height = extent[5] - extent[4] + 1;
          block.pixelSize = -components * ( extent[1] - extent[0] + 1 );
          block.pitch = components
            * ( extent[1] - extent[0] + 1 )
            * ( extent[3] - extent[2] + 1 );
          }
        break;
        }
      case VTKIMAGEDATATOTKPHOTO_CORONAL:
        {
        valid = ( slice >= extent[2] && slice <= extent[3] );
        if ( valid )
          {
          TempPointer = image->GetScalarPointer ( 0, slice, 0 );
          block.width = extent[1] - extent[0] + 1;
          block.height = extent[5] - extent[4] + 1;
          block.pixelSize = components;
          block.pitch = components
            * ( extent[1] - extent[0] + 1 )
            * ( extent[3] - extent[2] + 1 );
          }
        break;
        }
      }

    if ( !valid )
      {
      const char* m = "slice is outside the image extent";
      Tcl_SetResult ( interp, const_cast<char*>(m), TCL_VOLATILE );
      return TCL_ERROR;
      }

    // Extract the data, and reset the block
    unsigned char* photobuffer = new unsigned char[block.width * block.height * components];
    double shift, scale;
    shift = window / 2.0 - level;
    scale = 255.0 / window;
    switch ( image->GetScalarType() )
      {
      vtkTemplateMacro ( 
        vtkExtractImageData(photobuffer,
                            static_cast<VTK_TT*> (TempPointer),
                            shift,
                            scale,
                            block.width,
                            block.height,
                            block.pitch,
                            block.pixelSize,
                            components ));
      }
    block.pitch = block.width * components;
    block.pixelSize = components;
    block.pixelPtr = photobuffer;
  
    block.offset[0] = 0;
    block.offset[1] = 1;
    block.offset[2] = 2;
    block.offset[3] = 0;
    switch ( components )
      {
      case 1:
        block.offset[0] = 0;
        block.offset[1] = 0;
        block.offset[2] = 0;
        block.offset[3] = 0;
        break;
      case 3:
        block.offset[3] = 0;
        break;
      case 4:
        block.offset[3] = 3;
        break;
      }
    Tk_PhotoSetSize ( photo, block.width, block.height );
    Tk_PhotoPutBlock ( photo, &block, 0, 0, block.width, block.height );
    delete[] photobuffer;
    return TCL_OK;
  }
}
    
//----------------------------------------------------------------------------
// It's possible to change with this function or in a script some
// options like width, height or the render widget.
int vtkTkRenderWidget_Configure(Tcl_Interp *interp, 
                                struct vtkTkRenderWidget *self,
                                int argc, char *argv[], int flags) 
{
  // Let Tk handle generic configure options.
  if (Tk_ConfigureWidget(interp, 
                         self->TkWin, 
                         vtkTkRenderWidgetConfigSpecs,
                         argc, 
#if (TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION >= 4)
                         const_cast<CONST84 char **>(argv), 
#else
                         argv, 
#endif
                         (char *)self, 
                         flags) == TCL_ERROR) 
    {
    return(TCL_ERROR);
    }
  
  // Get the new  width and height of the widget
  Tk_GeometryRequest(self->TkWin, self->Width, self->Height);

  // Make sure the render window has been set.  If not, create one.
  if (vtkTkRenderWidget_MakeRenderWindow(self) == TCL_ERROR) 
    {
    return TCL_ERROR;
    }
  
  return TCL_OK;
}

//----------------------------------------------------------------------------
// This function is called when the render widget name is 
// evaluated in a Tcl script.  It will compare string parameters
// to choose the appropriate method to invoke.
extern "C"
{
  int vtkTkRenderWidget_Widget(ClientData clientData, 
                               Tcl_Interp *interp,
                               int argc, 
#if (TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION >= 4)
                               CONST84
#endif
                               char *argv[]) 
  {
    struct vtkTkRenderWidget *self = (struct vtkTkRenderWidget *)clientData;
    int result = TCL_OK;
    
    // Check to see if the command has enough arguments.
    if (argc < 2) 
      {
      Tcl_AppendResult(interp, "wrong # args: should be \"",
                       argv[0], " ?options?\"", NULL);
      return TCL_ERROR;
      }
    
    // Make sure the widget is not deleted during this function
    Tk_Preserve((ClientData)self);
    
    // Handle render call to the widget
    if (strncmp(argv[1], "render", VTK_MAX(1, strlen(argv[1]))) == 0 || 
        strncmp(argv[1], "Render", VTK_MAX(1, strlen(argv[1]))) == 0) 
      {
      // make sure we have a window
      if (self->RenderWindow == NULL)
        {
        vtkTkRenderWidget_MakeRenderWindow(self); 
        }
      self->RenderWindow->Render();
      }
    // Handle configure method
    else if (!strncmp(argv[1], "configure", VTK_MAX(1, strlen(argv[1])))) 
      {
      if (argc == 2) 
        {
        /* Return list of all configuration parameters */
        result = Tk_ConfigureInfo(interp, self->TkWin, 
                                  vtkTkRenderWidgetConfigSpecs,
                                  (char *)self, (char *)NULL, 0);
        }
      else if (argc == 3) 
        {
        /* Return a specific configuration parameter */
        result = Tk_ConfigureInfo(interp, self->TkWin, 
                                  vtkTkRenderWidgetConfigSpecs,
                                  (char *)self, argv[2], 0);
        }
      else 
        {
        /* Execute a configuration change */
        result = vtkTkRenderWidget_Configure(interp, self, argc-2, 
#if (TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION >= 4)
                                             const_cast<char **>(argv+2), 
#else
                                             argv+2, 
#endif
                                             TK_CONFIG_ARGV_ONLY);
        }
      }
    else if (!strcmp(argv[1], "GetRenderWindow"))
      { // Get RenderWindow is my own method
      // Create a RenderWidget if one has not been set yet.
      result = vtkTkRenderWidget_MakeRenderWindow(self);
      if (result != TCL_ERROR)
        {
        // Return the name (Make Tcl copy the string)
        Tcl_SetResult(interp, self->RW, TCL_VOLATILE);
        }
      }
    else 
      {
      // Unknown method name.
      Tcl_AppendResult(interp, "vtkTkRenderWidget: Unknown option: ", argv[1], 
                       "\n", "Try: configure or GetRenderWindow\n", NULL);
      result = TCL_ERROR;
      }
    
    // Unlock the object so it can be deleted.
    Tk_Release((ClientData)self);
    return result;
  }
}

//----------------------------------------------------------------------------
// vtkTkRenderWidget_Cmd
// Called when vtkTkRenderWidget is executed 
// - creation of a vtkTkRenderWidget widget.
//     * Creates a new window
//     * Creates an 'vtkTkRenderWidget' data structure
//     * Creates an event handler for this window
//     * Creates a command that handles this object
//     * Configures this vtkTkRenderWidget for the given arguments
extern "C" 
{
  int vtkTkRenderWidget_Cmd(ClientData clientData, 
                            Tcl_Interp *interp, 
                            int argc, 
#if (TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION >= 4)
                            CONST84
#endif
                            char **argv)
  {
#if (TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION >= 4)
    CONST84
#endif
    char *name;
    Tk_Window main = (Tk_Window)clientData;
    Tk_Window tkwin;
    struct vtkTkRenderWidget *self;
    
    // Make sure we have an instance name.
    if (argc <= 1) 
      {
      Tcl_ResetResult(interp);
      Tcl_AppendResult(interp, 
                       "wrong # args: should be \"pathName read filename\"", 
                       NULL);
      return(TCL_ERROR);
      }
    
    // Create the window.
    name = argv[1];
    // Possibly X dependent
    tkwin = Tk_CreateWindowFromPath(interp, main, name, (char *) NULL);
    if (tkwin == NULL) 
      {
      return TCL_ERROR;
      }
    
    // Tcl needs this for setting options and matching event bindings.
    Tk_SetClass(tkwin, (char *) "vtkTkRenderWidget");
    
    // Create vtkTkRenderWidget data structure 
    self=(struct vtkTkRenderWidget *)ckalloc(sizeof(struct vtkTkRenderWidget));
    self->TkWin = tkwin;
    self->Interp = interp;
    self->Width = 0;
    self->Height = 0;
    self->RenderWindow = NULL;
    self->RW = NULL;
    
    // ...
    // Create command event handler
    Tcl_CreateCommand(interp, Tk_PathName(tkwin), vtkTkRenderWidget_Widget, 
                      (ClientData)self, (void (*)(ClientData)) NULL);
    Tk_CreateEventHandler(tkwin, ExposureMask | StructureNotifyMask,
                          vtkTkRenderWidget_EventProc, (ClientData)self);
    
    // Configure vtkTkRenderWidget widget
    if (vtkTkRenderWidget_Configure(interp, 
                                    self, 
                                    argc-2, 
#if (TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION >= 4)
                                    const_cast<char **>(argv+2), 
#else
                                    argv+2, 
#endif
                                    0) 
        == TCL_ERROR) 
      {
      Tk_DestroyWindow(tkwin);
      Tcl_DeleteCommand(interp, (char *) "vtkTkRenderWidget");
      // Don't free it, if we do a crash occurs later...
      //free(self);  
      return TCL_ERROR;
      }
    
    Tcl_AppendResult(interp, Tk_PathName(tkwin), NULL);
    return TCL_OK;
  }
}


//----------------------------------------------------------------------------
const char *vtkTkRenderWidget_RW(const struct vtkTkRenderWidget *self)
{
  return self->RW;
}


//----------------------------------------------------------------------------
int vtkTkRenderWidget_Width( const struct vtkTkRenderWidget *self)
{
   return self->Width;
}


//----------------------------------------------------------------------------
int vtkTkRenderWidget_Height( const struct vtkTkRenderWidget *self)
{
   return self->Height;
}


/*
 *----------------------------------------------------------------------
 *
 * vtkTkRenderWidget_Destroy ---
 *
 *      This procedure is invoked by Tcl_EventuallyFree or Tcl_Release
 *      to clean up the internal structure of a canvas at a safe time
 *      (when no-one is using it anymore).
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Everything associated with the canvas is freed up.
 *
 *----------------------------------------------------------------------
 */

extern "C"
{
  void vtkTkRenderWidget_Destroy(char *memPtr)
  {
    struct vtkTkRenderWidget *self = (struct vtkTkRenderWidget *)memPtr;

    if (self->RenderWindow)
      {
      if (self->RenderWindow->GetInteractor() &&
          self->RenderWindow->GetInteractor()->GetRenderWindow() == self->RenderWindow)
        {
        self->RenderWindow->GetInteractor()->SetRenderWindow(0);
        }
      if (self->RenderWindow->GetReferenceCount() > 1)
        {
        vtkGenericWarningMacro(
          "A TkRenderWidget is being destroyed before it associated vtkRenderWindow is destroyed."
          "This is very bad and usually due to the order in which objects are being destroyed."
          "Always destroy the vtkRenderWindow before destroying the user interface components.");
        }
      self->RenderWindow->UnRegister(NULL);
      self->RenderWindow = NULL;
      }
    ckfree (self->RW);
    ckfree(memPtr);
  }
}

//----------------------------------------------------------------------------
// This gets called to handle vtkTkRenderWidget window configuration events
// Possibly X dependent
extern "C"
{
  void vtkTkRenderWidget_EventProc(ClientData clientData, 
                                   XEvent *eventPtr) 
  {
    struct vtkTkRenderWidget *self = (struct vtkTkRenderWidget *)clientData;
    
    switch (eventPtr->type) 
      {
      case Expose:
        if ((eventPtr->xexpose.count == 0)
            /* && !self->UpdatePending*/) 
          {
          // let the user bind expose events
          // self->RenderWindow->Render();
          }
        break;
      case ConfigureNotify:
        //if ( Tk_IsMapped(self->TkWin) ) 
      {
      self->Width = Tk_Width(self->TkWin);
      self->Height = Tk_Height(self->TkWin);
      //Tk_GeometryRequest(self->TkWin,self->Width,self->Height);
      if (self->RenderWindow)
        {
#if defined(VTK_USE_CARBON) || defined(VTK_USE_COCOA)
        // Do not call SetSize or SetPosition until we're mapped.
        if (Tk_IsMapped(self->TkWin))
          {
          // On Carbon and Cocoa, compute coordinates relative to toplevel
          int x = Tk_X(self->TkWin);
          int y = Tk_Y(self->TkWin);
          for (TkWindow *curPtr = ((TkWindow *)self->TkWin)->parentPtr;
               (NULL != curPtr) && !(curPtr->flags & TK_TOP_LEVEL);
               curPtr = curPtr->parentPtr)
            {
            x += Tk_X(curPtr);
            y += Tk_Y(curPtr);
            }
          self->RenderWindow->SetPosition(x, y);
          self->RenderWindow->SetSize(self->Width, self->Height);
          }
#ifdef VTK_USE_CARBON
        // On Carbon, if we aren't mapped, clear the AGL_BUFFER_RECT.
        else
          {
          self->RenderWindow->SetSize(0, 0);
          }
#endif
#else
        self->RenderWindow->SetPosition(Tk_X(self->TkWin),Tk_Y(self->TkWin));
        self->RenderWindow->SetSize(self->Width, self->Height);
#endif
        }
      //vtkTkRenderWidget_PostRedisplay(self);
      }
      break;
      case MapNotify:
      {
#if defined(VTK_USE_CARBON) || defined(VTK_USE_COCOA)
      // On Carbon and Cocoa, compute coordinates relative to the toplevel
      int x = Tk_X(self->TkWin);
      int y = Tk_Y(self->TkWin);
      for (TkWindow *curPtr = ((TkWindow *)self->TkWin)->parentPtr;
           (NULL != curPtr) && !(curPtr->flags & TK_TOP_LEVEL);
           curPtr = curPtr->parentPtr)
        {
        x += Tk_X(curPtr);
        y += Tk_Y(curPtr);
        }
      self->RenderWindow->SetPosition(x, y);
      self->RenderWindow->SetSize(self->Width, self->Height);
#endif
      break;
      }
#if defined(VTK_USE_CARBON) || defined(VTK_USE_COCOA)
      case UnmapNotify:
      {
#ifdef VTK_USE_CARBON
      // On Carbon, clear the AGL_BUFFER_RECT by calling SetSize(0, 0).
      self->RenderWindow->SetSize(0, 0);
#endif
      break;
      }
#endif
      case DestroyNotify:
        Tcl_EventuallyFree((ClientData) self, vtkTkRenderWidget_Destroy );
        break;
      default:
        // nothing
        ;
      }
  }
}



//----------------------------------------------------------------------------
// vtkTkRenderWidget_Init
// Called upon system startup to create vtkTkRenderWidget command.
extern "C" {VTK_TK_EXPORT int Vtktkrenderwidget_Init(Tcl_Interp *interp);}

#define VTKTK_TO_STRING(x) VTKTK_TO_STRING0(x)
#define VTKTK_TO_STRING0(x) VTKTK_TO_STRING1(x)
#define VTKTK_TO_STRING1(x) #x
#define VTKTK_VERSION VTKTK_TO_STRING(VTK_MAJOR_VERSION) "." VTKTK_TO_STRING(VTK_MINOR_VERSION)

int VTK_TK_EXPORT Vtktkrenderwidget_Init(Tcl_Interp *interp)
{
  // This widget requires Tk to function.
  Tcl_PkgRequire(interp, (char *)"Tk", (char*)TK_VERSION, 0);
  if(Tcl_PkgPresent(interp, (char *)"Tk", (char*)TK_VERSION, 0))
    {
    // Register the commands for this package.
    Tcl_CreateCommand(interp, (char*)"vtkTkRenderWidget",
                      vtkTkRenderWidget_Cmd, Tk_MainWindow(interp), NULL);
    Tcl_CreateCommand(interp, (char*)"vtkImageDataToTkPhoto",
                      vtkImageDataToTkPhoto_Cmd, NULL, NULL);

    // Report that the package is provided.
    return Tcl_PkgProvide(interp, (char*)"Vtktkrenderwidget",
                          (char*)VTKTK_VERSION);
    }
  else
    {
    // Tk is not available.
    return TCL_ERROR;
    }
}


// Here is the windows specific code for creating the window
// The Xwindows version follows after this
#ifdef _WIN32

LRESULT APIENTRY vtkTkRenderWidgetProc(HWND hWnd, UINT message, 
                                       WPARAM wParam, LPARAM lParam)
{
  LRESULT rval;
  struct vtkTkRenderWidget *self = 
    (struct vtkTkRenderWidget *)vtkGetWindowLong(hWnd,sizeof(vtkLONG));

  if (!self)
    {
    return 1;
    }
  
  // watch for WM_USER + 12  this is a special message
  // from the vtkRenderWindowInteractor letting us
  // know it wants to get events also
  if ((message == WM_USER+12)&&(wParam == 24))
    {
    WNDPROC tmp = (WNDPROC)lParam;
    // we need to tell it what the original vtk event handler was 
    vtkSetWindowLong(hWnd,sizeof(vtkLONG),(vtkLONG)self->RenderWindow);
    tmp(hWnd, WM_USER+13,26,(vtkLONG)self->OldProc);
    vtkSetWindowLong(hWnd,sizeof(vtkLONG),(vtkLONG)self);
    self->OldProc = tmp;
    return 1;
    }
  if ((message == WM_USER+14)&&(wParam == 28))
    {
    WNDPROC tmp = (WNDPROC)lParam;
    self->OldProc = tmp;
    return 1;
    }

  if (!self->TkWin)
    {
    return 1;
    }

  // forward message to Tk handler
  vtkSetWindowLong(hWnd,sizeof(vtkLONG),(vtkLONG)((TkWindow *)self->TkWin)->window);
  if (((TkWindow *)self->TkWin)->parentPtr)
    {
    vtkSetWindowLong(hWnd,vtkGWL_WNDPROC,(vtkLONG)TkWinChildProc);
    rval = TkWinChildProc(hWnd,message,wParam,lParam);
    }
  else
    {
#if(TK_MAJOR_VERSION < 8)
    vtkSetWindowLong(hWnd,vtkGWL_WNDPROC,(vtkLONG)TkWinTopLevelProc);
    rval = TkWinTopLevelProc(hWnd,message,wParam,lParam);
#else
    if (message == WM_WINDOWPOSCHANGED) 
      {
      XEvent event;
            WINDOWPOS *pos = (WINDOWPOS *) lParam;
            TkWindow *winPtr = (TkWindow *) Tk_HWNDToWindow(pos->hwnd);
    
            if (winPtr == NULL) {
              return 0;
              }

            /*
             * Update the shape of the contained window.
             */
            if (!(pos->flags & SWP_NOSIZE)) {
              winPtr->changes.width = pos->cx;
              winPtr->changes.height = pos->cy;
              }
            if (!(pos->flags & SWP_NOMOVE)) {
              winPtr->changes.x = pos->x;
              winPtr->changes.y = pos->y;
              }


      /*
       *  Generate a ConfigureNotify event.
       */
      event.type = ConfigureNotify;
      event.xconfigure.serial = winPtr->display->request;
      event.xconfigure.send_event = False;
      event.xconfigure.display = winPtr->display;
      event.xconfigure.event = winPtr->window;
      event.xconfigure.window = winPtr->window;
      event.xconfigure.border_width = winPtr->changes.border_width;
      event.xconfigure.override_redirect = winPtr->atts.override_redirect;
      event.xconfigure.x = winPtr->changes.x;
      event.xconfigure.y = winPtr->changes.y;
      event.xconfigure.width = winPtr->changes.width;
      event.xconfigure.height = winPtr->changes.height;
      event.xconfigure.above = None;
      Tk_QueueWindowEvent(&event, TCL_QUEUE_TAIL);

            Tcl_ServiceAll();
            return 0;
      }
    vtkSetWindowLong(hWnd,vtkGWL_WNDPROC,(vtkLONG)TkWinChildProc);
    rval = TkWinChildProc(hWnd,message,wParam,lParam);
#endif
    }

    if (message != WM_PAINT)
      {
      if (self->RenderWindow)
        {
        vtkSetWindowLong(hWnd,sizeof(vtkLONG),(vtkLONG)self->RenderWindow);
        vtkSetWindowLong(hWnd,vtkGWL_WNDPROC,(vtkLONG)self->OldProc);
        CallWindowProc(self->OldProc,hWnd,message,wParam,lParam);
        }
      }

  // now reset to the original config
  vtkSetWindowLong(hWnd,sizeof(vtkLONG),(vtkLONG)self);
  vtkSetWindowLong(hWnd,vtkGWL_WNDPROC,(vtkLONG)vtkTkRenderWidgetProc);
  return rval;
}

//-----------------------------------------------------------------------------
// Creates a render window and forces Tk to use the window.
static int vtkTkRenderWidget_MakeRenderWindow(struct vtkTkRenderWidget *self) 
{
  Display *dpy;
  TkWindow *winPtr = (TkWindow *) self->TkWin;
  Tcl_HashEntry *hPtr;
  int new_flag;
  vtkWin32OpenGLRenderWindow *renderWindow;
  TkWinDrawable *twdPtr;
  HWND parentWin;

  if (self->RenderWindow)
    {
    return TCL_OK;
    }
  
  dpy = Tk_Display(self->TkWin);

  if (winPtr->window != None) 
    {
    // XDestroyWindow(dpy, winPtr->window);
    }

  if (self->RW[0] == '\0')
    {
    // Make the Render window.
    self->RenderWindow = vtkRenderWindow::New();
    self->RenderWindow->Register(NULL);
    self->RenderWindow->Delete();
    renderWindow = (vtkWin32OpenGLRenderWindow *)(self->RenderWindow);
#ifndef VTK_PYTHON_BUILD
    vtkTclGetObjectFromPointer(self->Interp, self->RenderWindow,
                               "vtkRenderWindow");
#endif
    self->RW = ckalloc(
      static_cast<unsigned int>(strlen(Tcl_GetStringResult(self->Interp)) + 1));
    strcpy(self->RW, Tcl_GetStringResult(self->Interp));
    Tcl_ResetResult(self->Interp);
    }
  else
    {
    // is RW an address ? big ole python hack here
    if (self->RW[0] == 'A' && self->RW[1] == 'd' && 
        self->RW[2] == 'd' && self->RW[3] == 'r')
      {
      void *tmp;
      sscanf(self->RW+5,"%p",&tmp);
      renderWindow = (vtkWin32OpenGLRenderWindow *)tmp;
      }
    else
      {
#ifndef VTK_PYTHON_BUILD
      renderWindow = (vtkWin32OpenGLRenderWindow *)
        vtkTclGetPointerFromObject(self->RW, "vtkRenderWindow", self->Interp,
                                   new_flag);
#else
      renderWindow = 0;
#endif
      }
    if (renderWindow != self->RenderWindow)
      {
      if (self->RenderWindow != NULL) 
        {
        self->RenderWindow->UnRegister(NULL);
        }
      self->RenderWindow = (vtkRenderWindow *)(renderWindow);
      if (self->RenderWindow != NULL)
        {
        self->RenderWindow->Register(NULL);
        }
      }
    }
  
  // Set the size
  self->RenderWindow->SetSize(self->Width, self->Height);
  
  // Set the parent correctly
  // Possibly X dependent
  if ((winPtr->parentPtr != NULL) && !(winPtr->flags & TK_TOP_LEVEL)) 
    {
    if (winPtr->parentPtr->window == None) 
      {
      Tk_MakeWindowExist((Tk_Window) winPtr->parentPtr);
      }

    parentWin = ((TkWinDrawable *)winPtr->parentPtr->window)->window.handle;
    renderWindow->SetParentId(parentWin);
    }
  
  // Use the same display
  self->RenderWindow->SetDisplayId(dpy);
  
  /* Make sure Tk knows to switch to the new colormap when the cursor
   * is over this window when running in color index mode.
   */
  //Tk_SetWindowVisual(self->TkWin, renderWindow->GetDesiredVisual(), 
  //renderWindow->GetDesiredDepth(), 
  //renderWindow->GetDesiredColormap());
  
  self->RenderWindow->Render();  

#if(TK_MAJOR_VERSION >=  8)
  twdPtr = (TkWinDrawable*)Tk_AttachHWND(self->TkWin, renderWindow->GetWindowId());
#else
  twdPtr = (TkWinDrawable*) ckalloc(sizeof(TkWinDrawable));
  twdPtr->type = TWD_WINDOW;
  twdPtr->window.winPtr = winPtr;
  twdPtr->window.handle = renderWindow->GetWindowId();
#endif

  self->OldProc = (WNDPROC)vtkGetWindowLong(twdPtr->window.handle,vtkGWL_WNDPROC);
  vtkSetWindowLong(twdPtr->window.handle,sizeof(vtkLONG),(vtkLONG)self);
  vtkSetWindowLong(twdPtr->window.handle,vtkGWL_WNDPROC,
     (vtkLONG)vtkTkRenderWidgetProc);

  winPtr->window = (Window)twdPtr;
  
  hPtr = Tcl_CreateHashEntry(&winPtr->dispPtr->winTable,
        (char *) winPtr->window, &new_flag);
  Tcl_SetHashValue(hPtr, winPtr);

  
  winPtr->dirtyAtts = 0;
  winPtr->dirtyChanges = 0;
#ifdef TK_USE_INPUT_METHODS
  winPtr->inputContext = NULL;
#endif // TK_USE_INPUT_METHODS 

  if (!(winPtr->flags & TK_TOP_LEVEL)) 
    {
    /*
     * If this window has a different colormap than its parent, add
     * the window to the WM_COLORMAP_WINDOWS property for its top-level.
     */
    if ((winPtr->parentPtr != NULL) &&
       (winPtr->atts.colormap != winPtr->parentPtr->atts.colormap)) 
      {
      TkWmAddToColormapWindows(winPtr);
      }
    } 

  /*
   * Issue a ConfigureNotify event if there were deferred configuration
   * changes (but skip it if the window is being deleted;  the
   * ConfigureNotify event could cause problems if we're being called
   * from Tk_DestroyWindow under some conditions).
   */
  if ((winPtr->flags & TK_NEED_CONFIG_NOTIFY)
      && !(winPtr->flags & TK_ALREADY_DEAD))
    {
    XEvent event;
    
    winPtr->flags &= ~TK_NEED_CONFIG_NOTIFY;
    
    event.type = ConfigureNotify;
    event.xconfigure.serial = LastKnownRequestProcessed(winPtr->display);
    event.xconfigure.send_event = False;
    event.xconfigure.display = winPtr->display;
    event.xconfigure.event = winPtr->window;
    event.xconfigure.window = winPtr->window;
    event.xconfigure.x = winPtr->changes.x;
    event.xconfigure.y = winPtr->changes.y;
    event.xconfigure.width = winPtr->changes.width;
    event.xconfigure.height = winPtr->changes.height;
    event.xconfigure.border_width = winPtr->changes.border_width;
    if (winPtr->changes.stack_mode == Above) 
      {
      event.xconfigure.above = winPtr->changes.sibling;
      }
    else 
      {
      event.xconfigure.above = None;
      }
    event.xconfigure.override_redirect = winPtr->atts.override_redirect;
    Tk_HandleEvent(&event);
    }

  return TCL_OK;
}
#else

// the cocoa and carbon versions
#if defined(VTK_USE_CARBON) || defined(VTK_USE_COCOA)
//----------------------------------------------------------------------------
// Creates a render window and forces Tk to use the window.
static int
vtkTkRenderWidget_MakeRenderWindow(struct vtkTkRenderWidget *self) 
{
  vtkRenderWindow *renderWindow = NULL;

  if (self->RenderWindow)
    {
    return TCL_OK;
    }
  
  if (self->RW[0] == '\0')
    {
    // Make the Render window.
    self->RenderWindow = vtkRenderWindow::New();
    self->RenderWindow->Register(NULL);
    self->RenderWindow->Delete();
    renderWindow = self->RenderWindow;
#ifndef VTK_PYTHON_BUILD
    vtkTclGetObjectFromPointer(self->Interp, self->RenderWindow,
          "vtkRenderWindow");
#endif
    self->RW = ckalloc(
      static_cast<unsigned int>(strlen(Tcl_GetStringResult(self->Interp)) + 1));
    strcpy(self->RW, Tcl_GetStringResult(self->Interp));
    Tcl_ResetResult(self->Interp);
    }
  else
    {
    // is RW an address ? big ole python hack here
    if (self->RW[0] == 'A' && self->RW[1] == 'd' && 
        self->RW[2] == 'd' && self->RW[3] == 'r')
      {
      void *tmp;
      sscanf(self->RW+5,"%p",&tmp);
      renderWindow = reinterpret_cast<vtkRenderWindow *>(tmp);
      }
    else
      {
#ifndef VTK_PYTHON_BUILD
      int new_flag;
      renderWindow = static_cast<vtkRenderWindow *>(
        vtkTclGetPointerFromObject(self->RW,"vtkRenderWindow",self->Interp, 
                                   new_flag));
#endif
      }

    if (renderWindow != self->RenderWindow)
      {
      if (self->RenderWindow != NULL) 
        {
        self->RenderWindow->UnRegister(NULL);
        }
      self->RenderWindow = renderWindow;
      if (self->RenderWindow != NULL) 
        {
        self->RenderWindow->Register(NULL);
        }
      }
    }

  TkWindow *winPtr = reinterpret_cast<TkWindow *>(self->TkWin);

#ifdef VTK_USE_CARBON
  WindowPtr parentWin;
  // Position should be set, but only if winPtr properly initialized
  // 
  //self->RenderWindow->SetPosition(winPtr->privatePtr->xOff,
  //                                winPtr->privatePtr->yOff);
  self->RenderWindow->SetSize(self->Width, self->Height);

  // Set the parent correctly and get the actual OSX window on the screen
  // Window must be up so that the aglContext can be attached to it
  if ((winPtr->parentPtr != NULL) && !(winPtr->flags & TK_TOP_LEVEL))
    {
    if (winPtr->parentPtr->window == None)
      {
      // Look at each parent TK window in order until we run out
      // of windows or find the top level. Then the OSX window that will be
      // the parent is created so that we have a window to pass to the
      // vtkRenderWindow so it can attach its openGL context.
      // Ideally the Tk_MakeWindowExist call would do the deed. (I think)
      TkWindow *curWin = winPtr->parentPtr;
      while ((NULL != curWin->parentPtr) && !(curWin->flags & TK_TOP_LEVEL))
        {
        curWin = curWin->parentPtr;
        }
      Tk_MakeWindowExist((Tk_Window) winPtr->parentPtr);
      if (NULL != curWin)
        {
        TkMacOSXMakeRealWindowExist(curWin);
        }
      else
        {
        vtkGenericWarningMacro("Could not find the TK_TOP_LEVEL. This is bad.");
        }
      }

    parentWin = GetWindowFromPort((CGrafPtr)TkMacOSXGetDrawablePort(
                                  Tk_WindowId(winPtr->parentPtr)));
    // Carbon does not have 'sub-windows', so the ParentId is used more
    // as a flag to indicate that the renderwindow is being used as a sub-
    // view of its 'parent' window.
    vtkCarbonRenderWindow *carbonRenderWindow = 
      static_cast<vtkCarbonRenderWindow *>(renderWindow);
    carbonRenderWindow->SetParentId(parentWin);
    carbonRenderWindow->SetRootWindow(parentWin);
    }
  
  renderWindow->SetDisplayId(Tk_Display(self->TkWin));

#else /* now the VTK_USE_COCOA section */
  Tk_MakeWindowExist(self->TkWin);
  // set the ParentId to the NSView of the Tk toplevel
  renderWindow->SetParentId(vtkCocoaTkUtilities::GetDrawableView(self->TkWin));
  renderWindow->SetSize(self->Width, self->Height);
#endif

#ifdef VTK_USE_CARBON
  Display *dpy = Tk_Display(self->TkWin);

  // Don't render yet, the widget isn't necessarily mapped
  // self->RenderWindow->Render();

  XSelectInput(dpy, Tk_WindowId(self->TkWin), VTK_ALL_EVENTS_MASK);
#endif

  /*
   * Issue a ConfigureNotify event if there were deferred configuration
   * changes (but skip it if the window is being deleted;  the
   * ConfigureNotify event could cause problems if we're being called
   * from Tk_DestroyWindow under some conditions).
   */
  if ((winPtr->flags & TK_NEED_CONFIG_NOTIFY)
      && !(winPtr->flags & TK_ALREADY_DEAD))
    {
      XEvent event;

      winPtr->flags &= ~TK_NEED_CONFIG_NOTIFY;

      event.type = ConfigureNotify;
      event.xconfigure.serial = LastKnownRequestProcessed(winPtr->display);
      event.xconfigure.send_event = False;
      event.xconfigure.display = winPtr->display;
      event.xconfigure.event = winPtr->window;
      event.xconfigure.window = winPtr->window;
      event.xconfigure.x = winPtr->changes.x;
      event.xconfigure.y = winPtr->changes.y;
      event.xconfigure.width = winPtr->changes.width;
      event.xconfigure.height = winPtr->changes.height;
      event.xconfigure.border_width = winPtr->changes.border_width;
      if (winPtr->changes.stack_mode == Above)
        {
        event.xconfigure.above = winPtr->changes.sibling;
        }
      else
        {
        event.xconfigure.above = None;
        }
      event.xconfigure.override_redirect = winPtr->atts.override_redirect;
      Tk_HandleEvent(&event);
    }
#ifdef VTK_USE_CARBON
  else
    {
    // Assume that vtkTkRenderWidget will be packed after this
    // method is called.  Reset the AGL_BUFFER_RECT to avoid getting the
    // initial 'black square'.

    // Cast to a vtkCarbonRenderWindow so we can access the necessary members.
    vtkCarbonRenderWindow *carbonRW =
      static_cast<vtkCarbonRenderWindow *>(self->RenderWindow);
    carbonRW->Initialize();
    carbonRW->UpdateSizeAndPosition(0, 0, 0, 0);
    carbonRW->UpdateGLRegion();
    }
#endif

  return TCL_OK;
}

// now the Xwindows version
#else

//----------------------------------------------------------------------------
// Creates a render window and forces Tk to use the window.
static int
vtkTkRenderWidget_MakeRenderWindow(struct vtkTkRenderWidget *self) 
{
  Display *dpy;
  vtkXOpenGLRenderWindow *renderWindow = 0;
  
  if (self->RenderWindow)
    {
    return TCL_OK;
    }
  
  dpy = Tk_Display(self->TkWin);
  
  if (Tk_WindowId(self->TkWin) != None) 
    {
    XDestroyWindow(dpy, Tk_WindowId(self->TkWin));
    }

  if (self->RW[0] == '\0')
    {
    // Make the Render window.
    self->RenderWindow = vtkRenderWindow::New();
    self->RenderWindow->Register(NULL);
    self->RenderWindow->Delete();
    renderWindow = (vtkXOpenGLRenderWindow *)(self->RenderWindow);
#ifndef VTK_PYTHON_BUILD
    vtkTclGetObjectFromPointer(self->Interp, self->RenderWindow,
          "vtkRenderWindow");
#endif
    self->RW = ckalloc(
      static_cast<unsigned int>(strlen(Tcl_GetStringResult(self->Interp)) + 1));
    strcpy(self->RW, Tcl_GetStringResult(self->Interp));
    Tcl_ResetResult(self->Interp);
    }
  else
    {
    // is RW an address ? big ole python hack here
    if (self->RW[0] == 'A' && self->RW[1] == 'd' && 
        self->RW[2] == 'd' && self->RW[3] == 'r')
      {
      void *tmp;
      sscanf(self->RW+5,"%p",&tmp);
      renderWindow = (vtkXOpenGLRenderWindow *)tmp;
      }
    else
      {
#ifndef VTK_PYTHON_BUILD
      int new_flag;
      renderWindow = (vtkXOpenGLRenderWindow *)
        vtkTclGetPointerFromObject(self->RW,"vtkRenderWindow",self->Interp, 
          new_flag);
#endif
      }
    if (renderWindow != self->RenderWindow)
      {
      if (self->RenderWindow != NULL) {self->RenderWindow->UnRegister(NULL);}
      self->RenderWindow = (vtkRenderWindow *)(renderWindow);
      if (self->RenderWindow != NULL) {self->RenderWindow->Register(NULL);}
      }
    }
  
  // If window already exists, return an error
  if ( renderWindow->GetWindowId() != (Window)NULL )
    {
    return TCL_ERROR;
    }
 
  // Use the same display
  renderWindow->SetDisplayId(dpy);
  
  /* Make sure Tk knows to switch to the new colormap when the cursor
   * is over this window when running in color index mode.
   */
  // The visual MUST BE SET BEFORE the window is created.
  Tk_SetWindowVisual(self->TkWin, renderWindow->GetDesiredVisual(), 
                     renderWindow->GetDesiredDepth(), 
                     renderWindow->GetDesiredColormap());
  
  // Make this window exist, then use that information to make the vtkImageViewer in sync
  Tk_MakeWindowExist ( self->TkWin );
  renderWindow->SetWindowId ( (void*)Tk_WindowId ( self->TkWin ) );
  
  // Set the size
  self->RenderWindow->SetSize(self->Width, self->Height);
  
  // Set the parent correctly
  // Possibly X dependent
  if ((Tk_Parent(self->TkWin) == NULL) || (Tk_IsTopLevel(self->TkWin))) 
    {
    renderWindow->SetParentId(XRootWindow(Tk_Display(self->TkWin),
                                          Tk_ScreenNumber(self->TkWin)));
    }
  else 
    {
    renderWindow->SetParentId(Tk_WindowId(Tk_Parent(self->TkWin) ));
    }

  self->RenderWindow->Render();  
  XSelectInput(dpy, Tk_WindowId(self->TkWin), VTK_ALL_EVENTS_MASK);
  
  return TCL_OK;
}
#endif


#endif
