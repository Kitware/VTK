/*=========================================================================

  Program:   Visualization Toolkit
  Module:    BoxWidget2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSmartPointer.h"
#include "vtkAppendPolyData.h"
#include "vtkBoxWidget2.h"
#include "vtkBoxRepresentation.h"
#include "vtkCommand.h"
#include "vtkConeSource.h"
#include "vtkGlyph3D.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTransform.h"

// Callback for the interaction
class vtkBWCallback2 : public vtkCommand
{
public:
  static vtkBWCallback2 *New() 
  { return new vtkBWCallback2; }
  virtual void Execute(vtkObject *caller, unsigned long, void*)
  {
    vtkBoxWidget2 *boxWidget = 
      reinterpret_cast<vtkBoxWidget2*>(caller);
    vtkBoxRepresentation *boxRep = 
      reinterpret_cast<vtkBoxRepresentation*>(boxWidget->GetRepresentation());
    boxRep->GetTransform(this->Transform);
    this->Actor->SetUserTransform(this->Transform);
  }
  vtkBWCallback2():Transform(0),Actor(0) {}
  vtkTransform *Transform;
  vtkActor     *Actor;
};

char BoxWidgetEventLog2[] =
  "# StreamVersion 1\n"
  "CharEvent 187 242 0 0 105 1 i\n"
  "KeyReleaseEvent 187 242 0 0 105 1 i\n"
  "MouseMoveEvent 187 241 0 0 0 0 i\n"
  "MouseMoveEvent 186 241 0 0 0 0 i\n"
  "MouseMoveEvent 184 241 0 0 0 0 i\n"
  "MouseMoveEvent 182 242 0 0 0 0 i\n"
  "MouseMoveEvent 178 242 0 0 0 0 i\n"
  "MouseMoveEvent 177 243 0 0 0 0 i\n"
  "MouseMoveEvent 175 244 0 0 0 0 i\n"
  "MouseMoveEvent 174 244 0 0 0 0 i\n"
  "MouseMoveEvent 173 245 0 0 0 0 i\n"
  "MouseMoveEvent 172 245 0 0 0 0 i\n"
  "MouseMoveEvent 171 245 0 0 0 0 i\n"
  "MouseMoveEvent 170 246 0 0 0 0 i\n"
  "MouseMoveEvent 169 246 0 0 0 0 i\n"
  "MouseMoveEvent 168 246 0 0 0 0 i\n"
  "MouseMoveEvent 167 246 0 0 0 0 i\n"
  "MouseMoveEvent 166 246 0 0 0 0 i\n"
  "MouseMoveEvent 165 246 0 0 0 0 i\n"
  "MouseMoveEvent 164 246 0 0 0 0 i\n"
  "MouseMoveEvent 163 246 0 0 0 0 i\n"
  "MouseMoveEvent 162 246 0 0 0 0 i\n"
  "MouseMoveEvent 161 246 0 0 0 0 i\n"
  "MouseMoveEvent 160 247 0 0 0 0 i\n"
  "MouseMoveEvent 159 248 0 0 0 0 i\n"
  "MouseMoveEvent 158 248 0 0 0 0 i\n"
  "MouseMoveEvent 156 248 0 0 0 0 i\n"
  "MouseMoveEvent 155 249 0 0 0 0 i\n"
  "MouseMoveEvent 155 250 0 0 0 0 i\n"
  "MouseMoveEvent 155 251 0 0 0 0 i\n"
  "MouseMoveEvent 155 252 0 0 0 0 i\n"
  "MouseMoveEvent 154 252 0 0 0 0 i\n"
  "MouseMoveEvent 153 252 0 0 0 0 i\n"
  "MouseMoveEvent 153 253 0 0 0 0 i\n"
  "LeftButtonPressEvent 153 253 0 0 0 0 i\n"
  "MouseMoveEvent 153 252 0 0 0 0 i\n"
  "MouseMoveEvent 154 252 0 0 0 0 i\n"
  "MouseMoveEvent 155 251 0 0 0 0 i\n"
  "MouseMoveEvent 155 250 0 0 0 0 i\n"
  "MouseMoveEvent 155 249 0 0 0 0 i\n"
  "MouseMoveEvent 155 248 0 0 0 0 i\n"
  "MouseMoveEvent 155 247 0 0 0 0 i\n"
  "MouseMoveEvent 155 246 0 0 0 0 i\n"
  "MouseMoveEvent 155 245 0 0 0 0 i\n"
  "MouseMoveEvent 155 244 0 0 0 0 i\n"
  "MouseMoveEvent 155 243 0 0 0 0 i\n"
  "MouseMoveEvent 155 242 0 0 0 0 i\n"
  "MouseMoveEvent 155 241 0 0 0 0 i\n"
  "MouseMoveEvent 155 240 0 0 0 0 i\n"
  "MouseMoveEvent 155 239 0 0 0 0 i\n"
  "MouseMoveEvent 155 238 0 0 0 0 i\n"
  "MouseMoveEvent 155 237 0 0 0 0 i\n"
  "MouseMoveEvent 155 236 0 0 0 0 i\n"
  "MouseMoveEvent 155 235 0 0 0 0 i\n"
  "MouseMoveEvent 155 234 0 0 0 0 i\n"
  "MouseMoveEvent 155 232 0 0 0 0 i\n"
  "MouseMoveEvent 155 231 0 0 0 0 i\n"
  "MouseMoveEvent 155 230 0 0 0 0 i\n"
  "MouseMoveEvent 155 229 0 0 0 0 i\n"
  "MouseMoveEvent 155 228 0 0 0 0 i\n"
  "MouseMoveEvent 154 228 0 0 0 0 i\n"
  "LeftButtonReleaseEvent 154 228 0 0 0 0 i\n"
  "MouseMoveEvent 154 228 0 0 0 0 i\n"
  "MouseMoveEvent 154 227 0 0 0 0 i\n"
  "MouseMoveEvent 153 226 0 0 0 0 i\n"
  "MouseMoveEvent 151 226 0 0 0 0 i\n"
  "MouseMoveEvent 150 225 0 0 0 0 i\n"
  "MouseMoveEvent 148 223 0 0 0 0 i\n"
  "MouseMoveEvent 146 223 0 0 0 0 i\n"
  "MouseMoveEvent 144 222 0 0 0 0 i\n"
  "MouseMoveEvent 143 222 0 0 0 0 i\n"
  "MouseMoveEvent 143 221 0 0 0 0 i\n"
  "MouseMoveEvent 141 221 0 0 0 0 i\n"
  "MouseMoveEvent 137 219 0 0 0 0 i\n"
  "MouseMoveEvent 130 216 0 0 0 0 i\n"
  "MouseMoveEvent 120 212 0 0 0 0 i\n"
  "MouseMoveEvent 111 209 0 0 0 0 i\n"
  "MouseMoveEvent 100 204 0 0 0 0 i\n"
  "MouseMoveEvent 97 204 0 0 0 0 i\n"
  "MouseMoveEvent 96 202 0 0 0 0 i\n"
  "MouseMoveEvent 96 201 0 0 0 0 i\n"
  "MouseMoveEvent 96 200 0 0 0 0 i\n"
  "MouseMoveEvent 96 199 0 0 0 0 i\n"
  "MouseMoveEvent 96 198 0 0 0 0 i\n"
  "MouseMoveEvent 96 197 0 0 0 0 i\n"
  "MouseMoveEvent 96 196 0 0 0 0 i\n"
  "LeftButtonPressEvent 96 196 0 0 0 0 i\n"
  "MouseMoveEvent 96 195 0 0 0 0 i\n"
  "MouseMoveEvent 96 194 0 0 0 0 i\n"
  "MouseMoveEvent 96 193 0 0 0 0 i\n"
  "MouseMoveEvent 96 192 0 0 0 0 i\n"
  "MouseMoveEvent 96 191 0 0 0 0 i\n"
  "MouseMoveEvent 96 190 0 0 0 0 i\n"
  "MouseMoveEvent 96 189 0 0 0 0 i\n"
  "MouseMoveEvent 96 188 0 0 0 0 i\n"
  "MouseMoveEvent 97 188 0 0 0 0 i\n"
  "MouseMoveEvent 97 187 0 0 0 0 i\n"
  "MouseMoveEvent 98 186 0 0 0 0 i\n"
  "MouseMoveEvent 98 185 0 0 0 0 i\n"
  "MouseMoveEvent 99 185 0 0 0 0 i\n"
  "MouseMoveEvent 99 184 0 0 0 0 i\n"
  "MouseMoveEvent 99 182 0 0 0 0 i\n"
  "MouseMoveEvent 100 182 0 0 0 0 i\n"
  "MouseMoveEvent 101 179 0 0 0 0 i\n"
  "MouseMoveEvent 103 178 0 0 0 0 i\n"
  "MouseMoveEvent 106 177 0 0 0 0 i\n"
  "MouseMoveEvent 109 177 0 0 0 0 i\n"
  "MouseMoveEvent 112 176 0 0 0 0 i\n"
  "MouseMoveEvent 115 175 0 0 0 0 i\n"
  "MouseMoveEvent 117 174 0 0 0 0 i\n"
  "MouseMoveEvent 118 174 0 0 0 0 i\n"
  "MouseMoveEvent 119 174 0 0 0 0 i\n"
  "MouseMoveEvent 119 173 0 0 0 0 i\n"
  "MouseMoveEvent 120 173 0 0 0 0 i\n"
  "MouseMoveEvent 121 172 0 0 0 0 i\n"
  "MouseMoveEvent 121 171 0 0 0 0 i\n"
  "MouseMoveEvent 122 171 0 0 0 0 i\n"
  "MouseMoveEvent 123 171 0 0 0 0 i\n"
  "MouseMoveEvent 123 170 0 0 0 0 i\n"
  "MouseMoveEvent 124 170 0 0 0 0 i\n"
  "MouseMoveEvent 125 170 0 0 0 0 i\n"
  "MouseMoveEvent 125 169 0 0 0 0 i\n"
  "MouseMoveEvent 128 169 0 0 0 0 i\n"
  "MouseMoveEvent 129 168 0 0 0 0 i\n"
  "MouseMoveEvent 130 168 0 0 0 0 i\n"
  "MouseMoveEvent 132 168 0 0 0 0 i\n"
  "MouseMoveEvent 133 168 0 0 0 0 i\n"
  "MouseMoveEvent 135 167 0 0 0 0 i\n"
  "MouseMoveEvent 136 167 0 0 0 0 i\n"
  "MouseMoveEvent 137 167 0 0 0 0 i\n"
  "MouseMoveEvent 138 167 0 0 0 0 i\n"
  "MouseMoveEvent 140 168 0 0 0 0 i\n"
  "MouseMoveEvent 141 168 0 0 0 0 i\n"
  "MouseMoveEvent 142 168 0 0 0 0 i\n"
  "MouseMoveEvent 142 167 0 0 0 0 i\n"
  "MouseMoveEvent 143 167 0 0 0 0 i\n"
  "MouseMoveEvent 144 167 0 0 0 0 i\n"
  "MouseMoveEvent 145 166 0 0 0 0 i\n"
  "MouseMoveEvent 146 166 0 0 0 0 i\n"
  "MouseMoveEvent 147 166 0 0 0 0 i\n"
  "MouseMoveEvent 148 166 0 0 0 0 i\n"
  "MouseMoveEvent 149 166 0 0 0 0 i\n"
  "MouseMoveEvent 150 167 0 0 0 0 i\n"
  "MouseMoveEvent 151 167 0 0 0 0 i\n"
  "MouseMoveEvent 152 167 0 0 0 0 i\n"
  "MouseMoveEvent 152 168 0 0 0 0 i\n"
  "MouseMoveEvent 153 168 0 0 0 0 i\n"
  "MouseMoveEvent 153 167 0 0 0 0 i\n"
  "MouseMoveEvent 154 167 0 0 0 0 i\n"
  "MouseMoveEvent 155 167 0 0 0 0 i\n"
  "LeftButtonReleaseEvent 155 167 0 0 0 0 i\n"
  "MouseMoveEvent 155 167 0 0 0 0 i\n"
  "MouseMoveEvent 155 166 0 0 0 0 i\n"
  "MouseMoveEvent 156 166 0 0 0 0 i\n"
  "MouseMoveEvent 158 166 0 0 0 0 i\n"
  "MouseMoveEvent 159 167 0 0 0 0 i\n"
  "MouseMoveEvent 159 168 0 0 0 0 i\n"
  "MouseMoveEvent 159 169 0 0 0 0 i\n"
  "MouseMoveEvent 158 169 0 0 0 0 i\n"
  "MouseMoveEvent 158 170 0 0 0 0 i\n"
  "MouseMoveEvent 157 170 0 0 0 0 i\n"
  "MouseMoveEvent 157 171 0 0 0 0 i\n"
  "MouseMoveEvent 156 172 0 0 0 0 i\n"
  "RightButtonPressEvent 156 172 0 0 0 0 i\n"
  "MouseMoveEvent 156 173 0 0 0 0 i\n"
  "MouseMoveEvent 155 173 0 0 0 0 i\n"
  "MouseMoveEvent 153 173 0 0 0 0 i\n"
  "MouseMoveEvent 150 174 0 0 0 0 i\n"
  "MouseMoveEvent 150 176 0 0 0 0 i\n"
  "MouseMoveEvent 149 177 0 0 0 0 i\n"
  "MouseMoveEvent 148 178 0 0 0 0 i\n"
  "MouseMoveEvent 148 181 0 0 0 0 i\n"
  "MouseMoveEvent 147 184 0 0 0 0 i\n"
  "MouseMoveEvent 147 186 0 0 0 0 i\n"
  "MouseMoveEvent 147 187 0 0 0 0 i\n"
  "MouseMoveEvent 146 187 0 0 0 0 i\n"
  "MouseMoveEvent 146 188 0 0 0 0 i\n"
  "MouseMoveEvent 146 189 0 0 0 0 i\n"
  "MouseMoveEvent 145 190 0 0 0 0 i\n"
  "MouseMoveEvent 144 190 0 0 0 0 i\n"
  "MouseMoveEvent 143 192 0 0 0 0 i\n"
  "MouseMoveEvent 141 194 0 0 0 0 i\n"
  "MouseMoveEvent 140 194 0 0 0 0 i\n"
  "MouseMoveEvent 140 195 0 0 0 0 i\n"
  "MouseMoveEvent 139 195 0 0 0 0 i\n"
  "MouseMoveEvent 138 195 0 0 0 0 i\n"
  "MouseMoveEvent 137 196 0 0 0 0 i\n"
  "MouseMoveEvent 137 197 0 0 0 0 i\n"
  "MouseMoveEvent 137 198 0 0 0 0 i\n"
  "MouseMoveEvent 137 199 0 0 0 0 i\n"
  "MouseMoveEvent 137 200 0 0 0 0 i\n"
  "MouseMoveEvent 137 201 0 0 0 0 i\n"
  "MouseMoveEvent 138 202 0 0 0 0 i\n"
  "MouseMoveEvent 138 203 0 0 0 0 i\n"
  "MouseMoveEvent 139 203 0 0 0 0 i\n"
  "MouseMoveEvent 140 203 0 0 0 0 i\n"
  "MouseMoveEvent 141 202 0 0 0 0 i\n"
  "MouseMoveEvent 142 202 0 0 0 0 i\n"
  "MouseMoveEvent 145 203 0 0 0 0 i\n"
  "MouseMoveEvent 149 204 0 0 0 0 i\n"
  "MouseMoveEvent 150 205 0 0 0 0 i\n"
  "MouseMoveEvent 151 205 0 0 0 0 i\n"
  "RightButtonReleaseEvent 151 205 0 0 0 0 i\n"
  "MouseMoveEvent 151 205 0 0 0 0 i\n"
  "MouseMoveEvent 151 204 0 0 0 0 i\n"
  "MouseMoveEvent 150 204 0 0 0 0 i\n"
  "RightButtonPressEvent 150 204 0 0 0 0 i\n"
  "MouseMoveEvent 150 203 0 0 0 0 i\n"
  "MouseMoveEvent 150 202 0 0 0 0 i\n"
  "MouseMoveEvent 151 201 0 0 0 0 i\n"
  "MouseMoveEvent 152 201 0 0 0 0 i\n"
  "MouseMoveEvent 152 200 0 0 0 0 i\n"
  "MouseMoveEvent 153 200 0 0 0 0 i\n"
  "MouseMoveEvent 153 199 0 0 0 0 i\n"
  "MouseMoveEvent 153 198 0 0 0 0 i\n"
  "MouseMoveEvent 153 196 0 0 0 0 i\n"
  "MouseMoveEvent 153 195 0 0 0 0 i\n"
  "MouseMoveEvent 153 194 0 0 0 0 i\n"
  "MouseMoveEvent 153 191 0 0 0 0 i\n"
  "MouseMoveEvent 153 190 0 0 0 0 i\n"
  "MouseMoveEvent 153 189 0 0 0 0 i\n"
  "MouseMoveEvent 153 188 0 0 0 0 i\n"
  "MouseMoveEvent 153 187 0 0 0 0 i\n"
  "MouseMoveEvent 153 186 0 0 0 0 i\n"
  "MouseMoveEvent 153 185 0 0 0 0 i\n"
  "MouseMoveEvent 153 183 0 0 0 0 i\n"
  "MouseMoveEvent 153 182 0 0 0 0 i\n"
  "MouseMoveEvent 153 181 0 0 0 0 i\n"
  "MouseMoveEvent 153 180 0 0 0 0 i\n"
  "MouseMoveEvent 153 179 0 0 0 0 i\n"
  "MouseMoveEvent 153 178 0 0 0 0 i\n"
  "MouseMoveEvent 153 177 0 0 0 0 i\n"
  "MouseMoveEvent 153 176 0 0 0 0 i\n"
  "MouseMoveEvent 153 175 0 0 0 0 i\n"
  "MouseMoveEvent 153 176 0 0 0 0 i\n"
  "RightButtonReleaseEvent 153 176 0 0 0 0 i\n"
  "MouseMoveEvent 153 176 0 0 0 0 i\n"
  "MouseMoveEvent 154 176 0 0 0 0 i\n"
  "MouseMoveEvent 156 177 0 0 0 0 i\n"
  "MouseMoveEvent 156 178 0 0 0 0 i\n"
  "MouseMoveEvent 156 179 0 0 0 0 i\n"
  "MouseMoveEvent 156 180 0 0 0 0 i\n"
  "MouseMoveEvent 155 180 0 0 0 0 i\n"
  "MiddleButtonPressEvent 155 180 0 0 0 0 i\n"
  "MouseMoveEvent 154 180 0 0 0 0 i\n"
  "MouseMoveEvent 154 181 0 0 0 0 i\n"
  "MouseMoveEvent 153 181 0 0 0 0 i\n"
  "MouseMoveEvent 152 181 0 0 0 0 i\n"
  "MouseMoveEvent 151 181 0 0 0 0 i\n"
  "MouseMoveEvent 148 181 0 0 0 0 i\n"
  "MouseMoveEvent 141 182 0 0 0 0 i\n"
  "MouseMoveEvent 139 181 0 0 0 0 i\n"
  "MouseMoveEvent 139 182 0 0 0 0 i\n"
  "MouseMoveEvent 138 183 0 0 0 0 i\n"
  "MouseMoveEvent 138 184 0 0 0 0 i\n"
  "MouseMoveEvent 139 185 0 0 0 0 i\n"
  "MouseMoveEvent 140 186 0 0 0 0 i\n"
  "MouseMoveEvent 141 186 0 0 0 0 i\n"
  "MouseMoveEvent 143 186 0 0 0 0 i\n"
  "MouseMoveEvent 147 189 0 0 0 0 i\n"
  "MouseMoveEvent 151 189 0 0 0 0 i\n"
  "MouseMoveEvent 153 190 0 0 0 0 i\n"
  "MouseMoveEvent 154 190 0 0 0 0 i\n"
  "MouseMoveEvent 155 191 0 0 0 0 i\n"
  "MouseMoveEvent 156 192 0 0 0 0 i\n"
  "MouseMoveEvent 157 192 0 0 0 0 i\n"
  "MouseMoveEvent 157 193 0 0 0 0 i\n"
  "MouseMoveEvent 158 194 0 0 0 0 i\n"
  "MouseMoveEvent 159 194 0 0 0 0 i\n"
  "MouseMoveEvent 159 195 0 0 0 0 i\n"
  "MouseMoveEvent 160 195 0 0 0 0 i\n"
  "MouseMoveEvent 160 197 0 0 0 0 i\n"
  "MouseMoveEvent 162 198 0 0 0 0 i\n"
  "MouseMoveEvent 166 199 0 0 0 0 i\n"
  "MouseMoveEvent 167 200 0 0 0 0 i\n"
  "MouseMoveEvent 168 200 0 0 0 0 i\n"
  "MouseMoveEvent 168 201 0 0 0 0 i\n"
  "MouseMoveEvent 168 202 0 0 0 0 i\n"
  "MouseMoveEvent 168 203 0 0 0 0 i\n"
  "MouseMoveEvent 168 204 0 0 0 0 i\n"
  "MouseMoveEvent 168 205 0 0 0 0 i\n"
  "MouseMoveEvent 167 205 0 0 0 0 i\n"
  "MouseMoveEvent 167 206 0 0 0 0 i\n"
  "MouseMoveEvent 166 208 0 0 0 0 i\n"
  "MouseMoveEvent 165 209 0 0 0 0 i\n"
  "MouseMoveEvent 165 210 0 0 0 0 i\n"
  "MouseMoveEvent 164 210 0 0 0 0 i\n"
  "MouseMoveEvent 164 211 0 0 0 0 i\n"
  "MouseMoveEvent 165 211 0 0 0 0 i\n"
  "MouseMoveEvent 165 212 0 0 0 0 i\n"
  "MouseMoveEvent 166 212 0 0 0 0 i\n"
  "MouseMoveEvent 166 213 0 0 0 0 i\n"
  "MouseMoveEvent 166 214 0 0 0 0 i\n"
  "MouseMoveEvent 166 215 0 0 0 0 i\n"
  "MouseMoveEvent 166 216 0 0 0 0 i\n"
  "MouseMoveEvent 166 217 0 0 0 0 i\n"
  "MouseMoveEvent 167 218 0 0 0 0 i\n"
  "MouseMoveEvent 168 218 0 0 0 0 i\n"
  "MouseMoveEvent 168 219 0 0 0 0 i\n"
  "MouseMoveEvent 169 220 0 0 0 0 i\n"
  "MouseMoveEvent 169 221 0 0 0 0 i\n"
  "MouseMoveEvent 169 222 0 0 0 0 i\n"
  "MouseMoveEvent 169 223 0 0 0 0 i\n"
  "MouseMoveEvent 169 224 0 0 0 0 i\n"
  "MouseMoveEvent 169 223 0 0 0 0 i\n"
  "MouseMoveEvent 170 222 0 0 0 0 i\n"
  "MouseMoveEvent 170 221 0 0 0 0 i\n"
  "MiddleButtonReleaseEvent 170 221 0 0 0 0 i\n"
  "MouseMoveEvent 170 221 0 0 0 0 i\n"
  "MouseMoveEvent 170 220 0 0 0 0 i\n"
  "MouseMoveEvent 170 219 0 0 0 0 i\n"
  "MouseMoveEvent 170 218 0 0 0 0 i\n"
  "MouseMoveEvent 170 217 0 0 0 0 i\n"
  "MouseMoveEvent 169 216 0 0 0 0 i\n"
  "MouseMoveEvent 168 216 0 0 0 0 i\n"
  "MouseMoveEvent 168 215 0 0 0 0 i\n"
  "MouseMoveEvent 167 214 0 0 0 0 i\n"
  "MouseMoveEvent 164 214 0 0 0 0 i\n"
  "MouseMoveEvent 163 213 0 0 0 0 i\n"
  "MouseMoveEvent 163 212 0 0 0 0 i\n"
  "MouseMoveEvent 162 210 0 0 0 0 i\n"
  "MouseMoveEvent 162 209 0 0 0 0 i\n"
  "MouseMoveEvent 162 206 0 0 0 0 i\n"
  "MouseMoveEvent 161 202 0 0 0 0 i\n"
  "MouseMoveEvent 160 197 0 0 0 0 i\n"
  "MouseMoveEvent 160 192 0 0 0 0 i\n"
  "MouseMoveEvent 160 187 0 0 0 0 i\n"
  "MouseMoveEvent 158 182 0 0 0 0 i\n"
  "MouseMoveEvent 157 174 0 0 0 0 i\n"
  "MouseMoveEvent 156 169 0 0 0 0 i\n"
  "MouseMoveEvent 156 164 0 0 0 0 i\n"
  "MouseMoveEvent 156 157 0 0 0 0 i\n"
  "MouseMoveEvent 156 156 0 0 0 0 i\n"
  "MouseMoveEvent 156 152 0 0 0 0 i\n"
  "MouseMoveEvent 156 151 0 0 0 0 i\n"
  "MouseMoveEvent 156 148 0 0 0 0 i\n"
  "MouseMoveEvent 156 146 0 0 0 0 i\n"
  "MouseMoveEvent 156 145 0 0 0 0 i\n"
  "MouseMoveEvent 156 143 0 0 0 0 i\n"
  "MouseMoveEvent 156 142 0 0 0 0 i\n"
  "MouseMoveEvent 156 141 0 0 0 0 i\n"
  "MouseMoveEvent 155 140 0 0 0 0 i\n"
  "MouseMoveEvent 154 140 0 0 0 0 i\n"
  "MouseMoveEvent 154 139 0 0 0 0 i\n"
  "MouseMoveEvent 151 137 0 0 0 0 i\n"
  "MouseMoveEvent 149 136 0 0 0 0 i\n"
  "MouseMoveEvent 147 135 0 0 0 0 i\n"
  "MouseMoveEvent 144 132 0 0 0 0 i\n"
  "MouseMoveEvent 143 132 0 0 0 0 i\n"
  "MouseMoveEvent 143 131 0 0 0 0 i\n"
  "MouseMoveEvent 143 130 0 0 0 0 i\n"
  "LeftButtonPressEvent 143 130 0 0 0 0 i\n"
  "MouseMoveEvent 143 129 0 0 0 0 i\n"
  "MouseMoveEvent 143 126 0 0 0 0 i\n"
  "MouseMoveEvent 143 125 0 0 0 0 i\n"
  "MouseMoveEvent 143 123 0 0 0 0 i\n"
  "MouseMoveEvent 143 120 0 0 0 0 i\n"
  "MouseMoveEvent 143 118 0 0 0 0 i\n"
  "MouseMoveEvent 143 116 0 0 0 0 i\n"
  "MouseMoveEvent 143 115 0 0 0 0 i\n"
  "MouseMoveEvent 142 115 0 0 0 0 i\n"
  "MouseMoveEvent 142 114 0 0 0 0 i\n"
  "MouseMoveEvent 142 113 0 0 0 0 i\n"
  "MouseMoveEvent 141 113 0 0 0 0 i\n"
  "MouseMoveEvent 141 112 0 0 0 0 i\n"
  "MouseMoveEvent 141 111 0 0 0 0 i\n"
  "MouseMoveEvent 141 110 0 0 0 0 i\n"
  "MouseMoveEvent 140 109 0 0 0 0 i\n"
  "MouseMoveEvent 140 108 0 0 0 0 i\n"
  "MouseMoveEvent 139 108 0 0 0 0 i\n"
  "MouseMoveEvent 138 107 0 0 0 0 i\n"
  "MouseMoveEvent 137 104 0 0 0 0 i\n"
  "MouseMoveEvent 137 103 0 0 0 0 i\n"
  "MouseMoveEvent 137 102 0 0 0 0 i\n"
  "MouseMoveEvent 137 101 0 0 0 0 i\n"
  "MouseMoveEvent 137 100 0 0 0 0 i\n"
  "MouseMoveEvent 137 99 0 0 0 0 i\n"
  "MouseMoveEvent 137 98 0 0 0 0 i\n"
  "MouseMoveEvent 137 97 0 0 0 0 i\n"
  "MouseMoveEvent 137 96 0 0 0 0 i\n"
  "MouseMoveEvent 137 95 0 0 0 0 i\n"
  "MouseMoveEvent 137 94 0 0 0 0 i\n"
  "MouseMoveEvent 137 93 0 0 0 0 i\n"
  "MouseMoveEvent 137 92 0 0 0 0 i\n"
  "MouseMoveEvent 137 91 0 0 0 0 i\n"
  "MouseMoveEvent 137 90 0 0 0 0 i\n"
  "MouseMoveEvent 136 89 0 0 0 0 i\n"
  "MouseMoveEvent 136 88 0 0 0 0 i\n"
  "LeftButtonReleaseEvent 136 88 0 0 0 0 i\n"
  "MouseMoveEvent 136 88 0 0 0 0 i\n"
  "MouseMoveEvent 136 89 0 0 0 0 i\n"
  "MouseMoveEvent 136 91 0 0 0 0 i\n"
  "MouseMoveEvent 136 92 0 0 0 0 i\n"
  "MouseMoveEvent 136 91 0 0 0 0 i\n"
  "MouseMoveEvent 136 90 0 0 0 0 i\n"
  "MouseMoveEvent 136 89 0 0 0 0 i\n"
  "MouseMoveEvent 136 90 0 0 0 0 i\n"
  "MouseMoveEvent 136 91 0 0 0 0 i\n"
  "MouseMoveEvent 136 92 0 0 0 0 i\n"
  "MouseMoveEvent 135 93 0 0 0 0 i\n"
  ;

int BoxWidget2( int , char *[] )
{
  vtkSmartPointer<vtkRenderer> renderer =
    vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer(renderer);
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  vtkSmartPointer<vtkConeSource> cone =
    vtkSmartPointer<vtkConeSource>::New();
  cone->SetResolution(6);
  vtkSmartPointer<vtkSphereSource> sphere =
    vtkSmartPointer<vtkSphereSource>::New();
  sphere->SetThetaResolution(8); sphere->SetPhiResolution(8);
  vtkSmartPointer<vtkGlyph3D> glyph =
    vtkSmartPointer<vtkGlyph3D>::New();
  glyph->SetInputConnection(sphere->GetOutputPort());
  glyph->SetSource(cone->GetOutput());
  glyph->SetVectorModeToUseNormal();
  glyph->SetScaleModeToScaleByVector();
  glyph->SetScaleFactor(0.25);
  glyph->Update();
                                                        
  vtkSmartPointer<vtkAppendPolyData> append =
    vtkSmartPointer<vtkAppendPolyData>::New();
  append->AddInput(glyph->GetOutput());
  append->AddInput(sphere->GetOutput());
  
  vtkSmartPointer<vtkPolyDataMapper> maceMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  maceMapper->SetInputConnection(append->GetOutputPort());

  vtkSmartPointer<vtkActor> maceActor =
    vtkSmartPointer<vtkActor>::New();
  maceActor->SetMapper(maceMapper);

  // Configure the box widget including callbacks
  vtkSmartPointer<vtkTransform> t =
    vtkSmartPointer<vtkTransform>::New();
  vtkSmartPointer<vtkBWCallback2> myCallback =
    vtkSmartPointer<vtkBWCallback2>::New();
  myCallback->Transform = t;
  myCallback->Actor = maceActor;

  vtkSmartPointer<vtkBoxRepresentation> boxRep =
    vtkSmartPointer<vtkBoxRepresentation>::New();
  boxRep->SetPlaceFactor( 1.25 );
  boxRep->PlaceWidget(glyph->GetOutput()->GetBounds());

  vtkSmartPointer<vtkBoxWidget2> boxWidget =
    vtkSmartPointer<vtkBoxWidget2>::New();
  boxWidget->SetInteractor( iren );
  boxWidget->SetRepresentation( boxRep );
  boxWidget->AddObserver(vtkCommand::InteractionEvent,myCallback);
  boxWidget->SetPriority(1);

  renderer->AddActor(maceActor);
  renderer->SetBackground(0,0,0);
  renWin->SetSize(300,300);

  // record events
  vtkSmartPointer<vtkInteractorEventRecorder> recorder =
    vtkSmartPointer<vtkInteractorEventRecorder>::New();
  recorder->SetInteractor(iren);
//  recorder->SetFileName("c:/record.log");
//  recorder->Record();
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(BoxWidgetEventLog2);

  // interact with data
  // render the image
  //
  iren->Initialize();
  renWin->Render();
  recorder->Play();

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();

  boxRep->SetPlaceFactor( 1.0 );
  boxRep->HandlesOff();

  boxRep->SetPlaceFactor( 1.25 );
  boxRep->HandlesOn();
  
  iren->Start();
  
  // Clean up
  recorder->Off();

  return EXIT_SUCCESS;
}
