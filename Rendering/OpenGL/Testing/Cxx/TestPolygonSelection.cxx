/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPolygonSelection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkActor.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkExtractSelectedPolyDataIds.h"
#include "vtkHardwareSelector.h"
#include "vtkIntArray.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkInteractorStyleDrawPolygon.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"

const char eventLog[] =
  "# StreamVersion 1\n"
  "RenderEvent 0 0 0 0 0 0 0\n"
  "EnterEvent 278 0 0 0 0 0 0\n"
  "MouseMoveEvent 278 0 0 0 0 0 0\n"
  "MouseMoveEvent 274 8 0 0 0 0 0\n"
  "MouseMoveEvent 144 44 0 0 0 0 0\n"
  "MouseMoveEvent 144 43 0 0 0 0 0\n"
  "LeftButtonPressEvent 144 43 0 0 0 0 0\n"
  "StartInteractionEvent 144 43 0 0 0 0 0\n"
  "MouseMoveEvent 143 43 0 0 0 0 0\n"
  "MouseMoveEvent 142 43 0 0 0 0 0\n"
  "MouseMoveEvent 141 44 0 0 0 0 0\n"
  "MouseMoveEvent 140 44 0 0 0 0 0\n"
  "MouseMoveEvent 140 45 0 0 0 0 0\n"
  "MouseMoveEvent 139 45 0 0 0 0 0\n"
  "MouseMoveEvent 137 45 0 0 0 0 0\n"
  "MouseMoveEvent 136 45 0 0 0 0 0\n"
  "MouseMoveEvent 135 45 0 0 0 0 0\n"
  "MouseMoveEvent 134 46 0 0 0 0 0\n"
  "MouseMoveEvent 131 46 0 0 0 0 0\n"
  "MouseMoveEvent 129 46 0 0 0 0 0\n"
  "MouseMoveEvent 126 47 0 0 0 0 0\n"
  "MouseMoveEvent 124 47 0 0 0 0 0\n"
  "MouseMoveEvent 121 48 0 0 0 0 0\n"
  "MouseMoveEvent 118 50 0 0 0 0 0\n"
  "MouseMoveEvent 116 50 0 0 0 0 0\n"
  "MouseMoveEvent 114 50 0 0 0 0 0\n"
  "MouseMoveEvent 112 51 0 0 0 0 0\n"
  "MouseMoveEvent 109 52 0 0 0 0 0\n"
  "MouseMoveEvent 108 53 0 0 0 0 0\n"
  "MouseMoveEvent 106 53 0 0 0 0 0\n"
  "MouseMoveEvent 104 53 0 0 0 0 0\n"
  "MouseMoveEvent 102 53 0 0 0 0 0\n"
  "MouseMoveEvent 100 55 0 0 0 0 0\n"
  "MouseMoveEvent 99 55 0 0 0 0 0\n"
  "MouseMoveEvent 96 56 0 0 0 0 0\n"
  "MouseMoveEvent 95 57 0 0 0 0 0\n"
  "MouseMoveEvent 93 58 0 0 0 0 0\n"
  "MouseMoveEvent 90 59 0 0 0 0 0\n"
  "MouseMoveEvent 88 60 0 0 0 0 0\n"
  "MouseMoveEvent 86 62 0 0 0 0 0\n"
  "MouseMoveEvent 84 62 0 0 0 0 0\n"
  "MouseMoveEvent 84 64 0 0 0 0 0\n"
  "MouseMoveEvent 82 64 0 0 0 0 0\n"
  "MouseMoveEvent 80 66 0 0 0 0 0\n"
  "MouseMoveEvent 79 67 0 0 0 0 0\n"
  "MouseMoveEvent 77 69 0 0 0 0 0\n"
  "MouseMoveEvent 76 70 0 0 0 0 0\n"
  "MouseMoveEvent 74 72 0 0 0 0 0\n"
  "MouseMoveEvent 72 73 0 0 0 0 0\n"
  "MouseMoveEvent 70 75 0 0 0 0 0\n"
  "MouseMoveEvent 68 77 0 0 0 0 0\n"
  "MouseMoveEvent 67 78 0 0 0 0 0\n"
  "MouseMoveEvent 65 79 0 0 0 0 0\n"
  "MouseMoveEvent 64 81 0 0 0 0 0\n"
  "MouseMoveEvent 62 82 0 0 0 0 0\n"
  "MouseMoveEvent 61 83 0 0 0 0 0\n"
  "MouseMoveEvent 60 84 0 0 0 0 0\n"
  "MouseMoveEvent 59 86 0 0 0 0 0\n"
  "MouseMoveEvent 58 87 0 0 0 0 0\n"
  "MouseMoveEvent 57 89 0 0 0 0 0\n"
  "MouseMoveEvent 55 89 0 0 0 0 0\n"
  "MouseMoveEvent 54 91 0 0 0 0 0\n"
  "MouseMoveEvent 52 92 0 0 0 0 0\n"
  "MouseMoveEvent 51 94 0 0 0 0 0\n"
  "MouseMoveEvent 50 95 0 0 0 0 0\n"
  "MouseMoveEvent 49 97 0 0 0 0 0\n"
  "MouseMoveEvent 47 97 0 0 0 0 0\n"
  "MouseMoveEvent 46 100 0 0 0 0 0\n"
  "MouseMoveEvent 45 100 0 0 0 0 0\n"
  "MouseMoveEvent 44 101 0 0 0 0 0\n"
  "MouseMoveEvent 44 103 0 0 0 0 0\n"
  "MouseMoveEvent 43 104 0 0 0 0 0\n"
  "MouseMoveEvent 42 106 0 0 0 0 0\n"
  "MouseMoveEvent 42 107 0 0 0 0 0\n"
  "MouseMoveEvent 41 108 0 0 0 0 0\n"
  "MouseMoveEvent 40 109 0 0 0 0 0\n"
  "MouseMoveEvent 40 110 0 0 0 0 0\n"
  "MouseMoveEvent 40 111 0 0 0 0 0\n"
  "MouseMoveEvent 39 113 0 0 0 0 0\n"
  "MouseMoveEvent 38 113 0 0 0 0 0\n"
  "MouseMoveEvent 38 115 0 0 0 0 0\n"
  "MouseMoveEvent 38 116 0 0 0 0 0\n"
  "MouseMoveEvent 37 117 0 0 0 0 0\n"
  "MouseMoveEvent 36 118 0 0 0 0 0\n"
  "MouseMoveEvent 36 120 0 0 0 0 0\n"
  "MouseMoveEvent 35 121 0 0 0 0 0\n"
  "MouseMoveEvent 35 123 0 0 0 0 0\n"
  "MouseMoveEvent 35 124 0 0 0 0 0\n"
  "MouseMoveEvent 34 125 0 0 0 0 0\n"
  "MouseMoveEvent 34 128 0 0 0 0 0\n"
  "MouseMoveEvent 33 130 0 0 0 0 0\n"
  "MouseMoveEvent 33 131 0 0 0 0 0\n"
  "MouseMoveEvent 33 132 0 0 0 0 0\n"
  "MouseMoveEvent 33 136 0 0 0 0 0\n"
  "MouseMoveEvent 33 138 0 0 0 0 0\n"
  "MouseMoveEvent 33 139 0 0 0 0 0\n"
  "MouseMoveEvent 32 142 0 0 0 0 0\n"
  "MouseMoveEvent 32 144 0 0 0 0 0\n"
  "MouseMoveEvent 32 147 0 0 0 0 0\n"
  "MouseMoveEvent 31 148 0 0 0 0 0\n"
  "MouseMoveEvent 31 149 0 0 0 0 0\n"
  "MouseMoveEvent 31 151 0 0 0 0 0\n"
  "MouseMoveEvent 31 152 0 0 0 0 0\n"
  "MouseMoveEvent 31 153 0 0 0 0 0\n"
  "MouseMoveEvent 31 154 0 0 0 0 0\n"
  "MouseMoveEvent 30 156 0 0 0 0 0\n"
  "MouseMoveEvent 30 158 0 0 0 0 0\n"
  "MouseMoveEvent 30 160 0 0 0 0 0\n"
  "MouseMoveEvent 30 162 0 0 0 0 0\n"
  "MouseMoveEvent 30 164 0 0 0 0 0\n"
  "MouseMoveEvent 30 166 0 0 0 0 0\n"
  "MouseMoveEvent 30 168 0 0 0 0 0\n"
  "MouseMoveEvent 29 170 0 0 0 0 0\n"
  "MouseMoveEvent 29 171 0 0 0 0 0\n"
  "MouseMoveEvent 29 173 0 0 0 0 0\n"
  "MouseMoveEvent 29 174 0 0 0 0 0\n"
  "MouseMoveEvent 29 175 0 0 0 0 0\n"
  "MouseMoveEvent 29 177 0 0 0 0 0\n"
  "MouseMoveEvent 29 179 0 0 0 0 0\n"
  "MouseMoveEvent 29 181 0 0 0 0 0\n"
  "MouseMoveEvent 29 183 0 0 0 0 0\n"
  "MouseMoveEvent 29 185 0 0 0 0 0\n"
  "MouseMoveEvent 30 188 0 0 0 0 0\n"
  "MouseMoveEvent 30 189 0 0 0 0 0\n"
  "MouseMoveEvent 30 192 0 0 0 0 0\n"
  "MouseMoveEvent 31 194 0 0 0 0 0\n"
  "MouseMoveEvent 31 196 0 0 0 0 0\n"
  "MouseMoveEvent 31 198 0 0 0 0 0\n"
  "MouseMoveEvent 31 200 0 0 0 0 0\n"
  "MouseMoveEvent 32 202 0 0 0 0 0\n"
  "MouseMoveEvent 32 205 0 0 0 0 0\n"
  "MouseMoveEvent 33 208 0 0 0 0 0\n"
  "MouseMoveEvent 34 212 0 0 0 0 0\n"
  "MouseMoveEvent 35 214 0 0 0 0 0\n"
  "MouseMoveEvent 35 217 0 0 0 0 0\n"
  "MouseMoveEvent 36 219 0 0 0 0 0\n"
  "MouseMoveEvent 37 222 0 0 0 0 0\n"
  "MouseMoveEvent 38 224 0 0 0 0 0\n"
  "MouseMoveEvent 38 226 0 0 0 0 0\n"
  "MouseMoveEvent 39 228 0 0 0 0 0\n"
  "MouseMoveEvent 39 229 0 0 0 0 0\n"
  "MouseMoveEvent 40 229 0 0 0 0 0\n"
  "MouseMoveEvent 41 231 0 0 0 0 0\n"
  "MouseMoveEvent 42 233 0 0 0 0 0\n"
  "MouseMoveEvent 42 234 0 0 0 0 0\n"
  "MouseMoveEvent 43 236 0 0 0 0 0\n"
  "MouseMoveEvent 44 236 0 0 0 0 0\n"
  "MouseMoveEvent 44 237 0 0 0 0 0\n"
  "MouseMoveEvent 45 238 0 0 0 0 0\n"
  "MouseMoveEvent 45 239 0 0 0 0 0\n"
  "MouseMoveEvent 47 241 0 0 0 0 0\n"
  "MouseMoveEvent 48 243 0 0 0 0 0\n"
  "MouseMoveEvent 49 244 0 0 0 0 0\n"
  "MouseMoveEvent 50 246 0 0 0 0 0\n"
  "MouseMoveEvent 51 246 0 0 0 0 0\n"
  "MouseMoveEvent 51 247 0 0 0 0 0\n"
  "MouseMoveEvent 52 247 0 0 0 0 0\n"
  "MouseMoveEvent 53 248 0 0 0 0 0\n"
  "MouseMoveEvent 53 249 0 0 0 0 0\n"
  "MouseMoveEvent 54 250 0 0 0 0 0\n"
  "MouseMoveEvent 55 250 0 0 0 0 0\n"
  "MouseMoveEvent 56 252 0 0 0 0 0\n"
  "MouseMoveEvent 57 253 0 0 0 0 0\n"
  "MouseMoveEvent 58 254 0 0 0 0 0\n"
  "MouseMoveEvent 59 255 0 0 0 0 0\n"
  "MouseMoveEvent 61 257 0 0 0 0 0\n"
  "MouseMoveEvent 62 258 0 0 0 0 0\n"
  "MouseMoveEvent 63 258 0 0 0 0 0\n"
  "MouseMoveEvent 65 259 0 0 0 0 0\n"
  "MouseMoveEvent 66 259 0 0 0 0 0\n"
  "MouseMoveEvent 67 260 0 0 0 0 0\n"
  "MouseMoveEvent 69 262 0 0 0 0 0\n"
  "MouseMoveEvent 70 262 0 0 0 0 0\n"
  "MouseMoveEvent 71 263 0 0 0 0 0\n"
  "MouseMoveEvent 73 265 0 0 0 0 0\n"
  "MouseMoveEvent 74 265 0 0 0 0 0\n"
  "MouseMoveEvent 75 265 0 0 0 0 0\n"
  "MouseMoveEvent 75 266 0 0 0 0 0\n"
  "MouseMoveEvent 76 267 0 0 0 0 0\n"
  "MouseMoveEvent 77 267 0 0 0 0 0\n"
  "MouseMoveEvent 78 267 0 0 0 0 0\n"
  "MouseMoveEvent 78 268 0 0 0 0 0\n"
  "MouseMoveEvent 79 268 0 0 0 0 0\n"
  "MouseMoveEvent 81 268 0 0 0 0 0\n"
  "MouseMoveEvent 81 269 0 0 0 0 0\n"
  "MouseMoveEvent 82 269 0 0 0 0 0\n"
  "MouseMoveEvent 83 270 0 0 0 0 0\n"
  "MouseMoveEvent 84 270 0 0 0 0 0\n"
  "MouseMoveEvent 85 270 0 0 0 0 0\n"
  "MouseMoveEvent 86 271 0 0 0 0 0\n"
  "MouseMoveEvent 88 272 0 0 0 0 0\n"
  "MouseMoveEvent 89 272 0 0 0 0 0\n"
  "MouseMoveEvent 90 273 0 0 0 0 0\n"
  "MouseMoveEvent 91 273 0 0 0 0 0\n"
  "MouseMoveEvent 92 274 0 0 0 0 0\n"
  "MouseMoveEvent 94 274 0 0 0 0 0\n"
  "MouseMoveEvent 94 275 0 0 0 0 0\n"
  "MouseMoveEvent 95 275 0 0 0 0 0\n"
  "MouseMoveEvent 96 275 0 0 0 0 0\n"
  "MouseMoveEvent 96 276 0 0 0 0 0\n"
  "MouseMoveEvent 97 276 0 0 0 0 0\n"
  "MouseMoveEvent 98 276 0 0 0 0 0\n"
  "MouseMoveEvent 98 277 0 0 0 0 0\n"
  "MouseMoveEvent 99 277 0 0 0 0 0\n"
  "MouseMoveEvent 100 277 0 0 0 0 0\n"
  "MouseMoveEvent 101 278 0 0 0 0 0\n"
  "MouseMoveEvent 103 279 0 0 0 0 0\n"
  "MouseMoveEvent 104 279 0 0 0 0 0\n"
  "MouseMoveEvent 105 279 0 0 0 0 0\n"
  "MouseMoveEvent 106 279 0 0 0 0 0\n"
  "MouseMoveEvent 107 279 0 0 0 0 0\n"
  "MouseMoveEvent 108 279 0 0 0 0 0\n"
  "MouseMoveEvent 109 279 0 0 0 0 0\n"
  "MouseMoveEvent 111 279 0 0 0 0 0\n"
  "MouseMoveEvent 111 280 0 0 0 0 0\n"
  "MouseMoveEvent 112 280 0 0 0 0 0\n"
  "MouseMoveEvent 113 280 0 0 0 0 0\n"
  "MouseMoveEvent 114 280 0 0 0 0 0\n"
  "MouseMoveEvent 115 280 0 0 0 0 0\n"
  "MouseMoveEvent 116 281 0 0 0 0 0\n"
  "MouseMoveEvent 117 281 0 0 0 0 0\n"
  "MouseMoveEvent 118 281 0 0 0 0 0\n"
  "MouseMoveEvent 119 281 0 0 0 0 0\n"
  "MouseMoveEvent 120 282 0 0 0 0 0\n"
  "MouseMoveEvent 122 282 0 0 0 0 0\n"
  "MouseMoveEvent 123 282 0 0 0 0 0\n"
  "MouseMoveEvent 125 282 0 0 0 0 0\n"
  "MouseMoveEvent 126 282 0 0 0 0 0\n"
  "MouseMoveEvent 127 282 0 0 0 0 0\n"
  "MouseMoveEvent 128 282 0 0 0 0 0\n"
  "MouseMoveEvent 130 282 0 0 0 0 0\n"
  "MouseMoveEvent 131 282 0 0 0 0 0\n"
  "MouseMoveEvent 131 283 0 0 0 0 0\n"
  "MouseMoveEvent 132 283 0 0 0 0 0\n"
  "MouseMoveEvent 133 283 0 0 0 0 0\n"
  "MouseMoveEvent 134 283 0 0 0 0 0\n"
  "MouseMoveEvent 134 282 0 0 0 0 0\n"
  "MouseMoveEvent 134 281 0 0 0 0 0\n"
  "MouseMoveEvent 135 281 0 0 0 0 0\n"
  "MouseMoveEvent 136 280 0 0 0 0 0\n"
  "MouseMoveEvent 137 280 0 0 0 0 0\n"
  "MouseMoveEvent 138 280 0 0 0 0 0\n"
  "MouseMoveEvent 138 279 0 0 0 0 0\n"
  "MouseMoveEvent 139 279 0 0 0 0 0\n"
  "MouseMoveEvent 139 278 0 0 0 0 0\n"
  "MouseMoveEvent 140 278 0 0 0 0 0\n"
  "MouseMoveEvent 141 278 0 0 0 0 0\n"
  "MouseMoveEvent 143 278 0 0 0 0 0\n"
  "MouseMoveEvent 144 278 0 0 0 0 0\n"
  "MouseMoveEvent 146 278 0 0 0 0 0\n"
  "LeftButtonReleaseEvent 146 278 0 0 0 0 0\n"
  "EndInteractionEvent 146 278 0 0 0 0 0\n"
  "MouseMoveEvent 146 278 0 0 0 0 0\n"
  "MouseMoveEvent 146 279 0 0 0 0 0\n"
  "MouseMoveEvent 146 280 0 0 0 0 0\n"
  "MouseMoveEvent 294 207 0 0 0 0 0\n"
  "LeaveEvent 294 207 0 0 0 0 0\n"
  ;

int TestPolygonSelection( int argc, char* argv[] )
{
  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(16);
  sphere->SetPhiResolution(16);
  sphere->SetRadius(0.5);

  vtkNew<vtkActor> sactor;
  sactor->PickableOn(); //lets the HardwareSelector select in it
  vtkNew<vtkPolyDataMapper> smapper;
  sactor->SetMapper(smapper.GetPointer());

  vtkNew<vtkRenderer> ren;
  ren->AddActor(sactor.GetPointer());
  // extracted part
  vtkNew<vtkPolyDataMapper> emapper;
  vtkNew<vtkActor> eactor;
  eactor->PickableOff();
  eactor->SetMapper(emapper.GetPointer());
  ren->AddActor(eactor.GetPointer());

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(300,300);
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(ren.GetPointer());

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  //use the draw-polygon interactor style
  vtkRenderWindowInteractor* rwi = renWin->GetInteractor();
  vtkNew<vtkInteractorStyleDrawPolygon> polyStyle;
  rwi->SetInteractorStyle(polyStyle.GetPointer());

  // record events
  vtkNew<vtkInteractorEventRecorder> recorder;
  recorder->SetInteractor(rwi);

#ifdef RECORD
  recorder->SetFileName("record.log");
  recorder->On();
  recorder->Record();
#else
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(eventLog);
#endif

  smapper->SetInputConnection(sphere->GetOutputPort());

  iren->Initialize();
  renWin->Render();

#ifndef RECORD
  recorder->Play();
  recorder->Off();
#endif

  renWin->Render();
  int rgba[4];
  renWin->GetColorBufferSizes(rgba);
  if (rgba[0] < 8 || rgba[1] < 8 || rgba[2] < 8)
    {
    cout <<"Color buffer depth must be atleast 8 bit. Currently: "
      << rgba[0] << ", " << rgba[1] << ", " << rgba[2] << endl;
    return 0;
    }

  std::vector<vtkVector2i> points = polyStyle->GetPolygonPoints();
  if(points.size() >= 3)
    {
    vtkNew<vtkIntArray> polygonPointsArray;
    polygonPointsArray->SetNumberOfComponents(2);
    polygonPointsArray->SetNumberOfTuples(points.size());
    for (unsigned int j = 0; j < points.size(); ++j)
      {
      const vtkVector2i &v = points[j];
      int pos[2] = {v[0], v[1]};
      polygonPointsArray->SetTupleValue(j, pos);
      }

    vtkNew<vtkHardwareSelector> hardSel;
    hardSel->SetRenderer(ren.GetPointer());

    int* wsize = renWin->GetSize();
    hardSel->SetArea(0, 0, wsize[0]-1, wsize[1]-1);
    hardSel->SetFieldAssociation(vtkDataObject::FIELD_ASSOCIATION_CELLS);

    if (hardSel->CaptureBuffers())
      {
      vtkSelection* psel = hardSel->GeneratePolygonSelection(
        polygonPointsArray->GetPointer(0),
        polygonPointsArray->GetNumberOfTuples()*2);
      hardSel->ClearBuffers();

      vtkSmartPointer<vtkSelection> sel;
      sel.TakeReference(psel);
      vtkNew<vtkExtractSelectedPolyDataIds> selFilter;
      selFilter->SetInputConnection(0,sphere->GetOutputPort());
      selFilter->SetInputData(1, sel);
      selFilter->Update();

      emapper->SetInputConnection(selFilter->GetOutputPort());
      emapper->Update();

      sactor->SetVisibility(false);
      renWin->Render();
      }
    }
  int retVal = vtkRegressionTestImage( renWin.GetPointer() );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  return !retVal;
}
