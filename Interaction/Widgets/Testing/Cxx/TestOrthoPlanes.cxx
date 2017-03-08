/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOrthoPlanes.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSmartPointer.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellPicker.h"
#include "vtkCommand.h"
#include "vtkImageActor.h"
#include "vtkImageMapper3D.h"
#include "vtkImageMapToColors.h"
#include "vtkImageOrthoPlanes.h"
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
#include "vtkImageData.h"

#include "vtkTestUtilities.h"

static char IOPeventLog[] =
  "# StreamVersion 1\n"
  "CharEvent 179 195 0 0 98 1 i\n"
  "MiddleButtonPressEvent 179 195 0 0 0 0 i\n"
  "MouseMoveEvent 179 190 0 0 0 0 i\n"
  "MouseMoveEvent 179 185 0 0 0 0 i\n"
  "MouseMoveEvent 179 180 0 0 0 0 i\n"
  "MouseMoveEvent 179 175 0 0 0 0 i\n"
  "MouseMoveEvent 179 170 0 0 0 0 i\n"
  "MouseMoveEvent 179 165 0 0 0 0 i\n"
  "MouseMoveEvent 179 160 0 0 0 0 i\n"
  "MouseMoveEvent 179 155 0 0 0 0 i\n"
  "MouseMoveEvent 179 150 0 0 0 0 i\n"
  "MouseMoveEvent 179 145 0 0 0 0 i\n"
  "MouseMoveEvent 179 140 0 0 0 0 i\n"
  "MouseMoveEvent 179 135 0 0 0 0 i\n"
  "MiddleButtonReleaseEvent 179 135 0 0 0 0 i\n"
  "RightButtonPressEvent 179 135 0 0 0 0 i\n"
  "MouseMoveEvent 180 135 0 0 0 0 i\n"
  "MouseMoveEvent 181 136 0 0 0 0 i\n"
  "MouseMoveEvent 181 137 0 0 0 0 i\n"
  "MouseMoveEvent 181 138 0 0 0 0 i\n"
  "MouseMoveEvent 181 139 0 0 0 0 i\n"
  "MouseMoveEvent 181 140 0 0 0 0 i\n"
  "MouseMoveEvent 180 140 0 0 0 0 i\n"
  "MouseMoveEvent 175 135 0 0 0 0 i\n"
  "MouseMoveEvent 170 130 0 0 0 0 i\n"
  "MouseMoveEvent 165 130 0 0 0 0 i\n"
  "MouseMoveEvent 160 130 0 0 0 0 i\n"
  "MouseMoveEvent 155 125 0 0 0 0 i\n"
  "MouseMoveEvent 150 120 0 0 0 0 i\n"
  "MouseMoveEvent 145 115 0 0 0 0 i\n"
  "MouseMoveEvent 140 110 0 0 0 0 i\n"
  "RightButtonReleaseEvent 140 110 0 0 0 0 i\n"
  "MouseMoveEvent 135 120 0 0 0 0 i\n"
  "MouseMoveEvent 130 135 0 0 0 0 i\n"
  "MouseMoveEvent 125 170 0 0 0 0 i\n"
  "MouseMoveEvent 120 180 0 0 0 0 i\n"
  "MouseMoveEvent 115 190 0 0 0 0 i\n"
  "MouseMoveEvent 110 200 0 0 0 0 i\n"
  "MouseMoveEvent 106 218 0 0 0 0 i\n"
  "LeftButtonPressEvent 106 218 0 0 0 0 i\n"
  "MouseMoveEvent 107 219 0 0 0 0 i\n"
  "MouseMoveEvent 110 218 0 0 0 0 i\n"
  "MouseMoveEvent 114 216 0 0 0 0 i\n"
  "MouseMoveEvent 118 214 0 0 0 0 i\n"
  "MouseMoveEvent 123 213 0 0 0 0 i\n"
  "MouseMoveEvent 128 212 0 0 0 0 i\n"
  "MouseMoveEvent 132 210 0 0 0 0 i\n"
  "MouseMoveEvent 138 207 0 0 0 0 i\n"
  "MouseMoveEvent 144 205 0 0 0 0 i\n"
  "MouseMoveEvent 150 203 0 0 0 0 i\n"
  "MouseMoveEvent 157 201 0 0 0 0 i\n"
  "MouseMoveEvent 164 200 0 0 0 0 i\n"
  "MouseMoveEvent 168 198 0 0 0 0 i\n"
  "MouseMoveEvent 176 196 0 0 0 0 i\n"
  "MouseMoveEvent 183 194 0 0 0 0 i\n"
  "MouseMoveEvent 190 192 0 0 0 0 i\n"
  "MouseMoveEvent 197 190 0 0 0 0 i\n"
  "MouseMoveEvent 199 189 0 0 0 0 i\n"
  "MouseMoveEvent 204 189 0 0 0 0 i\n"
  "MouseMoveEvent 206 189 0 0 0 0 i\n"
  "MouseMoveEvent 209 188 0 0 0 0 i\n"
  "MouseMoveEvent 211 187 0 0 0 0 i\n"
  "LeftButtonReleaseEvent 211 187 0 0 0 0 i\n"
  "MouseMoveEvent 259 183 0 0 0 0 i\n"
  "KeyPressEvent 259 183 -128 0 0 1 Control_L\n"
  "MiddleButtonPressEvent 259 183 8 0 0 0 Control_L\n"
  "MouseMoveEvent 261 183 8 0 0 0 Control_L\n"
  "MouseMoveEvent 263 182 8 0 0 0 Control_L\n"
  "MouseMoveEvent 266 181 8 0 0 0 Control_L\n"
  "MouseMoveEvent 268 180 8 0 0 0 Control_L\n"
  "MouseMoveEvent 270 179 8 0 0 0 Control_L\n"
  "MouseMoveEvent 273 178 8 0 0 0 Control_L\n"
  "MouseMoveEvent 276 177 8 0 0 0 Control_L\n"
  "MouseMoveEvent 279 176 8 0 0 0 Control_L\n"
  "MouseMoveEvent 282 175 8 0 0 0 Control_L\n"
  "MouseMoveEvent 287 174 8 0 0 0 Control_L\n"
  "MouseMoveEvent 286 173 8 0 0 0 Control_L\n"
  "MouseMoveEvent 284 173 8 0 0 0 Control_L\n"
  "MouseMoveEvent 281 174 8 0 0 0 Control_L\n"
  "MouseMoveEvent 277 175 8 0 0 0 Control_L\n"
  "MouseMoveEvent 274 176 8 0 0 0 Control_L\n"
  "MouseMoveEvent 269 177 8 0 0 0 Control_L\n"
  "MouseMoveEvent 267 177 8 0 0 0 Control_L\n"
  "KeyReleaseEvent 267 177 0 0 0 1 Control_L\n"
  "MiddleButtonReleaseEvent 267 177 0 0 0 0 Control_L\n"
  "MouseMoveEvent 240 229 0 0 0 0 Control_L\n"
  "KeyPressEvent 240 229 -128 0 0 1 Control_L\n"
  "MiddleButtonPressEvent 240 229 8 0 0 0 Control_L\n"
  "MouseMoveEvent 240 230 8 0 0 0 Control_L\n"
  "MouseMoveEvent 240 235 8 0 0 0 Control_L\n"
  "MouseMoveEvent 240 240 8 0 0 0 Control_L\n"
  "MouseMoveEvent 240 245 8 0 0 0 Control_L\n"
  "MouseMoveEvent 240 250 8 0 0 0 Control_L\n"
  "MouseMoveEvent 241 255 8 0 0 0 Control_L\n"
  "MouseMoveEvent 242 260 8 0 0 0 Control_L\n"
  "MouseMoveEvent 242 265 8 0 0 0 Control_L\n"
  "MouseMoveEvent 242 260 8 0 0 0 Control_L\n"
  "MouseMoveEvent 242 255 8 0 0 0 Control_L\n"
  "MouseMoveEvent 242 250 8 0 0 0 Control_L\n"
  "MouseMoveEvent 242 245 8 0 0 0 Control_L\n"
  "MouseMoveEvent 242 240 8 0 0 0 Control_L\n"
  "MouseMoveEvent 241 238 8 0 0 0 Control_L\n"
  "KeyReleaseEvent 241 238 0 0 0 1 Control_L\n"
  "MiddleButtonReleaseEvent 241 238 0 0 0 0 Control_L\n"
  "MouseMoveEvent 103 250 0 0 0 0 Control_L\n"
  "KeyPressEvent 103 250 -128 0 0 1 Control_L\n"
  "MiddleButtonPressEvent 103 250 8 0 0 0 Control_L\n"
  "MouseMoveEvent 100 250 8 0 0 0 Control_L\n"
  "MouseMoveEvent 97 251 8 0 0 0 Control_L\n"
  "MouseMoveEvent 94 251 8 0 0 0 Control_L\n"
  "MouseMoveEvent 91 252 8 0 0 0 Control_L\n"
  "MouseMoveEvent 90 253 8 0 0 0 Control_L\n"
  "MouseMoveEvent 85 253 8 0 0 0 Control_L\n"
  "MouseMoveEvent 80 253 8 0 0 0 Control_L\n"
  "MouseMoveEvent 85 253 8 0 0 0 Control_L\n"
  "KeyReleaseEvent 85 253 0 0 0 1 Control_L\n"
  "MiddleButtonReleaseEvent 85 253 0 0 0 0 Control_L\n"
  "MouseMoveEvent 228 88 0 0 0 0 Control_L\n"
  "KeyPressEvent 228 88 -128 0 0 1 Control_L\n"
  "MiddleButtonPressEvent 228 88 8 0 0 0 Control_L\n"
  "MouseMoveEvent 228 86 8 0 0 0 Control_L\n"
  "MouseMoveEvent 227 83 8 0 0 0 Control_L\n"
  "MouseMoveEvent 226 83 8 0 0 0 Control_L\n"
  "MouseMoveEvent 225 80 8 0 0 0 Control_L\n"
  "MouseMoveEvent 225 75 8 0 0 0 Control_L\n"
  "MouseMoveEvent 224 70 8 0 0 0 Control_L\n"
  "MouseMoveEvent 223 70 8 0 0 0 Control_L\n"
  "MouseMoveEvent 223 75 8 0 0 0 Control_L\n"
  "MouseMoveEvent 222 80 8 0 0 0 Control_L\n"
  "MouseMoveEvent 222 85 8 0 0 0 Control_L\n"
  "MouseMoveEvent 222 90 8 0 0 0 Control_L\n"
  "KeyReleaseEvent 222 93 0 0 0 1 Control_L\n"
  "MiddleButtonReleaseEvent 222 93 0 0 0 0 Control_L\n"
  "MouseMoveEvent 260 76 0 0 0 0 Control_L\n"
  "KeyPressEvent 260 76 -128 0 0 1 Control_L\n"
  "MiddleButtonPressEvent 260 76 8 0 0 0 Control_L\n"
  "MouseMoveEvent 260 75 8 0 0 0 Control_L\n"
  "MouseMoveEvent 261 72 8 0 0 0 Control_L\n"
  "MouseMoveEvent 262 69 8 0 0 0 Control_L\n"
  "MouseMoveEvent 263 67 8 0 0 0 Control_L\n"
  "MouseMoveEvent 263 65 8 0 0 0 Control_L\n"
  "MouseMoveEvent 264 63 8 0 0 0 Control_L\n"
  "MouseMoveEvent 265 61 8 0 0 0 Control_L\n"
  "MouseMoveEvent 266 60 8 0 0 0 Control_L\n"
  "MouseMoveEvent 266 55 8 0 0 0 Control_L\n"
  "MouseMoveEvent 267 53 8 0 0 0 Control_L\n"
  "KeyReleaseEvent 267 53 0 0 0 1 Control_L\n"
  "MiddleButtonReleaseEvent 267 53 0 0 0 0 Control_L\n"
  "MouseMoveEvent 278 226 0 0 0 0 Control_L\n"
  "KeyPressEvent 278 226 -128 0 0 1 Control_L\n"
  "MiddleButtonPressEvent 278 226 8 0 0 0 Control_L\n"
  "MouseMoveEvent 278 227 8 0 0 0 Control_L\n"
  "MouseMoveEvent 278 230 8 0 0 0 Control_L\n"
  "MouseMoveEvent 280 232 8 0 0 0 Control_L\n"
  "MouseMoveEvent 282 234 8 0 0 0 Control_L\n"
  "MouseMoveEvent 284 237 8 0 0 0 Control_L\n"
  "MouseMoveEvent 286 239 8 0 0 0 Control_L\n"
  "MouseMoveEvent 287 242 8 0 0 0 Control_L\n"
  "MouseMoveEvent 290 245 8 0 0 0 Control_L\n"
  "MouseMoveEvent 292 247 8 0 0 0 Control_L\n"
  "MouseMoveEvent 293 249 8 0 0 0 Control_L\n"
  "KeyReleaseEvent 283 249 0 0 0 1 Control_L\n"
  "MiddleButtonReleaseEvent 293 249 0 0 0 0 Control_L\n"
  "MouseMoveEvent 93 286 0 0 0 0 Control_L\n"
  "KeyPressEvent 93 286 -128 0 0 1 Control_L\n"
  "MiddleButtonPressEvent 93 286 8 0 0 0 Control_L\n"
  "MouseMoveEvent 92 288 8 0 0 0 Control_L\n"
  "MouseMoveEvent 90 290 8 0 0 0 Control_L\n"
  "MouseMoveEvent 87 292 8 0 0 0 Control_L\n"
  "MouseMoveEvent 84 295 8 0 0 0 Control_L\n"
  "MouseMoveEvent 82 297 8 0 0 0 Control_L\n"
  "MouseMoveEvent 80 298 8 0 0 0 Control_L\n"
  "MouseMoveEvent 78 300 8 0 0 0 Control_L\n"
  "KeyReleaseEvent 78 300 0 0 0 1 Control_L\n"
  "MiddleButtonReleaseEvent 78 300 0 0 0 0 Control_L\n"
  "MouseMoveEvent 198 194 0 0 0 0 Control_L\n"
  "KeyPressEvent 198 194 -128 0 0 1 Control_L\n"
  "MiddleButtonPressEvent 198 194 8 0 0 0 Control_L\n"
  "MouseMoveEvent 196 194 8 0 0 0 Control_L\n"
  "MouseMoveEvent 191 192 8 0 0 0 Control_L\n"
  "MouseMoveEvent 185 189 8 0 0 0 Control_L\n"
  "MouseMoveEvent 182 187 8 0 0 0 Control_L\n"
  "MouseMoveEvent 180 186 8 0 0 0 Control_L\n"
  "MouseMoveEvent 178 185 8 0 0 0 Control_L\n"
  "MouseMoveEvent 177 180 8 0 0 0 Control_L\n"
  "MouseMoveEvent 178 179 8 0 0 0 Control_L\n"
  "MouseMoveEvent 179 178 8 0 0 0 Control_L\n"
  "MouseMoveEvent 179 177 8 0 0 0 Control_L\n"
  "MouseMoveEvent 182 176 8 0 0 0 Control_L\n"
  "MouseMoveEvent 187 175 8 0 0 0 Control_L\n"
  "MouseMoveEvent 190 177 8 0 0 0 Control_L\n"
  "MouseMoveEvent 190 179 8 0 0 0 Control_L\n"
  "KeyReleaseEvent 190 179 0 0 0 1 Control_L\n"
  "MiddleButtonReleaseEvent 190 179 0 0 0 0 Control_L\n"
  "KeyPressEvent 190 179 0 -128 0 1 Shift_L\n"
  "MiddleButtonPressEvent 190 179 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 190 180 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 190 185 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 190 190 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 191 194 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 192 200 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 192 206 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 193 213 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 193 209 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 193 206 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 193 200 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 193 196 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 193 190 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 194 185 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 196 180 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 197 175 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 198 172 0 4 0 0 Shift_L\n"
  "KeyReleaseEvent 198 172 0 0 0 1 Shift_L\n"
  "MiddleButtonReleaseEvent 198 172 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 267 172 0 0 0 0 Shift_L\n"
  "MiddleButtonPressEvent 267 172 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 264 171 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 260 171 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 255 171 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 250 171 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 245 172 0 0 0 0 Shift_L\n"
  "MiddleButtonReleaseEvent 245 172 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 203 65 0 0 0 0 Shift_L\n"
  "MiddleButtonPressEvent 203 65 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 200 65 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 195 66 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 193 67 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 190 68 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 184 71 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 180 73 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 178 74 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 176 75 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 175 76 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 174 77 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 173 78 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 172 79 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 170 80 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 169 81 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 168 82 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 167 83 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 166 84 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 164 84 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 163 85 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 162 86 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 160 86 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 158 87 0 0 0 0 Shift_L\n"
  "MiddleButtonReleaseEvent 158 87 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 95 251 0 0 0 0 Shift_L\n"
  "MiddleButtonPressEvent 95 251 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 90 251 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 85 252 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 80 252 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 75 252 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 70 252 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 65 251 0 0 0 0 Shift_L\n"
  "MiddleButtonReleaseEvent 65 251 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 133 281 0 0 0 0 Shift_L\n"
  "MiddleButtonPressEvent 133 281 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 130 280 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 125 277 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 120 274 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 115 270 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 113 267 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 110 265 0 0 0 0 Shift_L\n"
  "MiddleButtonReleaseEvent 110 265 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 99 286 0 0 0 0 Shift_L\n"
  "MiddleButtonPressEvent 99 286 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 100 287 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 105 289 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 110 290 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 115 290 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 120 290 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 125 285 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 129 281 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 130 279 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 128 281 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 126 282 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 123 283 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 120 284 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 115 285 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 110 286 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 106 286 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 102 286 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 99 285 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 95 283 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 92 281 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 89 279 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 88 276 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 86 274 0 0 0 0 Shift_L\n"
  "MiddleButtonReleaseEvent 86 274 0 0 0 0 Shift_L\n"
  ;

//----------------------------------------------------------------------------
class vtkOrthoPlanesCallback : public vtkCommand
{
public:
  static vtkOrthoPlanesCallback *New()
  { return new vtkOrthoPlanesCallback; }

  void Execute( vtkObject *caller, unsigned long vtkNotUsed( event ),
                void *callData ) VTK_OVERRIDE
  {
    vtkImagePlaneWidget* self =
      reinterpret_cast< vtkImagePlaneWidget* >( caller );
    if(!self) return;

    double* wl = static_cast<double*>( callData );

    if ( self == this->WidgetX )
    {
      this->WidgetY->SetWindowLevel(wl[0],wl[1],1);
      this->WidgetZ->SetWindowLevel(wl[0],wl[1],1);
    }
    else if( self == this->WidgetY )
    {
      this->WidgetX->SetWindowLevel(wl[0],wl[1],1);
      this->WidgetZ->SetWindowLevel(wl[0],wl[1],1);
    }
    else if (self == this->WidgetZ)
    {
      this->WidgetX->SetWindowLevel(wl[0],wl[1],1);
      this->WidgetY->SetWindowLevel(wl[0],wl[1],1);
    }
  }

  vtkOrthoPlanesCallback():WidgetX( 0 ), WidgetY( 0 ), WidgetZ ( 0 ) {}

  vtkImagePlaneWidget* WidgetX;
  vtkImagePlaneWidget* WidgetY;
  vtkImagePlaneWidget* WidgetZ;
};

int TestOrthoPlanes( int argc, char *argv[] )
{
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");

  vtkSmartPointer<vtkVolume16Reader> v16 =
    vtkSmartPointer<vtkVolume16Reader>::New();
  v16->SetDataDimensions( 64, 64);
  v16->SetDataByteOrderToLittleEndian();
  v16->SetImageRange( 1, 93);
  v16->SetDataSpacing( 3.2, 3.2, 1.5);
  v16->SetFilePrefix( fname );
  v16->SetDataMask( 0x7fff);
  v16->Update();

  delete[] fname;

  vtkSmartPointer<vtkOutlineFilter> outline =
    vtkSmartPointer<vtkOutlineFilter>::New();
  outline->SetInputConnection(v16->GetOutputPort());

  vtkSmartPointer<vtkPolyDataMapper> outlineMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  outlineMapper->SetInputConnection(outline->GetOutputPort());

  vtkSmartPointer<vtkActor> outlineActor =
    vtkSmartPointer<vtkActor>::New();
  outlineActor->SetMapper( outlineMapper);

  vtkSmartPointer<vtkRenderer> ren1 =
    vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderer> ren2 =
    vtkSmartPointer<vtkRenderer>::New();

  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(ren2);
  renWin->AddRenderer(ren1);

  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  vtkSmartPointer<vtkCellPicker> picker =
    vtkSmartPointer<vtkCellPicker>::New();
  picker->SetTolerance(0.005);

  vtkSmartPointer<vtkProperty> ipwProp =
    vtkSmartPointer<vtkProperty>::New();
  //assign default props to the ipw's texture plane actor

  vtkSmartPointer<vtkImagePlaneWidget> planeWidgetX =
    vtkSmartPointer<vtkImagePlaneWidget>::New();
  planeWidgetX->SetInteractor( iren);
  planeWidgetX->SetKeyPressActivationValue('x');
  planeWidgetX->SetPicker(picker);
  planeWidgetX->RestrictPlaneToVolumeOn();
  planeWidgetX->GetPlaneProperty()->SetColor(1,0,0);
  planeWidgetX->SetTexturePlaneProperty(ipwProp);
  planeWidgetX->TextureInterpolateOff();
  planeWidgetX->SetResliceInterpolateToNearestNeighbour();
  planeWidgetX->SetInputConnection(v16->GetOutputPort());
  planeWidgetX->SetPlaneOrientationToXAxes();
  planeWidgetX->SetSliceIndex(32);
  planeWidgetX->DisplayTextOn();
  planeWidgetX->On();
  planeWidgetX->InteractionOff();
  planeWidgetX->InteractionOn();

  vtkSmartPointer<vtkImagePlaneWidget> planeWidgetY =
    vtkSmartPointer<vtkImagePlaneWidget>::New();
  planeWidgetY->SetInteractor( iren);
  planeWidgetY->SetKeyPressActivationValue('y');
  planeWidgetY->SetPicker(picker);
  planeWidgetY->GetPlaneProperty()->SetColor(1,1,0);
  planeWidgetY->SetTexturePlaneProperty(ipwProp);
  planeWidgetY->TextureInterpolateOn();
  planeWidgetY->SetResliceInterpolateToLinear();
  planeWidgetY->SetInputConnection(v16->GetOutputPort());
  planeWidgetY->SetPlaneOrientationToYAxes();
  planeWidgetY->SetSlicePosition(102.4);
  planeWidgetY->SetLookupTable( planeWidgetX->GetLookupTable());
  planeWidgetY->DisplayTextOff();
  planeWidgetY->UpdatePlacement();
  planeWidgetY->On();

  vtkSmartPointer<vtkImagePlaneWidget> planeWidgetZ =
    vtkSmartPointer<vtkImagePlaneWidget>::New();
  planeWidgetZ->SetInteractor( iren);
  planeWidgetZ->SetKeyPressActivationValue('z');
  planeWidgetZ->SetPicker(picker);
  planeWidgetZ->GetPlaneProperty()->SetColor(0,0,1);
  planeWidgetZ->SetTexturePlaneProperty(ipwProp);
  planeWidgetZ->TextureInterpolateOn();
  planeWidgetZ->SetResliceInterpolateToCubic();
  planeWidgetZ->SetInputConnection(v16->GetOutputPort());
  planeWidgetZ->SetPlaneOrientationToZAxes();
  planeWidgetZ->SetSliceIndex(25);
  planeWidgetZ->SetLookupTable( planeWidgetX->GetLookupTable());
  planeWidgetZ->DisplayTextOn();
  planeWidgetZ->On();

  vtkSmartPointer<vtkImageOrthoPlanes> orthoPlanes =
    vtkSmartPointer<vtkImageOrthoPlanes>::New();
  orthoPlanes->SetPlane(0, planeWidgetX);
  orthoPlanes->SetPlane(1, planeWidgetY);
  orthoPlanes->SetPlane(2, planeWidgetZ);
  orthoPlanes->ResetPlanes();

  vtkSmartPointer<vtkOrthoPlanesCallback> cbk =
    vtkSmartPointer<vtkOrthoPlanesCallback>::New();
  cbk->WidgetX = planeWidgetX;
  cbk->WidgetY = planeWidgetY;
  cbk->WidgetZ = planeWidgetZ;
  planeWidgetX->AddObserver( vtkCommand::EndWindowLevelEvent, cbk );
  planeWidgetY->AddObserver( vtkCommand::EndWindowLevelEvent, cbk );
  planeWidgetZ->AddObserver( vtkCommand::EndWindowLevelEvent, cbk );

  double wl[2];
  planeWidgetZ->GetWindowLevel(wl);

  // Add a 2D image to test the GetReslice method
  //
  vtkSmartPointer<vtkImageMapToColors> colorMap =
    vtkSmartPointer<vtkImageMapToColors>::New();
  colorMap->PassAlphaToOutputOff();
  colorMap->SetActiveComponent(0);
  colorMap->SetOutputFormatToLuminance();
  colorMap->SetInputData(planeWidgetZ->GetResliceOutput());
  colorMap->SetLookupTable(planeWidgetX->GetLookupTable());

  vtkSmartPointer<vtkImageActor> imageActor =
    vtkSmartPointer<vtkImageActor>::New();
  imageActor->PickableOff();
  imageActor->GetMapper()->SetInputConnection(colorMap->GetOutputPort());

  // Add the actors
  //
  ren1->AddActor( outlineActor);
  ren2->AddActor( imageActor);

  ren1->SetBackground( 0.1, 0.1, 0.2);
  ren2->SetBackground( 0.2, 0.1, 0.2);

  renWin->SetSize( 600, 350);

  ren1->SetViewport(0,0,0.58333,1);
  ren2->SetViewport(0.58333,0,1,1);

  // Set the actors' positions
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
  vtkSmartPointer<vtkInteractorEventRecorder> recorder =
    vtkSmartPointer<vtkInteractorEventRecorder>::New();
  recorder->SetInteractor(iren);
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(IOPeventLog);

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

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();

  iren->Start();

  return EXIT_SUCCESS;
}
