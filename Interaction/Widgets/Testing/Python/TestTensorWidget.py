#!/usr/bin/env python
# -*- coding: utf-8 -*-

'''
=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTensorWidget.py

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================
'''

# Test the vtkTensorWidget and vtkTensorRepresentation classes

import vtk
import vtk.test.Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# These are the pre-recorded events to drive the test
Recording = \
    "# StreamVersion 1.1\n\
    RenderEvent 0 299 0 0 0 0\n\
    EnterEvent 299 51 0 0 0 0\n\
    MouseMoveEvent 299 51 0 0 0 0\n\
    MouseMoveEvent 297 51 0 0 0 0\n\
    MouseMoveEvent 295 51 0 0 0 0\n\
    MouseMoveEvent 294 51 0 0 0 0\n\
    MouseMoveEvent 293 52 0 0 0 0\n\
    MouseMoveEvent 292 52 0 0 0 0\n\
    MouseMoveEvent 291 52 0 0 0 0\n\
    MouseMoveEvent 290 52 0 0 0 0\n\
    MouseMoveEvent 290 52 0 0 0 0\n\
    MouseMoveEvent 289 52 0 0 0 0\n\
    MouseMoveEvent 288 52 0 0 0 0\n\
    MouseMoveEvent 287 52 0 0 0 0\n\
    MouseMoveEvent 285 52 0 0 0 0\n\
    MouseMoveEvent 283 52 0 0 0 0\n\
    MouseMoveEvent 281 52 0 0 0 0\n\
    MouseMoveEvent 280 52 0 0 0 0\n\
    MouseMoveEvent 279 52 0 0 0 0\n\
    KeyPressEvent 279 52 0 116 1 t\n\
    CharEvent 279 52 0 116 1 t\n\
    KeyReleaseEvent 279 52 0 116 1 t\n\
    MouseMoveEvent 278 52 0 0 0 t\n\
    MouseMoveEvent 277 52 0 0 0 t\n\
    MouseMoveEvent 276 52 0 0 0 t\n\
    MouseMoveEvent 275 52 0 0 0 t\n\
    MouseMoveEvent 274 52 0 0 0 t\n\
    MouseMoveEvent 273 52 0 0 0 t\n\
    MouseMoveEvent 273 52 0 0 0 t\n\
    LeftButtonPressEvent 273 52 0 0 0 t\n\
    StartInteractionEvent 273 52 0 0 0 t\n\
    MouseMoveEvent 273 52 0 0 0 t\n\
    RenderEvent 273 52 0 0 0 t\n\
    InteractionEvent 273 52 0 0 0 t\n\
    MouseMoveEvent 274 52 0 0 0 t\n\
    RenderEvent 274 52 0 0 0 t\n\
    InteractionEvent 274 52 0 0 0 t\n\
    MouseMoveEvent 275 52 0 0 0 t\n\
    RenderEvent 275 52 0 0 0 t\n\
    InteractionEvent 275 52 0 0 0 t\n\
    MouseMoveEvent 276 51 0 0 0 t\n\
    RenderEvent 276 51 0 0 0 t\n\
    InteractionEvent 276 51 0 0 0 t\n\
    MouseMoveEvent 277 51 0 0 0 t\n\
    RenderEvent 277 51 0 0 0 t\n\
    InteractionEvent 277 51 0 0 0 t\n\
    MouseMoveEvent 277 51 0 0 0 t\n\
    RenderEvent 277 51 0 0 0 t\n\
    InteractionEvent 277 51 0 0 0 t\n\
    MouseMoveEvent 278 51 0 0 0 t\n\
    RenderEvent 278 51 0 0 0 t\n\
    InteractionEvent 278 51 0 0 0 t\n\
    MouseMoveEvent 279 51 0 0 0 t\n\
    RenderEvent 279 51 0 0 0 t\n\
    InteractionEvent 279 51 0 0 0 t\n\
    MouseMoveEvent 280 51 0 0 0 t\n\
    RenderEvent 280 51 0 0 0 t\n\
    InteractionEvent 280 51 0 0 0 t\n\
    MouseMoveEvent 282 50 0 0 0 t\n\
    RenderEvent 282 50 0 0 0 t\n\
    InteractionEvent 282 50 0 0 0 t\n\
    MouseMoveEvent 283 50 0 0 0 t\n\
    RenderEvent 283 50 0 0 0 t\n\
    InteractionEvent 283 50 0 0 0 t\n\
    MouseMoveEvent 284 49 0 0 0 t\n\
    RenderEvent 284 49 0 0 0 t\n\
    InteractionEvent 284 49 0 0 0 t\n\
    MouseMoveEvent 285 49 0 0 0 t\n\
    RenderEvent 285 49 0 0 0 t\n\
    InteractionEvent 285 49 0 0 0 t\n\
    MouseMoveEvent 288 49 0 0 0 t\n\
    RenderEvent 288 49 0 0 0 t\n\
    InteractionEvent 288 49 0 0 0 t\n\
    MouseMoveEvent 290 49 0 0 0 t\n\
    RenderEvent 290 49 0 0 0 t\n\
    InteractionEvent 290 49 0 0 0 t\n\
    MouseMoveEvent 292 49 0 0 0 t\n\
    RenderEvent 292 49 0 0 0 t\n\
    InteractionEvent 292 49 0 0 0 t\n\
    MouseMoveEvent 293 49 0 0 0 t\n\
    RenderEvent 293 49 0 0 0 t\n\
    InteractionEvent 293 49 0 0 0 t\n\
    MouseMoveEvent 294 49 0 0 0 t\n\
    RenderEvent 294 49 0 0 0 t\n\
    InteractionEvent 294 49 0 0 0 t\n\
    MouseMoveEvent 295 49 0 0 0 t\n\
    RenderEvent 295 49 0 0 0 t\n\
    InteractionEvent 295 49 0 0 0 t\n\
    MouseMoveEvent 297 49 0 0 0 t\n\
    RenderEvent 297 49 0 0 0 t\n\
    InteractionEvent 297 49 0 0 0 t\n\
    MouseMoveEvent 298 49 0 0 0 t\n\
    RenderEvent 298 49 0 0 0 t\n\
    InteractionEvent 298 49 0 0 0 t\n\
    MouseMoveEvent 299 49 0 0 0 t\n\
    RenderEvent 299 49 0 0 0 t\n\
    InteractionEvent 299 49 0 0 0 t\n\
    MouseMoveEvent 300 49 0 0 0 t\n\
    RenderEvent 300 49 0 0 0 t\n\
    InteractionEvent 300 49 0 0 0 t\n\
    LeaveEvent 300 49 0 0 0 t\n\
    MouseMoveEvent 302 49 0 0 0 t\n\
    RenderEvent 302 49 0 0 0 t\n\
    InteractionEvent 302 49 0 0 0 t\n\
    MouseMoveEvent 303 50 0 0 0 t\n\
    RenderEvent 303 50 0 0 0 t\n\
    InteractionEvent 303 50 0 0 0 t\n\
    MouseMoveEvent 305 51 0 0 0 t\n\
    RenderEvent 305 51 0 0 0 t\n\
    InteractionEvent 305 51 0 0 0 t\n\
    MouseMoveEvent 306 51 0 0 0 t\n\
    RenderEvent 306 51 0 0 0 t\n\
    InteractionEvent 306 51 0 0 0 t\n\
    MouseMoveEvent 306 51 0 0 0 t\n\
    RenderEvent 306 51 0 0 0 t\n\
    InteractionEvent 306 51 0 0 0 t\n\
    LeftButtonReleaseEvent 306 51 0 0 0 t\n\
    EndInteractionEvent 306 51 0 0 0 t\n\
    RenderEvent 306 51 0 0 0 t\n\
    LeaveEvent 306 51 0 0 0 t\n\
    EnterEvent 298 52 0 0 0 t\n\
    MouseMoveEvent 298 52 0 0 0 t\n\
    MouseMoveEvent 295 50 0 0 0 t\n\
    MouseMoveEvent 290 49 0 0 0 t\n\
    MouseMoveEvent 285 48 0 0 0 t\n\
    MouseMoveEvent 277 45 0 0 0 t\n\
    MouseMoveEvent 273 45 0 0 0 t\n\
    MouseMoveEvent 270 43 0 0 0 t\n\
    MouseMoveEvent 265 43 0 0 0 t\n\
    MouseMoveEvent 261 41 0 0 0 t\n\
    MouseMoveEvent 259 41 0 0 0 t\n\
    MouseMoveEvent 256 41 0 0 0 t\n\
    MouseMoveEvent 254 41 0 0 0 t\n\
    MouseMoveEvent 251 40 0 0 0 t\n\
    MouseMoveEvent 249 39 0 0 0 t\n\
    MouseMoveEvent 247 39 0 0 0 t\n\
    MouseMoveEvent 245 39 0 0 0 t\n\
    MouseMoveEvent 240 39 0 0 0 t\n\
    MouseMoveEvent 239 39 0 0 0 t\n\
    MouseMoveEvent 238 38 0 0 0 t\n\
    MouseMoveEvent 237 38 0 0 0 t\n\
    MouseMoveEvent 236 37 0 0 0 t\n\
    MouseMoveEvent 235 37 0 0 0 t\n\
    MouseMoveEvent 233 37 0 0 0 t\n\
    MouseMoveEvent 233 37 0 0 0 t\n\
    LeftButtonPressEvent 233 37 0 0 0 t\n\
    StartInteractionEvent 233 37 0 0 0 t\n\
    MouseMoveEvent 233 37 0 0 0 t\n\
    RenderEvent 233 37 0 0 0 t\n\
    InteractionEvent 233 37 0 0 0 t\n\
    MouseMoveEvent 234 37 0 0 0 t\n\
    RenderEvent 234 37 0 0 0 t\n\
    InteractionEvent 234 37 0 0 0 t\n\
    MouseMoveEvent 236 36 0 0 0 t\n\
    RenderEvent 236 36 0 0 0 t\n\
    InteractionEvent 236 36 0 0 0 t\n\
    MouseMoveEvent 238 36 0 0 0 t\n\
    RenderEvent 238 36 0 0 0 t\n\
    InteractionEvent 238 36 0 0 0 t\n\
    MouseMoveEvent 244 33 0 0 0 t\n\
    RenderEvent 244 33 0 0 0 t\n\
    InteractionEvent 244 33 0 0 0 t\n\
    MouseMoveEvent 250 32 0 0 0 t\n\
    RenderEvent 250 32 0 0 0 t\n\
    InteractionEvent 250 32 0 0 0 t\n\
    MouseMoveEvent 261 30 0 0 0 t\n\
    RenderEvent 261 30 0 0 0 t\n\
    InteractionEvent 261 30 0 0 0 t\n\
    MouseMoveEvent 270 27 0 0 0 t\n\
    RenderEvent 270 27 0 0 0 t\n\
    InteractionEvent 270 27 0 0 0 t\n\
    MouseMoveEvent 274 26 0 0 0 t\n\
    RenderEvent 274 26 0 0 0 t\n\
    InteractionEvent 274 26 0 0 0 t\n\
    MouseMoveEvent 277 25 0 0 0 t\n\
    RenderEvent 277 25 0 0 0 t\n\
    InteractionEvent 277 25 0 0 0 t\n\
    MouseMoveEvent 280 24 0 0 0 t\n\
    RenderEvent 280 24 0 0 0 t\n\
    InteractionEvent 280 24 0 0 0 t\n\
    MouseMoveEvent 281 24 0 0 0 t\n\
    RenderEvent 281 24 0 0 0 t\n\
    InteractionEvent 281 24 0 0 0 t\n\
    MouseMoveEvent 282 24 0 0 0 t\n\
    RenderEvent 282 24 0 0 0 t\n\
    InteractionEvent 282 24 0 0 0 t\n\
    MouseMoveEvent 284 23 0 0 0 t\n\
    RenderEvent 284 23 0 0 0 t\n\
    InteractionEvent 284 23 0 0 0 t\n\
    MouseMoveEvent 286 22 0 0 0 t\n\
    RenderEvent 286 22 0 0 0 t\n\
    InteractionEvent 286 22 0 0 0 t\n\
    MouseMoveEvent 287 22 0 0 0 t\n\
    RenderEvent 287 22 0 0 0 t\n\
    InteractionEvent 287 22 0 0 0 t\n\
    MouseMoveEvent 288 22 0 0 0 t\n\
    RenderEvent 288 22 0 0 0 t\n\
    InteractionEvent 288 22 0 0 0 t\n\
    LeftButtonReleaseEvent 288 22 0 0 0 t\n\
    EndInteractionEvent 288 22 0 0 0 t\n\
    RenderEvent 288 22 0 0 0 t\n\
    MouseMoveEvent 287 22 0 0 0 t\n\
    MouseMoveEvent 286 23 0 0 0 t\n\
    MouseMoveEvent 285 23 0 0 0 t\n\
    MouseMoveEvent 283 25 0 0 0 t\n\
    MouseMoveEvent 282 25 0 0 0 t\n\
    MouseMoveEvent 279 29 0 0 0 t\n\
    MouseMoveEvent 276 31 0 0 0 t\n\
    MouseMoveEvent 273 35 0 0 0 t\n\
    MouseMoveEvent 270 38 0 0 0 t\n\
    MouseMoveEvent 266 44 0 0 0 t\n\
    MouseMoveEvent 262 51 0 0 0 t\n\
    MouseMoveEvent 259 57 0 0 0 t\n\
    MouseMoveEvent 256 65 0 0 0 t\n\
    MouseMoveEvent 251 73 0 0 0 t\n\
    MouseMoveEvent 248 79 0 0 0 t\n\
    MouseMoveEvent 244 84 0 0 0 t\n\
    MouseMoveEvent 241 90 0 0 0 t\n\
    MouseMoveEvent 240 94 0 0 0 t\n\
    MouseMoveEvent 237 99 0 0 0 t\n\
    MouseMoveEvent 236 101 0 0 0 t\n\
    MouseMoveEvent 235 103 0 0 0 t\n\
    MouseMoveEvent 234 105 0 0 0 t\n\
    MouseMoveEvent 233 107 0 0 0 t\n\
    MouseMoveEvent 232 107 0 0 0 t\n\
    MouseMoveEvent 232 109 0 0 0 t\n\
    MouseMoveEvent 232 110 0 0 0 t\n\
    MouseMoveEvent 231 111 0 0 0 t\n\
    MouseMoveEvent 231 112 0 0 0 t\n\
    MouseMoveEvent 231 113 0 0 0 t\n\
    MouseMoveEvent 231 115 0 0 0 t\n\
    MouseMoveEvent 231 116 0 0 0 t\n\
    MouseMoveEvent 231 117 0 0 0 t\n\
    MouseMoveEvent 231 119 0 0 0 t\n\
    MouseMoveEvent 231 121 0 0 0 t\n\
    MouseMoveEvent 231 123 0 0 0 t\n\
    MouseMoveEvent 232 125 0 0 0 t\n\
    MouseMoveEvent 233 127 0 0 0 t\n\
    MouseMoveEvent 233 128 0 0 0 t\n\
    MouseMoveEvent 233 130 0 0 0 t\n\
    MouseMoveEvent 234 131 0 0 0 t\n\
    MouseMoveEvent 234 132 0 0 0 t\n\
    MouseMoveEvent 234 134 0 0 0 t\n\
    MouseMoveEvent 235 135 0 0 0 t\n\
    MouseMoveEvent 235 136 0 0 0 t\n\
    MouseMoveEvent 235 137 0 0 0 t\n\
    MouseMoveEvent 235 138 0 0 0 t\n\
    MouseMoveEvent 236 139 0 0 0 t\n\
    MouseMoveEvent 236 140 0 0 0 t\n\
    MouseMoveEvent 235 140 0 0 0 t\n\
    MouseMoveEvent 235 139 0 0 0 t\n\
    MouseMoveEvent 234 139 0 0 0 t\n\
    MouseMoveEvent 233 139 0 0 0 t\n\
    MouseMoveEvent 232 138 0 0 0 t\n\
    MouseMoveEvent 231 137 0 0 0 t\n\
    MouseMoveEvent 230 137 0 0 0 t\n\
    MouseMoveEvent 229 136 0 0 0 t\n\
    LeftButtonPressEvent 229 136 0 0 0 t\n\
    RenderEvent 229 136 0 0 0 t\n\
    MouseMoveEvent 229 135 0 0 0 t\n\
    RenderEvent 229 135 0 0 0 t\n\
    MouseMoveEvent 230 134 0 0 0 t\n\
    RenderEvent 230 134 0 0 0 t\n\
    MouseMoveEvent 231 134 0 0 0 t\n\
    RenderEvent 231 134 0 0 0 t\n\
    MouseMoveEvent 233 133 0 0 0 t\n\
    RenderEvent 233 133 0 0 0 t\n\
    MouseMoveEvent 235 132 0 0 0 t\n\
    RenderEvent 235 132 0 0 0 t\n\
    MouseMoveEvent 242 130 0 0 0 t\n\
    RenderEvent 242 130 0 0 0 t\n\
    MouseMoveEvent 250 127 0 0 0 t\n\
    RenderEvent 250 127 0 0 0 t\n\
    MouseMoveEvent 254 125 0 0 0 t\n\
    RenderEvent 254 125 0 0 0 t\n\
    MouseMoveEvent 256 125 0 0 0 t\n\
    RenderEvent 256 125 0 0 0 t\n\
    MouseMoveEvent 257 125 0 0 0 t\n\
    RenderEvent 257 125 0 0 0 t\n\
    MouseMoveEvent 257 124 0 0 0 t\n\
    RenderEvent 257 124 0 0 0 t\n\
    MouseMoveEvent 259 124 0 0 0 t\n\
    RenderEvent 259 124 0 0 0 t\n\
    MouseMoveEvent 261 124 0 0 0 t\n\
    RenderEvent 261 124 0 0 0 t\n\
    MouseMoveEvent 262 123 0 0 0 t\n\
    RenderEvent 262 123 0 0 0 t\n\
    MouseMoveEvent 263 123 0 0 0 t\n\
    RenderEvent 263 123 0 0 0 t\n\
    MouseMoveEvent 264 122 0 0 0 t\n\
    RenderEvent 264 122 0 0 0 t\n\
    MouseMoveEvent 265 121 0 0 0 t\n\
    RenderEvent 265 121 0 0 0 t\n\
    MouseMoveEvent 266 121 0 0 0 t\n\
    RenderEvent 266 121 0 0 0 t\n\
    MouseMoveEvent 267 121 0 0 0 t\n\
    RenderEvent 267 121 0 0 0 t\n\
    MouseMoveEvent 269 121 0 0 0 t\n\
    RenderEvent 269 121 0 0 0 t\n\
    MouseMoveEvent 270 121 0 0 0 t\n\
    RenderEvent 270 121 0 0 0 t\n\
    MouseMoveEvent 271 120 0 0 0 t\n\
    RenderEvent 271 120 0 0 0 t\n\
    MouseMoveEvent 273 120 0 0 0 t\n\
    RenderEvent 273 120 0 0 0 t\n\
    MouseMoveEvent 275 119 0 0 0 t\n\
    RenderEvent 275 119 0 0 0 t\n\
    MouseMoveEvent 276 118 0 0 0 t\n\
    RenderEvent 276 118 0 0 0 t\n\
    MouseMoveEvent 277 118 0 0 0 t\n\
    RenderEvent 277 118 0 0 0 t\n\
    MouseMoveEvent 278 118 0 0 0 t\n\
    RenderEvent 278 118 0 0 0 t\n\
    MouseMoveEvent 278 118 0 0 0 t\n\
    RenderEvent 278 118 0 0 0 t\n\
    MouseMoveEvent 280 118 0 0 0 t\n\
    RenderEvent 280 118 0 0 0 t\n\
    MouseMoveEvent 280 117 0 0 0 t\n\
    RenderEvent 280 117 0 0 0 t\n\
    MouseMoveEvent 281 117 0 0 0 t\n\
    RenderEvent 281 117 0 0 0 t\n\
    MouseMoveEvent 282 117 0 0 0 t\n\
    RenderEvent 282 117 0 0 0 t\n\
    MouseMoveEvent 283 116 0 0 0 t\n\
    RenderEvent 283 116 0 0 0 t\n\
    MouseMoveEvent 285 116 0 0 0 t\n\
    RenderEvent 285 116 0 0 0 t\n\
    MouseMoveEvent 286 115 0 0 0 t\n\
    RenderEvent 286 115 0 0 0 t\n\
    MouseMoveEvent 287 115 0 0 0 t\n\
    RenderEvent 287 115 0 0 0 t\n\
    MouseMoveEvent 288 115 0 0 0 t\n\
    RenderEvent 288 115 0 0 0 t\n\
    MouseMoveEvent 288 115 0 0 0 t\n\
    RenderEvent 288 115 0 0 0 t\n\
    MouseMoveEvent 290 114 0 0 0 t\n\
    RenderEvent 290 114 0 0 0 t\n\
    MouseMoveEvent 291 114 0 0 0 t\n\
    RenderEvent 291 114 0 0 0 t\n\
    MouseMoveEvent 292 114 0 0 0 t\n\
    RenderEvent 292 114 0 0 0 t\n\
    MouseMoveEvent 292 113 0 0 0 t\n\
    RenderEvent 292 113 0 0 0 t\n\
    MouseMoveEvent 293 112 0 0 0 t\n\
    RenderEvent 293 112 0 0 0 t\n\
    LeftButtonReleaseEvent 293 112 0 0 0 t\n\
    RenderEvent 293 112 0 0 0 t\n\
    MouseMoveEvent 293 113 0 0 0 t\n\
    MouseMoveEvent 291 113 0 0 0 t\n\
    MouseMoveEvent 291 115 0 0 0 t\n\
    MouseMoveEvent 289 115 0 0 0 t\n\
    MouseMoveEvent 288 117 0 0 0 t\n\
    MouseMoveEvent 285 117 0 0 0 t\n\
    MouseMoveEvent 281 119 0 0 0 t\n\
    MouseMoveEvent 276 123 0 0 0 t\n\
    MouseMoveEvent 272 128 0 0 0 t\n\
    MouseMoveEvent 266 132 0 0 0 t\n\
    MouseMoveEvent 258 138 0 0 0 t\n\
    MouseMoveEvent 250 145 0 0 0 t\n\
    MouseMoveEvent 241 152 0 0 0 t\n\
    MouseMoveEvent 235 158 0 0 0 t\n\
    MouseMoveEvent 230 164 0 0 0 t\n\
    MouseMoveEvent 226 170 0 0 0 t\n\
    MouseMoveEvent 221 176 0 0 0 t\n\
    MouseMoveEvent 219 180 0 0 0 t\n\
    MouseMoveEvent 217 185 0 0 0 t\n\
    MouseMoveEvent 215 188 0 0 0 t\n\
    MouseMoveEvent 212 193 0 0 0 t\n\
    MouseMoveEvent 211 195 0 0 0 t\n\
    MouseMoveEvent 208 198 0 0 0 t\n\
    MouseMoveEvent 208 204 0 0 0 t\n\
    MouseMoveEvent 205 206 0 0 0 t\n\
    MouseMoveEvent 203 209 0 0 0 t\n\
    MouseMoveEvent 202 213 0 0 0 t\n\
    MouseMoveEvent 198 218 0 0 0 t\n\
    MouseMoveEvent 197 224 0 0 0 t\n\
    MouseMoveEvent 192 231 0 0 0 t\n\
    MouseMoveEvent 189 239 0 0 0 t\n\
    MouseMoveEvent 186 245 0 0 0 t\n\
    MouseMoveEvent 183 250 0 0 0 t\n\
    MouseMoveEvent 181 256 0 0 0 t\n\
    MouseMoveEvent 178 261 0 0 0 t\n\
    MouseMoveEvent 177 263 0 0 0 t\n\
    MouseMoveEvent 176 267 0 0 0 t\n\
    MouseMoveEvent 175 270 0 0 0 t\n\
    MouseMoveEvent 173 275 0 0 0 t\n\
    MouseMoveEvent 172 280 0 0 0 t\n\
    MouseMoveEvent 171 285 0 0 0 t\n\
    MouseMoveEvent 169 291 0 0 0 t\n\
    MouseMoveEvent 169 296 0 0 0 t\n\
    LeaveEvent 168 300 0 0 0 t\n\
    EnterEvent 168 298 0 0 0 t\n\
    MouseMoveEvent 168 298 0 0 0 t\n\
    MouseMoveEvent 168 296 0 0 0 t\n\
    MouseMoveEvent 168 293 0 0 0 t\n\
    MouseMoveEvent 168 291 0 0 0 t\n\
    MouseMoveEvent 168 289 0 0 0 t\n\
    MouseMoveEvent 168 287 0 0 0 t\n\
    MouseMoveEvent 168 283 0 0 0 t\n\
    MouseMoveEvent 168 282 0 0 0 t\n\
    MouseMoveEvent 169 279 0 0 0 t\n\
    MouseMoveEvent 169 275 0 0 0 t\n\
    MouseMoveEvent 169 273 0 0 0 t\n\
    MouseMoveEvent 169 271 0 0 0 t\n\
    MouseMoveEvent 169 269 0 0 0 t\n\
    MouseMoveEvent 169 267 0 0 0 t\n\
    MouseMoveEvent 170 266 0 0 0 t\n\
    MouseMoveEvent 170 265 0 0 0 t\n\
    MouseMoveEvent 170 264 0 0 0 t\n\
    MouseMoveEvent 170 263 0 0 0 t\n\
    MouseMoveEvent 171 263 0 0 0 t\n\
    MouseMoveEvent 171 262 0 0 0 t\n\
    MouseMoveEvent 172 261 0 0 0 t\n\
    MouseMoveEvent 173 260 0 0 0 t\n\
    MouseMoveEvent 173 259 0 0 0 t\n\
    MouseMoveEvent 174 258 0 0 0 t\n\
    MouseMoveEvent 175 257 0 0 0 t\n\
    MouseMoveEvent 175 256 0 0 0 t\n\
    MouseMoveEvent 175 256 0 0 0 t\n\
    MouseMoveEvent 176 256 0 0 0 t\n\
    KeyPressEvent 176 256 0 114 1 r\n\
    CharEvent 176 256 0 114 1 r\n\
    RenderEvent 176 256 0 114 1 r\n\
    KeyReleaseEvent 176 256 0 114 1 r\n\
    MouseMoveEvent 176 255 0 0 0 r\n\
    MouseMoveEvent 176 254 0 0 0 r\n\
    MouseMoveEvent 175 253 0 0 0 r\n\
    MouseMoveEvent 175 251 0 0 0 r\n\
    MouseMoveEvent 174 249 0 0 0 r\n\
    MouseMoveEvent 174 246 0 0 0 r\n\
    MouseMoveEvent 172 241 0 0 0 r\n\
    MouseMoveEvent 171 239 0 0 0 r\n\
    MouseMoveEvent 171 234 0 0 0 r\n\
    MouseMoveEvent 170 229 0 0 0 r\n\
    MouseMoveEvent 169 220 0 0 0 r\n\
    MouseMoveEvent 167 216 0 0 0 r\n\
    MouseMoveEvent 166 211 0 0 0 r\n\
    MouseMoveEvent 165 207 0 0 0 r\n\
    MouseMoveEvent 163 202 0 0 0 r\n\
    MouseMoveEvent 161 198 0 0 0 r\n\
    MouseMoveEvent 161 196 0 0 0 r\n\
    MouseMoveEvent 159 193 0 0 0 r\n\
    MouseMoveEvent 159 192 0 0 0 r\n\
    MouseMoveEvent 159 189 0 0 0 r\n\
    MouseMoveEvent 158 187 0 0 0 r\n\
    MouseMoveEvent 157 186 0 0 0 r\n\
    MouseMoveEvent 157 183 0 0 0 r\n\
    MouseMoveEvent 155 182 0 0 0 r\n\
    MouseMoveEvent 155 180 0 0 0 r\n\
    MouseMoveEvent 154 178 0 0 0 r\n\
    MouseMoveEvent 153 176 0 0 0 r\n\
    MouseMoveEvent 152 174 0 0 0 r\n\
    MouseMoveEvent 152 172 0 0 0 r\n\
    MouseMoveEvent 151 170 0 0 0 r\n\
    MouseMoveEvent 151 172 0 0 0 r\n\
    MouseMoveEvent 151 173 0 0 0 r\n\
    MouseMoveEvent 151 174 0 0 0 r\n\
    MouseMoveEvent 151 179 0 0 0 r\n\
    MouseMoveEvent 151 180 0 0 0 r\n\
    MouseMoveEvent 152 182 0 0 0 r\n\
    MouseMoveEvent 152 184 0 0 0 r\n\
    MouseMoveEvent 153 186 0 0 0 r\n\
    MouseMoveEvent 153 187 0 0 0 r\n\
    MouseMoveEvent 153 188 0 0 0 r\n\
    MouseMoveEvent 154 189 0 0 0 r\n\
    MouseMoveEvent 154 190 0 0 0 r\n\
    MouseMoveEvent 155 191 0 0 0 r\n\
    MouseMoveEvent 155 192 0 0 0 r\n\
    MouseMoveEvent 155 193 0 0 0 r\n\
    MouseMoveEvent 156 195 0 0 0 r\n\
    MouseMoveEvent 156 197 0 0 0 r\n\
    MouseMoveEvent 157 199 0 0 0 r\n\
    MouseMoveEvent 157 201 0 0 0 r\n\
    MouseMoveEvent 157 203 0 0 0 r\n\
    MouseMoveEvent 157 205 0 0 0 r\n\
    MouseMoveEvent 158 207 0 0 0 r\n\
    MouseMoveEvent 158 209 0 0 0 r\n\
    MouseMoveEvent 158 210 0 0 0 r\n\
    MouseMoveEvent 159 211 0 0 0 r\n\
    MouseMoveEvent 159 212 0 0 0 r\n\
    MouseMoveEvent 159 212 0 0 0 r\n\
    MouseMoveEvent 159 213 0 0 0 r\n\
    MouseMoveEvent 158 213 0 0 0 r\n\
    LeftButtonPressEvent 158 213 0 0 0 r\n\
    RenderEvent 158 213 0 0 0 r\n\
    MouseMoveEvent 158 214 0 0 0 r\n\
    RenderEvent 158 214 0 0 0 r\n\
    MouseMoveEvent 158 214 0 0 0 r\n\
    RenderEvent 158 214 0 0 0 r\n\
    MouseMoveEvent 158 215 0 0 0 r\n\
    RenderEvent 158 215 0 0 0 r\n\
    MouseMoveEvent 158 218 0 0 0 r\n\
    RenderEvent 158 218 0 0 0 r\n\
    MouseMoveEvent 158 220 0 0 0 r\n\
    RenderEvent 158 220 0 0 0 r\n\
    MouseMoveEvent 158 222 0 0 0 r\n\
    RenderEvent 158 222 0 0 0 r\n\
    MouseMoveEvent 157 223 0 0 0 r\n\
    RenderEvent 157 223 0 0 0 r\n\
    MouseMoveEvent 157 224 0 0 0 r\n\
    RenderEvent 157 224 0 0 0 r\n\
    MouseMoveEvent 156 225 0 0 0 r\n\
    RenderEvent 156 225 0 0 0 r\n\
    MouseMoveEvent 156 226 0 0 0 r\n\
    RenderEvent 156 226 0 0 0 r\n\
    MouseMoveEvent 156 227 0 0 0 r\n\
    RenderEvent 156 227 0 0 0 r\n\
    MouseMoveEvent 156 229 0 0 0 r\n\
    RenderEvent 156 229 0 0 0 r\n\
    MouseMoveEvent 156 231 0 0 0 r\n\
    RenderEvent 156 231 0 0 0 r\n\
    MouseMoveEvent 156 233 0 0 0 r\n\
    RenderEvent 156 233 0 0 0 r\n\
    MouseMoveEvent 156 235 0 0 0 r\n\
    RenderEvent 156 235 0 0 0 r\n\
    MouseMoveEvent 156 236 0 0 0 r\n\
    RenderEvent 156 236 0 0 0 r\n\
    MouseMoveEvent 157 236 0 0 0 r\n\
    RenderEvent 157 236 0 0 0 r\n\
    MouseMoveEvent 160 237 0 0 0 r\n\
    RenderEvent 160 237 0 0 0 r\n\
    MouseMoveEvent 164 237 0 0 0 r\n\
    RenderEvent 164 237 0 0 0 r\n\
    MouseMoveEvent 168 238 0 0 0 r\n\
    RenderEvent 168 238 0 0 0 r\n\
    MouseMoveEvent 171 238 0 0 0 r\n\
    RenderEvent 171 238 0 0 0 r\n\
    MouseMoveEvent 172 238 0 0 0 r\n\
    RenderEvent 172 238 0 0 0 r\n\
    MouseMoveEvent 175 238 0 0 0 r\n\
    RenderEvent 175 238 0 0 0 r\n\
    MouseMoveEvent 177 238 0 0 0 r\n\
    RenderEvent 177 238 0 0 0 r\n\
    MouseMoveEvent 179 238 0 0 0 r\n\
    RenderEvent 179 238 0 0 0 r\n\
    MouseMoveEvent 180 237 0 0 0 r\n\
    RenderEvent 180 237 0 0 0 r\n\
    MouseMoveEvent 181 237 0 0 0 r\n\
    RenderEvent 181 237 0 0 0 r\n\
    MouseMoveEvent 183 237 0 0 0 r\n\
    RenderEvent 183 237 0 0 0 r\n\
    MouseMoveEvent 184 237 0 0 0 r\n\
    RenderEvent 184 237 0 0 0 r\n\
    MouseMoveEvent 184 237 0 0 0 r\n\
    RenderEvent 184 237 0 0 0 r\n\
    MouseMoveEvent 186 237 0 0 0 r\n\
    RenderEvent 186 237 0 0 0 r\n\
    MouseMoveEvent 186 237 0 0 0 r\n\
    RenderEvent 186 237 0 0 0 r\n\
    MouseMoveEvent 187 237 0 0 0 r\n\
    RenderEvent 187 237 0 0 0 r\n\
    MouseMoveEvent 188 237 0 0 0 r\n\
    RenderEvent 188 237 0 0 0 r\n\
    MouseMoveEvent 190 237 0 0 0 r\n\
    RenderEvent 190 237 0 0 0 r\n\
    MouseMoveEvent 192 236 0 0 0 r\n\
    RenderEvent 192 236 0 0 0 r\n\
    MouseMoveEvent 193 236 0 0 0 r\n\
    RenderEvent 193 236 0 0 0 r\n\
    MouseMoveEvent 194 236 0 0 0 r\n\
    RenderEvent 194 236 0 0 0 r\n\
    MouseMoveEvent 195 236 0 0 0 r\n\
    RenderEvent 195 236 0 0 0 r\n\
    MouseMoveEvent 196 235 0 0 0 r\n\
    RenderEvent 196 235 0 0 0 r\n\
    MouseMoveEvent 197 234 0 0 0 r\n\
    RenderEvent 197 234 0 0 0 r\n\
    MouseMoveEvent 198 233 0 0 0 r\n\
    RenderEvent 198 233 0 0 0 r\n\
    MouseMoveEvent 200 231 0 0 0 r\n\
    RenderEvent 200 231 0 0 0 r\n\
    MouseMoveEvent 201 230 0 0 0 r\n\
    RenderEvent 201 230 0 0 0 r\n\
    MouseMoveEvent 203 227 0 0 0 r\n\
    RenderEvent 203 227 0 0 0 r\n\
    MouseMoveEvent 203 226 0 0 0 r\n\
    RenderEvent 203 226 0 0 0 r\n\
    MouseMoveEvent 204 225 0 0 0 r\n\
    RenderEvent 204 225 0 0 0 r\n\
    MouseMoveEvent 205 224 0 0 0 r\n\
    RenderEvent 205 224 0 0 0 r\n\
    MouseMoveEvent 205 223 0 0 0 r\n\
    RenderEvent 205 223 0 0 0 r\n\
    MouseMoveEvent 205 221 0 0 0 r\n\
    RenderEvent 205 221 0 0 0 r\n\
    MouseMoveEvent 205 219 0 0 0 r\n\
    RenderEvent 205 219 0 0 0 r\n\
    MouseMoveEvent 205 217 0 0 0 r\n\
    RenderEvent 205 217 0 0 0 r\n\
    MouseMoveEvent 205 215 0 0 0 r\n\
    RenderEvent 205 215 0 0 0 r\n\
    MouseMoveEvent 205 214 0 0 0 r\n\
    RenderEvent 205 214 0 0 0 r\n\
    MouseMoveEvent 205 212 0 0 0 r\n\
    RenderEvent 205 212 0 0 0 r\n\
    MouseMoveEvent 205 210 0 0 0 r\n\
    RenderEvent 205 210 0 0 0 r\n\
    MouseMoveEvent 204 207 0 0 0 r\n\
    RenderEvent 204 207 0 0 0 r\n\
    MouseMoveEvent 204 206 0 0 0 r\n\
    RenderEvent 204 206 0 0 0 r\n\
    MouseMoveEvent 203 205 0 0 0 r\n\
    RenderEvent 203 205 0 0 0 r\n\
    MouseMoveEvent 203 203 0 0 0 r\n\
    RenderEvent 203 203 0 0 0 r\n\
    MouseMoveEvent 202 202 0 0 0 r\n\
    RenderEvent 202 202 0 0 0 r\n\
    MouseMoveEvent 202 199 0 0 0 r\n\
    RenderEvent 202 199 0 0 0 r\n\
    MouseMoveEvent 201 198 0 0 0 r\n\
    RenderEvent 201 198 0 0 0 r\n\
    MouseMoveEvent 201 197 0 0 0 r\n\
    RenderEvent 201 197 0 0 0 r\n\
    MouseMoveEvent 200 195 0 0 0 r\n\
    RenderEvent 200 195 0 0 0 r\n\
    MouseMoveEvent 200 193 0 0 0 r\n\
    RenderEvent 200 193 0 0 0 r\n\
    MouseMoveEvent 200 191 0 0 0 r\n\
    RenderEvent 200 191 0 0 0 r\n\
    MouseMoveEvent 199 189 0 0 0 r\n\
    RenderEvent 199 189 0 0 0 r\n\
    MouseMoveEvent 198 188 0 0 0 r\n\
    RenderEvent 198 188 0 0 0 r\n\
    MouseMoveEvent 198 187 0 0 0 r\n\
    RenderEvent 198 187 0 0 0 r\n\
    LeftButtonReleaseEvent 198 187 0 0 0 r\n\
    RenderEvent 198 187 0 0 0 r\n\
    MouseMoveEvent 197 187 0 0 0 r\n\
    MouseMoveEvent 197 188 0 0 0 r\n\
    MouseMoveEvent 196 188 0 0 0 r\n\
    MouseMoveEvent 194 189 0 0 0 r\n\
    MouseMoveEvent 192 190 0 0 0 r\n\
    MouseMoveEvent 187 191 0 0 0 r\n\
    MouseMoveEvent 183 191 0 0 0 r\n\
    MouseMoveEvent 180 194 0 0 0 r\n\
    MouseMoveEvent 176 195 0 0 0 r\n\
    MouseMoveEvent 171 196 0 0 0 r\n\
    MouseMoveEvent 166 197 0 0 0 r\n\
    MouseMoveEvent 164 198 0 0 0 r\n\
    MouseMoveEvent 162 199 0 0 0 r\n\
    MouseMoveEvent 160 200 0 0 0 r\n\
    MouseMoveEvent 160 201 0 0 0 r\n\
    MouseMoveEvent 158 201 0 0 0 r\n\
    MouseMoveEvent 156 201 0 0 0 r\n\
    MouseMoveEvent 155 202 0 0 0 r\n\
    MouseMoveEvent 154 202 0 0 0 r\n\
    MouseMoveEvent 153 202 0 0 0 r\n\
    MouseMoveEvent 152 202 0 0 0 r\n\
    MouseMoveEvent 151 203 0 0 0 r\n\
    MouseMoveEvent 150 203 0 0 0 r\n\
    MouseMoveEvent 150 203 0 0 0 r\n\
    MouseMoveEvent 148 204 0 0 0 r\n\
    MouseMoveEvent 147 204 0 0 0 r\n\
    MouseMoveEvent 146 205 0 0 0 r\n\
    MouseMoveEvent 145 205 0 0 0 r\n\
    MouseMoveEvent 144 207 0 0 0 r\n\
    LeftButtonPressEvent 144 207 0 0 0 r\n\
    RenderEvent 144 207 0 0 0 r\n\
    MouseMoveEvent 144 208 0 0 0 r\n\
    RenderEvent 144 208 0 0 0 r\n\
    MouseMoveEvent 144 209 0 0 0 r\n\
    RenderEvent 144 209 0 0 0 r\n\
    MouseMoveEvent 144 210 0 0 0 r\n\
    RenderEvent 144 210 0 0 0 r\n\
    MouseMoveEvent 144 211 0 0 0 r\n\
    RenderEvent 144 211 0 0 0 r\n\
    MouseMoveEvent 144 212 0 0 0 r\n\
    RenderEvent 144 212 0 0 0 r\n\
    MouseMoveEvent 144 213 0 0 0 r\n\
    RenderEvent 144 213 0 0 0 r\n\
    MouseMoveEvent 144 214 0 0 0 r\n\
    RenderEvent 144 214 0 0 0 r\n\
    MouseMoveEvent 145 216 0 0 0 r\n\
    RenderEvent 145 216 0 0 0 r\n\
    MouseMoveEvent 145 217 0 0 0 r\n\
    RenderEvent 145 217 0 0 0 r\n\
    MouseMoveEvent 145 219 0 0 0 r\n\
    RenderEvent 145 219 0 0 0 r\n\
    MouseMoveEvent 145 220 0 0 0 r\n\
    RenderEvent 145 220 0 0 0 r\n\
    MouseMoveEvent 145 221 0 0 0 r\n\
    RenderEvent 145 221 0 0 0 r\n\
    MouseMoveEvent 145 223 0 0 0 r\n\
    RenderEvent 145 223 0 0 0 r\n\
    MouseMoveEvent 145 224 0 0 0 r\n\
    RenderEvent 145 224 0 0 0 r\n\
    MouseMoveEvent 145 225 0 0 0 r\n\
    RenderEvent 145 225 0 0 0 r\n\
    MouseMoveEvent 145 225 0 0 0 r\n\
    RenderEvent 145 225 0 0 0 r\n\
    MouseMoveEvent 145 226 0 0 0 r\n\
    RenderEvent 145 226 0 0 0 r\n\
    MouseMoveEvent 145 227 0 0 0 r\n\
    RenderEvent 145 227 0 0 0 r\n\
    MouseMoveEvent 145 228 0 0 0 r\n\
    RenderEvent 145 228 0 0 0 r\n\
    MouseMoveEvent 145 229 0 0 0 r\n\
    RenderEvent 145 229 0 0 0 r\n\
    MouseMoveEvent 145 230 0 0 0 r\n\
    RenderEvent 145 230 0 0 0 r\n\
    MouseMoveEvent 145 231 0 0 0 r\n\
    RenderEvent 145 231 0 0 0 r\n\
    MouseMoveEvent 145 232 0 0 0 r\n\
    RenderEvent 145 232 0 0 0 r\n\
    MouseMoveEvent 145 233 0 0 0 r\n\
    RenderEvent 145 233 0 0 0 r\n\
    MouseMoveEvent 145 235 0 0 0 r\n\
    RenderEvent 145 235 0 0 0 r\n\
    MouseMoveEvent 145 236 0 0 0 r\n\
    RenderEvent 145 236 0 0 0 r\n\
    MouseMoveEvent 144 237 0 0 0 r\n\
    RenderEvent 144 237 0 0 0 r\n\
    MouseMoveEvent 144 237 0 0 0 r\n\
    RenderEvent 144 237 0 0 0 r\n\
    MouseMoveEvent 144 238 0 0 0 r\n\
    RenderEvent 144 238 0 0 0 r\n\
    MouseMoveEvent 144 239 0 0 0 r\n\
    RenderEvent 144 239 0 0 0 r\n\
    MouseMoveEvent 144 239 0 0 0 r\n\
    RenderEvent 144 239 0 0 0 r\n\
    MouseMoveEvent 144 240 0 0 0 r\n\
    RenderEvent 144 240 0 0 0 r\n\
    MouseMoveEvent 144 241 0 0 0 r\n\
    RenderEvent 144 241 0 0 0 r\n\
    MouseMoveEvent 144 242 0 0 0 r\n\
    RenderEvent 144 242 0 0 0 r\n\
    MouseMoveEvent 144 243 0 0 0 r\n\
    RenderEvent 144 243 0 0 0 r\n\
    LeftButtonReleaseEvent 144 243 0 0 0 r\n\
    RenderEvent 144 243 0 0 0 r\n\
    "

# Create the RenderWindow, Renderer and both Actors
#
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)

iRen = vtk.vtkRenderWindowInteractor()
iRen.SetRenderWindow(renWin);

# Define callback for the widget
def SelectPolygons(widget, event_string):
    '''
    The callback takes two parameters.
    Parameters:
      widget - the object that generates the event.
      event_string - the event name (which is a string).
    '''
    tsWidget

# Create a representation for the widget
bbox = [-1,1,-1,1,-1,1]
rep = vtk.vtkTensorRepresentation()
rep.GetEllipsoidProperty().SetColor(0,0,0)
rep.GetEllipsoidProperty().SetRepresentationToWireframe()
rep.GetOutlineProperty().SetColor(0,0,0)
rep.SetPlaceFactor(1)
rep.PlaceWidget(bbox)

# The widget proper
tsWidget = vtk.vtkTensorWidget()
tsWidget.SetInteractor(iRen)
tsWidget.SetRepresentation(rep)
tsWidget.AddObserver("EndInteractionEvent", SelectPolygons)
tsWidget.On()

# Handle playback of events
recorder = vtk.vtkInteractorEventRecorder()
recorder.SetInteractor(iRen)
#recorder.SetFileName("record.log")
#recorder.On()
#recorder.Record()
recorder.ReadFromInputStringOn()
recorder.SetInputString(Recording)

# Add the actors to the renderer, set the background and size
#
ren.SetBackground(1,1,1)
renWin.SetSize(300, 300)

# render and interact with data
ren.ResetCamera()
renWin.Render()

# Playack events
recorder.Play()

iRen.Start()
