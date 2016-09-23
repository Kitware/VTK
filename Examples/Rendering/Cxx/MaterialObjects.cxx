/*=========================================================================

  Program:   Visualization Toolkit
  Module:    MaterialObjects.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This examples demonstrates the effect different materials.
//
#include "vtkTexturedSphereSource.h"
#include "vtkCubeSource.h"
#include "vtkCylinderSource.h"
#include "vtkPlaneSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkProperty.h"
#include "vtkCamera.h"
#include "vtkLight.h"

#include <vector>



vtkActor* makeActor( const char* type, const char* material )
{
  if( !type && !material )
  {
    return NULL;
  }

  cout << "\t" << material << endl;
  vtkPolyDataMapper *mapper = vtkPolyDataMapper::New();
  mapper->ImmediateModeRenderingOn();

  if( strcmp(type,"sphere")==0 )
  {
    vtkTexturedSphereSource *sphere = vtkTexturedSphereSource::New();
    sphere->SetThetaResolution(25);
    sphere->SetPhiResolution(25);
    mapper->SetInputConnection(sphere->GetOutputPort());
    sphere->Delete();
  }
  else if( strcmp(type,"cube")==0 )
  {
    vtkCubeSource *cube= vtkCubeSource::New();
    mapper->SetInputConnection(cube->GetOutputPort());
    cube->Delete();
  }
  else if( strcmp(type,"cylinder")==0 )
  {
    vtkCylinderSource *cylinder= vtkCylinderSource::New();
    mapper->SetInputConnection(cylinder->GetOutputPort());
    cylinder->Delete();
  }
  else if( strcmp(type,"plane")==0 )
  {
    vtkPlaneSource *plane= vtkPlaneSource::New();
    mapper->SetInputConnection(plane->GetOutputPort());
    plane->Delete();
  }


  vtkActor *actor = vtkActor::New();
  actor->GetProperty()->SetColor(1,0,0);
  actor->GetProperty()->SetAmbient(0.3);
  actor->GetProperty()->SetDiffuse(0.0);
  actor->GetProperty()->SetSpecular(1.0);
  actor->GetProperty()->SetSpecularPower(5.0);
  actor->GetProperty()->LoadMaterial( material );

  double appVar1[4] = {0.37714, 0.61465, 0.48399, 0.68252};
  double appVar2[4] = {0.03900, 0.15857, 0.57913, 0.54458};
  double appVar3[4] = {0.97061, 0.86053, 0.63583, 0.51058};
  double appVar4[4] = {0.12885, 0.91490, 0.86394, 0.58951};
  double appVar5[4] = {0.23403, 0.35340, 0.52559, 0.77830};
  double appVar6[4] = {0.19550, 0.17429, 0.89958, 0.15063};
  double appVar7[4] = {0.75796, 0.48072, 0.07728, 0.16434};

  actor->GetProperty()->AddShaderVariable("appVar1", 4, appVar1);
  actor->GetProperty()->AddShaderVariable("appVar2", 4, appVar2);
  actor->GetProperty()->AddShaderVariable("appVar3", 4, appVar3);
  actor->GetProperty()->AddShaderVariable("appVar4", 4, appVar4);
  actor->GetProperty()->AddShaderVariable("appVar5", 4, appVar5);
  actor->GetProperty()->AddShaderVariable("appVar6", 4, appVar6);
  actor->GetProperty()->AddShaderVariable("appVar7", 4, appVar7);

  actor->GetProperty()->ShadingOn();
  actor->SetMapper(mapper);

  mapper->Delete();

  return actor;

}

void gridLayoutActors( std::vector<vtkActor*> actors )
{
  if( (int)actors.size() <= 1 )
  {
    return;
  }

  double bounds[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

  std::vector<vtkActor*>::iterator it = actors.begin();
  std::vector<vtkActor*>::iterator itEnd = actors.end();
  while( it != itEnd )
  {
    // move to the origin
    (*it)->SetPosition( 0.0, 0.0, 0.0 );
    double *b = (*it)->GetBounds();
    // X
    if( b[0]<bounds[0] ) bounds[0] = b[0];
    if( b[1]>bounds[1] ) bounds[1] = b[1];

    // Y
    if( b[2]<bounds[2] ) bounds[2] = b[2];
    if( b[3]>bounds[3] ) bounds[3] = b[3];

    // Z
    if( b[4]<bounds[4] ) bounds[4] = b[4];
    if( b[5]>bounds[5] ) bounds[5] = b[5];
    it++;
  }

  double step[3] = {1.25 * (bounds[1]-bounds[0]),
                    1.25 * (bounds[3]-bounds[2]),
                    1.25 * (bounds[5]-bounds[4])};


  double dim = ceil( pow( (double)actors.size(), 0.5 ) );
  for( int i=0; i<dim; i++ )
  {
    for( int j=0; j<dim; j++ )
    {
      int id = i*(int)dim+j;
      if( (id) < (int)actors.size() )
      {
#if 0
        if(i>0)
        {
          actors[id]->RotateX(15.0 * i );
          actors[id]->RotateY(15.0 * j);
          actors[id]->RotateZ(15.0);
        }
#endif
        actors[id]->AddPosition( i*step[0], j*step[1], 0.0 );
#if 0
        cout << id << " : ";
        cout << actors[id]->GetPosition()[0] << " , ";
        cout << actors[id]->GetPosition()[1] << " , ";
        cout << actors[id]->GetPosition()[2] << endl;
#endif
      }
    }
#if 0
    cout << endl;
#endif
  }
}

int main(int argc, char* argv[])
{
#if 0
  if( argc == 1 )
  {
    cout << "Syntax: MaterialObjects obj0 material0 obj1 material1...objN materialN" << endl;
    cout << "\tobji may be one of : arrow - vtkArrowSource" << endl;
    cout << "                       cone - vtkConeSource" << endl;
    cout << "                       cube - vtkCubeSource" << endl;
    cout << "                       cylinder - vtkCylinderSource" << endl;
    cout << "                       disk - vtkDiskSource" << endl;
    cout << "                       line - vtkLineSource" << endl;
    cout << "                       plane - vtkPlaneSource" << endl;
    cout << "                       sphere - vtkSphereSource" << endl;
#if 0    cout << "                       superquadric - vtkSuperQuadricSource" << endl;
#endif
    cout << "                       plTetrahedron - vtkPlatonicSolidSource" << endl;
    cout << "                       plCube - vtkPlatonicSolidSource" << endl;
    cout << "                       plOctahedron - vtkPlatonicSolidSource" << endl;
    cout << "                       plIcosahedron - vtkPlatonicSolidSource" << endl;
    cout << "                       plDodecahedron - vtkPlatonicSolidSource" << endl;
    cout << "                       texturedSphere - vtkPlatonicSolidSource" << endl;
    cout << "                       superQuadratic - vtkPlatonicSolidSource" << endl;
    cout << "\tmateriali may be any material in VTK/Utilities/Materials." << endl;
  }
#endif

  cout << "Syntax: MaterialObjects material0 material1 ... materialn" << endl;
  cout << "Apply the nth material to the nth sphere, 0 <= n <= 7" << endl;

  std::vector<int> geom;
  std::vector<int> mat;
  int numActors = 0;
  int count=0;
  int i = 0;

  for( i=1; i<argc; i++ )
  {
    if( count==0 )
    {
      geom.push_back( i );
      numActors++;
      count++;
    }
    else if( count==1 )
    {
      mat.push_back( i );
      count = 0;
    }
  }

  std::vector<vtkActor*> actors;
  i = 0;
  for( i=0; i<numActors; i++ )
  {
    if( i < (int)geom.size() && i < (int)mat.size() )
    {
      if( geom[i] < argc && mat[i] < argc )
      {
        actors.push_back( makeActor(argv[geom[i]],argv[mat[i]]) );
      }
    }
  }

  // layout actore in a grid
  gridLayoutActors( actors );


  // Create the graphics structure. The renderer renders into the
  // render window. The render window interactor captures mouse events
  // and will perform appropriate camera or actor manipulation
  // depending on the nature of the events.
  //
  vtkRenderer *ren1 = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren1);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  // Add the actors to the renderer, set the background and size.
  std::vector<vtkActor*>::iterator it = actors.begin();
  std::vector<vtkActor*>::iterator itEnd = actors.end();
  while( it != itEnd )
  {
    ren1->AddActor(*it);
    it++;
  }

  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(400, 200);

  // Set up the lighting.
  //
  vtkLight *light = vtkLight::New();
  light->SetFocalPoint(1.875,0.6125,0);
  light->SetPosition(0.875,1.6125,1);
  ren1->AddLight(light);

  vtkLight *light2 = vtkLight::New();
  light2->SetFocalPoint(1.875,0.6125,0);
  light2->SetPosition(0.875,1.6125,1);
  ren1->AddLight(light2);

  // We want to eliminate perspective effects on the apparent lighting.
  // Parallel camera projection will be used. To zoom in parallel projection
  // mode, the ParallelScale is set.
  //
  ren1->GetActiveCamera()->SetFocalPoint(0,0,0);
  ren1->GetActiveCamera()->SetPosition(0,0,1);
  ren1->GetActiveCamera()->SetViewUp(0,1,0);
  ren1->GetActiveCamera()->ParallelProjectionOff();
  ren1->ResetCamera();
#if 0
  ren1->GetActiveCamera()->SetParallelScale(1.5);
#endif

  // This starts the event loop and invokes an initial render.
  //
  iren->Initialize();
  iren->Start();

#if 0
  // Exiting from here, we have to delete all the instances that
  // have been created.
  //
  count = 0;
  it = actors.begin();
  while( it != itEnd )
  {
    cout << "Actor " << count << " : " << (*it)->GetPosition()[0] << ", "
                                       << (*it)->GetPosition()[1] << ", "
                                       << (*it)->GetPosition()[2] << endl;
    (*it)->Delete();
    it++;
    count++;
  }
#endif

  ren1->Delete();
  renWin->Delete();
  iren->Delete();
  light->Delete();

  return 0;
}
