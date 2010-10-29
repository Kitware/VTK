/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAngleWidget2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the vtkAngleWidget.

// First include the required header files for the VTK classes we are using.
#include "vtkSmartPointer.h"
#include "vtkAngleWidget.h"
#include "vtkAngleRepresentation2D.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCommand.h"
#include "vtkCoordinate.h"
#include "vtkMath.h"
#include "vtkHandleWidget.h"
#include "vtkPointHandleRepresentation2D.h"
#include "vtkAxisActor2D.h"
#include "vtkProperty2D.h"
#include "vtkTesting.h"

char TestAngleWidget2DEventLog[] =
  "# StreamVersion 1\n"
  "EnterEvent 185 1 0 0 0 0 0\n"
  "MouseMoveEvent 179 19 0 0 0 0 0\n"
  "MouseMoveEvent 175 35 0 0 0 0 0\n"
  "MouseMoveEvent 173 49 0 0 0 0 0\n"
  "MouseMoveEvent 171 61 0 0 0 0 0\n"
  "MouseMoveEvent 171 71 0 0 0 0 0\n"
  "MouseMoveEvent 169 79 0 0 0 0 0\n"
  "MouseMoveEvent 169 82 0 0 0 0 0\n"
  "MouseMoveEvent 169 85 0 0 0 0 0\n"
  "MouseMoveEvent 169 88 0 0 0 0 0\n"
  "MouseMoveEvent 169 90 0 0 0 0 0\n"
  "MouseMoveEvent 168 92 0 0 0 0 0\n"
  "MouseMoveEvent 168 93 0 0 0 0 0\n"
  "MouseMoveEvent 168 94 0 0 0 0 0\n"
  "MouseMoveEvent 168 95 0 0 0 0 0\n"
  "MouseMoveEvent 168 96 0 0 0 0 0\n"
  "MouseMoveEvent 168 98 0 0 0 0 0\n"
  "MouseMoveEvent 167 99 0 0 0 0 0\n"
  "MouseMoveEvent 167 100 0 0 0 0 0\n"
  "MouseMoveEvent 167 102 0 0 0 0 0\n"
  "MouseMoveEvent 166 104 0 0 0 0 0\n"
  "MouseMoveEvent 166 107 0 0 0 0 0\n"
  "MouseMoveEvent 165 109 0 0 0 0 0\n"
  "MouseMoveEvent 164 111 0 0 0 0 0\n"
  "MouseMoveEvent 163 113 0 0 0 0 0\n"
  "MouseMoveEvent 162 115 0 0 0 0 0\n"
  "MouseMoveEvent 160 116 0 0 0 0 0\n"
  "MouseMoveEvent 154 122 0 0 0 0 0\n"
  "MouseMoveEvent 148 128 0 0 0 0 0\n"
  "MouseMoveEvent 142 132 0 0 0 0 0\n"
  "MouseMoveEvent 136 138 0 0 0 0 0\n"
  "MouseMoveEvent 134 139 0 0 0 0 0\n"
  "MouseMoveEvent 130 143 0 0 0 0 0\n"
  "MouseMoveEvent 128 144 0 0 0 0 0\n"
  "MouseMoveEvent 126 145 0 0 0 0 0\n"
  "MouseMoveEvent 124 146 0 0 0 0 0\n"
  "MouseMoveEvent 124 147 0 0 0 0 0\n"
  "MouseMoveEvent 122 147 0 0 0 0 0\n"
  "MouseMoveEvent 121 148 0 0 0 0 0\n"
  "MouseMoveEvent 120 148 0 0 0 0 0\n"
  "MouseMoveEvent 119 149 0 0 0 0 0\n"
  "MouseMoveEvent 118 150 0 0 0 0 0\n"
  "MouseMoveEvent 116 151 0 0 0 0 0\n"
  "MouseMoveEvent 116 152 0 0 0 0 0\n"
  "MouseMoveEvent 115 152 0 0 0 0 0\n"
  "MouseMoveEvent 114 152 0 0 0 0 0\n"
  "MouseMoveEvent 113 153 0 0 0 0 0\n"
  "MouseMoveEvent 112 154 0 0 0 0 0\n"
  "MouseMoveEvent 111 155 0 0 0 0 0\n"
  "MouseMoveEvent 110 156 0 0 0 0 0\n"
  "MouseMoveEvent 110 157 0 0 0 0 0\n"
  "MouseMoveEvent 109 158 0 0 0 0 0\n"
  "MouseMoveEvent 108 158 0 0 0 0 0\n"
  "MouseMoveEvent 108 159 0 0 0 0 0\n"
  "MouseMoveEvent 107 159 0 0 0 0 0\n"
  "MouseMoveEvent 107 160 0 0 0 0 0\n"
  "MouseMoveEvent 106 161 0 0 0 0 0\n"
  "MouseMoveEvent 106 162 0 0 0 0 0\n"
  "MouseMoveEvent 105 162 0 0 0 0 0\n"
  "MouseMoveEvent 104 163 0 0 0 0 0\n"
  "MouseMoveEvent 103 163 0 0 0 0 0\n"
  "MouseMoveEvent 102 164 0 0 0 0 0\n"
  "MouseMoveEvent 101 164 0 0 0 0 0\n"
  "MouseMoveEvent 101 165 0 0 0 0 0\n"
  "MouseMoveEvent 100 166 0 0 0 0 0\n"
  "MouseMoveEvent 99 167 0 0 0 0 0\n"
  "MouseMoveEvent 98 168 0 0 0 0 0\n"
  "MouseMoveEvent 98 169 0 0 0 0 0\n"
  "MouseMoveEvent 97 169 0 0 0 0 0\n"
  "MouseMoveEvent 96 169 0 0 0 0 0\n"
  "MouseMoveEvent 95 170 0 0 0 0 0\n"
  "MouseMoveEvent 95 171 0 0 0 0 0\n"
  "MouseMoveEvent 93 172 0 0 0 0 0\n"
  "MouseMoveEvent 92 172 0 0 0 0 0\n"
  "MouseMoveEvent 86 170 0 0 0 0 0\n"
  "MouseMoveEvent 85 170 0 0 0 0 0\n"
  "MouseMoveEvent 83 169 0 0 0 0 0\n"
  "MouseMoveEvent 82 169 0 0 0 0 0\n"
  "MouseMoveEvent 81 169 0 0 0 0 0\n"
  "MouseMoveEvent 80 169 0 0 0 0 0\n"
  "MouseMoveEvent 79 169 0 0 0 0 0\n"
  "MouseMoveEvent 78 169 0 0 0 0 0\n"
  "MouseMoveEvent 77 169 0 0 0 0 0\n"
  "MouseMoveEvent 76 170 0 0 0 0 0\n"
  "MouseMoveEvent 75 170 0 0 0 0 0\n"
  "MouseMoveEvent 74 171 0 0 0 0 0\n"
  "MouseMoveEvent 73 171 0 0 0 0 0\n"
  "MouseMoveEvent 72 171 0 0 0 0 0\n"
  "MouseMoveEvent 71 172 0 0 0 0 0\n"
  "MouseMoveEvent 70 172 0 0 0 0 0\n"
  "MouseMoveEvent 70 173 0 0 0 0 0\n"
  "MouseMoveEvent 69 173 0 0 0 0 0\n"
  "MouseMoveEvent 69 174 0 0 0 0 0\n"
  "MouseMoveEvent 68 175 0 0 0 0 0\n"
  "MouseMoveEvent 64 179 0 0 0 0 0\n"
  "MouseMoveEvent 62 180 0 0 0 0 0\n"
  "MouseMoveEvent 61 181 0 0 0 0 0\n"
  "MouseMoveEvent 60 183 0 0 0 0 0\n"
  "MouseMoveEvent 59 184 0 0 0 0 0\n"
  "MouseMoveEvent 58 185 0 0 0 0 0\n"
  "MouseMoveEvent 57 187 0 0 0 0 0\n"
  "MouseMoveEvent 56 187 0 0 0 0 0\n"
  "MouseMoveEvent 56 188 0 0 0 0 0\n"
  "MouseMoveEvent 55 188 0 0 0 0 0\n"
  "MouseMoveEvent 55 189 0 0 0 0 0\n"
  "MouseMoveEvent 56 189 0 0 0 0 0\n"
  "MouseMoveEvent 57 189 0 0 0 0 0\n"
  "MouseMoveEvent 58 189 0 0 0 0 0\n"
  "MouseMoveEvent 59 189 0 0 0 0 0\n"
  "MouseMoveEvent 60 189 0 0 0 0 0\n"
  "MouseMoveEvent 61 189 0 0 0 0 0\n"
  "LeftButtonPressEvent 61 189 0 0 0 0 0\n"
  "RenderEvent 61 189 0 0 0 0 0\n"
  "LeftButtonReleaseEvent 61 189 0 0 0 0 0\n"
  "MouseMoveEvent 61 187 0 0 0 0 0\n"
  "RenderEvent 61 187 0 0 0 0 0\n"
  "MouseMoveEvent 61 185 0 0 0 0 0\n"
  "RenderEvent 61 185 0 0 0 0 0\n"
  "MouseMoveEvent 61 184 0 0 0 0 0\n"
  "RenderEvent 61 184 0 0 0 0 0\n"
  "MouseMoveEvent 61 181 0 0 0 0 0\n"
  "RenderEvent 61 181 0 0 0 0 0\n"
  "MouseMoveEvent 58 172 0 0 0 0 0\n"
  "RenderEvent 58 172 0 0 0 0 0\n"
  "MouseMoveEvent 58 169 0 0 0 0 0\n"
  "RenderEvent 58 169 0 0 0 0 0\n"
  "MouseMoveEvent 56 161 0 0 0 0 0\n"
  "RenderEvent 56 161 0 0 0 0 0\n"
  "MouseMoveEvent 52 143 0 0 0 0 0\n"
  "RenderEvent 52 143 0 0 0 0 0\n"
  "MouseMoveEvent 51 141 0 0 0 0 0\n"
  "RenderEvent 51 141 0 0 0 0 0\n"
  "MouseMoveEvent 41 123 0 0 0 0 0\n"
  "RenderEvent 41 123 0 0 0 0 0\n"
  "MouseMoveEvent 40 121 0 0 0 0 0\n"
  "RenderEvent 40 121 0 0 0 0 0\n"
  "MouseMoveEvent 33 109 0 0 0 0 0\n"
  "RenderEvent 33 109 0 0 0 0 0\n"
  "MouseMoveEvent 32 107 0 0 0 0 0\n"
  "RenderEvent 32 107 0 0 0 0 0\n"
  "MouseMoveEvent 28 103 0 0 0 0 0\n"
  "RenderEvent 28 103 0 0 0 0 0\n"
  "MouseMoveEvent 27 102 0 0 0 0 0\n"
  "RenderEvent 27 102 0 0 0 0 0\n"
  "MouseMoveEvent 26 100 0 0 0 0 0\n"
  "RenderEvent 26 100 0 0 0 0 0\n"
  "MouseMoveEvent 25 99 0 0 0 0 0\n"
  "RenderEvent 25 99 0 0 0 0 0\n"
  "MouseMoveEvent 22 95 0 0 0 0 0\n"
  "RenderEvent 22 95 0 0 0 0 0\n"
  "MouseMoveEvent 18 91 0 0 0 0 0\n"
  "RenderEvent 18 91 0 0 0 0 0\n"
  "MouseMoveEvent 12 85 0 0 0 0 0\n"
  "RenderEvent 12 85 0 0 0 0 0\n"
  "MouseMoveEvent 12 84 0 0 0 0 0\n"
  "RenderEvent 12 84 0 0 0 0 0\n"
  "MouseMoveEvent 10 83 0 0 0 0 0\n"
  "RenderEvent 10 83 0 0 0 0 0\n"
  "MouseMoveEvent 9 82 0 0 0 0 0\n"
  "RenderEvent 9 82 0 0 0 0 0\n"
  "MouseMoveEvent 9 80 0 0 0 0 0\n"
  "RenderEvent 9 80 0 0 0 0 0\n"
  "MouseMoveEvent 9 76 0 0 0 0 0\n"
  "RenderEvent 9 76 0 0 0 0 0\n"
  "MouseMoveEvent 9 74 0 0 0 0 0\n"
  "RenderEvent 9 74 0 0 0 0 0\n"
  "MouseMoveEvent 9 73 0 0 0 0 0\n"
  "RenderEvent 9 73 0 0 0 0 0\n"
  "MouseMoveEvent 10 72 0 0 0 0 0\n"
  "RenderEvent 10 72 0 0 0 0 0\n"
  "MouseMoveEvent 12 71 0 0 0 0 0\n"
  "RenderEvent 12 71 0 0 0 0 0\n"
  "MouseMoveEvent 14 71 0 0 0 0 0\n"
  "RenderEvent 14 71 0 0 0 0 0\n"
  "MouseMoveEvent 16 71 0 0 0 0 0\n"
  "RenderEvent 16 71 0 0 0 0 0\n"
  "MouseMoveEvent 16 70 0 0 0 0 0\n"
  "RenderEvent 16 70 0 0 0 0 0\n"
  "MouseMoveEvent 20 68 0 0 0 0 0\n"
  "RenderEvent 20 68 0 0 0 0 0\n"
  "MouseMoveEvent 21 67 0 0 0 0 0\n"
  "RenderEvent 21 67 0 0 0 0 0\n"
  "MouseMoveEvent 24 66 0 0 0 0 0\n"
  "RenderEvent 24 66 0 0 0 0 0\n"
  "MouseMoveEvent 25 66 0 0 0 0 0\n"
  "RenderEvent 25 66 0 0 0 0 0\n"
  "LeftButtonPressEvent 25 66 0 0 0 0 0\n"
  "RenderEvent 25 66 0 0 0 0 0\n"
  "LeftButtonReleaseEvent 25 66 0 0 0 0 0\n"
  "MouseMoveEvent 26 66 0 0 0 0 0\n"
  "RenderEvent 26 66 0 0 0 0 0\n"
  "MouseMoveEvent 27 66 0 0 0 0 0\n"
  "RenderEvent 27 66 0 0 0 0 0\n"
  "MouseMoveEvent 28 66 0 0 0 0 0\n"
  "RenderEvent 28 66 0 0 0 0 0\n"
  "MouseMoveEvent 29 66 0 0 0 0 0\n"
  "RenderEvent 29 66 0 0 0 0 0\n"
  "MouseMoveEvent 37 67 0 0 0 0 0\n"
  "RenderEvent 37 67 0 0 0 0 0\n"
  "MouseMoveEvent 40 67 0 0 0 0 0\n"
  "RenderEvent 40 67 0 0 0 0 0\n"
  "MouseMoveEvent 54 67 0 0 0 0 0\n"
  "RenderEvent 54 67 0 0 0 0 0\n"
  "MouseMoveEvent 62 67 0 0 0 0 0\n"
  "RenderEvent 62 67 0 0 0 0 0\n"
  "MouseMoveEvent 76 67 0 0 0 0 0\n"
  "RenderEvent 76 67 0 0 0 0 0\n"
  "MouseMoveEvent 86 67 0 0 0 0 0\n"
  "RenderEvent 86 67 0 0 0 0 0\n"
  "MouseMoveEvent 114 67 0 0 0 0 0\n"
  "RenderEvent 114 67 0 0 0 0 0\n"
  "MouseMoveEvent 122 67 0 0 0 0 0\n"
  "RenderEvent 122 67 0 0 0 0 0\n"
  "MouseMoveEvent 146 67 0 0 0 0 0\n"
  "RenderEvent 146 67 0 0 0 0 0\n"
  "MouseMoveEvent 149 67 0 0 0 0 0\n"
  "RenderEvent 149 67 0 0 0 0 0\n"
  "MouseMoveEvent 157 67 0 0 0 0 0\n"
  "RenderEvent 157 67 0 0 0 0 0\n"
  "MouseMoveEvent 159 67 0 0 0 0 0\n"
  "RenderEvent 159 67 0 0 0 0 0\n"
  "MouseMoveEvent 163 69 0 0 0 0 0\n"
  "RenderEvent 163 69 0 0 0 0 0\n"
  "MouseMoveEvent 165 70 0 0 0 0 0\n"
  "RenderEvent 165 70 0 0 0 0 0\n"
  "MouseMoveEvent 170 71 0 0 0 0 0\n"
  "RenderEvent 170 71 0 0 0 0 0\n"
  "MouseMoveEvent 172 71 0 0 0 0 0\n"
  "RenderEvent 172 71 0 0 0 0 0\n"
  "MouseMoveEvent 196 71 0 0 0 0 0\n"
  "RenderEvent 196 71 0 0 0 0 0\n"
  "MouseMoveEvent 202 69 0 0 0 0 0\n"
  "RenderEvent 202 69 0 0 0 0 0\n"
  "MouseMoveEvent 223 69 0 0 0 0 0\n"
  "RenderEvent 223 69 0 0 0 0 0\n"
  "MouseMoveEvent 226 69 0 0 0 0 0\n"
  "RenderEvent 226 69 0 0 0 0 0\n"
  "MouseMoveEvent 236 71 0 0 0 0 0\n"
  "RenderEvent 236 71 0 0 0 0 0\n"
  "MouseMoveEvent 238 70 0 0 0 0 0\n"
  "RenderEvent 238 70 0 0 0 0 0\n"
  "MouseMoveEvent 240 71 0 0 0 0 0\n"
  "RenderEvent 240 71 0 0 0 0 0\n"
  "MouseMoveEvent 240 72 0 0 0 0 0\n"
  "RenderEvent 240 72 0 0 0 0 0\n"
  "MouseMoveEvent 240 73 0 0 0 0 0\n"
  "RenderEvent 240 73 0 0 0 0 0\n"
  "MouseMoveEvent 240 74 0 0 0 0 0\n"
  "RenderEvent 240 74 0 0 0 0 0\n"
  "MouseMoveEvent 240 75 0 0 0 0 0\n"
  "RenderEvent 240 75 0 0 0 0 0\n"
  "MouseMoveEvent 237 77 0 0 0 0 0\n"
  "RenderEvent 237 77 0 0 0 0 0\n"
  "MouseMoveEvent 235 78 0 0 0 0 0\n"
  "RenderEvent 235 78 0 0 0 0 0\n"
  "MouseMoveEvent 231 82 0 0 0 0 0\n"
  "RenderEvent 231 82 0 0 0 0 0\n"
  "MouseMoveEvent 229 83 0 0 0 0 0\n"
  "RenderEvent 229 83 0 0 0 0 0\n"
  "MouseMoveEvent 223 87 0 0 0 0 0\n"
  "RenderEvent 223 87 0 0 0 0 0\n"
  "MouseMoveEvent 221 88 0 0 0 0 0\n"
  "RenderEvent 221 88 0 0 0 0 0\n"
  "MouseMoveEvent 215 90 0 0 0 0 0\n"
  "RenderEvent 215 90 0 0 0 0 0\n"
  "MouseMoveEvent 209 94 0 0 0 0 0\n"
  "RenderEvent 209 94 0 0 0 0 0\n"
  "MouseMoveEvent 207 94 0 0 0 0 0\n"
  "RenderEvent 207 94 0 0 0 0 0\n"
  "MouseMoveEvent 200 100 0 0 0 0 0\n"
  "RenderEvent 200 100 0 0 0 0 0\n"
  "MouseMoveEvent 199 101 0 0 0 0 0\n"
  "RenderEvent 199 101 0 0 0 0 0\n"
  "MouseMoveEvent 198 102 0 0 0 0 0\n"
  "RenderEvent 198 102 0 0 0 0 0\n"
  "MouseMoveEvent 198 103 0 0 0 0 0\n"
  "RenderEvent 198 103 0 0 0 0 0\n"
  "MouseMoveEvent 198 104 0 0 0 0 0\n"
  "RenderEvent 198 104 0 0 0 0 0\n"
  "MouseMoveEvent 199 104 0 0 0 0 0\n"
  "RenderEvent 199 104 0 0 0 0 0\n"
  "MouseMoveEvent 199 105 0 0 0 0 0\n"
  "RenderEvent 199 105 0 0 0 0 0\n"
  "MouseMoveEvent 201 106 0 0 0 0 0\n"
  "RenderEvent 201 106 0 0 0 0 0\n"
  "MouseMoveEvent 203 106 0 0 0 0 0\n"
  "RenderEvent 203 106 0 0 0 0 0\n"
  "MouseMoveEvent 204 106 0 0 0 0 0\n"
  "RenderEvent 204 106 0 0 0 0 0\n"
  "MouseMoveEvent 206 107 0 0 0 0 0\n"
  "RenderEvent 206 107 0 0 0 0 0\n"
  "MouseMoveEvent 207 107 0 0 0 0 0\n"
  "RenderEvent 207 107 0 0 0 0 0\n"
  "LeftButtonPressEvent 207 107 0 0 0 0 0\n"
  "RenderEvent 207 107 0 0 0 0 0\n"
  "LeftButtonReleaseEvent 207 107 0 0 0 0 0\n"
  "MouseMoveEvent 207 106 0 0 0 0 0\n"
  "MouseMoveEvent 208 105 0 0 0 0 0\n"
  "MouseMoveEvent 209 104 0 0 0 0 0\n"
  "MouseMoveEvent 209 103 0 0 0 0 0\n"
  "MouseMoveEvent 210 102 0 0 0 0 0\n"
  "MouseMoveEvent 211 101 0 0 0 0 0\n"
  "MouseMoveEvent 211 100 0 0 0 0 0\n"
  "MouseMoveEvent 212 99 0 0 0 0 0\n"
  "MouseMoveEvent 213 97 0 0 0 0 0\n"
  "MouseMoveEvent 214 96 0 0 0 0 0\n"
  "MouseMoveEvent 215 95 0 0 0 0 0\n"
  "MouseMoveEvent 216 95 0 0 0 0 0\n"
  "MouseMoveEvent 216 94 0 0 0 0 0\n"
  "MouseMoveEvent 217 93 0 0 0 0 0\n"
  "MouseMoveEvent 217 92 0 0 0 0 0\n"
  "MouseMoveEvent 218 91 0 0 0 0 0\n"
  "MouseMoveEvent 219 90 0 0 0 0 0\n"
  "MouseMoveEvent 220 90 0 0 0 0 0\n"
  "MouseMoveEvent 220 89 0 0 0 0 0\n"
  "MouseMoveEvent 221 89 0 0 0 0 0\n"
  "MouseMoveEvent 222 89 0 0 0 0 0\n"
  "MouseMoveEvent 222 88 0 0 0 0 0\n"
  "KeyPressEvent 222 88 0 0 113 1 q\n"
  "CharEvent 222 88 0 0 113 1 q\n"
  "ExitEvent 222 88 0 0 113 1 q\n"
  ;


// This callback is responsible for setting the angle label.
class vtkAngleCallback : public vtkCommand
{
public:
  static vtkAngleCallback *New() 
  { return new vtkAngleCallback; }
  virtual void Execute(vtkObject*, unsigned long eid, void*)
  {
    if ( eid == vtkCommand::PlacePointEvent )
      {
      std::cout << "point placed\n";
      }
    else if ( eid == vtkCommand::InteractionEvent )
      {
      double point1[3], center[3], point2[3];
      this->Rep->GetPoint1WorldPosition(point1);
      this->Rep->GetCenterWorldPosition(center);
      this->Rep->GetPoint2WorldPosition(point2);
      std::cout << "Angle between " << "("
           << point1[0] << ","
           << point1[1] << ","
           << point1[2] << "), ("
           << center[0] << ","
           << center[1] << ","
           << center[2] << ") and ("
           << point2[0] << ","
           << point2[1] << ","
           << point2[2] << ") is "
           << this->Rep->GetAngle() << std::endl;
      }
  }
  vtkAngleRepresentation2D *Rep;
  vtkAngleCallback():Rep(0) {}
};


// The actual test function
int TestAngleWidget2D( int argc, char *argv[] )
{
  // Create the RenderWindow, Renderer and both Actors
  //
  vtkSmartPointer< vtkRenderer > ren1
    = vtkSmartPointer< vtkRenderer >::New();
  vtkSmartPointer< vtkRenderWindow > renWin
    = vtkSmartPointer< vtkRenderWindow >::New();
  renWin->AddRenderer(ren1);

  vtkSmartPointer< vtkRenderWindowInteractor > iren
    = vtkSmartPointer< vtkRenderWindowInteractor >::New();
  iren->SetRenderWindow(renWin);

  // Create a test pipeline
  //
  vtkSmartPointer< vtkSphereSource > ss
    = vtkSmartPointer< vtkSphereSource >::New();
  vtkSmartPointer< vtkPolyDataMapper > mapper
    = vtkSmartPointer< vtkPolyDataMapper >::New();
  mapper->SetInput(ss->GetOutput());
  vtkSmartPointer< vtkActor > actor
    = vtkSmartPointer< vtkActor >::New();
  actor->SetMapper(mapper);

  // Create the widget and its representation
  vtkSmartPointer< vtkPointHandleRepresentation2D > handle
    = vtkSmartPointer< vtkPointHandleRepresentation2D >::New();
  handle->GetProperty()->SetColor(1,0,0);
  vtkSmartPointer< vtkAngleRepresentation2D > rep
    = vtkSmartPointer< vtkAngleRepresentation2D >::New();
  rep->SetHandleRepresentation(handle);

  vtkSmartPointer< vtkAngleWidget > widget
    = vtkSmartPointer< vtkAngleWidget >::New();
  widget->SetInteractor(iren);
  widget->CreateDefaultRepresentation();
  widget->SetRepresentation(rep);

  vtkSmartPointer< vtkAngleCallback > mcbk
    = vtkSmartPointer< vtkAngleCallback >::New();
  mcbk->Rep = rep;
  widget->AddObserver(vtkCommand::PlacePointEvent,mcbk);
  widget->AddObserver(vtkCommand::InteractionEvent,mcbk);

  // Add the actors to the renderer, set the background and size
  //
  ren1->AddActor(actor);
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // render the image
  //
  iren->Initialize();
  widget->On();
  renWin->Render();

  return vtkTesting::InteractorEventLoop(
      argc, argv, iren, TestAngleWidget2DEventLog );
}
