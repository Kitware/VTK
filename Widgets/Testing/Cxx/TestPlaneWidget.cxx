/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPlaneWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSmartPointer.h"

#include "vtkActor.h"
#include "vtkCommand.h"
#include "vtkConeSource.h"
#include "vtkGlyph3D.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkPLOT3DReader.h"
#include "vtkPlaneWidget.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProbeFilter.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredGridOutlineFilter.h"

#include "vtkTestUtilities.h"

char TPWeventLog[] =
  "# StreamVersion 1\n"
  "CharEvent 192 232 0 0 105 1 i\n"
  "KeyReleaseEvent 192 230 0 0 105 1 i\n"
  "MouseMoveEvent 192 229 0 0 0 0 i\n"
  "MouseMoveEvent 192 228 0 0 0 0 i\n"
  "MouseMoveEvent 193 228 0 0 0 0 i\n"
  "MouseMoveEvent 194 227 0 0 0 0 i\n"
  "MouseMoveEvent 195 225 0 0 0 0 i\n"
  "MouseMoveEvent 196 220 0 0 0 0 i\n"
  "MouseMoveEvent 196 215 0 0 0 0 i\n"
  "MouseMoveEvent 196 206 0 0 0 0 i\n"
  "MouseMoveEvent 198 197 0 0 0 0 i\n"
  "MouseMoveEvent 198 190 0 0 0 0 i\n"
  "MouseMoveEvent 198 185 0 0 0 0 i\n"
  "MouseMoveEvent 199 178 0 0 0 0 i\n"
  "MouseMoveEvent 199 173 0 0 0 0 i\n"
  "MouseMoveEvent 198 168 0 0 0 0 i\n"
  "MouseMoveEvent 196 163 0 0 0 0 i\n"
  "MouseMoveEvent 196 159 0 0 0 0 i\n"
  "MouseMoveEvent 196 156 0 0 0 0 i\n"
  "MouseMoveEvent 195 151 0 0 0 0 i\n"
  "MouseMoveEvent 192 145 0 0 0 0 i\n"
  "MouseMoveEvent 187 142 0 0 0 0 i\n"
  "MouseMoveEvent 185 141 0 0 0 0 i\n"
  "MouseMoveEvent 181 139 0 0 0 0 i\n"
  "MouseMoveEvent 179 139 0 0 0 0 i\n"
  "MouseMoveEvent 177 138 0 0 0 0 i\n"
  "MouseMoveEvent 173 138 0 0 0 0 i\n"
  "MouseMoveEvent 171 138 0 0 0 0 i\n"
  "MouseMoveEvent 170 137 0 0 0 0 i\n"
  "MouseMoveEvent 170 138 0 0 0 0 i\n"
  "MouseMoveEvent 170 139 0 0 0 0 i\n"
  "MouseMoveEvent 170 142 0 0 0 0 i\n"
  "MouseMoveEvent 170 144 0 0 0 0 i\n"
  "MouseMoveEvent 163 147 0 0 0 0 i\n"
  "MouseMoveEvent 159 149 0 0 0 0 i\n"
  "MouseMoveEvent 155 149 0 0 0 0 i\n"
  "MouseMoveEvent 151 153 0 0 0 0 i\n"
  "MouseMoveEvent 150 154 0 0 0 0 i\n"
  "MouseMoveEvent 147 155 0 0 0 0 i\n"
  "MouseMoveEvent 146 158 0 0 0 0 i\n"
  "MouseMoveEvent 146 160 0 0 0 0 i\n"
  "MouseMoveEvent 146 161 0 0 0 0 i\n"
  "MouseMoveEvent 146 163 0 0 0 0 i\n"
  "MouseMoveEvent 146 164 0 0 0 0 i\n"
  "MouseMoveEvent 146 167 0 0 0 0 i\n"
  "MouseMoveEvent 146 171 0 0 0 0 i\n"
  "MouseMoveEvent 146 172 0 0 0 0 i\n"
  "MouseMoveEvent 146 173 0 0 0 0 i\n"
  "MouseMoveEvent 147 173 0 0 0 0 i\n"
  "MouseMoveEvent 147 172 0 0 0 0 i\n"
  "MouseMoveEvent 148 169 0 0 0 0 i\n"
  "MouseMoveEvent 149 167 0 0 0 0 i\n"
  "MouseMoveEvent 151 163 0 0 0 0 i\n"
  "MouseMoveEvent 153 158 0 0 0 0 i\n"
  "MouseMoveEvent 156 154 0 0 0 0 i\n"
  "MouseMoveEvent 161 150 0 0 0 0 i\n"
  "MouseMoveEvent 162 148 0 0 0 0 i\n"
  "MouseMoveEvent 163 147 0 0 0 0 i\n"
  "MouseMoveEvent 164 146 0 0 0 0 i\n"
  "MouseMoveEvent 165 145 0 0 0 0 i\n"
  "MouseMoveEvent 166 145 0 0 0 0 i\n"
  "MouseMoveEvent 167 146 0 0 0 0 i\n"
  "MouseMoveEvent 168 146 0 0 0 0 i\n"
  "MouseMoveEvent 169 146 0 0 0 0 i\n"
  "MouseMoveEvent 169 147 0 0 0 0 i\n"
  "MouseMoveEvent 170 147 0 0 0 0 i\n"
  "MouseMoveEvent 170 148 0 0 0 0 i\n"
  "MouseMoveEvent 170 149 0 0 0 0 i\n"
  "MouseMoveEvent 171 149 0 0 0 0 i\n"
  "MouseMoveEvent 171 150 0 0 0 0 i\n"
  "MouseMoveEvent 172 150 0 0 0 0 i\n"
  "MouseMoveEvent 172 151 0 0 0 0 i\n"
  "MouseMoveEvent 173 151 0 0 0 0 i\n"
  "MouseMoveEvent 174 151 0 0 0 0 i\n"
  "MouseMoveEvent 175 151 0 0 0 0 i\n"
  "MouseMoveEvent 177 151 0 0 0 0 i\n"
  "MouseMoveEvent 178 151 0 0 0 0 i\n"
  "MouseMoveEvent 179 151 0 0 0 0 i\n"
  "LeftButtonPressEvent 179 151 0 0 0 0 i\n"
  "MouseMoveEvent 179 152 0 0 0 0 i\n"
  "MouseMoveEvent 178 153 0 0 0 0 i\n"
  "MouseMoveEvent 177 154 0 0 0 0 i\n"
  "MouseMoveEvent 176 155 0 0 0 0 i\n"
  "MouseMoveEvent 175 156 0 0 0 0 i\n"
  "MouseMoveEvent 172 156 0 0 0 0 i\n"
  "MouseMoveEvent 167 157 0 0 0 0 i\n"
  "MouseMoveEvent 163 157 0 0 0 0 i\n"
  "MouseMoveEvent 160 158 0 0 0 0 i\n"
  "MouseMoveEvent 159 158 0 0 0 0 i\n"
  "MouseMoveEvent 158 159 0 0 0 0 i\n"
  "MouseMoveEvent 155 162 0 0 0 0 i\n"
  "MouseMoveEvent 153 165 0 0 0 0 i\n"
  "MouseMoveEvent 153 167 0 0 0 0 i\n"
  "MouseMoveEvent 151 168 0 0 0 0 i\n"
  "MouseMoveEvent 148 170 0 0 0 0 i\n"
  "MouseMoveEvent 146 170 0 0 0 0 i\n"
  "MouseMoveEvent 142 172 0 0 0 0 i\n"
  "MouseMoveEvent 140 172 0 0 0 0 i\n"
  "MouseMoveEvent 139 173 0 0 0 0 i\n"
  "MouseMoveEvent 138 174 0 0 0 0 i\n"
  "MouseMoveEvent 137 176 0 0 0 0 i\n"
  "MouseMoveEvent 133 177 0 0 0 0 i\n"
  "MouseMoveEvent 129 178 0 0 0 0 i\n"
  "MouseMoveEvent 128 179 0 0 0 0 i\n"
  "MouseMoveEvent 127 179 0 0 0 0 i\n"
  "MouseMoveEvent 122 179 0 0 0 0 i\n"
  "MouseMoveEvent 115 179 0 0 0 0 i\n"
  "MouseMoveEvent 114 179 0 0 0 0 i\n"
  "MouseMoveEvent 113 179 0 0 0 0 i\n"
  "LeftButtonReleaseEvent 113 179 0 0 0 0 i\n"
  "MouseMoveEvent 113 179 0 0 0 0 i\n"
  "MouseMoveEvent 113 178 0 0 0 0 i\n"
  "MouseMoveEvent 114 178 0 0 0 0 i\n"
  "MouseMoveEvent 115 178 0 0 0 0 i\n"
  "MouseMoveEvent 116 177 0 0 0 0 i\n"
  "MouseMoveEvent 117 177 0 0 0 0 i\n"
  "MouseMoveEvent 118 177 0 0 0 0 i\n"
  "MouseMoveEvent 120 176 0 0 0 0 i\n"
  "MouseMoveEvent 121 176 0 0 0 0 i\n"
  "MouseMoveEvent 124 174 0 0 0 0 i\n"
  "MouseMoveEvent 128 174 0 0 0 0 i\n"
  "MouseMoveEvent 133 174 0 0 0 0 i\n"
  "MouseMoveEvent 138 173 0 0 0 0 i\n"
  "MouseMoveEvent 141 173 0 0 0 0 i\n"
  "MouseMoveEvent 144 171 0 0 0 0 i\n"
  "MouseMoveEvent 147 169 0 0 0 0 i\n"
  "MouseMoveEvent 153 168 0 0 0 0 i\n"
  "MouseMoveEvent 156 168 0 0 0 0 i\n"
  "MouseMoveEvent 159 168 0 0 0 0 i\n"
  "MouseMoveEvent 160 168 0 0 0 0 i\n"
  "MouseMoveEvent 160 169 0 0 0 0 i\n"
  "MouseMoveEvent 159 169 0 0 0 0 i\n"
  "MouseMoveEvent 157 171 0 0 0 0 i\n"
  "MouseMoveEvent 153 173 0 0 0 0 i\n"
  "MouseMoveEvent 152 174 0 0 0 0 i\n"
  "MouseMoveEvent 151 174 0 0 0 0 i\n"
  "MouseMoveEvent 150 175 0 0 0 0 i\n"
  "MouseMoveEvent 149 175 0 0 0 0 i\n"
  "MouseMoveEvent 149 176 0 0 0 0 i\n"
  "MouseMoveEvent 148 176 0 0 0 0 i\n"
  "MouseMoveEvent 148 177 0 0 0 0 i\n"
  "MouseMoveEvent 147 177 0 0 0 0 i\n"
  "MouseMoveEvent 147 178 0 0 0 0 i\n"
  "MouseMoveEvent 146 179 0 0 0 0 i\n"
  "MouseMoveEvent 144 180 0 0 0 0 i\n"
  "MouseMoveEvent 141 180 0 0 0 0 i\n"
  "MouseMoveEvent 139 182 0 0 0 0 i\n"
  "MouseMoveEvent 138 183 0 0 0 0 i\n"
  "MouseMoveEvent 137 183 0 0 0 0 i\n"
  "MouseMoveEvent 137 184 0 0 0 0 i\n"
  "MouseMoveEvent 136 184 0 0 0 0 i\n"
  "MouseMoveEvent 136 185 0 0 0 0 i\n"
  "MouseMoveEvent 136 186 0 0 0 0 i\n"
  "MouseMoveEvent 135 186 0 0 0 0 i\n"
  "MouseMoveEvent 135 187 0 0 0 0 i\n"
  "MouseMoveEvent 135 188 0 0 0 0 i\n"
  "MouseMoveEvent 134 189 0 0 0 0 i\n"
  "MouseMoveEvent 133 189 0 0 0 0 i\n"
  "MouseMoveEvent 132 189 0 0 0 0 i\n"
  "LeftButtonPressEvent 132 189 0 0 0 0 i\n"
  "MouseMoveEvent 132 188 0 0 0 0 i\n"
  "MouseMoveEvent 132 186 0 0 0 0 i\n"
  "MouseMoveEvent 132 184 0 0 0 0 i\n"
  "MouseMoveEvent 132 182 0 0 0 0 i\n"
  "MouseMoveEvent 132 181 0 0 0 0 i\n"
  "MouseMoveEvent 132 179 0 0 0 0 i\n"
  "MouseMoveEvent 132 176 0 0 0 0 i\n"
  "MouseMoveEvent 133 175 0 0 0 0 i\n"
  "MouseMoveEvent 134 174 0 0 0 0 i\n"
  "MouseMoveEvent 134 172 0 0 0 0 i\n"
  "MouseMoveEvent 134 171 0 0 0 0 i\n"
  "MouseMoveEvent 135 168 0 0 0 0 i\n"
  "MouseMoveEvent 135 167 0 0 0 0 i\n"
  "LeftButtonReleaseEvent 135 167 0 0 0 0 i\n"
  "MouseMoveEvent 135 167 0 0 0 0 i\n"
  "MouseMoveEvent 136 166 0 0 0 0 i\n"
  "MouseMoveEvent 136 165 0 0 0 0 i\n"
  "MouseMoveEvent 136 164 0 0 0 0 i\n"
  "MouseMoveEvent 136 163 0 0 0 0 i\n"
  "MouseMoveEvent 136 162 0 0 0 0 i\n"
  "MouseMoveEvent 135 162 0 0 0 0 i\n"
  "MouseMoveEvent 134 162 0 0 0 0 i\n"
  "MouseMoveEvent 133 161 0 0 0 0 i\n"
  "RightButtonPressEvent 133 161 0 0 0 0 i\n"
  "TimerEvent 133 161 0 0 0 0 i\n"
  "TimerEvent 133 161 0 0 0 0 i\n"
  "TimerEvent 133 161 0 0 0 0 i\n"
  "TimerEvent 133 161 0 0 0 0 i\n"
  "TimerEvent 133 161 0 0 0 0 i\n"
  "TimerEvent 133 161 0 0 0 0 i\n"
  "TimerEvent 133 161 0 0 0 0 i\n"
  "TimerEvent 133 161 0 0 0 0 i\n"
  "TimerEvent 133 161 0 0 0 0 i\n"
  "TimerEvent 133 161 0 0 0 0 i\n"
  "TimerEvent 133 161 0 0 0 0 i\n"
  "TimerEvent 133 161 0 0 0 0 i\n"
  "TimerEvent 133 161 0 0 0 0 i\n"
  "TimerEvent 133 161 0 0 0 0 i\n"
  "TimerEvent 133 161 0 0 0 0 i\n"
  "TimerEvent 133 161 0 0 0 0 i\n"
  "TimerEvent 133 161 0 0 0 0 i\n"
  "TimerEvent 133 161 0 0 0 0 i\n"
  "TimerEvent 133 161 0 0 0 0 i\n"
  "TimerEvent 133 161 0 0 0 0 i\n"
  "TimerEvent 133 161 0 0 0 0 i\n"
  "TimerEvent 133 161 0 0 0 0 i\n"
  "TimerEvent 133 161 0 0 0 0 i\n"
  "TimerEvent 133 161 0 0 0 0 i\n"
  "TimerEvent 133 161 0 0 0 0 i\n"
  "TimerEvent 133 161 0 0 0 0 i\n"
  "MouseMoveEvent 133 160 0 0 0 0 i\n"
  "TimerEvent 133 160 0 0 0 0 i\n"
  "TimerEvent 133 160 0 0 0 0 i\n"
  "TimerEvent 133 160 0 0 0 0 i\n"
  "TimerEvent 133 160 0 0 0 0 i\n"
  "TimerEvent 133 160 0 0 0 0 i\n"
  "TimerEvent 133 160 0 0 0 0 i\n"
  "TimerEvent 133 160 0 0 0 0 i\n"
  "TimerEvent 133 160 0 0 0 0 i\n"
  "TimerEvent 133 160 0 0 0 0 i\n"
  "TimerEvent 133 160 0 0 0 0 i\n"
  "TimerEvent 133 160 0 0 0 0 i\n"
  "TimerEvent 133 160 0 0 0 0 i\n"
  "TimerEvent 133 160 0 0 0 0 i\n"
  "TimerEvent 133 160 0 0 0 0 i\n"
  "TimerEvent 133 160 0 0 0 0 i\n"
  "TimerEvent 133 160 0 0 0 0 i\n"
  "TimerEvent 133 160 0 0 0 0 i\n"
  "TimerEvent 133 160 0 0 0 0 i\n"
  "TimerEvent 133 160 0 0 0 0 i\n"
  "TimerEvent 133 160 0 0 0 0 i\n"
  "TimerEvent 133 160 0 0 0 0 i\n"
  "TimerEvent 133 160 0 0 0 0 i\n"
  "TimerEvent 133 160 0 0 0 0 i\n"
  "TimerEvent 133 160 0 0 0 0 i\n"
  "TimerEvent 133 160 0 0 0 0 i\n"
  "TimerEvent 133 160 0 0 0 0 i\n"
  "TimerEvent 133 160 0 0 0 0 i\n"
  "TimerEvent 133 160 0 0 0 0 i\n"
  "TimerEvent 133 160 0 0 0 0 i\n"
  "TimerEvent 133 160 0 0 0 0 i\n"
  "TimerEvent 133 160 0 0 0 0 i\n"
  "TimerEvent 133 160 0 0 0 0 i\n"
  "TimerEvent 133 160 0 0 0 0 i\n"
  "TimerEvent 133 160 0 0 0 0 i\n"
  "TimerEvent 133 160 0 0 0 0 i\n"
  "TimerEvent 133 160 0 0 0 0 i\n"
  "TimerEvent 133 160 0 0 0 0 i\n"
  "TimerEvent 133 160 0 0 0 0 i\n"
  "TimerEvent 133 160 0 0 0 0 i\n"
  "TimerEvent 133 160 0 0 0 0 i\n"
  "TimerEvent 133 160 0 0 0 0 i\n"
  "TimerEvent 133 160 0 0 0 0 i\n"
  "TimerEvent 133 160 0 0 0 0 i\n"
  "MouseMoveEvent 132 160 0 0 0 0 i\n"
  "TimerEvent 132 160 0 0 0 0 i\n"
  "MouseMoveEvent 132 159 0 0 0 0 i\n"
  "TimerEvent 132 159 0 0 0 0 i\n"
  "MouseMoveEvent 131 159 0 0 0 0 i\n"
  "RightButtonReleaseEvent 131 159 0 0 0 0 i\n"
  "MouseMoveEvent 131 159 0 0 0 0 i\n"
  "MouseMoveEvent 131 158 0 0 0 0 i\n"
  "MouseMoveEvent 131 155 0 0 0 0 i\n"
  "MouseMoveEvent 132 154 0 0 0 0 i\n"
  "MouseMoveEvent 132 152 0 0 0 0 i\n"
  "MouseMoveEvent 132 151 0 0 0 0 i\n"
  "MouseMoveEvent 133 150 0 0 0 0 i\n"
  "MouseMoveEvent 134 150 0 0 0 0 i\n"
  "RightButtonPressEvent 134 150 0 0 0 0 i\n"
  "MouseMoveEvent 134 149 0 0 0 0 i\n"
  "MouseMoveEvent 134 148 0 0 0 0 i\n"
  "MouseMoveEvent 134 149 0 0 0 0 i\n"
  "MouseMoveEvent 133 150 0 0 0 0 i\n"
  "MouseMoveEvent 131 151 0 0 0 0 i\n"
  "MouseMoveEvent 131 152 0 0 0 0 i\n"
  "MouseMoveEvent 131 153 0 0 0 0 i\n"
  "MouseMoveEvent 131 154 0 0 0 0 i\n"
  "MouseMoveEvent 130 157 0 0 0 0 i\n"
  "MouseMoveEvent 129 160 0 0 0 0 i\n"
  "MouseMoveEvent 129 161 0 0 0 0 i\n"
  "MouseMoveEvent 128 164 0 0 0 0 i\n"
  "MouseMoveEvent 125 167 0 0 0 0 i\n"
  "MouseMoveEvent 122 171 0 0 0 0 i\n"
  "MouseMoveEvent 122 173 0 0 0 0 i\n"
  "MouseMoveEvent 121 173 0 0 0 0 i\n"
  "MouseMoveEvent 121 175 0 0 0 0 i\n"
  "MouseMoveEvent 121 178 0 0 0 0 i\n"
  "MouseMoveEvent 121 179 0 0 0 0 i\n"
  "MouseMoveEvent 120 179 0 0 0 0 i\n"
  "MouseMoveEvent 120 181 0 0 0 0 i\n"
  "MouseMoveEvent 119 183 0 0 0 0 i\n"
  "MouseMoveEvent 118 185 0 0 0 0 i\n"
  "MouseMoveEvent 118 186 0 0 0 0 i\n"
  "MouseMoveEvent 117 187 0 0 0 0 i\n"
  "MouseMoveEvent 116 189 0 0 0 0 i\n"
  "MouseMoveEvent 113 190 0 0 0 0 i\n"
  "MouseMoveEvent 113 191 0 0 0 0 i\n"
  "MouseMoveEvent 113 190 0 0 0 0 i\n"
  "MouseMoveEvent 113 189 0 0 0 0 i\n"
  "RightButtonReleaseEvent 113 189 0 0 0 0 i\n"
  "MouseMoveEvent 112 189 0 0 0 0 i\n"
  "MouseMoveEvent 112 188 0 0 0 0 i\n"
  "MouseMoveEvent 112 187 0 0 0 0 i\n"
  "MouseMoveEvent 113 186 0 0 0 0 i\n"
  "MouseMoveEvent 114 185 0 0 0 0 i\n"
  "MouseMoveEvent 115 184 0 0 0 0 i\n"
  "MouseMoveEvent 116 183 0 0 0 0 i\n"
  "MouseMoveEvent 116 182 0 0 0 0 i\n"
  "MouseMoveEvent 117 181 0 0 0 0 i\n"
  "MouseMoveEvent 117 180 0 0 0 0 i\n"
  "MouseMoveEvent 118 179 0 0 0 0 i\n"
  "MouseMoveEvent 119 179 0 0 0 0 i\n"
  "MouseMoveEvent 120 178 0 0 0 0 i\n"
  "MouseMoveEvent 120 176 0 0 0 0 i\n"
  "MouseMoveEvent 120 174 0 0 0 0 i\n"
  "MouseMoveEvent 120 173 0 0 0 0 i\n"
  "MouseMoveEvent 120 172 0 0 0 0 i\n"
  "MiddleButtonPressEvent 120 172 0 0 0 0 i\n"
  "MouseMoveEvent 120 171 0 0 0 0 i\n"
  "MouseMoveEvent 121 166 0 0 0 0 i\n"
  "MouseMoveEvent 123 165 0 0 0 0 i\n"
  "MouseMoveEvent 125 165 0 0 0 0 i\n"
  "MouseMoveEvent 126 166 0 0 0 0 i\n"
  "MouseMoveEvent 129 167 0 0 0 0 i\n"
  "MouseMoveEvent 130 167 0 0 0 0 i\n"
  "MouseMoveEvent 133 169 0 0 0 0 i\n"
  "MouseMoveEvent 135 169 0 0 0 0 i\n"
  "MouseMoveEvent 136 169 0 0 0 0 i\n"
  "MouseMoveEvent 137 168 0 0 0 0 i\n"
  "MouseMoveEvent 139 169 0 0 0 0 i\n"
  "MouseMoveEvent 148 168 0 0 0 0 i\n"
  "MouseMoveEvent 158 165 0 0 0 0 i\n"
  "MouseMoveEvent 159 165 0 0 0 0 i\n"
  "MouseMoveEvent 163 165 0 0 0 0 i\n"
  "MouseMoveEvent 164 164 0 0 0 0 i\n"
  "MouseMoveEvent 165 164 0 0 0 0 i\n"
  "MouseMoveEvent 167 163 0 0 0 0 i\n"
  "MouseMoveEvent 168 163 0 0 0 0 i\n"
  "MouseMoveEvent 174 162 0 0 0 0 i\n"
  "MouseMoveEvent 178 161 0 0 0 0 i\n"
  "MouseMoveEvent 179 161 0 0 0 0 i\n"
  "MouseMoveEvent 180 161 0 0 0 0 i\n"
  "MouseMoveEvent 181 161 0 0 0 0 i\n"
  "MouseMoveEvent 183 161 0 0 0 0 i\n"
  "MouseMoveEvent 183 160 0 0 0 0 i\n"
  "MouseMoveEvent 182 159 0 0 0 0 i\n"
  "MiddleButtonReleaseEvent 182 159 0 0 0 0 i\n"
  "MouseMoveEvent 181 158 0 0 0 0 i\n"
  "MouseMoveEvent 180 158 0 0 0 0 i\n"
  "MouseMoveEvent 179 158 0 0 0 0 i\n"
  ;

// This does the actual work: updates the probe.
// Callback for the interaction
class vtkTPWCallback : public vtkCommand
{
public:
  static vtkTPWCallback *New() 
  { return new vtkTPWCallback; }
  virtual void Execute(vtkObject *caller, unsigned long, void*)
  {
    vtkPlaneWidget *planeWidget = reinterpret_cast<vtkPlaneWidget*>(caller);
    planeWidget->GetPolyData(this->PolyData);
    this->Actor->VisibilityOn();
  }
  vtkTPWCallback():PolyData(0),Actor(0) {}
  vtkPolyData *PolyData;
  vtkActor *Actor;
};

int TestPlaneWidget( int argc, char *argv[] )
{
  char* fname = 
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/combxyz.bin");
  char* fname2 = 
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/combq.bin");

  // Start by loading some data.
  //
  vtkSmartPointer<vtkPLOT3DReader> pl3d =
    vtkSmartPointer<vtkPLOT3DReader>::New();
  pl3d->SetXYZFileName(fname);
  pl3d->SetQFileName(fname2);
  pl3d->SetScalarFunctionNumber(100);
  pl3d->SetVectorFunctionNumber(202);
  pl3d->Update();

  delete [] fname;
  delete [] fname2;

  vtkSmartPointer<vtkPolyData> plane =
    vtkSmartPointer<vtkPolyData>::New();

  vtkSmartPointer<vtkProbeFilter> probe =
    vtkSmartPointer<vtkProbeFilter>::New();
  probe->SetInput(plane);
  probe->SetSource(pl3d->GetOutput());

  vtkSmartPointer<vtkPolyDataMapper> probeMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  probeMapper->SetInput(probe->GetPolyDataOutput());
  double tmp[2];
  pl3d->GetOutput()->GetScalarRange(tmp);
  probeMapper->SetScalarRange(tmp[0], tmp[1]);
  
  vtkSmartPointer<vtkActor> probeActor =
    vtkSmartPointer<vtkActor>::New();
  probeActor->SetMapper(probeMapper);
  probeActor->VisibilityOff();

  // An outline is shown for context.
  vtkSmartPointer<vtkStructuredGridOutlineFilter> outline =
    vtkSmartPointer<vtkStructuredGridOutlineFilter>::New();
  outline->SetInputConnection(pl3d->GetOutputPort());

  vtkSmartPointer<vtkPolyDataMapper> outlineMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  outlineMapper->SetInputConnection(outline->GetOutputPort());

  vtkSmartPointer<vtkActor> outlineActor =
    vtkSmartPointer<vtkActor>::New();
  outlineActor->SetMapper(outlineMapper);

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
  vtkSmartPointer<vtkTPWCallback> myCallback =
    vtkSmartPointer<vtkTPWCallback>::New();
  myCallback->PolyData = plane;
  myCallback->Actor = probeActor;

  // The plane widget is used probe the dataset.
  //
  vtkSmartPointer<vtkPlaneWidget> planeWidget =
    vtkSmartPointer<vtkPlaneWidget>::New();
  planeWidget->SetInteractor(iren);
  planeWidget->SetInput(pl3d->GetOutput());
  planeWidget->NormalToXAxisOn();
  planeWidget->SetResolution(20);
  planeWidget->SetRepresentationToOutline();
  planeWidget->PlaceWidget();
  planeWidget->AddObserver(vtkCommand::InteractionEvent,myCallback);

  ren1->AddActor(probeActor);
  ren1->AddActor(outlineActor);

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
  recorder->SetInputString(TPWeventLog);

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
