/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ImagePlaneWidget.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellPicker.h"
#include "vtkCommand.h"
#include "vtkImageActor.h"
#include "vtkImageMapToColors.h"
#include "vtkImagePlaneWidget.h"
#include "vtkImageReader.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkLookupTable.h"
#include "vtkOutlineFilter.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkVolume16Reader.h"

#include "vtkRegressionTestImage.h"
#include "vtkDebugLeaks.h"

char ImagePlaneWidgetEventLog[] =
"# StreamVersion 1\n"
"CharEvent 168 186 0 0 98 1 i\n"
"KeyReleaseEvent 168 186 0 0 98 1 i\n"
"MiddleButtonPressEvent 168 186 0 0 0 0 i\n"
"MouseMoveEvent 168 181 0 0 0 0 i\n"
"MouseMoveEvent 168 178 0 0 0 0 i\n"
"MouseMoveEvent 169 175 0 0 0 0 i\n"
"MouseMoveEvent 169 146 0 0 0 0 i\n"
"MouseMoveEvent 170 145 0 0 0 0 i\n"
"MouseMoveEvent 171 142 0 0 0 0 i\n"
"MouseMoveEvent 171 137 0 0 0 0 i\n"
"MiddleButtonReleaseEvent 171 137 0 0 0 0 i\n"
"MouseMoveEvent 171 137 0 0 0 0 i\n"
"RightButtonPressEvent 171 137 0 0 0 0 i\n"
"MouseMoveEvent 174 137 0 0 0 0 i\n"
"MouseMoveEvent 176 138 0 0 0 0 i\n"
"MouseMoveEvent 179 138 0 0 0 0 i\n"
"MouseMoveEvent 181 139 0 0 0 0 i\n"
"MouseMoveEvent 183 139 0 0 0 0 i\n"
"MouseMoveEvent 184 140 0 0 0 0 i\n"
"MouseMoveEvent 187 140 0 0 0 0 i\n"
"MouseMoveEvent 188 141 0 0 0 0 i\n"
"MouseMoveEvent 190 141 0 0 0 0 i\n"
"MouseMoveEvent 191 142 0 0 0 0 i\n"
"MouseMoveEvent 193 142 0 0 0 0 i\n"
"MouseMoveEvent 193 143 0 0 0 0 i\n"
"MouseMoveEvent 193 144 0 0 0 0 i\n"
"MouseMoveEvent 191 145 0 0 0 0 i\n"
"MouseMoveEvent 189 146 0 0 0 0 i\n"
"MouseMoveEvent 187 147 0 0 0 0 i\n"
"MouseMoveEvent 185 148 0 0 0 0 i\n"
"MouseMoveEvent 183 150 0 0 0 0 i\n"
"MouseMoveEvent 182 151 0 0 0 0 i\n"
"MouseMoveEvent 181 152 0 0 0 0 i\n"
"MouseMoveEvent 178 154 0 0 0 0 i\n"
"MouseMoveEvent 176 156 0 0 0 0 i\n"
"MouseMoveEvent 174 158 0 0 0 0 i\n"
"MouseMoveEvent 173 159 0 0 0 0 i\n"
"MouseMoveEvent 171 161 0 0 0 0 i\n"
"MouseMoveEvent 171 162 0 0 0 0 i\n"
"MouseMoveEvent 169 163 0 0 0 0 i\n"
"MouseMoveEvent 168 166 0 0 0 0 i\n"
"MouseMoveEvent 167 168 0 0 0 0 i\n"
"MouseMoveEvent 165 170 0 0 0 0 i\n"
"MouseMoveEvent 164 171 0 0 0 0 i\n"
"MouseMoveEvent 163 173 0 0 0 0 i\n"
"MouseMoveEvent 162 173 0 0 0 0 i\n"
"MouseMoveEvent 162 174 0 0 0 0 i\n"
"MouseMoveEvent 160 176 0 0 0 0 i\n"
"MouseMoveEvent 158 178 0 0 0 0 i\n"
"MouseMoveEvent 156 180 0 0 0 0 i\n"
"MouseMoveEvent 153 181 0 0 0 0 i\n"
"MouseMoveEvent 149 184 0 0 0 0 i\n"
"MouseMoveEvent 148 185 0 0 0 0 i\n"
"MouseMoveEvent 145 187 0 0 0 0 i\n"
"MouseMoveEvent 145 188 0 0 0 0 i\n"
"MouseMoveEvent 142 190 0 0 0 0 i\n"
"MouseMoveEvent 141 191 0 0 0 0 i\n"
"MouseMoveEvent 139 192 0 0 0 0 i\n"
"MouseMoveEvent 138 193 0 0 0 0 i\n"
"MouseMoveEvent 137 194 0 0 0 0 i\n"
"MouseMoveEvent 135 195 0 0 0 0 i\n"
"MouseMoveEvent 133 196 0 0 0 0 i\n"
"MouseMoveEvent 130 198 0 0 0 0 i\n"
"MouseMoveEvent 128 200 0 0 0 0 i\n"
"MouseMoveEvent 125 202 0 0 0 0 i\n"
"MouseMoveEvent 124 202 0 0 0 0 i\n"
"MouseMoveEvent 124 203 0 0 0 0 i\n"
"MouseMoveEvent 122 203 0 0 0 0 i\n"
"MouseMoveEvent 121 204 0 0 0 0 i\n"
"MouseMoveEvent 120 206 0 0 0 0 i\n"
"MouseMoveEvent 116 208 0 0 0 0 i\n"
"MouseMoveEvent 114 209 0 0 0 0 i\n"
"MouseMoveEvent 113 209 0 0 0 0 i\n"
"MouseMoveEvent 111 210 0 0 0 0 i\n"
"MouseMoveEvent 111 211 0 0 0 0 i\n"
"MouseMoveEvent 110 211 0 0 0 0 i\n"
"MouseMoveEvent 109 212 0 0 0 0 i\n"
"MouseMoveEvent 106 213 0 0 0 0 i\n"
"MouseMoveEvent 104 214 0 0 0 0 i\n"
"MouseMoveEvent 101 215 0 0 0 0 i\n"
"MouseMoveEvent 100 215 0 0 0 0 i\n"
"MouseMoveEvent 99 216 0 0 0 0 i\n"
"MouseMoveEvent 96 216 0 0 0 0 i\n"
"MouseMoveEvent 96 217 0 0 0 0 i\n"
"MouseMoveEvent 90 217 0 0 0 0 i\n"
"MouseMoveEvent 90 216 0 0 0 0 i\n"
"MouseMoveEvent 89 216 0 0 0 0 i\n"
"RightButtonReleaseEvent 89 216 0 0 0 0 i\n"
"MouseMoveEvent 89 215 0 0 0 0 i\n"
"LeftButtonPressEvent 89 215 0 0 0 0 i\n"
"MouseMoveEvent 90 215 0 0 0 0 i\n"
"MouseMoveEvent 94 215 0 0 0 0 i\n"
"MouseMoveEvent 99 215 0 0 0 0 i\n"
"MouseMoveEvent 103 215 0 0 0 0 i\n"
"MouseMoveEvent 107 216 0 0 0 0 i\n"
"MouseMoveEvent 109 216 0 0 0 0 i\n"
"MouseMoveEvent 113 217 0 0 0 0 i\n"
"MouseMoveEvent 118 219 0 0 0 0 i\n"
"MouseMoveEvent 120 219 0 0 0 0 i\n"
"MouseMoveEvent 124 220 0 0 0 0 i\n"
"MouseMoveEvent 126 220 0 0 0 0 i\n"
"MouseMoveEvent 129 221 0 0 0 0 i\n"
"MouseMoveEvent 131 222 0 0 0 0 i\n"
"MouseMoveEvent 137 223 0 0 0 0 i\n"
"MouseMoveEvent 141 223 0 0 0 0 i\n"
"MouseMoveEvent 153 223 0 0 0 0 i\n"
"MouseMoveEvent 157 223 0 0 0 0 i\n"
"MouseMoveEvent 160 222 0 0 0 0 i\n"
"MouseMoveEvent 163 221 0 0 0 0 i\n"
"MouseMoveEvent 165 220 0 0 0 0 i\n"
"MouseMoveEvent 166 219 0 0 0 0 i\n"
"MouseMoveEvent 169 218 0 0 0 0 i\n"
"MouseMoveEvent 173 215 0 0 0 0 i\n"
"MouseMoveEvent 175 214 0 0 0 0 i\n"
"MouseMoveEvent 177 212 0 0 0 0 i\n"
"MouseMoveEvent 179 211 0 0 0 0 i\n"
"MouseMoveEvent 180 210 0 0 0 0 i\n"
"MouseMoveEvent 182 209 0 0 0 0 i\n"
"MouseMoveEvent 184 208 0 0 0 0 i\n"
"MouseMoveEvent 186 207 0 0 0 0 i\n"
"MouseMoveEvent 191 205 0 0 0 0 i\n"
"MouseMoveEvent 199 204 0 0 0 0 i\n"
"MouseMoveEvent 203 204 0 0 0 0 i\n"
"MouseMoveEvent 204 203 0 0 0 0 i\n"
"LeftButtonReleaseEvent 204 203 0 0 0 0 i\n"
"MouseMoveEvent 198 204 0 0 0 0 i\n"
"KeyPressEvent 198 204 0 -128 0 1 Shift_L\n"
"MiddleButtonPressEvent 198 204 0 4 0 0 Shift_L\n"
"MouseMoveEvent 198 206 0 4 0 0 Shift_L\n"
"MouseMoveEvent 198 210 0 4 0 0 Shift_L\n"
"MouseMoveEvent 199 211 0 4 0 0 Shift_L\n"
"MouseMoveEvent 200 213 0 4 0 0 Shift_L\n"
"MouseMoveEvent 201 215 0 4 0 0 Shift_L\n"
"MouseMoveEvent 202 217 0 4 0 0 Shift_L\n"
"MouseMoveEvent 203 218 0 4 0 0 Shift_L\n"
"MouseMoveEvent 203 220 0 4 0 0 Shift_L\n"
"MouseMoveEvent 204 221 0 4 0 0 Shift_L\n"
"MouseMoveEvent 204 224 0 4 0 0 Shift_L\n"
"MouseMoveEvent 205 224 0 4 0 0 Shift_L\n"
"MouseMoveEvent 205 226 0 4 0 0 Shift_L\n"
"MouseMoveEvent 206 228 0 4 0 0 Shift_L\n"
"MouseMoveEvent 207 230 0 4 0 0 Shift_L\n"
"MouseMoveEvent 207 233 0 4 0 0 Shift_L\n"
"MouseMoveEvent 208 235 0 4 0 0 Shift_L\n"
"MouseMoveEvent 209 237 0 4 0 0 Shift_L\n"
"MouseMoveEvent 209 239 0 4 0 0 Shift_L\n"
"MouseMoveEvent 210 240 0 4 0 0 Shift_L\n"
"MouseMoveEvent 210 243 0 4 0 0 Shift_L\n"
"MouseMoveEvent 211 244 0 4 0 0 Shift_L\n"
"MouseMoveEvent 211 246 0 4 0 0 Shift_L\n"
"MouseMoveEvent 212 246 0 4 0 0 Shift_L\n"
"MouseMoveEvent 212 250 0 4 0 0 Shift_L\n"
"MouseMoveEvent 213 251 0 4 0 0 Shift_L\n"
"MouseMoveEvent 214 253 0 4 0 0 Shift_L\n"
"MouseMoveEvent 215 254 0 4 0 0 Shift_L\n"
"MouseMoveEvent 215 253 0 4 0 0 Shift_L\n"
"MouseMoveEvent 215 250 0 4 0 0 Shift_L\n"
"MouseMoveEvent 215 247 0 4 0 0 Shift_L\n"
"MouseMoveEvent 215 245 0 4 0 0 Shift_L\n"
"MouseMoveEvent 214 245 0 4 0 0 Shift_L\n"
"MiddleButtonReleaseEvent 214 245 0 4 0 0 Shift_L\n"
"MouseMoveEvent 214 245 0 4 0 0 Shift_L\n"
"KeyReleaseEvent 214 245 0 0 0 1 Shift_L\n"
"KeyPressEvent 214 245 -128 0 0 1 Control_L\n"
"LeftButtonPressEvent 214 245 8 0 0 0 Control_L\n"
"MouseMoveEvent 215 247 8 0 0 0 Control_L\n"
"MouseMoveEvent 215 250 8 0 0 0 Control_L\n"
"MouseMoveEvent 216 251 8 0 0 0 Control_L\n"
"MouseMoveEvent 216 256 8 0 0 0 Control_L\n"
"MouseMoveEvent 217 257 8 0 0 0 Control_L\n"
"MouseMoveEvent 217 261 8 0 0 0 Control_L\n"
"LeftButtonReleaseEvent 217 261 8 0 0 0 Control_L\n"
"MouseMoveEvent 216 249 8 0 0 0 Control_L\n"
"MiddleButtonPressEvent 216 249 8 0 0 0 Control_L\n"
"MouseMoveEvent 216 253 8 0 0 0 Control_L\n"
"MouseMoveEvent 216 258 8 0 0 0 Control_L\n"
"MouseMoveEvent 216 262 8 0 0 0 Control_L\n"
"MouseMoveEvent 216 266 8 0 0 0 Control_L\n"
"MouseMoveEvent 216 271 8 0 0 0 Control_L\n"
"MouseMoveEvent 216 275 8 0 0 0 Control_L\n"
"MouseMoveEvent 217 279 8 0 0 0 Control_L\n"
"MouseMoveEvent 217 280 8 0 0 0 Control_L\n"
"MouseMoveEvent 217 276 8 0 0 0 Control_L\n"
"MouseMoveEvent 217 272 8 0 0 0 Control_L\n"
"MouseMoveEvent 217 268 8 0 0 0 Control_L\n"
"MouseMoveEvent 217 264 8 0 0 0 Control_L\n"
"MouseMoveEvent 217 258 8 0 0 0 Control_L\n"
"MouseMoveEvent 217 255 8 0 0 0 Control_L\n"
"MiddleButtonReleaseEvent 217 255 8 0 0 0 Control_L\n"
"MouseMoveEvent 282 242 8 0 0 0 Control_L\n"
"MiddleButtonPressEvent 282 242 8 0 0 0 Control_L\n"
"MouseMoveEvent 283 242 8 0 0 0 Control_L\n"
"MouseMoveEvent 288 243 8 0 0 0 Control_L\n"
"MouseMoveEvent 291 245 8 0 0 0 Control_L\n"
"MouseMoveEvent 296 247 8 0 0 0 Control_L\n"
"MouseMoveEvent 304 251 8 0 0 0 Control_L\n"
"MouseMoveEvent 313 253 8 0 0 0 Control_L\n"
"MouseMoveEvent 320 255 8 0 0 0 Control_L\n"
"MouseMoveEvent 325 257 8 0 0 0 Control_L\n"
"MouseMoveEvent 318 256 8 0 0 0 Control_L\n"
"MouseMoveEvent 312 254 8 0 0 0 Control_L\n"
"MouseMoveEvent 305 252 8 0 0 0 Control_L\n"
"MouseMoveEvent 297 250 8 0 0 0 Control_L\n"
"MouseMoveEvent 293 248 8 0 0 0 Control_L\n"
"MouseMoveEvent 290 247 8 0 0 0 Control_L\n"
"MouseMoveEvent 283 244 8 0 0 0 Control_L\n"
"MouseMoveEvent 277 241 8 0 0 0 Control_L\n"
"MiddleButtonReleaseEvent 277 241 8 0 0 0 Control_L\n"
"MouseMoveEvent 274 204 8 0 0 0 Control_L\n"
"MiddleButtonPressEvent 274 204 8 0 0 0 Control_L\n"
"MouseMoveEvent 275 203 8 0 0 0 Control_L\n"
"MouseMoveEvent 277 202 8 0 0 0 Control_L\n"
"MouseMoveEvent 283 201 8 0 0 0 Control_L\n"
"MouseMoveEvent 288 200 8 0 0 0 Control_L\n"
"MouseMoveEvent 292 199 8 0 0 0 Control_L\n"
"MouseMoveEvent 295 198 8 0 0 0 Control_L\n"
"MouseMoveEvent 300 196 8 0 0 0 Control_L\n"
"MouseMoveEvent 304 195 8 0 0 0 Control_L\n"
"MouseMoveEvent 303 195 8 0 0 0 Control_L\n"
"MouseMoveEvent 298 195 8 0 0 0 Control_L\n"
"MouseMoveEvent 289 195 8 0 0 0 Control_L\n"
"MouseMoveEvent 285 195 8 0 0 0 Control_L\n"
"MouseMoveEvent 283 195 8 0 0 0 Control_L\n"
"MouseMoveEvent 279 195 8 0 0 0 Control_L\n"
"MouseMoveEvent 277 195 8 0 0 0 Control_L\n"
"MouseMoveEvent 274 195 8 0 0 0 Control_L\n"
"MouseMoveEvent 272 195 8 0 0 0 Control_L\n"
"MouseMoveEvent 270 195 8 0 0 0 Control_L\n"
"MouseMoveEvent 265 194 8 0 0 0 Control_L\n"
"MiddleButtonReleaseEvent 265 194 8 0 0 0 Control_L\n"
"KeyReleaseEvent 265 194 0 0 0 1 Control_L\n"
"MouseMoveEvent 266 194 0 0 0 0 Control_L\n"
"MiddleButtonPressEvent 266 194 0 0 0 0 Control_L\n"
"MouseMoveEvent 267 194 0 0 0 0 Control_L\n"
"MouseMoveEvent 271 194 0 0 0 0 Control_L\n"
"MouseMoveEvent 275 195 0 0 0 0 Control_L\n"
"MouseMoveEvent 279 197 0 0 0 0 Control_L\n"
"MouseMoveEvent 284 199 0 0 0 0 Control_L\n"
"MouseMoveEvent 287 201 0 0 0 0 Control_L\n"
"MouseMoveEvent 291 203 0 0 0 0 Control_L\n"
"MouseMoveEvent 295 206 0 0 0 0 Control_L\n"
"MouseMoveEvent 299 208 0 0 0 0 Control_L\n"
"MouseMoveEvent 301 210 0 0 0 0 Control_L\n"
"MouseMoveEvent 303 213 0 0 0 0 Control_L\n"
"MouseMoveEvent 307 217 0 0 0 0 Control_L\n"
"MouseMoveEvent 309 218 0 0 0 0 Control_L\n"
"MiddleButtonReleaseEvent 309 218 0 0 0 0 Control_L\n"
"MouseMoveEvent 289 245 0 0 0 0 Control_L\n"
"MiddleButtonPressEvent 289 245 0 0 0 0 Control_L\n"
"MouseMoveEvent 290 244 0 0 0 0 Control_L\n"
"MouseMoveEvent 292 242 0 0 0 0 Control_L\n"
"MouseMoveEvent 293 239 0 0 0 0 Control_L\n"
"MouseMoveEvent 294 238 0 0 0 0 Control_L\n"
"MouseMoveEvent 294 225 0 0 0 0 Control_L\n"
"MouseMoveEvent 295 224 0 0 0 0 Control_L\n"
"MouseMoveEvent 295 218 0 0 0 0 Control_L\n"
"MouseMoveEvent 295 205 0 0 0 0 Control_L\n"
"MouseMoveEvent 295 203 0 0 0 0 Control_L\n"
"MouseMoveEvent 294 202 0 0 0 0 Control_L\n"
"MouseMoveEvent 294 198 0 0 0 0 Control_L\n"
"MouseMoveEvent 293 197 0 0 0 0 Control_L\n"
"MouseMoveEvent 292 193 0 0 0 0 Control_L\n"
"MouseMoveEvent 291 189 0 0 0 0 Control_L\n"
"MouseMoveEvent 290 185 0 0 0 0 Control_L\n"
"MouseMoveEvent 290 181 0 0 0 0 Control_L\n"
"MouseMoveEvent 290 179 0 0 0 0 Control_L\n"
"MouseMoveEvent 290 177 0 0 0 0 Control_L\n"
"MiddleButtonReleaseEvent 290 177 0 0 0 0 Control_L\n"
"MouseMoveEvent 246 223 0 0 0 0 Control_L\n"
"MiddleButtonPressEvent 246 223 0 0 0 0 Control_L\n"
"MouseMoveEvent 244 223 0 0 0 0 Control_L\n"
"MouseMoveEvent 239 223 0 0 0 0 Control_L\n"
"MouseMoveEvent 237 222 0 0 0 0 Control_L\n"
"MouseMoveEvent 233 221 0 0 0 0 Control_L\n"
"MouseMoveEvent 231 220 0 0 0 0 Control_L\n"
"MouseMoveEvent 229 219 0 0 0 0 Control_L\n"
"MouseMoveEvent 227 218 0 0 0 0 Control_L\n"
"MouseMoveEvent 225 217 0 0 0 0 Control_L\n"
"MouseMoveEvent 222 216 0 0 0 0 Control_L\n"
"MouseMoveEvent 220 215 0 0 0 0 Control_L\n"
"MouseMoveEvent 218 214 0 0 0 0 Control_L\n"
"MouseMoveEvent 216 213 0 0 0 0 Control_L\n"
"MouseMoveEvent 215 212 0 0 0 0 Control_L\n"
"MiddleButtonReleaseEvent 215 212 0 0 0 0 Control_L\n";

int ImagePlaneWidget( int argc, char *argv[] )
{
  vtkDebugLeaks::PromptUserOff();

  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");

  vtkVolume16Reader* v16 =  vtkVolume16Reader::New();
    v16->SetDataDimensions( 64, 64);
    v16->SetDataByteOrderToLittleEndian();
    v16->SetImageRange( 1, 93);
    v16->SetDataSpacing( 3.2, 3.2, 1.5);
    v16->SetFilePrefix( fname );
    v16->SetDataMask( 0x7fff);
    v16->Update();

  delete[] fname;

  vtkOutlineFilter* outline = vtkOutlineFilter::New();
    outline->SetInput(v16->GetOutput());

  vtkPolyDataMapper* outlineMapper = vtkPolyDataMapper::New();
    outlineMapper->SetInput(outline->GetOutput());

  vtkActor* outlineActor =  vtkActor::New();
    outlineActor->SetMapper( outlineMapper);

  vtkRenderer* ren1 = vtkRenderer::New();
  vtkRenderer* ren2 = vtkRenderer::New();

  vtkRenderWindow* renWin = vtkRenderWindow::New();
    renWin->AddRenderer(ren2);
    renWin->AddRenderer(ren1);

  vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);

  vtkCellPicker* picker = vtkCellPicker::New();
    picker->SetTolerance(0.005);

  vtkImagePlaneWidget* planeWidgetX = vtkImagePlaneWidget::New();
    planeWidgetX->SetInteractor( iren);
    planeWidgetX->SetKeyPressActivationValue('x');
    planeWidgetX->SetPicker(picker);
    planeWidgetX->GetPlaneProperty()->SetColor(1,0,0);
    planeWidgetX->TextureInterpolateOff();
    planeWidgetX->SetResliceInterpolateToNearestNeighbour();
    planeWidgetX->SetInput(v16->GetOutput());
    planeWidgetX->SetPlaneOrientationToXAxes();
    planeWidgetX->SetSliceIndex(32);
    planeWidgetX->DisplayTextOn();
    planeWidgetX->On();

  vtkImagePlaneWidget* planeWidgetY = vtkImagePlaneWidget::New();
    planeWidgetY->SetInteractor( iren);
    planeWidgetY->SetKeyPressActivationValue('y');
    planeWidgetY->SetPicker(picker);
    planeWidgetY->GetPlaneProperty()->SetColor(1,1,0);
    planeWidgetY->TextureInterpolateOn();
    planeWidgetY->SetResliceInterpolateToLinear();
    planeWidgetY->SetInput(v16->GetOutput());
    planeWidgetY->SetPlaneOrientationToYAxes();
    planeWidgetY->SetSlicePosition(102.4);
    planeWidgetY->SetLookupTable( planeWidgetX->GetLookupTable());
    planeWidgetY->DisplayTextOn();
    planeWidgetY->On();

  vtkImagePlaneWidget* planeWidgetZ = vtkImagePlaneWidget::New();
    planeWidgetZ->SetInteractor( iren);
    planeWidgetZ->SetKeyPressActivationValue('z');
    planeWidgetZ->SetPicker(picker);
    planeWidgetZ->GetPlaneProperty()->SetColor(0,0,1);
    planeWidgetZ->TextureInterpolateOn();
    planeWidgetZ->SetResliceInterpolateToCubic();
    planeWidgetZ->SetInput(v16->GetOutput());
    planeWidgetZ->SetPlaneOrientationToZAxes();
    planeWidgetZ->SetSliceIndex(25);
    planeWidgetZ->SetLookupTable( planeWidgetX->GetLookupTable());
    planeWidgetZ->DisplayTextOff();
    planeWidgetZ->On();

  // Add a 2D image to test the GetReslice method
  //
  vtkImageMapToColors* colorMap = vtkImageMapToColors::New();
    colorMap->PassAlphaToOutputOff();
    colorMap->SetActiveComponent(0);
    colorMap->SetOutputFormatToLuminance();
    colorMap->SetInput(planeWidgetZ->GetResliceOutput());
    colorMap->SetLookupTable(planeWidgetX->GetLookupTable());

  vtkImageActor* imageActor = vtkImageActor::New();
    imageActor->PickableOff();
    imageActor->SetInput(colorMap->GetOutput());

  // Add the actors
  //
  ren1->AddActor( outlineActor);
  ren2->AddActor( imageActor);

  ren1->SetBackground( 0.1, 0.1, 0.2);
  ren2->SetBackground( 0.2, 0.1, 0.2);

  renWin->SetSize( 600, 350);

  ren1->SetViewport(0,0,0.58333,1);
  ren2->SetViewport(0.58333,0,1,1);

  // Set the actors' postions
  //
  renWin->Render();
  iren->SetEventPosition( 175,175);
  iren->SetKeyCode('r');
  iren->InvokeEvent(vtkCommand::CharEvent,NULL);
  iren->SetEventPosition( 475,175);
  iren->SetKeyCode('r');
  iren->InvokeEvent(vtkCommand::CharEvent,NULL);
  renWin->Render();

  ren1->GetActiveCamera()->Elevation(110);
  ren1->GetActiveCamera()->SetViewUp(0, 0, -1);
  ren1->GetActiveCamera()->Azimuth(45);
  ren1->GetActiveCamera()->Dolly(1.15);
  ren1->ResetCameraClippingRange();

  // Playback recorded events
  //
  vtkInteractorEventRecorder *recorder = vtkInteractorEventRecorder::New();
  recorder->SetInteractor(iren);
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(ImagePlaneWidgetEventLog);

  // Interact with data
  // Render the image
  //
  iren->Initialize();
  renWin->Render();

  // Test SetKeyPressActivationValue for one of the widgets
  //
  iren->SetKeyCode('z');
  iren->InvokeEvent(vtkCommand::CharEvent,NULL);
  iren->SetKeyCode('z');
  iren->InvokeEvent(vtkCommand::CharEvent,NULL);

  recorder->Play();

  int retVal = vtkRegressionTestImage( renWin );
  
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  // Clean up
  //
  recorder->Off();
  recorder->Delete();

  planeWidgetX->Delete();
  planeWidgetY->Delete();
  planeWidgetZ->Delete();
  colorMap->Delete();
  imageActor->Delete();
  picker->Delete();
  outlineActor->Delete();
  outlineMapper->Delete();
  outline->Delete();
  iren->Delete();
  renWin->Delete();
  ren1->Delete();
  ren2->Delete();
  v16->Delete();

  return !retVal;
}
