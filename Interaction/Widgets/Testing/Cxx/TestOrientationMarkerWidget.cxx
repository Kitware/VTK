/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOrientationMarkerWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSmartPointer.h"
#include "vtkActor.h"
#include "vtkAnnotatedCubeActor.h"
#include "vtkAppendPolyData.h"
#include "vtkAxesActor.h"
#include "vtkCamera.h"
#include "vtkCaptionActor2D.h"
#include "vtkCellArray.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkMath.h"
#include "vtkOrientationMarkerWidget.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPropAssembly.h"
#include "vtkPropCollection.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTextProperty.h"
#include "vtkTransform.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkTubeFilter.h"

char TestOMWidgetEventLog[] =
  "# StreamVersion 1\n"
  "CharEvent 215 191 0 0 98 1 b\n"
  "KeyReleaseEvent 215 191 0 0 98 1 b\n"
  "RightButtonPressEvent 215 191 0 0 0 0 b\n"
  "TimerEvent 215 191 0 0 0 0 b\n"
  "MouseMoveEvent 216 191 0 0 0 0 b\n"
  "TimerEvent 216 191 0 0 0 0 b\n"
  "MouseMoveEvent 216 192 0 0 0 0 b\n"
  "TimerEvent 216 192 0 0 0 0 b\n"
  "MouseMoveEvent 216 193 0 0 0 0 b\n"
  "TimerEvent 216 193 0 0 0 0 b\n"
  "MouseMoveEvent 216 194 0 0 0 0 b\n"
  "TimerEvent 216 194 0 0 0 0 b\n"
  "MouseMoveEvent 216 195 0 0 0 0 b\n"
  "TimerEvent 216 195 0 0 0 0 b\n"
  "MouseMoveEvent 216 196 0 0 0 0 b\n"
  "TimerEvent 216 196 0 0 0 0 b\n"
  "MouseMoveEvent 216 197 0 0 0 0 b\n"
  "TimerEvent 216 197 0 0 0 0 b\n"
  "MouseMoveEvent 216 198 0 0 0 0 b\n"
  "TimerEvent 216 198 0 0 0 0 b\n"
  "MouseMoveEvent 216 199 0 0 0 0 b\n"
  "TimerEvent 216 199 0 0 0 0 b\n"
  "RightButtonReleaseEvent 216 199 0 0 0 0 b\n"
  "MouseMoveEvent 216 199 0 0 0 0 b\n"
  "KeyPressEvent 216 199 0 0 116 1 t\n"
  "CharEvent 216 199 0 0 116 1 t\n"
  "KeyReleaseEvent 216 199 0 0 116 1 t\n"
  "RightButtonPressEvent 216 199 0 0 0 0 t\n"
  "MouseMoveEvent 216 200 0 0 0 0 t\n"
  "MouseMoveEvent 219 240 0 0 0 0 t\n"
  "RightButtonReleaseEvent 219 240 0 0 0 0 t\n"
  "MouseMoveEvent 185 139 0 0 0 0 t\n"
  "LeftButtonPressEvent 185 139 0 0 0 0 t\n"
  "MouseMoveEvent 187 139 0 0 0 0 t\n"
  "MouseMoveEvent 199 139 0 0 0 0 t\n"
  "MouseMoveEvent 237 138 0 0 0 0 t\n"
  "MouseMoveEvent 250 137 0 0 0 0 t\n"
  "MouseMoveEvent 259 133 0 0 0 0 t\n"
  "MouseMoveEvent 276 119 0 0 0 0 t\n"
  "MouseMoveEvent 282 119 0 0 0 0 t\n"
  "MouseMoveEvent 269 136 0 0 0 0 t\n"
  "MouseMoveEvent 241 166 0 0 0 0 t\n"
  "MouseMoveEvent 218 199 0 0 0 0 t\n"
  "MouseMoveEvent 173 220 0 0 0 0 t\n"
  "MouseMoveEvent 152 230 0 0 0 0 t\n"
  "MouseMoveEvent 135 232 0 0 0 0 t\n"
  "MouseMoveEvent 124 233 0 0 0 0 t\n"
  "MouseMoveEvent 111 227 0 0 0 0 t\n"
  "MouseMoveEvent 96 218 0 0 0 0 t\n"
  "MouseMoveEvent 87 213 0 0 0 0 t\n"
  "MouseMoveEvent 81 212 0 0 0 0 t\n"
  "MouseMoveEvent 60 215 0 0 0 0 t\n"
  "MouseMoveEvent 51 226 0 0 0 0 t\n"
  "MouseMoveEvent 44 246 0 0 0 0 t\n"
  "MouseMoveEvent 40 263 0 0 0 0 t\n"
  "MouseMoveEvent 39 277 0 0 0 0 t\n"
  "MouseMoveEvent 39 299 0 0 0 0 t\n"
  "MouseMoveEvent 44 313 0 0 0 0 t\n"
  "MouseMoveEvent 52 324 0 0 0 0 t\n"
  "MouseMoveEvent 59 330 0 0 0 0 t\n"
  "MouseMoveEvent 67 332 0 0 0 0 t\n"
  "MouseMoveEvent 77 332 0 0 0 0 t\n"
  "MouseMoveEvent 99 331 0 0 0 0 t\n"
  "MouseMoveEvent 123 331 0 0 0 0 t\n"
  "MouseMoveEvent 138 335 0 0 0 0 t\n"
  "MouseMoveEvent 156 340 0 0 0 0 t\n"
  "MouseMoveEvent 175 345 0 0 0 0 t\n"
  "MouseMoveEvent 188 349 0 0 0 0 t\n"
  "MouseMoveEvent 199 352 0 0 0 0 t\n"
  "MouseMoveEvent 222 354 0 0 0 0 t\n"
  "MouseMoveEvent 237 356 0 0 0 0 t\n"
  "MouseMoveEvent 253 356 0 0 0 0 t\n"
  "MouseMoveEvent 259 356 0 0 0 0 t\n"
  "MouseMoveEvent 274 352 0 0 0 0 t\n"
  "MouseMoveEvent 278 350 0 0 0 0 t\n"
  "MouseMoveEvent 285 347 0 0 0 0 t\n"
  "MouseMoveEvent 289 343 0 0 0 0 t\n"
  "MouseMoveEvent 296 335 0 0 0 0 t\n"
  "MouseMoveEvent 306 325 0 0 0 0 t\n"
  "MouseMoveEvent 313 316 0 0 0 0 t\n"
  "MouseMoveEvent 322 304 0 0 0 0 t\n"
  "MouseMoveEvent 328 291 0 0 0 0 t\n"
  "MouseMoveEvent 331 285 0 0 0 0 t\n"
  "MouseMoveEvent 333 277 0 0 0 0 t\n"
  "MouseMoveEvent 335 267 0 0 0 0 t\n"
  "MouseMoveEvent 337 262 0 0 0 0 t\n"
  "LeftButtonReleaseEvent 337 262 0 0 0 0 t\n"
  "MouseMoveEvent 133 119 0 0 0 0 t\n"
  "LeftButtonPressEvent 133 119 0 0 0 0 t\n"
  "MouseMoveEvent 134 119 0 0 0 0 t\n"
  "MouseMoveEvent 139 125 0 0 0 0 t\n"
  "MouseMoveEvent 145 131 0 0 0 0 t\n"
  "MouseMoveEvent 156 141 0 0 0 0 t\n"
  "MouseMoveEvent 168 146 0 0 0 0 t\n"
  "MouseMoveEvent 211 158 0 0 0 0 t\n"
  "MouseMoveEvent 253 163 0 0 0 0 t\n"
  "MouseMoveEvent 300 167 0 0 0 0 t\n"
  "MouseMoveEvent 345 172 0 0 0 0 t\n"
  "MouseMoveEvent 370 179 0 0 0 0 t\n"
  "MouseMoveEvent 384 180 0 0 0 0 t\n"
  "MouseMoveEvent 396 182 0 0 0 0 t\n"
  "LeaveEvent 402 183 0 0 0 0 t\n"
  "MouseMoveEvent 402 183 0 0 0 0 t\n"
  "MouseMoveEvent 408 182 0 0 0 0 t\n"
  "EnterEvent 388 182 0 0 0 0 t\n"
  "MouseMoveEvent 388 182 0 0 0 0 t\n"
  "MouseMoveEvent 358 193 0 0 0 0 t\n"
  "MouseMoveEvent 327 227 0 0 0 0 t\n"
  "MouseMoveEvent 307 265 0 0 0 0 t\n"
  "MouseMoveEvent 290 325 0 0 0 0 t\n"
  "MouseMoveEvent 281 367 0 0 0 0 t\n"
  "MouseMoveEvent 280 396 0 0 0 0 t\n"
  "LeaveEvent 280 400 0 0 0 0 t\n"
  "MouseMoveEvent 280 400 0 0 0 0 t\n"
  "MouseMoveEvent 240 401 0 0 0 0 t\n"
  "EnterEvent 230 396 0 0 0 0 t\n"
  "MouseMoveEvent 230 396 0 0 0 0 t\n"
  "MouseMoveEvent 178 371 0 0 0 0 t\n"
  "MouseMoveEvent 120 351 0 0 0 0 t\n"
  "MouseMoveEvent 94 341 0 0 0 0 t\n"
  "MouseMoveEvent 68 337 0 0 0 0 t\n"
  "MouseMoveEvent 58 332 0 0 0 0 t\n"
  "MouseMoveEvent 47 319 0 0 0 0 t\n"
  "MouseMoveEvent 42 296 0 0 0 0 t\n"
  "MouseMoveEvent 44 255 0 0 0 0 t\n"
  "MouseMoveEvent 64 223 0 0 0 0 t\n"
  "MouseMoveEvent 86 195 0 0 0 0 t\n"
  "MouseMoveEvent 102 172 0 0 0 0 t\n"
  "MouseMoveEvent 111 159 0 0 0 0 t\n"
  "MouseMoveEvent 115 153 0 0 0 0 t\n"
  "MouseMoveEvent 116 138 0 0 0 0 t\n"
  "MouseMoveEvent 110 124 0 0 0 0 t\n"
  "MouseMoveEvent 99 111 0 0 0 0 t\n"
  "MouseMoveEvent 93 104 0 0 0 0 t\n"
  "MouseMoveEvent 92 102 0 0 0 0 t\n"
  "LeftButtonReleaseEvent 92 102 0 0 0 0 t\n"
  "MouseMoveEvent 92 102 0 0 0 0 t\n"
  "MouseMoveEvent 168 179 0 0 0 0 t\n"
  "LeftButtonPressEvent 168 179 0 0 0 0 t\n"
  "MouseMoveEvent 167 178 0 0 0 0 t\n"
  "MouseMoveEvent 162 172 0 0 0 0 t\n"
  "MouseMoveEvent 153 164 0 0 0 0 t\n"
  "MouseMoveEvent 148 158 0 0 0 0 t\n"
  "MouseMoveEvent 140 150 0 0 0 0 t\n"
  "MouseMoveEvent 135 145 0 0 0 0 t\n"
  "MouseMoveEvent 131 141 0 0 0 0 t\n"
  "MouseMoveEvent 125 135 0 0 0 0 t\n"
  "MouseMoveEvent 123 132 0 0 0 0 t\n"
  "MouseMoveEvent 121 131 0 0 0 0 t\n"
  "MouseMoveEvent 120 129 0 0 0 0 t\n"
  "LeftButtonReleaseEvent 120 129 0 0 0 0 t\n"
  "MouseMoveEvent 120 129 0 0 0 0 t\n"
  "MouseMoveEvent 116 22 0 0 0 0 t\n"
  "LeftButtonPressEvent 116 22 0 0 0 0 t\n"
  "MouseMoveEvent 117 22 0 0 0 0 t\n"
  "MouseMoveEvent 119 20 0 0 0 0 t\n"
  "MouseMoveEvent 130 10 0 0 0 0 t\n"
  "MouseMoveEvent 143 2 0 0 0 0 t\n"
  "MouseMoveEvent 145 0 0 0 0 0 t\n"
  "LeaveEvent 146 -1 0 0 0 0 t\n"
  "MouseMoveEvent 146 -1 0 0 0 0 t\n"
  "MouseMoveEvent 159 -9 0 0 0 0 t\n"
  "MouseMoveEvent 162 -11 0 0 0 0 t\n"
  "MouseMoveEvent 166 -14 0 0 0 0 t\n"
  "MouseMoveEvent 169 -17 0 0 0 0 t\n"
  "MouseMoveEvent 173 -22 0 0 0 0 t\n"
  "MouseMoveEvent 176 -25 0 0 0 0 t\n"
  "MouseMoveEvent 177 -25 0 0 0 0 t\n"
  "LeftButtonReleaseEvent 177 -25 0 0 0 0 t\n"
  "EnterEvent 156 2 0 0 0 0 t\n"
  "MouseMoveEvent 156 2 0 0 0 0 t\n"
  "MouseMoveEvent 140 25 0 0 0 0 t\n"
  "MouseMoveEvent 113 53 0 0 0 0 t\n"
  "MouseMoveEvent 92 74 0 0 0 0 t\n"
  "MouseMoveEvent 76 89 0 0 0 0 t\n"
  "MouseMoveEvent 67 96 0 0 0 0 t\n"
  "MouseMoveEvent 61 105 0 0 0 0 t\n"
  "MouseMoveEvent 57 111 0 0 0 0 t\n"
  "MouseMoveEvent 55 115 0 0 0 0 t\n"
  "MouseMoveEvent 53 121 0 0 0 0 t\n"
  "MouseMoveEvent 51 126 0 0 0 0 t\n"
  "MouseMoveEvent 50 128 0 0 0 0 t\n"
  "LeftButtonPressEvent 50 128 0 0 0 0 t\n"
  "MouseMoveEvent 50 129 0 0 0 0 t\n"
  "MouseMoveEvent 49 133 0 0 0 0 t\n"
  "MouseMoveEvent 46 137 0 0 0 0 t\n"
  "MouseMoveEvent 42 144 0 0 0 0 t\n"
  "MouseMoveEvent 40 149 0 0 0 0 t\n"
  "MouseMoveEvent 38 154 0 0 0 0 t\n"
  "MouseMoveEvent 35 158 0 0 0 0 t\n"
  "MouseMoveEvent 33 161 0 0 0 0 t\n"
  "MouseMoveEvent 29 166 0 0 0 0 t\n"
  "MouseMoveEvent 27 168 0 0 0 0 t\n"
  "MouseMoveEvent 24 168 0 0 0 0 t\n"
  "MouseMoveEvent 22 169 0 0 0 0 t\n"
  "MouseMoveEvent 20 170 0 0 0 0 t\n"
  "MouseMoveEvent 18 170 0 0 0 0 t\n"
  "LeftButtonReleaseEvent 18 170 0 0 0 0 t\n"
  "MouseMoveEvent 18 170 0 0 0 0 t\n"
  "MouseMoveEvent 20 15 0 0 0 0 t\n"
  "LeftButtonPressEvent 20 15 0 0 0 0 t\n"
  "MouseMoveEvent 19 15 0 0 0 0 t\n"
  "MouseMoveEvent 18 13 0 0 0 0 t\n"
  "MouseMoveEvent 16 11 0 0 0 0 t\n"
  "MouseMoveEvent 15 9 0 0 0 0 t\n"
  "MouseMoveEvent 13 7 0 0 0 0 t\n"
  "MouseMoveEvent 10 5 0 0 0 0 t\n"
  "MouseMoveEvent 9 4 0 0 0 0 t\n"
  "LeftButtonReleaseEvent 9 4 0 0 0 0 t\n"
  "MouseMoveEvent 9 4 0 0 0 0 t\n"
  "MouseMoveEvent 119 82 0 0 0 0 t\n"
  "LeftButtonPressEvent 119 82 0 0 0 0 t\n"
  "MouseMoveEvent 120 82 0 0 0 0 t\n"
  "MouseMoveEvent 126 79 0 0 0 0 t\n"
  "MouseMoveEvent 130 75 0 0 0 0 t\n"
  "MouseMoveEvent 131 73 0 0 0 0 t\n"
  "MouseMoveEvent 133 72 0 0 0 0 t\n"
  "MouseMoveEvent 134 72 0 0 0 0 t\n"
  "LeftButtonReleaseEvent 134 72 0 0 0 0 t\n"
  "MouseMoveEvent 134 72 0 0 0 0 t\n"
  "MouseMoveEvent 132 184 0 0 0 0 t\n"
  "LeftButtonPressEvent 132 184 0 0 0 0 t\n"
  "MouseMoveEvent 130 184 0 0 0 0 t\n"
  "MouseMoveEvent 99 179 0 0 0 0 t\n"
  "MouseMoveEvent 94 176 0 0 0 0 t\n"
  "MouseMoveEvent 83 174 0 0 0 0 t\n"
  "MouseMoveEvent 77 186 0 0 0 0 t\n"
  "MouseMoveEvent 67 212 0 0 0 0 t\n"
  "MouseMoveEvent 65 233 0 0 0 0 t\n"
  "MouseMoveEvent 66 244 0 0 0 0 t\n"
  "MouseMoveEvent 72 253 0 0 0 0 t\n"
  "MouseMoveEvent 86 259 0 0 0 0 t\n"
  "MouseMoveEvent 106 266 0 0 0 0 t\n"
  "MouseMoveEvent 132 271 0 0 0 0 t\n"
  "MouseMoveEvent 145 272 0 0 0 0 t\n"
  "MouseMoveEvent 154 264 0 0 0 0 t\n"
  "MouseMoveEvent 163 240 0 0 0 0 t\n"
  "MouseMoveEvent 169 212 0 0 0 0 t\n"
  "MouseMoveEvent 171 199 0 0 0 0 t\n"
  "MouseMoveEvent 170 189 0 0 0 0 t\n"
  "MouseMoveEvent 146 185 0 0 0 0 t\n"
  "MouseMoveEvent 136 188 0 0 0 0 t\n"
  "MouseMoveEvent 127 189 0 0 0 0 t\n"
  "MouseMoveEvent 123 188 0 0 0 0 t\n"
  "LeftButtonReleaseEvent 123 188 0 0 0 0 t\n"
  "MouseMoveEvent 123 188 0 0 0 0 t\n"
  ;

int TestOrientationMarkerWidget( int, char *[] )
{
  // create an actor out of parts of vtkAxesActor and vtkAnnotatedCubeActor
  // to have the widget follow.
  // part 1 is a helical spring to test vtkAxesActor SetUserDefinedShaft
  //
  double dt = vtkMath::Pi() / 20.0;
  double t = 0.0;
  double x = 0.0;
  int nPoints = 120;
  double dx = 0.8 / nPoints;

  vtkSmartPointer<vtkPoints> points =
    vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkCellArray> line =
    vtkSmartPointer<vtkCellArray>::New();
  line->InsertNextCell( nPoints + 80 );

  int i = 0;
  do
    {
    points->InsertPoint( i, 0.1*cos(t), x, 0.1*sin(t) );
    line->InsertCellPoint( i );
    t += dt;
    }
  while( ++i < 40 );

  do
    {
    points->InsertPoint( i, 0.1*cos(t), x, 0.1*sin(t) );
    line->InsertCellPoint(i);
    t += dt;
    x += dx;
    }
  while( ++i < nPoints + 40 );

  do
    {
    points->InsertPoint( i, 0.1*cos(t), x, 0.1*sin(t) );
    line->InsertCellPoint( i );
    t += dt;
    }
  while( ++i < nPoints + 80 );

  vtkSmartPointer<vtkPolyData> wiggle =
    vtkSmartPointer<vtkPolyData>::New();
  wiggle->SetPoints( points );
  wiggle->SetLines( line );

  vtkSmartPointer<vtkTubeFilter> tube =
    vtkSmartPointer<vtkTubeFilter>::New();
  tube->SetInputData( wiggle );
  tube->SetGenerateTCoordsToOff();
  tube->CappingOff();
  tube->SetVaryRadiusToVaryRadiusOff();
  tube->SetRadius( 0.02 );
  tube->SetNumberOfSides( 5 );
  tube->Update();

  // part 2 is generated from vtkAnnotatedCubeActor to test
  // vtkAxesActor SetUserDefinedTip
  //
  vtkSmartPointer<vtkAnnotatedCubeActor> cube =
    vtkSmartPointer<vtkAnnotatedCubeActor>::New();
  cube->SetXPlusFaceText ( "V" );
  cube->SetXMinusFaceText( "K" );
  cube->SetYPlusFaceText ( "T" );
  cube->SetZPlusFaceText ( "" );
  cube->SetZMinusFaceText( "" );
  cube->SetYMinusFaceText( "" );
  cube->SetFaceTextScale( 0.666667 );

  vtkSmartPointer<vtkPropCollection> props =
    vtkSmartPointer<vtkPropCollection>::New();
  cube->GetActors( props );

  vtkSmartPointer<vtkAppendPolyData> append =
    vtkSmartPointer<vtkAppendPolyData>::New();

  vtkSmartPointer<vtkTransformPolyDataFilter> transformFilter =
    vtkSmartPointer<vtkTransformPolyDataFilter>::New();
  vtkSmartPointer<vtkTransform> transform =
    vtkSmartPointer<vtkTransform>::New();
  transformFilter->SetTransform( transform );

  vtkCollectionSimpleIterator sit;
  props->InitTraversal( sit );
  int nprops = props->GetNumberOfItems();

  for ( i = 0; i < nprops; i++ )
    {
    vtkActor *node = vtkActor::SafeDownCast( props->GetNextProp( sit ) );

    // the first prop in the collection will be the cube outline, the last
    // will be the text outlines
    //
    if ( node && (i == 0 || i == (nprops - 1)) )
      {
      vtkPolyData* poly = vtkPolyData::SafeDownCast(node->GetMapper()->GetInput());
      if ( poly )
        {
        transformFilter->SetInputConnection( node->GetMapper()->GetInputConnection(0, 0) );
        transform->Identity();
        transform->SetMatrix( node->GetMatrix() );
        transform->Scale( 2.0, 2.0, 2.0 );
        transformFilter->Update();

        vtkSmartPointer<vtkPolyData> newpoly =
          vtkSmartPointer<vtkPolyData>::New();
        newpoly->DeepCopy( transformFilter->GetOutput() );
        append->AddInputData( newpoly );
        }
      }
    }

  append->Update();

  // the final actor the widget will follow
  //
  vtkSmartPointer<vtkAxesActor> axes =
    vtkSmartPointer<vtkAxesActor>::New();

  axes->SetTotalLength( 1.2, 1.2 , 1.2 );
  axes->SetUserDefinedTip( append->GetOutput() );
  axes->SetTipTypeToUserDefined();
  axes->SetNormalizedShaftLength( 0.85, 0.85, 0.85 );
  axes->SetNormalizedTipLength( 0.15, 0.15, 0.15 );
  axes->AxisLabelsOff();
  axes->SetUserDefinedShaft( tube->GetOutput() );
  axes->SetShaftTypeToUserDefined();

  vtkProperty* property = axes->GetXAxisTipProperty();
  property->SetRepresentationToWireframe();
  property->SetDiffuse(0);
  property->SetAmbient(1);
  property->SetColor( 1, 0, 1 );

  property = axes->GetYAxisTipProperty();
  property->SetRepresentationToWireframe();
  property->SetDiffuse(0);
  property->SetAmbient(1);
  property->SetColor( 1, 1, 0 );

  property = axes->GetZAxisTipProperty();
  property->SetRepresentationToWireframe();
  property->SetDiffuse(0);
  property->SetAmbient(1);
  property->SetColor( 0, 1, 1 );

  // set up the renderer, window, and interactor
  //
  vtkSmartPointer<vtkRenderer> renderer =
    vtkSmartPointer<vtkRenderer>::New();
  renderer->SetBackground( 0.0980, 0.0980, 0.4392 );

  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer( renderer );
  renWin->SetSize( 400, 400 );

  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow( renWin );

  renderer->AddViewProp( axes );

  // set up an interesting view
  //
  vtkCamera* camera = renderer->GetActiveCamera();
  camera->SetViewUp( 0, 0, 1 );
  camera->SetFocalPoint( 0, 0, 0 );
  camera->SetPosition( 4.5, 4.5, 2.5 );
  renderer->ResetCameraClippingRange();

  // the orientation marker passed to the widget will be composed of two
  // actors: vtkAxesActor and a vtkAnnotatedCubeActor
  //
  cube->SetFaceTextScale( 0.65 );
  property = cube->GetCubeProperty();
  property->SetColor( 0.5, 1, 1 );

  property = cube->GetTextEdgesProperty();
  property->SetLineWidth( 1 );
  property->SetDiffuse( 0 );
  property->SetAmbient( 1 );
  property->SetColor( 0.1800, 0.2800, 0.2300 );

  // this static function improves the appearance of the text edges
  // since they are overlaid on a surface rendering of the cube's faces
  //
  vtkMapper::SetResolveCoincidentTopologyToPolygonOffset();

  // anatomic labelling
  //
  cube->SetXPlusFaceText ( "A" );
  cube->SetXMinusFaceText( "P" );
  cube->SetYPlusFaceText ( "L" );
  cube->SetYMinusFaceText( "R" );
  cube->SetZPlusFaceText ( "S" );
  cube->SetZMinusFaceText( "I" );

  // change the vector text colors
  //
  property = cube->GetXPlusFaceProperty();
  property->SetColor(0, 0, 1);
  property->SetInterpolationToFlat();
  property = cube->GetXMinusFaceProperty();
  property->SetColor(0, 0, 1);
  property->SetInterpolationToFlat();
  property = cube->GetYPlusFaceProperty();
  property->SetColor(0, 1, 0);
  property->SetInterpolationToFlat();
  property = cube->GetYMinusFaceProperty();
  property->SetColor(0, 1, 0);
  property->SetInterpolationToFlat();
  property = cube->GetZPlusFaceProperty();
  property->SetColor(1, 0, 0);
  property->SetInterpolationToFlat();
  property = cube->GetZMinusFaceProperty();
  property->SetColor(1, 0, 0);
  property->SetInterpolationToFlat();

  vtkSmartPointer<vtkAxesActor> axes2 =
    vtkSmartPointer<vtkAxesActor>::New();

  // simulate a left-handed coordinate system
  //
  transform->Identity();
  transform->RotateY(90);
  axes2->SetShaftTypeToCylinder();
  axes2->SetUserTransform( transform );
  axes2->SetXAxisLabelText( "w" );
  axes2->SetYAxisLabelText( "v" );
  axes2->SetZAxisLabelText( "u" );

  axes2->SetTotalLength( 1.5, 1.5, 1.5 );
  axes2->SetCylinderRadius( 0.500 * axes2->GetCylinderRadius() );
  axes2->SetConeRadius    ( 1.025 * axes2->GetConeRadius() );
  axes2->SetSphereRadius  ( 1.500 * axes2->GetSphereRadius() );

  vtkTextProperty* tprop = axes2->GetXAxisCaptionActor2D()->
    GetCaptionTextProperty();
  tprop->ItalicOn();
  tprop->ShadowOn();
  tprop->SetFontFamilyToTimes();

  axes2->GetYAxisCaptionActor2D()->GetCaptionTextProperty()->ShallowCopy( tprop );
  axes2->GetZAxisCaptionActor2D()->GetCaptionTextProperty()->ShallowCopy( tprop );

  // combine orientation markers into one with an assembly
  //
  vtkSmartPointer<vtkPropAssembly> assembly =
    vtkSmartPointer<vtkPropAssembly>::New();
  assembly->AddPart( axes2 );
  assembly->AddPart( cube );

  // set up the widget
  //
  vtkSmartPointer<vtkOrientationMarkerWidget> widget =
    vtkSmartPointer<vtkOrientationMarkerWidget>::New();
  widget->SetOutlineColor( 0.9300, 0.5700, 0.1300 );
  widget->SetOrientationMarker( assembly );
  widget->SetInteractor( iren );
  widget->SetViewport( 0.0, 0.0, 0.4, 0.4 );
  widget->SetEnabled( 1 );
  widget->InteractiveOff();
  widget->InteractiveOn();

  // recorder to play back previously events
  //
  vtkSmartPointer<vtkInteractorEventRecorder> recorder =
    vtkSmartPointer<vtkInteractorEventRecorder>::New();
  recorder->SetInteractor(iren);
//  recorder->SetFileName("record.log");
//  recorder->SetKeyPressActivationValue('b');

  recorder->ReadFromInputStringOn();
  recorder->SetInputString(TestOMWidgetEventLog);

  iren->Initialize();
  renWin->Render();
  recorder->Play();

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();

  iren->Start();

  return EXIT_SUCCESS;
}
