/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSliderWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the vtkSliderWidget.

// First include the required header files for the VTK classes we are using.
#include "vtkSmartPointer.h"

#include "vtkSliderWidget.h"
#include "vtkSliderRepresentation3D.h"
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
#include "vtkProperty.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkPolyData.h"
#include "vtkLineSource.h"
#include "vtkWidgetEventTranslator.h"
#include "vtkWidgetEvent.h"


char TestSliderWidgetEventLog[] =
  "# StreamVersion 1\n"
  "EnterEvent 294 33 0 0 0 0 0\n"
  "MouseMoveEvent 286 41 0 0 0 0 0\n"
  "MouseMoveEvent 280 47 0 0 0 0 0\n"
  "MouseMoveEvent 276 55 0 0 0 0 0\n"
  "MouseMoveEvent 268 61 0 0 0 0 0\n"
  "MouseMoveEvent 264 69 0 0 0 0 0\n"
  "MouseMoveEvent 258 73 0 0 0 0 0\n"
  "MouseMoveEvent 254 77 0 0 0 0 0\n"
  "MouseMoveEvent 253 79 0 0 0 0 0\n"
  "MouseMoveEvent 245 85 0 0 0 0 0\n"
  "MouseMoveEvent 239 87 0 0 0 0 0\n"
  "MouseMoveEvent 237 88 0 0 0 0 0\n"
  "MouseMoveEvent 233 92 0 0 0 0 0\n"
  "MouseMoveEvent 231 92 0 0 0 0 0\n"
  "MouseMoveEvent 229 93 0 0 0 0 0\n"
  "MouseMoveEvent 227 94 0 0 0 0 0\n"
  "MouseMoveEvent 225 95 0 0 0 0 0\n"
  "MouseMoveEvent 223 95 0 0 0 0 0\n"
  "MouseMoveEvent 217 97 0 0 0 0 0\n"
  "MouseMoveEvent 214 97 0 0 0 0 0\n"
  "MouseMoveEvent 211 97 0 0 0 0 0\n"
  "MouseMoveEvent 203 97 0 0 0 0 0\n"
  "MouseMoveEvent 195 97 0 0 0 0 0\n"
  "MouseMoveEvent 192 97 0 0 0 0 0\n"
  "MouseMoveEvent 189 97 0 0 0 0 0\n"
  "MouseMoveEvent 181 97 0 0 0 0 0\n"
  "MouseMoveEvent 178 97 0 0 0 0 0\n"
  "MouseMoveEvent 175 97 0 0 0 0 0\n"
  "MouseMoveEvent 173 97 0 0 0 0 0\n"
  "MouseMoveEvent 171 98 0 0 0 0 0\n"
  "MouseMoveEvent 169 98 0 0 0 0 0\n"
  "MouseMoveEvent 167 98 0 0 0 0 0\n"
  "MouseMoveEvent 166 98 0 0 0 0 0\n"
  "MouseMoveEvent 165 98 0 0 0 0 0\n"
  "MouseMoveEvent 165 99 0 0 0 0 0\n"
  "MouseMoveEvent 164 99 0 0 0 0 0\n"
  "MouseMoveEvent 161 99 0 0 0 0 0\n"
  "MouseMoveEvent 158 99 0 0 0 0 0\n"
  "MouseMoveEvent 155 99 0 0 0 0 0\n"
  "MouseMoveEvent 152 99 0 0 0 0 0\n"
  "MouseMoveEvent 149 99 0 0 0 0 0\n"
  "MouseMoveEvent 146 99 0 0 0 0 0\n"
  "MouseMoveEvent 143 99 0 0 0 0 0\n"
  "MouseMoveEvent 141 100 0 0 0 0 0\n"
  "MouseMoveEvent 139 100 0 0 0 0 0\n"
  "MouseMoveEvent 137 100 0 0 0 0 0\n"
  "MouseMoveEvent 136 101 0 0 0 0 0\n"
  "MouseMoveEvent 135 101 0 0 0 0 0\n"
  "MouseMoveEvent 134 101 0 0 0 0 0\n"
  "MouseMoveEvent 134 102 0 0 0 0 0\n"
  "MouseMoveEvent 134 103 0 0 0 0 0\n"
  "MouseMoveEvent 134 104 0 0 0 0 0\n"
  "MouseMoveEvent 133 105 0 0 0 0 0\n"
  "MouseMoveEvent 132 106 0 0 0 0 0\n"
  "MouseMoveEvent 131 106 0 0 0 0 0\n"
  "MouseMoveEvent 131 107 0 0 0 0 0\n"
  "MouseMoveEvent 130 108 0 0 0 0 0\n"
  "MouseMoveEvent 129 108 0 0 0 0 0\n"
  "MouseMoveEvent 129 109 0 0 0 0 0\n"
  "MouseMoveEvent 128 109 0 0 0 0 0\n"
  "MouseMoveEvent 127 109 0 0 0 0 0\n"
  "MouseMoveEvent 126 109 0 0 0 0 0\n"
  "MouseMoveEvent 125 110 0 0 0 0 0\n"
  "MouseMoveEvent 124 110 0 0 0 0 0\n"
  "MouseMoveEvent 123 110 0 0 0 0 0\n"
  "MouseMoveEvent 122 110 0 0 0 0 0\n"
  "MouseMoveEvent 121 110 0 0 0 0 0\n"
  "MouseMoveEvent 120 110 0 0 0 0 0\n"
  "MouseMoveEvent 119 110 0 0 0 0 0\n"
  "MouseMoveEvent 118 110 0 0 0 0 0\n"
  "LeftButtonPressEvent 118 110 0 0 0 0 0\n"
  "RenderEvent 118 110 0 0 0 0 0\n"
  "MouseMoveEvent 119 110 0 0 0 0 0\n"
  "RenderEvent 119 110 0 0 0 0 0\n"
  "MouseMoveEvent 120 110 0 0 0 0 0\n"
  "RenderEvent 120 110 0 0 0 0 0\n"
  "MouseMoveEvent 121 110 0 0 0 0 0\n"
  "RenderEvent 121 110 0 0 0 0 0\n"
  "MouseMoveEvent 122 110 0 0 0 0 0\n"
  "RenderEvent 122 110 0 0 0 0 0\n"
  "MouseMoveEvent 123 110 0 0 0 0 0\n"
  "RenderEvent 123 110 0 0 0 0 0\n"
  "MouseMoveEvent 124 110 0 0 0 0 0\n"
  "RenderEvent 124 110 0 0 0 0 0\n"
  "MouseMoveEvent 125 110 0 0 0 0 0\n"
  "RenderEvent 125 110 0 0 0 0 0\n"
  "MouseMoveEvent 126 110 0 0 0 0 0\n"
  "RenderEvent 126 110 0 0 0 0 0\n"
  "MouseMoveEvent 127 110 0 0 0 0 0\n"
  "RenderEvent 127 110 0 0 0 0 0\n"
  "MouseMoveEvent 128 110 0 0 0 0 0\n"
  "RenderEvent 128 110 0 0 0 0 0\n"
  "MouseMoveEvent 130 110 0 0 0 0 0\n"
  "RenderEvent 130 110 0 0 0 0 0\n"
  "MouseMoveEvent 131 110 0 0 0 0 0\n"
  "RenderEvent 131 110 0 0 0 0 0\n"
  "MouseMoveEvent 132 110 0 0 0 0 0\n"
  "RenderEvent 132 110 0 0 0 0 0\n"
  "MouseMoveEvent 133 110 0 0 0 0 0\n"
  "RenderEvent 133 110 0 0 0 0 0\n"
  "MouseMoveEvent 134 110 0 0 0 0 0\n"
  "RenderEvent 134 110 0 0 0 0 0\n"
  "MouseMoveEvent 135 111 0 0 0 0 0\n"
  "RenderEvent 135 111 0 0 0 0 0\n"
  "MouseMoveEvent 137 110 0 0 0 0 0\n"
  "RenderEvent 137 110 0 0 0 0 0\n"
  "MouseMoveEvent 138 110 0 0 0 0 0\n"
  "RenderEvent 138 110 0 0 0 0 0\n"
  "MouseMoveEvent 139 110 0 0 0 0 0\n"
  "RenderEvent 139 110 0 0 0 0 0\n"
  "MouseMoveEvent 140 110 0 0 0 0 0\n"
  "RenderEvent 140 110 0 0 0 0 0\n"
  "MouseMoveEvent 141 110 0 0 0 0 0\n"
  "RenderEvent 141 110 0 0 0 0 0\n"
  "MouseMoveEvent 142 110 0 0 0 0 0\n"
  "RenderEvent 142 110 0 0 0 0 0\n"
  "MouseMoveEvent 143 109 0 0 0 0 0\n"
  "RenderEvent 143 109 0 0 0 0 0\n"
  "MouseMoveEvent 144 109 0 0 0 0 0\n"
  "RenderEvent 144 109 0 0 0 0 0\n"
  "MouseMoveEvent 145 109 0 0 0 0 0\n"
  "RenderEvent 145 109 0 0 0 0 0\n"
  "MouseMoveEvent 146 109 0 0 0 0 0\n"
  "RenderEvent 146 109 0 0 0 0 0\n"
  "MouseMoveEvent 146 108 0 0 0 0 0\n"
  "RenderEvent 146 108 0 0 0 0 0\n"
  "MouseMoveEvent 147 108 0 0 0 0 0\n"
  "RenderEvent 147 108 0 0 0 0 0\n"
  "MouseMoveEvent 148 108 0 0 0 0 0\n"
  "RenderEvent 148 108 0 0 0 0 0\n"
  "MouseMoveEvent 149 108 0 0 0 0 0\n"
  "RenderEvent 149 108 0 0 0 0 0\n"
  "MouseMoveEvent 150 108 0 0 0 0 0\n"
  "RenderEvent 150 108 0 0 0 0 0\n"
  "MouseMoveEvent 151 108 0 0 0 0 0\n"
  "RenderEvent 151 108 0 0 0 0 0\n"
  "MouseMoveEvent 152 108 0 0 0 0 0\n"
  "RenderEvent 152 108 0 0 0 0 0\n"
  "MouseMoveEvent 153 108 0 0 0 0 0\n"
  "RenderEvent 153 108 0 0 0 0 0\n"
  "MouseMoveEvent 154 108 0 0 0 0 0\n"
  "RenderEvent 154 108 0 0 0 0 0\n"
  "MouseMoveEvent 155 108 0 0 0 0 0\n"
  "RenderEvent 155 108 0 0 0 0 0\n"
  "MouseMoveEvent 156 108 0 0 0 0 0\n"
  "RenderEvent 156 108 0 0 0 0 0\n"
  "MouseMoveEvent 157 108 0 0 0 0 0\n"
  "RenderEvent 157 108 0 0 0 0 0\n"
  "MouseMoveEvent 158 108 0 0 0 0 0\n"
  "RenderEvent 158 108 0 0 0 0 0\n"
  "MouseMoveEvent 159 108 0 0 0 0 0\n"
  "RenderEvent 159 108 0 0 0 0 0\n"
  "MouseMoveEvent 160 108 0 0 0 0 0\n"
  "RenderEvent 160 108 0 0 0 0 0\n"
  "MouseMoveEvent 161 108 0 0 0 0 0\n"
  "RenderEvent 161 108 0 0 0 0 0\n"
  "MouseMoveEvent 162 108 0 0 0 0 0\n"
  "RenderEvent 162 108 0 0 0 0 0\n"
  "MouseMoveEvent 163 108 0 0 0 0 0\n"
  "RenderEvent 163 108 0 0 0 0 0\n"
  "MouseMoveEvent 164 108 0 0 0 0 0\n"
  "RenderEvent 164 108 0 0 0 0 0\n"
  "MouseMoveEvent 165 108 0 0 0 0 0\n"
  "RenderEvent 165 108 0 0 0 0 0\n"
  "MouseMoveEvent 166 107 0 0 0 0 0\n"
  "RenderEvent 166 107 0 0 0 0 0\n"
  "MouseMoveEvent 167 107 0 0 0 0 0\n"
  "RenderEvent 167 107 0 0 0 0 0\n"
  "MouseMoveEvent 168 107 0 0 0 0 0\n"
  "RenderEvent 168 107 0 0 0 0 0\n"
  "MouseMoveEvent 169 107 0 0 0 0 0\n"
  "RenderEvent 169 107 0 0 0 0 0\n"
  "MouseMoveEvent 170 107 0 0 0 0 0\n"
  "RenderEvent 170 107 0 0 0 0 0\n"
  "MouseMoveEvent 171 107 0 0 0 0 0\n"
  "RenderEvent 171 107 0 0 0 0 0\n"
  "MouseMoveEvent 172 107 0 0 0 0 0\n"
  "RenderEvent 172 107 0 0 0 0 0\n"
  "MouseMoveEvent 173 107 0 0 0 0 0\n"
  "RenderEvent 173 107 0 0 0 0 0\n"
  "MouseMoveEvent 174 107 0 0 0 0 0\n"
  "RenderEvent 174 107 0 0 0 0 0\n"
  "MouseMoveEvent 175 107 0 0 0 0 0\n"
  "RenderEvent 175 107 0 0 0 0 0\n"
  "MouseMoveEvent 176 107 0 0 0 0 0\n"
  "RenderEvent 176 107 0 0 0 0 0\n"
  "MouseMoveEvent 177 107 0 0 0 0 0\n"
  "RenderEvent 177 107 0 0 0 0 0\n"
  "MouseMoveEvent 178 107 0 0 0 0 0\n"
  "RenderEvent 178 107 0 0 0 0 0\n"
  "LeftButtonReleaseEvent 178 107 0 0 0 0 0\n"
  "RenderEvent 178 107 0 0 0 0 0\n"
  "KeyPressEvent 178 107 0 0 119 1 w\n"
  "CharEvent 178 107 0 0 119 1 w\n"
  "RenderEvent 178 107 0 0 119 1 w\n"
  "KeyReleaseEvent 178 107 0 0 119 1 w\n"
  "MouseMoveEvent 178 108 0 0 0 0 w\n"
  "LeftButtonPressEvent 178 108 0 0 0 0 w\n"
  "RenderEvent 178 108 0 0 0 0 w\n"
  "MouseMoveEvent 157 109 0 0 0 0 w\n"
  "RenderEvent 157 109 0 0 0 0 w\n"
  "MouseMoveEvent 156 109 0 0 0 0 w\n"
  "RenderEvent 156 109 0 0 0 0 w\n"
  "MouseMoveEvent 155 109 0 0 0 0 w\n"
  "RenderEvent 155 109 0 0 0 0 w\n"
  "MouseMoveEvent 154 109 0 0 0 0 w\n"
  "RenderEvent 154 109 0 0 0 0 w\n"
  "MouseMoveEvent 153 109 0 0 0 0 w\n"
  "RenderEvent 153 109 0 0 0 0 w\n"
  "MouseMoveEvent 152 109 0 0 0 0 w\n"
  "RenderEvent 152 109 0 0 0 0 w\n"
  "MouseMoveEvent 151 109 0 0 0 0 w\n"
  "RenderEvent 151 109 0 0 0 0 w\n"
  "MouseMoveEvent 149 109 0 0 0 0 w\n"
  "RenderEvent 149 109 0 0 0 0 w\n"
  "MouseMoveEvent 148 109 0 0 0 0 w\n"
  "RenderEvent 148 109 0 0 0 0 w\n"
  "MouseMoveEvent 147 109 0 0 0 0 w\n"
  "RenderEvent 147 109 0 0 0 0 w\n"
  "MouseMoveEvent 146 109 0 0 0 0 w\n"
  "RenderEvent 146 109 0 0 0 0 w\n"
  "MouseMoveEvent 145 109 0 0 0 0 w\n"
  "RenderEvent 145 109 0 0 0 0 w\n"
  "MouseMoveEvent 144 109 0 0 0 0 w\n"
  "RenderEvent 144 109 0 0 0 0 w\n"
  "MouseMoveEvent 143 109 0 0 0 0 w\n"
  "RenderEvent 143 109 0 0 0 0 w\n"
  "MouseMoveEvent 141 109 0 0 0 0 w\n"
  "RenderEvent 141 109 0 0 0 0 w\n"
  "MouseMoveEvent 139 109 0 0 0 0 w\n"
  "RenderEvent 139 109 0 0 0 0 w\n"
  "MouseMoveEvent 136 109 0 0 0 0 w\n"
  "RenderEvent 136 109 0 0 0 0 w\n"
  "MouseMoveEvent 133 109 0 0 0 0 w\n"
  "RenderEvent 133 109 0 0 0 0 w\n"
  "MouseMoveEvent 130 109 0 0 0 0 w\n"
  "RenderEvent 130 109 0 0 0 0 w\n"
  "MouseMoveEvent 128 109 0 0 0 0 w\n"
  "RenderEvent 128 109 0 0 0 0 w\n"
  "MouseMoveEvent 125 109 0 0 0 0 w\n"
  "RenderEvent 125 109 0 0 0 0 w\n"
  "MouseMoveEvent 123 109 0 0 0 0 w\n"
  "RenderEvent 123 109 0 0 0 0 w\n"
  "MouseMoveEvent 120 109 0 0 0 0 w\n"
  "RenderEvent 120 109 0 0 0 0 w\n"
  "MouseMoveEvent 119 109 0 0 0 0 w\n"
  "RenderEvent 119 109 0 0 0 0 w\n"
  "MouseMoveEvent 118 109 0 0 0 0 w\n"
  "RenderEvent 118 109 0 0 0 0 w\n"
  "MouseMoveEvent 117 109 0 0 0 0 w\n"
  "RenderEvent 117 109 0 0 0 0 w\n"
  "MouseMoveEvent 116 109 0 0 0 0 w\n"
  "RenderEvent 116 109 0 0 0 0 w\n"
  "MouseMoveEvent 114 109 0 0 0 0 w\n"
  "RenderEvent 114 109 0 0 0 0 w\n"
  "MouseMoveEvent 113 109 0 0 0 0 w\n"
  "RenderEvent 113 109 0 0 0 0 w\n"
  "MouseMoveEvent 112 109 0 0 0 0 w\n"
  "RenderEvent 112 109 0 0 0 0 w\n"
  "MouseMoveEvent 111 109 0 0 0 0 w\n"
  "RenderEvent 111 109 0 0 0 0 w\n"
  "MouseMoveEvent 110 109 0 0 0 0 w\n"
  "RenderEvent 110 109 0 0 0 0 w\n"
  "MouseMoveEvent 109 109 0 0 0 0 w\n"
  "RenderEvent 109 109 0 0 0 0 w\n"
  "MouseMoveEvent 108 109 0 0 0 0 w\n"
  "RenderEvent 108 109 0 0 0 0 w\n"
  "MouseMoveEvent 107 109 0 0 0 0 w\n"
  "RenderEvent 107 109 0 0 0 0 w\n"
  "MouseMoveEvent 106 109 0 0 0 0 w\n"
  "RenderEvent 106 109 0 0 0 0 w\n"
  "MouseMoveEvent 105 109 0 0 0 0 w\n"
  "RenderEvent 105 109 0 0 0 0 w\n"
  "MouseMoveEvent 104 109 0 0 0 0 w\n"
  "RenderEvent 104 109 0 0 0 0 w\n"
  "MouseMoveEvent 103 109 0 0 0 0 w\n"
  "RenderEvent 103 109 0 0 0 0 w\n"
  "MouseMoveEvent 102 109 0 0 0 0 w\n"
  "RenderEvent 102 109 0 0 0 0 w\n"
  "MouseMoveEvent 101 109 0 0 0 0 w\n"
  "RenderEvent 101 109 0 0 0 0 w\n"
  "LeftButtonReleaseEvent 101 109 0 0 0 0 w\n"
  "RenderEvent 101 109 0 0 0 0 w\n"
  "KeyPressEvent 101 109 0 0 115 1 s\n"
  "CharEvent 101 109 0 0 115 1 s\n"
  "RenderEvent 101 109 0 0 115 1 s\n"
  "KeyReleaseEvent 101 109 0 0 115 1 s\n"
  "LeftButtonPressEvent 101 109 0 0 0 0 s\n"
  "RenderEvent 101 109 0 0 0 0 s\n"
  "MouseMoveEvent 102 109 0 0 0 0 s\n"
  "RenderEvent 102 109 0 0 0 0 s\n"
  "MouseMoveEvent 103 109 0 0 0 0 s\n"
  "RenderEvent 103 109 0 0 0 0 s\n"
  "MouseMoveEvent 105 109 0 0 0 0 s\n"
  "RenderEvent 105 109 0 0 0 0 s\n"
  "MouseMoveEvent 106 109 0 0 0 0 s\n"
  "RenderEvent 106 109 0 0 0 0 s\n"
  "MouseMoveEvent 108 108 0 0 0 0 s\n"
  "RenderEvent 108 108 0 0 0 0 s\n"
  "MouseMoveEvent 111 108 0 0 0 0 s\n"
  "RenderEvent 111 108 0 0 0 0 s\n"
  "MouseMoveEvent 113 108 0 0 0 0 s\n"
  "RenderEvent 113 108 0 0 0 0 s\n"
  "MouseMoveEvent 116 108 0 0 0 0 s\n"
  "RenderEvent 116 108 0 0 0 0 s\n"
  "MouseMoveEvent 118 108 0 0 0 0 s\n"
  "RenderEvent 118 108 0 0 0 0 s\n"
  "MouseMoveEvent 120 108 0 0 0 0 s\n"
  "RenderEvent 120 108 0 0 0 0 s\n"
  "MouseMoveEvent 122 108 0 0 0 0 s\n"
  "RenderEvent 122 108 0 0 0 0 s\n"
  "MouseMoveEvent 124 108 0 0 0 0 s\n"
  "RenderEvent 124 108 0 0 0 0 s\n"
  "MouseMoveEvent 126 108 0 0 0 0 s\n"
  "RenderEvent 126 108 0 0 0 0 s\n"
  "MouseMoveEvent 128 108 0 0 0 0 s\n"
  "RenderEvent 128 108 0 0 0 0 s\n"
  "MouseMoveEvent 129 108 0 0 0 0 s\n"
  "RenderEvent 129 108 0 0 0 0 s\n"
  "MouseMoveEvent 130 108 0 0 0 0 s\n"
  "RenderEvent 130 108 0 0 0 0 s\n"
  "MouseMoveEvent 131 108 0 0 0 0 s\n"
  "RenderEvent 131 108 0 0 0 0 s\n"
  "MouseMoveEvent 133 108 0 0 0 0 s\n"
  "RenderEvent 133 108 0 0 0 0 s\n"
  "MouseMoveEvent 134 108 0 0 0 0 s\n"
  "RenderEvent 134 108 0 0 0 0 s\n"
  "MouseMoveEvent 135 108 0 0 0 0 s\n"
  "RenderEvent 135 108 0 0 0 0 s\n"
  "MouseMoveEvent 136 108 0 0 0 0 s\n"
  "RenderEvent 136 108 0 0 0 0 s\n"
  "MouseMoveEvent 137 108 0 0 0 0 s\n"
  "RenderEvent 137 108 0 0 0 0 s\n"
  "MouseMoveEvent 138 108 0 0 0 0 s\n"
  "RenderEvent 138 108 0 0 0 0 s\n"
  "MouseMoveEvent 139 108 0 0 0 0 s\n"
  "RenderEvent 139 108 0 0 0 0 s\n"
  "MouseMoveEvent 140 108 0 0 0 0 s\n"
  "RenderEvent 140 108 0 0 0 0 s\n"
  "MouseMoveEvent 142 108 0 0 0 0 s\n"
  "RenderEvent 142 108 0 0 0 0 s\n"
  "MouseMoveEvent 144 107 0 0 0 0 s\n"
  "RenderEvent 144 107 0 0 0 0 s\n"
  "MouseMoveEvent 146 107 0 0 0 0 s\n"
  "RenderEvent 146 107 0 0 0 0 s\n"
  "MouseMoveEvent 148 107 0 0 0 0 s\n"
  "RenderEvent 148 107 0 0 0 0 s\n"
  "MouseMoveEvent 150 107 0 0 0 0 s\n"
  "RenderEvent 150 107 0 0 0 0 s\n"
  "MouseMoveEvent 152 107 0 0 0 0 s\n"
  "RenderEvent 152 107 0 0 0 0 s\n"
  "MouseMoveEvent 154 107 0 0 0 0 s\n"
  "RenderEvent 154 107 0 0 0 0 s\n"
  "MouseMoveEvent 155 107 0 0 0 0 s\n"
  "RenderEvent 155 107 0 0 0 0 s\n"
  "MouseMoveEvent 157 107 0 0 0 0 s\n"
  "RenderEvent 157 107 0 0 0 0 s\n"
  "MouseMoveEvent 159 107 0 0 0 0 s\n"
  "RenderEvent 159 107 0 0 0 0 s\n"
  "MouseMoveEvent 160 107 0 0 0 0 s\n"
  "RenderEvent 160 107 0 0 0 0 s\n"
  "MouseMoveEvent 162 107 0 0 0 0 s\n"
  "RenderEvent 162 107 0 0 0 0 s\n"
  "MouseMoveEvent 163 107 0 0 0 0 s\n"
  "RenderEvent 163 107 0 0 0 0 s\n"
  "MouseMoveEvent 164 107 0 0 0 0 s\n"
  "RenderEvent 164 107 0 0 0 0 s\n"
  "MouseMoveEvent 165 107 0 0 0 0 s\n"
  "RenderEvent 165 107 0 0 0 0 s\n"
  "MouseMoveEvent 166 107 0 0 0 0 s\n"
  "RenderEvent 166 107 0 0 0 0 s\n"
  "MouseMoveEvent 167 107 0 0 0 0 s\n"
  "RenderEvent 167 107 0 0 0 0 s\n"
  "MouseMoveEvent 168 107 0 0 0 0 s\n"
  "RenderEvent 168 107 0 0 0 0 s\n"
  "MouseMoveEvent 169 107 0 0 0 0 s\n"
  "RenderEvent 169 107 0 0 0 0 s\n"
  "MouseMoveEvent 171 107 0 0 0 0 s\n"
  "RenderEvent 171 107 0 0 0 0 s\n"
  "MouseMoveEvent 172 106 0 0 0 0 s\n"
  "RenderEvent 172 106 0 0 0 0 s\n"
  "MouseMoveEvent 174 106 0 0 0 0 s\n"
  "RenderEvent 174 106 0 0 0 0 s\n"
  "MouseMoveEvent 175 106 0 0 0 0 s\n"
  "RenderEvent 175 106 0 0 0 0 s\n"
  "MouseMoveEvent 177 106 0 0 0 0 s\n"
  "RenderEvent 177 106 0 0 0 0 s\n"
  "MouseMoveEvent 178 106 0 0 0 0 s\n"
  "RenderEvent 178 106 0 0 0 0 s\n"
  "MouseMoveEvent 180 106 0 0 0 0 s\n"
  "RenderEvent 180 106 0 0 0 0 s\n"
  "MouseMoveEvent 181 106 0 0 0 0 s\n"
  "RenderEvent 181 106 0 0 0 0 s\n"
  "MouseMoveEvent 182 106 0 0 0 0 s\n"
  "RenderEvent 182 106 0 0 0 0 s\n"
  "MouseMoveEvent 183 106 0 0 0 0 s\n"
  "RenderEvent 183 106 0 0 0 0 s\n"
  "MouseMoveEvent 184 106 0 0 0 0 s\n"
  "RenderEvent 184 106 0 0 0 0 s\n"
  "LeftButtonReleaseEvent 184 106 0 0 0 0 s\n"
  "RenderEvent 184 106 0 0 0 0 s\n"
  "MouseMoveEvent 185 106 0 0 0 0 s\n"
  "MouseMoveEvent 185 105 0 0 0 0 s\n"
  "MouseMoveEvent 186 104 0 0 0 0 s\n"
  "MouseMoveEvent 187 104 0 0 0 0 s\n"
  "MouseMoveEvent 187 103 0 0 0 0 s\n"
  "MouseMoveEvent 188 103 0 0 0 0 s\n"
  "MouseMoveEvent 188 102 0 0 0 0 s\n"
  "MouseMoveEvent 188 101 0 0 0 0 s\n"
  "MouseMoveEvent 190 101 0 0 0 0 s\n"
  "MouseMoveEvent 190 100 0 0 0 0 s\n"
  "MouseMoveEvent 190 99 0 0 0 0 s\n"
  "MouseMoveEvent 191 99 0 0 0 0 s\n"
  "MouseMoveEvent 192 98 0 0 0 0 s\n"
  "MouseMoveEvent 193 97 0 0 0 0 s\n"
  "MouseMoveEvent 194 97 0 0 0 0 s\n"
  "MouseMoveEvent 194 96 0 0 0 0 s\n"
  "MouseMoveEvent 195 96 0 0 0 0 s\n"
  "MouseMoveEvent 195 95 0 0 0 0 s\n"
  "MouseMoveEvent 195 94 0 0 0 0 s\n"
  "MouseMoveEvent 196 94 0 0 0 0 s\n"
  "MouseMoveEvent 197 94 0 0 0 0 s\n"
  "MouseMoveEvent 197 93 0 0 0 0 s\n"
  "MouseMoveEvent 198 93 0 0 0 0 s\n"
  "MouseMoveEvent 198 92 0 0 0 0 s\n"
  "MouseMoveEvent 199 91 0 0 0 0 s\n"
  "MouseMoveEvent 200 90 0 0 0 0 s\n"
  "MouseMoveEvent 201 89 0 0 0 0 s\n"
  "MouseMoveEvent 203 89 0 0 0 0 s\n"
  "MouseMoveEvent 204 87 0 0 0 0 s\n"
  "MouseMoveEvent 206 86 0 0 0 0 s\n"
  "MouseMoveEvent 212 82 0 0 0 0 s\n"
  "MouseMoveEvent 218 80 0 0 0 0 s\n"
  "MouseMoveEvent 226 78 0 0 0 0 s\n"
  "MouseMoveEvent 234 76 0 0 0 0 s\n"
  "MouseMoveEvent 242 72 0 0 0 0 s\n"
  "MouseMoveEvent 250 68 0 0 0 0 s\n"
  "MouseMoveEvent 260 66 0 0 0 0 s\n"
  "MouseMoveEvent 266 62 0 0 0 0 s\n"
  "MouseMoveEvent 272 58 0 0 0 0 s\n"
  "MouseMoveEvent 278 56 0 0 0 0 s\n"
  "MouseMoveEvent 279 56 0 0 0 0 s\n"
  "MouseMoveEvent 279 55 0 0 0 0 s\n"
  "MouseMoveEvent 278 55 0 0 0 0 s\n"
  "MouseMoveEvent 276 55 0 0 0 0 s\n"
  "MouseMoveEvent 275 55 0 0 0 0 s\n"
  "MouseMoveEvent 274 55 0 0 0 0 s\n"
  "MouseMoveEvent 273 55 0 0 0 0 s\n"
  "MouseMoveEvent 271 55 0 0 0 0 s\n"
  "MouseMoveEvent 270 55 0 0 0 0 s\n"
  "KeyPressEvent 270 55 0 0 113 1 q\n"
  "CharEvent 270 55 0 0 113 1 q\n"
  "ExitEvent 270 55 0 0 113 1 q\n"
  ;


// This does the actual work: updates the probe.
// Callback for the interaction
class vtkSliderCallback : public vtkCommand
{
public:
  static vtkSliderCallback *New() 
  { return new vtkSliderCallback; }
  virtual void Execute(vtkObject *caller, unsigned long, void*)
  {
    vtkSliderWidget *sliderWidget = 
      reinterpret_cast<vtkSliderWidget*>(caller);
    this->Glyph->SetScaleFactor(static_cast<vtkSliderRepresentation *>(sliderWidget->GetRepresentation())->GetValue());
  }
  vtkSliderCallback():Glyph(0) {}
  vtkGlyph3D *Glyph;
};

int TestSliderWidget(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  // Create a mace out of filters.
  //
  vtkSmartPointer<vtkSphereSource> sphereSource =
    vtkSmartPointer<vtkSphereSource>::New();
  vtkSmartPointer<vtkConeSource> cone =
    vtkSmartPointer<vtkConeSource>::New();
  vtkSmartPointer<vtkGlyph3D> glyph =
    vtkSmartPointer<vtkGlyph3D>::New();
  glyph->SetInput(sphereSource->GetOutput());
  glyph->SetSource(cone->GetOutput());
  glyph->SetVectorModeToUseNormal();
  glyph->SetScaleModeToScaleByVector();
  glyph->SetScaleFactor(0.25);

  // The sphere and spikes are appended into a single polydata. 
  // This just makes things simpler to manage.
  vtkSmartPointer<vtkAppendPolyData> apd =
    vtkSmartPointer<vtkAppendPolyData>::New();
  apd->AddInput(glyph->GetOutput());
  apd->AddInput(sphereSource->GetOutput());

  vtkSmartPointer<vtkPolyDataMapper> maceMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  maceMapper->SetInput(apd->GetOutput());

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
  vtkSmartPointer<vtkSliderRepresentation3D> sliderRep =
    vtkSmartPointer<vtkSliderRepresentation3D>::New();
  sliderRep->SetValue(0.25);
  sliderRep->SetTitleText("Spike Size");
  sliderRep->GetPoint1Coordinate()->SetCoordinateSystemToWorld();
  sliderRep->GetPoint1Coordinate()->SetValue(0,0,0);
  sliderRep->GetPoint2Coordinate()->SetCoordinateSystemToWorld();
  sliderRep->GetPoint2Coordinate()->SetValue(2,0,0);
  sliderRep->SetSliderLength(0.075);
  sliderRep->SetSliderWidth(0.05);
  sliderRep->SetEndCapLength(0.05);

  vtkSmartPointer<vtkSliderWidget> sliderWidget =
    vtkSmartPointer<vtkSliderWidget>::New();
  sliderWidget->GetEventTranslator()->SetTranslation(vtkCommand::RightButtonPressEvent,
                                                     vtkWidgetEvent::Select);
  sliderWidget->GetEventTranslator()->SetTranslation(vtkCommand::RightButtonReleaseEvent,
                                                     vtkWidgetEvent::EndSelect);
  sliderWidget->SetInteractor(iren);
  sliderWidget->SetRepresentation(sliderRep);
  sliderWidget->SetAnimationModeToAnimate();
  sliderWidget->EnabledOn();


  vtkSmartPointer<vtkSliderCallback> callback =
    vtkSmartPointer<vtkSliderCallback>::New();
  callback->Glyph = glyph;
  sliderWidget->AddObserver(vtkCommand::InteractionEvent,callback);

  ren1->AddActor(maceActor);

  // Add the actors to the renderer, set the background and size
  //
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // record events
  vtkSmartPointer<vtkInteractorEventRecorder> recorder =
    vtkSmartPointer<vtkInteractorEventRecorder>::New();
  recorder->SetInteractor(iren);
  recorder->SetInputString(TestSliderWidgetEventLog);
  recorder->ReadFromInputStringOn();

  // render the image
  //
  iren->Initialize();
  renWin->Render();
  recorder->On();
  recorder->Play();

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();

  iren->Start();

  return EXIT_SUCCESS;

}
