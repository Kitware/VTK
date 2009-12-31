/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestHandleWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the vtkHandleWidget.
//
// The handle that you see is always constrained to lie on a plane (
// defined by a vtkImplicitPlaneWidget2). It  goes to show that you can place 
// constraints on the movement of the handle. You can move the plane around 
// interactively. It exercises the class vtkBoundedPlanePointPlacer.


#include "vtkConeSource.h"
#include "vtkGlyph3D.h"
#include "vtkSphereSource.h"
#include "vtkAppendPolyData.h"
#include "vtkHandleWidget.h"
#include "vtkPointHandleRepresentation3D.h"
#include "vtkCoordinate.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCommand.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkDebugLeaks.h"
#include "vtkOutlineFilter.h"
#include "vtkImplicitPlaneWidget2.h"
#include "vtkImplicitPlaneRepresentation.h"
#include "vtkBoundedPlanePointPlacer.h"
#include "vtkCutter.h"
#include "vtkLODActor.h"
#include "vtkPlane.h"
#include "vtkProperty.h"
#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

// -----------------------------------------------------------------------
// This does the actual work: updates the vtkPline implicit function.
// This in turn causes the pipeline to update and clip the object.
// Callback for the interaction
class vtkTIPW3Callback : public vtkCommand
{
public:
  static vtkTIPW3Callback *New() 
    { return new vtkTIPW3Callback; }
  virtual void Execute(vtkObject *caller, unsigned long, void*)
    {
      vtkImplicitPlaneWidget2 *planeWidget = 
        reinterpret_cast<vtkImplicitPlaneWidget2*>(caller);
      vtkImplicitPlaneRepresentation *rep = 
        reinterpret_cast<vtkImplicitPlaneRepresentation*>(planeWidget->GetRepresentation());
      rep->GetPlane(this->Plane);
      this->Actor->VisibilityOn();
    }

  vtkTIPW3Callback() : Actor(0) { this->Plane = vtkPlane::New(); }
  ~vtkTIPW3Callback() { this->Plane->Delete(); }
  
  vtkPlane *Plane;
  vtkActor *Actor;
};

// -----------------------------------------------------------------------
char HandleWidgetLog[] = 
"# StreamVersion 1\n"
"RenderEvent 0 0 0 0 0 0 0\n"
"RenderEvent 0 0 0 0 0 0 0\n"
"RenderEvent 0 0 0 0 0 0 0\n"
"EnterEvent 294 159 0 0 0 0 0\n"
"MouseMoveEvent 294 159 0 0 0 0 0\n"
"MouseMoveEvent 289 159 0 0 0 0 0\n"
"MouseMoveEvent 288 159 0 0 0 0 0\n"
"MouseMoveEvent 284 159 0 0 0 0 0\n"
"MouseMoveEvent 282 160 0 0 0 0 0\n"
"MouseMoveEvent 278 160 0 0 0 0 0\n"
"MouseMoveEvent 270 160 0 0 0 0 0\n"
"MouseMoveEvent 263 161 0 0 0 0 0\n"
"MouseMoveEvent 261 161 0 0 0 0 0\n"
"MouseMoveEvent 256 162 0 0 0 0 0\n"
"MouseMoveEvent 253 162 0 0 0 0 0\n"
"MouseMoveEvent 252 162 0 0 0 0 0\n"
"MouseMoveEvent 251 163 0 0 0 0 0\n"
"MouseMoveEvent 249 163 0 0 0 0 0\n"
"MouseMoveEvent 247 164 0 0 0 0 0\n"
"MouseMoveEvent 243 164 0 0 0 0 0\n"
"MouseMoveEvent 241 164 0 0 0 0 0\n"
"MouseMoveEvent 239 164 0 0 0 0 0\n"
"MouseMoveEvent 238 164 0 0 0 0 0\n"
"MouseMoveEvent 238 166 0 0 0 0 0\n"
"MouseMoveEvent 237 167 0 0 0 0 0\n"
"MouseMoveEvent 235 167 0 0 0 0 0\n"
"MouseMoveEvent 235 168 0 0 0 0 0\n"
"MouseMoveEvent 233 168 0 0 0 0 0\n"
"MouseMoveEvent 233 169 0 0 0 0 0\n"
"MouseMoveEvent 232 169 0 0 0 0 0\n"
"MouseMoveEvent 232 170 0 0 0 0 0\n"
"LeftButtonPressEvent 232 170 0 0 0 0 0\n"
"RenderEvent 232 170 0 0 0 0 0\n"
"MouseMoveEvent 232 171 0 0 0 0 0\n"
"RenderEvent 232 171 0 0 0 0 0\n"
"MouseMoveEvent 231 172 0 0 0 0 0\n"
"RenderEvent 231 172 0 0 0 0 0\n"
"MouseMoveEvent 227 172 0 0 0 0 0\n"
"RenderEvent 227 172 0 0 0 0 0\n"
"MouseMoveEvent 220 172 0 0 0 0 0\n"
"RenderEvent 220 172 0 0 0 0 0\n"
"MouseMoveEvent 218 172 0 0 0 0 0\n"
"RenderEvent 218 172 0 0 0 0 0\n"
"MouseMoveEvent 216 172 0 0 0 0 0\n"
"RenderEvent 216 172 0 0 0 0 0\n"
"MouseMoveEvent 214 172 0 0 0 0 0\n"
"RenderEvent 214 172 0 0 0 0 0\n"
"MouseMoveEvent 212 172 0 0 0 0 0\n"
"RenderEvent 212 172 0 0 0 0 0\n"
"MouseMoveEvent 210 172 0 0 0 0 0\n"
"RenderEvent 210 172 0 0 0 0 0\n"
"MouseMoveEvent 207 172 0 0 0 0 0\n"
"RenderEvent 207 172 0 0 0 0 0\n"
"MouseMoveEvent 206 172 0 0 0 0 0\n"
"RenderEvent 206 172 0 0 0 0 0\n"
"MouseMoveEvent 204 172 0 0 0 0 0\n"
"RenderEvent 204 172 0 0 0 0 0\n"
"MouseMoveEvent 203 173 0 0 0 0 0\n"
"RenderEvent 203 173 0 0 0 0 0\n"
"MouseMoveEvent 202 176 0 0 0 0 0\n"
"RenderEvent 202 176 0 0 0 0 0\n"
"MouseMoveEvent 200 179 0 0 0 0 0\n"
"RenderEvent 200 179 0 0 0 0 0\n"
"MouseMoveEvent 197 181 0 0 0 0 0\n"
"RenderEvent 197 181 0 0 0 0 0\n"
"MouseMoveEvent 196 183 0 0 0 0 0\n"
"RenderEvent 196 183 0 0 0 0 0\n"
"MouseMoveEvent 194 185 0 0 0 0 0\n"
"RenderEvent 194 185 0 0 0 0 0\n"
"MouseMoveEvent 190 187 0 0 0 0 0\n"
"RenderEvent 190 187 0 0 0 0 0\n"
"MouseMoveEvent 189 188 0 0 0 0 0\n"
"RenderEvent 189 188 0 0 0 0 0\n"
"MouseMoveEvent 187 189 0 0 0 0 0\n"
"RenderEvent 187 189 0 0 0 0 0\n"
"MouseMoveEvent 186 191 0 0 0 0 0\n"
"RenderEvent 186 191 0 0 0 0 0\n"
"MouseMoveEvent 184 193 0 0 0 0 0\n"
"RenderEvent 184 193 0 0 0 0 0\n"
"MouseMoveEvent 183 194 0 0 0 0 0\n"
"RenderEvent 183 194 0 0 0 0 0\n"
"LeftButtonReleaseEvent 183 194 0 0 0 0 0\n"
"RenderEvent 183 194 0 0 0 0 0\n"
"MouseMoveEvent 183 194 0 0 0 0 0\n"
"MouseMoveEvent 183 193 0 0 0 0 0\n"
"MouseMoveEvent 182 192 0 0 0 0 0\n"
"MouseMoveEvent 182 190 0 0 0 0 0\n"
"MouseMoveEvent 181 188 0 0 0 0 0\n"
"MouseMoveEvent 180 186 0 0 0 0 0\n"
"MouseMoveEvent 178 185 0 0 0 0 0\n"
"MouseMoveEvent 176 182 0 0 0 0 0\n"
"MouseMoveEvent 174 180 0 0 0 0 0\n"
"MouseMoveEvent 172 178 0 0 0 0 0\n"
"MouseMoveEvent 170 174 0 0 0 0 0\n"
"MouseMoveEvent 168 172 0 0 0 0 0\n"
"MouseMoveEvent 167 171 0 0 0 0 0\n"
"MouseMoveEvent 165 168 0 0 0 0 0\n"
"MouseMoveEvent 165 166 0 0 0 0 0\n"
"MouseMoveEvent 164 165 0 0 0 0 0\n"
"MouseMoveEvent 163 162 0 0 0 0 0\n"
"MouseMoveEvent 161 159 0 0 0 0 0\n"
"MouseMoveEvent 160 157 0 0 0 0 0\n"
"MouseMoveEvent 158 155 0 0 0 0 0\n"
"MouseMoveEvent 156 153 0 0 0 0 0\n"
"MouseMoveEvent 154 152 0 0 0 0 0\n"
"MouseMoveEvent 154 154 0 0 0 0 0\n"
"MouseMoveEvent 154 155 0 0 0 0 0\n"
"MouseMoveEvent 154 157 0 0 0 0 0\n"
"MouseMoveEvent 155 159 0 0 0 0 0\n"
"MouseMoveEvent 157 161 0 0 0 0 0\n"
"MouseMoveEvent 157 163 0 0 0 0 0\n"
"MouseMoveEvent 157 164 0 0 0 0 0\n"
"MouseMoveEvent 157 165 0 0 0 0 0\n"
"MouseMoveEvent 156 166 0 0 0 0 0\n"
"MouseMoveEvent 155 167 0 0 0 0 0\n"
"MouseMoveEvent 153 168 0 0 0 0 0\n"
"MouseMoveEvent 153 169 0 0 0 0 0\n"
"MouseMoveEvent 152 170 0 0 0 0 0\n"
"MouseMoveEvent 150 171 0 0 0 0 0\n"
"MouseMoveEvent 150 172 0 0 0 0 0\n"
"MouseMoveEvent 148 172 0 0 0 0 0\n"
"MouseMoveEvent 147 173 0 0 0 0 0\n"
"MouseMoveEvent 146 174 0 0 0 0 0\n"
"MouseMoveEvent 145 174 0 0 0 0 0\n"
"MouseMoveEvent 144 175 0 0 0 0 0\n"
"MouseMoveEvent 143 176 0 0 0 0 0\n"
"MouseMoveEvent 142 176 0 0 0 0 0\n"
"MouseMoveEvent 141 177 0 0 0 0 0\n"
"MouseMoveEvent 141 179 0 0 0 0 0\n"
"MouseMoveEvent 141 178 0 0 0 0 0\n"
"MouseMoveEvent 141 177 0 0 0 0 0\n"
"MouseMoveEvent 141 176 0 0 0 0 0\n"
"MouseMoveEvent 142 175 0 0 0 0 0\n"
"MouseMoveEvent 143 175 0 0 0 0 0\n"
"LeftButtonPressEvent 143 175 0 0 0 0 0\n"
"RenderEvent 143 175 0 0 0 0 0\n"
"MouseMoveEvent 143 174 0 0 0 0 0\n"
"RenderEvent 143 174 0 0 0 0 0\n"
"MouseMoveEvent 143 173 0 0 0 0 0\n"
"RenderEvent 143 173 0 0 0 0 0\n"
"MouseMoveEvent 142 172 0 0 0 0 0\n"
"RenderEvent 142 172 0 0 0 0 0\n"
"MouseMoveEvent 142 171 0 0 0 0 0\n"
"RenderEvent 142 171 0 0 0 0 0\n"
"MouseMoveEvent 141 170 0 0 0 0 0\n"
"RenderEvent 141 170 0 0 0 0 0\n"
"MouseMoveEvent 141 169 0 0 0 0 0\n"
"RenderEvent 141 169 0 0 0 0 0\n"
"MouseMoveEvent 140 169 0 0 0 0 0\n"
"RenderEvent 140 169 0 0 0 0 0\n"
"MouseMoveEvent 138 169 0 0 0 0 0\n"
"RenderEvent 138 169 0 0 0 0 0\n"
"MouseMoveEvent 138 168 0 0 0 0 0\n"
"RenderEvent 138 168 0 0 0 0 0\n"
"MouseMoveEvent 136 167 0 0 0 0 0\n"
"RenderEvent 136 167 0 0 0 0 0\n"
"MouseMoveEvent 135 167 0 0 0 0 0\n"
"RenderEvent 135 167 0 0 0 0 0\n"
"MouseMoveEvent 134 167 0 0 0 0 0\n"
"RenderEvent 134 167 0 0 0 0 0\n"
"MouseMoveEvent 133 167 0 0 0 0 0\n"
"RenderEvent 133 167 0 0 0 0 0\n"
"MouseMoveEvent 132 167 0 0 0 0 0\n"
"RenderEvent 132 167 0 0 0 0 0\n"
"MouseMoveEvent 132 166 0 0 0 0 0\n"
"RenderEvent 132 166 0 0 0 0 0\n"
"MouseMoveEvent 131 166 0 0 0 0 0\n"
"RenderEvent 131 166 0 0 0 0 0\n"
"MouseMoveEvent 131 165 0 0 0 0 0\n"
"RenderEvent 131 165 0 0 0 0 0\n"
"MouseMoveEvent 131 164 0 0 0 0 0\n"
"RenderEvent 131 164 0 0 0 0 0\n"
"MouseMoveEvent 131 163 0 0 0 0 0\n"
"RenderEvent 131 163 0 0 0 0 0\n"
"MouseMoveEvent 131 162 0 0 0 0 0\n"
"RenderEvent 131 162 0 0 0 0 0\n"
"MouseMoveEvent 130 162 0 0 0 0 0\n"
"RenderEvent 130 162 0 0 0 0 0\n"
"MouseMoveEvent 130 161 0 0 0 0 0\n"
"RenderEvent 130 161 0 0 0 0 0\n"
"LeftButtonReleaseEvent 130 161 0 0 0 0 0\n"
"RenderEvent 130 161 0 0 0 0 0\n"
"MouseMoveEvent 130 161 0 0 0 0 0\n"
"MouseMoveEvent 131 161 0 0 0 0 0\n"
"MouseMoveEvent 132 161 0 0 0 0 0\n"
"MouseMoveEvent 132 160 0 0 0 0 0\n"
"MouseMoveEvent 133 159 0 0 0 0 0\n"
"MouseMoveEvent 134 159 0 0 0 0 0\n"
"MouseMoveEvent 135 158 0 0 0 0 0\n"
"MouseMoveEvent 137 157 0 0 0 0 0\n"
"MouseMoveEvent 137 156 0 0 0 0 0\n"
"MouseMoveEvent 138 156 0 0 0 0 0\n"
"MouseMoveEvent 138 155 0 0 0 0 0\n"
"MouseMoveEvent 138 154 0 0 0 0 0\n"
"MouseMoveEvent 139 153 0 0 0 0 0\n"
"MouseMoveEvent 140 153 0 0 0 0 0\n"
"MouseMoveEvent 140 152 0 0 0 0 0\n"
"MouseMoveEvent 140 151 0 0 0 0 0\n"
"MouseMoveEvent 140 150 0 0 0 0 0\n"
"MouseMoveEvent 141 150 0 0 0 0 0\n"
"MouseMoveEvent 142 150 0 0 0 0 0\n"
"MouseMoveEvent 143 150 0 0 0 0 0\n"
"MouseMoveEvent 144 150 0 0 0 0 0\n"
"MouseMoveEvent 145 150 0 0 0 0 0\n"
"MouseMoveEvent 146 150 0 0 0 0 0\n"
"MouseMoveEvent 147 149 0 0 0 0 0\n"
"LeftButtonPressEvent 147 149 0 0 0 0 0\n"
"RenderEvent 147 149 0 0 0 0 0\n"
"MouseMoveEvent 150 149 0 0 0 0 0\n"
"RenderEvent 150 149 0 0 0 0 0\n"
"MouseMoveEvent 153 147 0 0 0 0 0\n"
"RenderEvent 153 147 0 0 0 0 0\n"
"MouseMoveEvent 156 145 0 0 0 0 0\n"
"RenderEvent 156 145 0 0 0 0 0\n"
"MouseMoveEvent 157 145 0 0 0 0 0\n"
"RenderEvent 157 145 0 0 0 0 0\n"
"MouseMoveEvent 158 145 0 0 0 0 0\n"
"RenderEvent 158 145 0 0 0 0 0\n"
"MouseMoveEvent 160 145 0 0 0 0 0\n"
"RenderEvent 160 145 0 0 0 0 0\n"
"MouseMoveEvent 162 144 0 0 0 0 0\n"
"RenderEvent 162 144 0 0 0 0 0\n"
"MouseMoveEvent 163 144 0 0 0 0 0\n"
"RenderEvent 163 144 0 0 0 0 0\n"
"MouseMoveEvent 166 143 0 0 0 0 0\n"
"RenderEvent 166 143 0 0 0 0 0\n"
"MouseMoveEvent 170 143 0 0 0 0 0\n"
"RenderEvent 170 143 0 0 0 0 0\n"
"MouseMoveEvent 174 144 0 0 0 0 0\n"
"RenderEvent 174 144 0 0 0 0 0\n"
"MouseMoveEvent 178 146 0 0 0 0 0\n"
"RenderEvent 178 146 0 0 0 0 0\n"
"MouseMoveEvent 179 146 0 0 0 0 0\n"
"RenderEvent 179 146 0 0 0 0 0\n"
"MouseMoveEvent 180 147 0 0 0 0 0\n"
"RenderEvent 180 147 0 0 0 0 0\n"
"LeftButtonReleaseEvent 180 147 0 0 0 0 0\n"
"RenderEvent 180 147 0 0 0 0 0\n"
"MouseMoveEvent 180 147 0 0 0 0 0\n"
"MouseMoveEvent 178 148 0 0 0 0 0\n"
"MouseMoveEvent 177 149 0 0 0 0 0\n"
"MouseMoveEvent 176 149 0 0 0 0 0\n"
"MouseMoveEvent 176 150 0 0 0 0 0\n"
"MouseMoveEvent 174 151 0 0 0 0 0\n"
"MouseMoveEvent 173 152 0 0 0 0 0\n"
"MouseMoveEvent 171 153 0 0 0 0 0\n"
"MouseMoveEvent 170 154 0 0 0 0 0\n"
"MouseMoveEvent 169 156 0 0 0 0 0\n"
"MouseMoveEvent 168 156 0 0 0 0 0\n"
"MouseMoveEvent 167 158 0 0 0 0 0\n"
"MouseMoveEvent 166 158 0 0 0 0 0\n"
"MouseMoveEvent 164 160 0 0 0 0 0\n"
"MouseMoveEvent 163 161 0 0 0 0 0\n"
"MouseMoveEvent 162 162 0 0 0 0 0\n"
"MouseMoveEvent 161 162 0 0 0 0 0\n"
"MouseMoveEvent 161 163 0 0 0 0 0\n"
"MouseMoveEvent 161 162 0 0 0 0 0\n"
"MouseMoveEvent 161 161 0 0 0 0 0\n"
"MouseMoveEvent 161 159 0 0 0 0 0\n"
"MouseMoveEvent 161 158 0 0 0 0 0\n"
"LeftButtonPressEvent 161 158 0 0 0 0 0\n"
"RenderEvent 161 158 0 0 0 0 0\n"
"MouseMoveEvent 162 156 0 0 0 0 0\n"
"RenderEvent 162 156 0 0 0 0 0\n"
"MouseMoveEvent 166 154 0 0 0 0 0\n"
"RenderEvent 166 154 0 0 0 0 0\n"
"MouseMoveEvent 168 154 0 0 0 0 0\n"
"RenderEvent 168 154 0 0 0 0 0\n"
"MouseMoveEvent 170 153 0 0 0 0 0\n"
"RenderEvent 170 153 0 0 0 0 0\n"
"MouseMoveEvent 171 153 0 0 0 0 0\n"
"RenderEvent 171 153 0 0 0 0 0\n"
"MouseMoveEvent 171 152 0 0 0 0 0\n"
"RenderEvent 171 152 0 0 0 0 0\n"
"MouseMoveEvent 171 151 0 0 0 0 0\n"
"RenderEvent 171 151 0 0 0 0 0\n"
"MouseMoveEvent 173 151 0 0 0 0 0\n"
"RenderEvent 173 151 0 0 0 0 0\n"
"MouseMoveEvent 174 151 0 0 0 0 0\n"
"RenderEvent 174 151 0 0 0 0 0\n"
"MouseMoveEvent 174 149 0 0 0 0 0\n"
"RenderEvent 174 149 0 0 0 0 0\n"
"MouseMoveEvent 175 149 0 0 0 0 0\n"
"RenderEvent 175 149 0 0 0 0 0\n"
"LeftButtonReleaseEvent 175 149 0 0 0 0 0\n"
"RenderEvent 175 149 0 0 0 0 0\n"
"MouseMoveEvent 175 149 0 0 0 0 0\n"
"MouseMoveEvent 166 153 0 0 0 0 0\n"
"MouseMoveEvent 156 154 0 0 0 0 0\n"
"MouseMoveEvent 152 156 0 0 0 0 0\n"
"MouseMoveEvent 148 158 0 0 0 0 0\n"
"MouseMoveEvent 146 159 0 0 0 0 0\n"
"MouseMoveEvent 144 159 0 0 0 0 0\n"
"MouseMoveEvent 142 159 0 0 0 0 0\n"
"MouseMoveEvent 141 159 0 0 0 0 0\n"
"MouseMoveEvent 140 159 0 0 0 0 0\n"
"MouseMoveEvent 139 159 0 0 0 0 0\n"
"MouseMoveEvent 137 159 0 0 0 0 0\n"
"MouseMoveEvent 135 159 0 0 0 0 0\n"
"LeftButtonPressEvent 135 159 0 0 0 0 0\n"
"RenderEvent 135 159 0 0 0 0 0\n"
"MouseMoveEvent 139 162 0 0 0 0 0\n"
"RenderEvent 139 162 0 0 0 0 0\n"
"MouseMoveEvent 142 162 0 0 0 0 0\n"
"RenderEvent 142 162 0 0 0 0 0\n"
"MouseMoveEvent 143 163 0 0 0 0 0\n"
"RenderEvent 143 163 0 0 0 0 0\n"
"MouseMoveEvent 147 164 0 0 0 0 0\n"
"RenderEvent 147 164 0 0 0 0 0\n"
"MouseMoveEvent 149 164 0 0 0 0 0\n"
"RenderEvent 149 164 0 0 0 0 0\n"
"MouseMoveEvent 149 165 0 0 0 0 0\n"
"RenderEvent 149 165 0 0 0 0 0\n"
"MouseMoveEvent 149 166 0 0 0 0 0\n"
"RenderEvent 149 166 0 0 0 0 0\n"
"MouseMoveEvent 149 167 0 0 0 0 0\n"
"RenderEvent 149 167 0 0 0 0 0\n"
"MouseMoveEvent 150 167 0 0 0 0 0\n"
"RenderEvent 150 167 0 0 0 0 0\n"
"MouseMoveEvent 150 168 0 0 0 0 0\n"
"RenderEvent 150 168 0 0 0 0 0\n"
"MouseMoveEvent 150 169 0 0 0 0 0\n"
"RenderEvent 150 169 0 0 0 0 0\n"
"MouseMoveEvent 151 169 0 0 0 0 0\n"
"RenderEvent 151 169 0 0 0 0 0\n"
"MouseMoveEvent 152 170 0 0 0 0 0\n"
"RenderEvent 152 170 0 0 0 0 0\n"
"MouseMoveEvent 153 170 0 0 0 0 0\n"
"RenderEvent 153 170 0 0 0 0 0\n"
"MouseMoveEvent 155 170 0 0 0 0 0\n"
"RenderEvent 155 170 0 0 0 0 0\n"
"MouseMoveEvent 155 171 0 0 0 0 0\n"
"RenderEvent 155 171 0 0 0 0 0\n"
"MouseMoveEvent 156 172 0 0 0 0 0\n"
"RenderEvent 156 172 0 0 0 0 0\n"
"MouseMoveEvent 157 173 0 0 0 0 0\n"
"RenderEvent 157 173 0 0 0 0 0\n"
"MouseMoveEvent 159 174 0 0 0 0 0\n"
"RenderEvent 159 174 0 0 0 0 0\n"
"MouseMoveEvent 160 174 0 0 0 0 0\n"
"RenderEvent 160 174 0 0 0 0 0\n"
"LeftButtonReleaseEvent 160 174 0 0 0 0 0\n"
"RenderEvent 160 174 0 0 0 0 0\n"
"MouseMoveEvent 160 174 0 0 0 0 0\n"
"MouseMoveEvent 161 174 0 0 0 0 0\n"
"MouseMoveEvent 163 174 0 0 0 0 0\n"
"MouseMoveEvent 166 175 0 0 0 0 0\n"
"MouseMoveEvent 168 175 0 0 0 0 0\n"
"MouseMoveEvent 171 175 0 0 0 0 0\n"
"MouseMoveEvent 176 175 0 0 0 0 0\n"
"MouseMoveEvent 180 175 0 0 0 0 0\n"
"MouseMoveEvent 183 175 0 0 0 0 0\n"
"MouseMoveEvent 188 176 0 0 0 0 0\n"
"MouseMoveEvent 192 176 0 0 0 0 0\n"
"MouseMoveEvent 197 176 0 0 0 0 0\n"
"MouseMoveEvent 200 176 0 0 0 0 0\n"
"MouseMoveEvent 205 176 0 0 0 0 0\n"
"MouseMoveEvent 208 175 0 0 0 0 0\n"
"MouseMoveEvent 211 175 0 0 0 0 0\n"
"MouseMoveEvent 212 175 0 0 0 0 0\n"
"MouseMoveEvent 213 175 0 0 0 0 0\n"
"MouseMoveEvent 214 175 0 0 0 0 0\n"
"MouseMoveEvent 215 175 0 0 0 0 0\n"
"MouseMoveEvent 216 177 0 0 0 0 0\n"
"MouseMoveEvent 214 178 0 0 0 0 0\n"
"MouseMoveEvent 214 180 0 0 0 0 0\n"
"MouseMoveEvent 215 179 0 0 0 0 0\n"
"MouseMoveEvent 216 179 0 0 0 0 0\n"
"MouseMoveEvent 217 179 0 0 0 0 0\n"
"MouseMoveEvent 218 180 0 0 0 0 0\n"
"MouseMoveEvent 219 181 0 0 0 0 0\n"
"MouseMoveEvent 222 181 0 0 0 0 0\n"
"MouseMoveEvent 223 181 0 0 0 0 0\n"
"MouseMoveEvent 224 182 0 0 0 0 0\n"
"MouseMoveEvent 225 184 0 0 0 0 0\n"
"MouseMoveEvent 226 185 0 0 0 0 0\n"
"MouseMoveEvent 226 186 0 0 0 0 0\n"
"MouseMoveEvent 226 187 0 0 0 0 0\n"
"MouseMoveEvent 226 188 0 0 0 0 0\n"
"MouseMoveEvent 227 190 0 0 0 0 0\n"
"MouseMoveEvent 228 191 0 0 0 0 0\n"
"MouseMoveEvent 228 192 0 0 0 0 0\n"
"MouseMoveEvent 228 193 0 0 0 0 0\n"
"MouseMoveEvent 228 194 0 0 0 0 0\n"
"LeftButtonPressEvent 228 194 0 0 0 0 0\n"
"RenderEvent 228 194 0 0 0 0 0\n"
"MouseMoveEvent 228 193 0 0 0 0 0\n"
"RenderEvent 228 193 0 0 0 0 0\n"
"MouseMoveEvent 228 191 0 0 0 0 0\n"
"RenderEvent 228 191 0 0 0 0 0\n"
"MouseMoveEvent 228 185 0 0 0 0 0\n"
"RenderEvent 228 185 0 0 0 0 0\n"
"MouseMoveEvent 228 183 0 0 0 0 0\n"
"RenderEvent 228 183 0 0 0 0 0\n"
"MouseMoveEvent 228 182 0 0 0 0 0\n"
"RenderEvent 228 182 0 0 0 0 0\n"
"MouseMoveEvent 227 179 0 0 0 0 0\n"
"RenderEvent 227 179 0 0 0 0 0\n"
"MouseMoveEvent 226 178 0 0 0 0 0\n"
"RenderEvent 226 178 0 0 0 0 0\n"
"MouseMoveEvent 223 176 0 0 0 0 0\n"
"RenderEvent 223 176 0 0 0 0 0\n"
"MouseMoveEvent 221 174 0 0 0 0 0\n"
"RenderEvent 221 174 0 0 0 0 0\n"
"MouseMoveEvent 219 173 0 0 0 0 0\n"
"RenderEvent 219 173 0 0 0 0 0\n"
"MouseMoveEvent 218 172 0 0 0 0 0\n"
"RenderEvent 218 172 0 0 0 0 0\n"
"MouseMoveEvent 214 168 0 0 0 0 0\n"
"RenderEvent 214 168 0 0 0 0 0\n"
"MouseMoveEvent 213 168 0 0 0 0 0\n"
"RenderEvent 213 168 0 0 0 0 0\n"
"MouseMoveEvent 213 167 0 0 0 0 0\n"
"RenderEvent 213 167 0 0 0 0 0\n"
"MouseMoveEvent 211 167 0 0 0 0 0\n"
"RenderEvent 211 167 0 0 0 0 0\n"
"MouseMoveEvent 209 166 0 0 0 0 0\n"
"RenderEvent 209 166 0 0 0 0 0\n"
"MouseMoveEvent 208 166 0 0 0 0 0\n"
"RenderEvent 208 166 0 0 0 0 0\n"
"MouseMoveEvent 207 165 0 0 0 0 0\n"
"RenderEvent 207 165 0 0 0 0 0\n"
"MouseMoveEvent 206 164 0 0 0 0 0\n"
"RenderEvent 206 164 0 0 0 0 0\n"
"MouseMoveEvent 205 164 0 0 0 0 0\n"
"RenderEvent 205 164 0 0 0 0 0\n"
"MouseMoveEvent 204 164 0 0 0 0 0\n"
"RenderEvent 204 164 0 0 0 0 0\n"
"MouseMoveEvent 204 163 0 0 0 0 0\n"
"RenderEvent 204 163 0 0 0 0 0\n"
"MouseMoveEvent 203 163 0 0 0 0 0\n"
"RenderEvent 203 163 0 0 0 0 0\n"
"LeftButtonReleaseEvent 203 163 0 0 0 0 0\n"
"RenderEvent 203 163 0 0 0 0 0\n"
"MouseMoveEvent 203 163 0 0 0 0 0\n"
"MouseMoveEvent 202 163 0 0 0 0 0\n"
"MouseMoveEvent 198 163 0 0 0 0 0\n"
"MouseMoveEvent 195 165 0 0 0 0 0\n"
"MouseMoveEvent 190 166 0 0 0 0 0\n"
"MouseMoveEvent 184 170 0 0 0 0 0\n"
"MouseMoveEvent 182 171 0 0 0 0 0\n"
"MouseMoveEvent 180 172 0 0 0 0 0\n"
"MouseMoveEvent 177 172 0 0 0 0 0\n"
"MouseMoveEvent 172 173 0 0 0 0 0\n"
"MouseMoveEvent 168 174 0 0 0 0 0\n"
"MouseMoveEvent 165 174 0 0 0 0 0\n"
"MouseMoveEvent 164 175 0 0 0 0 0\n"
"MouseMoveEvent 162 175 0 0 0 0 0\n"
"MouseMoveEvent 160 176 0 0 0 0 0\n"
"MouseMoveEvent 158 176 0 0 0 0 0\n"
"MouseMoveEvent 157 176 0 0 0 0 0\n"
"LeftButtonPressEvent 157 176 0 0 0 0 0\n"
"RenderEvent 157 176 0 0 0 0 0\n"
"MouseMoveEvent 156 174 0 0 0 0 0\n"
"RenderEvent 156 174 0 0 0 0 0\n"
"MouseMoveEvent 152 173 0 0 0 0 0\n"
"RenderEvent 152 173 0 0 0 0 0\n"
"MouseMoveEvent 151 173 0 0 0 0 0\n"
"RenderEvent 151 173 0 0 0 0 0\n"
"MouseMoveEvent 150 173 0 0 0 0 0\n"
"RenderEvent 150 173 0 0 0 0 0\n"
"MouseMoveEvent 149 173 0 0 0 0 0\n"
"RenderEvent 149 173 0 0 0 0 0\n"
"MouseMoveEvent 148 172 0 0 0 0 0\n"
"RenderEvent 148 172 0 0 0 0 0\n"
"MouseMoveEvent 147 172 0 0 0 0 0\n"
"RenderEvent 147 172 0 0 0 0 0\n"
"MouseMoveEvent 147 170 0 0 0 0 0\n"
"RenderEvent 147 170 0 0 0 0 0\n"
"MouseMoveEvent 147 168 0 0 0 0 0\n"
"RenderEvent 147 168 0 0 0 0 0\n"
"MouseMoveEvent 146 167 0 0 0 0 0\n"
"RenderEvent 146 167 0 0 0 0 0\n"
"MouseMoveEvent 146 166 0 0 0 0 0\n"
"RenderEvent 146 166 0 0 0 0 0\n"
"MouseMoveEvent 145 165 0 0 0 0 0\n"
"RenderEvent 145 165 0 0 0 0 0\n"
"MouseMoveEvent 144 164 0 0 0 0 0\n"
"RenderEvent 144 164 0 0 0 0 0\n"
"MouseMoveEvent 142 164 0 0 0 0 0\n"
"RenderEvent 142 164 0 0 0 0 0\n"
"MouseMoveEvent 142 163 0 0 0 0 0\n"
"RenderEvent 142 163 0 0 0 0 0\n"
"MouseMoveEvent 141 163 0 0 0 0 0\n"
"RenderEvent 141 163 0 0 0 0 0\n"
"LeftButtonReleaseEvent 141 163 0 0 0 0 0\n"
"RenderEvent 141 163 0 0 0 0 0\n"
"MouseMoveEvent 141 163 0 0 0 0 0\n"
"MouseMoveEvent 141 162 0 0 0 0 0\n"
"MouseMoveEvent 141 164 0 0 0 0 0\n"
"KeyPressEvent 141 164 0 0 113 1 q\n"
"CharEvent 141 164 0 0 113 1 q\n"
"ExitEvent 141 164 0 0 113 1 q\n"
;

// -----------------------------------------------------------------------
int TestHandleWidget( int argc, char *argv[] )
{
  // Create a mace out of filters.
  //
  vtkSphereSource *sphere = vtkSphereSource::New();
  vtkConeSource *cone = vtkConeSource::New();
  vtkGlyph3D *glyph = vtkGlyph3D::New();
  glyph->SetInputConnection(sphere->GetOutputPort());
  glyph->SetSource(cone->GetOutput());
  glyph->SetVectorModeToUseNormal();
  glyph->SetScaleModeToScaleByVector();
  glyph->SetScaleFactor(0.25);

  // The sphere and spikes are appended into a single polydata. 
  // This just makes things simpler to manage.
  vtkAppendPolyData *apd = vtkAppendPolyData::New();
  apd->AddInput(glyph->GetOutput());
  apd->AddInput(sphere->GetOutput());

  // This portion of the code clips the mace with the vtkPlanes 
  // implicit function. The cut region is colored green.
  vtkTIPW3Callback *myCallback = vtkTIPW3Callback::New();
  vtkCutter *cutter = vtkCutter::New();
  cutter->SetInputConnection(apd->GetOutputPort());
  cutter->SetCutFunction( myCallback->Plane );

  vtkPolyDataMapper *selectMapper = vtkPolyDataMapper::New();
  selectMapper->SetInputConnection(cutter->GetOutputPort());

  vtkLODActor *selectActor = vtkLODActor::New();
  selectActor->SetMapper(selectMapper);
  selectActor->GetProperty()->SetColor(0,1,0);
  selectActor->VisibilityOff();
  selectActor->SetScale(1.01, 1.01, 1.01);  
  
  vtkOutlineFilter* outline = vtkOutlineFilter::New();
    outline->SetInputConnection(apd->GetOutputPort());
  vtkPolyDataMapper* outlineMapper = vtkPolyDataMapper::New();
    outlineMapper->SetInputConnection(outline->GetOutputPort());
  vtkActor* outlineActor =  vtkActor::New();
    outlineActor->SetMapper( outlineMapper);

  vtkImplicitPlaneRepresentation *rep = vtkImplicitPlaneRepresentation::New();
  rep->SetPlaceFactor(0.7);
  rep->GetPlaneProperty()->SetAmbientColor(0.0, 0.5, 0.5);
  rep->GetPlaneProperty()->SetOpacity(0.3);
  rep->PlaceWidget(outline->GetOutput()->GetBounds());  
  vtkImplicitPlaneWidget2 *planeWidget = vtkImplicitPlaneWidget2::New();
  planeWidget->SetRepresentation(rep);

  myCallback->Actor = selectActor;

  planeWidget->AddObserver(vtkCommand::InteractionEvent,myCallback);
  

  // Create the RenderWindow, Renderer and both Actors
  //
  vtkRenderer *ren1 = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(ren1);

  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  // VTK widgets consist of two parts: the widget part that handles event processing;
  // and the widget representation that defines how the widget appears in the scene 
  // (i.e., matters pertaining to geometry).
  vtkPointHandleRepresentation3D *handleRep 
    = vtkPointHandleRepresentation3D::New();
  handleRep->SetPlaceFactor(2.5);
  handleRep->PlaceWidget(outlineActor->GetBounds());
  handleRep->SetHandleSize(10);

  vtkHandleWidget *handleWidget = vtkHandleWidget::New();
  handleWidget->SetInteractor(iren);
  planeWidget->SetInteractor(iren);
  handleWidget->SetRepresentation(handleRep);

  ren1->AddActor(selectActor);
  ren1->AddActor(outlineActor);

  // Add the actors to the renderer, set the background and size
  //

  // record events
  vtkInteractorEventRecorder *recorder = vtkInteractorEventRecorder::New();
  recorder->SetInteractor(iren);
//  recorder->SetFileName("c:/record.log");
//  recorder->Record();
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(HandleWidgetLog); 
  recorder->EnabledOn();
  
  // Should we constrain the handles to the oblique plane ?
  bool constrainHandlesToObliquePlane = false;
  for (int i = 0; i < argc; i++)
    {
    if (strcmp("-ConstrainHandlesToPlane", argv[i]) == 0)
      {
      constrainHandlesToObliquePlane = true;
      break;
      }
    }

  // Set some defaults.
  //
  rep->SetNormal(0.942174, 0.25322, 0.219519);
  double worldPos[3] = {-0.0417953, 0.202206, -0.0538641};
  handleRep->SetWorldPosition(worldPos);
  rep->GetPlane(myCallback->Plane);
  
  if (constrainHandlesToObliquePlane)
    {
    vtkBoundedPlanePointPlacer *placer = vtkBoundedPlanePointPlacer::New();

    // Define the the plane as the image plane widget's plane
    placer->SetProjectionNormalToOblique();
    placer->SetObliquePlane(myCallback->Plane);

    // Also add bounding planes for the bounds of the dataset.
    double bounds[6];
    outline->GetOutput()->GetBounds(bounds);
    vtkPlane *plane;
    plane = vtkPlane::New();
    plane->SetOrigin( bounds[0], bounds[2], bounds[4] );
    plane->SetNormal( 1.0, 0.0, 0.0 );
    placer->AddBoundingPlane( plane );
    plane->Delete();
    
    plane = vtkPlane::New();
    plane->SetOrigin( bounds[1], bounds[3], bounds[5] );
    plane->SetNormal( -1.0, 0.0, 0.0 );
    placer->AddBoundingPlane( plane );
    plane->Delete();
      
    plane = vtkPlane::New();
    plane->SetOrigin( bounds[0], bounds[2], bounds[4] );
    plane->SetNormal( 0.0, 1.0, 0.0 );
    placer->AddBoundingPlane( plane );
    plane->Delete();
    
    plane = vtkPlane::New();
    plane->SetOrigin( bounds[1], bounds[3], bounds[5] );
    plane->SetNormal( 0.0, -1.0, 0.0 );
    placer->AddBoundingPlane( plane );
    plane->Delete();
    
    plane = vtkPlane::New();
    plane->SetOrigin( bounds[0], bounds[2], bounds[4] );
    plane->SetNormal( 0.0, 0.0, 1.0 );
    placer->AddBoundingPlane( plane );
    plane->Delete();
    
    plane = vtkPlane::New();
    plane->SetOrigin( bounds[1], bounds[3], bounds[5] );
    plane->SetNormal( 0.0, 0.0, -1.0 );
    placer->AddBoundingPlane( plane );
    plane->Delete();

    handleRep->SetPointPlacer(placer);
    placer->Delete();

    }

  iren->Initialize();
  renWin->Render();
  handleWidget->EnabledOn();
  planeWidget->EnabledOn();
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);
  ren1->ResetCamera();
  ren1->ResetCameraClippingRange();
  renWin->Render();

  recorder->Play();
  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  recorder->Off();
  recorder->Delete();
  apd->Delete();
  planeWidget->Delete();
  glyph->Delete();
  sphere->Delete();
  cone->Delete();
  rep->Delete();
  myCallback->Delete();
  selectMapper->Delete();
  selectActor->Delete();
  cutter->Delete();
  outline->Delete();
  outlineMapper->Delete();
  outlineActor->Delete();
  handleWidget->Delete();
  handleRep->Delete();
  iren->Delete();
  renWin->Delete();
  ren1->Delete();

  return !retVal;

}

