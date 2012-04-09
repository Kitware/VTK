/*=========================================================================

  Program:   Visualization Toolkit
  Module:    otherCoordinate.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME 
// .SECTION Description
// this program tests vtkCoordinate

#include "vtkCoordinate.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkCamera.h"

#include <vtksys/ios/sstream>

#include "vtkDebugLeaks.h"

void ToAll (ostream& strm, vtkCoordinate *c1, vtkViewport *ren1, 
            double *from)
{
  double *value;
  int *ivalue;
  const char *whichCoord = c1->GetCoordinateSystemAsString();

  c1->SetValue (from);

  strm << endl << "========" << endl;
  strm << *c1;
  value = c1->GetComputedWorldValue (ren1);
  strm << whichCoord <<"(" << from[0] << ", " << from[1] << ", " << from[2]
       << ") -> World(" << value[0] << ", " << value[1] << ", " << value[2] 
       << ")" << endl;
  ivalue = c1->GetComputedDisplayValue (ren1);
  strm << whichCoord << "(" << from[0] << ", " << from[1] << ", " << from[2]
       << ") -> Display(" << ivalue[0] << ", " << ivalue[1] << ")" << endl;
  ivalue = c1->GetComputedLocalDisplayValue (ren1);
  strm << whichCoord << "(" << from[0] << ", " << from[1] << ", " << from[2]
       << ") -> LocalDisplay(" << ivalue[0] << ", " << ivalue[1] 
       << ")" << endl;
  ivalue = c1->GetComputedViewportValue (ren1);
  strm << whichCoord << "(" << from[0] << ", " << from[1] << ", " << from[2]
       << ") -> Viewport(" << ivalue[0] << ", " << ivalue[1] << ")" << endl;


}
int Test(ostream& strm)
{
  // actual test
  strm << "Testing vtkCoordinate" << endl;
  vtkCoordinate *c1 = vtkCoordinate::New();
  vtkCoordinate *c2 = vtkCoordinate::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  vtkRenderer *ren1 = vtkRenderer::New();
  vtkCamera *camera = vtkCamera::New();
  double from[3];
  
  ren1->SetActiveCamera (camera);
  renWin->AddRenderer (ren1);
  renWin->SetSize (100, 100);
  
  strm << "Origin: (" << ren1->GetOrigin()[0] << ", " << ren1->GetOrigin()[1] << ")" << endl;
  strm << "Center: (" << ren1->GetCenter()[0] << ", " << ren1->GetOrigin()[1] << ")" << endl;

  strm << endl << "********** A NULL Viewport **********" << endl;
  
  c1->SetCoordinateSystemToWorld();
  from[0] = 0.0; from[1] = 0.0; from[2] = 0.0;
  ToAll (strm, c1, ren1, from);  

  c1->SetCoordinateSystemToDisplay();
  from[0] = 50; from[1] = 50; from[2] = 0;
  ToAll (strm, c1, ren1, from);
  
  c1->SetCoordinateSystemToNormalizedDisplay();
  from[0] = .5; from[1] = .5; from[2] = 0.0;
  ToAll (strm, c1, ren1, from);
  
  c1->SetCoordinateSystemToViewport();
  from[0] = 50; from[1] = 50; from[2] = 0;
  ToAll (strm, c1, ren1, from);
  
  c1->SetCoordinateSystemToNormalizedViewport();
  from[0] = .5; from[1] = .5; from[2] = 0;
  ToAll (strm, c1, ren1, from);
  
  c1->SetCoordinateSystemToView();
  from[0] = 0.0; from[1] = 0.0; from[2] = 0.0;
  ToAll (strm, c1, ren1, from);
  
  strm << endl << "********** A specified Viewport **********" << endl;
  c1->SetViewport (ren1);
  
  c1->SetCoordinateSystemToWorld();
  from[0] = 0.0; from[1] = 0.0; from[2] = 0.0;
  ToAll (strm, c1, ren1, from);  

  c1->SetCoordinateSystemToDisplay();
  from[0] = 50; from[1] = 50; from[2] = 0;
  ToAll (strm, c1, ren1, from);
  
  c1->SetCoordinateSystemToNormalizedDisplay();
  from[0] = .5; from[1] = .5; from[2] = 0.0;
  ToAll (strm, c1, ren1, from);
  
  c1->SetCoordinateSystemToViewport();
  from[0] = 50; from[1] = 50; from[2] = 0;
  ToAll (strm, c1, ren1, from);
  
  c1->SetCoordinateSystemToNormalizedViewport();
  from[0] = .5; from[1] = .5; from[2] = 0;
  ToAll (strm, c1, ren1, from);
  
  c1->SetCoordinateSystemToView();
  from[0] = 0.0; from[1] = 0.0; from[2] = 0.0;
  ToAll (strm, c1, ren1, from);
  
  strm << endl << "********** With a Reference Coordinate **********" << endl;

  c2->SetCoordinateSystemToNormalizedDisplay();
  c2->SetCoordinateSystemToWorld();
  c2->SetValue (0.0, 0.0, 0.0);
  c1->SetReferenceCoordinate (c2);
  
  strm << *c2;
  
  c1->SetCoordinateSystemToWorld();
  from[0] = 0.0; from[1] = 0.0; from[2] = 0.0;
  ToAll (strm, c1, ren1, from);  

  c1->SetCoordinateSystemToDisplay();
  from[0] = 50; from[1] = 50; from[2] = 0;
  ToAll (strm, c1, ren1, from);
  
  c1->SetCoordinateSystemToNormalizedDisplay();
  from[0] = .5; from[1] = .5; from[2] = 0.0;
  ToAll (strm, c1, ren1, from);
  
  c1->SetCoordinateSystemToViewport();
  from[0] = 50; from[1] = 50; from[2] = 0;
  ToAll (strm, c1, ren1, from);
  
  c1->SetCoordinateSystemToNormalizedViewport();
  from[0] = .5; from[1] = .5; from[2] = 0;
  ToAll (strm, c1, ren1, from);
  
  c1->SetCoordinateSystemToView();
  from[0] = 0.0; from[1] = 0.0; from[2] = 0.0;
  ToAll (strm, c1, ren1, from);
  
  c1->Delete ();
  c2->Delete ();
  renWin->Delete ();
  ren1->Delete ();
  camera->Delete ();

  strm << "Testing completed" << endl;
  return 0;
}

int otherCoordinate(int,char *[])
{
  vtksys_ios::ostringstream vtkmsg_with_warning_C4701; 
  return Test(vtkmsg_with_warning_C4701);
}

