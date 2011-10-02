/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGenericCell.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test demonstrates the vtkCellCentersPointPlacer. The placer may
// be used to constrain handle widgets to the centers of cells. Thus it
// may be used by any of the widgets that use the handles (distance, angle
// etc).
//   Here we demonstrates constraining  the distance widget to the centers
// of various cells.
//
#include <vtkSmartPointer.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkUnstructuredGrid.h>
#include <vtkDataSetMapper.h>
#include <vtkActor.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkCamera.h>
#include <vtkTransformFilter.h>
#include <vtkMatrixToLinearTransform.h>
#include <vtkMatrix4x4.h>
#include <vtkProperty.h>
#include <vtkDistanceWidget.h>
#include <vtkDistanceRepresentation2D.h>
#include <vtkCellCentersPointPlacer.h>
#include <vtkPointHandleRepresentation3D.h>
#include <vtkCellPicker.h>
#include <vtkAxisActor2D.h>
#include <vtkProperty2D.h>
#include <vtkTesting.h>

#include <vtkHexagonalPrism.h>
#include <vtkHexahedron.h>
#include <vtkPentagonalPrism.h>
#include <vtkPolyhedron.h>
#include <vtkPyramid.h>
#include <vtkTetra.h>
#include <vtkVoxel.h>
#include <vtkWedge.h>

#include <vector>

//---------------------------------------------------------------------------
char TestCellCentersPointPlacerEventLog[] =
"# StreamVersion 1\n"
"EnterEvent 384 226 0 0 0 0 0\n"
"MouseMoveEvent 384 226 0 0 0 0 0\n"
"RenderEvent 384 226 0 0 0 0 0\n"
"MouseMoveEvent 384 226 0 0 0 0 0\n"
"MouseMoveEvent 383 226 0 0 0 0 0\n"
"MouseMoveEvent 382 227 0 0 0 0 0\n"
"MouseMoveEvent 381 227 0 0 0 0 0\n"
"MouseMoveEvent 381 229 0 0 0 0 0\n"
"MouseMoveEvent 379 229 0 0 0 0 0\n"
"MouseMoveEvent 377 230 0 0 0 0 0\n"
"MouseMoveEvent 376 231 0 0 0 0 0\n"
"MouseMoveEvent 374 232 0 0 0 0 0\n"
"MouseMoveEvent 370 234 0 0 0 0 0\n"
"MouseMoveEvent 366 236 0 0 0 0 0\n"
"MouseMoveEvent 360 237 0 0 0 0 0\n"
"MouseMoveEvent 355 237 0 0 0 0 0\n"
"MouseMoveEvent 348 237 0 0 0 0 0\n"
"MouseMoveEvent 342 237 0 0 0 0 0\n"
"MouseMoveEvent 336 237 0 0 0 0 0\n"
"MouseMoveEvent 330 237 0 0 0 0 0\n"
"MouseMoveEvent 324 237 0 0 0 0 0\n"
"MouseMoveEvent 318 237 0 0 0 0 0\n"
"MouseMoveEvent 313 237 0 0 0 0 0\n"
"MouseMoveEvent 307 237 0 0 0 0 0\n"
"MouseMoveEvent 303 237 0 0 0 0 0\n"
"MouseMoveEvent 299 237 0 0 0 0 0\n"
"MouseMoveEvent 293 237 0 0 0 0 0\n"
"MouseMoveEvent 290 235 0 0 0 0 0\n"
"MouseMoveEvent 287 234 0 0 0 0 0\n"
"MouseMoveEvent 285 233 0 0 0 0 0\n"
"MouseMoveEvent 282 232 0 0 0 0 0\n"
"MouseMoveEvent 280 231 0 0 0 0 0\n"
"MouseMoveEvent 278 230 0 0 0 0 0\n"
"MouseMoveEvent 276 230 0 0 0 0 0\n"
"MouseMoveEvent 275 229 0 0 0 0 0\n"
"MouseMoveEvent 274 229 0 0 0 0 0\n"
"MouseMoveEvent 274 228 0 0 0 0 0\n"
"MouseMoveEvent 272 221 0 0 0 0 0\n"
"MouseMoveEvent 272 213 0 0 0 0 0\n"
"MouseMoveEvent 270 202 0 0 0 0 0\n"
"MouseMoveEvent 269 193 0 0 0 0 0\n"
"MouseMoveEvent 269 184 0 0 0 0 0\n"
"MouseMoveEvent 267 174 0 0 0 0 0\n"
"MouseMoveEvent 267 165 0 0 0 0 0\n"
"MouseMoveEvent 267 157 0 0 0 0 0\n"
"MouseMoveEvent 267 151 0 0 0 0 0\n"
"MouseMoveEvent 265 147 0 0 0 0 0\n"
"MouseMoveEvent 264 144 0 0 0 0 0\n"
"MouseMoveEvent 263 142 0 0 0 0 0\n"
"MouseMoveEvent 262 139 0 0 0 0 0\n"
"MouseMoveEvent 262 137 0 0 0 0 0\n"
"MouseMoveEvent 261 134 0 0 0 0 0\n"
"MouseMoveEvent 259 132 0 0 0 0 0\n"
"MouseMoveEvent 259 129 0 0 0 0 0\n"
"MouseMoveEvent 257 127 0 0 0 0 0\n"
"MouseMoveEvent 256 124 0 0 0 0 0\n"
"MouseMoveEvent 255 123 0 0 0 0 0\n"
"MouseMoveEvent 254 121 0 0 0 0 0\n"
"MouseMoveEvent 253 121 0 0 0 0 0\n"
"MouseMoveEvent 251 121 0 0 0 0 0\n"
"MouseMoveEvent 251 120 0 0 0 0 0\n"
"MouseMoveEvent 250 120 0 0 0 0 0\n"
"MouseMoveEvent 249 120 0 0 0 0 0\n"
"MouseMoveEvent 248 120 0 0 0 0 0\n"
"MouseMoveEvent 247 120 0 0 0 0 0\n"
"MouseMoveEvent 246 120 0 0 0 0 0\n"
"MouseMoveEvent 245 120 0 0 0 0 0\n"
"MouseMoveEvent 244 119 0 0 0 0 0\n"
"MouseMoveEvent 242 119 0 0 0 0 0\n"
"MouseMoveEvent 240 119 0 0 0 0 0\n"
"MouseMoveEvent 238 119 0 0 0 0 0\n"
"MouseMoveEvent 237 119 0 0 0 0 0\n"
"MouseMoveEvent 236 119 0 0 0 0 0\n"
"MouseMoveEvent 235 119 0 0 0 0 0\n"
"MouseMoveEvent 234 119 0 0 0 0 0\n"
"MouseMoveEvent 233 119 0 0 0 0 0\n"
"MouseMoveEvent 229 119 0 0 0 0 0\n"
"MouseMoveEvent 227 119 0 0 0 0 0\n"
"MouseMoveEvent 224 119 0 0 0 0 0\n"
"MouseMoveEvent 220 119 0 0 0 0 0\n"
"MouseMoveEvent 217 118 0 0 0 0 0\n"
"MouseMoveEvent 215 117 0 0 0 0 0\n"
"MouseMoveEvent 213 116 0 0 0 0 0\n"
"MouseMoveEvent 211 115 0 0 0 0 0\n"
"MouseMoveEvent 209 114 0 0 0 0 0\n"
"MouseMoveEvent 207 114 0 0 0 0 0\n"
"MouseMoveEvent 205 113 0 0 0 0 0\n"
"MouseMoveEvent 204 111 0 0 0 0 0\n"
"MouseMoveEvent 203 111 0 0 0 0 0\n"
"MouseMoveEvent 202 110 0 0 0 0 0\n"
"MouseMoveEvent 201 110 0 0 0 0 0\n"
"MouseMoveEvent 200 110 0 0 0 0 0\n"
"MouseMoveEvent 199 109 0 0 0 0 0\n"
"MouseMoveEvent 198 108 0 0 0 0 0\n"
"MouseMoveEvent 198 107 0 0 0 0 0\n"
"MouseMoveEvent 197 106 0 0 0 0 0\n"
"LeftButtonPressEvent 197 106 0 0 0 0 0\n"
"RenderEvent 197 106 0 0 0 0 0\n"
"LeftButtonReleaseEvent 197 106 0 0 0 0 0\n"
"MouseMoveEvent 197 106 0 0 0 0 0\n"
"RenderEvent 197 106 0 0 0 0 0\n"
"MouseMoveEvent 197 107 0 0 0 0 0\n"
"RenderEvent 197 107 0 0 0 0 0\n"
"MouseMoveEvent 198 107 0 0 0 0 0\n"
"RenderEvent 198 107 0 0 0 0 0\n"
"MouseMoveEvent 199 107 0 0 0 0 0\n"
"RenderEvent 199 107 0 0 0 0 0\n"
"MouseMoveEvent 199 108 0 0 0 0 0\n"
"RenderEvent 199 108 0 0 0 0 0\n"
"MouseMoveEvent 201 108 0 0 0 0 0\n"
"RenderEvent 201 108 0 0 0 0 0\n"
"MouseMoveEvent 201 109 0 0 0 0 0\n"
"RenderEvent 201 109 0 0 0 0 0\n"
"MouseMoveEvent 202 110 0 0 0 0 0\n"
"RenderEvent 202 110 0 0 0 0 0\n"
"MouseMoveEvent 204 110 0 0 0 0 0\n"
"RenderEvent 204 110 0 0 0 0 0\n"
"MouseMoveEvent 205 112 0 0 0 0 0\n"
"RenderEvent 205 112 0 0 0 0 0\n"
"MouseMoveEvent 208 114 0 0 0 0 0\n"
"RenderEvent 208 114 0 0 0 0 0\n"
"MouseMoveEvent 209 114 0 0 0 0 0\n"
"RenderEvent 209 114 0 0 0 0 0\n"
"MouseMoveEvent 211 116 0 0 0 0 0\n"
"RenderEvent 211 116 0 0 0 0 0\n"
"MouseMoveEvent 214 117 0 0 0 0 0\n"
"RenderEvent 214 117 0 0 0 0 0\n"
"MouseMoveEvent 216 119 0 0 0 0 0\n"
"RenderEvent 216 119 0 0 0 0 0\n"
"MouseMoveEvent 219 121 0 0 0 0 0\n"
"RenderEvent 219 121 0 0 0 0 0\n"
"MouseMoveEvent 222 122 0 0 0 0 0\n"
"RenderEvent 222 122 0 0 0 0 0\n"
"MouseMoveEvent 226 124 0 0 0 0 0\n"
"RenderEvent 226 124 0 0 0 0 0\n"
"MouseMoveEvent 229 126 0 0 0 0 0\n"
"RenderEvent 229 126 0 0 0 0 0\n"
"MouseMoveEvent 232 128 0 0 0 0 0\n"
"RenderEvent 232 128 0 0 0 0 0\n"
"MouseMoveEvent 236 130 0 0 0 0 0\n"
"RenderEvent 236 130 0 0 0 0 0\n"
"MouseMoveEvent 240 132 0 0 0 0 0\n"
"RenderEvent 240 132 0 0 0 0 0\n"
"MouseMoveEvent 245 133 0 0 0 0 0\n"
"RenderEvent 245 133 0 0 0 0 0\n"
"MouseMoveEvent 250 135 0 0 0 0 0\n"
"RenderEvent 250 135 0 0 0 0 0\n"
"MouseMoveEvent 255 138 0 0 0 0 0\n"
"RenderEvent 255 138 0 0 0 0 0\n"
"MouseMoveEvent 264 141 0 0 0 0 0\n"
"RenderEvent 264 141 0 0 0 0 0\n"
"MouseMoveEvent 269 142 0 0 0 0 0\n"
"RenderEvent 269 142 0 0 0 0 0\n"
"MouseMoveEvent 275 144 0 0 0 0 0\n"
"RenderEvent 275 144 0 0 0 0 0\n"
"MouseMoveEvent 279 146 0 0 0 0 0\n"
"RenderEvent 279 146 0 0 0 0 0\n"
"MouseMoveEvent 286 149 0 0 0 0 0\n"
"RenderEvent 286 149 0 0 0 0 0\n"
"MouseMoveEvent 288 151 0 0 0 0 0\n"
"RenderEvent 288 151 0 0 0 0 0\n"
"MouseMoveEvent 289 152 0 0 0 0 0\n"
"RenderEvent 289 152 0 0 0 0 0\n"
"MouseMoveEvent 292 155 0 0 0 0 0\n"
"RenderEvent 292 155 0 0 0 0 0\n"
"MouseMoveEvent 295 158 0 0 0 0 0\n"
"RenderEvent 295 158 0 0 0 0 0\n"
"MouseMoveEvent 296 159 0 0 0 0 0\n"
"RenderEvent 296 159 0 0 0 0 0\n"
"MouseMoveEvent 299 162 0 0 0 0 0\n"
"RenderEvent 299 162 0 0 0 0 0\n"
"MouseMoveEvent 302 164 0 0 0 0 0\n"
"RenderEvent 302 164 0 0 0 0 0\n"
"MouseMoveEvent 305 167 0 0 0 0 0\n"
"RenderEvent 305 167 0 0 0 0 0\n"
"MouseMoveEvent 307 168 0 0 0 0 0\n"
"RenderEvent 307 168 0 0 0 0 0\n"
"MouseMoveEvent 310 170 0 0 0 0 0\n"
"RenderEvent 310 170 0 0 0 0 0\n"
"MouseMoveEvent 313 171 0 0 0 0 0\n"
"RenderEvent 313 171 0 0 0 0 0\n"
"MouseMoveEvent 314 173 0 0 0 0 0\n"
"RenderEvent 314 173 0 0 0 0 0\n"
"MouseMoveEvent 317 175 0 0 0 0 0\n"
"RenderEvent 317 175 0 0 0 0 0\n"
"MouseMoveEvent 319 177 0 0 0 0 0\n"
"RenderEvent 319 177 0 0 0 0 0\n"
"MouseMoveEvent 321 179 0 0 0 0 0\n"
"RenderEvent 321 179 0 0 0 0 0\n"
"MouseMoveEvent 323 180 0 0 0 0 0\n"
"RenderEvent 323 180 0 0 0 0 0\n"
"MouseMoveEvent 325 181 0 0 0 0 0\n"
"RenderEvent 325 181 0 0 0 0 0\n"
"MouseMoveEvent 326 182 0 0 0 0 0\n"
"RenderEvent 326 182 0 0 0 0 0\n"
"MouseMoveEvent 330 185 0 0 0 0 0\n"
"RenderEvent 330 185 0 0 0 0 0\n"
"MouseMoveEvent 332 186 0 0 0 0 0\n"
"RenderEvent 332 186 0 0 0 0 0\n"
"MouseMoveEvent 333 187 0 0 0 0 0\n"
"RenderEvent 333 187 0 0 0 0 0\n"
"MouseMoveEvent 336 188 0 0 0 0 0\n"
"RenderEvent 336 188 0 0 0 0 0\n"
"MouseMoveEvent 337 189 0 0 0 0 0\n"
"RenderEvent 337 189 0 0 0 0 0\n"
"MouseMoveEvent 339 190 0 0 0 0 0\n"
"RenderEvent 339 190 0 0 0 0 0\n"
"MouseMoveEvent 340 190 0 0 0 0 0\n"
"RenderEvent 340 190 0 0 0 0 0\n"
"MouseMoveEvent 341 191 0 0 0 0 0\n"
"RenderEvent 341 191 0 0 0 0 0\n"
"MouseMoveEvent 342 191 0 0 0 0 0\n"
"RenderEvent 342 191 0 0 0 0 0\n"
"MouseMoveEvent 343 192 0 0 0 0 0\n"
"RenderEvent 343 192 0 0 0 0 0\n"
"MouseMoveEvent 344 193 0 0 0 0 0\n"
"RenderEvent 344 193 0 0 0 0 0\n"
"MouseMoveEvent 345 193 0 0 0 0 0\n"
"RenderEvent 345 193 0 0 0 0 0\n"
"MouseMoveEvent 346 193 0 0 0 0 0\n"
"RenderEvent 346 193 0 0 0 0 0\n"
"MouseMoveEvent 347 193 0 0 0 0 0\n"
"RenderEvent 347 193 0 0 0 0 0\n"
"MouseMoveEvent 347 194 0 0 0 0 0\n"
"RenderEvent 347 194 0 0 0 0 0\n"
"MouseMoveEvent 348 194 0 0 0 0 0\n"
"RenderEvent 348 194 0 0 0 0 0\n"
"MouseMoveEvent 349 194 0 0 0 0 0\n"
"RenderEvent 349 194 0 0 0 0 0\n"
"MouseMoveEvent 350 194 0 0 0 0 0\n"
"RenderEvent 350 194 0 0 0 0 0\n"
"MouseMoveEvent 350 195 0 0 0 0 0\n"
"RenderEvent 350 195 0 0 0 0 0\n"
"MouseMoveEvent 351 195 0 0 0 0 0\n"
"RenderEvent 351 195 0 0 0 0 0\n"
"MouseMoveEvent 352 195 0 0 0 0 0\n"
"RenderEvent 352 195 0 0 0 0 0\n"
"MouseMoveEvent 353 195 0 0 0 0 0\n"
"RenderEvent 353 195 0 0 0 0 0\n"
"MouseMoveEvent 354 195 0 0 0 0 0\n"
"RenderEvent 354 195 0 0 0 0 0\n"
"MouseMoveEvent 355 195 0 0 0 0 0\n"
"RenderEvent 355 195 0 0 0 0 0\n"
"MouseMoveEvent 356 195 0 0 0 0 0\n"
"RenderEvent 356 195 0 0 0 0 0\n"
"MouseMoveEvent 357 195 0 0 0 0 0\n"
"RenderEvent 357 195 0 0 0 0 0\n"
"MouseMoveEvent 357 194 0 0 0 0 0\n"
"RenderEvent 357 194 0 0 0 0 0\n"
"MouseMoveEvent 359 194 0 0 0 0 0\n"
"RenderEvent 359 194 0 0 0 0 0\n"
"MouseMoveEvent 360 194 0 0 0 0 0\n"
"RenderEvent 360 194 0 0 0 0 0\n"
"MouseMoveEvent 361 194 0 0 0 0 0\n"
"RenderEvent 361 194 0 0 0 0 0\n"
"MouseMoveEvent 362 194 0 0 0 0 0\n"
"RenderEvent 362 194 0 0 0 0 0\n"
"LeftButtonPressEvent 362 194 0 0 0 0 0\n"
"RenderEvent 362 194 0 0 0 0 0\n"
"LeftButtonReleaseEvent 362 194 0 0 0 0 0\n"
"MouseMoveEvent 362 194 0 0 0 0 0\n"
"RenderEvent 362 194 0 0 0 0 0\n"
"MouseMoveEvent 360 194 0 0 0 0 0\n"
"RenderEvent 360 194 0 0 0 0 0\n"
"MouseMoveEvent 357 194 0 0 0 0 0\n"
"RenderEvent 357 194 0 0 0 0 0\n"
"MouseMoveEvent 354 194 0 0 0 0 0\n"
"RenderEvent 354 194 0 0 0 0 0\n"
"MouseMoveEvent 350 194 0 0 0 0 0\n"
"RenderEvent 350 194 0 0 0 0 0\n"
"MouseMoveEvent 344 194 0 0 0 0 0\n"
"RenderEvent 344 194 0 0 0 0 0\n"
"MouseMoveEvent 336 194 0 0 0 0 0\n"
"RenderEvent 336 194 0 0 0 0 0\n"
"MouseMoveEvent 328 194 0 0 0 0 0\n"
"RenderEvent 328 194 0 0 0 0 0\n"
"MouseMoveEvent 320 192 0 0 0 0 0\n"
"RenderEvent 320 192 0 0 0 0 0\n"
"MouseMoveEvent 310 191 0 0 0 0 0\n"
"RenderEvent 310 191 0 0 0 0 0\n"
"MouseMoveEvent 302 188 0 0 0 0 0\n"
"RenderEvent 302 188 0 0 0 0 0\n"
"MouseMoveEvent 291 184 0 0 0 0 0\n"
"RenderEvent 291 184 0 0 0 0 0\n"
"MouseMoveEvent 280 179 0 0 0 0 0\n"
"RenderEvent 280 179 0 0 0 0 0\n"
"MouseMoveEvent 268 175 0 0 0 0 0\n"
"RenderEvent 268 175 0 0 0 0 0\n"
"MouseMoveEvent 258 169 0 0 0 0 0\n"
"RenderEvent 258 169 0 0 0 0 0\n"
"MouseMoveEvent 247 166 0 0 0 0 0\n"
"RenderEvent 247 166 0 0 0 0 0\n"
"MouseMoveEvent 238 162 0 0 0 0 0\n"
"RenderEvent 238 162 0 0 0 0 0\n"
"MouseMoveEvent 231 157 0 0 0 0 0\n"
"RenderEvent 231 157 0 0 0 0 0\n"
"MouseMoveEvent 224 153 0 0 0 0 0\n"
"RenderEvent 224 153 0 0 0 0 0\n"
"MouseMoveEvent 219 150 0 0 0 0 0\n"
"RenderEvent 219 150 0 0 0 0 0\n"
"MouseMoveEvent 214 146 0 0 0 0 0\n"
"RenderEvent 214 146 0 0 0 0 0\n"
"MouseMoveEvent 211 143 0 0 0 0 0\n"
"RenderEvent 211 143 0 0 0 0 0\n"
"MouseMoveEvent 209 141 0 0 0 0 0\n"
"RenderEvent 209 141 0 0 0 0 0\n"
"MouseMoveEvent 209 139 0 0 0 0 0\n"
"RenderEvent 209 139 0 0 0 0 0\n"
"MouseMoveEvent 209 138 0 0 0 0 0\n"
"RenderEvent 209 138 0 0 0 0 0\n"
"MouseMoveEvent 209 136 0 0 0 0 0\n"
"RenderEvent 209 136 0 0 0 0 0\n"
"MouseMoveEvent 209 134 0 0 0 0 0\n"
"RenderEvent 209 134 0 0 0 0 0\n"
"MouseMoveEvent 209 133 0 0 0 0 0\n"
"RenderEvent 209 133 0 0 0 0 0\n"
"MouseMoveEvent 209 132 0 0 0 0 0\n"
"RenderEvent 209 132 0 0 0 0 0\n"
"MouseMoveEvent 209 131 0 0 0 0 0\n"
"RenderEvent 209 131 0 0 0 0 0\n"
"MouseMoveEvent 209 130 0 0 0 0 0\n"
"RenderEvent 209 130 0 0 0 0 0\n"
"MouseMoveEvent 209 129 0 0 0 0 0\n"
"RenderEvent 209 129 0 0 0 0 0\n"
"MouseMoveEvent 210 128 0 0 0 0 0\n"
"RenderEvent 210 128 0 0 0 0 0\n"
"MouseMoveEvent 212 127 0 0 0 0 0\n"
"RenderEvent 212 127 0 0 0 0 0\n"
"MouseMoveEvent 214 126 0 0 0 0 0\n"
"RenderEvent 214 126 0 0 0 0 0\n"
"MouseMoveEvent 215 126 0 0 0 0 0\n"
"RenderEvent 215 126 0 0 0 0 0\n"
"MouseMoveEvent 215 125 0 0 0 0 0\n"
"RenderEvent 215 125 0 0 0 0 0\n"
"MouseMoveEvent 215 124 0 0 0 0 0\n"
"RenderEvent 215 124 0 0 0 0 0\n"
"MouseMoveEvent 214 123 0 0 0 0 0\n"
"RenderEvent 214 123 0 0 0 0 0\n"
"MouseMoveEvent 213 122 0 0 0 0 0\n"
"RenderEvent 213 122 0 0 0 0 0\n"
"MouseMoveEvent 212 121 0 0 0 0 0\n"
"RenderEvent 212 121 0 0 0 0 0\n"
"MouseMoveEvent 211 118 0 0 0 0 0\n"
"RenderEvent 211 118 0 0 0 0 0\n"
"MouseMoveEvent 209 117 0 0 0 0 0\n"
"RenderEvent 209 117 0 0 0 0 0\n"
"MouseMoveEvent 209 116 0 0 0 0 0\n"
"RenderEvent 209 116 0 0 0 0 0\n"
"MouseMoveEvent 209 115 0 0 0 0 0\n"
"RenderEvent 209 115 0 0 0 0 0\n"
"MouseMoveEvent 208 114 0 0 0 0 0\n"
"RenderEvent 208 114 0 0 0 0 0\n"
"MouseMoveEvent 208 113 0 0 0 0 0\n"
"RenderEvent 208 113 0 0 0 0 0\n"
"MouseMoveEvent 207 113 0 0 0 0 0\n"
"RenderEvent 207 113 0 0 0 0 0\n"
"MouseMoveEvent 207 112 0 0 0 0 0\n"
"RenderEvent 207 112 0 0 0 0 0\n"
"MouseMoveEvent 206 112 0 0 0 0 0\n"
"RenderEvent 206 112 0 0 0 0 0\n"
"MouseMoveEvent 205 112 0 0 0 0 0\n"
"RenderEvent 205 112 0 0 0 0 0\n"
"MouseMoveEvent 204 112 0 0 0 0 0\n"
"RenderEvent 204 112 0 0 0 0 0\n"
"MouseMoveEvent 203 112 0 0 0 0 0\n"
"RenderEvent 203 112 0 0 0 0 0\n"
"MouseMoveEvent 203 111 0 0 0 0 0\n"
"RenderEvent 203 111 0 0 0 0 0\n"
"MouseMoveEvent 203 110 0 0 0 0 0\n"
"RenderEvent 203 110 0 0 0 0 0\n"
"MouseMoveEvent 202 110 0 0 0 0 0\n"
"RenderEvent 202 110 0 0 0 0 0\n"
"LeftButtonPressEvent 202 110 0 0 0 0 0\n"
"RenderEvent 202 110 0 0 0 0 0\n"
"MouseMoveEvent 202 111 0 0 0 0 0\n"
"RenderEvent 202 111 0 0 0 0 0\n"
"MouseMoveEvent 202 112 0 0 0 0 0\n"
"RenderEvent 202 112 0 0 0 0 0\n"
"MouseMoveEvent 202 114 0 0 0 0 0\n"
"RenderEvent 202 114 0 0 0 0 0\n"
"MouseMoveEvent 202 116 0 0 0 0 0\n"
"RenderEvent 202 116 0 0 0 0 0\n"
"MouseMoveEvent 202 123 0 0 0 0 0\n"
"RenderEvent 202 123 0 0 0 0 0\n"
"MouseMoveEvent 202 127 0 0 0 0 0\n"
"RenderEvent 202 127 0 0 0 0 0\n"
"MouseMoveEvent 202 132 0 0 0 0 0\n"
"RenderEvent 202 132 0 0 0 0 0\n"
"MouseMoveEvent 202 139 0 0 0 0 0\n"
"RenderEvent 202 139 0 0 0 0 0\n"
"MouseMoveEvent 202 144 0 0 0 0 0\n"
"RenderEvent 202 144 0 0 0 0 0\n"
"MouseMoveEvent 202 152 0 0 0 0 0\n"
"RenderEvent 202 152 0 0 0 0 0\n"
"MouseMoveEvent 202 159 0 0 0 0 0\n"
"RenderEvent 202 159 0 0 0 0 0\n"
"MouseMoveEvent 202 166 0 0 0 0 0\n"
"RenderEvent 202 166 0 0 0 0 0\n"
"MouseMoveEvent 202 174 0 0 0 0 0\n"
"RenderEvent 202 174 0 0 0 0 0\n"
"MouseMoveEvent 202 179 0 0 0 0 0\n"
"RenderEvent 202 179 0 0 0 0 0\n"
"MouseMoveEvent 202 185 0 0 0 0 0\n"
"RenderEvent 202 185 0 0 0 0 0\n"
"MouseMoveEvent 202 189 0 0 0 0 0\n"
"RenderEvent 202 189 0 0 0 0 0\n"
"MouseMoveEvent 202 195 0 0 0 0 0\n"
"RenderEvent 202 195 0 0 0 0 0\n"
"MouseMoveEvent 202 199 0 0 0 0 0\n"
"RenderEvent 202 199 0 0 0 0 0\n"
"MouseMoveEvent 202 203 0 0 0 0 0\n"
"RenderEvent 202 203 0 0 0 0 0\n"
"MouseMoveEvent 202 206 0 0 0 0 0\n"
"RenderEvent 202 206 0 0 0 0 0\n"
"MouseMoveEvent 202 209 0 0 0 0 0\n"
"RenderEvent 202 209 0 0 0 0 0\n"
"MouseMoveEvent 202 211 0 0 0 0 0\n"
"RenderEvent 202 211 0 0 0 0 0\n"
"MouseMoveEvent 202 212 0 0 0 0 0\n"
"RenderEvent 202 212 0 0 0 0 0\n"
"MouseMoveEvent 202 215 0 0 0 0 0\n"
"RenderEvent 202 215 0 0 0 0 0\n"
"MouseMoveEvent 202 216 0 0 0 0 0\n"
"RenderEvent 202 216 0 0 0 0 0\n"
"MouseMoveEvent 203 217 0 0 0 0 0\n"
"RenderEvent 203 217 0 0 0 0 0\n"
"MouseMoveEvent 203 218 0 0 0 0 0\n"
"RenderEvent 203 218 0 0 0 0 0\n"
"MouseMoveEvent 203 219 0 0 0 0 0\n"
"RenderEvent 203 219 0 0 0 0 0\n"
"MouseMoveEvent 203 220 0 0 0 0 0\n"
"RenderEvent 203 220 0 0 0 0 0\n"
"MouseMoveEvent 203 221 0 0 0 0 0\n"
"RenderEvent 203 221 0 0 0 0 0\n"
"MouseMoveEvent 203 222 0 0 0 0 0\n"
"RenderEvent 203 222 0 0 0 0 0\n"
"MouseMoveEvent 203 223 0 0 0 0 0\n"
"RenderEvent 203 223 0 0 0 0 0\n"
"MouseMoveEvent 203 224 0 0 0 0 0\n"
"RenderEvent 203 224 0 0 0 0 0\n"
"MouseMoveEvent 203 226 0 0 0 0 0\n"
"RenderEvent 203 226 0 0 0 0 0\n"
"MouseMoveEvent 203 227 0 0 0 0 0\n"
"RenderEvent 203 227 0 0 0 0 0\n"
"MouseMoveEvent 203 228 0 0 0 0 0\n"
"RenderEvent 203 228 0 0 0 0 0\n"
"MouseMoveEvent 203 229 0 0 0 0 0\n"
"RenderEvent 203 229 0 0 0 0 0\n"
"MouseMoveEvent 203 230 0 0 0 0 0\n"
"RenderEvent 203 230 0 0 0 0 0\n"
"MouseMoveEvent 204 231 0 0 0 0 0\n"
"RenderEvent 204 231 0 0 0 0 0\n"
"MouseMoveEvent 204 233 0 0 0 0 0\n"
"RenderEvent 204 233 0 0 0 0 0\n"
"MouseMoveEvent 204 234 0 0 0 0 0\n"
"RenderEvent 204 234 0 0 0 0 0\n"
"MouseMoveEvent 204 235 0 0 0 0 0\n"
"RenderEvent 204 235 0 0 0 0 0\n"
"LeftButtonReleaseEvent 204 235 0 0 0 0 0\n"
"RenderEvent 204 235 0 0 0 0 0\n"
"MouseMoveEvent 204 235 0 0 0 0 0\n"
"RenderEvent 204 235 0 0 0 0 0\n"
"MouseMoveEvent 206 233 0 0 0 0 0\n"
"RenderEvent 206 233 0 0 0 0 0\n"
"MouseMoveEvent 209 232 0 0 0 0 0\n"
"RenderEvent 209 232 0 0 0 0 0\n"
"MouseMoveEvent 213 227 0 0 0 0 0\n"
"RenderEvent 213 227 0 0 0 0 0\n"
"MouseMoveEvent 220 224 0 0 0 0 0\n"
"RenderEvent 220 224 0 0 0 0 0\n"
"MouseMoveEvent 230 220 0 0 0 0 0\n"
"RenderEvent 230 220 0 0 0 0 0\n"
"MouseMoveEvent 241 213 0 0 0 0 0\n"
"RenderEvent 241 213 0 0 0 0 0\n"
"MouseMoveEvent 273 198 0 0 0 0 0\n"
"RenderEvent 273 198 0 0 0 0 0\n"
"MouseMoveEvent 286 191 0 0 0 0 0\n"
"RenderEvent 286 191 0 0 0 0 0\n"
"MouseMoveEvent 298 185 0 0 0 0 0\n"
"RenderEvent 298 185 0 0 0 0 0\n"
"MouseMoveEvent 311 180 0 0 0 0 0\n"
"RenderEvent 311 180 0 0 0 0 0\n"
"MouseMoveEvent 327 175 0 0 0 0 0\n"
"RenderEvent 327 175 0 0 0 0 0\n"
"MouseMoveEvent 334 173 0 0 0 0 0\n"
"RenderEvent 334 173 0 0 0 0 0\n"
"MouseMoveEvent 337 172 0 0 0 0 0\n"
"RenderEvent 337 172 0 0 0 0 0\n"
"MouseMoveEvent 341 172 0 0 0 0 0\n"
"RenderEvent 341 172 0 0 0 0 0\n"
"MouseMoveEvent 344 172 0 0 0 0 0\n"
"RenderEvent 344 172 0 0 0 0 0\n"
"MouseMoveEvent 345 172 0 0 0 0 0\n"
"RenderEvent 345 172 0 0 0 0 0\n"
"MouseMoveEvent 345 173 0 0 0 0 0\n"
"RenderEvent 345 173 0 0 0 0 0\n"
"MouseMoveEvent 345 174 0 0 0 0 0\n"
"RenderEvent 345 174 0 0 0 0 0\n"
"MouseMoveEvent 346 174 0 0 0 0 0\n"
"RenderEvent 346 174 0 0 0 0 0\n"
"MouseMoveEvent 346 175 0 0 0 0 0\n"
"RenderEvent 346 175 0 0 0 0 0\n"
"MouseMoveEvent 346 176 0 0 0 0 0\n"
"RenderEvent 346 176 0 0 0 0 0\n"
"MouseMoveEvent 346 177 0 0 0 0 0\n"
"RenderEvent 346 177 0 0 0 0 0\n"
"MouseMoveEvent 347 177 0 0 0 0 0\n"
"RenderEvent 347 177 0 0 0 0 0\n"
"MouseMoveEvent 348 178 0 0 0 0 0\n"
"RenderEvent 348 178 0 0 0 0 0\n"
"MouseMoveEvent 349 179 0 0 0 0 0\n"
"RenderEvent 349 179 0 0 0 0 0\n"
"MouseMoveEvent 350 179 0 0 0 0 0\n"
"RenderEvent 350 179 0 0 0 0 0\n"
"MouseMoveEvent 352 179 0 0 0 0 0\n"
"RenderEvent 352 179 0 0 0 0 0\n"
"MouseMoveEvent 353 179 0 0 0 0 0\n"
"RenderEvent 353 179 0 0 0 0 0\n"
"MouseMoveEvent 354 180 0 0 0 0 0\n"
"RenderEvent 354 180 0 0 0 0 0\n"
"MouseMoveEvent 355 181 0 0 0 0 0\n"
"RenderEvent 355 181 0 0 0 0 0\n"
"MouseMoveEvent 356 182 0 0 0 0 0\n"
"RenderEvent 356 182 0 0 0 0 0\n"
"MouseMoveEvent 356 183 0 0 0 0 0\n"
"RenderEvent 356 183 0 0 0 0 0\n"
"MouseMoveEvent 356 184 0 0 0 0 0\n"
"RenderEvent 356 184 0 0 0 0 0\n"
"MouseMoveEvent 356 185 0 0 0 0 0\n"
"RenderEvent 356 185 0 0 0 0 0\n"
"MouseMoveEvent 356 186 0 0 0 0 0\n"
"RenderEvent 356 186 0 0 0 0 0\n"
"MouseMoveEvent 356 187 0 0 0 0 0\n"
"RenderEvent 356 187 0 0 0 0 0\n"
"MouseMoveEvent 357 188 0 0 0 0 0\n"
"RenderEvent 357 188 0 0 0 0 0\n"
"MouseMoveEvent 357 189 0 0 0 0 0\n"
"RenderEvent 357 189 0 0 0 0 0\n"
"MouseMoveEvent 358 189 0 0 0 0 0\n"
"RenderEvent 358 189 0 0 0 0 0\n"
"MouseMoveEvent 359 190 0 0 0 0 0\n"
"RenderEvent 359 190 0 0 0 0 0\n"
"MouseMoveEvent 360 190 0 0 0 0 0\n"
"RenderEvent 360 190 0 0 0 0 0\n"
"MouseMoveEvent 361 190 0 0 0 0 0\n"
"RenderEvent 361 190 0 0 0 0 0\n"
"MouseMoveEvent 362 190 0 0 0 0 0\n"
"RenderEvent 362 190 0 0 0 0 0\n"
"MouseMoveEvent 362 191 0 0 0 0 0\n"
"RenderEvent 362 191 0 0 0 0 0\n"
"MouseMoveEvent 363 191 0 0 0 0 0\n"
"RenderEvent 363 191 0 0 0 0 0\n"
"MouseMoveEvent 363 192 0 0 0 0 0\n"
"RenderEvent 363 192 0 0 0 0 0\n"
"LeftButtonPressEvent 363 192 0 0 0 0 0\n"
"RenderEvent 363 192 0 0 0 0 0\n"
"MouseMoveEvent 363 193 0 0 0 0 0\n"
"RenderEvent 363 193 0 0 0 0 0\n"
"MouseMoveEvent 365 196 0 0 0 0 0\n"
"RenderEvent 365 196 0 0 0 0 0\n"
"MouseMoveEvent 367 198 0 0 0 0 0\n"
"RenderEvent 367 198 0 0 0 0 0\n"
"MouseMoveEvent 372 202 0 0 0 0 0\n"
"RenderEvent 372 202 0 0 0 0 0\n"
"MouseMoveEvent 383 213 0 0 0 0 0\n"
"RenderEvent 383 213 0 0 0 0 0\n"
"MouseMoveEvent 390 219 0 0 0 0 0\n"
"RenderEvent 390 219 0 0 0 0 0\n"
"MouseMoveEvent 397 226 0 0 0 0 0\n"
"RenderEvent 397 226 0 0 0 0 0\n"
"MouseMoveEvent 404 233 0 0 0 0 0\n"
"RenderEvent 404 233 0 0 0 0 0\n"
"MouseMoveEvent 412 239 0 0 0 0 0\n"
"RenderEvent 412 239 0 0 0 0 0\n"
"MouseMoveEvent 419 247 0 0 0 0 0\n"
"RenderEvent 419 247 0 0 0 0 0\n"
"MouseMoveEvent 429 255 0 0 0 0 0\n"
"RenderEvent 429 255 0 0 0 0 0\n"
"MouseMoveEvent 437 261 0 0 0 0 0\n"
"RenderEvent 437 261 0 0 0 0 0\n"
"MouseMoveEvent 445 270 0 0 0 0 0\n"
"RenderEvent 445 270 0 0 0 0 0\n"
"MouseMoveEvent 452 277 0 0 0 0 0\n"
"RenderEvent 452 277 0 0 0 0 0\n"
"MouseMoveEvent 458 284 0 0 0 0 0\n"
"RenderEvent 458 284 0 0 0 0 0\n"
"MouseMoveEvent 465 290 0 0 0 0 0\n"
"RenderEvent 465 290 0 0 0 0 0\n"
"MouseMoveEvent 471 295 0 0 0 0 0\n"
"RenderEvent 471 295 0 0 0 0 0\n"
"MouseMoveEvent 476 299 0 0 0 0 0\n"
"RenderEvent 476 299 0 0 0 0 0\n"
"MouseMoveEvent 482 304 0 0 0 0 0\n"
"RenderEvent 482 304 0 0 0 0 0\n"
"MouseMoveEvent 486 308 0 0 0 0 0\n"
"RenderEvent 486 308 0 0 0 0 0\n"
"MouseMoveEvent 488 310 0 0 0 0 0\n"
"RenderEvent 488 310 0 0 0 0 0\n"
"MouseMoveEvent 490 311 0 0 0 0 0\n"
"RenderEvent 490 311 0 0 0 0 0\n"
"MouseMoveEvent 491 312 0 0 0 0 0\n"
"RenderEvent 491 312 0 0 0 0 0\n"
"MouseMoveEvent 491 313 0 0 0 0 0\n"
"RenderEvent 491 313 0 0 0 0 0\n"
"MouseMoveEvent 491 314 0 0 0 0 0\n"
"RenderEvent 491 314 0 0 0 0 0\n"
"MouseMoveEvent 491 315 0 0 0 0 0\n"
"RenderEvent 491 315 0 0 0 0 0\n"
"LeftButtonReleaseEvent 491 315 0 0 0 0 0\n"
"RenderEvent 491 315 0 0 0 0 0\n"
"MouseMoveEvent 490 315 0 0 0 0 0\n"
"RenderEvent 490 315 0 0 0 0 0\n"
"MouseMoveEvent 487 315 0 0 0 0 0\n"
"RenderEvent 487 315 0 0 0 0 0\n"
"MouseMoveEvent 482 315 0 0 0 0 0\n"
"RenderEvent 482 315 0 0 0 0 0\n"
"MouseMoveEvent 475 314 0 0 0 0 0\n"
"RenderEvent 475 314 0 0 0 0 0\n"
"MouseMoveEvent 466 312 0 0 0 0 0\n"
"RenderEvent 466 312 0 0 0 0 0\n"
"MouseMoveEvent 455 310 0 0 0 0 0\n"
"RenderEvent 455 310 0 0 0 0 0\n"
"MouseMoveEvent 438 309 0 0 0 0 0\n"
"RenderEvent 438 309 0 0 0 0 0\n"
"MouseMoveEvent 415 306 0 0 0 0 0\n"
"RenderEvent 415 306 0 0 0 0 0\n"
"MouseMoveEvent 386 300 0 0 0 0 0\n"
"RenderEvent 386 300 0 0 0 0 0\n"
"MouseMoveEvent 354 295 0 0 0 0 0\n"
"RenderEvent 354 295 0 0 0 0 0\n"
"MouseMoveEvent 322 288 0 0 0 0 0\n"
"RenderEvent 322 288 0 0 0 0 0\n"
"MouseMoveEvent 287 279 0 0 0 0 0\n"
"RenderEvent 287 279 0 0 0 0 0\n"
"MouseMoveEvent 255 269 0 0 0 0 0\n"
"RenderEvent 255 269 0 0 0 0 0\n"
"MouseMoveEvent 230 263 0 0 0 0 0\n"
"RenderEvent 230 263 0 0 0 0 0\n"
"MouseMoveEvent 214 259 0 0 0 0 0\n"
"RenderEvent 214 259 0 0 0 0 0\n"
"MouseMoveEvent 202 256 0 0 0 0 0\n"
"RenderEvent 202 256 0 0 0 0 0\n"
"MouseMoveEvent 194 253 0 0 0 0 0\n"
"RenderEvent 194 253 0 0 0 0 0\n"
"MouseMoveEvent 189 250 0 0 0 0 0\n"
"RenderEvent 189 250 0 0 0 0 0\n"
"MouseMoveEvent 187 248 0 0 0 0 0\n"
"RenderEvent 187 248 0 0 0 0 0\n"
"MouseMoveEvent 185 247 0 0 0 0 0\n"
"RenderEvent 185 247 0 0 0 0 0\n"
"MouseMoveEvent 183 246 0 0 0 0 0\n"
"RenderEvent 183 246 0 0 0 0 0\n"
"MouseMoveEvent 183 245 0 0 0 0 0\n"
"RenderEvent 183 245 0 0 0 0 0\n"
"MouseMoveEvent 183 244 0 0 0 0 0\n"
"RenderEvent 183 244 0 0 0 0 0\n"
"MouseMoveEvent 182 244 0 0 0 0 0\n"
"RenderEvent 182 244 0 0 0 0 0\n"
"MouseMoveEvent 181 244 0 0 0 0 0\n"
"RenderEvent 181 244 0 0 0 0 0\n"
"MouseMoveEvent 180 243 0 0 0 0 0\n"
"RenderEvent 180 243 0 0 0 0 0\n"
"MouseMoveEvent 180 242 0 0 0 0 0\n"
"RenderEvent 180 242 0 0 0 0 0\n"
"MouseMoveEvent 179 241 0 0 0 0 0\n"
"RenderEvent 179 241 0 0 0 0 0\n"
"MouseMoveEvent 178 240 0 0 0 0 0\n"
"RenderEvent 178 240 0 0 0 0 0\n"
"MouseMoveEvent 178 239 0 0 0 0 0\n"
"RenderEvent 178 239 0 0 0 0 0\n"
"MouseMoveEvent 178 238 0 0 0 0 0\n"
"RenderEvent 178 238 0 0 0 0 0\n"
"MouseMoveEvent 178 237 0 0 0 0 0\n"
"RenderEvent 178 237 0 0 0 0 0\n"
"MouseMoveEvent 178 235 0 0 0 0 0\n"
"RenderEvent 178 235 0 0 0 0 0\n"
"MouseMoveEvent 178 233 0 0 0 0 0\n"
"RenderEvent 178 233 0 0 0 0 0\n"
"MouseMoveEvent 179 231 0 0 0 0 0\n"
"RenderEvent 179 231 0 0 0 0 0\n"
"MouseMoveEvent 183 228 0 0 0 0 0\n"
"RenderEvent 183 228 0 0 0 0 0\n"
"MouseMoveEvent 184 227 0 0 0 0 0\n"
"RenderEvent 184 227 0 0 0 0 0\n"
"MouseMoveEvent 185 227 0 0 0 0 0\n"
"RenderEvent 185 227 0 0 0 0 0\n"
"MouseMoveEvent 186 227 0 0 0 0 0\n"
"RenderEvent 186 227 0 0 0 0 0\n"
"MouseMoveEvent 187 227 0 0 0 0 0\n"
"RenderEvent 187 227 0 0 0 0 0\n"
"MouseMoveEvent 188 227 0 0 0 0 0\n"
"RenderEvent 188 227 0 0 0 0 0\n"
"MouseMoveEvent 189 228 0 0 0 0 0\n"
"RenderEvent 189 228 0 0 0 0 0\n"
"MouseMoveEvent 190 229 0 0 0 0 0\n"
"RenderEvent 190 229 0 0 0 0 0\n"
"MouseMoveEvent 192 229 0 0 0 0 0\n"
"RenderEvent 192 229 0 0 0 0 0\n"
"MouseMoveEvent 193 230 0 0 0 0 0\n"
"RenderEvent 193 230 0 0 0 0 0\n"
"MouseMoveEvent 195 231 0 0 0 0 0\n"
"RenderEvent 195 231 0 0 0 0 0\n"
"MouseMoveEvent 196 231 0 0 0 0 0\n"
"RenderEvent 196 231 0 0 0 0 0\n"
"MouseMoveEvent 198 231 0 0 0 0 0\n"
"RenderEvent 198 231 0 0 0 0 0\n"
"MouseMoveEvent 198 232 0 0 0 0 0\n"
"RenderEvent 198 232 0 0 0 0 0\n"
"MouseMoveEvent 199 232 0 0 0 0 0\n"
"RenderEvent 199 232 0 0 0 0 0\n"
"MouseMoveEvent 200 232 0 0 0 0 0\n"
"RenderEvent 200 232 0 0 0 0 0\n"
"MouseMoveEvent 201 233 0 0 0 0 0\n"
"RenderEvent 201 233 0 0 0 0 0\n"
"MouseMoveEvent 202 234 0 0 0 0 0\n"
"RenderEvent 202 234 0 0 0 0 0\n"
"MouseMoveEvent 203 235 0 0 0 0 0\n"
"RenderEvent 203 235 0 0 0 0 0\n"
"MouseMoveEvent 203 236 0 0 0 0 0\n"
"RenderEvent 203 236 0 0 0 0 0\n"
"MouseMoveEvent 204 236 0 0 0 0 0\n"
"RenderEvent 204 236 0 0 0 0 0\n"
"MouseMoveEvent 205 236 0 0 0 0 0\n"
"RenderEvent 205 236 0 0 0 0 0\n"
"MouseMoveEvent 206 236 0 0 0 0 0\n"
"RenderEvent 206 236 0 0 0 0 0\n"
"MouseMoveEvent 207 237 0 0 0 0 0\n"
"RenderEvent 207 237 0 0 0 0 0\n"
"MouseMoveEvent 208 237 0 0 0 0 0\n"
"RenderEvent 208 237 0 0 0 0 0\n"
"MouseMoveEvent 210 238 0 0 0 0 0\n"
"RenderEvent 210 238 0 0 0 0 0\n"
"MouseMoveEvent 212 239 0 0 0 0 0\n"
"RenderEvent 212 239 0 0 0 0 0\n"
"MouseMoveEvent 213 239 0 0 0 0 0\n"
"RenderEvent 213 239 0 0 0 0 0\n"
"MouseMoveEvent 213 240 0 0 0 0 0\n"
"RenderEvent 213 240 0 0 0 0 0\n"
"MouseMoveEvent 213 241 0 0 0 0 0\n"
"RenderEvent 213 241 0 0 0 0 0\n"
"LeftButtonPressEvent 213 241 0 0 0 0 0\n"
"RenderEvent 213 241 0 0 0 0 0\n"
"MouseMoveEvent 215 241 0 0 0 0 0\n"
"RenderEvent 215 241 0 0 0 0 0\n"
"MouseMoveEvent 218 239 0 0 0 0 0\n"
"RenderEvent 218 239 0 0 0 0 0\n"
"MouseMoveEvent 224 236 0 0 0 0 0\n"
"RenderEvent 224 236 0 0 0 0 0\n"
"MouseMoveEvent 231 232 0 0 0 0 0\n"
"RenderEvent 231 232 0 0 0 0 0\n"
"MouseMoveEvent 239 227 0 0 0 0 0\n"
"RenderEvent 239 227 0 0 0 0 0\n"
"MouseMoveEvent 248 223 0 0 0 0 0\n"
"RenderEvent 248 223 0 0 0 0 0\n"
"MouseMoveEvent 258 217 0 0 0 0 0\n"
"RenderEvent 258 217 0 0 0 0 0\n"
"MouseMoveEvent 278 206 0 0 0 0 0\n"
"RenderEvent 278 206 0 0 0 0 0\n"
"MouseMoveEvent 288 201 0 0 0 0 0\n"
"RenderEvent 288 201 0 0 0 0 0\n"
"MouseMoveEvent 296 196 0 0 0 0 0\n"
"RenderEvent 296 196 0 0 0 0 0\n"
"MouseMoveEvent 306 193 0 0 0 0 0\n"
"RenderEvent 306 193 0 0 0 0 0\n"
"MouseMoveEvent 314 190 0 0 0 0 0\n"
"RenderEvent 314 190 0 0 0 0 0\n"
"MouseMoveEvent 321 188 0 0 0 0 0\n"
"RenderEvent 321 188 0 0 0 0 0\n"
"MouseMoveEvent 327 185 0 0 0 0 0\n"
"RenderEvent 327 185 0 0 0 0 0\n"
"MouseMoveEvent 333 183 0 0 0 0 0\n"
"RenderEvent 333 183 0 0 0 0 0\n"
"MouseMoveEvent 338 181 0 0 0 0 0\n"
"RenderEvent 338 181 0 0 0 0 0\n"
"MouseMoveEvent 342 181 0 0 0 0 0\n"
"RenderEvent 342 181 0 0 0 0 0\n"
"MouseMoveEvent 345 179 0 0 0 0 0\n"
"RenderEvent 345 179 0 0 0 0 0\n"
"MouseMoveEvent 348 178 0 0 0 0 0\n"
"RenderEvent 348 178 0 0 0 0 0\n"
"MouseMoveEvent 350 178 0 0 0 0 0\n"
"RenderEvent 350 178 0 0 0 0 0\n"
"MouseMoveEvent 353 178 0 0 0 0 0\n"
"RenderEvent 353 178 0 0 0 0 0\n"
"MouseMoveEvent 356 177 0 0 0 0 0\n"
"RenderEvent 356 177 0 0 0 0 0\n"
"MouseMoveEvent 359 176 0 0 0 0 0\n"
"RenderEvent 359 176 0 0 0 0 0\n"
"MouseMoveEvent 363 175 0 0 0 0 0\n"
"RenderEvent 363 175 0 0 0 0 0\n"
"MouseMoveEvent 366 175 0 0 0 0 0\n"
"RenderEvent 366 175 0 0 0 0 0\n"
"MouseMoveEvent 369 174 0 0 0 0 0\n"
"RenderEvent 369 174 0 0 0 0 0\n"
"MouseMoveEvent 372 173 0 0 0 0 0\n"
"RenderEvent 372 173 0 0 0 0 0\n"
"MouseMoveEvent 377 173 0 0 0 0 0\n"
"RenderEvent 377 173 0 0 0 0 0\n"
"MouseMoveEvent 379 172 0 0 0 0 0\n"
"RenderEvent 379 172 0 0 0 0 0\n"
"MouseMoveEvent 382 171 0 0 0 0 0\n"
"RenderEvent 382 171 0 0 0 0 0\n"
"MouseMoveEvent 383 171 0 0 0 0 0\n"
"RenderEvent 383 171 0 0 0 0 0\n"
"MouseMoveEvent 386 171 0 0 0 0 0\n"
"RenderEvent 386 171 0 0 0 0 0\n"
"MouseMoveEvent 388 170 0 0 0 0 0\n"
"RenderEvent 388 170 0 0 0 0 0\n"
"MouseMoveEvent 391 169 0 0 0 0 0\n"
"RenderEvent 391 169 0 0 0 0 0\n"
"MouseMoveEvent 394 169 0 0 0 0 0\n"
"RenderEvent 394 169 0 0 0 0 0\n"
"MouseMoveEvent 396 167 0 0 0 0 0\n"
"RenderEvent 396 167 0 0 0 0 0\n"
"MouseMoveEvent 399 167 0 0 0 0 0\n"
"RenderEvent 399 167 0 0 0 0 0\n"
"MouseMoveEvent 400 166 0 0 0 0 0\n"
"RenderEvent 400 166 0 0 0 0 0\n"
"MouseMoveEvent 402 165 0 0 0 0 0\n"
"RenderEvent 402 165 0 0 0 0 0\n"
"MouseMoveEvent 403 165 0 0 0 0 0\n"
"RenderEvent 403 165 0 0 0 0 0\n"
"MouseMoveEvent 405 165 0 0 0 0 0\n"
"RenderEvent 405 165 0 0 0 0 0\n"
"MouseMoveEvent 408 165 0 0 0 0 0\n"
"RenderEvent 408 165 0 0 0 0 0\n"
"MouseMoveEvent 411 165 0 0 0 0 0\n"
"RenderEvent 411 165 0 0 0 0 0\n"
"MouseMoveEvent 413 165 0 0 0 0 0\n"
"RenderEvent 413 165 0 0 0 0 0\n"
"MouseMoveEvent 420 164 0 0 0 0 0\n"
"RenderEvent 420 164 0 0 0 0 0\n"
"MouseMoveEvent 422 163 0 0 0 0 0\n"
"RenderEvent 422 163 0 0 0 0 0\n"
"MouseMoveEvent 427 163 0 0 0 0 0\n"
"RenderEvent 427 163 0 0 0 0 0\n"
"MouseMoveEvent 431 162 0 0 0 0 0\n"
"RenderEvent 431 162 0 0 0 0 0\n"
"MouseMoveEvent 433 162 0 0 0 0 0\n"
"RenderEvent 433 162 0 0 0 0 0\n"
"MouseMoveEvent 436 162 0 0 0 0 0\n"
"RenderEvent 436 162 0 0 0 0 0\n"
"MouseMoveEvent 439 162 0 0 0 0 0\n"
"RenderEvent 439 162 0 0 0 0 0\n"
"MouseMoveEvent 442 162 0 0 0 0 0\n"
"RenderEvent 442 162 0 0 0 0 0\n"
"MouseMoveEvent 445 162 0 0 0 0 0\n"
"RenderEvent 445 162 0 0 0 0 0\n"
"MouseMoveEvent 447 162 0 0 0 0 0\n"
"RenderEvent 447 162 0 0 0 0 0\n"
"MouseMoveEvent 448 162 0 0 0 0 0\n"
"RenderEvent 448 162 0 0 0 0 0\n"
"MouseMoveEvent 449 162 0 0 0 0 0\n"
"RenderEvent 449 162 0 0 0 0 0\n"
"MouseMoveEvent 451 162 0 0 0 0 0\n"
"RenderEvent 451 162 0 0 0 0 0\n"
"MouseMoveEvent 453 162 0 0 0 0 0\n"
"RenderEvent 453 162 0 0 0 0 0\n"
"MouseMoveEvent 454 162 0 0 0 0 0\n"
"RenderEvent 454 162 0 0 0 0 0\n"
"MouseMoveEvent 456 162 0 0 0 0 0\n"
"RenderEvent 456 162 0 0 0 0 0\n"
"MouseMoveEvent 457 162 0 0 0 0 0\n"
"RenderEvent 457 162 0 0 0 0 0\n"
"MouseMoveEvent 458 162 0 0 0 0 0\n"
"RenderEvent 458 162 0 0 0 0 0\n"
"MouseMoveEvent 460 162 0 0 0 0 0\n"
"RenderEvent 460 162 0 0 0 0 0\n"
"MouseMoveEvent 462 162 0 0 0 0 0\n"
"RenderEvent 462 162 0 0 0 0 0\n"
"MouseMoveEvent 464 162 0 0 0 0 0\n"
"RenderEvent 464 162 0 0 0 0 0\n"
"MouseMoveEvent 467 160 0 0 0 0 0\n"
"RenderEvent 467 160 0 0 0 0 0\n"
"MouseMoveEvent 471 159 0 0 0 0 0\n"
"RenderEvent 471 159 0 0 0 0 0\n"
"MouseMoveEvent 475 159 0 0 0 0 0\n"
"RenderEvent 475 159 0 0 0 0 0\n"
"MouseMoveEvent 476 159 0 0 0 0 0\n"
"RenderEvent 476 159 0 0 0 0 0\n"
"MouseMoveEvent 477 159 0 0 0 0 0\n"
"RenderEvent 477 159 0 0 0 0 0\n"
"MouseMoveEvent 478 159 0 0 0 0 0\n"
"RenderEvent 478 159 0 0 0 0 0\n"
"LeftButtonReleaseEvent 478 159 0 0 0 0 0\n"
"RenderEvent 478 159 0 0 0 0 0\n"
"MouseMoveEvent 478 159 0 0 0 0 0\n"
"RenderEvent 478 159 0 0 0 0 0\n"
"MouseMoveEvent 476 159 0 0 0 0 0\n"
"RenderEvent 476 159 0 0 0 0 0\n"
"MouseMoveEvent 475 158 0 0 0 0 0\n"
"RenderEvent 475 158 0 0 0 0 0\n"
"MouseMoveEvent 474 158 0 0 0 0 0\n"
"RenderEvent 474 158 0 0 0 0 0\n"
"MouseMoveEvent 474 157 0 0 0 0 0\n"
"RenderEvent 474 157 0 0 0 0 0\n"
"MouseMoveEvent 473 157 0 0 0 0 0\n"
"RenderEvent 473 157 0 0 0 0 0\n"
"MouseMoveEvent 472 157 0 0 0 0 0\n"
"RenderEvent 472 157 0 0 0 0 0\n"
"MouseMoveEvent 470 157 0 0 0 0 0\n"
"RenderEvent 470 157 0 0 0 0 0\n"
"MouseMoveEvent 469 157 0 0 0 0 0\n"
"RenderEvent 469 157 0 0 0 0 0\n"
"MouseMoveEvent 468 157 0 0 0 0 0\n"
"RenderEvent 468 157 0 0 0 0 0\n"
"MouseMoveEvent 467 157 0 0 0 0 0\n"
"RenderEvent 467 157 0 0 0 0 0\n"
"MouseMoveEvent 466 157 0 0 0 0 0\n"
"RenderEvent 466 157 0 0 0 0 0\n"
"MouseMoveEvent 464 157 0 0 0 0 0\n"
"RenderEvent 464 157 0 0 0 0 0\n"
"MouseMoveEvent 463 157 0 0 0 0 0\n"
"RenderEvent 463 157 0 0 0 0 0\n"
"MouseMoveEvent 462 157 0 0 0 0 0\n"
"RenderEvent 462 157 0 0 0 0 0\n"
"MouseMoveEvent 461 157 0 0 0 0 0\n"
"RenderEvent 461 157 0 0 0 0 0\n"
"MouseMoveEvent 460 157 0 0 0 0 0\n"
"RenderEvent 460 157 0 0 0 0 0\n"
"MouseMoveEvent 459 157 0 0 0 0 0\n"
"RenderEvent 459 157 0 0 0 0 0\n"
"MouseMoveEvent 458 157 0 0 0 0 0\n"
"RenderEvent 458 157 0 0 0 0 0\n"
"MouseMoveEvent 455 157 0 0 0 0 0\n"
"RenderEvent 455 157 0 0 0 0 0\n"
"MouseMoveEvent 453 157 0 0 0 0 0\n"
"RenderEvent 453 157 0 0 0 0 0\n"
"MouseMoveEvent 451 156 0 0 0 0 0\n"
"RenderEvent 451 156 0 0 0 0 0\n"
"MouseMoveEvent 449 155 0 0 0 0 0\n"
"RenderEvent 449 155 0 0 0 0 0\n"
"MouseMoveEvent 447 155 0 0 0 0 0\n"
"RenderEvent 447 155 0 0 0 0 0\n"
"MouseMoveEvent 444 155 0 0 0 0 0\n"
"RenderEvent 444 155 0 0 0 0 0\n"
"MouseMoveEvent 442 155 0 0 0 0 0\n"
"RenderEvent 442 155 0 0 0 0 0\n"
"MouseMoveEvent 439 155 0 0 0 0 0\n"
"RenderEvent 439 155 0 0 0 0 0\n"
"MouseMoveEvent 434 155 0 0 0 0 0\n"
"RenderEvent 434 155 0 0 0 0 0\n"
"MouseMoveEvent 430 155 0 0 0 0 0\n"
"RenderEvent 430 155 0 0 0 0 0\n"
"MouseMoveEvent 426 155 0 0 0 0 0\n"
"RenderEvent 426 155 0 0 0 0 0\n"
"MouseMoveEvent 422 155 0 0 0 0 0\n"
"RenderEvent 422 155 0 0 0 0 0\n"
"MouseMoveEvent 418 155 0 0 0 0 0\n"
"RenderEvent 418 155 0 0 0 0 0\n"
"MouseMoveEvent 415 155 0 0 0 0 0\n"
"RenderEvent 415 155 0 0 0 0 0\n"
"MouseMoveEvent 412 155 0 0 0 0 0\n"
"RenderEvent 412 155 0 0 0 0 0\n"
"MouseMoveEvent 409 155 0 0 0 0 0\n"
"RenderEvent 409 155 0 0 0 0 0\n"
"MouseMoveEvent 403 155 0 0 0 0 0\n"
"RenderEvent 403 155 0 0 0 0 0\n"
"MouseMoveEvent 399 155 0 0 0 0 0\n"
"RenderEvent 399 155 0 0 0 0 0\n"
"MouseMoveEvent 395 155 0 0 0 0 0\n"
"RenderEvent 395 155 0 0 0 0 0\n"
"MouseMoveEvent 391 155 0 0 0 0 0\n"
"RenderEvent 391 155 0 0 0 0 0\n"
"MouseMoveEvent 382 155 0 0 0 0 0\n"
"RenderEvent 382 155 0 0 0 0 0\n"
"MouseMoveEvent 377 155 0 0 0 0 0\n"
"RenderEvent 377 155 0 0 0 0 0\n"
"MouseMoveEvent 374 155 0 0 0 0 0\n"
"RenderEvent 374 155 0 0 0 0 0\n"
"MouseMoveEvent 371 155 0 0 0 0 0\n"
"RenderEvent 371 155 0 0 0 0 0\n"
"MouseMoveEvent 369 155 0 0 0 0 0\n"
"RenderEvent 369 155 0 0 0 0 0\n"
"KeyPressEvent 369 155 0 0 113 1 q\n"
"CharEvent 369 155 0 0 113 1 q\n"
"ExitEvent 369 155 0 0 113 1 q\n"
;

//---------------------------------------------------------------------------
// Create cells of various types
void CreateHexahedronActor(vtkActor* actor);
void CreatePentagonalPrismActor(vtkActor* actor);
void CreatePyramidActor(vtkActor* actor);
void CreateTetraActor(vtkActor* actor);
void CreateVoxelActor(vtkActor* actor);
void CreateWedgeActor(vtkActor* actor);

//---------------------------------------------------------------------------
int TestCellCentersPointPlacer(int argc, char *argv[])
{
  std::vector<vtkSmartPointer<vtkActor> > actors;

  vtkSmartPointer<vtkActor> hexahedronActor =
    vtkSmartPointer<vtkActor>::New();
  CreateHexahedronActor(hexahedronActor);
  actors.push_back(hexahedronActor);

  vtkSmartPointer<vtkActor> pentagonalPrismActor =
    vtkSmartPointer<vtkActor>::New();
  CreatePentagonalPrismActor(pentagonalPrismActor);
  actors.push_back(pentagonalPrismActor);

  vtkSmartPointer<vtkActor> pyramidActor =
    vtkSmartPointer<vtkActor>::New();
  CreatePyramidActor(pyramidActor);
  actors.push_back(pyramidActor);

  vtkSmartPointer<vtkActor> tetraActor =
    vtkSmartPointer<vtkActor>::New();
  CreateTetraActor(tetraActor);
  actors.push_back(tetraActor);

  vtkSmartPointer<vtkActor> voxelActor =
    vtkSmartPointer<vtkActor>::New();
  CreateVoxelActor(voxelActor);
  actors.push_back(voxelActor);

  vtkSmartPointer<vtkActor> wedgeActor =
    vtkSmartPointer<vtkActor>::New();
  CreateWedgeActor(wedgeActor);
  actors.push_back(wedgeActor);

  vtkSmartPointer<vtkRenderer> renderer =
    vtkSmartPointer< vtkRenderer >::New();

  int gridDimensions = 3;
  int rendererSize = 200;

  // Create a render window, renderer and render window interactor.
  // Add the cells to the renderer, in a grid layout. We accomplish
  // this by using a transform filter to translate and arrange on
  // a grid.

  vtkSmartPointer<vtkRenderWindow> renderWindow =
    vtkSmartPointer<vtkRenderWindow>::New();
  renderWindow->SetSize(
    rendererSize*gridDimensions, rendererSize*gridDimensions);
  renderWindow->AddRenderer(renderer);
  renderer->SetBackground(.2, .3, .4);

  // Create a point placer to constrain to the cell centers and add
  // each of the actors to the placer, so that it includes them in
  // its constraints.
  vtkSmartPointer<vtkCellCentersPointPlacer>  pointPlacer =
    vtkSmartPointer<vtkCellCentersPointPlacer>::New();

  for(int row = 0; row < gridDimensions; row++)
    {
    for(int col = 0; col < gridDimensions; col++)
      {
      int index = row * gridDimensions + col;

      if(index > static_cast< int >(actors.size() - 1))
        {
        continue;
        }

      vtkSmartPointer< vtkTransformFilter > transformFilter =
        vtkSmartPointer< vtkTransformFilter >::New();
      vtkSmartPointer< vtkMatrix4x4 > matrix =
        vtkSmartPointer< vtkMatrix4x4 >::New();
      matrix->SetElement(0,3,5*col);
      matrix->SetElement(1,3,5*row);
      vtkSmartPointer< vtkMatrixToLinearTransform > mlt =
        vtkSmartPointer< vtkMatrixToLinearTransform >::New();
      mlt->SetInput(matrix);
      transformFilter->SetInputConnection(
        actors[index]->GetMapper()->GetInputConnection(0, 0));
      transformFilter->SetTransform(mlt);
      transformFilter->Update();

      vtkSmartPointer< vtkDataSetMapper > mapper2 =
        vtkSmartPointer< vtkDataSetMapper >::New();
      mapper2->SetInputConnection(transformFilter->GetOutputPort());
      actors[index]->SetMapper(mapper2);

      renderer->AddActor(actors[index]);
      pointPlacer->AddProp(actors[index]);
      }
    }

  // Default colors
  actors[0]->GetProperty()->SetColor(1,0,0.5);
  actors[1]->GetProperty()->SetColor(0,1,0);
  actors[2]->GetProperty()->SetColor(0,0,1);
  actors[3]->GetProperty()->SetColor(1,1,0);
  actors[4]->GetProperty()->SetColor(1,0,1);
  actors[5]->GetProperty()->SetColor(0,1,1);

  renderer->ResetCamera();
  renderer->GetActiveCamera()->Azimuth(30);
  renderer->GetActiveCamera()->Elevation(-30);
  renderer->ResetCameraClippingRange();

  vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  renderWindowInteractor->SetRenderWindow(renderWindow);

  renderWindow->Render();

  // Now add a distance widget.

  vtkSmartPointer<vtkDistanceWidget> widget =
    vtkSmartPointer<vtkDistanceWidget>::New();
  widget->CreateDefaultRepresentation();
  vtkSmartPointer< vtkDistanceRepresentation2D > rep =
    vtkSmartPointer< vtkDistanceRepresentation2D >::New();
  rep->GetAxis()->GetProperty()->SetColor( 1.0, 0.0, 0.0 );

  // Create a 3D handle reprensentation template for this distance
  // widget
  vtkSmartPointer< vtkPointHandleRepresentation3D > handleRep3D =
    vtkSmartPointer< vtkPointHandleRepresentation3D >::New();
  handleRep3D->GetProperty()->SetLineWidth(4.0);
  rep->SetHandleRepresentation( handleRep3D );
  handleRep3D->GetProperty()->SetColor( 0.8, 0.2, 0 );
  widget->SetRepresentation(rep);

  // Instantiate the handles and have them be constrained by the
  // placer.
  rep->InstantiateHandleRepresentation();
  rep->GetPoint1Representation()->SetPointPlacer(pointPlacer);
  rep->GetPoint2Representation()->SetPointPlacer(pointPlacer);

  // With a "snap" constraint, we can't have a smooth motion anymore, so
  // turn it off.
  static_cast< vtkPointHandleRepresentation3D * >(
    rep->GetPoint1Representation())->SmoothMotionOff();
  static_cast< vtkPointHandleRepresentation3D * >(
    rep->GetPoint2Representation())->SmoothMotionOff();

  widget->SetInteractor(renderWindowInteractor);
  widget->SetEnabled(1);

  renderWindow->Render();

  return vtkTesting::InteractorEventLoop(
      argc, argv, renderWindowInteractor, TestCellCentersPointPlacerEventLog );
}

//---------------------------------------------------------------------------
void CreateHexahedronActor(vtkActor* actor)
{
  // Setup the coordinates of eight points
  // (the two faces must be in counter clockwise order as viewd from the outside)

  // Create the points
  vtkSmartPointer<vtkPoints> points =
    vtkSmartPointer<vtkPoints>::New();
  points->InsertNextPoint(0.0, 0.0, 0.0);
  points->InsertNextPoint(1.0, 0.0, 0.0);
  points->InsertNextPoint(1.0, 1.0, 0.0);
  points->InsertNextPoint(0.0, 1.0, 0.0);
  points->InsertNextPoint(0.0, 0.0, 1.0);
  points->InsertNextPoint(1.0, 0.0, 1.0);
  points->InsertNextPoint(1.0, 1.0, 1.0);
  points->InsertNextPoint(0.0, 1.0, 1.0);

  // Create a hexahedron from the points
  vtkSmartPointer<vtkHexahedron> hex =
    vtkSmartPointer<vtkHexahedron>::New();
  hex->GetPointIds()->SetId(0,0);
  hex->GetPointIds()->SetId(1,1);
  hex->GetPointIds()->SetId(2,2);
  hex->GetPointIds()->SetId(3,3);
  hex->GetPointIds()->SetId(4,4);
  hex->GetPointIds()->SetId(5,5);
  hex->GetPointIds()->SetId(6,6);
  hex->GetPointIds()->SetId(7,7);

  // Add the hexahedron to a cell array
  vtkSmartPointer<vtkCellArray> hexs =
    vtkSmartPointer<vtkCellArray>::New();
  hexs->InsertNextCell(hex);

  // Add the points and hexahedron to an unstructured grid
  vtkSmartPointer<vtkUnstructuredGrid> uGrid =
    vtkSmartPointer<vtkUnstructuredGrid>::New();
  uGrid->SetPoints(points);
  uGrid->InsertNextCell(hex->GetCellType(), hex->GetPointIds());

  // Visualize
  vtkSmartPointer<vtkDataSetMapper> mapper =
    vtkSmartPointer<vtkDataSetMapper>::New();
  mapper->SetInputData(uGrid);

  actor->SetMapper(mapper);
}

//---------------------------------------------------------------------------
void CreatePentagonalPrismActor(vtkActor* actor)
{
  // Create the points
  vtkSmartPointer<vtkPoints> points =
    vtkSmartPointer<vtkPoints>::New();
  points->InsertNextPoint(1, 0, 0);
  points->InsertNextPoint(3, 0, 0);
  points->InsertNextPoint(4, 2, 0);
  points->InsertNextPoint(2, 4, 0);
  points->InsertNextPoint(0, 2, 0);
  points->InsertNextPoint(1, 0, 4);
  points->InsertNextPoint(3, 0, 4);
  points->InsertNextPoint(4, 2, 4);
  points->InsertNextPoint(2, 4, 4);
  points->InsertNextPoint(0, 2, 4);

  // Pentagonal Prism
  vtkSmartPointer<vtkPentagonalPrism> pentagonalPrism =
    vtkSmartPointer<vtkPentagonalPrism>::New();
  pentagonalPrism->GetPointIds()->SetId(0,0);
  pentagonalPrism->GetPointIds()->SetId(1,1);
  pentagonalPrism->GetPointIds()->SetId(2,2);
  pentagonalPrism->GetPointIds()->SetId(3,3);
  pentagonalPrism->GetPointIds()->SetId(4,4);
  pentagonalPrism->GetPointIds()->SetId(5,5);
  pentagonalPrism->GetPointIds()->SetId(6,6);
  pentagonalPrism->GetPointIds()->SetId(7,7);
  pentagonalPrism->GetPointIds()->SetId(8,8);
  pentagonalPrism->GetPointIds()->SetId(9,9);

  vtkSmartPointer<vtkCellArray> cellArray =
    vtkSmartPointer<vtkCellArray>::New();
  cellArray->InsertNextCell(pentagonalPrism);

  // Add the points and hexahedron to an unstructured grid
  vtkSmartPointer<vtkUnstructuredGrid> uGrid =
    vtkSmartPointer<vtkUnstructuredGrid>::New();
  uGrid->SetPoints(points);
  uGrid->InsertNextCell(pentagonalPrism->GetCellType(), pentagonalPrism->GetPointIds());

  // Visualize
  vtkSmartPointer<vtkDataSetMapper> mapper =
    vtkSmartPointer<vtkDataSetMapper>::New();
  mapper->SetInputData(uGrid);

  actor->SetMapper(mapper);

}

//---------------------------------------------------------------------------
void CreatePyramidActor(vtkActor* actor)
{
  vtkSmartPointer<vtkPoints> points =
      vtkSmartPointer<vtkPoints>::New();

  float p0[3] = {1.0, 1.0, 1.0};
  float p1[3] = {-1.0, 1.0, 1.0};
  float p2[3] = {-1.0, -1.0, 1.0};
  float p3[3] = {1.0, -1.0, 1.0};
  float p4[3] = {0.0, 0.0, 0.0};

  points->InsertNextPoint(p0);
  points->InsertNextPoint(p1);
  points->InsertNextPoint(p2);
  points->InsertNextPoint(p3);
  points->InsertNextPoint(p4);

  vtkSmartPointer<vtkPyramid> pyramid =
      vtkSmartPointer<vtkPyramid>::New();
  pyramid->GetPointIds()->SetId(0,0);
  pyramid->GetPointIds()->SetId(1,1);
  pyramid->GetPointIds()->SetId(2,2);
  pyramid->GetPointIds()->SetId(3,3);
  pyramid->GetPointIds()->SetId(4,4);

  vtkSmartPointer<vtkCellArray> cells =
      vtkSmartPointer<vtkCellArray>::New();
  cells->InsertNextCell (pyramid);

  vtkSmartPointer<vtkUnstructuredGrid> ug =
      vtkSmartPointer<vtkUnstructuredGrid>::New();
  ug->SetPoints(points);
  ug->InsertNextCell(pyramid->GetCellType(),pyramid->GetPointIds());

  //Create an actor and mapper
  vtkSmartPointer<vtkDataSetMapper> mapper =
      vtkSmartPointer<vtkDataSetMapper>::New();
  mapper->SetInputData(ug);

  actor->SetMapper(mapper);
}

//---------------------------------------------------------------------------
void CreateTetraActor(vtkActor* actor)
{
  vtkSmartPointer< vtkPoints > points =
    vtkSmartPointer< vtkPoints > :: New();
  points->InsertNextPoint(0, 0, 0);
  points->InsertNextPoint(1, 0, 0);
  points->InsertNextPoint(1, 1, 0);
  points->InsertNextPoint(0, 1, 1);
  points->InsertNextPoint(5, 5, 5);
  points->InsertNextPoint(6, 5, 5);
  points->InsertNextPoint(6, 6, 5);
  points->InsertNextPoint(5, 6, 6);

  vtkSmartPointer<vtkUnstructuredGrid> unstructuredGrid =
    vtkSmartPointer<vtkUnstructuredGrid>::New();
  unstructuredGrid->SetPoints(points);

  vtkSmartPointer<vtkTetra> tetra =
    vtkSmartPointer<vtkTetra>::New();
  tetra->GetPointIds()->SetId(0, 0);
  tetra->GetPointIds()->SetId(1, 1);
  tetra->GetPointIds()->SetId(2, 2);
  tetra->GetPointIds()->SetId(3, 3);

  vtkSmartPointer<vtkCellArray> cellArray =
    vtkSmartPointer<vtkCellArray>::New();
  cellArray->InsertNextCell(tetra);
  unstructuredGrid->SetCells(VTK_TETRA, cellArray);

  // Create a mapper and actor
  vtkSmartPointer<vtkDataSetMapper> mapper =
    vtkSmartPointer<vtkDataSetMapper>::New();
  mapper->SetInputData(unstructuredGrid);

  actor->SetMapper(mapper);
}

//---------------------------------------------------------------------------
void CreateVoxelActor(vtkActor* actor)
{
  vtkSmartPointer<vtkPoints> points =
    vtkSmartPointer<vtkPoints>::New();
  points->InsertNextPoint(0, 0, 0);
  points->InsertNextPoint(1, 0, 0);
  points->InsertNextPoint(0, 1, 0);
  points->InsertNextPoint(1, 1, 0);
  points->InsertNextPoint(0, 0, 1);
  points->InsertNextPoint(1, 0, 1);
  points->InsertNextPoint(0, 1, 1);
  points->InsertNextPoint(1, 1, 1);

  vtkSmartPointer<vtkVoxel> voxel =
    vtkSmartPointer<vtkVoxel>::New();
  voxel->GetPointIds()->SetId(0, 0);
  voxel->GetPointIds()->SetId(1, 1);
  voxel->GetPointIds()->SetId(2, 2);
  voxel->GetPointIds()->SetId(3, 3);
  voxel->GetPointIds()->SetId(4, 4);
  voxel->GetPointIds()->SetId(5, 5);
  voxel->GetPointIds()->SetId(6, 6);
  voxel->GetPointIds()->SetId(7, 7);

  vtkSmartPointer<vtkCellArray> cells =
    vtkSmartPointer<vtkCellArray>::New();
  cells->InsertNextCell(voxel);

  vtkSmartPointer<vtkUnstructuredGrid> ug =
    vtkSmartPointer<vtkUnstructuredGrid>::New();
  ug->SetPoints(points);
  ug->InsertNextCell(voxel->GetCellType(),voxel->GetPointIds());

  // Visualize
  vtkSmartPointer<vtkDataSetMapper> mapper =
    vtkSmartPointer<vtkDataSetMapper>::New();
  mapper->SetInputData(ug);

  actor->SetMapper(mapper);
}

//---------------------------------------------------------------------------
void CreateWedgeActor(vtkActor* actor)
{
  vtkSmartPointer<vtkPoints> points =
    vtkSmartPointer<vtkPoints>::New();

  points->InsertNextPoint(0, 1, 0);
  points->InsertNextPoint(0, 0, 0);
  points->InsertNextPoint(0, .5, .5);
  points->InsertNextPoint(1, 1, 0);
  points->InsertNextPoint(1, 0.0, 0.0);
  points->InsertNextPoint(1, .5, .5);

  vtkSmartPointer<vtkWedge> wedge =
    vtkSmartPointer<vtkWedge>::New();
  wedge->GetPointIds()->SetId(0,0);
  wedge->GetPointIds()->SetId(1,1);
  wedge->GetPointIds()->SetId(2,2);
  wedge->GetPointIds()->SetId(3,3);
  wedge->GetPointIds()->SetId(4,4);
  wedge->GetPointIds()->SetId(5,5);

  vtkSmartPointer<vtkCellArray> cells =
    vtkSmartPointer<vtkCellArray>::New();
  cells->InsertNextCell(wedge);

  vtkSmartPointer<vtkUnstructuredGrid> ug =
    vtkSmartPointer<vtkUnstructuredGrid>::New();
  ug->SetPoints(points);
  ug->InsertNextCell(wedge->GetCellType(),wedge->GetPointIds());

  // Visualize
  vtkSmartPointer<vtkDataSetMapper> mapper =
    vtkSmartPointer<vtkDataSetMapper>::New();
  mapper->SetInputData(ug);

  actor->SetMapper(mapper);
}
