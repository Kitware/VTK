/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you want to add, delete, or rename functions or slots, use
** Qt Designer to update this file, preserving your code.
**
** You should not define a constructor or destructor in this file.
** Instead, write your code in functions called init() and destroy().
** These will automatically be called by the form's constructor and
** destructor.
*****************************************************************************/

/*========================================================================
 For general information about using VTK and Qt, see:
 http://www.trolltech.com/products/3rdparty/vtksupport.html
=========================================================================*/

/*========================================================================
 !!! WARNING for those who want to contribute code to this file.
 !!! If you use a commercial edition of Qt, you can modify this code.
 !!! If you use an open source version of Qt, you are free to modify
 !!! and use this code within the guidelines of the GPL license.
 !!! Unfortunately, you cannot contribute the changes back into this
 !!! file.  Doing so creates a conflict between the GPL and BSD-like VTK
 !!! license.
=========================================================================*/


#include "vtkSphereSource.h"
#include "vtkConeSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"

#if QT_VERSION >= 0x040000
#include "q3popupmenu.h"
#define QPopupMenu Q3PopupMenu
#endif

void Form1::init()
{
  // create a window to make it stereo capable and give it to QVTKWidget
  vtkRenderWindow* renwin = vtkRenderWindow::New();
  renwin->StereoCapableWindowOn();
  qVTK1->SetRenderWindow(renwin);
  renwin->Delete();

  // add a renderer
  ren1 = vtkRenderer::New();
  qVTK1->GetRenderWindow()->AddRenderer(ren1);

  // add a popup menu for the window and connect it to our slot
  QPopupMenu* popup1 = new QPopupMenu(qVTK1);
  popup1->insertItem("Background White", 1);
  popup1->insertItem("Background Black", 2);
  popup1->insertItem("Stereo Rendering", 3);
  connect(popup1, SIGNAL(activated(int)), this, SLOT(color1(int)));
  
  // put cone in one window
  vtkConeSource* cone = vtkConeSource::New();
  vtkPolyDataMapper* mapper = vtkPolyDataMapper::New();
  mapper->SetInput(cone->GetOutput());
  vtkActor* actor = vtkActor::New();
  actor->SetMapper(mapper);
  ren1->AddViewProp(actor);
  actor->Delete();
  mapper->Delete();
  cone->Delete();
      
  // create a window to make it stereo capable and give it to QVTKWidget
  renwin = vtkRenderWindow::New();
  renwin->StereoCapableWindowOn();
  qVTK2->SetRenderWindow(renwin);
  renwin->Delete();

  // add a renderer
  ren2 = vtkRenderer::New();
  qVTK2->GetRenderWindow()->AddRenderer(ren2);
  
  // add a popup menu for the window and connect it to our slot
  QPopupMenu* popup2 = new QPopupMenu(qVTK2);
  popup2->insertItem("Background White", 1);
  popup2->insertItem("Background Black", 2);
  popup2->insertItem("Stereo Rendering", 3);
  connect(popup2, SIGNAL(activated(int)), this, SLOT(color2(int)));
  
  // put sphere in other window
  vtkSphereSource* sphere = vtkSphereSource::New();
  mapper = vtkPolyDataMapper::New();
  mapper->SetInput(sphere->GetOutput());
  actor = vtkActor::New();
  actor->SetMapper(mapper);
  ren2->AddViewProp(actor);
  actor->Delete();
  mapper->Delete();
  sphere->Delete();


  connections = vtkEventQtSlotConnect::New();

  // get right mouse pressed with high priority
  connections->Connect(qVTK1->GetRenderWindow()->GetInteractor(),
                       vtkCommand::RightButtonPressEvent,
                       this,
                       SLOT(popup( vtkObject*, unsigned long, void*, vtkCommand*)),
                       popup1, 1.0);
  
  // get right mouse pressed with high priority
  connections->Connect(qVTK2->GetRenderWindow()->GetInteractor(),
                       vtkCommand::RightButtonPressEvent,
                       this,
                       SLOT(popup( vtkObject*, unsigned long, void*, vtkCommand*)),
                       popup2, 1.0);
  
  // connect window enter event to radio button slot
  connections->Connect(qVTK1->GetRenderWindow()->GetInteractor(), 
                       vtkCommand::EnterEvent,
                       radio1, 
                       SLOT(animateClick()));
  
  // connect window enter event to radio button slot
  connections->Connect(qVTK2->GetRenderWindow()->GetInteractor(), 
                       vtkCommand::EnterEvent,
                       radio2, 
                       SLOT(animateClick()));
  
  // update coords as we move through the window
  connections->Connect(qVTK1->GetRenderWindow()->GetInteractor(),
                       vtkCommand::MouseMoveEvent,
                       this,
                       SLOT(updateCoords(vtkObject*)));
  
  // update coords as we move through the window
  connections->Connect(qVTK2->GetRenderWindow()->GetInteractor(),
                       vtkCommand::MouseMoveEvent,
                       this,
                       SLOT(updateCoords(vtkObject*)));

  connections->PrintSelf(cout, vtkIndent());
  
}

void Form1::destroy()
{
  ren1->Delete();
  ren2->Delete();

  connections->Delete();
}



void Form1::fileExit()
{
}


void Form1::updateCoords( vtkObject * obj)
{
  // get interactor
  vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::SafeDownCast(obj);
  // get event position
  int event_pos[2];
  iren->GetEventPosition(event_pos);
  // update label
  QString str;
  str.sprintf("x=%d : y=%d", event_pos[0], event_pos[1]);
  coord->setText(str);

}


void Form1::popup( vtkObject * obj, unsigned long , void * client_data, vtkCommand* command)
{

  // A note about context menus in Qt and the QVTKWidget
  // You may find it easy to just do context menus on right button up,
  // due to the event proxy mechanism in place.
  
  // That usually works, except in some cases.
  // One case is where you capture context menu events that 
  // child windows don't process.  You could end up with a second 
  // context menu after the first one.

  // See QVTKWidget::ContextMenuEvent enum which was added after the 
  // writing of this example.

  // get interactor
  vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::SafeDownCast(obj);
  // consume event so the interactor style doesn't get it
  command->AbortFlagOn();
  // get popup menu
  QPopupMenu* popup = static_cast<QPopupMenu*>(client_data);
  // get event location
  int* size = iren->GetSize();
  int* pos = iren->GetEventPosition();
  // remember to flip y
  QPoint pt = QPoint(pos[0], size[1]-pos[1]);
  // map to global
  QPoint global_pt = popup->parentWidget()->mapToGlobal(pt);
  // show popup menu at global point
  popup->popup(global_pt);
}



void Form1::color1( int color )
{
  if(color == 1)
    ren1->SetBackground(1,1,1);
  else if(color == 2)
    ren1->SetBackground(0,0,0);
  else if(color == 3)
  {
    ren1->GetRenderWindow()->SetStereoRender(!ren1->GetRenderWindow()->GetStereoRender());
  }
}


void Form1::color2( int color )
{
  if(color == 1)
    ren2->SetBackground(1,1,1);
  else if(color == 2)
    ren2->SetBackground(0,0,0);
  else if(color == 3)
  {
    ren2->GetRenderWindow()->SetStereoRender(!ren2->GetRenderWindow()->GetStereoRender());
  }
}
