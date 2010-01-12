/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCheckerboardWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the vtkCheckerboardWidget.

// First include the required header files for the VTK classes we are using.
#include "vtkSmartPointer.h"
#include "vtkCheckerboardWidget.h"
#include "vtkCheckerboardRepresentation.h"
#include "vtkSliderRepresentation3D.h"
#include "vtkImageCanvasSource2D.h"
#include "vtkImageWrapPad.h"
#include "vtkImageCanvasSource2D.h"
#include "vtkImageCheckerboard.h"
#include "vtkImageActor.h"
#include "vtkImageData.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCommand.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkDebugLeaks.h"

const char eventLog[] =
"# StreamVersion 1\n"
"RenderEvent 0 0 0 0 0 0 0\n"
"RenderEvent 0 0 0 0 0 0 0\n"
"EnterEvent 115 5 0 0 0 0 0\n"
"MouseMoveEvent 115 5 0 0 0 0 0\n"
"MouseMoveEvent 245 48 0 0 0 0 0\n"
"LeftButtonPressEvent 245 48 0 0 0 0 0\n"
"RenderEvent 245 48 0 0 0 0 0\n"
"MouseMoveEvent 244 48 0 0 0 0 0\n"
"RenderEvent 244 48 0 0 0 0 0\n"
"MouseMoveEvent 242 48 0 0 0 0 0\n"
"RenderEvent 242 48 0 0 0 0 0\n"
"MouseMoveEvent 241 48 0 0 0 0 0\n"
"RenderEvent 241 48 0 0 0 0 0\n"
"MouseMoveEvent 240 48 0 0 0 0 0\n"
"RenderEvent 240 48 0 0 0 0 0\n"
"MouseMoveEvent 239 48 0 0 0 0 0\n"
"RenderEvent 239 48 0 0 0 0 0\n"
"MouseMoveEvent 238 48 0 0 0 0 0\n"
"RenderEvent 238 48 0 0 0 0 0\n"
"MouseMoveEvent 237 48 0 0 0 0 0\n"
"RenderEvent 237 48 0 0 0 0 0\n"
"MouseMoveEvent 236 48 0 0 0 0 0\n"
"RenderEvent 236 48 0 0 0 0 0\n"
"MouseMoveEvent 235 48 0 0 0 0 0\n"
"RenderEvent 235 48 0 0 0 0 0\n"
"MouseMoveEvent 234 48 0 0 0 0 0\n"
"RenderEvent 234 48 0 0 0 0 0\n"
"MouseMoveEvent 233 48 0 0 0 0 0\n"
"RenderEvent 233 48 0 0 0 0 0\n"
"MouseMoveEvent 232 48 0 0 0 0 0\n"
"RenderEvent 232 48 0 0 0 0 0\n"
"MouseMoveEvent 231 48 0 0 0 0 0\n"
"RenderEvent 231 48 0 0 0 0 0\n"
"MouseMoveEvent 230 48 0 0 0 0 0\n"
"RenderEvent 230 48 0 0 0 0 0\n"
"MouseMoveEvent 229 47 0 0 0 0 0\n"
"RenderEvent 229 47 0 0 0 0 0\n"
"MouseMoveEvent 228 47 0 0 0 0 0\n"
"RenderEvent 228 47 0 0 0 0 0\n"
"MouseMoveEvent 226 47 0 0 0 0 0\n"
"RenderEvent 226 47 0 0 0 0 0\n"
"MouseMoveEvent 225 47 0 0 0 0 0\n"
"RenderEvent 225 47 0 0 0 0 0\n"
"MouseMoveEvent 221 47 0 0 0 0 0\n"
"RenderEvent 221 47 0 0 0 0 0\n"
"MouseMoveEvent 220 47 0 0 0 0 0\n"
"RenderEvent 220 47 0 0 0 0 0\n"
"MouseMoveEvent 218 47 0 0 0 0 0\n"
"RenderEvent 218 47 0 0 0 0 0\n"
"MouseMoveEvent 217 47 0 0 0 0 0\n"
"RenderEvent 217 47 0 0 0 0 0\n"
"MouseMoveEvent 216 47 0 0 0 0 0\n"
"RenderEvent 216 47 0 0 0 0 0\n"
"MouseMoveEvent 215 47 0 0 0 0 0\n"
"RenderEvent 215 47 0 0 0 0 0\n"
"MouseMoveEvent 214 47 0 0 0 0 0\n"
"RenderEvent 214 47 0 0 0 0 0\n"
"MouseMoveEvent 213 47 0 0 0 0 0\n"
"RenderEvent 213 47 0 0 0 0 0\n"
"MouseMoveEvent 212 47 0 0 0 0 0\n"
"RenderEvent 212 47 0 0 0 0 0\n"
"MouseMoveEvent 211 47 0 0 0 0 0\n"
"RenderEvent 211 47 0 0 0 0 0\n"
"MouseMoveEvent 209 47 0 0 0 0 0\n"
"RenderEvent 209 47 0 0 0 0 0\n"
"MouseMoveEvent 207 47 0 0 0 0 0\n"
"RenderEvent 207 47 0 0 0 0 0\n"
"MouseMoveEvent 206 47 0 0 0 0 0\n"
"RenderEvent 206 47 0 0 0 0 0\n"
"MouseMoveEvent 204 47 0 0 0 0 0\n"
"RenderEvent 204 47 0 0 0 0 0\n"
"MouseMoveEvent 203 47 0 0 0 0 0\n"
"RenderEvent 203 47 0 0 0 0 0\n"
"MouseMoveEvent 202 47 0 0 0 0 0\n"
"RenderEvent 202 47 0 0 0 0 0\n"
"MouseMoveEvent 201 47 0 0 0 0 0\n"
"RenderEvent 201 47 0 0 0 0 0\n"
"MouseMoveEvent 200 47 0 0 0 0 0\n"
"RenderEvent 200 47 0 0 0 0 0\n"
"MouseMoveEvent 199 47 0 0 0 0 0\n"
"RenderEvent 199 47 0 0 0 0 0\n"
"MouseMoveEvent 198 47 0 0 0 0 0\n"
"RenderEvent 198 47 0 0 0 0 0\n"
"MouseMoveEvent 197 47 0 0 0 0 0\n"
"RenderEvent 197 47 0 0 0 0 0\n"
"MouseMoveEvent 196 47 0 0 0 0 0\n"
"RenderEvent 196 47 0 0 0 0 0\n"
"MouseMoveEvent 195 47 0 0 0 0 0\n"
"RenderEvent 195 47 0 0 0 0 0\n"
"MouseMoveEvent 193 47 0 0 0 0 0\n"
"RenderEvent 193 47 0 0 0 0 0\n"
"MouseMoveEvent 192 47 0 0 0 0 0\n"
"RenderEvent 192 47 0 0 0 0 0\n"
"MouseMoveEvent 190 47 0 0 0 0 0\n"
"RenderEvent 190 47 0 0 0 0 0\n"
"MouseMoveEvent 189 47 0 0 0 0 0\n"
"RenderEvent 189 47 0 0 0 0 0\n"
"MouseMoveEvent 188 47 0 0 0 0 0\n"
"RenderEvent 188 47 0 0 0 0 0\n"
"MouseMoveEvent 187 47 0 0 0 0 0\n"
"RenderEvent 187 47 0 0 0 0 0\n"
"MouseMoveEvent 186 47 0 0 0 0 0\n"
"RenderEvent 186 47 0 0 0 0 0\n"
"MouseMoveEvent 185 47 0 0 0 0 0\n"
"RenderEvent 185 47 0 0 0 0 0\n"
"MouseMoveEvent 184 47 0 0 0 0 0\n"
"RenderEvent 184 47 0 0 0 0 0\n"
"MouseMoveEvent 183 47 0 0 0 0 0\n"
"RenderEvent 183 47 0 0 0 0 0\n"
"MouseMoveEvent 182 47 0 0 0 0 0\n"
"RenderEvent 182 47 0 0 0 0 0\n"
"MouseMoveEvent 181 47 0 0 0 0 0\n"
"RenderEvent 181 47 0 0 0 0 0\n"
"MouseMoveEvent 180 47 0 0 0 0 0\n"
"RenderEvent 180 47 0 0 0 0 0\n"
"MouseMoveEvent 179 47 0 0 0 0 0\n"
"RenderEvent 179 47 0 0 0 0 0\n"
"MouseMoveEvent 178 47 0 0 0 0 0\n"
"RenderEvent 178 47 0 0 0 0 0\n"
"MouseMoveEvent 177 47 0 0 0 0 0\n"
"RenderEvent 177 47 0 0 0 0 0\n"
"MouseMoveEvent 176 47 0 0 0 0 0\n"
"RenderEvent 176 47 0 0 0 0 0\n"
"MouseMoveEvent 175 47 0 0 0 0 0\n"
"RenderEvent 175 47 0 0 0 0 0\n"
"MouseMoveEvent 174 47 0 0 0 0 0\n"
"RenderEvent 174 47 0 0 0 0 0\n"
"MouseMoveEvent 173 47 0 0 0 0 0\n"
"RenderEvent 173 47 0 0 0 0 0\n"
"MouseMoveEvent 172 47 0 0 0 0 0\n"
"RenderEvent 172 47 0 0 0 0 0\n"
"MouseMoveEvent 172 48 0 0 0 0 0\n"
"RenderEvent 172 48 0 0 0 0 0\n"
"MouseMoveEvent 171 48 0 0 0 0 0\n"
"RenderEvent 171 48 0 0 0 0 0\n"
"MouseMoveEvent 170 48 0 0 0 0 0\n"
"RenderEvent 170 48 0 0 0 0 0\n"
"MouseMoveEvent 169 48 0 0 0 0 0\n"
"RenderEvent 169 48 0 0 0 0 0\n"
"MouseMoveEvent 168 48 0 0 0 0 0\n"
"RenderEvent 168 48 0 0 0 0 0\n"
"MouseMoveEvent 167 48 0 0 0 0 0\n"
"RenderEvent 167 48 0 0 0 0 0\n"
"MouseMoveEvent 166 48 0 0 0 0 0\n"
"RenderEvent 166 48 0 0 0 0 0\n"
"MouseMoveEvent 165 48 0 0 0 0 0\n"
"RenderEvent 165 48 0 0 0 0 0\n"
"MouseMoveEvent 164 48 0 0 0 0 0\n"
"RenderEvent 164 48 0 0 0 0 0\n"
"MouseMoveEvent 163 48 0 0 0 0 0\n"
"RenderEvent 163 48 0 0 0 0 0\n"
"MouseMoveEvent 161 48 0 0 0 0 0\n"
"RenderEvent 161 48 0 0 0 0 0\n"
"MouseMoveEvent 160 48 0 0 0 0 0\n"
"RenderEvent 160 48 0 0 0 0 0\n"
"MouseMoveEvent 157 48 0 0 0 0 0\n"
"RenderEvent 157 48 0 0 0 0 0\n"
"MouseMoveEvent 156 48 0 0 0 0 0\n"
"RenderEvent 156 48 0 0 0 0 0\n"
"MouseMoveEvent 155 48 0 0 0 0 0\n"
"RenderEvent 155 48 0 0 0 0 0\n"
"LeftButtonReleaseEvent 155 48 0 0 0 0 0\n"
"RenderEvent 155 48 0 0 0 0 0\n"
"MouseMoveEvent 155 48 0 0 0 0 0\n"
"MouseMoveEvent 252 138 0 0 0 0 0\n"
"LeftButtonPressEvent 252 138 0 0 0 0 0\n"
"RenderEvent 252 138 0 0 0 0 0\n"
"MouseMoveEvent 252 139 0 0 0 0 0\n"
"RenderEvent 252 139 0 0 0 0 0\n"
"MouseMoveEvent 252 140 0 0 0 0 0\n"
"RenderEvent 252 140 0 0 0 0 0\n"
"MouseMoveEvent 252 141 0 0 0 0 0\n"
"RenderEvent 252 141 0 0 0 0 0\n"
"MouseMoveEvent 252 142 0 0 0 0 0\n"
"RenderEvent 252 142 0 0 0 0 0\n"
"MouseMoveEvent 252 143 0 0 0 0 0\n"
"RenderEvent 252 143 0 0 0 0 0\n"
"MouseMoveEvent 252 144 0 0 0 0 0\n"
"RenderEvent 252 144 0 0 0 0 0\n"
"MouseMoveEvent 252 145 0 0 0 0 0\n"
"RenderEvent 252 145 0 0 0 0 0\n"
"MouseMoveEvent 252 146 0 0 0 0 0\n"
"RenderEvent 252 146 0 0 0 0 0\n"
"MouseMoveEvent 252 147 0 0 0 0 0\n"
"RenderEvent 252 147 0 0 0 0 0\n"
"MouseMoveEvent 252 148 0 0 0 0 0\n"
"RenderEvent 252 148 0 0 0 0 0\n"
"MouseMoveEvent 252 149 0 0 0 0 0\n"
"RenderEvent 252 149 0 0 0 0 0\n"
"MouseMoveEvent 252 150 0 0 0 0 0\n"
"RenderEvent 252 150 0 0 0 0 0\n"
"MouseMoveEvent 252 151 0 0 0 0 0\n"
"RenderEvent 252 151 0 0 0 0 0\n"
"MouseMoveEvent 252 151 0 0 0 0 0\n"
"RenderEvent 252 151 0 0 0 0 0\n"
"MouseMoveEvent 252 151 0 0 0 0 0\n"
"RenderEvent 252 151 0 0 0 0 0\n"
"LeftButtonReleaseEvent 252 151 0 0 0 0 0\n"
"RenderEvent 252 151 0 0 0 0 0\n"
"MouseMoveEvent 252 151 0 0 0 0 0\n"
  ;

int TestCheckerboardWidget( int, char *[] )
{
  // Create the RenderWindow, Renderer and both Actors
  //
  vtkSmartPointer<vtkRenderer> ren1 =
    vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer(ren1);

  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  // Create a checkerboard pipeline
  vtkSmartPointer<vtkImageCanvasSource2D> image1 =
    vtkSmartPointer<vtkImageCanvasSource2D>::New();
  image1->SetNumberOfScalarComponents(3);
  image1->SetScalarTypeToUnsignedChar();
  image1->SetExtent(0,511,0,511,0,0);
  image1->SetExtent(0,511,0,0,0,511);
  image1->SetDrawColor(255,255,0);
  image1->FillBox(0,511,0,511);

  vtkSmartPointer<vtkImageWrapPad> pad1 =
    vtkSmartPointer<vtkImageWrapPad>::New();
  pad1->SetInput(image1->GetOutput());
  pad1->SetOutputWholeExtent(0,511,0,511,0,0);

  vtkSmartPointer<vtkImageCanvasSource2D> image2 =
    vtkSmartPointer<vtkImageCanvasSource2D>::New();
  image2->SetNumberOfScalarComponents(3);
  image2->SetScalarTypeToUnsignedChar();
  image2->SetExtent(0,511,0,511,0,0);
  image2->SetDrawColor(0,255,255);
  image2->FillBox(0,511,0,511);

  vtkSmartPointer<vtkImageWrapPad> pad2 =
    vtkSmartPointer<vtkImageWrapPad>::New();
  pad2->SetInput(image2->GetOutput());
  pad2->SetOutputWholeExtent(0,511,0,511,0,0);

  vtkSmartPointer<vtkImageCheckerboard> checkers =
    vtkSmartPointer<vtkImageCheckerboard>::New();
  checkers->SetInput(0,pad1->GetOutput());
  checkers->SetInput(1,pad2->GetOutput());
  checkers->SetNumberOfDivisions(10,6,1);
 
  vtkSmartPointer<vtkImageActor> checkerboardActor =
    vtkSmartPointer<vtkImageActor>::New();
  checkerboardActor->SetInput(checkers->GetOutput());

  // VTK widgets consist of two parts: the widget part that handles event processing;
  // and the widget representation that defines how the widget appears in the scene 
  // (i.e., matters pertaining to geometry).
  vtkSmartPointer<vtkCheckerboardRepresentation> rep =
    vtkSmartPointer<vtkCheckerboardRepresentation>::New();
  rep->SetImageActor(checkerboardActor);
  rep->SetCheckerboard(checkers);
  rep->GetLeftRepresentation()->SetTitleText("Left");
  rep->GetRightRepresentation()->SetTitleText("Right");
  rep->GetTopRepresentation()->SetTitleText("Top");
  rep->GetBottomRepresentation()->SetTitleText("Bottom");
  rep->SetCornerOffset(.2);

  vtkSmartPointer<vtkCheckerboardWidget> checkerboardWidget =
    vtkSmartPointer<vtkCheckerboardWidget>::New();
  checkerboardWidget->SetInteractor(iren);
  checkerboardWidget->SetRepresentation(rep);

  // Add the actors to the renderer, set the background and size
  ren1->AddActor(checkerboardActor);
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // record events
  vtkSmartPointer<vtkInteractorEventRecorder> recorder =
    vtkSmartPointer<vtkInteractorEventRecorder>::New();
  recorder->SetInteractor(iren);

#ifdef RECORD
  recorder->SetFileName("record.log");
  recorder->On();
  recorder->Record();
#else
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(eventLog);
#endif

  // render the image
  //
  iren->Initialize();
  renWin->Render();
  checkerboardWidget->On();
#ifndef RECORD
  recorder->Play();
  recorder->Off();
#endif

  iren->Start();

  return EXIT_SUCCESS;

}


