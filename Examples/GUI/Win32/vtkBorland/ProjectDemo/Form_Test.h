//---------------------------------------------------------------------------
//
// Demo Borland + vtk Project,
//
//---------------------------------------------------------------------------
#ifndef Form_TestH
#define Form_TestH
//---------------------------------------------------------------------------
// VCL stuff
#include <Classes.hpp>
#include <Controls.hpp>
#include <ExtCtrls.hpp>
#include <StdCtrls.hpp>
#include <Buttons.hpp>
#include <Menus.hpp>
#include <ComCtrls.hpp>
#include <Dialogs.hpp>
//

#include "vtkBorlandRenderWindow.h"
//
class vtkShrinkPolyData;
//---------------------------------------------------------------------------
class TVTK_Form : public TForm {
  __published:  // IDE-managed Components
    TPanel *Panel3;
    TButton *bc1;
    TPanel *BorderWindow;
    TPopupMenu *ModeMenu;
    TMenuItem *TrackBallMode1;
    TMenuItem *JoystickMode1;
    TPopupMenu *WindowMenu;
    TMenuItem *BackgroundColour1;
    TMenuItem *ResetCamera1;
    TColorDialog *backgroundcolor;
    TPanel *Panel2;
    THeaderControl *HeaderControl1;
    TScrollBar *ShrinkScroll;
    TLabel *Label1;
    TMenuItem *FlightMode1;
    TvtkBorlandRenderWindow *vtkWindow1;
    void __fastcall FormDestroy(TObject *Sender);
    void __fastcall bc1Click(TObject *Sender);
    void __fastcall HeaderControl1SectionClick(THeaderControl *HeaderControl, THeaderSection *Section);
    void __fastcall TrackBallMode1Click(TObject *Sender);
    void __fastcall BackgroundColour1Click(TObject *Sender);
    void __fastcall ResetCamera1Click(TObject *Sender);
    void __fastcall ShrinkScrollChange(TObject *Sender);
    void __fastcall vtkWindow1Enter(TObject *Sender);
    void __fastcall vtkWindow1Exit(TObject *Sender);
    void __fastcall FormShow(TObject *Sender);

  private:
  public:
    __fastcall TVTK_Form(TComponent* Owner);

    vtkShrinkPolyData *shrink;
};
//---------------------------------------------------------------------------
extern TVTK_Form *VTK_Form;
//---------------------------------------------------------------------------
#endif

