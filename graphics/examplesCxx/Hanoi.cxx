/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Hanoi.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME Hanoi - application example does 3D towers of hanoi.
// .SECTION Description
// Hanoi -p # -s # -r #
// where -p is the number of starting pucks on the peg; 
//       -s is the number of steps to take during animation;
//       -r is the resolution of each puck
//	 -S save image for regression testing

#include "vtkStack.h"
#include "vtkRenderWindow.h"
#include "vtkActor.h"
#include "vtkMath.h"
#include "vtkRenderer.h"
#include "vtkLight.h"
#include "vtkCamera.h"
#include "vtkCylinderSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkPlaneSource.h"
#include "vtkPolyDataMapper.h"

#include "SaveImage.h"

// Some static variables to define geometry
static int NumberOfPucks; //number of pucks to move
static int NumberOfSteps; // number of steps as puck moves

static float L;    //height of puck
static float H;    //height of peg
static float R;    //radius of peg
static float rMin; //min allowable radius of disks
static float rMax; //max allowable radius of disks
static float D;    //distance between pegs
static vtkStack *pegStack[3]; //keep track of which pucks are on which pegs
static vtkRenderWindow *Renwin; //Window in which rendering occurs

static int NumberOfMoves; //number of times a puck is moved

// Routine is responsible for moving pucks from one peg to the next.
void MovePuck (int peg1, int peg2)
{
  float distance;
  float flipAngle;
  int i;
  vtkActor *movingActor;
  
  NumberOfMoves++;

  // get the actor to move
  movingActor = (vtkActor *)pegStack[peg1]->Pop();

  // get the distance to move up
  distance = (H - (L * (pegStack[peg1]->GetNumberOfItems() - 1)) + rMax) / 
             NumberOfSteps;

  for (i=0; i<NumberOfSteps; i++)
    {
    movingActor->AddPosition(0,distance,0);
    Renwin->Render();
    }

  // get the distance to move across
  distance = (peg2 - peg1) * D / NumberOfSteps;
  flipAngle = 180.0 / NumberOfSteps;
  for (i=0; i<NumberOfSteps; i++)
    {
    movingActor->AddPosition(distance,0,0);
    movingActor->RotateX(flipAngle);
    Renwin->Render();
    if ( NumberOfMoves == 13 && i == 3 ) //for making book image
      {
//      Renwin->Render();
//      Renwin->SetFileName("hanoi1.ppm");
//      Renwin->SaveImageAsPPM();
      }
    }

  // get the distance to move down
  distance = ((L * (pegStack[peg2]->GetNumberOfItems() - 1)) - H - rMax) / 
             NumberOfSteps;

  for (i=0; i<NumberOfSteps; i++)
    {
    movingActor->AddPosition(0,distance,0);
    Renwin->Render();
    }

  pegStack[peg2]->Push(movingActor);

}

static void Hanoi (int n, int peg1, int peg2, int peg3)
{
  if ( n != 1 )
    {
    Hanoi (n-1, peg1, peg3, peg2);
    Hanoi (1, peg1, peg2, peg3);
    Hanoi (n-1, peg3, peg2, peg1);
    }
  else
    {
    MovePuck (peg1, peg2);
    }
}

#define MAX_PUCKS 20
extern char * optarg;

int main(int argc, char *argv[])
{
  vtkActor *peg[3], *puck[MAX_PUCKS];
  int i, c;
  float scale;
  int puckResolution=48;
  float red, green, blue;


  // Parse command line
  //
  // Default values
  NumberOfPucks = 5;
  NumberOfSteps = 5;

  // parse
  while ((c = getopt(argc, argv, "p:s:r:S")) != EOF) 
    {
    switch (c) 
      {
      case '?':
        cerr << "usage: " << argv[0] << " [-p #] [-s #]\n";
        return 2;

      case 'p':
        NumberOfPucks = atoi(optarg);
        break;

      case 's':
        NumberOfSteps = atoi(optarg);
        break;

      case 'r':
        puckResolution = atoi(optarg);
        break;
      }
    }

  // Initialize static variables and check input
  //
  if ( NumberOfPucks < 2 )
    {
    cerr << "Please use more pucks!\n";
    return 0;
    }

  if ( NumberOfPucks > MAX_PUCKS )
    {
    cerr << "Too many pucks specified! Maximum is " << MAX_PUCKS <<"\n";
    return 0;
    }

  if ( NumberOfSteps < 3 )
    {
    cerr << "Please use more steps!\n";
    return 0;
    }

  pegStack[0] = vtkStack::New();
  pegStack[1] = vtkStack::New();
  pegStack[2] = vtkStack::New();

  L = 1.0;
  H = 1.1 * NumberOfPucks * L;
  R = 0.5;
  rMin = 4.0 * R;
  rMax = 12.0 * R;
  D = 1.1 * 1.25 * rMax;
  NumberOfMoves = 0;

  // Create renderer, lights, and camera
  //
  vtkRenderer *aren = vtkRenderer::New();
    Renwin = vtkRenderWindow::New();
    Renwin->AddRenderer(aren);
    Renwin->SetSize(300,200);

  aren->SetBackground(1,1,1);

  vtkLight *light = vtkLight::New();

  vtkCamera *camera = vtkCamera::New();
    camera->SetFocalPoint(0,0,0); //set up initial view orientation
    camera->SetPosition(1,1,1);
    camera->SetViewAngle(5.0);

  aren->SetActiveCamera(camera);
  aren->AddLight(light);

  // Create geometry: table, pegs, and pucks
  //
  vtkCylinderSource *pegGeometry = vtkCylinderSource::New();
    pegGeometry->SetResolution(8);
  vtkPolyDataMapper *pegMapper = vtkPolyDataMapper::New();
    pegMapper->SetInput(pegGeometry->GetOutput());

  vtkCylinderSource *puckGeometry = vtkCylinderSource::New();
    puckGeometry->SetResolution(puckResolution);
  vtkPolyDataMapper *puckMapper = vtkPolyDataMapper::New();
    puckMapper->SetInput(puckGeometry->GetOutput());

  vtkPlaneSource *tableGeometry = vtkPlaneSource::New();
    tableGeometry->SetResolution(10,10);
  vtkPolyDataMapper *tableMapper = vtkPolyDataMapper::New();
    tableMapper->SetInput(tableGeometry->GetOutput());

  // Create the actors: table top, pegs, and pucks
  //
  // The table
  vtkActor *table = vtkActor::New();
  aren->AddActor(table);
  table->SetMapper(tableMapper);
  table->GetProperty()->SetColor(0.9569,0.6431,0.3765);
  table->AddPosition(D,0,0);
  table->SetScale(4*D,2*D,3*D);
  table->RotateX(90);

  //The pegs (using cylinder geometry).  Note that the pegs have to translated 
  //in the  y-direction because the cylinder is centered about the origin.
  for (i=0; i<3; i++)
    {
    peg[i] = vtkActor::New();
    aren->AddActor(peg[i]);
    peg[i]->SetMapper(pegMapper);
    peg[i]->GetProperty()->SetColor(1,1,1);
    peg[i]->AddPosition(i*D, H/2, 0);
    peg[i]->SetScale(1,H,1);
    }

  // Initialize the random seed
  vtkMath::RandomSeed( 6 );

  //The pucks (using cylinder geometry). Always loaded on peg# 0.
  for (i=0; i<NumberOfPucks; i++)
    {
    puck[i] = vtkActor::New();
    puck[i]->SetMapper(puckMapper);
    red = vtkMath::Random (); 
    green = vtkMath::Random (); blue = vtkMath::Random ();
    puck[i]->GetProperty()->SetColor(red, green, blue);
    puck[i]->AddPosition(0,i*L+L/2, 0);
    scale = rMax - i*(rMax-rMin) / (NumberOfPucks-1);
    puck[i]->SetScale(scale,1,scale);
    aren->AddActor(puck[i]);
    pegStack[0]->Push(puck[i]);
    }

  // Reset the camera to view all actors.
  //
  aren->ResetCamera();
  camera->Dolly(2.5);
  aren->ResetCameraClippingRange();
  light->SetFocalPoint(camera->GetFocalPoint());
  light->SetPosition(camera->GetPosition());

//  Renwin->Render();
//  Renwin->SetFileName("hanoi0.ppm");
//  Renwin->SaveImageAsPPM();

  // Begin recursion
  //
  Hanoi (NumberOfPucks-1, 0, 2, 1);
  Hanoi (1, 0, 1, 2);
  Hanoi (NumberOfPucks-1, 2, 1, 0);

  SAVEIMAGE( Renwin );

  //Renwin->Render();
  //Renwin->SetFileName("hanoi2.ppm");
  //Renwin->SaveImageAsPPM();


  // Report output
  //
  cout << "Number of moves: " << NumberOfMoves << "\n"
       << "Polygons rendered each frame: " 
       << 3*8 + 1 + NumberOfPucks*(2+puckResolution) << "\n"
       << "Total number of frames: " 
       << NumberOfMoves*3*NumberOfSteps << "\n";
 
  // Clean up
  aren->Delete();
  Renwin->Delete();
  light->Delete();
  camera->Delete();
  pegGeometry->Delete();
  pegMapper->Delete();
  puckGeometry->Delete();
  puckMapper->Delete();
  tableGeometry->Delete();
  tableMapper->Delete();
  table->Delete();
  for (i=0; i<NumberOfPucks; i++) puck[i]->Delete();
  for (i=0; i<3; i++) peg[i]->Delete();
}
