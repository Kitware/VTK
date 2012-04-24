/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestRectilinearWipeWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the vtkRectilinearWipeWidget.

// First include the required header files for the VTK classes we are using.
#include "vtkSmartPointer.h"
#include "vtkRectilinearWipeWidget.h"
#include "vtkRectilinearWipeRepresentation.h"
#include "vtkImageCanvasSource2D.h"
#include "vtkImageWrapPad.h"
#include "vtkImageCanvasSource2D.h"
#include "vtkImageRectilinearWipe.h"
#include "vtkImageActor.h"
#include "vtkImageMapper3D.h"
#include "vtkImageData.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkProperty2D.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkCommand.h"

const char eventLog[] =
"# StreamVersion 1\n"
"RenderEvent 0 0 0 0 0 0 0\n"
"EnterEvent 296 73 0 0 0 0 0\n"
"MouseMoveEvent 296 73 0 0 0 0 0\n"
"MouseMoveEvent 88 148 0 0 0 0 0\n"
"LeftButtonPressEvent 88 148 0 0 0 0 0\n"
"MouseMoveEvent 87 148 0 0 0 0 0\n"
"RenderEvent 87 148 0 0 0 0 0\n"
"MouseMoveEvent 87 149 0 0 0 0 0\n"
"RenderEvent 87 149 0 0 0 0 0\n"
"MouseMoveEvent 87 150 0 0 0 0 0\n"
"RenderEvent 87 150 0 0 0 0 0\n"
"MouseMoveEvent 87 151 0 0 0 0 0\n"
"RenderEvent 87 151 0 0 0 0 0\n"
"MouseMoveEvent 87 152 0 0 0 0 0\n"
"RenderEvent 87 152 0 0 0 0 0\n"
"MouseMoveEvent 87 153 0 0 0 0 0\n"
"RenderEvent 87 153 0 0 0 0 0\n"
"MouseMoveEvent 87 154 0 0 0 0 0\n"
"RenderEvent 87 154 0 0 0 0 0\n"
"MouseMoveEvent 87 155 0 0 0 0 0\n"
"RenderEvent 87 155 0 0 0 0 0\n"
"MouseMoveEvent 87 156 0 0 0 0 0\n"
"RenderEvent 87 156 0 0 0 0 0\n"
"MouseMoveEvent 87 157 0 0 0 0 0\n"
"RenderEvent 87 157 0 0 0 0 0\n"
"MouseMoveEvent 87 158 0 0 0 0 0\n"
"RenderEvent 87 158 0 0 0 0 0\n"
"MouseMoveEvent 87 159 0 0 0 0 0\n"
"RenderEvent 87 159 0 0 0 0 0\n"
"MouseMoveEvent 87 161 0 0 0 0 0\n"
"RenderEvent 87 161 0 0 0 0 0\n"
"MouseMoveEvent 87 162 0 0 0 0 0\n"
"RenderEvent 87 162 0 0 0 0 0\n"
"MouseMoveEvent 87 163 0 0 0 0 0\n"
"RenderEvent 87 163 0 0 0 0 0\n"
"MouseMoveEvent 87 164 0 0 0 0 0\n"
"RenderEvent 87 164 0 0 0 0 0\n"
"MouseMoveEvent 87 165 0 0 0 0 0\n"
"RenderEvent 87 165 0 0 0 0 0\n"
"MouseMoveEvent 86 165 0 0 0 0 0\n"
"RenderEvent 86 165 0 0 0 0 0\n"
"MouseMoveEvent 86 167 0 0 0 0 0\n"
"RenderEvent 86 167 0 0 0 0 0\n"
"MouseMoveEvent 86 168 0 0 0 0 0\n"
"RenderEvent 86 168 0 0 0 0 0\n"
"MouseMoveEvent 85 170 0 0 0 0 0\n"
"RenderEvent 85 170 0 0 0 0 0\n"
"MouseMoveEvent 85 172 0 0 0 0 0\n"
"RenderEvent 85 172 0 0 0 0 0\n"
"MouseMoveEvent 85 175 0 0 0 0 0\n"
"RenderEvent 85 175 0 0 0 0 0\n"
"MouseMoveEvent 85 177 0 0 0 0 0\n"
"RenderEvent 85 177 0 0 0 0 0\n"
"MouseMoveEvent 85 178 0 0 0 0 0\n"
"RenderEvent 85 178 0 0 0 0 0\n"
"MouseMoveEvent 85 179 0 0 0 0 0\n"
"RenderEvent 85 179 0 0 0 0 0\n"
"MouseMoveEvent 85 181 0 0 0 0 0\n"
"RenderEvent 85 181 0 0 0 0 0\n"
"MouseMoveEvent 85 184 0 0 0 0 0\n"
"RenderEvent 85 184 0 0 0 0 0\n"
"MouseMoveEvent 85 185 0 0 0 0 0\n"
"RenderEvent 85 185 0 0 0 0 0\n"
"MouseMoveEvent 85 187 0 0 0 0 0\n"
"RenderEvent 85 187 0 0 0 0 0\n"
"MouseMoveEvent 85 188 0 0 0 0 0\n"
"RenderEvent 85 188 0 0 0 0 0\n"
"MouseMoveEvent 85 189 0 0 0 0 0\n"
"RenderEvent 85 189 0 0 0 0 0\n"
"MouseMoveEvent 85 190 0 0 0 0 0\n"
"RenderEvent 85 190 0 0 0 0 0\n"
"MouseMoveEvent 83 191 0 0 0 0 0\n"
"RenderEvent 83 191 0 0 0 0 0\n"
"MouseMoveEvent 83 192 0 0 0 0 0\n"
"RenderEvent 83 192 0 0 0 0 0\n"
"MouseMoveEvent 82 192 0 0 0 0 0\n"
"RenderEvent 82 192 0 0 0 0 0\n"
"MouseMoveEvent 83 192 0 0 0 0 0\n"
"RenderEvent 83 192 0 0 0 0 0\n"
"MouseMoveEvent 84 192 0 0 0 0 0\n"
"RenderEvent 84 192 0 0 0 0 0\n"
"MouseMoveEvent 86 192 0 0 0 0 0\n"
"RenderEvent 86 192 0 0 0 0 0\n"
"MouseMoveEvent 87 192 0 0 0 0 0\n"
"RenderEvent 87 192 0 0 0 0 0\n"
"MouseMoveEvent 89 192 0 0 0 0 0\n"
"RenderEvent 89 192 0 0 0 0 0\n"
"MouseMoveEvent 90 192 0 0 0 0 0\n"
"RenderEvent 90 192 0 0 0 0 0\n"
"MouseMoveEvent 91 192 0 0 0 0 0\n"
"RenderEvent 91 192 0 0 0 0 0\n"
"MouseMoveEvent 92 192 0 0 0 0 0\n"
"RenderEvent 92 192 0 0 0 0 0\n"
"MouseMoveEvent 93 192 0 0 0 0 0\n"
"RenderEvent 93 192 0 0 0 0 0\n"
"MouseMoveEvent 94 192 0 0 0 0 0\n"
"RenderEvent 94 192 0 0 0 0 0\n"
"MouseMoveEvent 95 192 0 0 0 0 0\n"
"RenderEvent 95 192 0 0 0 0 0\n"
"MouseMoveEvent 96 192 0 0 0 0 0\n"
"RenderEvent 96 192 0 0 0 0 0\n"
"MouseMoveEvent 100 192 0 0 0 0 0\n"
"RenderEvent 100 192 0 0 0 0 0\n"
"MouseMoveEvent 103 192 0 0 0 0 0\n"
"RenderEvent 103 192 0 0 0 0 0\n"
"MouseMoveEvent 104 192 0 0 0 0 0\n"
"RenderEvent 104 192 0 0 0 0 0\n"
"MouseMoveEvent 106 192 0 0 0 0 0\n"
"RenderEvent 106 192 0 0 0 0 0\n"
"MouseMoveEvent 107 192 0 0 0 0 0\n"
"RenderEvent 107 192 0 0 0 0 0\n"
"MouseMoveEvent 108 192 0 0 0 0 0\n"
"RenderEvent 108 192 0 0 0 0 0\n"
"MouseMoveEvent 109 192 0 0 0 0 0\n"
"RenderEvent 109 192 0 0 0 0 0\n"
"MouseMoveEvent 110 192 0 0 0 0 0\n"
"RenderEvent 110 192 0 0 0 0 0\n"
"MouseMoveEvent 112 192 0 0 0 0 0\n"
"RenderEvent 112 192 0 0 0 0 0\n"
"MouseMoveEvent 115 192 0 0 0 0 0\n"
"RenderEvent 115 192 0 0 0 0 0\n"
"MouseMoveEvent 119 192 0 0 0 0 0\n"
"RenderEvent 119 192 0 0 0 0 0\n"
"MouseMoveEvent 124 193 0 0 0 0 0\n"
"RenderEvent 124 193 0 0 0 0 0\n"
"MouseMoveEvent 130 193 0 0 0 0 0\n"
"RenderEvent 130 193 0 0 0 0 0\n"
"MouseMoveEvent 137 193 0 0 0 0 0\n"
"RenderEvent 137 193 0 0 0 0 0\n"
"MouseMoveEvent 142 193 0 0 0 0 0\n"
"RenderEvent 142 193 0 0 0 0 0\n"
"MouseMoveEvent 145 193 0 0 0 0 0\n"
"RenderEvent 145 193 0 0 0 0 0\n"
"MouseMoveEvent 147 193 0 0 0 0 0\n"
"RenderEvent 147 193 0 0 0 0 0\n"
"MouseMoveEvent 149 193 0 0 0 0 0\n"
"RenderEvent 149 193 0 0 0 0 0\n"
"MouseMoveEvent 149 194 0 0 0 0 0\n"
"RenderEvent 149 194 0 0 0 0 0\n"
"MouseMoveEvent 150 194 0 0 0 0 0\n"
"RenderEvent 150 194 0 0 0 0 0\n"
"MouseMoveEvent 151 194 0 0 0 0 0\n"
"RenderEvent 151 194 0 0 0 0 0\n"
"MouseMoveEvent 152 194 0 0 0 0 0\n"
"RenderEvent 152 194 0 0 0 0 0\n"
"MouseMoveEvent 153 194 0 0 0 0 0\n"
"RenderEvent 153 194 0 0 0 0 0\n"
"MouseMoveEvent 154 194 0 0 0 0 0\n"
"RenderEvent 154 194 0 0 0 0 0\n"
"MouseMoveEvent 157 194 0 0 0 0 0\n"
"RenderEvent 157 194 0 0 0 0 0\n"
"MouseMoveEvent 162 194 0 0 0 0 0\n"
"RenderEvent 162 194 0 0 0 0 0\n"
"MouseMoveEvent 166 194 0 0 0 0 0\n"
"RenderEvent 166 194 0 0 0 0 0\n"
"MouseMoveEvent 169 194 0 0 0 0 0\n"
"RenderEvent 169 194 0 0 0 0 0\n"
"MouseMoveEvent 173 194 0 0 0 0 0\n"
"RenderEvent 173 194 0 0 0 0 0\n"
"MouseMoveEvent 175 194 0 0 0 0 0\n"
"RenderEvent 175 194 0 0 0 0 0\n"
"MouseMoveEvent 176 194 0 0 0 0 0\n"
"RenderEvent 176 194 0 0 0 0 0\n"
"MouseMoveEvent 177 194 0 0 0 0 0\n"
"RenderEvent 177 194 0 0 0 0 0\n"
"MouseMoveEvent 178 194 0 0 0 0 0\n"
"RenderEvent 178 194 0 0 0 0 0\n"
"MouseMoveEvent 179 194 0 0 0 0 0\n"
"RenderEvent 179 194 0 0 0 0 0\n"
"MouseMoveEvent 180 194 0 0 0 0 0\n"
"RenderEvent 180 194 0 0 0 0 0\n"
"MouseMoveEvent 181 194 0 0 0 0 0\n"
"RenderEvent 181 194 0 0 0 0 0\n"
"MouseMoveEvent 182 194 0 0 0 0 0\n"
"RenderEvent 182 194 0 0 0 0 0\n"
"MouseMoveEvent 182 195 0 0 0 0 0\n"
"RenderEvent 182 195 0 0 0 0 0\n"
"MouseMoveEvent 183 195 0 0 0 0 0\n"
"RenderEvent 183 195 0 0 0 0 0\n"
"MouseMoveEvent 184 195 0 0 0 0 0\n"
"RenderEvent 184 195 0 0 0 0 0\n"
"MouseMoveEvent 185 195 0 0 0 0 0\n"
"RenderEvent 185 195 0 0 0 0 0\n"
"MouseMoveEvent 186 195 0 0 0 0 0\n"
"RenderEvent 186 195 0 0 0 0 0\n"
"MouseMoveEvent 186 194 0 0 0 0 0\n"
"RenderEvent 186 194 0 0 0 0 0\n"
"MouseMoveEvent 186 193 0 0 0 0 0\n"
"RenderEvent 186 193 0 0 0 0 0\n"
"MouseMoveEvent 186 192 0 0 0 0 0\n"
"RenderEvent 186 192 0 0 0 0 0\n"
"MouseMoveEvent 186 191 0 0 0 0 0\n"
"RenderEvent 186 191 0 0 0 0 0\n"
"MouseMoveEvent 186 189 0 0 0 0 0\n"
"RenderEvent 186 189 0 0 0 0 0\n"
"MouseMoveEvent 186 188 0 0 0 0 0\n"
"RenderEvent 186 188 0 0 0 0 0\n"
"MouseMoveEvent 187 184 0 0 0 0 0\n"
"RenderEvent 187 184 0 0 0 0 0\n"
"MouseMoveEvent 188 181 0 0 0 0 0\n"
"RenderEvent 188 181 0 0 0 0 0\n"
"MouseMoveEvent 189 177 0 0 0 0 0\n"
"RenderEvent 189 177 0 0 0 0 0\n"
"MouseMoveEvent 189 173 0 0 0 0 0\n"
"RenderEvent 189 173 0 0 0 0 0\n"
"MouseMoveEvent 191 170 0 0 0 0 0\n"
"RenderEvent 191 170 0 0 0 0 0\n"
"MouseMoveEvent 193 167 0 0 0 0 0\n"
"RenderEvent 193 167 0 0 0 0 0\n"
"MouseMoveEvent 194 164 0 0 0 0 0\n"
"RenderEvent 194 164 0 0 0 0 0\n"
"MouseMoveEvent 195 161 0 0 0 0 0\n"
"RenderEvent 195 161 0 0 0 0 0\n"
"MouseMoveEvent 198 158 0 0 0 0 0\n"
"RenderEvent 198 158 0 0 0 0 0\n"
"MouseMoveEvent 198 156 0 0 0 0 0\n"
"RenderEvent 198 156 0 0 0 0 0\n"
"MouseMoveEvent 199 154 0 0 0 0 0\n"
"RenderEvent 199 154 0 0 0 0 0\n"
"MouseMoveEvent 199 153 0 0 0 0 0\n"
"RenderEvent 199 153 0 0 0 0 0\n"
"MouseMoveEvent 199 152 0 0 0 0 0\n"
"RenderEvent 199 152 0 0 0 0 0\n"
"MouseMoveEvent 199 150 0 0 0 0 0\n"
"RenderEvent 199 150 0 0 0 0 0\n"
"MouseMoveEvent 199 149 0 0 0 0 0\n"
"RenderEvent 199 149 0 0 0 0 0\n"
"MouseMoveEvent 199 147 0 0 0 0 0\n"
"RenderEvent 199 147 0 0 0 0 0\n"
"MouseMoveEvent 199 146 0 0 0 0 0\n"
"RenderEvent 199 146 0 0 0 0 0\n"
"MouseMoveEvent 198 144 0 0 0 0 0\n"
"RenderEvent 198 144 0 0 0 0 0\n"
"MouseMoveEvent 198 143 0 0 0 0 0\n"
"RenderEvent 198 143 0 0 0 0 0\n"
"MouseMoveEvent 197 139 0 0 0 0 0\n"
"RenderEvent 197 139 0 0 0 0 0\n"
"MouseMoveEvent 196 135 0 0 0 0 0\n"
"RenderEvent 196 135 0 0 0 0 0\n"
"MouseMoveEvent 194 131 0 0 0 0 0\n"
"RenderEvent 194 131 0 0 0 0 0\n"
"MouseMoveEvent 193 129 0 0 0 0 0\n"
"RenderEvent 193 129 0 0 0 0 0\n"
"MouseMoveEvent 192 127 0 0 0 0 0\n"
"RenderEvent 192 127 0 0 0 0 0\n"
"MouseMoveEvent 190 125 0 0 0 0 0\n"
"RenderEvent 190 125 0 0 0 0 0\n"
"MouseMoveEvent 188 124 0 0 0 0 0\n"
"RenderEvent 188 124 0 0 0 0 0\n"
"MouseMoveEvent 185 120 0 0 0 0 0\n"
"RenderEvent 185 120 0 0 0 0 0\n"
"MouseMoveEvent 181 117 0 0 0 0 0\n"
"RenderEvent 181 117 0 0 0 0 0\n"
"MouseMoveEvent 180 116 0 0 0 0 0\n"
"RenderEvent 180 116 0 0 0 0 0\n"
"MouseMoveEvent 176 113 0 0 0 0 0\n"
"RenderEvent 176 113 0 0 0 0 0\n"
"MouseMoveEvent 174 112 0 0 0 0 0\n"
"RenderEvent 174 112 0 0 0 0 0\n"
"MouseMoveEvent 172 112 0 0 0 0 0\n"
"RenderEvent 172 112 0 0 0 0 0\n"
"MouseMoveEvent 170 112 0 0 0 0 0\n"
"RenderEvent 170 112 0 0 0 0 0\n"
"MouseMoveEvent 166 112 0 0 0 0 0\n"
"RenderEvent 166 112 0 0 0 0 0\n"
"MouseMoveEvent 161 112 0 0 0 0 0\n"
"RenderEvent 161 112 0 0 0 0 0\n"
"MouseMoveEvent 156 112 0 0 0 0 0\n"
"RenderEvent 156 112 0 0 0 0 0\n"
"MouseMoveEvent 152 112 0 0 0 0 0\n"
"RenderEvent 152 112 0 0 0 0 0\n"
"MouseMoveEvent 149 112 0 0 0 0 0\n"
"RenderEvent 149 112 0 0 0 0 0\n"
"MouseMoveEvent 146 114 0 0 0 0 0\n"
"RenderEvent 146 114 0 0 0 0 0\n"
"MouseMoveEvent 142 116 0 0 0 0 0\n"
"RenderEvent 142 116 0 0 0 0 0\n"
"MouseMoveEvent 139 118 0 0 0 0 0\n"
"RenderEvent 139 118 0 0 0 0 0\n"
"MouseMoveEvent 136 120 0 0 0 0 0\n"
"RenderEvent 136 120 0 0 0 0 0\n"
"MouseMoveEvent 134 121 0 0 0 0 0\n"
"RenderEvent 134 121 0 0 0 0 0\n"
"MouseMoveEvent 133 123 0 0 0 0 0\n"
"RenderEvent 133 123 0 0 0 0 0\n"
"MouseMoveEvent 132 126 0 0 0 0 0\n"
"RenderEvent 132 126 0 0 0 0 0\n"
"MouseMoveEvent 130 130 0 0 0 0 0\n"
"RenderEvent 130 130 0 0 0 0 0\n"
"MouseMoveEvent 127 135 0 0 0 0 0\n"
"RenderEvent 127 135 0 0 0 0 0\n"
"MouseMoveEvent 126 137 0 0 0 0 0\n"
"RenderEvent 126 137 0 0 0 0 0\n"
"MouseMoveEvent 125 139 0 0 0 0 0\n"
"RenderEvent 125 139 0 0 0 0 0\n"
"MouseMoveEvent 123 142 0 0 0 0 0\n"
"RenderEvent 123 142 0 0 0 0 0\n"
"MouseMoveEvent 123 144 0 0 0 0 0\n"
"RenderEvent 123 144 0 0 0 0 0\n"
"MouseMoveEvent 121 145 0 0 0 0 0\n"
"RenderEvent 121 145 0 0 0 0 0\n"
"MouseMoveEvent 120 150 0 0 0 0 0\n"
"RenderEvent 120 150 0 0 0 0 0\n"
"MouseMoveEvent 120 154 0 0 0 0 0\n"
"RenderEvent 120 154 0 0 0 0 0\n"
"MouseMoveEvent 120 156 0 0 0 0 0\n"
"RenderEvent 120 156 0 0 0 0 0\n"
"MouseMoveEvent 120 159 0 0 0 0 0\n"
"RenderEvent 120 159 0 0 0 0 0\n"
"MouseMoveEvent 120 161 0 0 0 0 0\n"
"RenderEvent 120 161 0 0 0 0 0\n"
"MouseMoveEvent 120 162 0 0 0 0 0\n"
"RenderEvent 120 162 0 0 0 0 0\n"
"MouseMoveEvent 121 163 0 0 0 0 0\n"
"RenderEvent 121 163 0 0 0 0 0\n"
"MouseMoveEvent 122 165 0 0 0 0 0\n"
"RenderEvent 122 165 0 0 0 0 0\n"
"MouseMoveEvent 124 166 0 0 0 0 0\n"
"RenderEvent 124 166 0 0 0 0 0\n"
"MouseMoveEvent 126 167 0 0 0 0 0\n"
"RenderEvent 126 167 0 0 0 0 0\n"
"MouseMoveEvent 132 168 0 0 0 0 0\n"
"RenderEvent 132 168 0 0 0 0 0\n"
"MouseMoveEvent 135 168 0 0 0 0 0\n"
"RenderEvent 135 168 0 0 0 0 0\n"
"MouseMoveEvent 140 168 0 0 0 0 0\n"
"RenderEvent 140 168 0 0 0 0 0\n"
"MouseMoveEvent 145 168 0 0 0 0 0\n"
"RenderEvent 145 168 0 0 0 0 0\n"
"MouseMoveEvent 149 168 0 0 0 0 0\n"
"RenderEvent 149 168 0 0 0 0 0\n"
"MouseMoveEvent 150 168 0 0 0 0 0\n"
"RenderEvent 150 168 0 0 0 0 0\n"
"MouseMoveEvent 151 168 0 0 0 0 0\n"
"RenderEvent 151 168 0 0 0 0 0\n"
"MouseMoveEvent 152 168 0 0 0 0 0\n"
"RenderEvent 152 168 0 0 0 0 0\n"
"MouseMoveEvent 153 168 0 0 0 0 0\n"
"RenderEvent 153 168 0 0 0 0 0\n"
"MouseMoveEvent 153 167 0 0 0 0 0\n"
"RenderEvent 153 167 0 0 0 0 0\n"
"MouseMoveEvent 153 166 0 0 0 0 0\n"
"RenderEvent 153 166 0 0 0 0 0\n"
"MouseMoveEvent 153 165 0 0 0 0 0\n"
"RenderEvent 153 165 0 0 0 0 0\n"
"MouseMoveEvent 153 164 0 0 0 0 0\n"
"RenderEvent 153 164 0 0 0 0 0\n"
"MouseMoveEvent 153 163 0 0 0 0 0\n"
"RenderEvent 153 163 0 0 0 0 0\n"
"MouseMoveEvent 152 163 0 0 0 0 0\n"
"RenderEvent 152 163 0 0 0 0 0\n"
"MouseMoveEvent 151 163 0 0 0 0 0\n"
"RenderEvent 151 163 0 0 0 0 0\n"
"MouseMoveEvent 151 162 0 0 0 0 0\n"
"RenderEvent 151 162 0 0 0 0 0\n"
"MouseMoveEvent 150 162 0 0 0 0 0\n"
"RenderEvent 150 162 0 0 0 0 0\n"
"MouseMoveEvent 150 161 0 0 0 0 0\n"
"RenderEvent 150 161 0 0 0 0 0\n"
"MouseMoveEvent 149 161 0 0 0 0 0\n"
"RenderEvent 149 161 0 0 0 0 0\n"
"MouseMoveEvent 149 160 0 0 0 0 0\n"
"RenderEvent 149 160 0 0 0 0 0\n"
"MouseMoveEvent 148 160 0 0 0 0 0\n"
"RenderEvent 148 160 0 0 0 0 0\n"
"MouseMoveEvent 147 160 0 0 0 0 0\n"
"RenderEvent 147 160 0 0 0 0 0\n"
"MouseMoveEvent 146 160 0 0 0 0 0\n"
"RenderEvent 146 160 0 0 0 0 0\n"
"MouseMoveEvent 145 160 0 0 0 0 0\n"
"RenderEvent 145 160 0 0 0 0 0\n"
"MouseMoveEvent 145 159 0 0 0 0 0\n"
"RenderEvent 145 159 0 0 0 0 0\n"
"MouseMoveEvent 144 159 0 0 0 0 0\n"
"RenderEvent 144 159 0 0 0 0 0\n"
"MouseMoveEvent 143 159 0 0 0 0 0\n"
"RenderEvent 143 159 0 0 0 0 0\n"
"MouseMoveEvent 143 158 0 0 0 0 0\n"
"RenderEvent 143 158 0 0 0 0 0\n"
"MouseMoveEvent 142 157 0 0 0 0 0\n"
"RenderEvent 142 157 0 0 0 0 0\n"
"MouseMoveEvent 142 156 0 0 0 0 0\n"
"RenderEvent 142 156 0 0 0 0 0\n"
"MouseMoveEvent 142 155 0 0 0 0 0\n"
"RenderEvent 142 155 0 0 0 0 0\n"
"MouseMoveEvent 142 154 0 0 0 0 0\n"
"RenderEvent 142 154 0 0 0 0 0\n"
"MouseMoveEvent 142 153 0 0 0 0 0\n"
"RenderEvent 142 153 0 0 0 0 0\n"
"MouseMoveEvent 142 152 0 0 0 0 0\n"
"RenderEvent 142 152 0 0 0 0 0\n"
"MouseMoveEvent 143 150 0 0 0 0 0\n"
"RenderEvent 143 150 0 0 0 0 0\n"
"MouseMoveEvent 143 149 0 0 0 0 0\n"
"RenderEvent 143 149 0 0 0 0 0\n"
"MouseMoveEvent 144 149 0 0 0 0 0\n"
"RenderEvent 144 149 0 0 0 0 0\n"
"MouseMoveEvent 144 148 0 0 0 0 0\n"
"RenderEvent 144 148 0 0 0 0 0\n"
"MouseMoveEvent 145 148 0 0 0 0 0\n"
"RenderEvent 145 148 0 0 0 0 0\n"
"MouseMoveEvent 145 147 0 0 0 0 0\n"
"RenderEvent 145 147 0 0 0 0 0\n"
"MouseMoveEvent 145 146 0 0 0 0 0\n"
"RenderEvent 145 146 0 0 0 0 0\n"
"LeftButtonReleaseEvent 145 146 0 0 0 0 0\n"
"MouseMoveEvent 145 146 0 0 0 0 0\n"

  ;

int TestRectilinearWipeWidget( int argc, char *argv[] )
{
  int wipeMode = 0;
  if (argc > 1)
    {
    wipeMode = atoi(argv[1]);
    }

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

  // Create a wipe pipeline
  vtkSmartPointer<vtkImageCanvasSource2D> image1 =
    vtkSmartPointer<vtkImageCanvasSource2D>::New();
  image1->SetNumberOfScalarComponents(3);
  image1->SetScalarTypeToUnsignedChar();
  image1->SetExtent(0,511,0,511,0,0);
  image1->SetDrawColor(255,255,0);
  image1->FillBox(0,511,0,511);

  vtkSmartPointer<vtkImageWrapPad> pad1 =
    vtkSmartPointer<vtkImageWrapPad>::New();
  pad1->SetInputConnection(image1->GetOutputPort());
  pad1->SetOutputWholeExtent(0,511,0,511,0,0);

  vtkSmartPointer<vtkImageCanvasSource2D> image2 =
    vtkSmartPointer<vtkImageCanvasSource2D>::New();
  image2->SetNumberOfScalarComponents(3);
  image2->SetScalarTypeToUnsignedChar();
  image2->SetExtent(0,511,0,511,0,0);
  image2->SetDrawColor(0,255,255);
  image2->FillBox(0,511,0,511);

  vtkSmartPointer<vtkImageWrapPad> pad2 =
    vtkSmartPointer<vtkImageWrapPad>::New();
  pad2->SetInputConnection(image2->GetOutputPort());
  pad2->SetOutputWholeExtent(0,511,0,511,0,0);

  vtkSmartPointer<vtkImageRectilinearWipe> wipe =
    vtkSmartPointer<vtkImageRectilinearWipe>::New();
  wipe->SetInputConnection(0,pad1->GetOutputPort());
  wipe->SetInputConnection(1,pad2->GetOutputPort());
  wipe->SetPosition(100,256);
  wipe->SetWipe(wipeMode);

  vtkSmartPointer<vtkImageActor> wipeActor =
    vtkSmartPointer<vtkImageActor>::New();
  wipeActor->GetMapper()->SetInputConnection(wipe->GetOutputPort());

  // VTK widgets consist of two parts: the widget part that handles
  // event processing; and the widget representation that defines how
  // the widget appears in the scene
  // (i.e., matters pertaining to geometry).
  vtkSmartPointer<vtkRectilinearWipeWidget> wipeWidget =
    vtkSmartPointer<vtkRectilinearWipeWidget>::New();
  wipeWidget->SetInteractor(iren);

  vtkRectilinearWipeRepresentation *wipeWidgetRep=
    static_cast<vtkRectilinearWipeRepresentation *>(wipeWidget->GetRepresentation());

  wipeWidgetRep->SetImageActor(wipeActor);
  wipeWidgetRep->SetRectilinearWipe(wipe);
  wipeWidgetRep->GetProperty()->SetLineWidth(2.0);
  wipeWidgetRep->GetProperty()->SetOpacity(0.75);

  // Add the actors to the renderer, set the background and size
  //
  ren1->AddActor(wipeActor);
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // render the image
  //
  iren->Initialize();
  renWin->Render();
  wipeWidget->On();

  return vtkTesting::InteractorEventLoop( argc, argv, iren, eventLog );
}

