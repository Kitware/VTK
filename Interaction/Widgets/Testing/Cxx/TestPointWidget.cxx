/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPointWidget.cxx

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
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiBlockPLOT3DReader.h"
#include "vtkPointWidget.h"
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

const char PointWidgetEventLog[] =
  "# StreamVersion 1\n"
  "CharEvent 204 169 0 0 105 1 i\n"
  "KeyReleaseEvent 204 169 0 0 105 1 i\n"
  "KeyPressEvent 204 169 0 0 116 1 t\n"
  "CharEvent 204 169 0 0 116 1 t\n"
  "KeyReleaseEvent 204 169 0 0 116 1 t\n"
  "MouseMoveEvent 204 168 0 0 0 0 t\n"
  "MouseMoveEvent 204 167 0 0 0 0 t\n"
  "MouseMoveEvent 202 165 0 0 0 0 t\n"
  "MouseMoveEvent 196 164 0 0 0 0 t\n"
  "MouseMoveEvent 196 163 0 0 0 0 t\n"
  "MouseMoveEvent 193 162 0 0 0 0 t\n"
  "MouseMoveEvent 192 161 0 0 0 0 t\n"
  "MouseMoveEvent 190 160 0 0 0 0 t\n"
  "MouseMoveEvent 190 159 0 0 0 0 t\n"
  "MouseMoveEvent 189 158 0 0 0 0 t\n"
  "MouseMoveEvent 187 156 0 0 0 0 t\n"
  "MouseMoveEvent 184 154 0 0 0 0 t\n"
  "MouseMoveEvent 178 150 0 0 0 0 t\n"
  "MouseMoveEvent 176 148 0 0 0 0 t\n"
  "MouseMoveEvent 175 147 0 0 0 0 t\n"
  "MouseMoveEvent 175 146 0 0 0 0 t\n"
  "MouseMoveEvent 175 147 0 0 0 0 t\n"
  "MouseMoveEvent 175 146 0 0 0 0 t\n"
  "MouseMoveEvent 176 146 0 0 0 0 t\n"
  "MouseMoveEvent 178 146 0 0 0 0 t\n"
  "MouseMoveEvent 179 147 0 0 0 0 t\n"
  "MouseMoveEvent 179 148 0 0 0 0 t\n"
  "MouseMoveEvent 178 148 0 0 0 0 t\n"
  "MouseMoveEvent 177 148 0 0 0 0 t\n"
  "MouseMoveEvent 177 149 0 0 0 0 t\n"
  "MouseMoveEvent 177 150 0 0 0 0 t\n"
  "MouseMoveEvent 177 151 0 0 0 0 t\n"
  "LeftButtonPressEvent 177 151 0 0 0 0 t\n"
  "MouseMoveEvent 177 152 0 0 0 0 t\n"
  "MouseMoveEvent 177 154 0 0 0 0 t\n"
  "MouseMoveEvent 177 155 0 0 0 0 t\n"
  "MouseMoveEvent 177 156 0 0 0 0 t\n"
  "MouseMoveEvent 177 157 0 0 0 0 t\n"
  "MouseMoveEvent 177 158 0 0 0 0 t\n"
  "MouseMoveEvent 177 159 0 0 0 0 t\n"
  "MouseMoveEvent 177 160 0 0 0 0 t\n"
  "MouseMoveEvent 177 161 0 0 0 0 t\n"
  "MouseMoveEvent 177 162 0 0 0 0 t\n"
  "MouseMoveEvent 176 162 0 0 0 0 t\n"
  "MouseMoveEvent 176 163 0 0 0 0 t\n"
  "MouseMoveEvent 176 164 0 0 0 0 t\n"
  "MouseMoveEvent 176 165 0 0 0 0 t\n"
  "MouseMoveEvent 176 166 0 0 0 0 t\n"
  "MouseMoveEvent 176 167 0 0 0 0 t\n"
  "MouseMoveEvent 176 168 0 0 0 0 t\n"
  "MouseMoveEvent 176 169 0 0 0 0 t\n"
  "MouseMoveEvent 176 170 0 0 0 0 t\n"
  "MouseMoveEvent 176 169 0 0 0 0 t\n"
  "MouseMoveEvent 176 168 0 0 0 0 t\n"
  "MouseMoveEvent 176 166 0 0 0 0 t\n"
  "MouseMoveEvent 176 165 0 0 0 0 t\n"
  "MouseMoveEvent 176 164 0 0 0 0 t\n"
  "MouseMoveEvent 176 163 0 0 0 0 t\n"
  "MouseMoveEvent 176 162 0 0 0 0 t\n"
  "MouseMoveEvent 176 161 0 0 0 0 t\n"
  "MouseMoveEvent 176 160 0 0 0 0 t\n"
  "MouseMoveEvent 176 159 0 0 0 0 t\n"
  "MouseMoveEvent 176 158 0 0 0 0 t\n"
  "MouseMoveEvent 176 157 0 0 0 0 t\n"
  "MouseMoveEvent 176 156 0 0 0 0 t\n"
  "MouseMoveEvent 176 155 0 0 0 0 t\n"
  "MouseMoveEvent 176 154 0 0 0 0 t\n"
  "MouseMoveEvent 176 153 0 0 0 0 t\n"
  "MouseMoveEvent 176 152 0 0 0 0 t\n"
  "MouseMoveEvent 176 151 0 0 0 0 t\n"
  "MouseMoveEvent 176 150 0 0 0 0 t\n"
  "MouseMoveEvent 176 149 0 0 0 0 t\n"
  "MouseMoveEvent 176 148 0 0 0 0 t\n"
  "LeftButtonReleaseEvent 176 148 0 0 0 0 t\n"
  "MouseMoveEvent 176 148 0 0 0 0 t\n"
  "MouseMoveEvent 176 147 0 0 0 0 t\n"
  "MouseMoveEvent 176 146 0 0 0 0 t\n"
  "MouseMoveEvent 176 145 0 0 0 0 t\n"
  "MouseMoveEvent 175 145 0 0 0 0 t\n"
  "MouseMoveEvent 173 145 0 0 0 0 t\n"
  "MouseMoveEvent 168 145 0 0 0 0 t\n"
  "MouseMoveEvent 164 145 0 0 0 0 t\n"
  "MouseMoveEvent 162 145 0 0 0 0 t\n"
  "MouseMoveEvent 161 145 0 0 0 0 t\n"
  "MouseMoveEvent 160 145 0 0 0 0 t\n"
  "MouseMoveEvent 158 145 0 0 0 0 t\n"
  "KeyPressEvent 158 145 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 157 146 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 157 147 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 157 148 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 157 147 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 158 144 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 159 144 0 4 0 0 Shift_L\n"
  "LeftButtonPressEvent 159 144 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 160 144 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 160 145 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 160 146 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 160 147 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 162 148 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 163 148 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 164 149 0 4 0 0 Shift_L\n"
  "KeyPressEvent 164 149 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 166 149 0 4 0 0 Shift_L\n"
  "KeyPressEvent 166 149 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 166 150 0 4 0 0 Shift_L\n"
  "KeyPressEvent 166 150 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 166 149 0 4 0 0 Shift_L\n"
  "KeyPressEvent 166 149 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 167 149 0 4 0 0 Shift_L\n"
  "KeyPressEvent 167 149 0 -128 0 1 Shift_L\n"
  "KeyPressEvent 167 149 0 -128 0 1 Shift_L\n"
  "KeyPressEvent 167 149 0 -128 0 1 Shift_L\n"
  "KeyPressEvent 167 149 0 -128 0 1 Shift_L\n"
  "KeyPressEvent 167 149 0 -128 0 1 Shift_L\n"
  "KeyPressEvent 167 149 0 -128 0 1 Shift_L\n"
  "KeyPressEvent 167 149 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 168 149 0 4 0 0 Shift_L\n"
  "KeyPressEvent 168 149 0 -128 0 1 Shift_L\n"
  "LeftButtonReleaseEvent 168 149 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 168 149 0 4 0 0 Shift_L\n"
  "KeyPressEvent 168 149 0 -128 0 1 Shift_L\n"
  "KeyPressEvent 168 149 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 169 149 0 4 0 0 Shift_L\n"
  "KeyPressEvent 169 149 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 169 150 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 170 150 0 4 0 0 Shift_L\n"
  "KeyPressEvent 170 150 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 170 151 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 171 151 0 4 0 0 Shift_L\n"
  "KeyPressEvent 171 151 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 171 152 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 172 152 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 172 153 0 4 0 0 Shift_L\n"
  "KeyPressEvent 172 153 0 -128 0 1 Shift_L\n"
  "KeyPressEvent 172 153 0 -128 0 1 Shift_L\n"
  "KeyPressEvent 172 153 0 -128 0 1 Shift_L\n"
  "KeyPressEvent 172 153 0 -128 0 1 Shift_L\n"
  "KeyPressEvent 172 153 0 -128 0 1 Shift_L\n"
  "KeyPressEvent 172 153 0 -128 0 1 Shift_L\n"
  "KeyPressEvent 172 153 0 -128 0 1 Shift_L\n"
  "LeftButtonPressEvent 172 153 0 4 0 0 Shift_L\n"
  "KeyPressEvent 172 153 0 -128 0 1 Shift_L\n"
  "KeyPressEvent 172 153 0 -128 0 1 Shift_L\n"
  "KeyPressEvent 172 153 0 -128 0 1 Shift_L\n"
  "KeyPressEvent 172 153 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 173 154 0 4 0 0 Shift_L\n"
  "KeyPressEvent 173 154 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 174 153 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 175 153 0 4 0 0 Shift_L\n"
  "KeyPressEvent 175 153 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 176 153 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 177 153 0 4 0 0 Shift_L\n"
  "KeyPressEvent 177 153 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 180 152 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 181 152 0 4 0 0 Shift_L\n"
  "KeyPressEvent 181 152 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 183 152 0 4 0 0 Shift_L\n"
  "KeyPressEvent 183 152 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 184 152 0 4 0 0 Shift_L\n"
  "KeyPressEvent 184 152 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 185 152 0 4 0 0 Shift_L\n"
  "KeyPressEvent 185 152 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 186 152 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 187 152 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 188 152 0 4 0 0 Shift_L\n"
  "KeyPressEvent 188 152 0 -128 0 1 Shift_L\n"
  "KeyPressEvent 188 152 0 -128 0 1 Shift_L\n"
  "KeyPressEvent 188 152 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 189 152 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 190 152 0 4 0 0 Shift_L\n"
  "KeyPressEvent 190 152 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 191 152 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 192 152 0 4 0 0 Shift_L\n"
  "KeyPressEvent 192 152 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 193 152 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 194 152 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 195 152 0 4 0 0 Shift_L\n"
  "KeyPressEvent 195 152 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 196 152 0 4 0 0 Shift_L\n"
  "KeyPressEvent 196 152 0 -128 0 1 Shift_L\n"
  "KeyPressEvent 196 152 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 197 152 0 4 0 0 Shift_L\n"
  "KeyPressEvent 197 152 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 197 151 0 4 0 0 Shift_L\n"
  "KeyPressEvent 197 151 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 198 151 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 199 151 0 4 0 0 Shift_L\n"
  "KeyPressEvent 199 151 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 200 151 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 201 151 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 203 151 0 4 0 0 Shift_L\n"
  "KeyPressEvent 203 151 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 204 151 0 4 0 0 Shift_L\n"
  "KeyPressEvent 204 151 0 -128 0 1 Shift_L\n"
  "KeyPressEvent 204 151 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 205 151 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 206 151 0 4 0 0 Shift_L\n"
  "KeyPressEvent 206 151 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 207 151 0 4 0 0 Shift_L\n"
  "KeyPressEvent 207 151 0 -128 0 1 Shift_L\n"
  "KeyPressEvent 207 151 0 -128 0 1 Shift_L\n"
  "KeyPressEvent 207 151 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 207 152 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 206 153 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 205 154 0 4 0 0 Shift_L\n"
  "KeyPressEvent 205 154 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 203 154 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 200 154 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 199 154 0 4 0 0 Shift_L\n"
  "KeyPressEvent 199 154 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 197 154 0 4 0 0 Shift_L\n"
  "KeyPressEvent 197 154 0 -128 0 1 Shift_L\n"
  "KeyPressEvent 197 154 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 196 154 0 4 0 0 Shift_L\n"
  "KeyPressEvent 196 154 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 195 154 0 4 0 0 Shift_L\n"
  "KeyPressEvent 195 154 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 194 154 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 193 154 0 4 0 0 Shift_L\n"
  "KeyPressEvent 193 154 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 189 154 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 187 154 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 186 154 0 4 0 0 Shift_L\n"
  "KeyPressEvent 186 154 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 185 153 0 4 0 0 Shift_L\n"
  "KeyPressEvent 185 153 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 185 154 0 4 0 0 Shift_L\n"
  "KeyPressEvent 185 154 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 184 154 0 4 0 0 Shift_L\n"
  "KeyPressEvent 184 154 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 184 155 0 4 0 0 Shift_L\n"
  "KeyPressEvent 184 155 0 -128 0 1 Shift_L\n"
  "KeyPressEvent 184 155 0 -128 0 1 Shift_L\n"
  "KeyPressEvent 184 155 0 -128 0 1 Shift_L\n"
  "KeyPressEvent 184 155 0 -128 0 1 Shift_L\n"
  "KeyPressEvent 184 155 0 -128 0 1 Shift_L\n"
  "KeyPressEvent 184 155 0 -128 0 1 Shift_L\n"
  "KeyPressEvent 184 155 0 -128 0 1 Shift_L\n"
  "MouseMoveEvent 184 154 0 4 0 0 Shift_L\n"
  "LeftButtonReleaseEvent 184 154 0 4 0 0 Shift_L\n"
  "MouseMoveEvent 184 154 0 4 0 0 Shift_L\n"
  "KeyReleaseEvent 184 154 0 0 0 1 Shift_L\n"
  "MouseMoveEvent 185 154 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 185 155 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 186 155 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 187 156 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 188 156 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 189 156 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 190 157 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 191 159 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 191 161 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 192 162 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 193 164 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 193 165 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 193 166 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 194 167 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 194 168 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 195 168 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 168 0 0 0 0 Shift_L\n"
  "LeftButtonPressEvent 196 168 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 169 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 170 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 171 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 172 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 173 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 174 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 175 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 176 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 177 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 178 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 179 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 180 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 181 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 182 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 183 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 184 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 185 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 187 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 188 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 189 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 190 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 191 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 192 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 194 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 195 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 196 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 197 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 198 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 199 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 200 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 201 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 202 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 203 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 204 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 205 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 206 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 208 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 210 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 212 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 213 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 214 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 215 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 216 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 217 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 218 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 219 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 220 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 221 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 222 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 223 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 224 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 223 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 222 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 221 0 0 0 0 Shift_L\n"
  "LeftButtonReleaseEvent 196 221 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 221 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 220 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 219 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 195 217 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 195 216 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 214 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 213 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 195 210 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 191 206 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 190 200 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 189 197 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 189 193 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 191 189 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 193 186 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 197 182 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 198 178 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 198 175 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 198 171 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 195 167 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 191 166 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 191 164 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 193 162 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 194 158 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 195 157 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 195 156 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 195 155 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 195 153 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 195 149 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 195 148 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 195 147 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 195 146 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 195 147 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 195 148 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 149 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 197 150 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 198 150 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 198 151 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 198 150 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 197 150 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 197 151 0 0 0 0 Shift_L\n"
  "MiddleButtonPressEvent 197 151 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 197 150 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 150 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 149 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 148 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 197 148 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 200 149 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 205 151 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 206 151 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 207 150 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 208 150 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 210 150 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 211 150 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 212 150 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 212 149 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 212 148 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 213 148 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 213 149 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 214 149 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 216 149 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 218 149 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 220 149 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 221 149 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 221 150 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 221 151 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 221 152 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 220 153 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 219 155 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 216 157 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 215 158 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 215 159 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 215 160 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 215 161 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 214 161 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 213 161 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 211 161 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 210 162 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 209 162 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 208 162 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 207 163 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 205 164 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 204 164 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 204 165 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 203 164 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 201 164 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 200 165 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 198 165 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 197 165 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 165 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 196 166 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 195 166 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 194 166 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 193 166 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 192 166 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 191 166 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 190 166 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 189 166 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 188 165 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 188 164 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 189 164 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 190 164 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 191 163 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 192 163 0 0 0 0 Shift_L\n"
  "MiddleButtonReleaseEvent 192 163 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 192 163 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 193 163 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 194 163 0 0 0 0 Shift_L\n"
  "RightButtonPressEvent 194 163 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 194 164 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 192 165 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 191 166 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 191 168 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 188 171 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 186 173 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 185 175 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 185 176 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 185 178 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 184 181 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 184 183 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 182 185 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 181 186 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 181 187 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 181 188 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 181 189 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 181 190 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 182 191 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 182 192 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 182 193 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 182 194 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 182 195 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 181 197 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 180 202 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 179 204 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 177 206 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 176 209 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 175 210 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 175 211 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 175 212 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 175 213 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 175 216 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 174 216 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 174 217 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 173 220 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 170 222 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 169 224 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 167 227 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 164 229 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 162 232 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 162 233 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 162 234 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 161 235 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 161 236 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 161 237 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 161 238 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 161 239 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 161 240 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 160 242 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 160 243 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 160 244 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 160 243 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 160 242 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 162 242 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 163 241 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 163 240 0 0 0 0 Shift_L\n"
  "RightButtonReleaseEvent 163 240 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 163 240 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 163 241 0 0 0 0 Shift_L\n"
  "MouseMoveEvent 163 242 0 0 0 0 Shift_L\n"
  ;

// This does the actual work: updates the probe.
// Callback for the interaction
class vtkmyPWCallback : public vtkCommand
{
public:
  static vtkmyPWCallback *New()
  { return new vtkmyPWCallback; }
  void Execute(vtkObject *caller, unsigned long, void*) VTK_OVERRIDE
  {
    vtkPointWidget *pointWidget = reinterpret_cast<vtkPointWidget*>(caller);
    pointWidget->GetPolyData(this->PolyData);
    this->Actor->VisibilityOn();
  }
  vtkmyPWCallback():PolyData(0),Actor(0) {}
  vtkPolyData *PolyData;
  vtkActor *Actor;
};

int TestPointWidget( int argc, char *argv[] )
{
  char* fname =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/combxyz.bin");
  char* fname2 =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/combq.bin");

  // Start by loading some data.
  //
  vtkSmartPointer<vtkMultiBlockPLOT3DReader> pl3d =
    vtkSmartPointer<vtkMultiBlockPLOT3DReader>::New();
  pl3d->SetXYZFileName(fname);
  pl3d->SetQFileName(fname2);
  pl3d->SetScalarFunctionNumber(100);
  pl3d->SetVectorFunctionNumber(202);
  pl3d->Update();
  vtkDataSet* pl3d_block0 = vtkDataSet::SafeDownCast(pl3d->GetOutput()->GetBlock(0));

  delete [] fname;
  delete [] fname2;

  vtkSmartPointer<vtkPolyData> point =
    vtkSmartPointer<vtkPolyData>::New();

  vtkSmartPointer<vtkProbeFilter> probe =
    vtkSmartPointer<vtkProbeFilter>::New();
  probe->SetInputData(point);
  probe->SetSourceData(pl3d_block0);

  // create glyph
  vtkSmartPointer<vtkConeSource> cone =
    vtkSmartPointer<vtkConeSource>::New();
  cone->SetResolution(16);

  vtkSmartPointer<vtkGlyph3D> glyph =
    vtkSmartPointer<vtkGlyph3D>::New();
  glyph->SetInputConnection(probe->GetOutputPort());
  glyph->SetSourceConnection(cone->GetOutputPort());
  glyph->SetVectorModeToUseVector();
  glyph->SetScaleModeToDataScalingOff();
  glyph->SetScaleFactor(pl3d_block0->GetLength() * 0.1);

  vtkSmartPointer<vtkPolyDataMapper> glyphMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  glyphMapper->SetInputConnection(glyph->GetOutputPort());

  vtkSmartPointer<vtkActor> glyphActor =
    vtkSmartPointer<vtkActor>::New();
  glyphActor->SetMapper(glyphMapper);
  glyphActor->VisibilityOff();

  // An outline is shown for context.
  vtkSmartPointer<vtkStructuredGridOutlineFilter> outline =
    vtkSmartPointer<vtkStructuredGridOutlineFilter>::New();
  outline->SetInputData(pl3d_block0);

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
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(ren1);

  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  // The SetInteractor method is how 3D widgets are associated with the render
  // window interactor. Internally, SetInteractor sets up a bunch of callbacks
  // using the Command/Observer mechanism (AddObserver()).
  vtkSmartPointer<vtkmyPWCallback> myCallback =
    vtkSmartPointer<vtkmyPWCallback>::New();
  myCallback->PolyData = point;
  myCallback->Actor = glyphActor;

  // The plane widget is used probe the dataset.
  //
  vtkSmartPointer<vtkPointWidget> pointWidget =
    vtkSmartPointer<vtkPointWidget>::New();
  pointWidget->SetInteractor(iren);
  pointWidget->SetInputData(pl3d_block0);
  pointWidget->AllOff();
  pointWidget->PlaceWidget();
  pointWidget->AddObserver(vtkCommand::InteractionEvent,myCallback);

  ren1->AddActor(outlineActor);
  ren1->AddActor(glyphActor);

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
  recorder->SetInputString(PointWidgetEventLog);

  // render the image
  //
  iren->Initialize();
  renWin->Render();
  recorder->Play();

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();

  iren->Start();

  recorder->Off();

  return EXIT_SUCCESS;
}
