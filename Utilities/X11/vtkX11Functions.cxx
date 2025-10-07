// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkLogger.h"

#include "vtkX11Functions.h"

#if defined(VTK_USE_X)
#include "vtkDynamicLoader.h"

#include <atomic>
#include <cstddef>
#include <dlfcn.h>
namespace
{
std::atomic<std::size_t> RequestId = 0;
const char* X11_LIBRARY_NAMES[] = { "libX11.so.6", "libX11.so", nullptr };
const char* XCURSOR_LIBRARY_NAMES[] = { "libXcursor.so.1", "libXcursor.so", nullptr };
}

#define NULLIFY_POINTER_TO_FUNCTION(name) name = nullptr
#define DEFINE_POINTER_TO_FUNCTION(name) name##Type NULLIFY_POINTER_TO_FUNCTION(name)

static void* libX11 = nullptr;
DEFINE_POINTER_TO_FUNCTION(vtkXInternAtom);
DEFINE_POINTER_TO_FUNCTION(vtkXQueryPointer);
DEFINE_POINTER_TO_FUNCTION(vtkXTranslateCoordinates);
DEFINE_POINTER_TO_FUNCTION(vtkXKeysymToString);
DEFINE_POINTER_TO_FUNCTION(vtkXListExtensions);
DEFINE_POINTER_TO_FUNCTION(vtkXCreateColormap);
DEFINE_POINTER_TO_FUNCTION(vtkXCreateFontCursor);
DEFINE_POINTER_TO_FUNCTION(vtkXCreatePixmapCursor);
DEFINE_POINTER_TO_FUNCTION(vtkXOpenDisplay);
DEFINE_POINTER_TO_FUNCTION(vtkXCreateGC);
DEFINE_POINTER_TO_FUNCTION(vtkXDefaultScreenOfDisplay);
DEFINE_POINTER_TO_FUNCTION(vtkXChangeProperty);
DEFINE_POINTER_TO_FUNCTION(vtkXChangeWindowAttributes);
DEFINE_POINTER_TO_FUNCTION(vtkXCheckIfEvent);
DEFINE_POINTER_TO_FUNCTION(vtkXCheckTypedWindowEvent);
DEFINE_POINTER_TO_FUNCTION(vtkXCloseDisplay);
DEFINE_POINTER_TO_FUNCTION(vtkXConvertSelection);
DEFINE_POINTER_TO_FUNCTION(vtkXDefaultScreen);
DEFINE_POINTER_TO_FUNCTION(vtkXDefineCursor);
DEFINE_POINTER_TO_FUNCTION(vtkXDestroyWindow);
DEFINE_POINTER_TO_FUNCTION(vtkXDisplayHeight);
DEFINE_POINTER_TO_FUNCTION(vtkXDisplayWidth);
DEFINE_POINTER_TO_FUNCTION(vtkXFlush);
DEFINE_POINTER_TO_FUNCTION(vtkXFreeCursor);
DEFINE_POINTER_TO_FUNCTION(vtkXFreeExtensionList);
DEFINE_POINTER_TO_FUNCTION(vtkXFreePixmap);
DEFINE_POINTER_TO_FUNCTION(vtkXFree);
DEFINE_POINTER_TO_FUNCTION(vtkXGetWindowAttributes);
DEFINE_POINTER_TO_FUNCTION(vtkXGetWindowProperty);
DEFINE_POINTER_TO_FUNCTION(vtkXGrabKeyboard);
DEFINE_POINTER_TO_FUNCTION(vtkXIfEvent);
DEFINE_POINTER_TO_FUNCTION(vtkXLookupString);
DEFINE_POINTER_TO_FUNCTION(vtkXMapWindow);
DEFINE_POINTER_TO_FUNCTION(vtkXMoveWindow);
DEFINE_POINTER_TO_FUNCTION(vtkXNextEvent);
DEFINE_POINTER_TO_FUNCTION(vtkXPending);
DEFINE_POINTER_TO_FUNCTION(vtkXPutBackEvent);
DEFINE_POINTER_TO_FUNCTION(vtkXResizeWindow);
DEFINE_POINTER_TO_FUNCTION(vtkXScreenNumberOfScreen);
DEFINE_POINTER_TO_FUNCTION(vtkXSelectInput);
DEFINE_POINTER_TO_FUNCTION(vtkXSendEvent);
DEFINE_POINTER_TO_FUNCTION(vtkXSetClassHint);
DEFINE_POINTER_TO_FUNCTION(vtkXSetErrorHandler);
DEFINE_POINTER_TO_FUNCTION(vtkXSetInputFocus);
DEFINE_POINTER_TO_FUNCTION(vtkXSetNormalHints);
DEFINE_POINTER_TO_FUNCTION(vtkXSetWMIconName);
DEFINE_POINTER_TO_FUNCTION(vtkXSetWMName);
DEFINE_POINTER_TO_FUNCTION(vtkXSetWMProtocols);
DEFINE_POINTER_TO_FUNCTION(vtkXStoreName);
DEFINE_POINTER_TO_FUNCTION(vtkXStringListToTextProperty);
DEFINE_POINTER_TO_FUNCTION(vtkXSync);
DEFINE_POINTER_TO_FUNCTION(vtkXUndefineCursor);
DEFINE_POINTER_TO_FUNCTION(vtkXUnmapWindow);
DEFINE_POINTER_TO_FUNCTION(vtkXCreateBitmapFromData);
DEFINE_POINTER_TO_FUNCTION(vtkXScreenOfDisplay);
DEFINE_POINTER_TO_FUNCTION(vtkXVisualIDFromVisual);
DEFINE_POINTER_TO_FUNCTION(vtkXCreateWindow);
DEFINE_POINTER_TO_FUNCTION(vtkXRootWindowOfScreen);
DEFINE_POINTER_TO_FUNCTION(vtkXRootWindow);
DEFINE_POINTER_TO_FUNCTION(vtkXGetVisualInfo);

// Xcursor API
#if VTK_HAVE_XCURSOR
static void* libXcursor = nullptr;
DEFINE_POINTER_TO_FUNCTION(vtkXcursorFilenameLoadCursor);
#endif

#define LOAD_POINTER_TO_FUNCTION(lib, symbol, name)                                                \
  name = reinterpret_cast<name##Type>(vtkDynamicLoader::GetSymbolAddress(lib, #symbol));           \
  if (name == nullptr)                                                                             \
  {                                                                                                \
    vtkLog(WARNING, "Failed to load symbol " #symbol);                                             \
  }

extern "C"
{
  void vtkX11FunctionsInitialize()
  {
    vtkLog(TRACE, "Initializing vtkX11Functions");
    if (++RequestId == 1)
    {
      vtkLog(TRACE, "Loading X11 function pointers");
      for (const char** libName = X11_LIBRARY_NAMES; *libName != nullptr; ++libName)
      {
        libX11 = dlopen(*libName, RTLD_LAZY | RTLD_GLOBAL);
        if (libX11 != nullptr)
        {
          vtkLog(TRACE, "Successfully loaded " << *libName);
          break;
        }
      }
      if (libX11 == nullptr)
      {
        vtkLog(WARNING, "Failed to load an X11 library");
        return;
      }
      LOAD_POINTER_TO_FUNCTION(libX11, XInternAtom, vtkXInternAtom);
      LOAD_POINTER_TO_FUNCTION(libX11, XQueryPointer, vtkXQueryPointer);
      LOAD_POINTER_TO_FUNCTION(libX11, XTranslateCoordinates, vtkXTranslateCoordinates);
      LOAD_POINTER_TO_FUNCTION(libX11, XKeysymToString, vtkXKeysymToString);
      LOAD_POINTER_TO_FUNCTION(libX11, XListExtensions, vtkXListExtensions);
      LOAD_POINTER_TO_FUNCTION(libX11, XCreateColormap, vtkXCreateColormap);
      LOAD_POINTER_TO_FUNCTION(libX11, XCreateFontCursor, vtkXCreateFontCursor);
      LOAD_POINTER_TO_FUNCTION(libX11, XCreatePixmapCursor, vtkXCreatePixmapCursor);
      LOAD_POINTER_TO_FUNCTION(libX11, XOpenDisplay, vtkXOpenDisplay);
      LOAD_POINTER_TO_FUNCTION(libX11, XCreateGC, vtkXCreateGC);
      LOAD_POINTER_TO_FUNCTION(libX11, XDefaultScreenOfDisplay, vtkXDefaultScreenOfDisplay);
      LOAD_POINTER_TO_FUNCTION(libX11, XChangeProperty, vtkXChangeProperty);
      LOAD_POINTER_TO_FUNCTION(libX11, XChangeWindowAttributes, vtkXChangeWindowAttributes);
      LOAD_POINTER_TO_FUNCTION(libX11, XCheckIfEvent, vtkXCheckIfEvent);
      LOAD_POINTER_TO_FUNCTION(libX11, XCheckTypedWindowEvent, vtkXCheckTypedWindowEvent);
      LOAD_POINTER_TO_FUNCTION(libX11, XCloseDisplay, vtkXCloseDisplay);
      LOAD_POINTER_TO_FUNCTION(libX11, XConvertSelection, vtkXConvertSelection);
      LOAD_POINTER_TO_FUNCTION(libX11, XDefaultScreen, vtkXDefaultScreen);
      LOAD_POINTER_TO_FUNCTION(libX11, XDefineCursor, vtkXDefineCursor);
      LOAD_POINTER_TO_FUNCTION(libX11, XDestroyWindow, vtkXDestroyWindow);
      LOAD_POINTER_TO_FUNCTION(libX11, XDisplayHeight, vtkXDisplayHeight);
      LOAD_POINTER_TO_FUNCTION(libX11, XDisplayWidth, vtkXDisplayWidth);
      LOAD_POINTER_TO_FUNCTION(libX11, XFlush, vtkXFlush);
      LOAD_POINTER_TO_FUNCTION(libX11, XFreeCursor, vtkXFreeCursor);
      LOAD_POINTER_TO_FUNCTION(libX11, XFreeExtensionList, vtkXFreeExtensionList);
      LOAD_POINTER_TO_FUNCTION(libX11, XFreePixmap, vtkXFreePixmap);
      LOAD_POINTER_TO_FUNCTION(libX11, XFree, vtkXFree);
      LOAD_POINTER_TO_FUNCTION(libX11, XGetWindowAttributes, vtkXGetWindowAttributes);
      LOAD_POINTER_TO_FUNCTION(libX11, XGetWindowProperty, vtkXGetWindowProperty);
      LOAD_POINTER_TO_FUNCTION(libX11, XGrabKeyboard, vtkXGrabKeyboard);
      LOAD_POINTER_TO_FUNCTION(libX11, XIfEvent, vtkXIfEvent);
      LOAD_POINTER_TO_FUNCTION(libX11, XLookupString, vtkXLookupString);
      LOAD_POINTER_TO_FUNCTION(libX11, XMapWindow, vtkXMapWindow);
      LOAD_POINTER_TO_FUNCTION(libX11, XMoveWindow, vtkXMoveWindow);
      LOAD_POINTER_TO_FUNCTION(libX11, XNextEvent, vtkXNextEvent);
      LOAD_POINTER_TO_FUNCTION(libX11, XPending, vtkXPending);
      LOAD_POINTER_TO_FUNCTION(libX11, XPutBackEvent, vtkXPutBackEvent);
      LOAD_POINTER_TO_FUNCTION(libX11, XResizeWindow, vtkXResizeWindow);
      LOAD_POINTER_TO_FUNCTION(libX11, XScreenNumberOfScreen, vtkXScreenNumberOfScreen);
      LOAD_POINTER_TO_FUNCTION(libX11, XSelectInput, vtkXSelectInput);
      LOAD_POINTER_TO_FUNCTION(libX11, XSendEvent, vtkXSendEvent);
      LOAD_POINTER_TO_FUNCTION(libX11, XSetClassHint, vtkXSetClassHint);
      LOAD_POINTER_TO_FUNCTION(libX11, XSetErrorHandler, vtkXSetErrorHandler);
      LOAD_POINTER_TO_FUNCTION(libX11, XSetInputFocus, vtkXSetInputFocus);
      LOAD_POINTER_TO_FUNCTION(libX11, XSetNormalHints, vtkXSetNormalHints);
      LOAD_POINTER_TO_FUNCTION(libX11, XSetWMIconName, vtkXSetWMIconName);
      LOAD_POINTER_TO_FUNCTION(libX11, XSetWMName, vtkXSetWMName);
      LOAD_POINTER_TO_FUNCTION(libX11, XSetWMProtocols, vtkXSetWMProtocols);
      LOAD_POINTER_TO_FUNCTION(libX11, XStoreName, vtkXStoreName);
      LOAD_POINTER_TO_FUNCTION(libX11, XStringListToTextProperty, vtkXStringListToTextProperty);
      LOAD_POINTER_TO_FUNCTION(libX11, XSync, vtkXSync);
      LOAD_POINTER_TO_FUNCTION(libX11, XUndefineCursor, vtkXUndefineCursor);
      LOAD_POINTER_TO_FUNCTION(libX11, XUnmapWindow, vtkXUnmapWindow);
      LOAD_POINTER_TO_FUNCTION(libX11, XCreateBitmapFromData, vtkXCreateBitmapFromData);
      LOAD_POINTER_TO_FUNCTION(libX11, XScreenOfDisplay, vtkXScreenOfDisplay);
      LOAD_POINTER_TO_FUNCTION(libX11, XVisualIDFromVisual, vtkXVisualIDFromVisual);
      LOAD_POINTER_TO_FUNCTION(libX11, XCreateWindow, vtkXCreateWindow);
      LOAD_POINTER_TO_FUNCTION(libX11, XRootWindowOfScreen, vtkXRootWindowOfScreen);
      LOAD_POINTER_TO_FUNCTION(libX11, XRootWindow, vtkXRootWindow);
      LOAD_POINTER_TO_FUNCTION(libX11, XGetVisualInfo, vtkXGetVisualInfo);
#if VTK_HAVE_XCURSOR
      for (const char** libName = XCURSOR_LIBRARY_NAMES; *libName != nullptr; ++libName)
      {
        libXcursor = dlopen(*libName, RTLD_LAZY | RTLD_GLOBAL);
        if (libXcursor != nullptr)
        {
          vtkLog(TRACE, "Successfully loaded " << *libName);
          break;
        }
      }
      if (libXcursor == nullptr)
      {
        vtkLog(WARNING, "Failed to load Xcursor library");
        return;
      }
      LOAD_POINTER_TO_FUNCTION(libXcursor, XcursorFilenameLoadCursor, vtkXcursorFilenameLoadCursor);
#endif
    }
  }

  void vtkX11FunctionsFinalize()
  {
    vtkLog(TRACE, "Releasing vtkX11Functions");
    if (--RequestId == 0)
    {
      vtkLog(TRACE, "Freeing X11 function pointers");
      NULLIFY_POINTER_TO_FUNCTION(vtkXInternAtom);
      NULLIFY_POINTER_TO_FUNCTION(vtkXQueryPointer);
      NULLIFY_POINTER_TO_FUNCTION(vtkXTranslateCoordinates);
      NULLIFY_POINTER_TO_FUNCTION(vtkXKeysymToString);
      NULLIFY_POINTER_TO_FUNCTION(vtkXListExtensions);
      NULLIFY_POINTER_TO_FUNCTION(vtkXCreateColormap);
      NULLIFY_POINTER_TO_FUNCTION(vtkXCreateFontCursor);
      NULLIFY_POINTER_TO_FUNCTION(vtkXCreatePixmapCursor);
      NULLIFY_POINTER_TO_FUNCTION(vtkXOpenDisplay);
      NULLIFY_POINTER_TO_FUNCTION(vtkXCreateGC);
      NULLIFY_POINTER_TO_FUNCTION(vtkXDefaultScreenOfDisplay);
      NULLIFY_POINTER_TO_FUNCTION(vtkXChangeProperty);
      NULLIFY_POINTER_TO_FUNCTION(vtkXChangeWindowAttributes);
      NULLIFY_POINTER_TO_FUNCTION(vtkXCheckIfEvent);
      NULLIFY_POINTER_TO_FUNCTION(vtkXCheckTypedWindowEvent);
      NULLIFY_POINTER_TO_FUNCTION(vtkXCloseDisplay);
      NULLIFY_POINTER_TO_FUNCTION(vtkXConvertSelection);
      NULLIFY_POINTER_TO_FUNCTION(vtkXDefaultScreen);
      NULLIFY_POINTER_TO_FUNCTION(vtkXDefineCursor);
      NULLIFY_POINTER_TO_FUNCTION(vtkXDestroyWindow);
      NULLIFY_POINTER_TO_FUNCTION(vtkXDisplayHeight);
      NULLIFY_POINTER_TO_FUNCTION(vtkXDisplayWidth);
      NULLIFY_POINTER_TO_FUNCTION(vtkXFlush);
      NULLIFY_POINTER_TO_FUNCTION(vtkXFreeCursor);
      NULLIFY_POINTER_TO_FUNCTION(vtkXFreeExtensionList);
      NULLIFY_POINTER_TO_FUNCTION(vtkXFreePixmap);
      NULLIFY_POINTER_TO_FUNCTION(vtkXFree);
      NULLIFY_POINTER_TO_FUNCTION(vtkXGetWindowAttributes);
      NULLIFY_POINTER_TO_FUNCTION(vtkXGetWindowProperty);
      NULLIFY_POINTER_TO_FUNCTION(vtkXGrabKeyboard);
      NULLIFY_POINTER_TO_FUNCTION(vtkXIfEvent);
      NULLIFY_POINTER_TO_FUNCTION(vtkXLookupString);
      NULLIFY_POINTER_TO_FUNCTION(vtkXMapWindow);
      NULLIFY_POINTER_TO_FUNCTION(vtkXMoveWindow);
      NULLIFY_POINTER_TO_FUNCTION(vtkXNextEvent);
      NULLIFY_POINTER_TO_FUNCTION(vtkXPending);
      NULLIFY_POINTER_TO_FUNCTION(vtkXPutBackEvent);
      NULLIFY_POINTER_TO_FUNCTION(vtkXResizeWindow);
      NULLIFY_POINTER_TO_FUNCTION(vtkXScreenNumberOfScreen);
      NULLIFY_POINTER_TO_FUNCTION(vtkXSelectInput);
      NULLIFY_POINTER_TO_FUNCTION(vtkXSendEvent);
      NULLIFY_POINTER_TO_FUNCTION(vtkXSetClassHint);
      NULLIFY_POINTER_TO_FUNCTION(vtkXSetErrorHandler);
      NULLIFY_POINTER_TO_FUNCTION(vtkXSetInputFocus);
      NULLIFY_POINTER_TO_FUNCTION(vtkXSetNormalHints);
      NULLIFY_POINTER_TO_FUNCTION(vtkXSetWMIconName);
      NULLIFY_POINTER_TO_FUNCTION(vtkXSetWMName);
      NULLIFY_POINTER_TO_FUNCTION(vtkXSetWMProtocols);
      NULLIFY_POINTER_TO_FUNCTION(vtkXStoreName);
      NULLIFY_POINTER_TO_FUNCTION(vtkXStringListToTextProperty);
      NULLIFY_POINTER_TO_FUNCTION(vtkXSync);
      NULLIFY_POINTER_TO_FUNCTION(vtkXUndefineCursor);
      NULLIFY_POINTER_TO_FUNCTION(vtkXUnmapWindow);
      NULLIFY_POINTER_TO_FUNCTION(vtkXCreateBitmapFromData);
      NULLIFY_POINTER_TO_FUNCTION(vtkXScreenOfDisplay);
      NULLIFY_POINTER_TO_FUNCTION(vtkXVisualIDFromVisual);
      NULLIFY_POINTER_TO_FUNCTION(vtkXCreateWindow);
      NULLIFY_POINTER_TO_FUNCTION(vtkXRootWindowOfScreen);
      NULLIFY_POINTER_TO_FUNCTION(vtkXRootWindow);
      NULLIFY_POINTER_TO_FUNCTION(vtkXGetVisualInfo);

// Xcursor API
#if VTK_HAVE_XCURSOR
      NULLIFY_POINTER_TO_FUNCTION(vtkXcursorFilenameLoadCursor);
#endif
      if (libX11)
      {
        dlclose(libX11);
        libX11 = nullptr;
      }
      if (libXcursor)
      {
        dlclose(libXcursor);
        libXcursor = nullptr;
      }
    }
  }
}
#else  // defined(VTK_USE_X)
extern "C"
{
  void vtkX11FunctionsInitialize()
  {
    vtkLog(WARNING, "vtkX11FunctionsInitialize called but VTK was built with VTK_USE_X=OFF");
  }
  void vtkX11FunctionsFinalize()
  {
    vtkLog(WARNING, "vtkX11FunctionsFinalize called but VTK was built with VTK_USE_X=OFF");
  }
}
#endif // defined(VTK_USE_X)
