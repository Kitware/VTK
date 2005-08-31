//---------------------------------------------------------------------------
//
// Demo Borland + vtk Project,
//
//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

//---------------------------------------------------------------------------
#include "Form_Test.h"
//
#include "vtkActor.h"
#include "vtkAssemblyPath.h"
#include "vtkAssemblyNode.h"
#include "vtkElevationFilter.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkShrinkPolyData.h"
#include "vtkSphereSource.h"
#include "vtkTriangleFilter.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)
#pragma link "vtkBorlandRenderWindow"
#pragma resource "*.dfm"

TVTK_Form *VTK_Form;
//---------------------------------------------------------------------------
__fastcall TVTK_Form::TVTK_Form(TComponent* Owner) : TForm(Owner)
{
  shrink = NULL;
}
//---------------------------------------------------------------------------
void __fastcall TVTK_Form::FormDestroy(TObject *Sender)
{
  if (shrink)
    {
    shrink->Delete();
    }

  // The release of graphics resources is required here in
  // the event that an actor is switched between solid
  // and wireframe representations.  This cannot be implemented within
  // vtkBorlandRenderWindow, since ReleaseGraphicsResources, when called
  // by a vtkProp's mapper, will cause the internal vtkWin32OpenGLRenderWindow
  // to fail during MakeCurrent.

  vtkRenderer* ren1 = vtkWindow1->GetRenderer();
  vtkRenderWindow* renwin1 = vtkWindow1->GetRenderWindow();

  vtkPropCollection* collection = ren1->GetViewProps();
  if(collection)
    {
    collection->InitTraversal();
    for (int i = 0; i < collection->GetNumberOfItems(); i++)
      {
      vtkActor *actor = vtkActor::SafeDownCast(collection->GetNextProp());
      if(actor)
        {
        actor->ReleaseGraphicsResources(renwin1);
        ren1->RemoveViewProp(actor);
        }
      }
    }
}
//---------------------------------------------------------------------------
void __fastcall TVTK_Form::HeaderControl1SectionClick(THeaderControl *HeaderControl, THeaderSection *Section)
{
  if (Section->Text=="Mode")
    {
    TPoint p = HeaderControl->ClientToScreen(TPoint(0,0));
    ModeMenu->Popup(p.x + Section->Left, p.y - 0);
    }
  else if (Section->Text=="Window")
    {
    TPoint p = HeaderControl->ClientToScreen(TPoint(0,0));
    WindowMenu->Popup(p.x + Section->Left, p.y - 0);
    }
}
//---------------------------------------------------------------------------
void __fastcall TVTK_Form::TrackBallMode1Click(TObject *Sender)
{
  if (Sender==JoystickMode1)
    {
    vtkWindow1->SetInteractorMode(IM_JoystickCamera);
    JoystickMode1->Checked = true;
    }
  if (Sender==TrackBallMode1)
    {
    vtkWindow1->SetInteractorMode(IM_TrackballCamera);
    TrackBallMode1->Checked = true;
    }
  if (Sender==FlightMode1)
    {
    vtkWindow1->SetInteractorMode(IM_Flight);
    FlightMode1->Checked = true;
    }
}
//---------------------------------------------------------------------------
void __fastcall TVTK_Form::BackgroundColour1Click(TObject *Sender)
{
  if (!backgroundcolor->Execute())
    {
    return;
    }
  DWORD  L = ColorToRGB(backgroundcolor->Color);
  double rgb[3] = { GetRValue(L)/255.0, GetGValue(L)/255.0, GetBValue(L)/255.0 };
  vtkWindow1->GetRenderer()->SetBackground(rgb);
  vtkWindow1->Invalidate();
}
//---------------------------------------------------------------------------
void __fastcall TVTK_Form::ResetCamera1Click(TObject *Sender)
{
  vtkWindow1->GetRenderer()->ResetCamera();
  vtkWindow1->Invalidate();
}
//---------------------------------------------------------------------------
//
//
// Here's a demo
//
//
//---------------------------------------------------------------------------
void __fastcall TVTK_Form::bc1Click(TObject *Sender)
{
  if (shrink)
    {
    return;
    }

  vtkSphereSource *sphere = vtkSphereSource::New();
    sphere->SetThetaResolution(36.0);
    sphere->SetPhiResolution(18.0);
    sphere->SetRadius(1.0);

  shrink = vtkShrinkPolyData::New();
    shrink->SetShrinkFactor( ShrinkScroll->Position/100.0 );
    shrink->SetInput( sphere->GetOutput() );

  vtkElevationFilter *elev = vtkElevationFilter::New();
    elev->SetInput( shrink->GetOutput() );
    elev->SetLowPoint(-1,-1,-1);
    elev->SetHighPoint( 1, 1, 1);
    elev->SetScalarRange(0,1);

  vtkPolyDataMapper *aMapper = vtkPolyDataMapper::New();
    aMapper->SetInput( elev->GetPolyDataOutput() );
    aMapper->SetScalarRange(0,1);

  vtkActor *anActor = vtkActor::New();
    anActor->SetMapper(aMapper);

  // Use these functions to get the actual RenderWindow/Renderers.
  vtkWindow1->GetRenderer()->AddActor(anActor);

  // We don't need these any more, they are reference counted by the
  // pipeline and we can delete them. They'll be destructed when everything
  // finishes.  We'll keep a pointer to the shrinkfilter so we can use our
  // scroller.

  anActor->Delete();
  aMapper->Delete();
  sphere->Delete();
  elev->Delete();

  vtkWindow1->GetRenderer()->ResetCamera();
  vtkWindow1->Invalidate();
}
//---------------------------------------------------------------------------
void __fastcall TVTK_Form::ShrinkScrollChange(TObject *Sender)
{
  if (!shrink)
    {
    return;
    }
  shrink->SetShrinkFactor( ShrinkScroll->Position/100.0 );
  vtkWindow1->Invalidate();
}
//---------------------------------------------------------------------------
void __fastcall TVTK_Form::vtkWindow1Enter(TObject *Sender)
{
  BorderWindow->Color = clMaroon;
}
//---------------------------------------------------------------------------
void __fastcall TVTK_Form::vtkWindow1Exit(TObject *Sender)
{
  BorderWindow->Color = clBtnFace;
}
//---------------------------------------------------------------------------

void __fastcall TVTK_Form::FormShow(TObject *Sender)
{
  // These calls are made to enforce creation of the internal
  // vtk components of the vtkBorlandRenderWindow.  If this were
  // not done, clicking on the component would attempt to pass
  // window messages to non-existent entities.  This behaviour
  // could be changed in future.

  vtkRenderWindowInteractor * iact = vtkWindow1->GetInteractor();
  vtkRenderer* ren1 = vtkWindow1->GetRenderer();
}
//---------------------------------------------------------------------------


