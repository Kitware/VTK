/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestFinitePlaneWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the vtkFinitePlaneWidget

#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkFinitePlaneRepresentation.h>
#include <vtkFinitePlaneWidget.h>
#include <vtkInteractorEventRecorder.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkNew.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>

const char eventLog[] =
  "EnterEvent 273 40 0 0 0 0 0\n"
  "MouseMoveEvent 273 40 0 0 0 0 0\n"
  "RenderEvent 273 40 0 0 0 0 0\n"
  "RenderEvent 273 40 0 0 0 0 0\n"
  "MouseMoveEvent 200 69 0 0 0 0 0\n"
  "RenderEvent 200 69 0 0 0 0 0\n"
  "RenderEvent 200 69 0 0 0 0 0\n"
  "MouseMoveEvent 124 113 0 0 0 0 0\n"
  "RenderEvent 124 113 0 0 0 0 0\n"
  "RenderEvent 124 113 0 0 0 0 0\n"
  "MouseMoveEvent 88 137 0 0 0 0 0\n"
  "RenderEvent 88 137 0 0 0 0 0\n"
  "RenderEvent 88 137 0 0 0 0 0\n"
  "MouseMoveEvent 63 146 0 0 0 0 0\n"
  "RenderEvent 63 146 0 0 0 0 0\n"
  "RenderEvent 63 146 0 0 0 0 0\n"
  "MouseMoveEvent 59 148 0 0 0 0 0\n"
  "RenderEvent 59 148 0 0 0 0 0\n"
  "RenderEvent 59 148 0 0 0 0 0\n"
  "MouseMoveEvent 59 149 0 0 0 0 0\n"
  "RenderEvent 59 149 0 0 0 0 0\n"
  "RenderEvent 59 149 0 0 0 0 0\n"
  "MouseMoveEvent 60 150 0 0 0 0 0\n"
  "RenderEvent 60 150 0 0 0 0 0\n"
  "RenderEvent 60 150 0 0 0 0 0\n"
  "MouseMoveEvent 68 151 0 0 0 0 0\n"
  "RenderEvent 68 151 0 0 0 0 0\n"
  "RenderEvent 68 151 0 0 0 0 0\n"
  "MouseMoveEvent 88 153 0 0 0 0 0\n"
  "RenderEvent 88 153 0 0 0 0 0\n"
  "RenderEvent 88 153 0 0 0 0 0\n"
  "MouseMoveEvent 100 153 0 0 0 0 0\n"
  "RenderEvent 100 153 0 0 0 0 0\n"
  "RenderEvent 100 153 0 0 0 0 0\n"
  "MouseMoveEvent 108 152 0 0 0 0 0\n"
  "RenderEvent 108 152 0 0 0 0 0\n"
  "RenderEvent 108 152 0 0 0 0 0\n"
  "MouseMoveEvent 114 150 0 0 0 0 0\n"
  "RenderEvent 114 150 0 0 0 0 0\n"
  "RenderEvent 114 150 0 0 0 0 0\n"
  "MouseMoveEvent 121 148 0 0 0 0 0\n"
  "RenderEvent 121 148 0 0 0 0 0\n"
  "RenderEvent 121 148 0 0 0 0 0\n"
  "MouseMoveEvent 127 148 0 0 0 0 0\n"
  "RenderEvent 127 148 0 0 0 0 0\n"
  "RenderEvent 127 148 0 0 0 0 0\n"
  "MouseMoveEvent 135 148 0 0 0 0 0\n"
  "RenderEvent 135 148 0 0 0 0 0\n"
  "RenderEvent 135 148 0 0 0 0 0\n"
  "MouseMoveEvent 138 148 0 0 0 0 0\n"
  "RenderEvent 138 148 0 0 0 0 0\n"
  "RenderEvent 138 148 0 0 0 0 0\n"
  "LeftButtonPressEvent 138 148 0 0 0 0 0\n"
  "RenderEvent 138 148 0 0 0 0 0\n"
  "MouseMoveEvent 137 148 0 0 0 0 0\n"
  "RenderEvent 137 148 0 0 0 0 0\n"
  "MouseMoveEvent 136 149 0 0 0 0 0\n"
  "RenderEvent 136 149 0 0 0 0 0\n"
  "MouseMoveEvent 134 149 0 0 0 0 0\n"
  "RenderEvent 134 149 0 0 0 0 0\n"
  "MouseMoveEvent 133 149 0 0 0 0 0\n"
  "RenderEvent 133 149 0 0 0 0 0\n"
  "MouseMoveEvent 132 149 0 0 0 0 0\n"
  "RenderEvent 132 149 0 0 0 0 0\n"
  "MouseMoveEvent 130 149 0 0 0 0 0\n"
  "RenderEvent 130 149 0 0 0 0 0\n"
  "MouseMoveEvent 129 149 0 0 0 0 0\n"
  "RenderEvent 129 149 0 0 0 0 0\n"
  "MouseMoveEvent 128 149 0 0 0 0 0\n"
  "RenderEvent 128 149 0 0 0 0 0\n"
  "MouseMoveEvent 125 149 0 0 0 0 0\n"
  "RenderEvent 125 149 0 0 0 0 0\n"
  "MouseMoveEvent 123 149 0 0 0 0 0\n"
  "RenderEvent 123 149 0 0 0 0 0\n"
  "MouseMoveEvent 121 149 0 0 0 0 0\n"
  "RenderEvent 121 149 0 0 0 0 0\n"
  "MouseMoveEvent 119 149 0 0 0 0 0\n"
  "RenderEvent 119 149 0 0 0 0 0\n"
  "MouseMoveEvent 118 149 0 0 0 0 0\n"
  "RenderEvent 118 149 0 0 0 0 0\n"
  "MouseMoveEvent 116 149 0 0 0 0 0\n"
  "RenderEvent 116 149 0 0 0 0 0\n"
  "LeftButtonReleaseEvent 116 149 0 0 0 0 0\n"
  "RenderEvent 116 149 0 0 0 0 0\n"
  "MouseMoveEvent 114 149 0 0 0 0 0\n"
  "RenderEvent 114 149 0 0 0 0 0\n"
  "MouseMoveEvent 112 152 0 0 0 0 0\n"
  "RenderEvent 112 152 0 0 0 0 0\n"
  "RenderEvent 112 152 0 0 0 0 0\n"
  "MouseMoveEvent 105 162 0 0 0 0 0\n"
  "RenderEvent 105 162 0 0 0 0 0\n"
  "RenderEvent 105 162 0 0 0 0 0\n"
  "MouseMoveEvent 99 173 0 0 0 0 0\n"
  "RenderEvent 99 173 0 0 0 0 0\n"
  "RenderEvent 99 173 0 0 0 0 0\n"
  "MouseMoveEvent 96 179 0 0 0 0 0\n"
  "RenderEvent 96 179 0 0 0 0 0\n"
  "RenderEvent 96 179 0 0 0 0 0\n"
  "MouseMoveEvent 94 184 0 0 0 0 0\n"
  "RenderEvent 94 184 0 0 0 0 0\n"
  "RenderEvent 94 184 0 0 0 0 0\n"
  "MouseMoveEvent 93 187 0 0 0 0 0\n"
  "RenderEvent 93 187 0 0 0 0 0\n"
  "RenderEvent 93 187 0 0 0 0 0\n"
  "MouseMoveEvent 92 189 0 0 0 0 0\n"
  "RenderEvent 92 189 0 0 0 0 0\n"
  "RenderEvent 92 189 0 0 0 0 0\n"
  "MouseMoveEvent 91 190 0 0 0 0 0\n"
  "RenderEvent 91 190 0 0 0 0 0\n"
  "RenderEvent 91 190 0 0 0 0 0\n"
  "MouseMoveEvent 89 192 0 0 0 0 0\n"
  "RenderEvent 89 192 0 0 0 0 0\n"
  "RenderEvent 89 192 0 0 0 0 0\n"
  "MouseMoveEvent 87 196 0 0 0 0 0\n"
  "RenderEvent 87 196 0 0 0 0 0\n"
  "RenderEvent 87 196 0 0 0 0 0\n"
  "MouseMoveEvent 86 196 0 0 0 0 0\n"
  "RenderEvent 86 196 0 0 0 0 0\n"
  "RenderEvent 86 196 0 0 0 0 0\n"
  "MouseMoveEvent 85 198 0 0 0 0 0\n"
  "RenderEvent 85 198 0 0 0 0 0\n"
  "RenderEvent 85 198 0 0 0 0 0\n"
  "MouseMoveEvent 84 199 0 0 0 0 0\n"
  "RenderEvent 84 199 0 0 0 0 0\n"
  "RenderEvent 84 199 0 0 0 0 0\n"
  "MouseMoveEvent 83 200 0 0 0 0 0\n"
  "RenderEvent 83 200 0 0 0 0 0\n"
  "RenderEvent 83 200 0 0 0 0 0\n"
  "MouseMoveEvent 83 201 0 0 0 0 0\n"
  "RenderEvent 83 201 0 0 0 0 0\n"
  "RenderEvent 83 201 0 0 0 0 0\n"
  "MouseMoveEvent 83 202 0 0 0 0 0\n"
  "RenderEvent 83 202 0 0 0 0 0\n"
  "RenderEvent 83 202 0 0 0 0 0\n"
  "MouseMoveEvent 83 203 0 0 0 0 0\n"
  "RenderEvent 83 203 0 0 0 0 0\n"
  "RenderEvent 83 203 0 0 0 0 0\n"
  "MouseMoveEvent 82 204 0 0 0 0 0\n"
  "RenderEvent 82 204 0 0 0 0 0\n"
  "RenderEvent 82 204 0 0 0 0 0\n"
  "MouseMoveEvent 82 205 0 0 0 0 0\n"
  "RenderEvent 82 205 0 0 0 0 0\n"
  "RenderEvent 82 205 0 0 0 0 0\n"
  "LeftButtonPressEvent 82 205 0 0 0 0 0\n"
  "RenderEvent 82 205 0 0 0 0 0\n"
  "MouseMoveEvent 82 204 0 0 0 0 0\n"
  "RenderEvent 82 204 0 0 0 0 0\n"
  "MouseMoveEvent 83 203 0 0 0 0 0\n"
  "RenderEvent 83 203 0 0 0 0 0\n"
  "MouseMoveEvent 83 202 0 0 0 0 0\n"
  "RenderEvent 83 202 0 0 0 0 0\n"
  "MouseMoveEvent 83 201 0 0 0 0 0\n"
  "RenderEvent 83 201 0 0 0 0 0\n"
  "MouseMoveEvent 83 200 0 0 0 0 0\n"
  "RenderEvent 83 200 0 0 0 0 0\n"
  "MouseMoveEvent 83 199 0 0 0 0 0\n"
  "RenderEvent 83 199 0 0 0 0 0\n"
  "MouseMoveEvent 83 198 0 0 0 0 0\n"
  "RenderEvent 83 198 0 0 0 0 0\n"
  "MouseMoveEvent 83 197 0 0 0 0 0\n"
  "RenderEvent 83 197 0 0 0 0 0\n"
  "MouseMoveEvent 83 196 0 0 0 0 0\n"
  "RenderEvent 83 196 0 0 0 0 0\n"
  "MouseMoveEvent 83 195 0 0 0 0 0\n"
  "RenderEvent 83 195 0 0 0 0 0\n"
  "MouseMoveEvent 83 193 0 0 0 0 0\n"
  "RenderEvent 83 193 0 0 0 0 0\n"
  "MouseMoveEvent 83 192 0 0 0 0 0\n"
  "RenderEvent 83 192 0 0 0 0 0\n"
  "MouseMoveEvent 83 191 0 0 0 0 0\n"
  "RenderEvent 83 191 0 0 0 0 0\n"
  "MouseMoveEvent 83 190 0 0 0 0 0\n"
  "RenderEvent 83 190 0 0 0 0 0\n"
  "MouseMoveEvent 83 189 0 0 0 0 0\n"
  "RenderEvent 83 189 0 0 0 0 0\n"
  "MouseMoveEvent 83 188 0 0 0 0 0\n"
  "RenderEvent 83 188 0 0 0 0 0\n"
  "MouseMoveEvent 83 187 0 0 0 0 0\n"
  "RenderEvent 83 187 0 0 0 0 0\n"
  "MouseMoveEvent 83 186 0 0 0 0 0\n"
  "RenderEvent 83 186 0 0 0 0 0\n"
  "MouseMoveEvent 83 184 0 0 0 0 0\n"
  "RenderEvent 83 184 0 0 0 0 0\n"
  "MouseMoveEvent 82 182 0 0 0 0 0\n"
  "RenderEvent 82 182 0 0 0 0 0\n"
  "MouseMoveEvent 82 180 0 0 0 0 0\n"
  "RenderEvent 82 180 0 0 0 0 0\n"
  "MouseMoveEvent 82 178 0 0 0 0 0\n"
  "RenderEvent 82 178 0 0 0 0 0\n"
  "MouseMoveEvent 82 177 0 0 0 0 0\n"
  "RenderEvent 82 177 0 0 0 0 0\n"
  "MouseMoveEvent 82 176 0 0 0 0 0\n"
  "RenderEvent 82 176 0 0 0 0 0\n"
  "MouseMoveEvent 82 175 0 0 0 0 0\n"
  "RenderEvent 82 175 0 0 0 0 0\n"
  "MouseMoveEvent 82 174 0 0 0 0 0\n"
  "RenderEvent 82 174 0 0 0 0 0\n"
  "MouseMoveEvent 82 173 0 0 0 0 0\n"
  "RenderEvent 82 173 0 0 0 0 0\n"
  "LeftButtonReleaseEvent 82 173 0 0 0 0 0\n"
  "RenderEvent 82 173 0 0 0 0 0\n"
  "MouseMoveEvent 83 171 0 0 0 0 0\n"
  "RenderEvent 83 171 0 0 0 0 0\n"
  "MouseMoveEvent 85 168 0 0 0 0 0\n"
  "RenderEvent 85 168 0 0 0 0 0\n"
  "RenderEvent 85 168 0 0 0 0 0\n"
  "MouseMoveEvent 98 145 0 0 0 0 0\n"
  "RenderEvent 98 145 0 0 0 0 0\n"
  "RenderEvent 98 145 0 0 0 0 0\n"
  "MouseMoveEvent 125 104 0 0 0 0 0\n"
  "RenderEvent 125 104 0 0 0 0 0\n"
  "RenderEvent 125 104 0 0 0 0 0\n"
  "MouseMoveEvent 155 63 0 0 0 0 0\n"
  "RenderEvent 155 63 0 0 0 0 0\n"
  "RenderEvent 155 63 0 0 0 0 0\n"
  "MouseMoveEvent 179 33 0 0 0 0 0\n"
  "RenderEvent 179 33 0 0 0 0 0\n"
  "RenderEvent 179 33 0 0 0 0 0\n"
  "MouseMoveEvent 193 18 0 0 0 0 0\n"
  "RenderEvent 193 18 0 0 0 0 0\n"
  "RenderEvent 193 18 0 0 0 0 0\n"
  "MouseMoveEvent 195 13 0 0 0 0 0\n"
  "RenderEvent 195 13 0 0 0 0 0\n"
  "RenderEvent 195 13 0 0 0 0 0\n"
  "MouseMoveEvent 194 15 0 0 0 0 0\n"
  "RenderEvent 194 15 0 0 0 0 0\n"
  "RenderEvent 194 15 0 0 0 0 0\n"
  "MouseMoveEvent 191 19 0 0 0 0 0\n"
  "RenderEvent 191 19 0 0 0 0 0\n"
  "RenderEvent 191 19 0 0 0 0 0\n"
  "MouseMoveEvent 187 27 0 0 0 0 0\n"
  "RenderEvent 187 27 0 0 0 0 0\n"
  "RenderEvent 187 27 0 0 0 0 0\n"
  "MouseMoveEvent 185 29 0 0 0 0 0\n"
  "RenderEvent 185 29 0 0 0 0 0\n"
  "RenderEvent 185 29 0 0 0 0 0\n"
  "MouseMoveEvent 183 32 0 0 0 0 0\n"
  "RenderEvent 183 32 0 0 0 0 0\n"
  "RenderEvent 183 32 0 0 0 0 0\n"
  "MouseMoveEvent 183 33 0 0 0 0 0\n"
  "RenderEvent 183 33 0 0 0 0 0\n"
  "RenderEvent 183 33 0 0 0 0 0\n"
  "MouseMoveEvent 182 33 0 0 0 0 0\n"
  "RenderEvent 182 33 0 0 0 0 0\n"
  "RenderEvent 182 33 0 0 0 0 0\n"
  "MouseMoveEvent 181 36 0 0 0 0 0\n"
  "RenderEvent 181 36 0 0 0 0 0\n"
  "RenderEvent 181 36 0 0 0 0 0\n"
  "LeftButtonPressEvent 181 35 0 0 0 0 0\n"
  "StartInteractionEvent 181 35 0 0 0 0 0\n"
  "MouseMoveEvent 179 43 0 0 0 0 0\n"
  "RenderEvent 179 43 0 0 0 0 0\n"
  "MouseMoveEvent 177 47 0 0 0 0 0\n"
  "RenderEvent 177 47 0 0 0 0 0\n"
  "MouseMoveEvent 176 51 0 0 0 0 0\n"
  "RenderEvent 176 51 0 0 0 0 0\n"
  "MouseMoveEvent 174 55 0 0 0 0 0\n"
  "RenderEvent 174 55 0 0 0 0 0\n"
  "MouseMoveEvent 174 57 0 0 0 0 0\n"
  "RenderEvent 174 57 0 0 0 0 0\n"
  "MouseMoveEvent 172 59 0 0 0 0 0\n"
  "RenderEvent 172 59 0 0 0 0 0\n"
  "MouseMoveEvent 171 61 0 0 0 0 0\n"
  "RenderEvent 171 61 0 0 0 0 0\n"
  "MouseMoveEvent 170 64 0 0 0 0 0\n"
  "RenderEvent 170 64 0 0 0 0 0\n"
  "MouseMoveEvent 169 68 0 0 0 0 0\n"
  "RenderEvent 169 68 0 0 0 0 0\n"
  "MouseMoveEvent 168 70 0 0 0 0 0\n"
  "RenderEvent 168 70 0 0 0 0 0\n"
  "MouseMoveEvent 168 72 0 0 0 0 0\n"
  "RenderEvent 168 72 0 0 0 0 0\n"
  "MouseMoveEvent 167 74 0 0 0 0 0\n"
  "RenderEvent 167 74 0 0 0 0 0\n"
  "MouseMoveEvent 167 77 0 0 0 0 0\n"
  "RenderEvent 167 77 0 0 0 0 0\n"
  "MouseMoveEvent 167 79 0 0 0 0 0\n"
  "RenderEvent 167 79 0 0 0 0 0\n"
  "MouseMoveEvent 166 82 0 0 0 0 0\n"
  "RenderEvent 166 82 0 0 0 0 0\n"
  "MouseMoveEvent 165 85 0 0 0 0 0\n"
  "RenderEvent 165 85 0 0 0 0 0\n"
  "MouseMoveEvent 164 89 0 0 0 0 0\n"
  "RenderEvent 164 89 0 0 0 0 0\n"
  "MouseMoveEvent 163 91 0 0 0 0 0\n"
  "RenderEvent 163 91 0 0 0 0 0\n"
  "MouseMoveEvent 163 93 0 0 0 0 0\n"
  "RenderEvent 163 93 0 0 0 0 0\n"
  "MouseMoveEvent 163 96 0 0 0 0 0\n"
  "RenderEvent 163 96 0 0 0 0 0\n"
  "MouseMoveEvent 163 98 0 0 0 0 0\n"
  "RenderEvent 163 98 0 0 0 0 0\n"
  "MouseMoveEvent 162 100 0 0 0 0 0\n"
  "RenderEvent 162 100 0 0 0 0 0\n"
  "MouseMoveEvent 161 102 0 0 0 0 0\n"
  "RenderEvent 161 102 0 0 0 0 0\n"
  "MouseMoveEvent 161 104 0 0 0 0 0\n"
  "RenderEvent 161 104 0 0 0 0 0\n"
  "MouseMoveEvent 162 107 0 0 0 0 0\n"
  "RenderEvent 162 107 0 0 0 0 0\n"
  "MouseMoveEvent 162 108 0 0 0 0 0\n"
  "RenderEvent 162 108 0 0 0 0 0\n"
  "MouseMoveEvent 162 109 0 0 0 0 0\n"
  "RenderEvent 162 109 0 0 0 0 0\n"
  "MouseMoveEvent 162 110 0 0 0 0 0\n"
  "RenderEvent 162 110 0 0 0 0 0\n"
  "MouseMoveEvent 162 110 0 0 0 0 0\n"
  "RenderEvent 162 110 0 0 0 0 0\n"
  "MouseMoveEvent 163 110 0 0 0 0 0\n"
  "RenderEvent 163 110 0 0 0 0 0\n"
  "MouseMoveEvent 164 110 0 0 0 0 0\n"
  "RenderEvent 164 110 0 0 0 0 0\n"
  "MouseMoveEvent 165 110 0 0 0 0 0\n"
  "RenderEvent 165 110 0 0 0 0 0\n"
  "MouseMoveEvent 166 110 0 0 0 0 0\n"
  "RenderEvent 166 110 0 0 0 0 0\n"
  "MouseMoveEvent 167 109 0 0 0 0 0\n"
  "RenderEvent 167 109 0 0 0 0 0\n"
  "MouseMoveEvent 168 108 0 0 0 0 0\n"
  "RenderEvent 168 108 0 0 0 0 0\n"
  "MouseMoveEvent 169 107 0 0 0 0 0\n"
  "RenderEvent 169 107 0 0 0 0 0\n"
  "MouseMoveEvent 169 106 0 0 0 0 0\n"
  "RenderEvent 169 106 0 0 0 0 0\n"
  "LeftButtonReleaseEvent 169 106 0 0 0 0 0\n"
  "EndInteractionEvent 169 106 0 0 0 0 0\n"
  "RenderEvent 169 106 0 0 0 0 0\n"
  "MouseMoveEvent 170 106 0 0 0 0 0\n"
  "MouseMoveEvent 172 106 0 0 0 0 0\n"
  "MouseMoveEvent 175 105 0 0 0 0 0\n"
  "MouseMoveEvent 180 104 0 0 0 0 0\n"
  "MouseMoveEvent 187 102 0 0 0 0 0\n"
  "MouseMoveEvent 196 100 0 0 0 0 0\n"
  "MouseMoveEvent 202 100 0 0 0 0 0\n"
  "MouseMoveEvent 207 100 0 0 0 0 0\n"
  "MouseMoveEvent 213 102 0 0 0 0 0\n"
  "MouseMoveEvent 217 102 0 0 0 0 0\n"
  "MouseMoveEvent 220 102 0 0 0 0 0\n"
  "MouseMoveEvent 221 102 0 0 0 0 0\n"
  "MouseMoveEvent 222 102 0 0 0 0 0\n"
  "MouseMoveEvent 223 102 0 0 0 0 0\n"
  "MouseMoveEvent 224 102 0 0 0 0 0\n"
  "MouseMoveEvent 225 101 0 0 0 0 0\n"
  "MouseMoveEvent 227 101 0 0 0 0 0\n"
  "MouseMoveEvent 228 101 0 0 0 0 0\n"
  "MouseMoveEvent 229 101 0 0 0 0 0\n";

int TestFinitePlaneWidget(int, char *[])
{
  // Create a renderer, render window and interactor
  vtkNew<vtkRenderer> ren;
  ren->SetBackground(0.1, 0.2, 0.4);
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(300, 300);
  renWin->AddRenderer(ren.Get());

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.Get());

  vtkNew<vtkFinitePlaneRepresentation> fpr;
  fpr->SetTubing(true);
  fpr->SetDrawPlane(true);
  fpr->SetHandles(true);

  double bounds[6] = { 0, 1, 0, 1, 0, 1 };
  fpr->PlaceWidget(bounds);

  vtkNew<vtkFinitePlaneWidget> finitePlaneWidget;
  finitePlaneWidget->SetInteractor(iren.Get());
  finitePlaneWidget->SetRepresentation(fpr.Get());
  finitePlaneWidget->On();

  vtkNew<vtkFinitePlaneRepresentation> fpr2;
  fpr2->SetTubing(false);
  fpr2->SetDrawPlane(false);
  fpr2->SetHandles(false);

  double bounds2[6] = { 1.2, 2.2, 0, 1, 0, 1 };
  fpr2->PlaceWidget(bounds2);

  vtkNew<vtkFinitePlaneWidget> finitePlaneWidget2;
  finitePlaneWidget2->SetInteractor(iren.Get());
  finitePlaneWidget2->SetRepresentation(fpr2.Get());
  finitePlaneWidget2->On();

  renWin->SetMultiSamples(0);
  renWin->Render();
  ren->ResetCamera();
  renWin->Render();

  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->Initialize();
  iren->SetInteractorStyle(style.Get());

  vtkNew<vtkInteractorEventRecorder> recorder;
  recorder->SetInteractor(iren.Get());
#if 0
  recorder->SetFileName("./record.log");
  recorder->Record();
  recorder->On();
#else
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(eventLog);
  recorder->Play();
#endif

  iren->Start();

  recorder->Stop();

  return EXIT_SUCCESS;
}
