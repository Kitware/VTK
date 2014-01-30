/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImplicitPlaneWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSmartPointer.h"

#include "vtkActor.h"
#include "vtkAppendPolyData.h"
#include "vtkClipPolyData.h"
#include "vtkCommand.h"
#include "vtkConeSource.h"
#include "vtkGlyph3D.h"
#include "vtkImplicitPlaneWidget.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkLODActor.h"
#include "vtkPlane.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"

const char eventLog[] =
  "# StreamVersion 1\n"
  "CharEvent 108 202 0 0 105 1 i\n"
  "KeyReleaseEvent 108 202 0 0 105 1 i\n"
  "MouseMoveEvent 255 120 0 0 0 0 i\n"
  "MouseMoveEvent 255 121 0 0 0 0 i\n"
  "MouseMoveEvent 255 122 0 0 0 0 i\n"
  "MouseMoveEvent 255 123 0 0 0 0 i\n"
  "MouseMoveEvent 255 125 0 0 0 0 i\n"
  "MouseMoveEvent 255 126 0 0 0 0 i\n"
  "MouseMoveEvent 253 129 0 0 0 0 i\n"
  "MouseMoveEvent 253 133 0 0 0 0 i\n"
  "MouseMoveEvent 253 138 0 0 0 0 i\n"
  "MouseMoveEvent 253 141 0 0 0 0 i\n"
  "MouseMoveEvent 251 147 0 0 0 0 i\n"
  "MouseMoveEvent 251 146 0 0 0 0 i\n"
  "MouseMoveEvent 251 147 0 0 0 0 i\n"
  "MouseMoveEvent 251 148 0 0 0 0 i\n"
  "LeftButtonPressEvent 251 148 0 0 0 0 i\n"
  "MouseMoveEvent 251 149 0 0 0 0 i\n"
  "MouseMoveEvent 251 150 0 0 0 0 i\n"
  "MouseMoveEvent 251 151 0 0 0 0 i\n"
  "MouseMoveEvent 251 152 0 0 0 0 i\n"
  "MouseMoveEvent 251 153 0 0 0 0 i\n"
  "MouseMoveEvent 251 154 0 0 0 0 i\n"
  "MouseMoveEvent 250 154 0 0 0 0 i\n"
  "MouseMoveEvent 250 155 0 0 0 0 i\n"
  "MouseMoveEvent 250 157 0 0 0 0 i\n"
  "MouseMoveEvent 250 158 0 0 0 0 i\n"
  "MouseMoveEvent 250 159 0 0 0 0 i\n"
  "MouseMoveEvent 249 160 0 0 0 0 i\n"
  "MouseMoveEvent 249 161 0 0 0 0 i\n"
  "MouseMoveEvent 249 163 0 0 0 0 i\n"
  "MouseMoveEvent 249 166 0 0 0 0 i\n"
  "MouseMoveEvent 249 167 0 0 0 0 i\n"
  "MouseMoveEvent 249 169 0 0 0 0 i\n"
  "MouseMoveEvent 249 170 0 0 0 0 i\n"
  "MouseMoveEvent 249 171 0 0 0 0 i\n"
  "MouseMoveEvent 248 172 0 0 0 0 i\n"
  "MouseMoveEvent 248 174 0 0 0 0 i\n"
  "MouseMoveEvent 247 174 0 0 0 0 i\n"
  "MouseMoveEvent 247 175 0 0 0 0 i\n"
  "MouseMoveEvent 247 174 0 0 0 0 i\n"
  "MouseMoveEvent 247 172 0 0 0 0 i\n"
  "MouseMoveEvent 246 170 0 0 0 0 i\n"
  "MouseMoveEvent 245 170 0 0 0 0 i\n"
  "MouseMoveEvent 244 170 0 0 0 0 i\n"
  "MouseMoveEvent 239 168 0 0 0 0 i\n"
  "MouseMoveEvent 235 166 0 0 0 0 i\n"
  "MouseMoveEvent 232 166 0 0 0 0 i\n"
  "MouseMoveEvent 232 167 0 0 0 0 i\n"
  "MouseMoveEvent 231 167 0 0 0 0 i\n"
  "MouseMoveEvent 230 168 0 0 0 0 i\n"
  "MouseMoveEvent 229 170 0 0 0 0 i\n"
  "MouseMoveEvent 227 173 0 0 0 0 i\n"
  "MouseMoveEvent 227 174 0 0 0 0 i\n"
  "MouseMoveEvent 226 176 0 0 0 0 i\n"
  "MouseMoveEvent 224 180 0 0 0 0 i\n"
  "MouseMoveEvent 224 182 0 0 0 0 i\n"
  "MouseMoveEvent 224 184 0 0 0 0 i\n"
  "MouseMoveEvent 224 186 0 0 0 0 i\n"
  "MouseMoveEvent 221 190 0 0 0 0 i\n"
  "MouseMoveEvent 218 191 0 0 0 0 i\n"
  "MouseMoveEvent 218 192 0 0 0 0 i\n"
  "MouseMoveEvent 218 195 0 0 0 0 i\n"
  "MouseMoveEvent 217 200 0 0 0 0 i\n"
  "MouseMoveEvent 217 203 0 0 0 0 i\n"
  "MouseMoveEvent 217 207 0 0 0 0 i\n"
  "MouseMoveEvent 217 208 0 0 0 0 i\n"
  "MouseMoveEvent 217 210 0 0 0 0 i\n"
  "MouseMoveEvent 217 211 0 0 0 0 i\n"
  "MouseMoveEvent 217 214 0 0 0 0 i\n"
  "MouseMoveEvent 217 216 0 0 0 0 i\n"
  "MouseMoveEvent 217 217 0 0 0 0 i\n"
  "MouseMoveEvent 219 218 0 0 0 0 i\n"
  "MouseMoveEvent 220 219 0 0 0 0 i\n"
  "MouseMoveEvent 220 222 0 0 0 0 i\n"
  "MouseMoveEvent 220 225 0 0 0 0 i\n"
  "MouseMoveEvent 220 227 0 0 0 0 i\n"
  "MouseMoveEvent 220 228 0 0 0 0 i\n"
  "MouseMoveEvent 220 230 0 0 0 0 i\n"
  "MouseMoveEvent 220 232 0 0 0 0 i\n"
  "MouseMoveEvent 220 236 0 0 0 0 i\n"
  "MouseMoveEvent 220 237 0 0 0 0 i\n"
  "MouseMoveEvent 220 238 0 0 0 0 i\n"
  "MouseMoveEvent 220 239 0 0 0 0 i\n"
  "MouseMoveEvent 219 239 0 0 0 0 i\n"
  "MouseMoveEvent 218 239 0 0 0 0 i\n"
  "MouseMoveEvent 217 239 0 0 0 0 i\n"
  "MouseMoveEvent 217 238 0 0 0 0 i\n"
  "MouseMoveEvent 217 237 0 0 0 0 i\n"
  "LeftButtonReleaseEvent 217 237 0 0 0 0 i\n"
  "MouseMoveEvent 213 233 0 0 0 0 i\n"
  "MouseMoveEvent 212 231 0 0 0 0 i\n"
  "MouseMoveEvent 212 227 0 0 0 0 i\n"
  "MouseMoveEvent 211 225 0 0 0 0 i\n"
  "MouseMoveEvent 211 221 0 0 0 0 i\n"
  "MouseMoveEvent 208 216 0 0 0 0 i\n"
  "MouseMoveEvent 206 214 0 0 0 0 i\n"
  "MouseMoveEvent 206 210 0 0 0 0 i\n"
  "MouseMoveEvent 206 206 0 0 0 0 i\n"
  "MouseMoveEvent 206 201 0 0 0 0 i\n"
  "MouseMoveEvent 206 198 0 0 0 0 i\n"
  "MouseMoveEvent 206 194 0 0 0 0 i\n"
  "MouseMoveEvent 206 189 0 0 0 0 i\n"
  "MouseMoveEvent 206 185 0 0 0 0 i\n"
  "MouseMoveEvent 206 180 0 0 0 0 i\n"
  "MouseMoveEvent 205 175 0 0 0 0 i\n"
  "MouseMoveEvent 203 171 0 0 0 0 i\n"
  "MouseMoveEvent 202 168 0 0 0 0 i\n"
  "MouseMoveEvent 202 161 0 0 0 0 i\n"
  "MouseMoveEvent 201 156 0 0 0 0 i\n"
  "MouseMoveEvent 200 150 0 0 0 0 i\n"
  "MouseMoveEvent 198 147 0 0 0 0 i\n"
  "MouseMoveEvent 198 143 0 0 0 0 i\n"
  "MouseMoveEvent 196 140 0 0 0 0 i\n"
  "MouseMoveEvent 196 136 0 0 0 0 i\n"
  "MouseMoveEvent 193 131 0 0 0 0 i\n"
  "MouseMoveEvent 192 121 0 0 0 0 i\n"
  "MouseMoveEvent 192 114 0 0 0 0 i\n"
  "MouseMoveEvent 192 109 0 0 0 0 i\n"
  "MouseMoveEvent 192 104 0 0 0 0 i\n"
  "MouseMoveEvent 192 100 0 0 0 0 i\n"
  "MouseMoveEvent 192 97 0 0 0 0 i\n"
  "MouseMoveEvent 189 93 0 0 0 0 i\n"
  "MouseMoveEvent 188 92 0 0 0 0 i\n"
  "MouseMoveEvent 187 90 0 0 0 0 i\n"
  "MouseMoveEvent 186 89 0 0 0 0 i\n"
  "MouseMoveEvent 185 88 0 0 0 0 i\n"
  "MouseMoveEvent 184 86 0 0 0 0 i\n"
  "MouseMoveEvent 184 85 0 0 0 0 i\n"
  "MouseMoveEvent 183 85 0 0 0 0 i\n"
  "MouseMoveEvent 182 84 0 0 0 0 i\n"
  "MouseMoveEvent 180 83 0 0 0 0 i\n"
  "MouseMoveEvent 179 83 0 0 0 0 i\n"
  "MouseMoveEvent 178 83 0 0 0 0 i\n"
  "MouseMoveEvent 175 82 0 0 0 0 i\n"
  "MouseMoveEvent 173 81 0 0 0 0 i\n"
  "MouseMoveEvent 169 79 0 0 0 0 i\n"
  "MouseMoveEvent 168 78 0 0 0 0 i\n"
  "MouseMoveEvent 167 78 0 0 0 0 i\n"
  "MouseMoveEvent 167 79 0 0 0 0 i\n"
  "LeftButtonPressEvent 167 79 0 0 0 0 i\n"
  "MouseMoveEvent 167 80 0 0 0 0 i\n"
  "MouseMoveEvent 167 81 0 0 0 0 i\n"
  "MouseMoveEvent 169 82 0 0 0 0 i\n"
  "MouseMoveEvent 170 83 0 0 0 0 i\n"
  "MouseMoveEvent 171 84 0 0 0 0 i\n"
  "MouseMoveEvent 172 84 0 0 0 0 i\n"
  "MouseMoveEvent 174 85 0 0 0 0 i\n"
  "MouseMoveEvent 174 86 0 0 0 0 i\n"
  "MouseMoveEvent 175 86 0 0 0 0 i\n"
  "MouseMoveEvent 176 88 0 0 0 0 i\n"
  "MouseMoveEvent 177 90 0 0 0 0 i\n"
  "MouseMoveEvent 177 91 0 0 0 0 i\n"
  "MouseMoveEvent 177 92 0 0 0 0 i\n"
  "MouseMoveEvent 177 93 0 0 0 0 i\n"
  "MouseMoveEvent 177 94 0 0 0 0 i\n"
  "MouseMoveEvent 178 96 0 0 0 0 i\n"
  "MouseMoveEvent 178 97 0 0 0 0 i\n"
  "MouseMoveEvent 180 99 0 0 0 0 i\n"
  "MouseMoveEvent 181 99 0 0 0 0 i\n"
  "MouseMoveEvent 181 100 0 0 0 0 i\n"
  "MouseMoveEvent 181 101 0 0 0 0 i\n"
  "MouseMoveEvent 181 102 0 0 0 0 i\n"
  "MouseMoveEvent 181 103 0 0 0 0 i\n"
  "LeftButtonReleaseEvent 181 103 0 0 0 0 i\n"
  "MouseMoveEvent 181 103 0 0 0 0 i\n"
  "MouseMoveEvent 181 102 0 0 0 0 i\n"
  "MouseMoveEvent 181 102 0 0 0 0 i\n"
  "MouseMoveEvent 181 102 0 0 0 0 i\n"
  "MouseMoveEvent 182 102 0 0 0 0 i\n"
  "MouseMoveEvent 183 103 0 0 0 0 i\n"
  "MouseMoveEvent 183 102 0 0 0 0 i\n"
  "MouseMoveEvent 184 102 0 0 0 0 i\n"
  "MouseMoveEvent 185 102 0 0 0 0 i\n"
  "MouseMoveEvent 185 101 0 0 0 0 i\n"
  "MouseMoveEvent 185 100 0 0 0 0 i\n"
  "MouseMoveEvent 185 99 0 0 0 0 i\n"
  "MiddleButtonPressEvent 185 99 0 0 0 0 i\n"
  "MouseMoveEvent 185 100 0 0 0 0 i\n"
  "MouseMoveEvent 182 101 0 0 0 0 i\n"
  "MouseMoveEvent 181 102 0 0 0 0 i\n"
  "MouseMoveEvent 180 103 0 0 0 0 i\n"
  "MouseMoveEvent 179 105 0 0 0 0 i\n"
  "MouseMoveEvent 176 107 0 0 0 0 i\n"
  "MouseMoveEvent 173 105 0 0 0 0 i\n"
  "MouseMoveEvent 173 106 0 0 0 0 i\n"
  "MouseMoveEvent 173 107 0 0 0 0 i\n"
  "MouseMoveEvent 173 108 0 0 0 0 i\n"
  "MouseMoveEvent 173 110 0 0 0 0 i\n"
  "MouseMoveEvent 172 112 0 0 0 0 i\n"
  "MouseMoveEvent 172 113 0 0 0 0 i\n"
  "MouseMoveEvent 172 114 0 0 0 0 i\n"
  "MouseMoveEvent 172 115 0 0 0 0 i\n"
  "MouseMoveEvent 173 116 0 0 0 0 i\n"
  "MouseMoveEvent 173 117 0 0 0 0 i\n"
  "MouseMoveEvent 174 118 0 0 0 0 i\n"
  "MouseMoveEvent 174 119 0 0 0 0 i\n"
  "MouseMoveEvent 174 120 0 0 0 0 i\n"
  "MouseMoveEvent 174 121 0 0 0 0 i\n"
  "MouseMoveEvent 174 122 0 0 0 0 i\n"
  "MouseMoveEvent 175 122 0 0 0 0 i\n"
  "MiddleButtonReleaseEvent 175 122 0 0 0 0 i\n"
  "MouseMoveEvent 175 122 0 0 0 0 i\n"
  "MouseMoveEvent 175 121 0 0 0 0 i\n"
  "MouseMoveEvent 174 120 0 0 0 0 i\n"
  "MouseMoveEvent 173 120 0 0 0 0 i\n"
  "MouseMoveEvent 173 119 0 0 0 0 i\n"
  "MouseMoveEvent 173 118 0 0 0 0 i\n"
  "MouseMoveEvent 172 117 0 0 0 0 i\n"
  "MouseMoveEvent 171 117 0 0 0 0 i\n"
  "MouseMoveEvent 171 116 0 0 0 0 i\n"
  "MouseMoveEvent 171 115 0 0 0 0 i\n"
  "MouseMoveEvent 171 114 0 0 0 0 i\n"
  "MouseMoveEvent 171 112 0 0 0 0 i\n"
  "MouseMoveEvent 171 109 0 0 0 0 i\n"
  "MouseMoveEvent 172 107 0 0 0 0 i\n"
  "MouseMoveEvent 172 105 0 0 0 0 i\n"
  "MouseMoveEvent 175 103 0 0 0 0 i\n"
  "MouseMoveEvent 176 100 0 0 0 0 i\n"
  "MouseMoveEvent 178 99 0 0 0 0 i\n"
  "MouseMoveEvent 179 97 0 0 0 0 i\n"
  "MouseMoveEvent 182 94 0 0 0 0 i\n"
  "MouseMoveEvent 183 92 0 0 0 0 i\n"
  "MouseMoveEvent 185 90 0 0 0 0 i\n"
  "MouseMoveEvent 186 87 0 0 0 0 i\n"
  "MouseMoveEvent 188 84 0 0 0 0 i\n"
  "MouseMoveEvent 190 81 0 0 0 0 i\n"
  "MouseMoveEvent 193 79 0 0 0 0 i\n"
  "MouseMoveEvent 195 78 0 0 0 0 i\n"
  "MouseMoveEvent 197 76 0 0 0 0 i\n"
  "MouseMoveEvent 201 75 0 0 0 0 i\n"
  "MouseMoveEvent 205 71 0 0 0 0 i\n"
  "MouseMoveEvent 209 67 0 0 0 0 i\n"
  "MouseMoveEvent 215 63 0 0 0 0 i\n"
  "MouseMoveEvent 221 58 0 0 0 0 i\n"
  "MouseMoveEvent 226 56 0 0 0 0 i\n"
  "MouseMoveEvent 228 54 0 0 0 0 i\n"
  "MouseMoveEvent 231 50 0 0 0 0 i\n"
  "MouseMoveEvent 235 47 0 0 0 0 i\n"
  "MouseMoveEvent 237 45 0 0 0 0 i\n"
  "MouseMoveEvent 239 42 0 0 0 0 i\n"
  "MouseMoveEvent 243 42 0 0 0 0 i\n"
  "MouseMoveEvent 245 41 0 0 0 0 i\n"
  "MouseMoveEvent 246 40 0 0 0 0 i\n"
  "MouseMoveEvent 246 39 0 0 0 0 i\n"
  "MouseMoveEvent 247 39 0 0 0 0 i\n"
  "MouseMoveEvent 248 38 0 0 0 0 i\n"
  "MouseMoveEvent 249 38 0 0 0 0 i\n"
  "MouseMoveEvent 250 38 0 0 0 0 i\n"
  "MouseMoveEvent 250 37 0 0 0 0 i\n"
  "MouseMoveEvent 252 36 0 0 0 0 i\n"
  "MouseMoveEvent 253 36 0 0 0 0 i\n"
  "MouseMoveEvent 254 36 0 0 0 0 i\n"
  "MouseMoveEvent 254 37 0 0 0 0 i\n"
  "MouseMoveEvent 255 37 0 0 0 0 i\n"
  "MouseMoveEvent 256 37 0 0 0 0 i\n"
  "MouseMoveEvent 257 37 0 0 0 0 i\n"
  "MouseMoveEvent 258 37 0 0 0 0 i\n"
  "MouseMoveEvent 259 38 0 0 0 0 i\n"
  "MouseMoveEvent 260 39 0 0 0 0 i\n"
  "MouseMoveEvent 260 40 0 0 0 0 i\n"
  "MouseMoveEvent 259 40 0 0 0 0 i\n"
  "MouseMoveEvent 258 40 0 0 0 0 i\n"
  "MouseMoveEvent 257 40 0 0 0 0 i\n"
  "MouseMoveEvent 257 41 0 0 0 0 i\n"
  "MouseMoveEvent 257 42 0 0 0 0 i\n"
  "MouseMoveEvent 257 43 0 0 0 0 i\n"
  "MouseMoveEvent 257 44 0 0 0 0 i\n"
  "MouseMoveEvent 258 44 0 0 0 0 i\n"
  "LeftButtonPressEvent 258 44 0 0 0 0 i\n"
  "MouseMoveEvent 258 45 0 0 0 0 i\n"
  "MouseMoveEvent 259 45 0 0 0 0 i\n"
  "MouseMoveEvent 259 46 0 0 0 0 i\n"
  "MouseMoveEvent 259 47 0 0 0 0 i\n"
  "MouseMoveEvent 259 49 0 0 0 0 i\n"
  "MouseMoveEvent 259 50 0 0 0 0 i\n"
  "MouseMoveEvent 259 51 0 0 0 0 i\n"
  "MouseMoveEvent 259 52 0 0 0 0 i\n"
  "MouseMoveEvent 260 53 0 0 0 0 i\n"
  "MouseMoveEvent 260 54 0 0 0 0 i\n"
  "MouseMoveEvent 260 55 0 0 0 0 i\n"
  "MouseMoveEvent 260 56 0 0 0 0 i\n"
  "MouseMoveEvent 260 57 0 0 0 0 i\n"
  "MouseMoveEvent 261 58 0 0 0 0 i\n"
  "MouseMoveEvent 261 59 0 0 0 0 i\n"
  "MouseMoveEvent 261 60 0 0 0 0 i\n"
  "MouseMoveEvent 261 61 0 0 0 0 i\n"
  "MouseMoveEvent 261 60 0 0 0 0 i\n"
  "MouseMoveEvent 260 59 0 0 0 0 i\n"
  "MouseMoveEvent 258 58 0 0 0 0 i\n"
  "MouseMoveEvent 255 57 0 0 0 0 i\n"
  "MouseMoveEvent 252 55 0 0 0 0 i\n"
  "MouseMoveEvent 251 54 0 0 0 0 i\n"
  "MouseMoveEvent 251 53 0 0 0 0 i\n"
  "MouseMoveEvent 249 51 0 0 0 0 i\n"
  "MouseMoveEvent 246 50 0 0 0 0 i\n"
  "MouseMoveEvent 245 50 0 0 0 0 i\n"
  "MouseMoveEvent 245 49 0 0 0 0 i\n"
  "MouseMoveEvent 244 48 0 0 0 0 i\n"
  "MouseMoveEvent 242 45 0 0 0 0 i\n"
  "MouseMoveEvent 241 44 0 0 0 0 i\n"
  "MouseMoveEvent 240 44 0 0 0 0 i\n"
  "MouseMoveEvent 239 44 0 0 0 0 i\n"
  "MouseMoveEvent 238 42 0 0 0 0 i\n"
  "MouseMoveEvent 237 42 0 0 0 0 i\n"
  "MouseMoveEvent 236 42 0 0 0 0 i\n"
  "MouseMoveEvent 236 41 0 0 0 0 i\n"
  "LeftButtonReleaseEvent 236 41 0 0 0 0 i\n"
  "MouseMoveEvent 236 40 0 0 0 0 i\n"
  "MouseMoveEvent 235 40 0 0 0 0 i\n"
  "MouseMoveEvent 234 40 0 0 0 0 i\n"
  "MouseMoveEvent 233 40 0 0 0 0 i\n"
  "MouseMoveEvent 233 41 0 0 0 0 i\n"
  "MouseMoveEvent 233 42 0 0 0 0 i\n"
  "MouseMoveEvent 232 42 0 0 0 0 i\n"
  "MouseMoveEvent 231 42 0 0 0 0 i\n"
  "MouseMoveEvent 231 43 0 0 0 0 i\n"
  "RightButtonPressEvent 231 43 0 0 0 0 i\n"
  "MouseMoveEvent 231 42 0 0 0 0 i\n"
  "MouseMoveEvent 231 41 0 0 0 0 i\n"
  "MouseMoveEvent 231 40 0 0 0 0 i\n"
  "MouseMoveEvent 230 40 0 0 0 0 i\n"
  "MouseMoveEvent 229 38 0 0 0 0 i\n"
  "MouseMoveEvent 227 33 0 0 0 0 i\n"
  "MouseMoveEvent 226 27 0 0 0 0 i\n"
  "MouseMoveEvent 222 23 0 0 0 0 i\n"
  "MouseMoveEvent 214 19 0 0 0 0 i\n"
  "MouseMoveEvent 208 14 0 0 0 0 i\n"
  "MouseMoveEvent 208 15 0 0 0 0 i\n"
  "MouseMoveEvent 208 14 0 0 0 0 i\n"
  "MouseMoveEvent 208 13 0 0 0 0 i\n"
  "MouseMoveEvent 210 11 0 0 0 0 i\n"
  "MouseMoveEvent 213 9 0 0 0 0 i\n"
  "MouseMoveEvent 214 8 0 0 0 0 i\n"
  "MouseMoveEvent 214 7 0 0 0 0 i\n"
  "MouseMoveEvent 215 7 0 0 0 0 i\n"
  "MouseMoveEvent 215 6 0 0 0 0 i\n"
  "MouseMoveEvent 215 5 0 0 0 0 i\n"
  "MouseMoveEvent 216 2 0 0 0 0 i\n"
  "MouseMoveEvent 218 1 0 0 0 0 i\n"
  "LeaveEvent 220 -1 0 0 0 0 i\n"
  "MouseMoveEvent 220 -1 0 0 0 0 i\n"
  "MouseMoveEvent 221 -1 0 0 0 0 i\n"
  "MouseMoveEvent 221 -2 0 0 0 0 i\n"
  "MouseMoveEvent 220 -2 0 0 0 0 i\n"
  "MouseMoveEvent 220 -3 0 0 0 0 i\n"
  "MouseMoveEvent 220 -4 0 0 0 0 i\n"
  "MouseMoveEvent 220 -5 0 0 0 0 i\n"
  "MouseMoveEvent 220 -6 0 0 0 0 i\n"
  "MouseMoveEvent 220 -7 0 0 0 0 i\n"
  "MouseMoveEvent 220 -8 0 0 0 0 i\n"
  "MouseMoveEvent 220 -9 0 0 0 0 i\n"
  "MouseMoveEvent 220 -11 0 0 0 0 i\n"
  "MouseMoveEvent 220 -12 0 0 0 0 i\n"
  "MouseMoveEvent 220 -13 0 0 0 0 i\n"
  "MouseMoveEvent 220 -12 0 0 0 0 i\n"
  "MouseMoveEvent 220 -11 0 0 0 0 i\n"
  "RightButtonReleaseEvent 220 -11 0 0 0 0 i\n"
  "EnterEvent 218 0 0 0 0 0 i\n"
  "MouseMoveEvent 218 0 0 0 0 0 i\n"
  "MouseMoveEvent 218 1 0 0 0 0 i\n"
  "MouseMoveEvent 218 2 0 0 0 0 i\n"
  "MouseMoveEvent 217 2 0 0 0 0 i\n"
  "MouseMoveEvent 217 4 0 0 0 0 i\n"
  "MouseMoveEvent 217 5 0 0 0 0 i\n"
  "MouseMoveEvent 217 6 0 0 0 0 i\n"
  "MouseMoveEvent 217 7 0 0 0 0 i\n"
  "MouseMoveEvent 217 8 0 0 0 0 i\n"
  "MouseMoveEvent 217 9 0 0 0 0 i\n"
  "MouseMoveEvent 216 11 0 0 0 0 i\n"
  "MouseMoveEvent 216 12 0 0 0 0 i\n"
  "MouseMoveEvent 215 12 0 0 0 0 i\n"
  "MouseMoveEvent 215 11 0 0 0 0 i\n"
  "MouseMoveEvent 215 12 0 0 0 0 i\n"
  "MouseMoveEvent 215 13 0 0 0 0 i\n"
  "MouseMoveEvent 215 14 0 0 0 0 i\n"
  ;

// This does the actual work: updates the vtkPlane implicit function.
// This in turn causes the pipeline to update and clip the object.
// Callback for the interaction
class vtkTIPWCallback : public vtkCommand
{
public:
  static vtkTIPWCallback *New()
  { return new vtkTIPWCallback; }
  virtual void Execute(vtkObject *caller, unsigned long, void*)
  {
    vtkImplicitPlaneWidget *planeWidget =
      reinterpret_cast<vtkImplicitPlaneWidget*>(caller);
    planeWidget->GetPlane(this->Plane);
    this->Actor->VisibilityOn();
  }
  vtkTIPWCallback():Plane(0),Actor(0) {}
  vtkPlane *Plane;
  vtkActor *Actor;

};

int TestImplicitPlaneWidget(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  // Create a mace out of filters.
  //
  vtkSmartPointer<vtkSphereSource> sphere =
    vtkSmartPointer<vtkSphereSource>::New();
  vtkSmartPointer<vtkConeSource> cone =
    vtkSmartPointer<vtkConeSource>::New();
  vtkSmartPointer<vtkGlyph3D> glyph =
    vtkSmartPointer<vtkGlyph3D>::New();
  glyph->SetInputConnection(sphere->GetOutputPort());
  glyph->SetSourceConnection(cone->GetOutputPort());
  glyph->SetVectorModeToUseNormal();
  glyph->SetScaleModeToScaleByVector();
  glyph->SetScaleFactor(0.25);

  // The sphere and spikes are appended into a single polydata.
  // This just makes things simpler to manage.
  vtkSmartPointer<vtkAppendPolyData> apd =
    vtkSmartPointer<vtkAppendPolyData>::New();
  apd->AddInputConnection(glyph->GetOutputPort());
  apd->AddInputConnection(sphere->GetOutputPort());

  vtkSmartPointer<vtkPolyDataMapper> maceMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  maceMapper->SetInputConnection(apd->GetOutputPort());

  vtkSmartPointer<vtkLODActor> maceActor =
    vtkSmartPointer<vtkLODActor>::New();
  maceActor->SetMapper(maceMapper);
  maceActor->VisibilityOn();

  // This portion of the code clips the mace with the vtkPlanes
  // implicit function. The clipped region is colored green.
  vtkSmartPointer<vtkPlane> plane =
    vtkSmartPointer<vtkPlane>::New();
  vtkSmartPointer<vtkClipPolyData> clipper =
    vtkSmartPointer<vtkClipPolyData>::New();
  clipper->SetInputConnection(apd->GetOutputPort());
  clipper->SetClipFunction(plane);
  clipper->InsideOutOn();

  vtkSmartPointer<vtkPolyDataMapper> selectMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  selectMapper->SetInputConnection(clipper->GetOutputPort());

  vtkSmartPointer<vtkLODActor> selectActor =
    vtkSmartPointer<vtkLODActor>::New();
  selectActor->SetMapper(selectMapper);
  selectActor->GetProperty()->SetColor(0,1,0);
  selectActor->VisibilityOff();
  selectActor->SetScale(1.01, 1.01, 1.01);

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

  // The SetInteractor method is how 3D widgets are associated with the render
  // window interactor. Internally, SetInteractor sets up a bunch of callbacks
  // using the Command/Observer mechanism (AddObserver()).
  vtkSmartPointer<vtkTIPWCallback> myCallback =
    vtkSmartPointer<vtkTIPWCallback>::New();
  myCallback->Plane = plane;
  myCallback->Actor = selectActor;

  vtkSmartPointer<vtkImplicitPlaneWidget> planeWidget =
    vtkSmartPointer<vtkImplicitPlaneWidget>::New();
  planeWidget->SetInteractor(iren);
  planeWidget->SetPlaceFactor(1.25);
  glyph->Update();
  planeWidget->SetInputConnection(glyph->GetOutputPort());
  planeWidget->PlaceWidget();
  planeWidget->AddObserver(vtkCommand::InteractionEvent,myCallback);

  ren1->AddActor(maceActor);
  ren1->AddActor(selectActor);

  // Add the actors to the renderer, set the background and size
  //
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // record events
  vtkSmartPointer<vtkInteractorEventRecorder> recorder =
    vtkSmartPointer<vtkInteractorEventRecorder>::New();
  recorder->SetInteractor(iren);
//  recorder->SetFileName("c:/record.log");
//  recorder->Record();
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(eventLog);

  // render the image
  //
  renWin->SetMultiSamples(0);
  iren->Initialize();
  renWin->Render();
  recorder->Play();

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();

  iren->Start();

  return EXIT_SUCCESS;
}
