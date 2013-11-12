/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSliderWidget2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the vtkSliderWidget with a 2D representation.

// First include the required header files for the VTK classes we are using.
#include "vtkSmartPointer.h"

#include "vtkSliderWidget.h"
#include "vtkSliderRepresentation2D.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkAppendPolyData.h"
#include "vtkClipPolyData.h"
#include "vtkConeSource.h"
#include "vtkGlyph3D.h"
#include "vtkActor.h"
#include "vtkLODActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCommand.h"
#include "vtkSphere.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkWidgetEventTranslator.h"
#include "vtkWidgetEvent.h"

const char TestSliderWidget2DEventLog[] =
  "# StreamVersion 1\n"
  "EnterEvent 285 73 0 0 0 0 0\n"
  "MouseMoveEvent 265 83 0 0 0 0 0\n"
  "MouseMoveEvent 249 89 0 0 0 0 0\n"
  "MouseMoveEvent 233 93 0 0 0 0 0\n"
  "MouseMoveEvent 219 99 0 0 0 0 0\n"
  "MouseMoveEvent 203 105 0 0 0 0 0\n"
  "MouseMoveEvent 189 115 0 0 0 0 0\n"
  "MouseMoveEvent 175 123 0 0 0 0 0\n"
  "MouseMoveEvent 161 131 0 0 0 0 0\n"
  "MouseMoveEvent 151 133 0 0 0 0 0\n"
  "MouseMoveEvent 145 135 0 0 0 0 0\n"
  "MouseMoveEvent 144 135 0 0 0 0 0\n"
  "MouseMoveEvent 143 135 0 0 0 0 0\n"
  "MouseMoveEvent 143 134 0 0 0 0 0\n"
  "MouseMoveEvent 143 132 0 0 0 0 0\n"
  "MouseMoveEvent 143 130 0 0 0 0 0\n"
  "MouseMoveEvent 143 122 0 0 0 0 0\n"
  "MouseMoveEvent 143 114 0 0 0 0 0\n"
  "MouseMoveEvent 145 106 0 0 0 0 0\n"
  "MouseMoveEvent 149 98 0 0 0 0 0\n"
  "MouseMoveEvent 153 92 0 0 0 0 0\n"
  "MouseMoveEvent 154 90 0 0 0 0 0\n"
  "MouseMoveEvent 158 86 0 0 0 0 0\n"
  "MouseMoveEvent 159 84 0 0 0 0 0\n"
  "MouseMoveEvent 160 84 0 0 0 0 0\n"
  "MouseMoveEvent 160 83 0 0 0 0 0\n"
  "MouseMoveEvent 160 82 0 0 0 0 0\n"
  "MouseMoveEvent 160 81 0 0 0 0 0\n"
  "MouseMoveEvent 159 81 0 0 0 0 0\n"
  "MouseMoveEvent 158 80 0 0 0 0 0\n"
  "MouseMoveEvent 158 79 0 0 0 0 0\n"
  "MouseMoveEvent 158 77 0 0 0 0 0\n"
  "MouseMoveEvent 158 76 0 0 0 0 0\n"
  "MouseMoveEvent 158 74 0 0 0 0 0\n"
  "MouseMoveEvent 157 73 0 0 0 0 0\n"
  "MouseMoveEvent 157 71 0 0 0 0 0\n"
  "MouseMoveEvent 156 69 0 0 0 0 0\n"
  "MouseMoveEvent 155 67 0 0 0 0 0\n"
  "MouseMoveEvent 151 63 0 0 0 0 0\n"
  "MouseMoveEvent 150 62 0 0 0 0 0\n"
  "MouseMoveEvent 149 61 0 0 0 0 0\n"
  "MouseMoveEvent 148 60 0 0 0 0 0\n"
  "MouseMoveEvent 147 59 0 0 0 0 0\n"
  "MouseMoveEvent 147 58 0 0 0 0 0\n"
  "MouseMoveEvent 146 58 0 0 0 0 0\n"
  "MouseMoveEvent 145 57 0 0 0 0 0\n"
  "MouseMoveEvent 143 57 0 0 0 0 0\n"
  "MouseMoveEvent 140 57 0 0 0 0 0\n"
  "MouseMoveEvent 138 57 0 0 0 0 0\n"
  "MouseMoveEvent 136 57 0 0 0 0 0\n"
  "MouseMoveEvent 135 57 0 0 0 0 0\n"
  "MouseMoveEvent 133 57 0 0 0 0 0\n"
  "MouseMoveEvent 132 57 0 0 0 0 0\n"
  "MouseMoveEvent 131 57 0 0 0 0 0\n"
  "MouseMoveEvent 130 57 0 0 0 0 0\n"
  "MouseMoveEvent 129 57 0 0 0 0 0\n"
  "MouseMoveEvent 128 57 0 0 0 0 0\n"
  "MouseMoveEvent 127 57 0 0 0 0 0\n"
  "MouseMoveEvent 127 56 0 0 0 0 0\n"
  "MouseMoveEvent 126 56 0 0 0 0 0\n"
  "MouseMoveEvent 126 55 0 0 0 0 0\n"
  "MouseMoveEvent 126 54 0 0 0 0 0\n"
  "MouseMoveEvent 125 54 0 0 0 0 0\n"
  "MouseMoveEvent 124 53 0 0 0 0 0\n"
  "MouseMoveEvent 123 52 0 0 0 0 0\n"
  "MouseMoveEvent 121 51 0 0 0 0 0\n"
  "MouseMoveEvent 120 50 0 0 0 0 0\n"
  "MouseMoveEvent 119 50 0 0 0 0 0\n"
  "MouseMoveEvent 117 49 0 0 0 0 0\n"
  "MouseMoveEvent 117 48 0 0 0 0 0\n"
  "MouseMoveEvent 116 47 0 0 0 0 0\n"
  "MouseMoveEvent 115 47 0 0 0 0 0\n"
  "MouseMoveEvent 115 46 0 0 0 0 0\n"
  "MouseMoveEvent 114 46 0 0 0 0 0\n"
  "MouseMoveEvent 113 45 0 0 0 0 0\n"
  "MouseMoveEvent 112 45 0 0 0 0 0\n"
  "MouseMoveEvent 111 45 0 0 0 0 0\n"
  "MouseMoveEvent 111 44 0 0 0 0 0\n"
  "MouseMoveEvent 110 44 0 0 0 0 0\n"
  "MouseMoveEvent 109 44 0 0 0 0 0\n"
  "MouseMoveEvent 108 43 0 0 0 0 0\n"
  "MouseMoveEvent 107 43 0 0 0 0 0\n"
  "MouseMoveEvent 106 42 0 0 0 0 0\n"
  "MouseMoveEvent 106 41 0 0 0 0 0\n"
  "MouseMoveEvent 106 40 0 0 0 0 0\n"
  "MouseMoveEvent 107 40 0 0 0 0 0\n"
  "MouseMoveEvent 107 39 0 0 0 0 0\n"
  "MouseMoveEvent 107 38 0 0 0 0 0\n"
  "MouseMoveEvent 107 37 0 0 0 0 0\n"
  "MouseMoveEvent 107 36 0 0 0 0 0\n"
  "MouseMoveEvent 107 35 0 0 0 0 0\n"
  "MouseMoveEvent 107 34 0 0 0 0 0\n"
  "LeftButtonPressEvent 107 34 0 0 0 0 0\n"
  "RenderEvent 107 34 0 0 0 0 0\n"
  "MouseMoveEvent 108 33 0 0 0 0 0\n"
  "RenderEvent 108 33 0 0 0 0 0\n"
  "MouseMoveEvent 109 33 0 0 0 0 0\n"
  "RenderEvent 109 33 0 0 0 0 0\n"
  "MouseMoveEvent 110 33 0 0 0 0 0\n"
  "RenderEvent 110 33 0 0 0 0 0\n"
  "MouseMoveEvent 111 33 0 0 0 0 0\n"
  "RenderEvent 111 33 0 0 0 0 0\n"
  "MouseMoveEvent 112 33 0 0 0 0 0\n"
  "RenderEvent 112 33 0 0 0 0 0\n"
  "MouseMoveEvent 113 33 0 0 0 0 0\n"
  "RenderEvent 113 33 0 0 0 0 0\n"
  "MouseMoveEvent 114 33 0 0 0 0 0\n"
  "RenderEvent 114 33 0 0 0 0 0\n"
  "MouseMoveEvent 115 33 0 0 0 0 0\n"
  "RenderEvent 115 33 0 0 0 0 0\n"
  "MouseMoveEvent 116 33 0 0 0 0 0\n"
  "RenderEvent 116 33 0 0 0 0 0\n"
  "MouseMoveEvent 117 33 0 0 0 0 0\n"
  "RenderEvent 117 33 0 0 0 0 0\n"
  "MouseMoveEvent 118 33 0 0 0 0 0\n"
  "RenderEvent 118 33 0 0 0 0 0\n"
  "MouseMoveEvent 120 33 0 0 0 0 0\n"
  "RenderEvent 120 33 0 0 0 0 0\n"
  "MouseMoveEvent 121 33 0 0 0 0 0\n"
  "RenderEvent 121 33 0 0 0 0 0\n"
  "MouseMoveEvent 123 33 0 0 0 0 0\n"
  "RenderEvent 123 33 0 0 0 0 0\n"
  "MouseMoveEvent 124 33 0 0 0 0 0\n"
  "RenderEvent 124 33 0 0 0 0 0\n"
  "MouseMoveEvent 127 33 0 0 0 0 0\n"
  "RenderEvent 127 33 0 0 0 0 0\n"
  "MouseMoveEvent 128 33 0 0 0 0 0\n"
  "RenderEvent 128 33 0 0 0 0 0\n"
  "MouseMoveEvent 130 33 0 0 0 0 0\n"
  "RenderEvent 130 33 0 0 0 0 0\n"
  "MouseMoveEvent 132 33 0 0 0 0 0\n"
  "RenderEvent 132 33 0 0 0 0 0\n"
  "MouseMoveEvent 134 33 0 0 0 0 0\n"
  "RenderEvent 134 33 0 0 0 0 0\n"
  "MouseMoveEvent 136 33 0 0 0 0 0\n"
  "RenderEvent 136 33 0 0 0 0 0\n"
  "MouseMoveEvent 138 33 0 0 0 0 0\n"
  "RenderEvent 138 33 0 0 0 0 0\n"
  "MouseMoveEvent 139 33 0 0 0 0 0\n"
  "RenderEvent 139 33 0 0 0 0 0\n"
  "MouseMoveEvent 141 33 0 0 0 0 0\n"
  "RenderEvent 141 33 0 0 0 0 0\n"
  "MouseMoveEvent 143 33 0 0 0 0 0\n"
  "RenderEvent 143 33 0 0 0 0 0\n"
  "MouseMoveEvent 144 33 0 0 0 0 0\n"
  "RenderEvent 144 33 0 0 0 0 0\n"
  "MouseMoveEvent 145 33 0 0 0 0 0\n"
  "RenderEvent 145 33 0 0 0 0 0\n"
  "MouseMoveEvent 146 33 0 0 0 0 0\n"
  "RenderEvent 146 33 0 0 0 0 0\n"
  "MouseMoveEvent 147 33 0 0 0 0 0\n"
  "RenderEvent 147 33 0 0 0 0 0\n"
  "MouseMoveEvent 149 33 0 0 0 0 0\n"
  "RenderEvent 149 33 0 0 0 0 0\n"
  "MouseMoveEvent 150 33 0 0 0 0 0\n"
  "RenderEvent 150 33 0 0 0 0 0\n"
  "MouseMoveEvent 152 33 0 0 0 0 0\n"
  "RenderEvent 152 33 0 0 0 0 0\n"
  "MouseMoveEvent 153 33 0 0 0 0 0\n"
  "RenderEvent 153 33 0 0 0 0 0\n"
  "MouseMoveEvent 155 33 0 0 0 0 0\n"
  "RenderEvent 155 33 0 0 0 0 0\n"
  "MouseMoveEvent 156 33 0 0 0 0 0\n"
  "RenderEvent 156 33 0 0 0 0 0\n"
  "MouseMoveEvent 157 33 0 0 0 0 0\n"
  "RenderEvent 157 33 0 0 0 0 0\n"
  "MouseMoveEvent 159 33 0 0 0 0 0\n"
  "RenderEvent 159 33 0 0 0 0 0\n"
  "MouseMoveEvent 160 33 0 0 0 0 0\n"
  "RenderEvent 160 33 0 0 0 0 0\n"
  "MouseMoveEvent 161 33 0 0 0 0 0\n"
  "RenderEvent 161 33 0 0 0 0 0\n"
  "MouseMoveEvent 162 33 0 0 0 0 0\n"
  "RenderEvent 162 33 0 0 0 0 0\n"
  "MouseMoveEvent 163 33 0 0 0 0 0\n"
  "RenderEvent 163 33 0 0 0 0 0\n"
  "MouseMoveEvent 164 33 0 0 0 0 0\n"
  "RenderEvent 164 33 0 0 0 0 0\n"
  "MouseMoveEvent 165 33 0 0 0 0 0\n"
  "RenderEvent 165 33 0 0 0 0 0\n"
  "MouseMoveEvent 166 33 0 0 0 0 0\n"
  "RenderEvent 166 33 0 0 0 0 0\n"
  "MouseMoveEvent 168 33 0 0 0 0 0\n"
  "RenderEvent 168 33 0 0 0 0 0\n"
  "MouseMoveEvent 169 33 0 0 0 0 0\n"
  "RenderEvent 169 33 0 0 0 0 0\n"
  "MouseMoveEvent 170 33 0 0 0 0 0\n"
  "RenderEvent 170 33 0 0 0 0 0\n"
  "MouseMoveEvent 171 33 0 0 0 0 0\n"
  "RenderEvent 171 33 0 0 0 0 0\n"
  "MouseMoveEvent 172 33 0 0 0 0 0\n"
  "RenderEvent 172 33 0 0 0 0 0\n"
  "MouseMoveEvent 173 32 0 0 0 0 0\n"
  "RenderEvent 173 32 0 0 0 0 0\n"
  "MouseMoveEvent 174 32 0 0 0 0 0\n"
  "RenderEvent 174 32 0 0 0 0 0\n"
  "MouseMoveEvent 175 32 0 0 0 0 0\n"
  "RenderEvent 175 32 0 0 0 0 0\n"
  "MouseMoveEvent 176 31 0 0 0 0 0\n"
  "RenderEvent 176 31 0 0 0 0 0\n"
  "MouseMoveEvent 177 31 0 0 0 0 0\n"
  "RenderEvent 177 31 0 0 0 0 0\n"
  "MouseMoveEvent 178 31 0 0 0 0 0\n"
  "RenderEvent 178 31 0 0 0 0 0\n"
  "MouseMoveEvent 179 31 0 0 0 0 0\n"
  "RenderEvent 179 31 0 0 0 0 0\n"
  "MouseMoveEvent 180 31 0 0 0 0 0\n"
  "RenderEvent 180 31 0 0 0 0 0\n"
  "MouseMoveEvent 181 31 0 0 0 0 0\n"
  "RenderEvent 181 31 0 0 0 0 0\n"
  "MouseMoveEvent 182 31 0 0 0 0 0\n"
  "RenderEvent 182 31 0 0 0 0 0\n"
  "MouseMoveEvent 183 31 0 0 0 0 0\n"
  "RenderEvent 183 31 0 0 0 0 0\n"
  "MouseMoveEvent 184 31 0 0 0 0 0\n"
  "RenderEvent 184 31 0 0 0 0 0\n"
  "MouseMoveEvent 185 31 0 0 0 0 0\n"
  "RenderEvent 185 31 0 0 0 0 0\n"
  "MouseMoveEvent 187 31 0 0 0 0 0\n"
  "RenderEvent 187 31 0 0 0 0 0\n"
  "MouseMoveEvent 188 31 0 0 0 0 0\n"
  "RenderEvent 188 31 0 0 0 0 0\n"
  "MouseMoveEvent 190 31 0 0 0 0 0\n"
  "RenderEvent 190 31 0 0 0 0 0\n"
  "MouseMoveEvent 191 31 0 0 0 0 0\n"
  "RenderEvent 191 31 0 0 0 0 0\n"
  "MouseMoveEvent 192 31 0 0 0 0 0\n"
  "RenderEvent 192 31 0 0 0 0 0\n"
  "MouseMoveEvent 193 31 0 0 0 0 0\n"
  "RenderEvent 193 31 0 0 0 0 0\n"
  "MouseMoveEvent 194 31 0 0 0 0 0\n"
  "RenderEvent 194 31 0 0 0 0 0\n"
  "MouseMoveEvent 195 31 0 0 0 0 0\n"
  "RenderEvent 195 31 0 0 0 0 0\n"
  "MouseMoveEvent 196 31 0 0 0 0 0\n"
  "RenderEvent 196 31 0 0 0 0 0\n"
  "MouseMoveEvent 197 31 0 0 0 0 0\n"
  "RenderEvent 197 31 0 0 0 0 0\n"
  "LeftButtonReleaseEvent 197 31 0 0 0 0 0\n"
  "RenderEvent 197 31 0 0 0 0 0\n"
  "KeyPressEvent 197 31 0 0 119 1 w\n"
  "CharEvent 197 31 0 0 119 1 w\n"
  "RenderEvent 197 31 0 0 119 1 w\n"
  "KeyReleaseEvent 197 31 0 0 119 1 w\n"
  "LeftButtonPressEvent 197 31 0 0 0 0 w\n"
  "RenderEvent 197 31 0 0 0 0 w\n"
  "MouseMoveEvent 196 31 0 0 0 0 w\n"
  "RenderEvent 196 31 0 0 0 0 w\n"
  "MouseMoveEvent 194 31 0 0 0 0 w\n"
  "RenderEvent 194 31 0 0 0 0 w\n"
  "MouseMoveEvent 191 31 0 0 0 0 w\n"
  "RenderEvent 191 31 0 0 0 0 w\n"
  "MouseMoveEvent 189 31 0 0 0 0 w\n"
  "RenderEvent 189 31 0 0 0 0 w\n"
  "MouseMoveEvent 186 31 0 0 0 0 w\n"
  "RenderEvent 186 31 0 0 0 0 w\n"
  "MouseMoveEvent 184 31 0 0 0 0 w\n"
  "RenderEvent 184 31 0 0 0 0 w\n"
  "MouseMoveEvent 181 31 0 0 0 0 w\n"
  "RenderEvent 181 31 0 0 0 0 w\n"
  "MouseMoveEvent 178 31 0 0 0 0 w\n"
  "RenderEvent 178 31 0 0 0 0 w\n"
  "MouseMoveEvent 175 31 0 0 0 0 w\n"
  "RenderEvent 175 31 0 0 0 0 w\n"
  "MouseMoveEvent 167 31 0 0 0 0 w\n"
  "RenderEvent 167 31 0 0 0 0 w\n"
  "MouseMoveEvent 164 31 0 0 0 0 w\n"
  "RenderEvent 164 31 0 0 0 0 w\n"
  "MouseMoveEvent 161 31 0 0 0 0 w\n"
  "RenderEvent 161 31 0 0 0 0 w\n"
  "MouseMoveEvent 155 33 0 0 0 0 w\n"
  "RenderEvent 155 33 0 0 0 0 w\n"
  "MouseMoveEvent 153 33 0 0 0 0 w\n"
  "RenderEvent 153 33 0 0 0 0 w\n"
  "MouseMoveEvent 151 33 0 0 0 0 w\n"
  "RenderEvent 151 33 0 0 0 0 w\n"
  "MouseMoveEvent 149 33 0 0 0 0 w\n"
  "RenderEvent 149 33 0 0 0 0 w\n"
  "MouseMoveEvent 146 33 0 0 0 0 w\n"
  "RenderEvent 146 33 0 0 0 0 w\n"
  "MouseMoveEvent 144 33 0 0 0 0 w\n"
  "RenderEvent 144 33 0 0 0 0 w\n"
  "MouseMoveEvent 142 33 0 0 0 0 w\n"
  "RenderEvent 142 33 0 0 0 0 w\n"
  "MouseMoveEvent 141 33 0 0 0 0 w\n"
  "RenderEvent 141 33 0 0 0 0 w\n"
  "MouseMoveEvent 140 33 0 0 0 0 w\n"
  "RenderEvent 140 33 0 0 0 0 w\n"
  "MouseMoveEvent 139 33 0 0 0 0 w\n"
  "RenderEvent 139 33 0 0 0 0 w\n"
  "MouseMoveEvent 138 33 0 0 0 0 w\n"
  "RenderEvent 138 33 0 0 0 0 w\n"
  "MouseMoveEvent 137 33 0 0 0 0 w\n"
  "RenderEvent 137 33 0 0 0 0 w\n"
  "MouseMoveEvent 136 33 0 0 0 0 w\n"
  "RenderEvent 136 33 0 0 0 0 w\n"
  "MouseMoveEvent 135 33 0 0 0 0 w\n"
  "RenderEvent 135 33 0 0 0 0 w\n"
  "MouseMoveEvent 132 33 0 0 0 0 w\n"
  "RenderEvent 132 33 0 0 0 0 w\n"
  "MouseMoveEvent 131 33 0 0 0 0 w\n"
  "RenderEvent 131 33 0 0 0 0 w\n"
  "MouseMoveEvent 130 33 0 0 0 0 w\n"
  "RenderEvent 130 33 0 0 0 0 w\n"
  "MouseMoveEvent 128 33 0 0 0 0 w\n"
  "RenderEvent 128 33 0 0 0 0 w\n"
  "MouseMoveEvent 127 33 0 0 0 0 w\n"
  "RenderEvent 127 33 0 0 0 0 w\n"
  "MouseMoveEvent 126 33 0 0 0 0 w\n"
  "RenderEvent 126 33 0 0 0 0 w\n"
  "MouseMoveEvent 124 33 0 0 0 0 w\n"
  "RenderEvent 124 33 0 0 0 0 w\n"
  "MouseMoveEvent 123 33 0 0 0 0 w\n"
  "RenderEvent 123 33 0 0 0 0 w\n"
  "MouseMoveEvent 122 33 0 0 0 0 w\n"
  "RenderEvent 122 33 0 0 0 0 w\n"
  "MouseMoveEvent 121 33 0 0 0 0 w\n"
  "RenderEvent 121 33 0 0 0 0 w\n"
  "MouseMoveEvent 120 33 0 0 0 0 w\n"
  "RenderEvent 120 33 0 0 0 0 w\n"
  "MouseMoveEvent 118 33 0 0 0 0 w\n"
  "RenderEvent 118 33 0 0 0 0 w\n"
  "MouseMoveEvent 117 33 0 0 0 0 w\n"
  "RenderEvent 117 33 0 0 0 0 w\n"
  "MouseMoveEvent 116 33 0 0 0 0 w\n"
  "RenderEvent 116 33 0 0 0 0 w\n"
  "MouseMoveEvent 115 33 0 0 0 0 w\n"
  "RenderEvent 115 33 0 0 0 0 w\n"
  "MouseMoveEvent 114 33 0 0 0 0 w\n"
  "RenderEvent 114 33 0 0 0 0 w\n"
  "MouseMoveEvent 113 33 0 0 0 0 w\n"
  "RenderEvent 113 33 0 0 0 0 w\n"
  "MouseMoveEvent 111 33 0 0 0 0 w\n"
  "RenderEvent 111 33 0 0 0 0 w\n"
  "MouseMoveEvent 109 33 0 0 0 0 w\n"
  "RenderEvent 109 33 0 0 0 0 w\n"
  "MouseMoveEvent 108 33 0 0 0 0 w\n"
  "RenderEvent 108 33 0 0 0 0 w\n"
  "MouseMoveEvent 107 32 0 0 0 0 w\n"
  "RenderEvent 107 32 0 0 0 0 w\n"
  "MouseMoveEvent 105 32 0 0 0 0 w\n"
  "RenderEvent 105 32 0 0 0 0 w\n"
  "MouseMoveEvent 104 32 0 0 0 0 w\n"
  "RenderEvent 104 32 0 0 0 0 w\n"
  "MouseMoveEvent 103 32 0 0 0 0 w\n"
  "RenderEvent 103 32 0 0 0 0 w\n"
  "MouseMoveEvent 102 32 0 0 0 0 w\n"
  "RenderEvent 102 32 0 0 0 0 w\n"
  "MouseMoveEvent 101 32 0 0 0 0 w\n"
  "RenderEvent 101 32 0 0 0 0 w\n"
  "MouseMoveEvent 100 32 0 0 0 0 w\n"
  "RenderEvent 100 32 0 0 0 0 w\n"
  "LeftButtonReleaseEvent 100 32 0 0 0 0 w\n"
  "RenderEvent 100 32 0 0 0 0 w\n"
  "KeyPressEvent 100 32 0 0 115 1 s\n"
  "CharEvent 100 32 0 0 115 1 s\n"
  "RenderEvent 100 32 0 0 115 1 s\n"
  "KeyReleaseEvent 100 32 0 0 115 1 s\n"
  "LeftButtonPressEvent 100 32 0 0 0 0 s\n"
  "RenderEvent 100 32 0 0 0 0 s\n"
  "MouseMoveEvent 101 32 0 0 0 0 s\n"
  "RenderEvent 101 32 0 0 0 0 s\n"
  "MouseMoveEvent 102 32 0 0 0 0 s\n"
  "RenderEvent 102 32 0 0 0 0 s\n"
  "MouseMoveEvent 104 32 0 0 0 0 s\n"
  "RenderEvent 104 32 0 0 0 0 s\n"
  "MouseMoveEvent 106 32 0 0 0 0 s\n"
  "RenderEvent 106 32 0 0 0 0 s\n"
  "MouseMoveEvent 108 32 0 0 0 0 s\n"
  "RenderEvent 108 32 0 0 0 0 s\n"
  "MouseMoveEvent 111 32 0 0 0 0 s\n"
  "RenderEvent 111 32 0 0 0 0 s\n"
  "MouseMoveEvent 119 32 0 0 0 0 s\n"
  "RenderEvent 119 32 0 0 0 0 s\n"
  "MouseMoveEvent 122 32 0 0 0 0 s\n"
  "RenderEvent 122 32 0 0 0 0 s\n"
  "MouseMoveEvent 125 32 0 0 0 0 s\n"
  "RenderEvent 125 32 0 0 0 0 s\n"
  "MouseMoveEvent 133 32 0 0 0 0 s\n"
  "RenderEvent 133 32 0 0 0 0 s\n"
  "MouseMoveEvent 141 32 0 0 0 0 s\n"
  "RenderEvent 141 32 0 0 0 0 s\n"
  "MouseMoveEvent 144 32 0 0 0 0 s\n"
  "RenderEvent 144 32 0 0 0 0 s\n"
  "MouseMoveEvent 152 32 0 0 0 0 s\n"
  "RenderEvent 152 32 0 0 0 0 s\n"
  "MouseMoveEvent 154 32 0 0 0 0 s\n"
  "RenderEvent 154 32 0 0 0 0 s\n"
  "MouseMoveEvent 156 32 0 0 0 0 s\n"
  "RenderEvent 156 32 0 0 0 0 s\n"
  "MouseMoveEvent 157 32 0 0 0 0 s\n"
  "RenderEvent 157 32 0 0 0 0 s\n"
  "MouseMoveEvent 158 32 0 0 0 0 s\n"
  "RenderEvent 158 32 0 0 0 0 s\n"
  "MouseMoveEvent 160 32 0 0 0 0 s\n"
  "RenderEvent 160 32 0 0 0 0 s\n"
  "MouseMoveEvent 161 32 0 0 0 0 s\n"
  "RenderEvent 161 32 0 0 0 0 s\n"
  "MouseMoveEvent 163 32 0 0 0 0 s\n"
  "RenderEvent 163 32 0 0 0 0 s\n"
  "MouseMoveEvent 164 32 0 0 0 0 s\n"
  "RenderEvent 164 32 0 0 0 0 s\n"
  "MouseMoveEvent 165 32 0 0 0 0 s\n"
  "RenderEvent 165 32 0 0 0 0 s\n"
  "MouseMoveEvent 166 32 0 0 0 0 s\n"
  "RenderEvent 166 32 0 0 0 0 s\n"
  "MouseMoveEvent 168 32 0 0 0 0 s\n"
  "RenderEvent 168 32 0 0 0 0 s\n"
  "MouseMoveEvent 169 32 0 0 0 0 s\n"
  "RenderEvent 169 32 0 0 0 0 s\n"
  "MouseMoveEvent 170 32 0 0 0 0 s\n"
  "RenderEvent 170 32 0 0 0 0 s\n"
  "MouseMoveEvent 171 31 0 0 0 0 s\n"
  "RenderEvent 171 31 0 0 0 0 s\n"
  "MouseMoveEvent 173 31 0 0 0 0 s\n"
  "RenderEvent 173 31 0 0 0 0 s\n"
  "MouseMoveEvent 174 31 0 0 0 0 s\n"
  "RenderEvent 174 31 0 0 0 0 s\n"
  "MouseMoveEvent 176 31 0 0 0 0 s\n"
  "RenderEvent 176 31 0 0 0 0 s\n"
  "MouseMoveEvent 177 31 0 0 0 0 s\n"
  "RenderEvent 177 31 0 0 0 0 s\n"
  "MouseMoveEvent 179 31 0 0 0 0 s\n"
  "RenderEvent 179 31 0 0 0 0 s\n"
  "MouseMoveEvent 181 31 0 0 0 0 s\n"
  "RenderEvent 181 31 0 0 0 0 s\n"
  "MouseMoveEvent 183 31 0 0 0 0 s\n"
  "RenderEvent 183 31 0 0 0 0 s\n"
  "MouseMoveEvent 185 31 0 0 0 0 s\n"
  "RenderEvent 185 31 0 0 0 0 s\n"
  "MouseMoveEvent 186 31 0 0 0 0 s\n"
  "RenderEvent 186 31 0 0 0 0 s\n"
  "MouseMoveEvent 188 31 0 0 0 0 s\n"
  "RenderEvent 188 31 0 0 0 0 s\n"
  "MouseMoveEvent 189 31 0 0 0 0 s\n"
  "RenderEvent 189 31 0 0 0 0 s\n"
  "MouseMoveEvent 190 31 0 0 0 0 s\n"
  "RenderEvent 190 31 0 0 0 0 s\n"
  "MouseMoveEvent 190 32 0 0 0 0 s\n"
  "RenderEvent 190 32 0 0 0 0 s\n"
  "MouseMoveEvent 191 32 0 0 0 0 s\n"
  "RenderEvent 191 32 0 0 0 0 s\n"
  "MouseMoveEvent 192 32 0 0 0 0 s\n"
  "RenderEvent 192 32 0 0 0 0 s\n"
  "MouseMoveEvent 193 32 0 0 0 0 s\n"
  "RenderEvent 193 32 0 0 0 0 s\n"
  "MouseMoveEvent 194 32 0 0 0 0 s\n"
  "RenderEvent 194 32 0 0 0 0 s\n"
  "MouseMoveEvent 195 32 0 0 0 0 s\n"
  "RenderEvent 195 32 0 0 0 0 s\n"
  "MouseMoveEvent 196 32 0 0 0 0 s\n"
  "RenderEvent 196 32 0 0 0 0 s\n"
  "MouseMoveEvent 197 32 0 0 0 0 s\n"
  "RenderEvent 197 32 0 0 0 0 s\n"
  "LeftButtonReleaseEvent 197 32 0 0 0 0 s\n"
  "RenderEvent 197 32 0 0 0 0 s\n"
  "ExitEvent 197 32 0 0 113 1 q\n"
  ;


// This does the actual work: updates the probe.
// Callback for the interaction
class vtkSlider2DCallback : public vtkCommand
{
public:
  static vtkSlider2DCallback *New()
  { return new vtkSlider2DCallback; }
  virtual void Execute(vtkObject *caller, unsigned long, void*)
  {
    vtkSliderWidget *sliderWidget =
      reinterpret_cast<vtkSliderWidget*>(caller);
    this->Glyph->SetScaleFactor(static_cast<vtkSliderRepresentation *>(sliderWidget->GetRepresentation())->GetValue());
  }
  vtkSlider2DCallback():Glyph(0) {}
  vtkGlyph3D *Glyph;
};

int TestSliderWidget2D(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  // Create a mace out of filters.
  //
  vtkSmartPointer<vtkSphereSource> sphereSource =
    vtkSmartPointer<vtkSphereSource>::New();
  vtkSmartPointer<vtkConeSource> cone =
    vtkSmartPointer<vtkConeSource>::New();
  vtkSmartPointer<vtkGlyph3D> glyph =
    vtkSmartPointer<vtkGlyph3D>::New();
  glyph->SetInputConnection(sphereSource->GetOutputPort());
  glyph->SetSourceConnection(cone->GetOutputPort());
  glyph->SetVectorModeToUseNormal();
  glyph->SetScaleModeToScaleByVector();
  glyph->SetScaleFactor(0.25);

  // The sphere and spikes are appended into a single polydata.
  // This just makes things simpler to manage.
  vtkSmartPointer<vtkAppendPolyData> apd =
    vtkSmartPointer<vtkAppendPolyData>::New();
  apd->AddInputConnection(glyph->GetOutputPort());
  apd->AddInputConnection(sphereSource->GetOutputPort());

  vtkSmartPointer<vtkPolyDataMapper> maceMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  maceMapper->SetInputConnection(apd->GetOutputPort());

  vtkSmartPointer<vtkLODActor> maceActor =
    vtkSmartPointer<vtkLODActor>::New();
  maceActor->SetMapper(maceMapper);
  maceActor->VisibilityOn();
  maceActor->SetPosition(1,1,1);

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

  // VTK widgets consist of two parts: the widget part that handles event processing;
  // and the widget representation that defines how the widget appears in the scene
  // (i.e., matters pertaining to geometry).
  vtkSmartPointer<vtkSliderRepresentation2D> sliderRep =
    vtkSmartPointer<vtkSliderRepresentation2D>::New();
  sliderRep->SetValue(0.25);
  sliderRep->SetTitleText("Spike Size");
  sliderRep->GetPoint1Coordinate()->SetCoordinateSystemToNormalizedDisplay();
  sliderRep->GetPoint1Coordinate()->SetValue(0.2,0.1);
  sliderRep->GetPoint2Coordinate()->SetCoordinateSystemToNormalizedDisplay();
  sliderRep->GetPoint2Coordinate()->SetValue(0.8,0.1);
  sliderRep->SetSliderLength(0.02);
  sliderRep->SetSliderWidth(0.03);
  sliderRep->SetEndCapLength(0.01);
  sliderRep->SetEndCapWidth(0.03);
  sliderRep->SetTubeWidth(0.005);

  vtkSmartPointer<vtkSliderWidget> sliderWidget =
    vtkSmartPointer<vtkSliderWidget>::New();
  sliderWidget->SetInteractor(iren);
  sliderWidget->SetRepresentation(sliderRep);
  sliderWidget->SetAnimationModeToAnimate();

  vtkSmartPointer<vtkSlider2DCallback> callback =
    vtkSmartPointer<vtkSlider2DCallback>::New();
  callback->Glyph = glyph;
  sliderWidget->AddObserver(vtkCommand::InteractionEvent,callback);
  ren1->AddActor(maceActor);
  sliderWidget->EnabledOn();

  // Add the actors to the renderer, set the background and size
  //
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // record events
  vtkSmartPointer<vtkInteractorEventRecorder> recorder =
    vtkSmartPointer<vtkInteractorEventRecorder>::New();
  recorder->SetInteractor(iren);
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(TestSliderWidget2DEventLog);

  // render the image
  //
  iren->Initialize();
  renWin->Render();
  recorder->Play();

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();

  iren->Start();

  return EXIT_SUCCESS;

}
