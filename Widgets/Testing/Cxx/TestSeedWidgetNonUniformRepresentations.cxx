/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSeedWidgetNonUniformRepresentations.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the vtkSeedWidget by instantiating it with handles
// composed of varied geometric representations and constraints.
// There are 4 handles. They are composed of heterogeneous representations.
// One of them is passive and does not respond to user interaction.
// It also shows how they are placed in a non-interacitve mode (ie
// programmatically).

#include "vtkSeedWidget.h"
#include "vtkSeedRepresentation.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCommand.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkCoordinate.h"
#include "vtkMath.h"
#include "vtkHandleWidget.h"
#include "vtkPointHandleRepresentation3D.h"
#include "vtkAxisActor2D.h"
#include "vtkProperty2D.h"
#include "vtkOrientedPolygonalHandleRepresentation3D.h"
#include "vtkPointHandleRepresentation3D.h"
#include "vtkProperty.h"
#include "vtkGlyphSource2D.h"
#include "vtkPolyData.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

// This callback is responsible for setting the seed label.
class vtkSeedCallback2 : public vtkCommand
{
public:
  static vtkSeedCallback2 *New() 
    { return new vtkSeedCallback2; }
  virtual void Execute( vtkObject *o, unsigned long event, void* )
    {
    vtkSeedWidget *sw = vtkSeedWidget::SafeDownCast(o);
    if (sw && event == vtkCommand::PlacePointEvent)
      {
      std::cout << "Point placed, total of:" << this->SeedRepresentation->GetNumberOfSeeds() << "\n";
      }
    }

  vtkSeedCallback2() : SeedRepresentation(0) {}
  vtkSeedRepresentation *SeedRepresentation;
};


// The actual test function
int TestSeedWidgetNonUniformRepresentations(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  VTK_CREATE( vtkSphereSource, ss );
  VTK_CREATE( vtkPolyDataMapper, mapper );
  VTK_CREATE( vtkActor, actor );
  VTK_CREATE( vtkRenderer, ren );
  VTK_CREATE( vtkRenderWindow, renWin );
  VTK_CREATE( vtkRenderWindowInteractor, iren );
  VTK_CREATE( vtkSeedWidget, widget );
  VTK_CREATE( vtkSeedRepresentation, seedRep );
  VTK_CREATE( vtkGlyphSource2D, glyphs );
  VTK_CREATE( vtkSeedCallback2, scbk );

  renWin->AddRenderer(ren);
  iren->SetRenderWindow(renWin);
  mapper->SetInput(ss->GetOutput());
  actor->SetMapper(mapper);
  ren->AddActor(actor);
  ren->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(500, 500);
  widget->SetInteractor(iren);
  widget->SetRepresentation(seedRep);
  scbk->SeedRepresentation = seedRep;
  widget->AddObserver(vtkCommand::PlacePointEvent,scbk);

  iren->Initialize();
  renWin->Render();

  widget->EnabledOn();


  // Now add some seeds programmatically...
  
  // First, get out out of the mode where we are interactively defining seeds
  widget->CompleteInteraction();

  // Let's add a seed of type vtkOrientedPolygonalHandleRepresentation3D with
  // a triangle glyph, facing the camera. 
  
  VTK_CREATE( vtkOrientedPolygonalHandleRepresentation3D, handleRep1 );
  glyphs->SetGlyphType( VTK_TRIANGLE_GLYPH );
  glyphs->SetScale( .1 );
  glyphs->Update();
  handleRep1->SetHandle(glyphs->GetOutput());  
  handleRep1->GetProperty()->SetColor(1,0,0);
  handleRep1->SetLabelVisibility(1);
  handleRep1->SetLabelText( "Seed-1" );
  seedRep->SetHandleRepresentation( handleRep1 );
  vtkHandleWidget * handleWidget1 = widget->CreateNewHandle();
  handleWidget1->SetEnabled(1);
  double p1[3] = { .3, .3, .6 };
  seedRep->GetHandleRepresentation(0)->SetWorldPosition( p1 );


  // Let's add a seed of type vtkPointHandleRepresentation3D with
  // a triangle glyph, facing the camera. 
  
  VTK_CREATE( vtkPointHandleRepresentation3D, handleRep2 );
  handleRep2->GetProperty()->SetColor(0,1,0);
  seedRep->SetHandleRepresentation( handleRep2 );
  vtkHandleWidget *handleWidget2 = widget->CreateNewHandle();
  handleWidget2->SetEnabled(1);
  double p2[3] = { .3, -.3, .6 };
  seedRep->GetHandleRepresentation(1)->SetWorldPosition( p2 );

  // Let's add a seed of type vtkOrientedPolygonalHandleRepresentation3D with
  // a triangle glyph, facing the camera. 
  
  VTK_CREATE( vtkOrientedPolygonalHandleRepresentation3D, handleRep3 );
  glyphs->SetGlyphType( VTK_THICKCROSS_GLYPH );
  glyphs->Update();
  handleRep3->SetHandle(glyphs->GetOutput());  
  handleRep3->GetProperty()->SetColor(1,1,0);
  handleRep3->SetLabelVisibility(1);
  handleRep3->SetLabelText( "Seed-2" );
  seedRep->SetHandleRepresentation( handleRep3 );
  vtkHandleWidget * handleWidget3 = widget->CreateNewHandle();
  handleWidget3->SetEnabled(1);
  double p3[3] = { -.3, .3, .6 };
  seedRep->GetHandleRepresentation(2)->SetWorldPosition( p3 );

  
  // Let's add a seed that does not respond to user interaction now.
  
  VTK_CREATE( vtkOrientedPolygonalHandleRepresentation3D, handleRep4 );
  glyphs->SetGlyphType( VTK_DIAMOND_GLYPH );
  glyphs->Update();
  handleRep4->SetHandle(glyphs->GetOutput());  
  handleRep4->GetProperty()->SetColor(1,0,1);
  handleRep4->SetLabelVisibility(1);
  handleRep4->SetLabelText( "Passive\nSeed" );
  seedRep->SetHandleRepresentation( handleRep4 );
  vtkHandleWidget * handleWidget4 = widget->CreateNewHandle();
  handleWidget4->SetEnabled(1);
  handleWidget4->ProcessEventsOff();
  double p4[3] = { -.3, -.3, .6 };
  seedRep->GetHandleRepresentation(3)->SetWorldPosition( p4 );    


  // Render..
  renWin->Render();

  iren->Start();

  return EXIT_SUCCESS;
}


