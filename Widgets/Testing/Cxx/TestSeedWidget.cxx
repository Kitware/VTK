/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSeedWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the vtkSeedWidget

// First include the required header files for the VTK classes we are using.
#include "vtkSeedWidget.h"
#include "vtkSeedRepresentation.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCommand.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkRegressionTestImage.h"
#include "vtkDebugLeaks.h"
#include "vtkCoordinate.h"
#include "vtkMath.h"
#include "vtkHandleWidget.h"
#include "vtkPointHandleRepresentation2D.h"
#include "vtkAxisActor2D.h"
#include "vtkProperty2D.h"


char TestSeedWidgetEventLog[] =
"# StreamVersion 1 i\n"
"CharEvent 185 179 0 0 105 1 i\n"
"KeyReleaseEvent 185 179 0 0 105 1 i\n"
"MouseMoveEvent 138 180 0 0 0 0 0 i\n"
"MouseMoveEvent 137 180 0 0 0 0 0 i\n"
"MouseMoveEvent 136 180 0 0 0 0 0 i\n"
"MouseMoveEvent 135 180 0 0 0 0 0 i\n"
"MouseMoveEvent 134 180 0 0 0 0 0 i\n"
"MouseMoveEvent 133 180 0 0 0 0 0 i\n"
"MouseMoveEvent 132 180 0 0 0 0 0 i\n"
"MouseMoveEvent 131 180 0 0 0 0 0 i\n"
"MouseMoveEvent 130 180 0 0 0 0 0 i\n"
"MouseMoveEvent 129 181 0 0 0 0 0 i\n"
"MouseMoveEvent 128 181 0 0 0 0 0 i\n"
"MouseMoveEvent 127 181 0 0 0 0 0 i\n"
"LeftButtonPressEvent 127 181 0 0 0 0 0 i\n"
"RenderEvent 127 181 0 0 0 0 0 i\n"
"MouseMoveEvent 124 181 0 0 0 0 0 i\n"
"RenderEvent 124 181 0 0 0 0 0 i\n"
"LeftButtonReleaseEvent 126 181 0 0 0 0 0 i\n"
"MouseMoveEvent 124 181 0 0 0 0 0 i\n"
"RenderEvent 124 181 0 0 0 0 0 i\n"
"MouseMoveEvent 96 144 0 0 0 0 0 i\n"
"RenderEvent 96 144 0 0 0 0 0 i\n"
"MouseMoveEvent 96 143 0 0 0 0 0 i\n"
"RenderEvent 96 143 0 0 0 0 0 i\n"
"MouseMoveEvent 96 142 0 0 0 0 0 i\n"
"RenderEvent 96 142 0 0 0 0 0 i\n"
"MouseMoveEvent 96 141 0 0 0 0 0 i\n"
"RenderEvent 96 141 0 0 0 0 0 i\n"
"MouseMoveEvent 96 140 0 0 0 0 0 i\n"
"RenderEvent 96 140 0 0 0 0 0 i\n"
"MouseMoveEvent 96 139 0 0 0 0 0 i\n"
"RenderEvent 96 139 0 0 0 0 0 i\n"
"MouseMoveEvent 96 138 0 0 0 0 0 i\n"
"RenderEvent 96 138 0 0 0 0 0 i\n"
"LeftButtonPressEvent 96 138 0 0 0 0 0 i\n"
"RenderEvent 96 138 0 0 0 0 0 i\n"
"LeftButtonReleaseEvent 96 138 0 0 0 0 0 i\n"
"MouseMoveEvent 96 137 0 0 0 0 0 i\n"
"RenderEvent 96 137 0 0 0 0 0 i\n"
"MouseMoveEvent 97 137 0 0 0 0 0 i\n"
"RenderEvent 97 137 0 0 0 0 0 i\n"
"MouseMoveEvent 164 113 0 0 0 0 t i\n"
"RenderEvent 164 113 0 0 0 0 t i\n"
"MouseMoveEvent 163 113 0 0 0 0 t i\n"
"RenderEvent 163 113 0 0 0 0 t i\n"
"MouseMoveEvent 162 113 0 0 0 0 t i\n"
"RenderEvent 162 113 0 0 0 0 t i\n"
"MouseMoveEvent 161 113 0 0 0 0 t i\n"
"RenderEvent 161 113 0 0 0 0 t i\n"
"MouseMoveEvent 161 114 0 0 0 0 t i\n"
"RenderEvent 161 114 0 0 0 0 t i\n"
"LeftButtonPressEvent 161 114 0 0 0 0 t i\n"
"RenderEvent 161 114 0 0 0 0 t i\n"
"LeftButtonReleaseEvent 161 114 0 0 0 0 t i\n"
"MouseMoveEvent 161 115 0 0 0 0 t i\n"
"RenderEvent 161 115 0 0 0 0 t i\n"
"MouseMoveEvent 161 116 0 0 0 0 t i\n"
"RenderEvent 161 116 0 0 0 0 t i\n"
"MouseMoveEvent 161 117 0 0 0 0 t i\n"
"RenderEvent 161 117 0 0 0 0 t i\n"
"MouseMoveEvent 185 158 0 0 0 0 t i\n"
"RenderEvent 185 158 0 0 0 0 t i\n"
"MouseMoveEvent 185 159 0 0 0 0 t i\n"
"RenderEvent 185 159 0 0 0 0 t i\n"
"MouseMoveEvent 186 159 0 0 0 0 t i\n"
"RenderEvent 186 159 0 0 0 0 t i\n"
"LeftButtonPressEvent 186 159 0 0 0 0 t i\n"
"RenderEvent 186 159 0 0 0 0 t i\n"
"LeftButtonReleaseEvent 186 159 0 0 0 0 t i\n"
"MouseMoveEvent 185 159 0 0 0 0 t i\n"
"RenderEvent 185 159 0 0 0 0 t i\n"
"MouseMoveEvent 184 159 0 0 0 0 t i\n"
"RenderEvent 184 159 0 0 0 0 t i\n"
"MouseMoveEvent 183 159 0 0 0 0 t i\n"
"RenderEvent 183 159 0 0 0 0 t i\n"
"MouseMoveEvent 182 159 0 0 0 0 t i\n"
"RenderEvent 182 159 0 0 0 0 t i\n"
"MouseMoveEvent 181 160 0 0 0 0 t i\n"
"RenderEvent 181 160 0 0 0 0 t i\n"
"MouseMoveEvent 131 176 0 0 0 0 t i\n"
"RenderEvent 131 176 0 0 0 0 t i\n"
"MouseMoveEvent 130 176 0 0 0 0 t i\n"
"RenderEvent 130 176 0 0 0 0 t i\n"
"MouseMoveEvent 130 177 0 0 0 0 t i\n"
"RenderEvent 130 177 0 0 0 0 t i\n"
"MouseMoveEvent 129 177 0 0 0 0 t i\n"
"RenderEvent 129 177 0 0 0 0 t i\n"
"MouseMoveEvent 128 177 0 0 0 0 t i\n"
"RenderEvent 128 177 0 0 0 0 t i\n"
"MouseMoveEvent 128 178 0 0 0 0 t i\n"
"RenderEvent 128 178 0 0 0 0 t i\n"
"MouseMoveEvent 127 179 0 0 0 0 t i\n"
"RenderEvent 127 179 0 0 0 0 t i\n"
"MouseMoveEvent 127 180 0 0 0 0 t i\n"
"RenderEvent 127 180 0 0 0 0 t i\n"
"LeftButtonPressEvent 127 180 0 0 0 0 t i\n"
"RenderEvent 127 180 0 0 0 0 t i\n"
"MouseMoveEvent 127 179 0 0 0 0 t i\n"
"RenderEvent 127 179 0 0 0 0 t i\n"
"MouseMoveEvent 128 178 0 0 0 0 t i\n"
"RenderEvent 128 178 0 0 0 0 t i\n"
"MouseMoveEvent 129 177 0 0 0 0 t i\n"
"RenderEvent 129 177 0 0 0 0 t i\n"
"MouseMoveEvent 129 176 0 0 0 0 t i\n"
"RenderEvent 129 176 0 0 0 0 t i\n"
"MouseMoveEvent 130 175 0 0 0 0 t i\n"
"RenderEvent 130 175 0 0 0 0 t i\n"
"MouseMoveEvent 131 173 0 0 0 0 t i\n"
"RenderEvent 131 173 0 0 0 0 t i\n"
"MouseMoveEvent 132 172 0 0 0 0 t i\n"
"RenderEvent 132 172 0 0 0 0 t i\n"
"MouseMoveEvent 133 171 0 0 0 0 t i\n"
"RenderEvent 133 171 0 0 0 0 t i\n"
"MouseMoveEvent 137 167 0 0 0 0 t i\n"
"RenderEvent 137 167 0 0 0 0 t i\n"
"MouseMoveEvent 138 166 0 0 0 0 t i\n"
"RenderEvent 138 166 0 0 0 0 t i\n"
"MouseMoveEvent 138 164 0 0 0 0 t i\n"
"RenderEvent 138 164 0 0 0 0 t i\n"
"MouseMoveEvent 140 163 0 0 0 0 t i\n"
"RenderEvent 140 163 0 0 0 0 t i\n"
"MouseMoveEvent 140 162 0 0 0 0 t i\n"
"RenderEvent 140 162 0 0 0 0 t i\n"
"MouseMoveEvent 141 161 0 0 0 0 t i\n"
"RenderEvent 141 161 0 0 0 0 t i\n"
"MouseMoveEvent 142 160 0 0 0 0 t i\n"
"RenderEvent 142 160 0 0 0 0 t i\n"
"MouseMoveEvent 143 159 0 0 0 0 t i\n"
"RenderEvent 143 159 0 0 0 0 t i\n"
"MouseMoveEvent 144 158 0 0 0 0 t i\n"
"RenderEvent 144 158 0 0 0 0 t i\n"
"MouseMoveEvent 144 157 0 0 0 0 t i\n"
"RenderEvent 144 157 0 0 0 0 t i\n"
"MouseMoveEvent 145 156 0 0 0 0 t i\n"
"RenderEvent 145 156 0 0 0 0 t i\n"
"MouseMoveEvent 146 155 0 0 0 0 t i\n"
"RenderEvent 146 155 0 0 0 0 t i\n"
"MouseMoveEvent 147 154 0 0 0 0 t i\n"
"RenderEvent 147 154 0 0 0 0 t i\n"
"MouseMoveEvent 148 153 0 0 0 0 t i\n"
"RenderEvent 148 153 0 0 0 0 t i\n"
"MouseMoveEvent 148 152 0 0 0 0 t i\n"
"RenderEvent 148 152 0 0 0 0 t i\n"
"MouseMoveEvent 149 151 0 0 0 0 t i\n"
"RenderEvent 149 151 0 0 0 0 t i\n"
"MouseMoveEvent 150 150 0 0 0 0 t i\n"
"RenderEvent 150 150 0 0 0 0 t i\n"
"MouseMoveEvent 151 149 0 0 0 0 t i\n"
"RenderEvent 151 149 0 0 0 0 t i\n"
"MouseMoveEvent 152 147 0 0 0 0 t i\n"
"RenderEvent 152 147 0 0 0 0 t i\n"
"MouseMoveEvent 153 146 0 0 0 0 t i\n"
"RenderEvent 153 146 0 0 0 0 t i\n"
"MouseMoveEvent 154 144 0 0 0 0 t i\n"
"RenderEvent 154 144 0 0 0 0 t i\n"
"MouseMoveEvent 156 143 0 0 0 0 t i\n"
"RenderEvent 156 143 0 0 0 0 t i\n"
"MouseMoveEvent 157 142 0 0 0 0 t i\n"
"RenderEvent 157 142 0 0 0 0 t i\n"
"MouseMoveEvent 158 141 0 0 0 0 t i\n"
"RenderEvent 158 141 0 0 0 0 t i\n"
"MouseMoveEvent 159 140 0 0 0 0 t i\n"
"RenderEvent 159 140 0 0 0 0 t i\n"
"MouseMoveEvent 160 139 0 0 0 0 t i\n"
"RenderEvent 160 139 0 0 0 0 t i\n"
"MouseMoveEvent 161 138 0 0 0 0 t i\n"
"RenderEvent 161 138 0 0 0 0 t i\n"
"MouseMoveEvent 162 138 0 0 0 0 t i\n"
"RenderEvent 162 138 0 0 0 0 t i\n"
"MouseMoveEvent 163 137 0 0 0 0 t i\n"
"RenderEvent 163 137 0 0 0 0 t i\n"
"MouseMoveEvent 164 136 0 0 0 0 t i\n"
"RenderEvent 164 136 0 0 0 0 t i\n"
"MouseMoveEvent 165 135 0 0 0 0 t i\n"
"RenderEvent 165 135 0 0 0 0 t i\n"
"MouseMoveEvent 171 133 0 0 0 0 t i\n"
"RenderEvent 171 133 0 0 0 0 t i\n"
"MouseMoveEvent 172 131 0 0 0 0 t i\n"
"RenderEvent 172 131 0 0 0 0 t i\n"
"MouseMoveEvent 174 130 0 0 0 0 t i\n"
"RenderEvent 174 130 0 0 0 0 t i\n"
"MouseMoveEvent 176 129 0 0 0 0 t i\n"
"RenderEvent 176 129 0 0 0 0 t i\n"
"MouseMoveEvent 180 125 0 0 0 0 t i\n"
"RenderEvent 180 125 0 0 0 0 t i\n"
"MouseMoveEvent 181 124 0 0 0 0 t i\n"
"RenderEvent 181 124 0 0 0 0 t i\n"
"MouseMoveEvent 183 123 0 0 0 0 t i\n"
"RenderEvent 183 123 0 0 0 0 t i\n"
"MouseMoveEvent 184 122 0 0 0 0 t i\n"
"RenderEvent 184 122 0 0 0 0 t i\n"
"MouseMoveEvent 186 121 0 0 0 0 t i\n"
"RenderEvent 186 121 0 0 0 0 t i\n"
"MouseMoveEvent 187 121 0 0 0 0 t i\n"
"RenderEvent 187 121 0 0 0 0 t i\n"
"MouseMoveEvent 188 120 0 0 0 0 t i\n"
"RenderEvent 188 120 0 0 0 0 t i\n"
"MouseMoveEvent 189 120 0 0 0 0 t i\n"
"RenderEvent 189 120 0 0 0 0 t i\n"
"MouseMoveEvent 189 119 0 0 0 0 t i\n"
"RenderEvent 189 119 0 0 0 0 t i\n"
"MouseMoveEvent 190 119 0 0 0 0 t i\n"
"RenderEvent 190 119 0 0 0 0 t i\n"
"MouseMoveEvent 191 119 0 0 0 0 t i\n"
"RenderEvent 191 119 0 0 0 0 t i\n"
"MouseMoveEvent 191 118 0 0 0 0 t i\n"
"RenderEvent 191 118 0 0 0 0 t i\n"
"MouseMoveEvent 192 118 0 0 0 0 t i\n"
"RenderEvent 192 118 0 0 0 0 t i\n"
"MouseMoveEvent 193 118 0 0 0 0 t i\n"
"RenderEvent 193 118 0 0 0 0 t i\n"
"MouseMoveEvent 194 118 0 0 0 0 t i\n"
"RenderEvent 194 118 0 0 0 0 t i\n"
"MouseMoveEvent 194 117 0 0 0 0 t i\n"
"RenderEvent 194 117 0 0 0 0 t i\n"
"MouseMoveEvent 195 117 0 0 0 0 t i\n"
"RenderEvent 195 117 0 0 0 0 t i\n"
"LeftButtonReleaseEvent 195 117 0 0 0 0 t i\n"
"RenderEvent 195 117 0 0 0 0 t i\n"
"MouseMoveEvent 194 117 0 0 0 0 t i\n"
"RenderEvent 194 117 0 0 0 0 t i\n"
"MouseMoveEvent 193 117 0 0 0 0 t i\n"
"RenderEvent 193 117 0 0 0 0 t i\n"
"MouseMoveEvent 192 117 0 0 0 0 t i\n"
"RenderEvent 192 117 0 0 0 0 t i\n"
"MouseMoveEvent 191 117 0 0 0 0 t i\n"
"RenderEvent 191 117 0 0 0 0 t i\n"
"MouseMoveEvent 190 117 0 0 0 0 t i\n"
"RenderEvent 190 117 0 0 0 0 t i\n"
"MouseMoveEvent 189 117 0 0 0 0 t i\n"
"RenderEvent 189 117 0 0 0 0 t i\n"
"MouseMoveEvent 188 117 0 0 0 0 t i\n"
"RenderEvent 188 117 0 0 0 0 t i\n"
"MouseMoveEvent 187 117 0 0 0 0 t i\n"
"RenderEvent 187 117 0 0 0 0 t i\n"
"MouseMoveEvent 186 116 0 0 0 0 t i\n"
"RenderEvent 186 116 0 0 0 0 t i\n"
"MouseMoveEvent 185 116 0 0 0 0 t i\n"
"RenderEvent 185 116 0 0 0 0 t i\n"
"MouseMoveEvent 184 116 0 0 0 0 t i\n"
"RenderEvent 184 116 0 0 0 0 t i\n"
"MouseMoveEvent 184 115 0 0 0 0 t i\n"
"RenderEvent 184 115 0 0 0 0 t i\n"
"MouseMoveEvent 183 115 0 0 0 0 t i\n"
"RenderEvent 183 115 0 0 0 0 t i\n"
"MouseMoveEvent 182 115 0 0 0 0 t i\n"
"RenderEvent 182 115 0 0 0 0 t i\n"
"MouseMoveEvent 181 114 0 0 0 0 t i\n"
"RenderEvent 181 114 0 0 0 0 t i\n"
"MouseMoveEvent 180 114 0 0 0 0 t i\n"
"RenderEvent 180 114 0 0 0 0 t i\n"
"MouseMoveEvent 179 114 0 0 0 0 t i\n"
"RenderEvent 179 114 0 0 0 0 t i\n"
"MouseMoveEvent 178 114 0 0 0 0 t i\n"
"RenderEvent 178 114 0 0 0 0 t i\n"
"MouseMoveEvent 177 113 0 0 0 0 t i\n"
"RenderEvent 177 113 0 0 0 0 t i\n"
"MouseMoveEvent 176 113 0 0 0 0 t i\n"
"RenderEvent 176 113 0 0 0 0 t i\n"
"MouseMoveEvent 174 112 0 0 0 0 t i\n"
"RenderEvent 174 112 0 0 0 0 t i\n"
"MouseMoveEvent 173 112 0 0 0 0 t i\n"
"RenderEvent 173 112 0 0 0 0 t i\n"
"MouseMoveEvent 171 112 0 0 0 0 t i\n"
"RenderEvent 171 112 0 0 0 0 t i\n"
"MouseMoveEvent 170 112 0 0 0 0 t i\n"
"RenderEvent 170 112 0 0 0 0 t i\n"
"MouseMoveEvent 169 112 0 0 0 0 t i\n"
"RenderEvent 169 112 0 0 0 0 t i\n"
"MouseMoveEvent 167 112 0 0 0 0 t i\n"
"RenderEvent 167 112 0 0 0 0 t i\n"
"MouseMoveEvent 166 111 0 0 0 0 t i\n"
"RenderEvent 166 111 0 0 0 0 t i\n"
"MouseMoveEvent 165 111 0 0 0 0 t i\n"
"RenderEvent 165 111 0 0 0 0 t i\n"
"MouseMoveEvent 164 111 0 0 0 0 t i\n"
"RenderEvent 164 111 0 0 0 0 t i\n"
"MouseMoveEvent 163 111 0 0 0 0 t i\n"
"RenderEvent 163 111 0 0 0 0 t i\n"
"MouseMoveEvent 162 110 0 0 0 0 t i\n"
"RenderEvent 162 110 0 0 0 0 t i\n"
"MouseMoveEvent 161 110 0 0 0 0 t i\n"
"RenderEvent 161 110 0 0 0 0 t i\n"
"MouseMoveEvent 160 110 0 0 0 0 t i\n"
"RenderEvent 160 110 0 0 0 0 t i\n"
"MouseMoveEvent 160 111 0 0 0 0 t i\n"
"RenderEvent 160 111 0 0 0 0 t i\n"
"MouseMoveEvent 159 111 0 0 0 0 t i\n"
"RenderEvent 159 111 0 0 0 0 t i\n"
"MouseMoveEvent 159 112 0 0 0 0 t i\n"
"RenderEvent 159 112 0 0 0 0 t i\n"
"MouseMoveEvent 159 113 0 0 0 0 t i\n"
"RenderEvent 159 113 0 0 0 0 t i\n"
"MouseMoveEvent 159 114 0 0 0 0 t i\n"
"RenderEvent 159 114 0 0 0 0 t i\n"
"LeftButtonPressEvent 159 114 0 0 0 0 t i\n"
"RenderEvent 159 114 0 0 0 0 t i\n"
"MouseMoveEvent 136 178 0 0 0 0 t i\n"
"RenderEvent 136 178 0 0 0 0 t i\n"
"MouseMoveEvent 135 179 0 0 0 0 t i\n"
"RenderEvent 135 179 0 0 0 0 t i\n"
"MouseMoveEvent 135 180 0 0 0 0 t i\n"
"RenderEvent 135 180 0 0 0 0 t i\n"
"MouseMoveEvent 134 181 0 0 0 0 t i\n"
"RenderEvent 134 181 0 0 0 0 t i\n"
"MouseMoveEvent 134 182 0 0 0 0 t i\n"
"RenderEvent 134 182 0 0 0 0 t i\n"
"LeftButtonReleaseEvent 134 182 0 0 0 0 t i\n"
"RenderEvent 134 182 0 0 0 0 t i\n"
"MouseMoveEvent 134 181 0 0 0 0 t i\n"
"RenderEvent 134 181 0 0 0 0 t i\n"
"MouseMoveEvent 72 222 0 0 0 0 t i\n"
"RenderEvent 72 222 0 0 0 0 t i\n"
"MouseMoveEvent 71 223 0 0 0 0 t i\n"
"RenderEvent 71 223 0 0 0 0 t i\n"
"MouseMoveEvent 71 224 0 0 0 0 t i\n"
"RenderEvent 71 224 0 0 0 0 t i\n"
"MouseMoveEvent 71 225 0 0 0 0 t i\n"
"RenderEvent 71 225 0 0 0 0 t i\n"
"LeftButtonPressEvent 71 225 0 0 0 0 t i\n"
"RenderEvent 71 225 0 0 0 0 t i\n"
"LeftButtonReleaseEvent 71 225 0 0 0 0 t i\n"
"MouseMoveEvent 70 225 0 0 0 0 t i\n"
"RenderEvent 70 225 0 0 0 0 t i\n"
"MouseMoveEvent 70 224 0 0 0 0 t i\n"
"RenderEvent 70 224 0 0 0 0 t i\n"
"MouseMoveEvent 69 223 0 0 0 0 t i\n"
"RenderEvent 69 223 0 0 0 0 t i\n"
"MouseMoveEvent 185 162 0 0 0 0 t i\n"
"RenderEvent 185 162 0 0 0 0 t i\n"
"MouseMoveEvent 184 162 0 0 0 0 t i\n"
"RenderEvent 184 162 0 0 0 0 t i\n"
"MouseMoveEvent 183 162 0 0 0 0 t i\n"
"RenderEvent 183 162 0 0 0 0 t i\n"
"MouseMoveEvent 182 162 0 0 0 0 t i\n"
"RenderEvent 182 162 0 0 0 0 t i\n"
"MouseMoveEvent 183 162 0 0 0 0 t i\n"
"RenderEvent 183 162 0 0 0 0 t i\n"
"MouseMoveEvent 184 162 0 0 0 0 t i\n"
"RenderEvent 184 162 0 0 0 0 t i\n"
"MouseMoveEvent 184 161 0 0 0 0 t i\n"
"RenderEvent 184 161 0 0 0 0 t i\n"
"MouseMoveEvent 185 161 0 0 0 0 t i\n"
"RenderEvent 185 161 0 0 0 0 t i\n"
"LeftButtonPressEvent 185 161 0 0 0 0 t i\n"
"RenderEvent 185 161 0 0 0 0 t i\n"
"MouseMoveEvent 185 160 0 0 0 0 t i\n"
"RenderEvent 185 160 0 0 0 0 t i\n"
"MouseMoveEvent 129 108 0 0 0 0 t i\n"
"RenderEvent 129 108 0 0 0 0 t i\n"
"MouseMoveEvent 129 107 0 0 0 0 t i\n"
"RenderEvent 129 107 0 0 0 0 t i\n"
"MouseMoveEvent 127 107 0 0 0 0 t i\n"
"RenderEvent 127 107 0 0 0 0 t i\n"
"MouseMoveEvent 126 106 0 0 0 0 t i\n"
"RenderEvent 126 106 0 0 0 0 t i\n"
"MouseMoveEvent 125 105 0 0 0 0 t i\n"
"RenderEvent 125 105 0 0 0 0 t i\n"
"MouseMoveEvent 124 105 0 0 0 0 t i\n"
"RenderEvent 124 105 0 0 0 0 t i\n"
"MouseMoveEvent 124 104 0 0 0 0 t i\n"
"RenderEvent 124 104 0 0 0 0 t i\n"
"LeftButtonReleaseEvent 124 104 0 0 0 0 t i\n"
"RenderEvent 124 104 0 0 0 0 t i\n"
"MouseMoveEvent 185 166 0 0 0 0 t i\n"
"RenderEvent 185 166 0 0 0 0 t i\n"
"RightButtonPressEvent 185 166 0 0 0 0 t i\n"
"RightButtonReleaseEvent 185 166 0 0 0 0 t i\n"
"MouseMoveEvent 184 166 0 0 0 0 t i\n"
"MouseMoveEvent 183 166 0 0 0 0 t i\n"
"MouseMoveEvent 181 166 0 0 0 0 t i\n"
"MouseMoveEvent 179 166 0 0 0 0 t i\n"
"MouseMoveEvent 178 166 0 0 0 0 t i\n"
"MouseMoveEvent 177 166 0 0 0 0 t i\n"
"MouseMoveEvent 157 168 0 0 0 0 t i\n"
"MouseMoveEvent 156 168 0 0 0 0 t i\n"
"MouseMoveEvent 155 168 0 0 0 0 t i\n"
"MouseMoveEvent 154 168 0 0 0 0 t i\n"
"LeftButtonPressEvent 154 168 0 0 0 0 t i\n"
"StartInteractionEvent 154 168 0 0 0 0 t i\n"
"MouseMoveEvent 155 168 0 0 0 0 t i\n"
"RenderEvent 155 168 0 0 0 0 t i\n"
"MouseMoveEvent 148 161 0 0 0 0 t i\n"
"RenderEvent 148 161 0 0 0 0 t i\n"
"LeftButtonReleaseEvent 148 161 0 0 0 0 t i\n"
"EndInteractionEvent 148 161 0 0 0 0 t i\n"
"RenderEvent 148 161 0 0 0 0 t i\n"
"MouseMoveEvent 148 160 0 0 0 0 t i\n"
"MouseMoveEvent 148 159 0 0 0 0 t i\n"
"MouseMoveEvent 144 140 0 0 0 0 t i\n"
"MouseMoveEvent 144 139 0 0 0 0 t i\n"
"MouseMoveEvent 144 138 0 0 0 0 t i\n"
"MouseMoveEvent 144 137 0 0 0 0 t i\n"
"MouseMoveEvent 144 136 0 0 0 0 t i\n"
"MouseMoveEvent 144 135 0 0 0 0 t i\n"
;



// This callback is responsible for setting the seed label.
class vtkSeedCallback : public vtkCommand
{
public:
  static vtkSeedCallback *New() 
    { return new vtkSeedCallback; }
  virtual void Execute(vtkObject*, unsigned long event, void *calldata)
    {
    if (event == vtkCommand::PlacePointEvent)
      {
      cout << "Point placed, total of: " 
           << this->SeedRepresentation->GetNumberOfSeeds() << endl;
      }
    if (event == vtkCommand::InteractionEvent)
      {
      if (calldata)
        {
        cout << "Interacting with seed : " 
             << *(static_cast< int * >(calldata)) << endl;
        }
      }
    }
  vtkSeedCallback() : SeedRepresentation(0) {}
  vtkSeedRepresentation *SeedRepresentation;
};


// The actual test function
int TestSeedWidget( int argc, char *argv[] )
{
  // Create the RenderWindow, Renderer and both Actors
  //
  vtkRenderer *ren1 = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren1);

  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  // Create a test pipeline
  //
  vtkSphereSource *ss = vtkSphereSource::New();
  vtkPolyDataMapper *mapper = vtkPolyDataMapper::New();
  mapper->SetInput(ss->GetOutput());
  vtkActor *actor = vtkActor::New();
  actor->SetMapper(mapper);

  // Create the widget and its representation
  vtkPointHandleRepresentation2D *handle = vtkPointHandleRepresentation2D::New();
  handle->GetProperty()->SetColor(1,0,0);
  vtkSeedRepresentation *rep = vtkSeedRepresentation::New();
  rep->SetHandleRepresentation(handle);

  vtkSeedWidget *widget = vtkSeedWidget::New();
  widget->SetInteractor(iren);
  widget->SetRepresentation(rep);

  vtkSeedCallback *scbk = vtkSeedCallback::New();
  scbk->SeedRepresentation = rep;
  widget->AddObserver(vtkCommand::PlacePointEvent,scbk);
  widget->AddObserver(vtkCommand::InteractionEvent,scbk);

  // Add the actors to the renderer, set the background and size
  //
  ren1->AddActor(actor);
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // record events
  vtkInteractorEventRecorder *recorder = vtkInteractorEventRecorder::New();
  recorder->SetInteractor(iren);
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(TestSeedWidgetEventLog);

  // render the image
  
  iren->Initialize();
  renWin->Render();
  recorder->Play();

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();

  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }


  // test removing seeds
  const int startNumSeeds = rep->GetNumberOfSeeds();
  for (int s = 0; s < startNumSeeds; s++)
    {
    widget->DeleteSeed(0);
    }

  const int endNumSeeds = rep->GetNumberOfSeeds();
  if (endNumSeeds != 0)
    {
    cerr << "After deleting " << startNumSeeds << ", now have " 
         << endNumSeeds << endl;
    retVal = 0;

    if (widget->GetSeed(0) != NULL)
      {
      vtkSeedRepresentation *seedRep =  vtkSeedRepresentation::SafeDownCast(
                                                  widget->GetRepresentation());
      const int widgetStartNumSeeds = seedRep->GetNumberOfSeeds();
      cerr << "Still have a seed 0 after deleting all seeds, "
           << "widget thinks it's rep has " << widgetStartNumSeeds << endl;    
      }
    
    }
  
  

  recorder->Delete();
  ss->Delete();
  mapper->Delete();
  actor->Delete();
  handle->Delete();
  rep->Delete();
  widget->RemoveObserver(scbk);
  scbk->Delete();
  widget->Off();
  widget->Delete();
  iren->Delete();
  renWin->Delete();
  ren1->Delete();
  
  return !retVal;
}
