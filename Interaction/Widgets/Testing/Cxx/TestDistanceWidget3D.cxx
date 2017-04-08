/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDistanceWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the vtkDistanceWidget

// First include the required header files for the VTK classes we are using.
#include "vtkDistanceWidget.h"
#include "vtkDistanceRepresentation3D.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkCommand.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkRegressionTestImage.h"
#include "vtkDebugLeaks.h"
#include "vtkCoordinate.h"
#include "vtkMath.h"
#include "vtkHandleWidget.h"
#include "vtkPointHandleRepresentation3D.h"
#include "vtkProperty.h"
#include "vtkCamera.h"
#include "vtkFollower.h"

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

const char TestDistanceWidget3DEventLog[] =
"# StreamVersion 1\n"
"RenderEvent 0 0 0 0 0 0 0\n"
"EnterEvent 292 123 0 0 0 0 0\n"
"MouseMoveEvent 280 131 0 0 0 0 0\n"
"MouseMoveEvent 268 137 0 0 0 0 0\n"
"MouseMoveEvent 258 143 0 0 0 0 0\n"
"MouseMoveEvent 250 147 0 0 0 0 0\n"
"MouseMoveEvent 246 153 0 0 0 0 0\n"
"MouseMoveEvent 245 155 0 0 0 0 0\n"
"MouseMoveEvent 244 157 0 0 0 0 0\n"
"MouseMoveEvent 240 161 0 0 0 0 0\n"
"MouseMoveEvent 239 163 0 0 0 0 0\n"
"MouseMoveEvent 235 167 0 0 0 0 0\n"
"MouseMoveEvent 233 173 0 0 0 0 0\n"
"MouseMoveEvent 229 177 0 0 0 0 0\n"
"MouseMoveEvent 223 183 0 0 0 0 0\n"
"MouseMoveEvent 222 184 0 0 0 0 0\n"
"MouseMoveEvent 221 186 0 0 0 0 0\n"
"MouseMoveEvent 220 187 0 0 0 0 0\n"
"MouseMoveEvent 219 188 0 0 0 0 0\n"
"MouseMoveEvent 219 189 0 0 0 0 0\n"
"LeftButtonPressEvent 219 189 0 0 0 0 0\n"
"RenderEvent 219 189 0 0 0 0 0\n"
"LeftButtonReleaseEvent 219 189 0 0 0 0 0\n"
"MouseMoveEvent 218 189 0 0 0 0 0\n"
"RenderEvent 218 189 0 0 0 0 0\n"
"MouseMoveEvent 218 188 0 0 0 0 0\n"
"RenderEvent 218 188 0 0 0 0 0\n"
"MouseMoveEvent 216 187 0 0 0 0 0\n"
"RenderEvent 216 187 0 0 0 0 0\n"
"MouseMoveEvent 212 183 0 0 0 0 0\n"
"RenderEvent 212 183 0 0 0 0 0\n"
"MouseMoveEvent 206 179 0 0 0 0 0\n"
"RenderEvent 206 179 0 0 0 0 0\n"
"MouseMoveEvent 202 175 0 0 0 0 0\n"
"RenderEvent 202 175 0 0 0 0 0\n"
"MouseMoveEvent 194 169 0 0 0 0 0\n"
"RenderEvent 194 169 0 0 0 0 0\n"
"MouseMoveEvent 188 165 0 0 0 0 0\n"
"RenderEvent 188 165 0 0 0 0 0\n"
"MouseMoveEvent 182 159 0 0 0 0 0\n"
"RenderEvent 182 159 0 0 0 0 0\n"
"MouseMoveEvent 174 153 0 0 0 0 0\n"
"RenderEvent 174 153 0 0 0 0 0\n"
"MouseMoveEvent 166 149 0 0 0 0 0\n"
"RenderEvent 166 149 0 0 0 0 0\n"
"MouseMoveEvent 162 143 0 0 0 0 0\n"
"RenderEvent 162 143 0 0 0 0 0\n"
"MouseMoveEvent 156 137 0 0 0 0 0\n"
"RenderEvent 156 137 0 0 0 0 0\n"
"MouseMoveEvent 148 135 0 0 0 0 0\n"
"RenderEvent 148 135 0 0 0 0 0\n"
"MouseMoveEvent 144 129 0 0 0 0 0\n"
"RenderEvent 144 129 0 0 0 0 0\n"
"MouseMoveEvent 140 125 0 0 0 0 0\n"
"RenderEvent 140 125 0 0 0 0 0\n"
"MouseMoveEvent 136 119 0 0 0 0 0\n"
"RenderEvent 136 119 0 0 0 0 0\n"
"MouseMoveEvent 134 118 0 0 0 0 0\n"
"RenderEvent 134 118 0 0 0 0 0\n"
"MouseMoveEvent 130 114 0 0 0 0 0\n"
"RenderEvent 130 114 0 0 0 0 0\n"
"MouseMoveEvent 128 113 0 0 0 0 0\n"
"RenderEvent 128 113 0 0 0 0 0\n"
"MouseMoveEvent 124 109 0 0 0 0 0\n"
"RenderEvent 124 109 0 0 0 0 0\n"
"MouseMoveEvent 118 105 0 0 0 0 0\n"
"RenderEvent 118 105 0 0 0 0 0\n"
"MouseMoveEvent 116 104 0 0 0 0 0\n"
"RenderEvent 116 104 0 0 0 0 0\n"
"MouseMoveEvent 112 100 0 0 0 0 0\n"
"RenderEvent 112 100 0 0 0 0 0\n"
"MouseMoveEvent 110 99 0 0 0 0 0\n"
"RenderEvent 110 99 0 0 0 0 0\n"
"MouseMoveEvent 106 95 0 0 0 0 0\n"
"RenderEvent 106 95 0 0 0 0 0\n"
"MouseMoveEvent 105 94 0 0 0 0 0\n"
"RenderEvent 105 94 0 0 0 0 0\n"
"MouseMoveEvent 104 93 0 0 0 0 0\n"
"RenderEvent 104 93 0 0 0 0 0\n"
"MouseMoveEvent 102 92 0 0 0 0 0\n"
"RenderEvent 102 92 0 0 0 0 0\n"
"MouseMoveEvent 101 91 0 0 0 0 0\n"
"RenderEvent 101 91 0 0 0 0 0\n"
"MouseMoveEvent 99 90 0 0 0 0 0\n"
"RenderEvent 99 90 0 0 0 0 0\n"
"MouseMoveEvent 97 89 0 0 0 0 0\n"
"RenderEvent 97 89 0 0 0 0 0\n"
"MouseMoveEvent 96 87 0 0 0 0 0\n"
"RenderEvent 96 87 0 0 0 0 0\n"
"MouseMoveEvent 94 86 0 0 0 0 0\n"
"RenderEvent 94 86 0 0 0 0 0\n"
"MouseMoveEvent 93 85 0 0 0 0 0\n"
"RenderEvent 93 85 0 0 0 0 0\n"
"MouseMoveEvent 92 85 0 0 0 0 0\n"
"RenderEvent 92 85 0 0 0 0 0\n"
"MouseMoveEvent 92 84 0 0 0 0 0\n"
"RenderEvent 92 84 0 0 0 0 0\n"
"MouseMoveEvent 90 84 0 0 0 0 0\n"
"RenderEvent 90 84 0 0 0 0 0\n"
"MouseMoveEvent 90 83 0 0 0 0 0\n"
"RenderEvent 90 83 0 0 0 0 0\n"
"MouseMoveEvent 89 83 0 0 0 0 0\n"
"RenderEvent 89 83 0 0 0 0 0\n"
"MouseMoveEvent 89 82 0 0 0 0 0\n"
"RenderEvent 89 82 0 0 0 0 0\n"
"LeftButtonPressEvent 89 82 0 0 0 0 0\n"
"RenderEvent 89 82 0 0 0 0 0\n"
"LeftButtonReleaseEvent 89 82 0 0 0 0 0\n"
"MouseMoveEvent 89 81 0 0 0 0 0\n"
"RenderEvent 89 81 0 0 0 0 0\n"
"MouseMoveEvent 89 80 0 0 0 0 0\n"
"RenderEvent 89 80 0 0 0 0 0\n"
"MouseMoveEvent 88 80 0 0 0 0 0\n"
"RenderEvent 88 80 0 0 0 0 0\n"
"MouseMoveEvent 88 79 0 0 0 0 0\n"
"RenderEvent 88 79 0 0 0 0 0\n"
"MouseMoveEvent 87 78 0 0 0 0 0\n"
"RenderEvent 87 78 0 0 0 0 0\n"
"MouseMoveEvent 87 77 0 0 0 0 0\n"
"RenderEvent 87 77 0 0 0 0 0\n"
"MouseMoveEvent 86 76 0 0 0 0 0\n"
"RenderEvent 86 76 0 0 0 0 0\n"
"MouseMoveEvent 86 74 0 0 0 0 0\n"
"RenderEvent 86 74 0 0 0 0 0\n"
"MouseMoveEvent 85 72 0 0 0 0 0\n"
"RenderEvent 85 72 0 0 0 0 0\n"
"MouseMoveEvent 85 71 0 0 0 0 0\n"
"RenderEvent 85 71 0 0 0 0 0\n"
"MouseMoveEvent 81 67 0 0 0 0 0\n"
"RenderEvent 81 67 0 0 0 0 0\n"
"MouseMoveEvent 81 64 0 0 0 0 0\n"
"RenderEvent 81 64 0 0 0 0 0\n"
"MouseMoveEvent 80 62 0 0 0 0 0\n"
"RenderEvent 80 62 0 0 0 0 0\n"
"MouseMoveEvent 79 60 0 0 0 0 0\n"
"RenderEvent 79 60 0 0 0 0 0\n"
"MouseMoveEvent 79 58 0 0 0 0 0\n"
"RenderEvent 79 58 0 0 0 0 0\n"
"MouseMoveEvent 78 56 0 0 0 0 0\n"
"RenderEvent 78 56 0 0 0 0 0\n"
"MouseMoveEvent 77 55 0 0 0 0 0\n"
"RenderEvent 77 55 0 0 0 0 0\n"
"MouseMoveEvent 77 54 0 0 0 0 0\n"
"RenderEvent 77 54 0 0 0 0 0\n"
"MouseMoveEvent 77 53 0 0 0 0 0\n"
"RenderEvent 77 53 0 0 0 0 0\n"
"MouseMoveEvent 78 53 0 0 0 0 0\n"
"RenderEvent 78 53 0 0 0 0 0\n"
"MouseMoveEvent 78 52 0 0 0 0 0\n"
"RenderEvent 78 52 0 0 0 0 0\n"
"MouseMoveEvent 79 52 0 0 0 0 0\n"
"RenderEvent 79 52 0 0 0 0 0\n"
"MouseMoveEvent 80 52 0 0 0 0 0\n"
"RenderEvent 80 52 0 0 0 0 0\n"
"MouseMoveEvent 80 53 0 0 0 0 0\n"
"RenderEvent 80 53 0 0 0 0 0\n"
"MouseMoveEvent 81 53 0 0 0 0 0\n"
"RenderEvent 81 53 0 0 0 0 0\n"
"MouseMoveEvent 81 54 0 0 0 0 0\n"
"RenderEvent 81 54 0 0 0 0 0\n"
"MouseMoveEvent 83 55 0 0 0 0 0\n"
"RenderEvent 83 55 0 0 0 0 0\n"
"MouseMoveEvent 84 56 0 0 0 0 0\n"
"RenderEvent 84 56 0 0 0 0 0\n"
"MouseMoveEvent 90 58 0 0 0 0 0\n"
"RenderEvent 90 58 0 0 0 0 0\n"
"MouseMoveEvent 92 59 0 0 0 0 0\n"
"RenderEvent 92 59 0 0 0 0 0\n"
"MouseMoveEvent 94 60 0 0 0 0 0\n"
"RenderEvent 94 60 0 0 0 0 0\n"
"MouseMoveEvent 102 64 0 0 0 0 0\n"
"RenderEvent 102 64 0 0 0 0 0\n"
"MouseMoveEvent 106 68 0 0 0 0 0\n"
"RenderEvent 106 68 0 0 0 0 0\n"
"MouseMoveEvent 114 74 0 0 0 0 0\n"
"RenderEvent 114 74 0 0 0 0 0\n"
"MouseMoveEvent 122 80 0 0 0 0 0\n"
"RenderEvent 122 80 0 0 0 0 0\n"
"MouseMoveEvent 130 86 0 0 0 0 0\n"
"RenderEvent 130 86 0 0 0 0 0\n"
"MouseMoveEvent 140 94 0 0 0 0 0\n"
"RenderEvent 140 94 0 0 0 0 0\n"
"MouseMoveEvent 150 102 0 0 0 0 0\n"
"RenderEvent 150 102 0 0 0 0 0\n"
"MouseMoveEvent 156 106 0 0 0 0 0\n"
"RenderEvent 156 106 0 0 0 0 0\n"
"MouseMoveEvent 166 112 0 0 0 0 0\n"
"RenderEvent 166 112 0 0 0 0 0\n"
"MouseMoveEvent 174 120 0 0 0 0 0\n"
"RenderEvent 174 120 0 0 0 0 0\n"
"MouseMoveEvent 182 128 0 0 0 0 0\n"
"RenderEvent 182 128 0 0 0 0 0\n"
"MouseMoveEvent 188 132 0 0 0 0 0\n"
"RenderEvent 188 132 0 0 0 0 0\n"
"MouseMoveEvent 194 138 0 0 0 0 0\n"
"RenderEvent 194 138 0 0 0 0 0\n"
"MouseMoveEvent 198 144 0 0 0 0 0\n"
"RenderEvent 198 144 0 0 0 0 0\n"
"MouseMoveEvent 199 146 0 0 0 0 0\n"
"RenderEvent 199 146 0 0 0 0 0\n"
"MouseMoveEvent 203 152 0 0 0 0 0\n"
"RenderEvent 203 152 0 0 0 0 0\n"
"MouseMoveEvent 204 154 0 0 0 0 0\n"
"RenderEvent 204 154 0 0 0 0 0\n"
"MouseMoveEvent 205 156 0 0 0 0 0\n"
"RenderEvent 205 156 0 0 0 0 0\n"
"MouseMoveEvent 206 157 0 0 0 0 0\n"
"RenderEvent 206 157 0 0 0 0 0\n"
"MouseMoveEvent 207 158 0 0 0 0 0\n"
"RenderEvent 207 158 0 0 0 0 0\n"
"MouseMoveEvent 207 159 0 0 0 0 0\n"
"RenderEvent 207 159 0 0 0 0 0\n"
"MouseMoveEvent 207 161 0 0 0 0 0\n"
"RenderEvent 207 161 0 0 0 0 0\n"
"MouseMoveEvent 208 162 0 0 0 0 0\n"
"RenderEvent 208 162 0 0 0 0 0\n"
"MouseMoveEvent 209 163 0 0 0 0 0\n"
"RenderEvent 209 163 0 0 0 0 0\n"
"MouseMoveEvent 210 164 0 0 0 0 0\n"
"RenderEvent 210 164 0 0 0 0 0\n"
"MouseMoveEvent 211 165 0 0 0 0 0\n"
"RenderEvent 211 165 0 0 0 0 0\n"
"MouseMoveEvent 211 166 0 0 0 0 0\n"
"RenderEvent 211 166 0 0 0 0 0\n"
"MouseMoveEvent 212 166 0 0 0 0 0\n"
"RenderEvent 212 166 0 0 0 0 0\n"
"MouseMoveEvent 212 167 0 0 0 0 0\n"
"RenderEvent 212 167 0 0 0 0 0\n"
"MouseMoveEvent 213 167 0 0 0 0 0\n"
"RenderEvent 213 167 0 0 0 0 0\n"
"MouseMoveEvent 213 168 0 0 0 0 0\n"
"RenderEvent 213 168 0 0 0 0 0\n"
"MouseMoveEvent 214 169 0 0 0 0 0\n"
"RenderEvent 214 169 0 0 0 0 0\n"
"MouseMoveEvent 215 170 0 0 0 0 0\n"
"RenderEvent 215 170 0 0 0 0 0\n"
"MouseMoveEvent 215 171 0 0 0 0 0\n"
"RenderEvent 215 171 0 0 0 0 0\n"
"MouseMoveEvent 216 172 0 0 0 0 0\n"
"RenderEvent 216 172 0 0 0 0 0\n"
"MouseMoveEvent 216 173 0 0 0 0 0\n"
"RenderEvent 216 173 0 0 0 0 0\n"
"MouseMoveEvent 217 174 0 0 0 0 0\n"
"RenderEvent 217 174 0 0 0 0 0\n"
"MouseMoveEvent 217 175 0 0 0 0 0\n"
"RenderEvent 217 175 0 0 0 0 0\n"
"MouseMoveEvent 217 176 0 0 0 0 0\n"
"RenderEvent 217 176 0 0 0 0 0\n"
"MouseMoveEvent 217 177 0 0 0 0 0\n"
"RenderEvent 217 177 0 0 0 0 0\n"
"MouseMoveEvent 217 178 0 0 0 0 0\n"
"RenderEvent 217 178 0 0 0 0 0\n"
"MouseMoveEvent 217 179 0 0 0 0 0\n"
"RenderEvent 217 179 0 0 0 0 0\n"
"MouseMoveEvent 217 180 0 0 0 0 0\n"
"RenderEvent 217 180 0 0 0 0 0\n"
"MouseMoveEvent 217 181 0 0 0 0 0\n"
"RenderEvent 217 181 0 0 0 0 0\n"
"MouseMoveEvent 217 182 0 0 0 0 0\n"
"RenderEvent 217 182 0 0 0 0 0\n"
"MouseMoveEvent 217 183 0 0 0 0 0\n"
"RenderEvent 217 183 0 0 0 0 0\n"
"MouseMoveEvent 217 184 0 0 0 0 0\n"
"RenderEvent 217 184 0 0 0 0 0\n"
"MouseMoveEvent 217 185 0 0 0 0 0\n"
"RenderEvent 217 185 0 0 0 0 0\n"
"MouseMoveEvent 217 186 0 0 0 0 0\n"
"RenderEvent 217 186 0 0 0 0 0\n"
"MouseMoveEvent 217 187 0 0 0 0 0\n"
"RenderEvent 217 187 0 0 0 0 0\n"
"MouseMoveEvent 216 188 0 0 0 0 0\n"
"RenderEvent 216 188 0 0 0 0 0\n"
"MouseMoveEvent 216 189 0 0 0 0 0\n"
"RenderEvent 216 189 0 0 0 0 0\n"
"LeftButtonPressEvent 216 189 0 0 0 0 0\n"
"RenderEvent 216 189 0 0 0 0 0\n"
"MouseMoveEvent 215 190 0 0 0 0 0\n"
"RenderEvent 215 190 0 0 0 0 0\n"
"MouseMoveEvent 213 191 0 0 0 0 0\n"
"RenderEvent 213 191 0 0 0 0 0\n"
"MouseMoveEvent 207 193 0 0 0 0 0\n"
"RenderEvent 207 193 0 0 0 0 0\n"
"MouseMoveEvent 199 197 0 0 0 0 0\n"
"RenderEvent 199 197 0 0 0 0 0\n"
"MouseMoveEvent 191 199 0 0 0 0 0\n"
"RenderEvent 191 199 0 0 0 0 0\n"
"MouseMoveEvent 183 203 0 0 0 0 0\n"
"RenderEvent 183 203 0 0 0 0 0\n"
"MouseMoveEvent 173 207 0 0 0 0 0\n"
"RenderEvent 173 207 0 0 0 0 0\n"
"MouseMoveEvent 165 211 0 0 0 0 0\n"
"RenderEvent 165 211 0 0 0 0 0\n"
"MouseMoveEvent 155 215 0 0 0 0 0\n"
"RenderEvent 155 215 0 0 0 0 0\n"
"MouseMoveEvent 147 219 0 0 0 0 0\n"
"RenderEvent 147 219 0 0 0 0 0\n"
"MouseMoveEvent 139 221 0 0 0 0 0\n"
"RenderEvent 139 221 0 0 0 0 0\n"
"MouseMoveEvent 131 223 0 0 0 0 0\n"
"RenderEvent 131 223 0 0 0 0 0\n"
"MouseMoveEvent 125 227 0 0 0 0 0\n"
"RenderEvent 125 227 0 0 0 0 0\n"
"MouseMoveEvent 119 229 0 0 0 0 0\n"
"RenderEvent 119 229 0 0 0 0 0\n"
"MouseMoveEvent 113 231 0 0 0 0 0\n"
"RenderEvent 113 231 0 0 0 0 0\n"
"MouseMoveEvent 107 233 0 0 0 0 0\n"
"RenderEvent 107 233 0 0 0 0 0\n"
"MouseMoveEvent 101 235 0 0 0 0 0\n"
"RenderEvent 101 235 0 0 0 0 0\n"
"MouseMoveEvent 99 236 0 0 0 0 0\n"
"RenderEvent 99 236 0 0 0 0 0\n"
"MouseMoveEvent 97 237 0 0 0 0 0\n"
"RenderEvent 97 237 0 0 0 0 0\n"
"MouseMoveEvent 95 237 0 0 0 0 0\n"
"RenderEvent 95 237 0 0 0 0 0\n"
"MouseMoveEvent 94 238 0 0 0 0 0\n"
"RenderEvent 94 238 0 0 0 0 0\n"
"MouseMoveEvent 92 239 0 0 0 0 0\n"
"RenderEvent 92 239 0 0 0 0 0\n"
"MouseMoveEvent 90 239 0 0 0 0 0\n"
"RenderEvent 90 239 0 0 0 0 0\n"
"MouseMoveEvent 88 239 0 0 0 0 0\n"
"RenderEvent 88 239 0 0 0 0 0\n"
"MouseMoveEvent 87 240 0 0 0 0 0\n"
"RenderEvent 87 240 0 0 0 0 0\n"
"MouseMoveEvent 86 240 0 0 0 0 0\n"
"RenderEvent 86 240 0 0 0 0 0\n"
"MouseMoveEvent 85 240 0 0 0 0 0\n"
"RenderEvent 85 240 0 0 0 0 0\n"
"MouseMoveEvent 84 240 0 0 0 0 0\n"
"RenderEvent 84 240 0 0 0 0 0\n"
"MouseMoveEvent 83 240 0 0 0 0 0\n"
"RenderEvent 83 240 0 0 0 0 0\n"
"MouseMoveEvent 82 240 0 0 0 0 0\n"
"RenderEvent 82 240 0 0 0 0 0\n"
"MouseMoveEvent 81 240 0 0 0 0 0\n"
"RenderEvent 81 240 0 0 0 0 0\n"
"MouseMoveEvent 80 241 0 0 0 0 0\n"
"RenderEvent 80 241 0 0 0 0 0\n"
"MouseMoveEvent 79 241 0 0 0 0 0\n"
"RenderEvent 79 241 0 0 0 0 0\n"
"MouseMoveEvent 77 241 0 0 0 0 0\n"
"RenderEvent 77 241 0 0 0 0 0\n"
"MouseMoveEvent 75 241 0 0 0 0 0\n"
"RenderEvent 75 241 0 0 0 0 0\n"
"MouseMoveEvent 74 241 0 0 0 0 0\n"
"RenderEvent 74 241 0 0 0 0 0\n"
"MouseMoveEvent 74 242 0 0 0 0 0\n"
"RenderEvent 74 242 0 0 0 0 0\n"
"MouseMoveEvent 73 242 0 0 0 0 0\n"
"RenderEvent 73 242 0 0 0 0 0\n"
"MouseMoveEvent 72 242 0 0 0 0 0\n"
"RenderEvent 72 242 0 0 0 0 0\n"
"MouseMoveEvent 71 242 0 0 0 0 0\n"
"RenderEvent 71 242 0 0 0 0 0\n"
"MouseMoveEvent 70 242 0 0 0 0 0\n"
"RenderEvent 70 242 0 0 0 0 0\n"
"MouseMoveEvent 69 242 0 0 0 0 0\n"
"RenderEvent 69 242 0 0 0 0 0\n"
"MouseMoveEvent 68 242 0 0 0 0 0\n"
"RenderEvent 68 242 0 0 0 0 0\n"
"MouseMoveEvent 67 242 0 0 0 0 0\n"
"RenderEvent 67 242 0 0 0 0 0\n"
"MouseMoveEvent 66 242 0 0 0 0 0\n"
"RenderEvent 66 242 0 0 0 0 0\n"
"MouseMoveEvent 65 242 0 0 0 0 0\n"
"RenderEvent 65 242 0 0 0 0 0\n"
"MouseMoveEvent 64 242 0 0 0 0 0\n"
"RenderEvent 64 242 0 0 0 0 0\n"
"MouseMoveEvent 64 241 0 0 0 0 0\n"
"RenderEvent 64 241 0 0 0 0 0\n"
"MouseMoveEvent 63 241 0 0 0 0 0\n"
"RenderEvent 63 241 0 0 0 0 0\n"
"MouseMoveEvent 63 240 0 0 0 0 0\n"
"RenderEvent 63 240 0 0 0 0 0\n"
"MouseMoveEvent 62 240 0 0 0 0 0\n"
"RenderEvent 62 240 0 0 0 0 0\n"
"MouseMoveEvent 62 239 0 0 0 0 0\n"
"RenderEvent 62 239 0 0 0 0 0\n"
"MouseMoveEvent 61 239 0 0 0 0 0\n"
"RenderEvent 61 239 0 0 0 0 0\n"
"MouseMoveEvent 60 239 0 0 0 0 0\n"
"RenderEvent 60 239 0 0 0 0 0\n"
"MouseMoveEvent 60 238 0 0 0 0 0\n"
"RenderEvent 60 238 0 0 0 0 0\n"
"MouseMoveEvent 60 237 0 0 0 0 0\n"
"RenderEvent 60 237 0 0 0 0 0\n"
"MouseMoveEvent 60 236 0 0 0 0 0\n"
"RenderEvent 60 236 0 0 0 0 0\n"
"MouseMoveEvent 60 235 0 0 0 0 0\n"
"RenderEvent 60 235 0 0 0 0 0\n"
"MouseMoveEvent 60 234 0 0 0 0 0\n"
"RenderEvent 60 234 0 0 0 0 0\n"
"MouseMoveEvent 60 233 0 0 0 0 0\n"
"RenderEvent 60 233 0 0 0 0 0\n"
"MouseMoveEvent 60 232 0 0 0 0 0\n"
"RenderEvent 60 232 0 0 0 0 0\n"
"MouseMoveEvent 60 231 0 0 0 0 0\n"
"RenderEvent 60 231 0 0 0 0 0\n"
"MouseMoveEvent 60 230 0 0 0 0 0\n"
"RenderEvent 60 230 0 0 0 0 0\n"
"MouseMoveEvent 60 229 0 0 0 0 0\n"
"RenderEvent 60 229 0 0 0 0 0\n"
"MouseMoveEvent 60 228 0 0 0 0 0\n"
"RenderEvent 60 228 0 0 0 0 0\n"
"MouseMoveEvent 60 227 0 0 0 0 0\n"
"RenderEvent 60 227 0 0 0 0 0\n"
"MouseMoveEvent 60 226 0 0 0 0 0\n"
"RenderEvent 60 226 0 0 0 0 0\n"
"MouseMoveEvent 60 225 0 0 0 0 0\n"
"RenderEvent 60 225 0 0 0 0 0\n"
"MouseMoveEvent 60 224 0 0 0 0 0\n"
"RenderEvent 60 224 0 0 0 0 0\n"
"MouseMoveEvent 60 223 0 0 0 0 0\n"
"RenderEvent 60 223 0 0 0 0 0\n"
"MouseMoveEvent 61 222 0 0 0 0 0\n"
"RenderEvent 61 222 0 0 0 0 0\n"
"MouseMoveEvent 61 221 0 0 0 0 0\n"
"RenderEvent 61 221 0 0 0 0 0\n"
"MouseMoveEvent 61 220 0 0 0 0 0\n"
"RenderEvent 61 220 0 0 0 0 0\n"
"MouseMoveEvent 61 219 0 0 0 0 0\n"
"RenderEvent 61 219 0 0 0 0 0\n"
"MouseMoveEvent 62 219 0 0 0 0 0\n"
"RenderEvent 62 219 0 0 0 0 0\n"
"MouseMoveEvent 62 218 0 0 0 0 0\n"
"RenderEvent 62 218 0 0 0 0 0\n"
"MouseMoveEvent 62 217 0 0 0 0 0\n"
"RenderEvent 62 217 0 0 0 0 0\n"
"LeftButtonReleaseEvent 62 217 0 0 0 0 0\n"
"RenderEvent 62 217 0 0 0 0 0\n"
"MouseMoveEvent 62 216 0 0 0 0 0\n"
"RenderEvent 62 216 0 0 0 0 0\n"
"MouseMoveEvent 62 215 0 0 0 0 0\n"
"RenderEvent 62 215 0 0 0 0 0\n"
"MouseMoveEvent 62 214 0 0 0 0 0\n"
"RenderEvent 62 214 0 0 0 0 0\n"
"MouseMoveEvent 63 214 0 0 0 0 0\n"
"RenderEvent 63 214 0 0 0 0 0\n"
"MouseMoveEvent 63 213 0 0 0 0 0\n"
"RenderEvent 63 213 0 0 0 0 0\n"
"MouseMoveEvent 64 211 0 0 0 0 0\n"
"RenderEvent 64 211 0 0 0 0 0\n"
"MouseMoveEvent 64 209 0 0 0 0 0\n"
"RenderEvent 64 209 0 0 0 0 0\n"
"MouseMoveEvent 65 207 0 0 0 0 0\n"
"RenderEvent 65 207 0 0 0 0 0\n"
"MouseMoveEvent 67 199 0 0 0 0 0\n"
"RenderEvent 67 199 0 0 0 0 0\n"
"MouseMoveEvent 67 191 0 0 0 0 0\n"
"RenderEvent 67 191 0 0 0 0 0\n"
"MouseMoveEvent 69 183 0 0 0 0 0\n"
"RenderEvent 69 183 0 0 0 0 0\n"
"MouseMoveEvent 71 175 0 0 0 0 0\n"
"RenderEvent 71 175 0 0 0 0 0\n"
"MouseMoveEvent 73 165 0 0 0 0 0\n"
"RenderEvent 73 165 0 0 0 0 0\n"
"MouseMoveEvent 73 155 0 0 0 0 0\n"
"RenderEvent 73 155 0 0 0 0 0\n"
"MouseMoveEvent 75 145 0 0 0 0 0\n"
"RenderEvent 75 145 0 0 0 0 0\n"
"MouseMoveEvent 77 133 0 0 0 0 0\n"
"RenderEvent 77 133 0 0 0 0 0\n"
"MouseMoveEvent 79 123 0 0 0 0 0\n"
"RenderEvent 79 123 0 0 0 0 0\n"
"MouseMoveEvent 81 113 0 0 0 0 0\n"
"RenderEvent 81 113 0 0 0 0 0\n"
"MouseMoveEvent 83 103 0 0 0 0 0\n"
"RenderEvent 83 103 0 0 0 0 0\n"
"MouseMoveEvent 83 95 0 0 0 0 0\n"
"RenderEvent 83 95 0 0 0 0 0\n"
"MouseMoveEvent 87 89 0 0 0 0 0\n"
"RenderEvent 87 89 0 0 0 0 0\n"
"MouseMoveEvent 89 83 0 0 0 0 0\n"
"RenderEvent 89 83 0 0 0 0 0\n"
"MouseMoveEvent 89 80 0 0 0 0 0\n"
"RenderEvent 89 80 0 0 0 0 0\n"
"MouseMoveEvent 89 78 0 0 0 0 0\n"
"RenderEvent 89 78 0 0 0 0 0\n"
"MouseMoveEvent 90 76 0 0 0 0 0\n"
"RenderEvent 90 76 0 0 0 0 0\n"
"MouseMoveEvent 90 75 0 0 0 0 0\n"
"RenderEvent 90 75 0 0 0 0 0\n"
"MouseMoveEvent 90 74 0 0 0 0 0\n"
"RenderEvent 90 74 0 0 0 0 0\n"
"MouseMoveEvent 90 73 0 0 0 0 0\n"
"RenderEvent 90 73 0 0 0 0 0\n"
"MouseMoveEvent 91 73 0 0 0 0 0\n"
"RenderEvent 91 73 0 0 0 0 0\n"
"MouseMoveEvent 91 74 0 0 0 0 0\n"
"RenderEvent 91 74 0 0 0 0 0\n"
"MouseMoveEvent 91 75 0 0 0 0 0\n"
"RenderEvent 91 75 0 0 0 0 0\n"
"MouseMoveEvent 91 76 0 0 0 0 0\n"
"RenderEvent 91 76 0 0 0 0 0\n"
"MouseMoveEvent 92 77 0 0 0 0 0\n"
"RenderEvent 92 77 0 0 0 0 0\n"
"MouseMoveEvent 92 78 0 0 0 0 0\n"
"RenderEvent 92 78 0 0 0 0 0\n"
"MouseMoveEvent 92 79 0 0 0 0 0\n"
"RenderEvent 92 79 0 0 0 0 0\n"
"MouseMoveEvent 92 80 0 0 0 0 0\n"
"RenderEvent 92 80 0 0 0 0 0\n"
"MouseMoveEvent 92 81 0 0 0 0 0\n"
"RenderEvent 92 81 0 0 0 0 0\n"
"LeftButtonPressEvent 92 81 0 0 0 0 0\n"
"RenderEvent 92 81 0 0 0 0 0\n"
"MouseMoveEvent 93 81 0 0 0 0 0\n"
"RenderEvent 93 81 0 0 0 0 0\n"
"MouseMoveEvent 94 81 0 0 0 0 0\n"
"RenderEvent 94 81 0 0 0 0 0\n"
"MouseMoveEvent 94 82 0 0 0 0 0\n"
"RenderEvent 94 82 0 0 0 0 0\n"
"MouseMoveEvent 95 83 0 0 0 0 0\n"
"RenderEvent 95 83 0 0 0 0 0\n"
"MouseMoveEvent 97 84 0 0 0 0 0\n"
"RenderEvent 97 84 0 0 0 0 0\n"
"MouseMoveEvent 98 86 0 0 0 0 0\n"
"RenderEvent 98 86 0 0 0 0 0\n"
"MouseMoveEvent 99 88 0 0 0 0 0\n"
"RenderEvent 99 88 0 0 0 0 0\n"
"MouseMoveEvent 101 89 0 0 0 0 0\n"
"RenderEvent 101 89 0 0 0 0 0\n"
"MouseMoveEvent 107 93 0 0 0 0 0\n"
"RenderEvent 107 93 0 0 0 0 0\n"
"MouseMoveEvent 113 97 0 0 0 0 0\n"
"RenderEvent 113 97 0 0 0 0 0\n"
"MouseMoveEvent 115 98 0 0 0 0 0\n"
"RenderEvent 115 98 0 0 0 0 0\n"
"MouseMoveEvent 119 102 0 0 0 0 0\n"
"RenderEvent 119 102 0 0 0 0 0\n"
"MouseMoveEvent 125 106 0 0 0 0 0\n"
"RenderEvent 125 106 0 0 0 0 0\n"
"MouseMoveEvent 129 110 0 0 0 0 0\n"
"RenderEvent 129 110 0 0 0 0 0\n"
"MouseMoveEvent 135 112 0 0 0 0 0\n"
"RenderEvent 135 112 0 0 0 0 0\n"
"MouseMoveEvent 139 116 0 0 0 0 0\n"
"RenderEvent 139 116 0 0 0 0 0\n"
"MouseMoveEvent 143 120 0 0 0 0 0\n"
"RenderEvent 143 120 0 0 0 0 0\n"
"MouseMoveEvent 145 121 0 0 0 0 0\n"
"RenderEvent 145 121 0 0 0 0 0\n"
"MouseMoveEvent 149 125 0 0 0 0 0\n"
"RenderEvent 149 125 0 0 0 0 0\n"
"MouseMoveEvent 151 126 0 0 0 0 0\n"
"RenderEvent 151 126 0 0 0 0 0\n"
"MouseMoveEvent 155 130 0 0 0 0 0\n"
"RenderEvent 155 130 0 0 0 0 0\n"
"MouseMoveEvent 157 131 0 0 0 0 0\n"
"RenderEvent 157 131 0 0 0 0 0\n"
"MouseMoveEvent 160 131 0 0 0 0 0\n"
"RenderEvent 160 131 0 0 0 0 0\n"
"MouseMoveEvent 162 132 0 0 0 0 0\n"
"RenderEvent 162 132 0 0 0 0 0\n"
"MouseMoveEvent 163 134 0 0 0 0 0\n"
"RenderEvent 163 134 0 0 0 0 0\n"
"MouseMoveEvent 165 135 0 0 0 0 0\n"
"RenderEvent 165 135 0 0 0 0 0\n"
"MouseMoveEvent 169 139 0 0 0 0 0\n"
"RenderEvent 169 139 0 0 0 0 0\n"
"MouseMoveEvent 171 140 0 0 0 0 0\n"
"RenderEvent 171 140 0 0 0 0 0\n"
"MouseMoveEvent 173 141 0 0 0 0 0\n"
"RenderEvent 173 141 0 0 0 0 0\n"
"MouseMoveEvent 174 143 0 0 0 0 0\n"
"RenderEvent 174 143 0 0 0 0 0\n"
"MouseMoveEvent 176 144 0 0 0 0 0\n"
"RenderEvent 176 144 0 0 0 0 0\n"
"MouseMoveEvent 178 144 0 0 0 0 0\n"
"RenderEvent 178 144 0 0 0 0 0\n"
"MouseMoveEvent 180 145 0 0 0 0 0\n"
"RenderEvent 180 145 0 0 0 0 0\n"
"MouseMoveEvent 182 146 0 0 0 0 0\n"
"RenderEvent 182 146 0 0 0 0 0\n"
"MouseMoveEvent 184 146 0 0 0 0 0\n"
"RenderEvent 184 146 0 0 0 0 0\n"
"MouseMoveEvent 186 147 0 0 0 0 0\n"
"RenderEvent 186 147 0 0 0 0 0\n"
"MouseMoveEvent 187 148 0 0 0 0 0\n"
"RenderEvent 187 148 0 0 0 0 0\n"
"MouseMoveEvent 189 148 0 0 0 0 0\n"
"RenderEvent 189 148 0 0 0 0 0\n"
"MouseMoveEvent 190 149 0 0 0 0 0\n"
"RenderEvent 190 149 0 0 0 0 0\n"
"MouseMoveEvent 191 149 0 0 0 0 0\n"
"RenderEvent 191 149 0 0 0 0 0\n"
"MouseMoveEvent 192 150 0 0 0 0 0\n"
"RenderEvent 192 150 0 0 0 0 0\n"
"MouseMoveEvent 194 150 0 0 0 0 0\n"
"RenderEvent 194 150 0 0 0 0 0\n"
"MouseMoveEvent 195 151 0 0 0 0 0\n"
"RenderEvent 195 151 0 0 0 0 0\n"
"MouseMoveEvent 196 151 0 0 0 0 0\n"
"RenderEvent 196 151 0 0 0 0 0\n"
"MouseMoveEvent 198 151 0 0 0 0 0\n"
"RenderEvent 198 151 0 0 0 0 0\n"
"MouseMoveEvent 199 152 0 0 0 0 0\n"
"RenderEvent 199 152 0 0 0 0 0\n"
"MouseMoveEvent 201 152 0 0 0 0 0\n"
"RenderEvent 201 152 0 0 0 0 0\n"
"MouseMoveEvent 203 153 0 0 0 0 0\n"
"RenderEvent 203 153 0 0 0 0 0\n"
"MouseMoveEvent 205 153 0 0 0 0 0\n"
"RenderEvent 205 153 0 0 0 0 0\n"
"MouseMoveEvent 207 153 0 0 0 0 0\n"
"RenderEvent 207 153 0 0 0 0 0\n"
"MouseMoveEvent 209 153 0 0 0 0 0\n"
"RenderEvent 209 153 0 0 0 0 0\n"
"MouseMoveEvent 210 153 0 0 0 0 0\n"
"RenderEvent 210 153 0 0 0 0 0\n"
"MouseMoveEvent 213 153 0 0 0 0 0\n"
"RenderEvent 213 153 0 0 0 0 0\n"
"MouseMoveEvent 215 153 0 0 0 0 0\n"
"RenderEvent 215 153 0 0 0 0 0\n"
"MouseMoveEvent 217 153 0 0 0 0 0\n"
"RenderEvent 217 153 0 0 0 0 0\n"
"MouseMoveEvent 219 153 0 0 0 0 0\n"
"RenderEvent 219 153 0 0 0 0 0\n"
"MouseMoveEvent 220 153 0 0 0 0 0\n"
"RenderEvent 220 153 0 0 0 0 0\n"
"MouseMoveEvent 221 153 0 0 0 0 0\n"
"RenderEvent 221 153 0 0 0 0 0\n"
"MouseMoveEvent 222 153 0 0 0 0 0\n"
"RenderEvent 222 153 0 0 0 0 0\n"
"MouseMoveEvent 224 153 0 0 0 0 0\n"
"RenderEvent 224 153 0 0 0 0 0\n"
"MouseMoveEvent 227 153 0 0 0 0 0\n"
"RenderEvent 227 153 0 0 0 0 0\n"
"MouseMoveEvent 228 153 0 0 0 0 0\n"
"RenderEvent 228 153 0 0 0 0 0\n"
"MouseMoveEvent 229 153 0 0 0 0 0\n"
"RenderEvent 229 153 0 0 0 0 0\n"
"MouseMoveEvent 230 153 0 0 0 0 0\n"
"RenderEvent 230 153 0 0 0 0 0\n"
"MouseMoveEvent 231 153 0 0 0 0 0\n"
"RenderEvent 231 153 0 0 0 0 0\n"
"MouseMoveEvent 232 153 0 0 0 0 0\n"
"RenderEvent 232 153 0 0 0 0 0\n"
"MouseMoveEvent 233 153 0 0 0 0 0\n"
"RenderEvent 233 153 0 0 0 0 0\n"
"MouseMoveEvent 234 153 0 0 0 0 0\n"
"RenderEvent 234 153 0 0 0 0 0\n"
"MouseMoveEvent 235 153 0 0 0 0 0\n"
"RenderEvent 235 153 0 0 0 0 0\n"
"MouseMoveEvent 236 153 0 0 0 0 0\n"
"RenderEvent 236 153 0 0 0 0 0\n"
"MouseMoveEvent 237 153 0 0 0 0 0\n"
"RenderEvent 237 153 0 0 0 0 0\n"
"MouseMoveEvent 238 153 0 0 0 0 0\n"
"RenderEvent 238 153 0 0 0 0 0\n"
"MouseMoveEvent 239 153 0 0 0 0 0\n"
"RenderEvent 239 153 0 0 0 0 0\n"
"MouseMoveEvent 240 153 0 0 0 0 0\n"
"RenderEvent 240 153 0 0 0 0 0\n"
"MouseMoveEvent 241 153 0 0 0 0 0\n"
"RenderEvent 241 153 0 0 0 0 0\n"
"MouseMoveEvent 242 153 0 0 0 0 0\n"
"RenderEvent 242 153 0 0 0 0 0\n"
"MouseMoveEvent 243 153 0 0 0 0 0\n"
"RenderEvent 243 153 0 0 0 0 0\n"
"MouseMoveEvent 244 153 0 0 0 0 0\n"
"RenderEvent 244 153 0 0 0 0 0\n"
"MouseMoveEvent 245 153 0 0 0 0 0\n"
"RenderEvent 245 153 0 0 0 0 0\n"
"MouseMoveEvent 246 153 0 0 0 0 0\n"
"RenderEvent 246 153 0 0 0 0 0\n"
"MouseMoveEvent 247 153 0 0 0 0 0\n"
"RenderEvent 247 153 0 0 0 0 0\n"
"MouseMoveEvent 248 153 0 0 0 0 0\n"
"RenderEvent 248 153 0 0 0 0 0\n"
"MouseMoveEvent 249 153 0 0 0 0 0\n"
"RenderEvent 249 153 0 0 0 0 0\n"
"MouseMoveEvent 250 153 0 0 0 0 0\n"
"RenderEvent 250 153 0 0 0 0 0\n"
"MouseMoveEvent 251 153 0 0 0 0 0\n"
"RenderEvent 251 153 0 0 0 0 0\n"
"MouseMoveEvent 252 153 0 0 0 0 0\n"
"RenderEvent 252 153 0 0 0 0 0\n"
"MouseMoveEvent 253 153 0 0 0 0 0\n"
"RenderEvent 253 153 0 0 0 0 0\n"
"MouseMoveEvent 254 153 0 0 0 0 0\n"
"RenderEvent 254 153 0 0 0 0 0\n"
"MouseMoveEvent 255 153 0 0 0 0 0\n"
"RenderEvent 255 153 0 0 0 0 0\n"
"MouseMoveEvent 256 153 0 0 0 0 0\n"
"RenderEvent 256 153 0 0 0 0 0\n"
"LeftButtonReleaseEvent 256 153 0 0 0 0 0\n"
"RenderEvent 256 153 0 0 0 0 0\n"
"MouseMoveEvent 256 152 0 0 0 0 0\n"
"RenderEvent 256 152 0 0 0 0 0\n"
"MouseMoveEvent 255 152 0 0 0 0 0\n"
"RenderEvent 255 152 0 0 0 0 0\n"
"MouseMoveEvent 255 150 0 0 0 0 0\n"
"RenderEvent 255 150 0 0 0 0 0\n"
"MouseMoveEvent 254 149 0 0 0 0 0\n"
"RenderEvent 254 149 0 0 0 0 0\n"
"MouseMoveEvent 253 148 0 0 0 0 0\n"
"RenderEvent 253 148 0 0 0 0 0\n"
"MouseMoveEvent 251 147 0 0 0 0 0\n"
"RenderEvent 251 147 0 0 0 0 0\n"
"MouseMoveEvent 250 146 0 0 0 0 0\n"
"RenderEvent 250 146 0 0 0 0 0\n"
"MouseMoveEvent 249 145 0 0 0 0 0\n"
"RenderEvent 249 145 0 0 0 0 0\n"
"MouseMoveEvent 247 144 0 0 0 0 0\n"
"RenderEvent 247 144 0 0 0 0 0\n"
"MouseMoveEvent 246 143 0 0 0 0 0\n"
"RenderEvent 246 143 0 0 0 0 0\n"
"MouseMoveEvent 244 142 0 0 0 0 0\n"
"RenderEvent 244 142 0 0 0 0 0\n"
"MouseMoveEvent 243 140 0 0 0 0 0\n"
"RenderEvent 243 140 0 0 0 0 0\n"
"MouseMoveEvent 241 139 0 0 0 0 0\n"
"RenderEvent 241 139 0 0 0 0 0\n"
"MouseMoveEvent 239 133 0 0 0 0 0\n"
"RenderEvent 239 133 0 0 0 0 0\n"
"MouseMoveEvent 235 127 0 0 0 0 0\n"
"RenderEvent 235 127 0 0 0 0 0\n"
"MouseMoveEvent 231 121 0 0 0 0 0\n"
"RenderEvent 231 121 0 0 0 0 0\n"
"MouseMoveEvent 229 113 0 0 0 0 0\n"
"RenderEvent 229 113 0 0 0 0 0\n"
"MouseMoveEvent 227 107 0 0 0 0 0\n"
"RenderEvent 227 107 0 0 0 0 0\n"
"MouseMoveEvent 223 101 0 0 0 0 0\n"
"RenderEvent 223 101 0 0 0 0 0\n"
"MouseMoveEvent 221 95 0 0 0 0 0\n"
"RenderEvent 221 95 0 0 0 0 0\n"
"MouseMoveEvent 219 87 0 0 0 0 0\n"
"RenderEvent 219 87 0 0 0 0 0\n"
"MouseMoveEvent 215 83 0 0 0 0 0\n"
"RenderEvent 215 83 0 0 0 0 0\n"
"MouseMoveEvent 213 77 0 0 0 0 0\n"
"RenderEvent 213 77 0 0 0 0 0\n"
"MouseMoveEvent 211 71 0 0 0 0 0\n"
"RenderEvent 211 71 0 0 0 0 0\n"
"MouseMoveEvent 209 65 0 0 0 0 0\n"
"RenderEvent 209 65 0 0 0 0 0\n"
"MouseMoveEvent 208 63 0 0 0 0 0\n"
"RenderEvent 208 63 0 0 0 0 0\n"
"MouseMoveEvent 207 61 0 0 0 0 0\n"
"RenderEvent 207 61 0 0 0 0 0\n"
"MouseMoveEvent 206 59 0 0 0 0 0\n"
"RenderEvent 206 59 0 0 0 0 0\n"
"MouseMoveEvent 206 56 0 0 0 0 0\n"
"RenderEvent 206 56 0 0 0 0 0\n"
"MouseMoveEvent 205 55 0 0 0 0 0\n"
"RenderEvent 205 55 0 0 0 0 0\n"
"MouseMoveEvent 204 53 0 0 0 0 0\n"
"RenderEvent 204 53 0 0 0 0 0\n"
"MouseMoveEvent 204 52 0 0 0 0 0\n"
"RenderEvent 204 52 0 0 0 0 0\n"
"MouseMoveEvent 203 50 0 0 0 0 0\n"
"RenderEvent 203 50 0 0 0 0 0\n"
"MouseMoveEvent 202 49 0 0 0 0 0\n"
"RenderEvent 202 49 0 0 0 0 0\n"
"MouseMoveEvent 201 48 0 0 0 0 0\n"
"RenderEvent 201 48 0 0 0 0 0\n"
"MouseMoveEvent 200 48 0 0 0 0 0\n"
"RenderEvent 200 48 0 0 0 0 0\n"
"KeyPressEvent 222 88 0 0 113 1 q\n"
"CharEvent 222 88 0 0 113 1 q\n"
"ExitEvent 222 88 0 0 113 1 q\n"
;

// This callback is responsible for adjusting the point position.
// It looks in the region around the point and finds the maximum or
// minimum value.
class vtkDistanceWidget3DCallback : public vtkCommand
{
public:
  static vtkDistanceWidget3DCallback *New()
    { return new vtkDistanceWidget3DCallback; }
  void Execute(vtkObject *caller, unsigned long, void*) VTK_OVERRIDE;
  vtkDistanceWidget3DCallback():Renderer(0),RenderWindow(0),DistanceWidget(0),Distance(0) {}
  vtkRenderer *Renderer;
  vtkRenderWindow *RenderWindow;
  vtkDistanceWidget *DistanceWidget;
  vtkDistanceRepresentation3D *Distance;
};


// Method re-positions the points using random perturbation
void vtkDistanceWidget3DCallback::Execute(vtkObject*, unsigned long eid, void* callData)
{
  if ( eid == vtkCommand::InteractionEvent ||
       eid == vtkCommand::EndInteractionEvent )
  {
        double pos1[3], pos2[3];
    // Modify the measure axis
    this->Distance->GetPoint1WorldPosition(pos1);
    this->Distance->GetPoint2WorldPosition(pos2);
    double dist=sqrt(vtkMath::Distance2BetweenPoints(pos1,pos2));

    char title[256];
//    this->Distance->GetAxis()->SetRange(0.0,dist);
    snprintf(title,sizeof(title),"%-#6.3g",dist);
//    this->Distance->GetAxis()->SetTitle(title);
  }
  else
  {
    int pid = *(reinterpret_cast<int*>(callData));

    //From the point id, get the display coordinates
    double pos1[3], pos2[3], *pos;
    this->Distance->GetPoint1DisplayPosition(pos1);
    this->Distance->GetPoint2DisplayPosition(pos2);
    if ( pid == 0 )
    {
      pos = pos1;
    }
    else
    {
      pos = pos2;
    }

    // Okay, render without the widget, and get the color buffer
    int enabled = this->DistanceWidget->GetEnabled();
    if ( enabled )
    {
      this->DistanceWidget->SetEnabled(0); //does a Render() as a side effect
    }

    // Pretend we are doing something serious....just randomly bump the
    // location of the point.
    double p[3];
    p[0] = pos[0] + static_cast<int>(vtkMath::Random(-5.5,5.5));
    p[1] = pos[1] + static_cast<int>(vtkMath::Random(-5.5,5.5));
    p[2] = 0.0;

    // Set the new position
    if ( pid == 0 )
    {
      this->Distance->SetPoint1DisplayPosition(p);
    }
    else
    {
      this->Distance->SetPoint2DisplayPosition(p);
    }

    // Side effect of a render here
    if ( enabled )
    {
      this->DistanceWidget->SetEnabled(1);
    }
  }
}

// The actual test function
int TestDistanceWidget3D( int argc, char *argv[] )
{
  // Create the RenderWindow, Renderer and both Actors
  //
  VTK_CREATE(vtkRenderer, ren1);
  VTK_CREATE(vtkRenderWindow, renWin);
  renWin->AddRenderer(ren1);

  VTK_CREATE(vtkRenderWindowInteractor, iren);;
  iren->SetRenderWindow(renWin);

  // Create a test pipeline
  //
  VTK_CREATE(vtkSphereSource, ss);
  VTK_CREATE(vtkPolyDataMapper, mapper);
  mapper->SetInputConnection(ss->GetOutputPort());
  VTK_CREATE(vtkActor, actor);
  actor->SetMapper(mapper);

  // Create the widget and its representation
  VTK_CREATE(vtkPointHandleRepresentation3D, handle);
  handle->GetProperty()->SetColor(1,0,0);
  VTK_CREATE(vtkDistanceRepresentation3D, rep);
  rep->SetHandleRepresentation(handle);
  rep->RulerModeOn();
  rep->SetRulerDistance(0.1);
  rep->SetNumberOfRulerTicks(4);
  double glyphScale = rep->GetGlyphScale();
  rep->SetGlyphScale(2.0);
  if (rep->GetGlyphScale() != 2.0)
  {
    std::cerr << "Error setting glyph scale to 2.0, returned " << rep->GetGlyphScale() << std::endl;
    return EXIT_FAILURE;
  }
  rep->SetGlyphScale(glyphScale);
  if (rep->GetGlyphScale() != glyphScale)
  {
    std::cerr << "Error setting glyph scale to " << glyphScale << ", returned " << rep->GetGlyphScale() << std::endl;
    return EXIT_FAILURE;
  }
  rep->SetGlyphScale(0.1);
  if (rep->GetGlyphScale() != 0.1)
  {
    cerr << "Error setting glyph scale to 0.1, returned " << rep->GetGlyphScale() << std::endl;
    return EXIT_FAILURE;
  }


  if (!rep->GetLineProperty())
  {
    std::cerr << "Error getting representation line property" << endl;
    return EXIT_FAILURE;
  }
  rep->GetLineProperty()->SetColor(1.0, 0.0, 1.0);
  rep->SetLabelPosition(0.45);
  if (rep->GetLabelPosition() != 0.45)
  {
    std::cerr << "Error setting label position to 0.45, returned : " << rep->GetLabelPosition() << std::endl;
    return EXIT_FAILURE;
  }
  for (int maxTicks = 1; maxTicks < 100; maxTicks += 10)
  {
    rep->SetMaximumNumberOfRulerTicks(maxTicks);
    if (rep->GetMaximumNumberOfRulerTicks() != maxTicks)
    {
      std::cerr << "Error setting maximum number of ruler ticks to " << maxTicks << ", get returned " << rep->GetMaximumNumberOfRulerTicks() << std::endl;
      return EXIT_FAILURE;
    }
  }
  vtkActor *glyphActor = rep->GetGlyphActor();
  if (!glyphActor)
  {
    std::cerr << "Error getting glyph actor" << std::endl;
    return EXIT_FAILURE;
  }
  glyphActor->GetProperty()->SetColor(1.0, 0.0, 0.0);
  vtkFollower *labelActor = rep->GetLabelActor();
  if (!labelActor)
  {
    std::cerr << "Error getting label actor" << std::endl;
    return EXIT_FAILURE;
  }
  labelActor->GetProperty()->SetColor(0.0, 1.0, 0.0);

  VTK_CREATE(vtkDistanceWidget, widget);
  widget->SetInteractor(iren);
  widget->SetRepresentation(rep);

  VTK_CREATE(vtkDistanceWidget3DCallback, mcbk);
  mcbk->Renderer = ren1;
  mcbk->RenderWindow = renWin;
  mcbk->Distance = rep;
  mcbk->DistanceWidget = widget;

  // Add the actors to the renderer, set the background and size
  //
  ren1->AddActor(actor);
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // record events
  VTK_CREATE(vtkInteractorEventRecorder, recorder);
  recorder->SetInteractor(iren);
  recorder->On();
  //recorder->SetFileName("/tmp/record2.log");
  //recorder->Record();
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(TestDistanceWidget3DEventLog);

  // render the image
  //
  iren->Initialize();
  renWin->Render();
  widget->On();
  renWin->Render();
  recorder->Play();

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.

  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  recorder->Off();
  widget->Off();
  widget->RemoveObserver(mcbk);

  return !retVal;
}
