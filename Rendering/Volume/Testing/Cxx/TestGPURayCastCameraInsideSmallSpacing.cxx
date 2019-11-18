/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastCameraInsideSmallSpacing.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Description
// This is a test for clipping of volume using the camera near plane when the
// camera is inside the volume. The test renders the ironProt dataset after
// changing it to have a very small spacing and dollies the camera inside the
// volume geometry.

#include "vtkColorTransferFunction.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkImageChangeInformation.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkNew.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStructuredPoints.h"
#include "vtkStructuredPointsReader.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

static const char* TestGPURayCastCameraInsideSmallSpacingLog =
  "# StreamVersion 1\n"
  "EnterEvent 188 3 0 0 0 0 0\n"
  "MouseMoveEvent 188 3 0 0 0 0 0\n"
  "MouseMoveEvent 164 67 0 0 0 0 0\n"
  "MouseWheelForwardEvent 164 67 0 0 0 0 0\n"

  "MouseWheelForwardEvent 164 67 0 0 0 1 0\n"

  "MouseWheelForwardEvent 164 67 0 0 0 0 0\n"

  "MouseWheelForwardEvent 164 67 0 0 0 1 0\n"

  "MouseWheelForwardEvent 164 67 0 0 0 0 0\n"

  "MouseWheelForwardEvent 164 67 0 0 0 1 0\n"

  "MouseWheelForwardEvent 164 67 0 0 0 0 0\n"

  "MouseWheelForwardEvent 164 67 0 0 0 1 0\n"

  "MouseWheelForwardEvent 164 67 0 0 0 0 0\n"

  "MouseWheelForwardEvent 164 67 0 0 0 0 0\n"

  "MouseWheelForwardEvent 164 67 0 0 0 1 0\n"

  "MouseWheelForwardEvent 164 67 0 0 0 0 0\n"

  "MouseWheelForwardEvent 164 67 0 0 0 1 0\n"

  "MouseWheelForwardEvent 164 67 0 0 0 0 0\n"

  "MouseWheelForwardEvent 164 67 0 0 0 1 0\n"

  "MouseWheelForwardEvent 164 67 0 0 0 0 0\n"

  "MouseWheelForwardEvent 164 67 0 0 0 0 0\n"

  "MouseWheelForwardEvent 164 67 0 0 0 1 0\n"

  "MouseWheelForwardEvent 164 67 0 0 0 0 0\n"

  "MouseWheelForwardEvent 164 67 0 0 0 1 0\n"

  "MouseWheelForwardEvent 164 67 0 0 0 0 0\n"

  "MouseWheelForwardEvent 164 67 0 0 0 1 0\n"

  "MouseWheelForwardEvent 164 67 0 0 0 0 0\n"

  "MouseWheelForwardEvent 164 67 0 0 0 1 0\n"

  "MouseWheelForwardEvent 164 67 0 0 0 0 0\n"

  "MouseWheelForwardEvent 164 67 0 0 0 1 0\n"

  "MouseWheelForwardEvent 164 67 0 0 0 0 0\n"

  "MouseWheelForwardEvent 164 67 0 0 0 1 0\n"

  "MouseWheelForwardEvent 164 67 0 0 0 0 0\n"

  "MouseWheelForwardEvent 164 67 0 0 0 1 0\n"

  "MouseWheelForwardEvent 164 67 0 0 0 0 0\n"

  "MouseWheelForwardEvent 164 67 0 0 0 1 0\n"

  "MouseWheelForwardEvent 164 67 0 0 0 0 0\n"

  "MouseWheelForwardEvent 164 67 0 0 0 1 0\n"

  "MouseWheelForwardEvent 164 67 0 0 0 0 0\n"

  "MouseWheelForwardEvent 164 67 0 0 0 1 0\n"

  "MouseWheelForwardEvent 164 67 0 0 0 0 0\n"

  "MouseWheelForwardEvent 164 67 0 0 0 1 0\n"

  "MouseWheelForwardEvent 164 67 0 0 0 0 0\n"

  "MouseWheelForwardEvent 164 67 0 0 0 1 0\n"

  "MouseWheelForwardEvent 164 67 0 0 0 0 0\n"

  "MouseWheelForwardEvent 164 67 0 0 0 1 0\n"

  "MouseWheelForwardEvent 164 67 0 0 0 0 0\n"

  "MouseWheelForwardEvent 164 67 0 0 0 1 0\n"

  "MouseWheelForwardEvent 164 67 0 0 0 0 0\n"

  "MouseWheelBackwardEvent 164 67 0 0 0 1 0\n"

  "MouseWheelBackwardEvent 164 67 0 0 0 0 0\n"

  "MouseWheelBackwardEvent 164 67 0 0 0 1 0\n"

  "MouseWheelBackwardEvent 164 67 0 0 0 0 0\n"

  "MouseWheelBackwardEvent 164 67 0 0 0 1 0\n"

  "MouseWheelBackwardEvent 164 67 0 0 0 0 0\n"

  "MouseWheelBackwardEvent 164 67 0 0 0 1 0\n"

  "MouseWheelBackwardEvent 164 67 0 0 0 0 0\n"

  "MouseWheelBackwardEvent 164 67 0 0 0 1 0\n"

  "MouseWheelBackwardEvent 164 67 0 0 0 0 0\n"

  "MouseWheelBackwardEvent 164 67 0 0 0 1 0\n"

  "MouseWheelBackwardEvent 164 67 0 0 0 0 0\n"

  "MouseWheelBackwardEvent 164 67 0 0 0 1 0\n"

  "MouseWheelBackwardEvent 164 67 0 0 0 0 0\n"

  "MouseWheelBackwardEvent 164 67 0 0 0 1 0\n"

  "MouseWheelBackwardEvent 164 67 0 0 0 0 0\n"

  "MouseWheelBackwardEvent 164 67 0 0 0 1 0\n"

  "MouseMoveEvent 163 66 0 0 0 0 0\n"
  "MouseWheelBackwardEvent 163 66 0 0 0 0 0\n"
  "StartInteractionEvent 163 66 0 0 0 0 0\n"

  "EndInteractionEvent 163 66 0 0 0 0 0\n"

  "MouseWheelBackwardEvent 163 66 0 0 0 1 0\n"
  "StartInteractionEvent 163 66 0 0 0 1 0\n"
  "RenderEvent 163 66 0 0 0 1 0\n"
  "EndInteractionEvent 163 66 0 0 0 1 0\n"
  "RenderEvent 163 66 0 0 0 1 0\n"
  "MouseWheelBackwardEvent 163 66 0 0 0 0 0\n"
  "StartInteractionEvent 163 66 0 0 0 0 0\n"
  "RenderEvent 163 66 0 0 0 0 0\n"
  "EndInteractionEvent 163 66 0 0 0 0 0\n"

  "MouseWheelBackwardEvent 163 66 0 0 0 1 0\n"
  "StartInteractionEvent 163 66 0 0 0 1 0\n"
  "RenderEvent 163 66 0 0 0 1 0\n"
  "EndInteractionEvent 163 66 0 0 0 1 0\n"
  "RenderEvent 163 66 0 0 0 1 0\n"
  "MouseWheelBackwardEvent 163 66 0 0 0 0 0\n"
  "StartInteractionEvent 163 66 0 0 0 0 0\n"

  "EndInteractionEvent 163 66 0 0 0 0 0\n"

  "MouseWheelBackwardEvent 163 66 0 0 0 1 0\n"
  "StartInteractionEvent 163 66 0 0 0 1 0\n"
  "RenderEvent 163 66 0 0 0 1 0\n"
  "EndInteractionEvent 163 66 0 0 0 1 0\n"
  "RenderEvent 163 66 0 0 0 1 0\n"
  "MouseWheelBackwardEvent 163 66 0 0 0 0 0\n"
  "StartInteractionEvent 163 66 0 0 0 0 0\n"
  "RenderEvent 163 66 0 0 0 0 0\n"
  "EndInteractionEvent 163 66 0 0 0 0 0\n"

  "MouseWheelBackwardEvent 163 66 0 0 0 1 0\n"
  "StartInteractionEvent 163 66 0 0 0 1 0\n"
  "RenderEvent 163 66 0 0 0 1 0\n"
  "EndInteractionEvent 163 66 0 0 0 1 0\n"
  "RenderEvent 163 66 0 0 0 1 0\n"
  "MouseWheelBackwardEvent 163 66 0 0 0 0 0\n"
  "StartInteractionEvent 163 66 0 0 0 0 0\n"

  "EndInteractionEvent 163 66 0 0 0 0 0\n"

  "MouseWheelBackwardEvent 163 66 0 0 0 1 0\n"
  "StartInteractionEvent 163 66 0 0 0 1 0\n"
  "RenderEvent 163 66 0 0 0 1 0\n"
  "EndInteractionEvent 163 66 0 0 0 1 0\n"
  "RenderEvent 163 66 0 0 0 1 0\n"
  "MouseWheelBackwardEvent 163 66 0 0 0 0 0\n"
  "StartInteractionEvent 163 66 0 0 0 0 0\n"
  "RenderEvent 163 66 0 0 0 0 0\n"
  "EndInteractionEvent 163 66 0 0 0 0 0\n"

  "MouseWheelBackwardEvent 163 66 0 0 0 1 0\n"
  "StartInteractionEvent 163 66 0 0 0 1 0\n"
  "RenderEvent 163 66 0 0 0 1 0\n"
  "EndInteractionEvent 163 66 0 0 0 1 0\n"
  "RenderEvent 163 66 0 0 0 1 0\n"
  "MouseWheelBackwardEvent 163 66 0 0 0 0 0\n"
  "StartInteractionEvent 163 66 0 0 0 0 0\n"

  "EndInteractionEvent 163 66 0 0 0 0 0\n"

  "MouseWheelBackwardEvent 163 66 0 0 0 1 0\n"
  "StartInteractionEvent 163 66 0 0 0 1 0\n"
  "RenderEvent 163 66 0 0 0 1 0\n"
  "EndInteractionEvent 163 66 0 0 0 1 0\n"
  "RenderEvent 163 66 0 0 0 1 0\n"
  "MouseWheelBackwardEvent 163 66 0 0 0 0 0\n"
  "StartInteractionEvent 163 66 0 0 0 0 0\n"

  "EndInteractionEvent 163 66 0 0 0 0 0\n"
  "RenderEvent 163 66 0 0 0 0 0\n"
  "MouseWheelBackwardEvent 163 66 0 0 0 1 0\n"
  "StartInteractionEvent 163 66 0 0 0 1 0\n"
  "RenderEvent 163 66 0 0 0 1 0\n"
  "EndInteractionEvent 163 66 0 0 0 1 0\n"
  "RenderEvent 163 66 0 0 0 1 0\n"
  "MouseWheelBackwardEvent 163 66 0 0 0 0 0\n"
  "StartInteractionEvent 163 66 0 0 0 0 0\n"

  "EndInteractionEvent 163 66 0 0 0 0 0\n"

  "MouseWheelBackwardEvent 163 66 0 0 0 1 0\n"
  "StartInteractionEvent 163 66 0 0 0 1 0\n"
  "RenderEvent 163 66 0 0 0 1 0\n"
  "EndInteractionEvent 163 66 0 0 0 1 0\n"
  "RenderEvent 163 66 0 0 0 1 0\n"
  "MouseWheelBackwardEvent 163 66 0 0 0 0 0\n"
  "StartInteractionEvent 163 66 0 0 0 0 0\n"

  "EndInteractionEvent 163 66 0 0 0 0 0\n"

  "MouseWheelBackwardEvent 163 66 0 0 0 0 0\n"
  "StartInteractionEvent 163 66 0 0 0 0 0\n"
  "RenderEvent 163 66 0 0 0 0 0\n"
  "EndInteractionEvent 163 66 0 0 0 0 0\n"

  "MouseWheelBackwardEvent 163 66 0 0 0 1 0\n"
  "StartInteractionEvent 163 66 0 0 0 1 0\n"
  "RenderEvent 163 66 0 0 0 1 0\n"
  "EndInteractionEvent 163 66 0 0 0 1 0\n"
  "RenderEvent 163 66 0 0 0 1 0\n"
  "MouseWheelBackwardEvent 163 66 0 0 0 0 0\n"
  "StartInteractionEvent 163 66 0 0 0 0 0\n"

  "EndInteractionEvent 163 66 0 0 0 0 0\n"

  "MouseWheelBackwardEvent 163 66 0 0 0 1 0\n"
  "StartInteractionEvent 163 66 0 0 0 1 0\n"
  "RenderEvent 163 66 0 0 0 1 0\n"
  "EndInteractionEvent 163 66 0 0 0 1 0\n"
  "RenderEvent 163 66 0 0 0 1 0\n"
  "MouseWheelBackwardEvent 163 66 0 0 0 0 0\n"
  "StartInteractionEvent 163 66 0 0 0 0 0\n"

  "EndInteractionEvent 163 66 0 0 0 0 0\n"

  "MouseMoveEvent 162 67 0 0 0 0 0\n"
  "MouseMoveEvent 179 83 0 0 0 0 0\n"
  "LeftButtonPressEvent 179 83 0 0 0 0 0\n"
  "StartInteractionEvent 179 83 0 0 0 0 0\n"
  "MouseMoveEvent 178 82 0 0 0 0 0\n"
  "RenderEvent 178 82 0 0 0 0 0\n"
  "InteractionEvent 178 82 0 0 0 0 0\n"
  "MouseMoveEvent 177 81 0 0 0 0 0\n"

  "InteractionEvent 177 81 0 0 0 0 0\n"
  "MouseMoveEvent 175 80 0 0 0 0 0\n"

  "InteractionEvent 175 80 0 0 0 0 0\n"
  "MouseMoveEvent 173 78 0 0 0 0 0\n"

  "InteractionEvent 173 78 0 0 0 0 0\n"
  "MouseMoveEvent 169 74 0 0 0 0 0\n"

  "InteractionEvent 169 74 0 0 0 0 0\n"
  "MouseMoveEvent 165 70 0 0 0 0 0\n"

  "InteractionEvent 165 70 0 0 0 0 0\n"
  "MouseMoveEvent 159 66 0 0 0 0 0\n"
  "RenderEvent 159 66 0 0 0 0 0\n"
  "InteractionEvent 159 66 0 0 0 0 0\n"
  "MouseMoveEvent 155 63 0 0 0 0 0\n"

  "InteractionEvent 155 63 0 0 0 0 0\n"
  "MouseMoveEvent 153 61 0 0 0 0 0\n"

  "InteractionEvent 153 61 0 0 0 0 0\n"
  "MouseMoveEvent 152 60 0 0 0 0 0\n"

  "InteractionEvent 152 60 0 0 0 0 0\n"
  "MouseMoveEvent 151 59 0 0 0 0 0\n"

  "InteractionEvent 151 59 0 0 0 0 0\n"
  "MouseMoveEvent 150 58 0 0 0 0 0\n"

  "InteractionEvent 150 58 0 0 0 0 0\n"
  "MouseMoveEvent 149 58 0 0 0 0 0\n"
  "RenderEvent 149 58 0 0 0 0 0\n"
  "InteractionEvent 149 58 0 0 0 0 0\n"
  "MouseMoveEvent 147 57 0 0 0 0 0\n"

  "InteractionEvent 147 57 0 0 0 0 0\n"
  "MouseMoveEvent 145 57 0 0 0 0 0\n"

  "InteractionEvent 145 57 0 0 0 0 0\n"
  "MouseMoveEvent 141 57 0 0 0 0 0\n"

  "InteractionEvent 141 57 0 0 0 0 0\n"
  "MouseMoveEvent 136 57 0 0 0 0 0\n"

  "InteractionEvent 136 57 0 0 0 0 0\n"
  "MouseMoveEvent 133 57 0 0 0 0 0\n"

  "InteractionEvent 133 57 0 0 0 0 0\n"
  "MouseMoveEvent 130 57 0 0 0 0 0\n"
  "RenderEvent 130 57 0 0 0 0 0\n"
  "InteractionEvent 130 57 0 0 0 0 0\n"
  "MouseMoveEvent 125 56 0 0 0 0 0\n"

  "InteractionEvent 125 56 0 0 0 0 0\n"
  "MouseMoveEvent 121 56 0 0 0 0 0\n"

  "InteractionEvent 121 56 0 0 0 0 0\n"
  "MouseMoveEvent 119 56 0 0 0 0 0\n"

  "InteractionEvent 119 56 0 0 0 0 0\n"
  "MouseMoveEvent 116 55 0 0 0 0 0\n"

  "InteractionEvent 116 55 0 0 0 0 0\n"
  "MouseMoveEvent 114 54 0 0 0 0 0\n"

  "InteractionEvent 114 54 0 0 0 0 0\n"
  "MouseMoveEvent 113 54 0 0 0 0 0\n"
  "RenderEvent 113 54 0 0 0 0 0\n"
  "InteractionEvent 113 54 0 0 0 0 0\n"
  "MouseMoveEvent 112 54 0 0 0 0 0\n"

  "InteractionEvent 112 54 0 0 0 0 0\n"
  "MouseMoveEvent 108 53 0 0 0 0 0\n"

  "InteractionEvent 108 53 0 0 0 0 0\n"
  "MouseMoveEvent 103 53 0 0 0 0 0\n"

  "InteractionEvent 103 53 0 0 0 0 0\n"
  "MouseMoveEvent 99 52 0 0 0 0 0\n"

  "InteractionEvent 99 52 0 0 0 0 0\n"
  "MouseMoveEvent 96 52 0 0 0 0 0\n"

  "InteractionEvent 96 52 0 0 0 0 0\n"
  "MouseMoveEvent 95 52 0 0 0 0 0\n"
  "RenderEvent 95 52 0 0 0 0 0\n"
  "InteractionEvent 95 52 0 0 0 0 0\n"
  "LeftButtonReleaseEvent 95 52 0 0 0 0 0\n"
  "EndInteractionEvent 95 52 0 0 0 0 0\n"

  "MouseMoveEvent 94 52 0 0 0 0 0\n"
  "MouseMoveEvent 126 70 0 0 0 0 0\n"
  "MouseWheelForwardEvent 126 70 0 0 0 0 0\n"
  "StartInteractionEvent 126 70 0 0 0 0 0\n"

  "EndInteractionEvent 126 70 0 0 0 0 0\n"

  "MouseWheelForwardEvent 126 70 0 0 0 1 0\n"
  "StartInteractionEvent 126 70 0 0 0 1 0\n"
  "RenderEvent 126 70 0 0 0 1 0\n"
  "EndInteractionEvent 126 70 0 0 0 1 0\n"
  "RenderEvent 126 70 0 0 0 1 0\n"
  "MouseWheelForwardEvent 126 70 0 0 0 0 0\n"
  "StartInteractionEvent 126 70 0 0 0 0 0\n"

  "EndInteractionEvent 126 70 0 0 0 0 0\n"

  "MouseWheelForwardEvent 126 70 0 0 0 1 0\n"
  "StartInteractionEvent 126 70 0 0 0 1 0\n"
  "RenderEvent 126 70 0 0 0 1 0\n"
  "EndInteractionEvent 126 70 0 0 0 1 0\n"
  "RenderEvent 126 70 0 0 0 1 0\n"
  "MouseMoveEvent 128 70 0 0 0 0 0\n"
  "MouseMoveEvent 195 182 0 0 0 0 0\n"
  "MiddleButtonPressEvent 195 182 0 0 0 0 0\n"
  "StartInteractionEvent 195 182 0 0 0 0 0\n"
  "MouseMoveEvent 195 181 0 0 0 0 0\n"
  "RenderEvent 195 181 0 0 0 0 0\n"
  "InteractionEvent 195 181 0 0 0 0 0\n"
  "MouseMoveEvent 194 180 0 0 0 0 0\n"

  "InteractionEvent 194 180 0 0 0 0 0\n"
  "MouseMoveEvent 193 180 0 0 0 0 0\n"

  "InteractionEvent 193 180 0 0 0 0 0\n"
  "MouseMoveEvent 192 180 0 0 0 0 0\n"

  "InteractionEvent 192 180 0 0 0 0 0\n"
  "MouseMoveEvent 191 180 0 0 0 0 0\n"

  "InteractionEvent 191 180 0 0 0 0 0\n"
  "MouseMoveEvent 189 181 0 0 0 0 0\n"

  "InteractionEvent 189 181 0 0 0 0 0\n"
  "MouseMoveEvent 187 183 0 0 0 0 0\n"
  "RenderEvent 187 183 0 0 0 0 0\n"
  "InteractionEvent 187 183 0 0 0 0 0\n"
  "MouseMoveEvent 182 189 0 0 0 0 0\n"

  "InteractionEvent 182 189 0 0 0 0 0\n"
  "MouseMoveEvent 179 195 0 0 0 0 0\n"

  "InteractionEvent 179 195 0 0 0 0 0\n"
  "MouseMoveEvent 176 201 0 0 0 0 0\n"

  "InteractionEvent 176 201 0 0 0 0 0\n"
  "MouseMoveEvent 175 205 0 0 0 0 0\n"

  "InteractionEvent 175 205 0 0 0 0 0\n"
  "MouseMoveEvent 173 209 0 0 0 0 0\n"

  "InteractionEvent 173 209 0 0 0 0 0\n"
  "MouseMoveEvent 172 211 0 0 0 0 0\n"
  "RenderEvent 172 211 0 0 0 0 0\n"
  "InteractionEvent 172 211 0 0 0 0 0\n"
  "MouseMoveEvent 171 213 0 0 0 0 0\n"

  "InteractionEvent 171 213 0 0 0 0 0\n"
  "MouseMoveEvent 170 216 0 0 0 0 0\n"

  "InteractionEvent 170 216 0 0 0 0 0\n"
  "MouseMoveEvent 170 217 0 0 0 0 0\n"

  "InteractionEvent 170 217 0 0 0 0 0\n"
  "MouseMoveEvent 169 218 0 0 0 0 0\n"

  "InteractionEvent 169 218 0 0 0 0 0\n"
  "MouseMoveEvent 168 221 0 0 0 0 0\n"

  "InteractionEvent 168 221 0 0 0 0 0\n"
  "MouseMoveEvent 168 222 0 0 0 0 0\n"
  "RenderEvent 168 222 0 0 0 0 0\n"
  "InteractionEvent 168 222 0 0 0 0 0\n"
  "MouseMoveEvent 167 224 0 0 0 0 0\n"

  "InteractionEvent 167 224 0 0 0 0 0\n"
  "MouseMoveEvent 167 225 0 0 0 0 0\n"

  "InteractionEvent 167 225 0 0 0 0 0\n"
  "MouseMoveEvent 167 227 0 0 0 0 0\n"

  "InteractionEvent 167 227 0 0 0 0 0\n"
  "MouseMoveEvent 167 230 0 0 0 0 0\n"

  "InteractionEvent 167 230 0 0 0 0 0\n"
  "MouseMoveEvent 167 232 0 0 0 0 0\n"

  "InteractionEvent 167 232 0 0 0 0 0\n"
  "MouseMoveEvent 166 236 0 0 0 0 0\n"

  "InteractionEvent 166 236 0 0 0 0 0\n"
  "MouseMoveEvent 166 239 0 0 0 0 0\n"
  "RenderEvent 166 239 0 0 0 0 0\n"
  "InteractionEvent 166 239 0 0 0 0 0\n"
  "MouseMoveEvent 166 243 0 0 0 0 0\n"

  "InteractionEvent 166 243 0 0 0 0 0\n"
  "MouseMoveEvent 166 245 0 0 0 0 0\n"

  "InteractionEvent 166 245 0 0 0 0 0\n"
  "MouseMoveEvent 166 250 0 0 0 0 0\n"

  "InteractionEvent 166 250 0 0 0 0 0\n"
  "MouseMoveEvent 166 253 0 0 0 0 0\n"

  "InteractionEvent 166 253 0 0 0 0 0\n"
  "MouseMoveEvent 166 255 0 0 0 0 0\n"

  "InteractionEvent 166 255 0 0 0 0 0\n"
  "MouseMoveEvent 166 259 0 0 0 0 0\n"

  "InteractionEvent 166 259 0 0 0 0 0\n"
  "MouseMoveEvent 166 261 0 0 0 0 0\n"
  "RenderEvent 166 261 0 0 0 0 0\n"
  "InteractionEvent 166 261 0 0 0 0 0\n"
  "MouseMoveEvent 166 262 0 0 0 0 0\n"

  "InteractionEvent 166 262 0 0 0 0 0\n"
  "MouseMoveEvent 166 263 0 0 0 0 0\n"

  "InteractionEvent 166 263 0 0 0 0 0\n"
  "MouseMoveEvent 166 266 0 0 0 0 0\n"

  "InteractionEvent 166 266 0 0 0 0 0\n"
  "MouseMoveEvent 166 267 0 0 0 0 0\n"

  "InteractionEvent 166 267 0 0 0 0 0\n"
  "MouseMoveEvent 166 268 0 0 0 0 0\n"

  "InteractionEvent 166 268 0 0 0 0 0\n"
  "MouseMoveEvent 167 269 0 0 0 0 0\n"

  "InteractionEvent 167 269 0 0 0 0 0\n"
  "MiddleButtonReleaseEvent 167 269 0 0 0 0 0\n"
  "EndInteractionEvent 167 269 0 0 0 0 0\n"
  "RenderEvent 167 269 0 0 0 0 0\n"
  "MouseMoveEvent 167 267 0 0 0 0 0\n"
  "MouseMoveEvent 180 125 0 0 0 0 0\n"
  "MouseWheelForwardEvent 180 125 0 0 0 0 0\n"
  "StartInteractionEvent 180 125 0 0 0 0 0\n"

  "EndInteractionEvent 180 125 0 0 0 0 0\n"

  "MouseWheelForwardEvent 180 125 0 0 0 1 0\n"
  "StartInteractionEvent 180 125 0 0 0 1 0\n"
  "RenderEvent 180 125 0 0 0 1 0\n"
  "EndInteractionEvent 180 125 0 0 0 1 0\n"
  "RenderEvent 180 125 0 0 0 1 0\n"
  "MouseWheelForwardEvent 180 125 0 0 0 0 0\n"
  "StartInteractionEvent 180 125 0 0 0 0 0\n"

  "EndInteractionEvent 180 125 0 0 0 0 0\n"

  "MouseWheelForwardEvent 180 125 0 0 0 1 0\n"
  "StartInteractionEvent 180 125 0 0 0 1 0\n"
  "RenderEvent 180 125 0 0 0 1 0\n"
  "EndInteractionEvent 180 125 0 0 0 1 0\n"
  "RenderEvent 180 125 0 0 0 1 0\n"
  "MouseWheelForwardEvent 180 125 0 0 0 0 0\n"
  "StartInteractionEvent 180 125 0 0 0 0 0\n"

  "EndInteractionEvent 180 125 0 0 0 0 0\n"

  "MouseWheelForwardEvent 180 125 0 0 0 0 0\n"
  "StartInteractionEvent 180 125 0 0 0 0 0\n"
  "RenderEvent 180 125 0 0 0 0 0\n"
  "EndInteractionEvent 180 125 0 0 0 0 0\n"

  "MouseWheelForwardEvent 180 125 0 0 0 1 0\n"
  "StartInteractionEvent 180 125 0 0 0 1 0\n"
  "RenderEvent 180 125 0 0 0 1 0\n"
  "EndInteractionEvent 180 125 0 0 0 1 0\n"
  "RenderEvent 180 125 0 0 0 1 0\n"
  "MouseWheelForwardEvent 180 125 0 0 0 0 0\n"
  "StartInteractionEvent 180 125 0 0 0 0 0\n"

  "EndInteractionEvent 180 125 0 0 0 0 0\n"

  "MouseMoveEvent 181 124 0 0 0 0 0\n"
  "MouseMoveEvent 277 129 0 0 0 0 0\n"
  "LeftButtonPressEvent 277 129 0 0 0 0 0\n"
  "StartInteractionEvent 277 129 0 0 0 0 0\n"
  "MouseMoveEvent 276 130 0 0 0 0 0\n"

  "InteractionEvent 276 130 0 0 0 0 0\n"
  "MouseMoveEvent 275 130 0 0 0 0 0\n"

  "InteractionEvent 275 130 0 0 0 0 0\n"
  "MouseMoveEvent 273 130 0 0 0 0 0\n"

  "InteractionEvent 273 130 0 0 0 0 0\n"
  "MouseMoveEvent 272 130 0 0 0 0 0\n"
  "RenderEvent 272 130 0 0 0 0 0\n"
  "InteractionEvent 272 130 0 0 0 0 0\n"
  "MouseMoveEvent 271 130 0 0 0 0 0\n"

  "InteractionEvent 271 130 0 0 0 0 0\n"
  "MouseMoveEvent 270 130 0 0 0 0 0\n"

  "InteractionEvent 270 130 0 0 0 0 0\n"
  "MouseMoveEvent 269 130 0 0 0 0 0\n"

  "InteractionEvent 269 130 0 0 0 0 0\n"
  "MouseMoveEvent 267 130 0 0 0 0 0\n"

  "InteractionEvent 267 130 0 0 0 0 0\n"
  "MouseMoveEvent 266 130 0 0 0 0 0\n"

  "InteractionEvent 266 130 0 0 0 0 0\n"
  "MouseMoveEvent 265 130 0 0 0 0 0\n"

  "InteractionEvent 265 130 0 0 0 0 0\n"
  "MouseMoveEvent 264 130 0 0 0 0 0\n"
  "RenderEvent 264 130 0 0 0 0 0\n"
  "InteractionEvent 264 130 0 0 0 0 0\n"
  "MouseMoveEvent 263 130 0 0 0 0 0\n"

  "InteractionEvent 263 130 0 0 0 0 0\n"
  "MouseMoveEvent 261 130 0 0 0 0 0\n"

  "InteractionEvent 261 130 0 0 0 0 0\n"
  "MouseMoveEvent 260 130 0 0 0 0 0\n"

  "InteractionEvent 260 130 0 0 0 0 0\n"
  "MouseMoveEvent 259 130 0 0 0 0 0\n"

  "InteractionEvent 259 130 0 0 0 0 0\n"
  "MouseMoveEvent 258 130 0 0 0 0 0\n"

  "InteractionEvent 258 130 0 0 0 0 0\n"
  "MouseMoveEvent 256 130 0 0 0 0 0\n"

  "InteractionEvent 256 130 0 0 0 0 0\n"
  "MouseMoveEvent 255 130 0 0 0 0 0\n"
  "RenderEvent 255 130 0 0 0 0 0\n"
  "InteractionEvent 255 130 0 0 0 0 0\n"
  "MouseMoveEvent 254 130 0 0 0 0 0\n"

  "InteractionEvent 254 130 0 0 0 0 0\n"
  "MouseMoveEvent 251 130 0 0 0 0 0\n"

  "InteractionEvent 251 130 0 0 0 0 0\n"
  "MouseMoveEvent 248 130 0 0 0 0 0\n"

  "InteractionEvent 248 130 0 0 0 0 0\n"
  "MouseMoveEvent 245 130 0 0 0 0 0\n"

  "InteractionEvent 245 130 0 0 0 0 0\n"
  "MouseMoveEvent 244 130 0 0 0 0 0\n"

  "InteractionEvent 244 130 0 0 0 0 0\n"
  "MouseMoveEvent 245 130 0 0 0 0 0\n"

  "InteractionEvent 245 130 0 0 0 0 0\n"
  "MouseMoveEvent 247 130 0 0 0 0 0\n"
  "RenderEvent 247 130 0 0 0 0 0\n"
  "InteractionEvent 247 130 0 0 0 0 0\n"
  "MouseMoveEvent 248 130 0 0 0 0 0\n"

  "InteractionEvent 248 130 0 0 0 0 0\n"
  "MouseMoveEvent 250 130 0 0 0 0 0\n"

  "InteractionEvent 250 130 0 0 0 0 0\n"
  "MouseMoveEvent 253 130 0 0 0 0 0\n"

  "InteractionEvent 253 130 0 0 0 0 0\n"
  "MouseMoveEvent 257 130 0 0 0 0 0\n"

  "InteractionEvent 257 130 0 0 0 0 0\n"
  "MouseMoveEvent 258 130 0 0 0 0 0\n"

  "InteractionEvent 258 130 0 0 0 0 0\n"
  "MouseMoveEvent 260 130 0 0 0 0 0\n"

  "InteractionEvent 260 130 0 0 0 0 0\n"
  "MouseMoveEvent 261 130 0 0 0 0 0\n"
  "RenderEvent 261 130 0 0 0 0 0\n"
  "InteractionEvent 261 130 0 0 0 0 0\n"
  "MouseMoveEvent 262 130 0 0 0 0 0\n"

  "InteractionEvent 262 130 0 0 0 0 0\n"
  "MouseMoveEvent 263 130 0 0 0 0 0\n"

  "InteractionEvent 263 130 0 0 0 0 0\n"
  "MouseMoveEvent 266 130 0 0 0 0 0\n"

  "InteractionEvent 266 130 0 0 0 0 0\n"
  "MouseMoveEvent 269 130 0 0 0 0 0\n"

  "InteractionEvent 269 130 0 0 0 0 0\n"
  "MouseMoveEvent 271 130 0 0 0 0 0\n"

  "InteractionEvent 271 130 0 0 0 0 0\n"
  "MouseMoveEvent 275 130 0 0 0 0 0\n"

  "InteractionEvent 275 130 0 0 0 0 0\n"
  "MouseMoveEvent 276 130 0 0 0 0 0\n"
  "RenderEvent 276 130 0 0 0 0 0\n"
  "InteractionEvent 276 130 0 0 0 0 0\n"
  "MouseMoveEvent 277 130 0 0 0 0 0\n"

  "InteractionEvent 277 130 0 0 0 0 0\n"
  "MouseMoveEvent 278 130 0 0 0 0 0\n"

  "InteractionEvent 278 130 0 0 0 0 0\n"
  "MouseMoveEvent 279 130 0 0 0 0 0\n"

  "InteractionEvent 279 130 0 0 0 0 0\n"
  "MouseMoveEvent 280 130 0 0 0 0 0\n"

  "InteractionEvent 280 130 0 0 0 0 0\n"
  "MouseMoveEvent 282 130 0 0 0 0 0\n"

  "InteractionEvent 282 130 0 0 0 0 0\n"
  "MouseMoveEvent 283 130 0 0 0 0 0\n"

  "InteractionEvent 283 130 0 0 0 0 0\n"
  "MouseMoveEvent 284 130 0 0 0 0 0\n"
  "RenderEvent 284 130 0 0 0 0 0\n"
  "InteractionEvent 284 130 0 0 0 0 0\n"
  "MouseMoveEvent 285 130 0 0 0 0 0\n"

  "InteractionEvent 285 130 0 0 0 0 0\n"
  "MouseMoveEvent 286 130 0 0 0 0 0\n"

  "InteractionEvent 286 130 0 0 0 0 0\n"
  "MouseMoveEvent 287 130 0 0 0 0 0\n"

  "InteractionEvent 287 130 0 0 0 0 0\n"
  "MouseMoveEvent 289 130 0 0 0 0 0\n"

  "InteractionEvent 289 130 0 0 0 0 0\n"
  "MouseMoveEvent 291 130 0 0 0 0 0\n"

  "InteractionEvent 291 130 0 0 0 0 0\n"
  "MouseMoveEvent 293 130 0 0 0 0 0\n"

  "InteractionEvent 293 130 0 0 0 0 0\n"
  "MouseMoveEvent 296 130 0 0 0 0 0\n"
  "RenderEvent 296 130 0 0 0 0 0\n"
  "InteractionEvent 296 130 0 0 0 0 0\n"
  "MouseMoveEvent 298 130 0 0 0 0 0\n"

  "InteractionEvent 298 130 0 0 0 0 0\n"
  "MouseMoveEvent 300 130 0 0 0 0 0\n"

  "InteractionEvent 300 130 0 0 0 0 0\n"
  "LeaveEvent 301 130 0 0 0 0 0\n"
  "MouseMoveEvent 301 130 0 0 0 0 0\n"

  "InteractionEvent 301 130 0 0 0 0 0\n"
  "MouseMoveEvent 303 130 0 0 0 0 0\n"

  "InteractionEvent 303 130 0 0 0 0 0\n"
  "MouseMoveEvent 304 130 0 0 0 0 0\n"
  "RenderEvent 304 130 0 0 0 0 0\n"
  "InteractionEvent 304 130 0 0 0 0 0\n"
  "LeftButtonReleaseEvent 304 130 0 0 0 0 0\n"
  "EndInteractionEvent 304 130 0 0 0 0 0\n"

  "LeaveEvent 304 130 0 0 0 0 0\n"
  "EnterEvent 300 124 0 0 0 0 0\n"
  "MouseMoveEvent 300 124 0 0 0 0 0\n"
  "MouseMoveEvent 297 121 0 0 0 0 0\n"
  "MouseMoveEvent 224 89 0 0 0 0 0\n"
  "MouseMoveEvent 224 90 0 0 0 0 0\n"
  "MouseWheelForwardEvent 224 90 0 0 0 0 0\n"
  "StartInteractionEvent 224 90 0 0 0 0 0\n"

  "EndInteractionEvent 224 90 0 0 0 0 0\n"

  "MouseWheelBackwardEvent 224 90 0 0 0 0 0\n"
  "StartInteractionEvent 224 90 0 0 0 0 0\n"

  "EndInteractionEvent 224 90 0 0 0 0 0\n"

  "MouseWheelBackwardEvent 224 90 0 0 0 1 0\n"
  "StartInteractionEvent 224 90 0 0 0 1 0\n"
  "RenderEvent 224 90 0 0 0 1 0\n"
  "EndInteractionEvent 224 90 0 0 0 1 0\n"
  "RenderEvent 224 90 0 0 0 1 0\n"
  "MouseWheelBackwardEvent 224 90 0 0 0 0 0\n"
  "StartInteractionEvent 224 90 0 0 0 0 0\n"

  "EndInteractionEvent 224 90 0 0 0 0 0\n"
  "RenderEvent 224 90 0 0 0 0 0\n"
  "MouseWheelBackwardEvent 224 90 0 0 0 0 0\n"
  "StartInteractionEvent 224 90 0 0 0 0 0\n"

  "EndInteractionEvent 224 90 0 0 0 0 0\n"

  "MouseWheelBackwardEvent 224 90 0 0 0 1 0\n"
  "StartInteractionEvent 224 90 0 0 0 1 0\n"
  "RenderEvent 224 90 0 0 0 1 0\n"
  "EndInteractionEvent 224 90 0 0 0 1 0\n"
  "RenderEvent 224 90 0 0 0 1 0\n"
  "MouseWheelBackwardEvent 224 90 0 0 0 0 0\n"
  "StartInteractionEvent 224 90 0 0 0 0 0\n"
  "RenderEvent 224 90 0 0 0 0 0\n"
  "EndInteractionEvent 224 90 0 0 0 0 0\n"

  "MouseMoveEvent 223 89 0 0 0 0 0\n"
  "MouseMoveEvent 222 89 0 0 0 0 0\n"
  "MouseMoveEvent 273 212 0 0 0 0 0\n"
  "MouseMoveEvent 284 206 0 0 0 0 0\n"
  "MouseMoveEvent 296 198 0 0 0 0 0\n"
  "LeaveEvent 304 189 0 0 0 0 0\n"
  "EnterEvent 296 131 0 0 0 0 0\n"
  "MouseMoveEvent 296 131 0 0 0 0 0\n"
  "MouseMoveEvent 291 133 0 0 0 0 0\n"
  "MouseMoveEvent 128 224 0 0 0 0 0\n"
  "MouseMoveEvent 127 224 0 0 0 0 0\n"
  "MouseMoveEvent 126 224 0 0 0 0 0\n"
  "LeftButtonPressEvent 126 224 0 0 0 0 0\n"
  "StartInteractionEvent 126 224 0 0 0 0 0\n"
  "MouseMoveEvent 127 223 0 0 0 0 0\n"
  "RenderEvent 127 223 0 0 0 0 0\n"
  "InteractionEvent 127 223 0 0 0 0 0\n"
  "MouseMoveEvent 128 222 0 0 0 0 0\n"

  "InteractionEvent 128 222 0 0 0 0 0\n"
  "MouseMoveEvent 129 221 0 0 0 0 0\n"

  "InteractionEvent 129 221 0 0 0 0 0\n"
  "MouseMoveEvent 130 220 0 0 0 0 0\n"

  "InteractionEvent 130 220 0 0 0 0 0\n"
  "MouseMoveEvent 131 218 0 0 0 0 0\n"

  "InteractionEvent 131 218 0 0 0 0 0\n"
  "MouseMoveEvent 133 215 0 0 0 0 0\n"

  "InteractionEvent 133 215 0 0 0 0 0\n"
  "MouseMoveEvent 135 212 0 0 0 0 0\n"

  "InteractionEvent 135 212 0 0 0 0 0\n"
  "MouseMoveEvent 138 208 0 0 0 0 0\n"

  "InteractionEvent 138 208 0 0 0 0 0\n"
  "MouseMoveEvent 139 206 0 0 0 0 0\n"

  "InteractionEvent 139 206 0 0 0 0 0\n"
  "MouseMoveEvent 140 204 0 0 0 0 0\n"
  "RenderEvent 140 204 0 0 0 0 0\n"
  "InteractionEvent 140 204 0 0 0 0 0\n"
  "MouseMoveEvent 141 202 0 0 0 0 0\n"

  "InteractionEvent 141 202 0 0 0 0 0\n"
  "MouseMoveEvent 141 201 0 0 0 0 0\n"

  "InteractionEvent 141 201 0 0 0 0 0\n"
  "MouseMoveEvent 141 200 0 0 0 0 0\n"

  "InteractionEvent 141 200 0 0 0 0 0\n"
  "MouseMoveEvent 141 199 0 0 0 0 0\n"

  "InteractionEvent 141 199 0 0 0 0 0\n"
  "MouseMoveEvent 141 196 0 0 0 0 0\n"

  "InteractionEvent 141 196 0 0 0 0 0\n"
  "MouseMoveEvent 142 194 0 0 0 0 0\n"

  "InteractionEvent 142 194 0 0 0 0 0\n"
  "MouseMoveEvent 142 193 0 0 0 0 0\n"

  "InteractionEvent 142 193 0 0 0 0 0\n"
  "MouseMoveEvent 142 192 0 0 0 0 0\n"

  "InteractionEvent 142 192 0 0 0 0 0\n"
  "MouseMoveEvent 142 191 0 0 0 0 0\n"
  "RenderEvent 142 191 0 0 0 0 0\n"
  "InteractionEvent 142 191 0 0 0 0 0\n"
  "MouseMoveEvent 142 190 0 0 0 0 0\n"

  "InteractionEvent 142 190 0 0 0 0 0\n"
  "MouseMoveEvent 142 189 0 0 0 0 0\n"

  "InteractionEvent 142 189 0 0 0 0 0\n"
  "MouseMoveEvent 142 188 0 0 0 0 0\n"

  "InteractionEvent 142 188 0 0 0 0 0\n"
  "MouseMoveEvent 142 187 0 0 0 0 0\n"

  "InteractionEvent 142 187 0 0 0 0 0\n"
  "MouseMoveEvent 143 186 0 0 0 0 0\n"

  "InteractionEvent 143 186 0 0 0 0 0\n"
  "MouseMoveEvent 144 185 0 0 0 0 0\n"

  "InteractionEvent 144 185 0 0 0 0 0\n"
  "MouseMoveEvent 143 184 0 0 0 0 0\n"

  "InteractionEvent 143 184 0 0 0 0 0\n"
  "LeftButtonReleaseEvent 143 184 0 0 0 0 0\n"
  "EndInteractionEvent 143 184 0 0 0 0 0\n"

  "MouseMoveEvent 145 182 0 0 0 0 0\n"
  "MouseMoveEvent 147 179 0 0 0 0 0\n"
  "MouseMoveEvent 175 131 0 0 0 0 0\n"
  "MouseMoveEvent 175 129 0 0 0 0 0\n"
  "LeftButtonPressEvent 175 129 0 0 0 0 0\n"
  "StartInteractionEvent 175 129 0 0 0 0 0\n"
  "MouseMoveEvent 174 131 0 0 0 0 0\n"

  "InteractionEvent 174 131 0 0 0 0 0\n"
  "MouseMoveEvent 173 133 0 0 0 0 0\n"
  "RenderEvent 173 133 0 0 0 0 0\n"
  "InteractionEvent 173 133 0 0 0 0 0\n"
  "MouseMoveEvent 173 134 0 0 0 0 0\n"

  "InteractionEvent 173 134 0 0 0 0 0\n"
  "MouseMoveEvent 170 140 0 0 0 0 0\n"

  "InteractionEvent 170 140 0 0 0 0 0\n"
  "MouseMoveEvent 170 142 0 0 0 0 0\n"

  "InteractionEvent 170 142 0 0 0 0 0\n"
  "MouseMoveEvent 169 144 0 0 0 0 0\n"

  "InteractionEvent 169 144 0 0 0 0 0\n"
  "MouseMoveEvent 169 145 0 0 0 0 0\n"

  "InteractionEvent 169 145 0 0 0 0 0\n"
  "MouseMoveEvent 169 146 0 0 0 0 0\n"
  "RenderEvent 169 146 0 0 0 0 0\n"
  "InteractionEvent 169 146 0 0 0 0 0\n"
  "LeftButtonReleaseEvent 169 146 0 0 0 0 0\n"
  "EndInteractionEvent 169 146 0 0 0 0 0\n"
  "RenderEvent 169 146 0 0 0 0 0\n"
  "MouseMoveEvent 170 145 0 0 0 0 0\n"
  "MouseMoveEvent 171 144 0 0 0 0 0\n"
  "MouseMoveEvent 270 285 0 0 0 0 0\n"
  "MouseMoveEvent 270 287 0 0 0 0 0\n"
  "MouseMoveEvent 271 288 0 0 0 0 0\n"
  "MouseMoveEvent 272 290 0 0 0 0 0\n"
  "MouseMoveEvent 273 291 0 0 0 0 0\n"
  "MouseMoveEvent 274 293 0 0 0 0 0\n"
  "MouseMoveEvent 275 297 0 0 0 0 0\n"
  "MouseMoveEvent 276 298 0 0 0 0 0\n"
  "LeaveEvent 276 300 0 0 0 0 0\n";

int TestGPURayCastCameraInsideSmallSpacing(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  char* volumeFile = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/ironProt.vtk");
  vtkNew<vtkStructuredPointsReader> reader;
  reader->SetFileName(volumeFile);
  delete[] volumeFile;

  int dims[3];
  reader->Update();
  reader->GetOutput()->GetDimensions(dims);

  double desiredBounds = 0.0005;
  double desiredSpacing[3];
  for (int i = 0; i < 3; ++i)
  {
    desiredSpacing[i] = desiredBounds / static_cast<double>(dims[i]);
  }

  vtkNew<vtkImageChangeInformation> imageChangeInfo;
  imageChangeInfo->SetInputConnection(reader->GetOutputPort());
  imageChangeInfo->SetOutputSpacing(desiredSpacing);

  vtkNew<vtkGPUVolumeRayCastMapper> mapper;
  mapper->SetInputConnection(imageChangeInfo->GetOutputPort());
  mapper->SetAutoAdjustSampleDistances(0);
  mapper->SetSampleDistance(7e-6);

  vtkNew<vtkColorTransferFunction> color;
  color->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
  color->AddRGBPoint(64.0, 1.0, 0.0, 0.0);
  color->AddRGBPoint(128.0, 0.0, 0.0, 1.0);
  color->AddRGBPoint(192.0, 0.0, 1.0, 0.0);
  color->AddRGBPoint(255.0, 0.0, 0.2, 0.0);

  vtkNew<vtkPiecewiseFunction> opacity;
  opacity->AddPoint(0.0, 0.0);
  opacity->AddPoint(255.0, 1.0);

  vtkNew<vtkVolumeProperty> property;
  property->SetColor(color);
  property->SetScalarOpacity(opacity);
  property->SetInterpolationTypeToLinear();
  property->ShadeOff();
  property->SetScalarOpacityUnitDistance(7e-6);

  vtkNew<vtkVolume> volume;
  volume->SetMapper(mapper);
  volume->SetProperty(property);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(301, 300);
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style);

  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren);

  ren->AddVolume(volume);
  ren->ResetCamera();

  renWin->Render();
  iren->Initialize();

  return vtkTesting::InteractorEventLoop(
    argc, argv, iren, TestGPURayCastCameraInsideSmallSpacingLog);
}
