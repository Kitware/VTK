//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <vcl\Sysutils.hpp>

#include "vtkBorlandRenderWindow.h"

#include "vtkInteractorStyleSwitch.h"
#include "vtkInteractorStyleFlight.h"
#include "vtkInteractorStyleImage.h"
#include "vtkInteractorStyleUser.h"  

#pragma package(smart_init)

//---------------------------------------------------------------------------
// ValidCtrCheck is used to assure that the components created do not have
// any pure virtual functions.
//
static inline void ValidCtrCheck(TvtkBorlandRenderWindow *)
{
  new TvtkBorlandRenderWindow(NULL);
}

//---------------------------------------------------------------------------
namespace Vtkborlandrenderwindow
{
  void __fastcall PACKAGE Register()
    {
    TComponentClass classes[1] = {__classid(TvtkBorlandRenderWindow)};
    RegisterComponents("Samples", classes, 0);
    }
}

//---------------------------------------------------------------------------
__fastcall TvtkBorlandRenderWindow::TvtkBorlandRenderWindow(TComponent* Owner)
        : inherited(Owner)
{
  // Do want these
  ControlStyle << csCaptureMouse << csClickEvents << csOpaque << csDoubleClicks;
  // Don't want these
  ControlStyle >> csAcceptsControls >> csSetCaption;
  
  FUsevtkInteractor   = true;
  FInteractorMode     = IM_TrackballCamera;
  FInteractor         = 0;
  FOnVtkClose         = 0;
  FRenderWindow       = 0;
  FRenderer           = 0;
  FAbortCallback      = vtkAbortCallback::New();
}

//---------------------------------------------------------------------------
__fastcall TvtkBorlandRenderWindow::~TvtkBorlandRenderWindow()
{
    // Delete this first because Renderwindow has a hold on it too
  if ( FInteractor )
    {
    FInteractor->Delete();
    FInteractor = 0;
    }
  if ( FRenderer )
    {
    FRenderer->GetProps()->RemoveAllItems();
    FRenderWindow->RemoveRenderer(FRenderer);
    FRenderer->Delete();
    FRenderer = 0;
    }
  if ( FRenderWindow )
    {
    FRenderWindow->RemoveObserver(FAbortCallback);
    FRenderWindow->Delete();
    FRenderWindow = 0;
    }
  FAbortCallback->Delete();
}

//---------------------------------------------------------------------------
vtkWin32OpenGLRenderWindow * __fastcall TvtkBorlandRenderWindow::GetRenderWindow(void)
{
  if ( ! FRenderWindow )
    {
    // Stuff the renderwindow into our window
    FRenderWindow = vtkWin32OpenGLRenderWindow::New();
    FRenderWindow->AddObserver( vtkCommand::AbortCheckEvent, FAbortCallback);
    FRenderWindow->SetParentId(Parent->Handle);
    FRenderWindow->SetWindowId(Handle);
    FRenderWindow->DoubleBufferOn();
    FRenderWindow->SwapBuffersOn();
    // Frame to avoid unsightly garbage during initial
    // display which may be long when a complex scene is first rendered
    FRenderWindow->Frame();
    Invalidate();
    }
    // We create the interactor here because it makes maintenance better
  if (!FInteractor)
    {
    FInteractor = vtkWin32RenderWindowInteractor::New();
    FInteractor->SetRenderWindow(FRenderWindow);
    FInteractor->SetInstallMessageProc(false);
    SetInteractorMode(FInteractorMode);
    FInteractor->UpdateSize(Width,Height);
    FInteractor->Initialize();
    }
  return FRenderWindow;
}

//---------------------------------------------------------------------------
vtkWin32RenderWindowInteractor * __fastcall TvtkBorlandRenderWindow::GetInteractor(void)
{
  if (FRenderWindow)
    {
    if (!FInteractor)
      {
      throw Exception("Window exists but no Interactor, this shouldn't happen");
      }  
    }
  else
    {
    this->GetRenderWindow();  
    }
  return FInteractor;
}

//---------------------------------------------------------------------------
vtkRenderer * __fastcall TvtkBorlandRenderWindow::GetRenderer(void)
{
  if (!FRenderer)
    {
    FRenderer = vtkRenderer::New();
    GetRenderWindow()->AddRenderer(FRenderer);
    FRenderer->ResetCamera();
    DWORD  L = ColorToRGB(Color);
    float rgb[3] = { GetRValue(L)/255.0, GetGValue(L)/255.0, GetBValue(L)/255.0 };
    FRenderer->SetBackground(rgb);
    }
  return FRenderer;
}

//---------------------------------------------------------------------------
void __fastcall TvtkBorlandRenderWindow::SetInteractorMode(const vtkBorlandInteractorMode& im)
{
  if ( im <= IM_TrackballActor && im >= IM_JoystickCamera )
    {
    vtkInteractorStyleSwitch *iass = dynamic_cast<vtkInteractorStyleSwitch*>(FInteractor->GetInteractorStyle());
    if (!iass)
      {
      iass = vtkInteractorStyleSwitch::New();
      FInteractor->SetInteractorStyle(iass);
      iass->Delete();
      }

    switch ( im )
      {
      case IM_JoystickCamera: iass->SetCurrentStyleToJoystickCamera(); break;
      case IM_JoystickActor: iass->SetCurrentStyleToJoystickActor(); break;
      case IM_TrackballCamera: iass->SetCurrentStyleToTrackballCamera(); break;
      case IM_TrackballActor: iass->SetCurrentStyleToTrackballActor(); break;
      default: break;
      }
      FInteractorMode = im;
    }
    else if (im==IM_Flight)
      {
      vtkInteractorStyleFlight *iafl = dynamic_cast<vtkInteractorStyleFlight*>(FInteractor->GetInteractorStyle());
      if (!iafl)
        {
        iafl = vtkInteractorStyleFlight::New();
        FInteractor->SetInteractorStyle(iafl);
        iafl->Delete();
        }
      FInteractorMode = IM_Flight;
      }
    else if (im==IM_Image)
      {
      vtkInteractorStyleImage *iasi = dynamic_cast<vtkInteractorStyleImage*>(FInteractor->GetInteractorStyle());
      if (!iasi)
        {
        iasi = vtkInteractorStyleImage::New();
        FInteractor->SetInteractorStyle(iasi);
        iasi->Delete();
        }
      FInteractorMode = IM_Image;
      }
    else if (im==IM_User)
      {
      vtkInteractorStyleUser *iasu = dynamic_cast<vtkInteractorStyleUser*>(FInteractor->GetInteractorStyle());
      if (!iasu)
        {
        iasu = vtkInteractorStyleUser::New();
        FInteractor->SetInteractorStyle(iasu);
        iasu->Delete();
        }
      FInteractorMode = IM_User;
      }
    else
      {
      return;
      }
}

//---------------------------------------------------------------------------
// Paint
//---------------------------------------------------------------------------
void __fastcall TvtkBorlandRenderWindow::Paint(void)
{
  if (FRenderWindow)
    {
    try
      {
      FRenderWindow->Render();
      }
    catch (...)
      {
      // Some error trap should occurr here
      ShowMessage("An exception occurred whilst rendering");
      }
    }
  else
    { // Design time or before RenderWindow creation
    inherited::Paint();
    }
}

//---------------------------------------------------------------------------
// Event handlers
//---------------------------------------------------------------------------
void __fastcall TvtkBorlandRenderWindow::WMEraseBkgnd(TWMEraseBkgnd &Message)
{
  if (!FRenderWindow)
    {
    inherited::Dispatch(&Message);
    }
  else
    {
    Message.Result = 1; // No, but thanks for asking.
    } 
}

//---------------------------------------------------------------------------
void __fastcall TvtkBorlandRenderWindow::WMGetDlgCode(TMessage &Message)
{
  Message.Result = DLGC_WANTARROWS;
}

//---------------------------------------------------------------------------
void __fastcall TvtkBorlandRenderWindow::WMChar(TWMKey &Message)
{
  switch (Message.CharCode)
    {
    case 'e':
    case 'E':
    case 'q':
    case 'Q': if (!FOnVtkClose || (FOnVtkClose && FOnVtkClose(this)))
                {
                FInteractor->OnChar(Handle,Message.CharCode, 0, 0);
                }
              break;
    default:  FInteractor->OnChar(Handle,Message.CharCode, 0, 0);
    }
}

//---------------------------------------------------------------------------
void __fastcall TvtkBorlandRenderWindow::WMKeyDown(TWMKey &Message)
{
  FInteractor->OnKeyDown(Handle,Message.CharCode, 0, 0);
}

//---------------------------------------------------------------------------
void __fastcall TvtkBorlandRenderWindow::WMKeyUp(TWMKey &Message)
{
  FInteractor->OnKeyUp(Handle,Message.CharCode, 0, 0);
}

//---------------------------------------------------------------------------
void __fastcall TvtkBorlandRenderWindow::MouseMove(TShiftState shift, int x, int y )
{
  if(this->OnMouseMove && shift.Contains(ssCtrl))
    {
    this->OnMouseMove(this,shift,x,y);
    return;
    }

  if (FInteractor && FUsevtkInteractor)
    {
    int flags = 0;
    if (shift.Contains(ssShift)) flags += MK_SHIFT;
    if (shift.Contains(ssCtrl))  flags += MK_CONTROL;
    FInteractor->OnMouseMove(Handle, flags, x, y );
    }
}

//---------------------------------------------------------------------------
void __fastcall TvtkBorlandRenderWindow::MouseDown(TMouseButton button, TShiftState shift, int x, int y )
{
  if (::GetFocus()!=Handle) SetFocus();

  if(this->OnMouseDown && shift.Contains(ssCtrl))
    {
    this->OnMouseDown(this,button,shift,x,y);
    return;
    }

  if (FInteractor && FUsevtkInteractor)
    {
    int flags = 0;
    if (shift.Contains(ssShift)) flags += MK_SHIFT;
    if (shift.Contains(ssCtrl))  flags += MK_CONTROL;
    switch (button)
      {
      case mbLeft:   FInteractor->OnLButtonDown(Handle, flags, x,y); break;
      case mbRight:  FInteractor->OnRButtonDown(Handle, flags, x,y); break;
      case mbMiddle: FInteractor->OnMButtonDown(Handle, flags, x,y); break;
      }
    }
}

//---------------------------------------------------------------------------
void __fastcall TvtkBorlandRenderWindow::MouseUp(TMouseButton button, TShiftState shift, int x, int y )
{
  if(this->OnMouseUp  && shift.Contains(ssCtrl))
    {
    this->OnMouseUp(this,button,shift,x,y);
    return;
    }

  if (FInteractor && FUsevtkInteractor)
    {
    int flags = 0;
    if (shift.Contains(ssShift)) flags += MK_SHIFT;
    if (shift.Contains(ssCtrl))  flags += MK_CONTROL;
    switch (button)
      {
      case mbLeft:   FInteractor->OnLButtonUp(Handle, flags, x,y); break;
      case mbRight:  FInteractor->OnRButtonUp(Handle, flags, x,y); break;
      case mbMiddle: FInteractor->OnMButtonUp(Handle, flags, x,y); break;
      }
    }
}

//---------------------------------------------------------------------------
void __fastcall TvtkBorlandRenderWindow::Resize(void)
{
  if (FInteractor)
    {
    FInteractor->OnSize(this->Handle, 0, this->Width, this->Height);
    }
}

//---------------------------------------------------------------------------
void __fastcall TvtkBorlandRenderWindow::WMTimer(TWMTimer &Message)
{
  if (FInteractor)
    {
    FInteractor->OnTimer(Handle,Message.TimerID);
    }
}




