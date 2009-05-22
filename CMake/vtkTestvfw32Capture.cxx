#include "windows.h"
#include "vfw.h"

// Can vtkWin32VideoSource.cxx be compiled?
// Test by using some of the structs in a class and
// calling some of the functions Win32VideoSource
// uses...

class vtkWin32VideoSourceInternal
{
public:
  vtkWin32VideoSourceInternal() {}
  HWND CapWnd;
  HWND ParentWnd;
  CAPSTATUS CapStatus;
  CAPDRIVERCAPS CapDriverCaps;
  CAPTUREPARMS CaptureParms;
  LPBITMAPINFO BitMapPtr;
};

int main()
{
  vtkWin32VideoSourceInternal internal;

  internal.CapWnd = capCreateCaptureWindow(
    "Capture", WS_CHILD|WS_VISIBLE, 0, 0, 100, 100, NULL, 1);

//  capDriverConnect
//  capDriverGetCaps
//  capCaptureGetSetup
//  capCaptureSetSetup
  // test if constants used in vtkWin32VideoSource exist.
  int controlcallback_preroll = CONTROLCALLBACK_PREROLL;
  int controlcallback_capturing = CONTROLCALLBACK_CAPTURING;
  int ids_begin = IDS_CAP_BEGIN;
  int ids_cap_end = IDS_CAP_END;
  int avstreammaster = AVSTREAMMASTER_NONE;

  capSetUserData(internal.CapWnd, &internal);

//  capSetCallbackOnCapControl
//  capSetCallbackOnFrame
//  capSetCallbackOnVideoStream
//  capSetCallbackOnStatus
//  capSetCallbackOnError

  (void) capOverlay(internal.CapWnd, TRUE);

  (void) capGetUserData(internal.CapWnd);

  return 0;
}
