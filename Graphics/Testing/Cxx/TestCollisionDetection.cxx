#include "vtkProperty.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkCamera.h"
#include "vtkSphereSource.h"
#include "vtkMatrix4x4.h"
#include "vtkCollisionDetectionFilter.h"
#include "vtkGlyph3D.h"
#include "vtkInteractorStyleJoystickActor.h"
#include "vtkTextActor.h"
#include "vtkCommand.h"
#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"
#include <ostream>


class vtkCollisionCallback : public vtkCommand
{
public:
  static vtkCollisionCallback *New() 
    { return new vtkCollisionCallback; }

  void SetTextActor(vtkTextActor *txt)
    {
    this->TextActor = txt;
    }
  void SetRenderWindow(vtkRenderWindow *renWin)
    {
    this->RenWin = renWin;
    }

  virtual void Execute(vtkObject *caller, unsigned long, void*)
    {
      vtkCollisionDetectionFilter *collide = reinterpret_cast<vtkCollisionDetectionFilter*>(caller);
      if (collide->GetNumberOfContacts() > 0)
        {
        sprintf(this->TextBuff, "Number Of Contacts: %d", collide->GetNumberOfContacts());
        }
      else
        {
        sprintf(this->TextBuff, "No Contacts");
        }
      this->TextActor->SetInput(this->TextBuff);
      this->RenWin->Render();
    }
protected:
  vtkTextActor *TextActor;
  vtkRenderWindow *RenWin;
  char TextBuff[128];
};

int TestCollisionDetection(int argc, char* argv[])
{
   vtkSphereSource *sphere0 = vtkSphereSource::New();
   sphere0->SetPhiResolution(3);
   sphere0->SetThetaResolution(3);
   sphere0->SetCenter(-0.0, 0, 0);

   vtkSphereSource *sphere1 = vtkSphereSource::New();
   sphere1->SetPhiResolution(30);
   sphere1->SetThetaResolution(30);
   sphere1->SetRadius(0.3);

   vtkMatrix4x4 *matrix0 = vtkMatrix4x4::New();
   vtkMatrix4x4 *matrix1 = vtkMatrix4x4::New();

   vtkCollisionDetectionFilter *collide = vtkCollisionDetectionFilter::New();
   collide->SetInputConnection(0, sphere0->GetOutputPort());
   collide->SetMatrix(0, matrix0);
   collide->SetInputConnection(1, sphere1->GetOutputPort());
   collide->SetMatrix(1, matrix1);
   collide->SetBoxTolerance(0.0);
   collide->SetCellTolerance(0.0);
   collide->SetNumberOfCellsPerNode(2);
   collide->SetCollisionModeToAllContacts();
   collide->GenerateScalarsOn();

   vtkPolyDataMapper *mapper1 = vtkPolyDataMapper::New();
   mapper1->SetInputConnection(collide->GetOutputPort(0));
   vtkActor *actor1 = vtkActor::New();
   actor1->SetMapper(mapper1);
   (actor1->GetProperty())->BackfaceCullingOn();
   actor1->SetUserMatrix(matrix0);

   vtkPolyDataMapper *mapper2 = vtkPolyDataMapper::New();
   mapper2->SetInputConnection(collide->GetOutputPort(1));
   vtkActor *actor2 = vtkActor::New();
   actor2->SetMapper(mapper2);
   (actor2->GetProperty())->BackfaceCullingOn();
   actor2->SetUserMatrix(matrix1);

   vtkPolyDataMapper *mapper3 = vtkPolyDataMapper::New();
   mapper3->SetInputConnection(collide->GetContactsOutputPort());
   mapper3->SetResolveCoincidentTopologyToPolygonOffset();
   vtkActor *actor3 = vtkActor::New();
   actor3->SetMapper(mapper3);
   (actor3->GetProperty())->SetColor(0,0,0);
   (actor3->GetProperty())->SetLineWidth(3.0);

   vtkTextActor *txt = vtkTextActor::New();

   vtkRenderer *ren = vtkRenderer::New();
   ren->AddActor(actor1);
   ren->AddActor(actor2);
   ren->AddActor(actor3);
   ren->AddActor(txt);
   ren->SetBackground(0.5,0.5,0.5);

   vtkRenderWindow *renWin = vtkRenderWindow::New();
   renWin->AddRenderer(ren);

   vtkInteractorStyleJoystickActor *istyle = vtkInteractorStyleJoystickActor::New();
   vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
   iren->SetRenderWindow(renWin);
   iren->SetInteractorStyle(istyle);

   vtkCollisionCallback *cbCollide = vtkCollisionCallback::New();
   cbCollide->SetTextActor(txt);
   cbCollide->SetRenderWindow(renWin);
   collide->AddObserver(vtkCommand::EndEvent, cbCollide);

   renWin->Render();

 
  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  return !retVal;

   sphere0->Delete();
   sphere1->Delete();
   matrix0->Delete();
   matrix1->Delete();
   collide->Delete();
   mapper1->Delete();
   mapper2->Delete();
   mapper3->Delete();
   actor1->Delete();
   actor2->Delete();
   actor3->Delete();
   txt->Delete();
   ren->Delete();
   cbCollide->Delete();
   renWin->Delete();
   istyle->Delete();
   iren->Delete();




}
