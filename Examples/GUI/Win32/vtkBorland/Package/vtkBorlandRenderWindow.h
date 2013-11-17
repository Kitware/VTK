//---------------------------------------------------------------------------
#ifndef vtkBorlandRenderWindowH
#define vtkBorlandRenderWindowH
//---------------------------------------------------------------------------
#include <SysUtils.hpp>
#include <Controls.hpp>
#include <Classes.hpp>
#include <Forms.hpp>

#include "vtkCommand.h"
#include "vtkRenderer.h";
#include "vtkWin32OpenGLRenderWindow.h";
#include "vtkWin32RenderWindowInteractor.h";

typedef bool __fastcall (__closure *TvtkBorlandCloseEvent)(TObject *Sender);

// Callback for abort check
class vtkAbortCallback : public vtkCommand
{
public:
  static vtkAbortCallback *New()
    { return new vtkAbortCallback; }
  virtual void Execute(vtkObject *caller, unsigned long, void*)
    {
    vtkWin32OpenGLRenderWindow* ptrWin = reinterpret_cast<vtkWin32OpenGLRenderWindow*>(caller);
    if (ptrWin)
      {
      if(ptrWin->GetEventPending())
        {
        ptrWin->SetAbortRender( 1 );
       // Beep();
        }
      }
    }
  vtkAbortCallback(){}
};

//---------------------------------------------------------------------------
enum vtkBorlandInteractorMode { IM_JoystickCamera,IM_JoystickActor,
                                IM_TrackballCamera, IM_TrackballActor,
                                IM_Flight, IM_Image,IM_User };

//---------------------------------------------------------------------------
class PACKAGE TvtkBorlandRenderWindow : public TCustomControl
{
  typedef TCustomControl inherited;
private:
protected:
   //
    // Events and control related stuff
    //
    DYNAMIC void __fastcall MouseMove(TShiftState shift, int x, int y );
    DYNAMIC void __fastcall MouseDown(TMouseButton button, TShiftState shift, int x, int y );
    DYNAMIC void __fastcall MouseUp(TMouseButton button, TShiftState shift, int x, int y );
    DYNAMIC bool __fastcall DoMouseWheelDown(TShiftState Shift, const TPoint &MousePos);
    DYNAMIC bool __fastcall DoMouseWheelUp(TShiftState Shift, const TPoint &MousePos);

    //
    // Messages : We want to directly intercept these
    //
    void __fastcall WMEraseBkgnd(TWMEraseBkgnd &Message);
    void __fastcall WMGetDlgCode(TMessage &Message);
    void __fastcall WMKeyDown(TWMKey &Message);
    void __fastcall WMKeyUp(TWMKey &Message);
    void __fastcall WMChar(TWMKey &Message);
    void __fastcall WMTimer(TWMTimer &Message);
    // Here's the Dispatch(void &Message) message map for the above functions
    BEGIN_MESSAGE_MAP
        MESSAGE_HANDLER(WM_ERASEBKGND,       TWMEraseBkgnd,      WMEraseBkgnd)
        MESSAGE_HANDLER(WM_GETDLGCODE,       TMessage,           WMGetDlgCode)
        MESSAGE_HANDLER(WM_KEYDOWN,          TWMKeyDown,         WMKeyDown)
        MESSAGE_HANDLER(WM_KEYUP,            TWMKeyUp,           WMKeyUp)
        MESSAGE_HANDLER(WM_CHAR,             TWMChar,            WMChar)
        MESSAGE_HANDLER(WM_TIMER,            TWMTimer,           WMTimer)
    END_MESSAGE_MAP(inherited)
    //
    DYNAMIC void    __fastcall Resize(void);
    //
    // This does all the work
    //
    virtual void __fastcall Paint(void);
    //
    // Our Data Members
    //
    vtkWin32OpenGLRenderWindow      *FRenderWindow;
    vtkRenderer                     *FRenderer;
    vtkWin32RenderWindowInteractor  *FInteractor;
    //
    TvtkBorlandCloseEvent            FOnVtkClose;
    bool                             FUsevtkInteractor;
    vtkBorlandInteractorMode         FInteractorMode;
    vtkAbortCallback                *FAbortCallback;

    //
public:
    //
    // Constructor and Destructor
    //
        __fastcall TvtkBorlandRenderWindow(TComponent* Owner);
    virtual __fastcall ~TvtkBorlandRenderWindow(void);
    //
    // Make user friendly by handling a single rendererer and
    // assorted bits for the user
    //
    virtual vtkWin32OpenGLRenderWindow *      __fastcall GetRenderWindow(void);
    virtual vtkRenderer *                     __fastcall GetRenderer(void);
    virtual vtkWin32RenderWindowInteractor *  __fastcall GetInteractor(void);
    virtual void                              __fastcall SetInteractorMode(const vtkBorlandInteractorMode& im);

__published:
   // Some
    __property bool UsevtkInteractor = {read=FUsevtkInteractor, write=FUsevtkInteractor, default=true, stored=true};
    __property vtkBorlandInteractorMode InteractorMode = {read=FInteractorMode, write=FInteractorMode, default=IM_TrackballCamera, stored=true};
    //
    // Cosmetic Properties inherited from TCustomControl
    //
    __property Align;
    __property Anchors;
    __property Color;
    __property Constraints;
    __property Enabled;
    __property ParentColor;
    __property PopupMenu;
    __property TabOrder;
    __property TabStop;
    __property Visible;
    //
    // Cosmetic Properties inherited from TWinControl
    //
    __property BevelEdges;
    __property BevelInner;
    __property BevelOuter;
    __property BevelKind;
    __property BevelWidth;
    __property BorderWidth;

    //
    // Useful Event notifications
    //
    __property OnCanResize;
    __property OnClick;
    __property OnConstrainedResize;
    __property OnDblClick;
    __property OnDragDrop;
    __property OnDragOver;
    __property OnEnter;
    __property OnExit;
    __property OnResize;
    __property OnStartDrag;
    __property TvtkBorlandCloseEvent OnVtkClose = {read=FOnVtkClose, write=FOnVtkClose};
    //
    // Main events for user interaction
    //
    __property OnKeyUp;
    __property OnKeyDown;
    __property OnKeyPress;
    __property OnMouseDown;
    __property OnMouseMove;
    __property OnMouseUp;
    __property OnMouseWheelDown;
    __property OnMouseWheelUp;
};
//---------------------------------------------------------------------------
#endif

void CheckAbortFunc(void *);


