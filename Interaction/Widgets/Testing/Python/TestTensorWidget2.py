#!/usr/bin/env python
# -*- coding: utf-8 -*-

'''
=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTensorWidget2.py

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
    ExposeEvent 0 299 0 0 0 0\n\
    RenderEvent 0 299 0 0 0 0\n\
    EnterEvent 185 1 0 0 0 0\n\
    MouseMoveEvent 185 1 0 0 0 0\n\
    MouseMoveEvent 180 10 0 0 0 0\n\
    MouseMoveEvent 177 14 0 0 0 0\n\
    MouseMoveEvent 173 19 0 0 0 0\n\
    MouseMoveEvent 170 25 0 0 0 0\n\
    MouseMoveEvent 169 29 0 0 0 0\n\
    MouseMoveEvent 166 34 0 0 0 0\n\
    MouseMoveEvent 165 36 0 0 0 0\n\
    MouseMoveEvent 165 39 0 0 0 0\n\
    MouseMoveEvent 164 42 0 0 0 0\n\
    MouseMoveEvent 164 45 0 0 0 0\n\
    MouseMoveEvent 164 48 0 0 0 0\n\
    MouseMoveEvent 164 51 0 0 0 0\n\
    MouseMoveEvent 164 54 0 0 0 0\n\
    MouseMoveEvent 164 56 0 0 0 0\n\
    MouseMoveEvent 164 60 0 0 0 0\n\
    MouseMoveEvent 164 63 0 0 0 0\n\
    MouseMoveEvent 164 66 0 0 0 0\n\
    MouseMoveEvent 165 71 0 0 0 0\n\
    MouseMoveEvent 165 75 0 0 0 0\n\
    MouseMoveEvent 165 78 0 0 0 0\n\
    MouseMoveEvent 165 81 0 0 0 0\n\
    MouseMoveEvent 164 86 0 0 0 0\n\
    MouseMoveEvent 164 90 0 0 0 0\n\
    MouseMoveEvent 163 95 0 0 0 0\n\
    MouseMoveEvent 163 100 0 0 0 0\n\
    MouseMoveEvent 161 109 0 0 0 0\n\
    MouseMoveEvent 160 117 0 0 0 0\n\
    MouseMoveEvent 158 126 0 0 0 0\n\
    MouseMoveEvent 156 136 0 0 0 0\n\
    MouseMoveEvent 156 148 0 0 0 0\n\
    MouseMoveEvent 156 158 0 0 0 0\n\
    MouseMoveEvent 156 173 0 0 0 0\n\
    MouseMoveEvent 156 182 0 0 0 0\n\
    MouseMoveEvent 155 189 0 0 0 0\n\
    MouseMoveEvent 155 196 0 0 0 0\n\
    MouseMoveEvent 155 204 0 0 0 0\n\
    MouseMoveEvent 155 209 0 0 0 0\n\
    MouseMoveEvent 154 214 0 0 0 0\n\
    MouseMoveEvent 152 218 0 0 0 0\n\
    MouseMoveEvent 152 221 0 0 0 0\n\
    MouseMoveEvent 151 226 0 0 0 0\n\
    MouseMoveEvent 151 229 0 0 0 0\n\
    MouseMoveEvent 151 231 0 0 0 0\n\
    MouseMoveEvent 150 232 0 0 0 0\n\
    MouseMoveEvent 149 235 0 0 0 0\n\
    MouseMoveEvent 149 236 0 0 0 0\n\
    MouseMoveEvent 149 238 0 0 0 0\n\
    MouseMoveEvent 148 239 0 0 0 0\n\
    MouseMoveEvent 148 240 0 0 0 0\n\
    MouseMoveEvent 148 241 0 0 0 0\n\
    MouseMoveEvent 147 243 0 0 0 0\n\
    MouseMoveEvent 146 244 0 0 0 0\n\
    MouseMoveEvent 146 245 0 0 0 0\n\
    MouseMoveEvent 145 247 0 0 0 0\n\
    MouseMoveEvent 144 249 0 0 0 0\n\
    MouseMoveEvent 144 251 0 0 0 0\n\
    MouseMoveEvent 144 250 0 0 0 0\n\
    MouseMoveEvent 144 249 0 0 0 0\n\
    MouseMoveEvent 144 247 0 0 0 0\n\
    MouseMoveEvent 145 245 0 0 0 0\n\
    MouseMoveEvent 146 243 0 0 0 0\n\
    MouseMoveEvent 146 241 0 0 0 0\n\
    MouseMoveEvent 146 240 0 0 0 0\n\
    MouseMoveEvent 146 239 0 0 0 0\n\
    MouseMoveEvent 146 237 0 0 0 0\n\
    MouseMoveEvent 147 235 0 0 0 0\n\
    MouseMoveEvent 147 234 0 0 0 0\n\
    MouseMoveEvent 147 232 0 0 0 0\n\
    MouseMoveEvent 148 230 0 0 0 0\n\
    MouseMoveEvent 148 228 0 0 0 0\n\
    MouseMoveEvent 148 226 0 0 0 0\n\
    MouseMoveEvent 148 225 0 0 0 0\n\
    MouseMoveEvent 148 224 0 0 0 0\n\
    MouseMoveEvent 149 222 0 0 0 0\n\
    MouseMoveEvent 150 221 0 0 0 0\n\
    MouseMoveEvent 150 220 0 0 0 0\n\
    MouseMoveEvent 150 220 0 0 0 0\n\
    MouseMoveEvent 150 219 0 0 0 0\n\
    MouseMoveEvent 150 218 0 0 0 0\n\
    MouseMoveEvent 151 216 0 0 0 0\n\
    MouseMoveEvent 151 214 0 0 0 0\n\
    MouseMoveEvent 151 212 0 0 0 0\n\
    MouseMoveEvent 151 211 0 0 0 0\n\
    MouseMoveEvent 151 210 0 0 0 0\n\
    MouseMoveEvent 151 209 0 0 0 0\n\
    LeftButtonPressEvent 151 209 0 0 0 0\n\
    RenderEvent 151 209 0 0 0 0\n\
    MouseMoveEvent 152 210 0 0 0 0\n\
    RenderEvent 152 210 0 0 0 0\n\
    MouseMoveEvent 152 212 0 0 0 0\n\
    RenderEvent 152 212 0 0 0 0\n\
    MouseMoveEvent 153 214 0 0 0 0\n\
    RenderEvent 153 214 0 0 0 0\n\
    MouseMoveEvent 153 216 0 0 0 0\n\
    RenderEvent 153 216 0 0 0 0\n\
    MouseMoveEvent 154 218 0 0 0 0\n\
    RenderEvent 154 218 0 0 0 0\n\
    MouseMoveEvent 154 222 0 0 0 0\n\
    RenderEvent 154 222 0 0 0 0\n\
    MouseMoveEvent 155 224 0 0 0 0\n\
    RenderEvent 155 224 0 0 0 0\n\
    MouseMoveEvent 156 226 0 0 0 0\n\
    RenderEvent 156 226 0 0 0 0\n\
    MouseMoveEvent 156 227 0 0 0 0\n\
    RenderEvent 156 227 0 0 0 0\n\
    MouseMoveEvent 156 228 0 0 0 0\n\
    RenderEvent 156 228 0 0 0 0\n\
    MouseMoveEvent 156 229 0 0 0 0\n\
    RenderEvent 156 229 0 0 0 0\n\
    MouseMoveEvent 157 231 0 0 0 0\n\
    RenderEvent 157 231 0 0 0 0\n\
    MouseMoveEvent 157 233 0 0 0 0\n\
    RenderEvent 157 233 0 0 0 0\n\
    MouseMoveEvent 157 234 0 0 0 0\n\
    RenderEvent 157 234 0 0 0 0\n\
    MouseMoveEvent 158 235 0 0 0 0\n\
    RenderEvent 158 235 0 0 0 0\n\
    MouseMoveEvent 158 236 0 0 0 0\n\
    RenderEvent 158 236 0 0 0 0\n\
    MouseMoveEvent 158 237 0 0 0 0\n\
    RenderEvent 158 237 0 0 0 0\n\
    MouseMoveEvent 158 238 0 0 0 0\n\
    RenderEvent 158 238 0 0 0 0\n\
    MouseMoveEvent 158 239 0 0 0 0\n\
    RenderEvent 158 239 0 0 0 0\n\
    MouseMoveEvent 158 242 0 0 0 0\n\
    RenderEvent 158 242 0 0 0 0\n\
    MouseMoveEvent 158 244 0 0 0 0\n\
    RenderEvent 158 244 0 0 0 0\n\
    MouseMoveEvent 158 246 0 0 0 0\n\
    RenderEvent 158 246 0 0 0 0\n\
    MouseMoveEvent 158 248 0 0 0 0\n\
    RenderEvent 158 248 0 0 0 0\n\
    MouseMoveEvent 158 249 0 0 0 0\n\
    RenderEvent 158 249 0 0 0 0\n\
    MouseMoveEvent 158 250 0 0 0 0\n\
    RenderEvent 158 250 0 0 0 0\n\
    MouseMoveEvent 158 250 0 0 0 0\n\
    RenderEvent 158 250 0 0 0 0\n\
    MouseMoveEvent 158 252 0 0 0 0\n\
    RenderEvent 158 252 0 0 0 0\n\
    MouseMoveEvent 158 252 0 0 0 0\n\
    RenderEvent 158 252 0 0 0 0\n\
    MouseMoveEvent 158 253 0 0 0 0\n\
    RenderEvent 158 253 0 0 0 0\n\
    MouseMoveEvent 158 254 0 0 0 0\n\
    RenderEvent 158 254 0 0 0 0\n\
    MouseMoveEvent 158 255 0 0 0 0\n\
    RenderEvent 158 255 0 0 0 0\n\
    MouseMoveEvent 158 257 0 0 0 0\n\
    RenderEvent 158 257 0 0 0 0\n\
    MouseMoveEvent 158 259 0 0 0 0\n\
    RenderEvent 158 259 0 0 0 0\n\
    MouseMoveEvent 158 260 0 0 0 0\n\
    RenderEvent 158 260 0 0 0 0\n\
    MouseMoveEvent 158 261 0 0 0 0\n\
    RenderEvent 158 261 0 0 0 0\n\
    LeftButtonReleaseEvent 158 261 0 0 0 0\n\
    RenderEvent 158 261 0 0 0 0\n\
    MouseMoveEvent 158 260 0 0 0 0\n\
    MouseMoveEvent 158 259 0 0 0 0\n\
    MouseMoveEvent 158 257 0 0 0 0\n\
    MouseMoveEvent 157 255 0 0 0 0\n\
    MouseMoveEvent 157 253 0 0 0 0\n\
    MouseMoveEvent 156 250 0 0 0 0\n\
    MouseMoveEvent 155 245 0 0 0 0\n\
    MouseMoveEvent 155 241 0 0 0 0\n\
    MouseMoveEvent 155 235 0 0 0 0\n\
    MouseMoveEvent 153 227 0 0 0 0\n\
    MouseMoveEvent 153 222 0 0 0 0\n\
    MouseMoveEvent 153 215 0 0 0 0\n\
    MouseMoveEvent 153 211 0 0 0 0\n\
    MouseMoveEvent 153 208 0 0 0 0\n\
    MouseMoveEvent 153 203 0 0 0 0\n\
    MouseMoveEvent 153 201 0 0 0 0\n\
    MouseMoveEvent 153 199 0 0 0 0\n\
    MouseMoveEvent 153 198 0 0 0 0\n\
    MouseMoveEvent 154 197 0 0 0 0\n\
    MouseMoveEvent 154 196 0 0 0 0\n\
    MouseMoveEvent 154 195 0 0 0 0\n\
    MouseMoveEvent 154 194 0 0 0 0\n\
    MouseMoveEvent 155 192 0 0 0 0\n\
    MouseMoveEvent 155 190 0 0 0 0\n\
    MouseMoveEvent 156 190 0 0 0 0\n\
    MouseMoveEvent 156 188 0 0 0 0\n\
    MouseMoveEvent 156 186 0 0 0 0\n\
    MouseMoveEvent 157 182 0 0 0 0\n\
    MouseMoveEvent 158 181 0 0 0 0\n\
    MouseMoveEvent 158 178 0 0 0 0\n\
    MouseMoveEvent 159 176 0 0 0 0\n\
    MouseMoveEvent 160 174 0 0 0 0\n\
    MouseMoveEvent 160 172 0 0 0 0\n\
    MouseMoveEvent 161 170 0 0 0 0\n\
    MouseMoveEvent 162 167 0 0 0 0\n\
    MouseMoveEvent 163 165 0 0 0 0\n\
    MouseMoveEvent 164 163 0 0 0 0\n\
    MouseMoveEvent 165 161 0 0 0 0\n\
    MouseMoveEvent 165 159 0 0 0 0\n\
    MouseMoveEvent 166 159 0 0 0 0\n\
    MouseMoveEvent 167 158 0 0 0 0\n\
    MouseMoveEvent 168 157 0 0 0 0\n\
    MouseMoveEvent 169 157 0 0 0 0\n\
    MouseMoveEvent 170 157 0 0 0 0\n\
    MouseMoveEvent 172 157 0 0 0 0\n\
    MouseMoveEvent 174 157 0 0 0 0\n\
    MouseMoveEvent 176 157 0 0 0 0\n\
    MouseMoveEvent 181 156 0 0 0 0\n\
    MouseMoveEvent 183 156 0 0 0 0\n\
    MouseMoveEvent 185 156 0 0 0 0\n\
    MouseMoveEvent 188 155 0 0 0 0\n\
    MouseMoveEvent 190 155 0 0 0 0\n\
    MouseMoveEvent 192 154 0 0 0 0\n\
    MouseMoveEvent 195 154 0 0 0 0\n\
    MouseMoveEvent 197 154 0 0 0 0\n\
    MouseMoveEvent 199 153 0 0 0 0\n\
    MouseMoveEvent 200 152 0 0 0 0\n\
    MouseMoveEvent 201 152 0 0 0 0\n\
    MouseMoveEvent 203 151 0 0 0 0\n\
    MouseMoveEvent 203 150 0 0 0 0\n\
    MouseMoveEvent 205 150 0 0 0 0\n\
    MouseMoveEvent 207 149 0 0 0 0\n\
    MouseMoveEvent 209 147 0 0 0 0\n\
    MouseMoveEvent 210 147 0 0 0 0\n\
    MouseMoveEvent 212 145 0 0 0 0\n\
    MouseMoveEvent 213 144 0 0 0 0\n\
    MouseMoveEvent 215 143 0 0 0 0\n\
    MouseMoveEvent 216 142 0 0 0 0\n\
    MouseMoveEvent 217 142 0 0 0 0\n\
    MouseMoveEvent 218 141 0 0 0 0\n\
    MouseMoveEvent 219 140 0 0 0 0\n\
    MouseMoveEvent 220 139 0 0 0 0\n\
    MouseMoveEvent 220 138 0 0 0 0\n\
    MouseMoveEvent 220 137 0 0 0 0\n\
    MouseMoveEvent 220 136 0 0 0 0\n\
    MouseMoveEvent 221 135 0 0 0 0\n\
    MouseMoveEvent 222 134 0 0 0 0\n\
    MouseMoveEvent 222 133 0 0 0 0\n\
    MouseMoveEvent 222 132 0 0 0 0\n\
    MouseMoveEvent 223 131 0 0 0 0\n\
    MouseMoveEvent 223 132 0 0 0 0\n\
    MouseMoveEvent 222 133 0 0 0 0\n\
    MouseMoveEvent 221 134 0 0 0 0\n\
    MouseMoveEvent 221 135 0 0 0 0\n\
    MouseMoveEvent 221 136 0 0 0 0\n\
    MouseMoveEvent 221 137 0 0 0 0\n\
    MouseMoveEvent 221 138 0 0 0 0\n\
    MouseMoveEvent 220 138 0 0 0 0\n\
    KeyPressEvent 220 138 0 116 1 t\n\
    CharEvent 220 138 0 116 1 t\n\
    KeyReleaseEvent 220 138 0 116 1 t\n\
    LeftButtonPressEvent 220 138 0 0 0 t\n\
    RenderEvent 220 138 0 0 0 t\n\
    MouseMoveEvent 220 138 0 0 0 t\n\
    RenderEvent 220 138 0 0 0 t\n\
    MouseMoveEvent 219 138 0 0 0 t\n\
    RenderEvent 219 138 0 0 0 t\n\
    MouseMoveEvent 218 138 0 0 0 t\n\
    RenderEvent 218 138 0 0 0 t\n\
    MouseMoveEvent 217 138 0 0 0 t\n\
    RenderEvent 217 138 0 0 0 t\n\
    MouseMoveEvent 216 138 0 0 0 t\n\
    RenderEvent 216 138 0 0 0 t\n\
    MouseMoveEvent 215 138 0 0 0 t\n\
    RenderEvent 215 138 0 0 0 t\n\
    MouseMoveEvent 214 138 0 0 0 t\n\
    RenderEvent 214 138 0 0 0 t\n\
    MouseMoveEvent 212 137 0 0 0 t\n\
    RenderEvent 212 137 0 0 0 t\n\
    MouseMoveEvent 209 137 0 0 0 t\n\
    RenderEvent 209 137 0 0 0 t\n\
    MouseMoveEvent 206 137 0 0 0 t\n\
    RenderEvent 206 137 0 0 0 t\n\
    MouseMoveEvent 204 137 0 0 0 t\n\
    RenderEvent 204 137 0 0 0 t\n\
    MouseMoveEvent 202 137 0 0 0 t\n\
    RenderEvent 202 137 0 0 0 t\n\
    MouseMoveEvent 200 137 0 0 0 t\n\
    RenderEvent 200 137 0 0 0 t\n\
    MouseMoveEvent 197 137 0 0 0 t\n\
    RenderEvent 197 137 0 0 0 t\n\
    MouseMoveEvent 195 137 0 0 0 t\n\
    RenderEvent 195 137 0 0 0 t\n\
    MouseMoveEvent 194 137 0 0 0 t\n\
    RenderEvent 194 137 0 0 0 t\n\
    MouseMoveEvent 193 137 0 0 0 t\n\
    RenderEvent 193 137 0 0 0 t\n\
    MouseMoveEvent 192 137 0 0 0 t\n\
    RenderEvent 192 137 0 0 0 t\n\
    MouseMoveEvent 190 137 0 0 0 t\n\
    RenderEvent 190 137 0 0 0 t\n\
    MouseMoveEvent 188 137 0 0 0 t\n\
    RenderEvent 188 137 0 0 0 t\n\
    MouseMoveEvent 187 137 0 0 0 t\n\
    RenderEvent 187 137 0 0 0 t\n\
    MouseMoveEvent 185 137 0 0 0 t\n\
    RenderEvent 185 137 0 0 0 t\n\
    MouseMoveEvent 183 136 0 0 0 t\n\
    RenderEvent 183 136 0 0 0 t\n\
    MouseMoveEvent 183 135 0 0 0 t\n\
    RenderEvent 183 135 0 0 0 t\n\
    MouseMoveEvent 182 135 0 0 0 t\n\
    RenderEvent 182 135 0 0 0 t\n\
    MouseMoveEvent 181 135 0 0 0 t\n\
    RenderEvent 181 135 0 0 0 t\n\
    MouseMoveEvent 180 135 0 0 0 t\n\
    RenderEvent 180 135 0 0 0 t\n\
    LeftButtonReleaseEvent 180 135 0 0 0 t\n\
    RenderEvent 180 135 0 0 0 t\n\
    MouseMoveEvent 181 136 0 0 0 t\n\
    MouseMoveEvent 184 136 0 0 0 t\n\
    MouseMoveEvent 187 137 0 0 0 t\n\
    MouseMoveEvent 195 138 0 0 0 t\n\
    MouseMoveEvent 201 138 0 0 0 t\n\
    MouseMoveEvent 206 140 0 0 0 t\n\
    MouseMoveEvent 212 141 0 0 0 t\n\
    MouseMoveEvent 217 142 0 0 0 t\n\
    MouseMoveEvent 221 143 0 0 0 t\n\
    MouseMoveEvent 222 143 0 0 0 t\n\
    MouseMoveEvent 224 143 0 0 0 t\n\
    MouseMoveEvent 225 143 0 0 0 t\n\
    MouseMoveEvent 225 143 0 0 0 t\n\
    MouseMoveEvent 226 143 0 0 0 t\n\
    MouseMoveEvent 227 143 0 0 0 t\n\
    MouseMoveEvent 228 143 0 0 0 t\n\
    MouseMoveEvent 229 143 0 0 0 t\n\
    MouseMoveEvent 230 144 0 0 0 t\n\
    MouseMoveEvent 231 144 0 0 0 t\n\
    MouseMoveEvent 232 144 0 0 0 t\n\
    MouseMoveEvent 233 144 0 0 0 t\n\
    MouseMoveEvent 234 144 0 0 0 t\n\
    MouseMoveEvent 235 144 0 0 0 t\n\
    MouseMoveEvent 236 144 0 0 0 t\n\
    MouseMoveEvent 237 144 0 0 0 t\n\
    MouseMoveEvent 239 144 0 0 0 t\n\
    MouseMoveEvent 241 144 0 0 0 t\n\
    MouseMoveEvent 242 144 0 0 0 t\n\
    MouseMoveEvent 243 144 0 0 0 t\n\
    MouseMoveEvent 244 144 0 0 0 t\n\
    MouseMoveEvent 245 144 0 0 0 t\n\
    MouseMoveEvent 246 144 0 0 0 t\n\
    MouseMoveEvent 247 144 0 0 0 t\n\
    MouseMoveEvent 247 145 0 0 0 t\n\
    MouseMoveEvent 248 145 0 0 0 t\n\
    MouseMoveEvent 247 144 0 0 0 t\n\
    MouseMoveEvent 246 144 0 0 0 t\n\
    MouseMoveEvent 244 142 0 0 0 t\n\
    MouseMoveEvent 239 139 0 0 0 t\n\
    MouseMoveEvent 237 138 0 0 0 t\n\
    MouseMoveEvent 232 136 0 0 0 t\n\
    MouseMoveEvent 227 133 0 0 0 t\n\
    MouseMoveEvent 220 131 0 0 0 t\n\
    MouseMoveEvent 214 130 0 0 0 t\n\
    MouseMoveEvent 209 130 0 0 0 t\n\
    MouseMoveEvent 204 130 0 0 0 t\n\
    MouseMoveEvent 198 130 0 0 0 t\n\
    MouseMoveEvent 195 133 0 0 0 t\n\
    MouseMoveEvent 190 133 0 0 0 t\n\
    MouseMoveEvent 184 135 0 0 0 t\n\
    MouseMoveEvent 180 138 0 0 0 t\n\
    MouseMoveEvent 177 138 0 0 0 t\n\
    MouseMoveEvent 176 139 0 0 0 t\n\
    MouseMoveEvent 174 140 0 0 0 t\n\
    MouseMoveEvent 172 141 0 0 0 t\n\
    MouseMoveEvent 171 142 0 0 0 t\n\
    MouseMoveEvent 170 142 0 0 0 t\n\
    MouseMoveEvent 169 142 0 0 0 t\n\
    MouseMoveEvent 168 143 0 0 0 t\n\
    MouseMoveEvent 167 143 0 0 0 t\n\
    MouseMoveEvent 166 144 0 0 0 t\n\
    MouseMoveEvent 164 145 0 0 0 t\n\
    MouseMoveEvent 162 146 0 0 0 t\n\
    MouseMoveEvent 161 146 0 0 0 t\n\
    MouseMoveEvent 160 146 0 0 0 t\n\
    MouseMoveEvent 159 146 0 0 0 t\n\
    MouseMoveEvent 157 146 0 0 0 t\n\
    MouseMoveEvent 156 147 0 0 0 t\n\
    MouseMoveEvent 155 147 0 0 0 t\n\
    MouseMoveEvent 155 147 0 0 0 t\n\
    MouseMoveEvent 154 147 0 0 0 t\n\
    MouseMoveEvent 153 148 0 0 0 t\n\
    MouseMoveEvent 153 149 0 0 0 t\n\
    MouseMoveEvent 153 149 0 0 0 t\n\
    MouseMoveEvent 154 150 0 0 0 t\n\
    MouseMoveEvent 155 150 0 0 0 t\n\
    MouseMoveEvent 157 150 0 0 0 t\n\
    MouseMoveEvent 158 150 0 0 0 t\n\
    MouseMoveEvent 160 150 0 0 0 t\n\
    MouseMoveEvent 161 150 0 0 0 t\n\
    MouseMoveEvent 162 150 0 0 0 t\n\
    MouseMoveEvent 163 150 0 0 0 t\n\
    MouseMoveEvent 164 150 0 0 0 t\n\
    MouseMoveEvent 165 150 0 0 0 t\n\
    MouseMoveEvent 166 150 0 0 0 t\n\
    MouseMoveEvent 167 150 0 0 0 t\n\
    MouseMoveEvent 169 150 0 0 0 t\n\
    MouseMoveEvent 170 150 0 0 0 t\n\
    MouseMoveEvent 171 150 0 0 0 t\n\
    MouseMoveEvent 172 150 0 0 0 t\n\
    MouseMoveEvent 174 149 0 0 0 t\n\
    MouseMoveEvent 176 149 0 0 0 t\n\
    MouseMoveEvent 178 149 0 0 0 t\n\
    MouseMoveEvent 180 149 0 0 0 t\n\
    MouseMoveEvent 183 149 0 0 0 t\n\
    MouseMoveEvent 188 149 0 0 0 t\n\
    MouseMoveEvent 190 149 0 0 0 t\n\
    MouseMoveEvent 195 149 0 0 0 t\n\
    MouseMoveEvent 197 149 0 0 0 t\n\
    MouseMoveEvent 199 149 0 0 0 t\n\
    MouseMoveEvent 202 149 0 0 0 t\n\
    MouseMoveEvent 205 149 0 0 0 t\n\
    MouseMoveEvent 208 149 0 0 0 t\n\
    MouseMoveEvent 210 149 0 0 0 t\n\
    MouseMoveEvent 211 149 0 0 0 t\n\
    MouseMoveEvent 212 149 0 0 0 t\n\
    MouseMoveEvent 213 149 0 0 0 t\n\
    MouseMoveEvent 213 149 0 0 0 t\n\
    MouseMoveEvent 214 149 0 0 0 t\n\
    MouseMoveEvent 215 149 0 0 0 t\n\
    MouseMoveEvent 216 149 0 0 0 t\n\
    MouseMoveEvent 218 149 0 0 0 t\n\
    MouseMoveEvent 220 149 0 0 0 t\n\
    MouseMoveEvent 223 149 0 0 0 t\n\
    MouseMoveEvent 225 149 0 0 0 t\n\
    MouseMoveEvent 226 149 0 0 0 t\n\
    MouseMoveEvent 227 149 0 0 0 t\n\
    MouseMoveEvent 229 149 0 0 0 t\n\
    MouseMoveEvent 230 149 0 0 0 t\n\
    MouseMoveEvent 231 149 0 0 0 t\n\
    MouseMoveEvent 232 149 0 0 0 t\n\
    MouseMoveEvent 233 149 0 0 0 t\n\
    MouseMoveEvent 234 149 0 0 0 t\n\
    MouseMoveEvent 235 149 0 0 0 t\n\
    MouseMoveEvent 236 149 0 0 0 t\n\
    MouseMoveEvent 238 149 0 0 0 t\n\
    MouseMoveEvent 240 149 0 0 0 t\n\
    MouseMoveEvent 243 149 0 0 0 t\n\
    MouseMoveEvent 244 149 0 0 0 t\n\
    MouseMoveEvent 245 149 0 0 0 t\n\
    MouseMoveEvent 246 149 0 0 0 t\n\
    MouseMoveEvent 247 149 0 0 0 t\n\
    MouseMoveEvent 248 149 0 0 0 t\n\
    MouseMoveEvent 249 149 0 0 0 t\n\
    MouseMoveEvent 250 149 0 0 0 t\n\
    MouseMoveEvent 251 149 0 0 0 t\n\
    MouseMoveEvent 252 149 0 0 0 t\n\
    MouseMoveEvent 253 149 0 0 0 t\n\
    MouseMoveEvent 254 148 0 0 0 t\n\
    MouseMoveEvent 255 148 0 0 0 t\n\
    MouseMoveEvent 257 147 0 0 0 t\n\
    MouseMoveEvent 258 147 0 0 0 t\n\
    MouseMoveEvent 260 146 0 0 0 t\n\
    MouseMoveEvent 262 145 0 0 0 t\n\
    MouseMoveEvent 263 144 0 0 0 t\n\
    MouseMoveEvent 264 144 0 0 0 t\n\
    MouseMoveEvent 265 144 0 0 0 t\n\
    MouseMoveEvent 266 144 0 0 0 t\n\
    LeftButtonPressEvent 266 144 0 0 0 t\n\
    RenderEvent 266 144 0 0 0 t\n\
    MouseMoveEvent 265 144 0 0 0 t\n\
    RenderEvent 265 144 0 0 0 t\n\
    MouseMoveEvent 264 144 0 0 0 t\n\
    RenderEvent 264 144 0 0 0 t\n\
    MouseMoveEvent 263 144 0 0 0 t\n\
    RenderEvent 263 144 0 0 0 t\n\
    MouseMoveEvent 260 144 0 0 0 t\n\
    RenderEvent 260 144 0 0 0 t\n\
    MouseMoveEvent 258 144 0 0 0 t\n\
    RenderEvent 258 144 0 0 0 t\n\
    MouseMoveEvent 255 144 0 0 0 t\n\
    RenderEvent 255 144 0 0 0 t\n\
    MouseMoveEvent 253 144 0 0 0 t\n\
    RenderEvent 253 144 0 0 0 t\n\
    MouseMoveEvent 250 144 0 0 0 t\n\
    RenderEvent 250 144 0 0 0 t\n\
    MouseMoveEvent 248 144 0 0 0 t\n\
    RenderEvent 248 144 0 0 0 t\n\
    MouseMoveEvent 247 144 0 0 0 t\n\
    RenderEvent 247 144 0 0 0 t\n\
    MouseMoveEvent 245 144 0 0 0 t\n\
    RenderEvent 245 144 0 0 0 t\n\
    MouseMoveEvent 245 144 0 0 0 t\n\
    RenderEvent 245 144 0 0 0 t\n\
    MouseMoveEvent 244 144 0 0 0 t\n\
    RenderEvent 244 144 0 0 0 t\n\
    MouseMoveEvent 243 144 0 0 0 t\n\
    RenderEvent 243 144 0 0 0 t\n\
    MouseMoveEvent 242 144 0 0 0 t\n\
    RenderEvent 242 144 0 0 0 t\n\
    MouseMoveEvent 241 144 0 0 0 t\n\
    RenderEvent 241 144 0 0 0 t\n\
    MouseMoveEvent 240 145 0 0 0 t\n\
    RenderEvent 240 145 0 0 0 t\n\
    MouseMoveEvent 239 145 0 0 0 t\n\
    RenderEvent 239 145 0 0 0 t\n\
    MouseMoveEvent 238 145 0 0 0 t\n\
    RenderEvent 238 145 0 0 0 t\n\
    MouseMoveEvent 237 145 0 0 0 t\n\
    RenderEvent 237 145 0 0 0 t\n\
    MouseMoveEvent 235 145 0 0 0 t\n\
    RenderEvent 235 145 0 0 0 t\n\
    MouseMoveEvent 233 145 0 0 0 t\n\
    RenderEvent 233 145 0 0 0 t\n\
    MouseMoveEvent 232 145 0 0 0 t\n\
    RenderEvent 232 145 0 0 0 t\n\
    MouseMoveEvent 230 145 0 0 0 t\n\
    RenderEvent 230 145 0 0 0 t\n\
    MouseMoveEvent 229 145 0 0 0 t\n\
    RenderEvent 229 145 0 0 0 t\n\
    MouseMoveEvent 228 145 0 0 0 t\n\
    RenderEvent 228 145 0 0 0 t\n\
    MouseMoveEvent 227 145 0 0 0 t\n\
    RenderEvent 227 145 0 0 0 t\n\
    MouseMoveEvent 226 145 0 0 0 t\n\
    RenderEvent 226 145 0 0 0 t\n\
    MouseMoveEvent 225 145 0 0 0 t\n\
    RenderEvent 225 145 0 0 0 t\n\
    MouseMoveEvent 225 145 0 0 0 t\n\
    RenderEvent 225 145 0 0 0 t\n\
    MouseMoveEvent 224 145 0 0 0 t\n\
    RenderEvent 224 145 0 0 0 t\n\
    MouseMoveEvent 223 145 0 0 0 t\n\
    RenderEvent 223 145 0 0 0 t\n\
    MouseMoveEvent 222 145 0 0 0 t\n\
    RenderEvent 222 145 0 0 0 t\n\
    MouseMoveEvent 221 145 0 0 0 t\n\
    RenderEvent 221 145 0 0 0 t\n\
    MouseMoveEvent 220 145 0 0 0 t\n\
    RenderEvent 220 145 0 0 0 t\n\
    MouseMoveEvent 219 145 0 0 0 t\n\
    RenderEvent 219 145 0 0 0 t\n\
    MouseMoveEvent 218 145 0 0 0 t\n\
    RenderEvent 218 145 0 0 0 t\n\
    MouseMoveEvent 217 145 0 0 0 t\n\
    RenderEvent 217 145 0 0 0 t\n\
    MouseMoveEvent 217 145 0 0 0 t\n\
    RenderEvent 217 145 0 0 0 t\n\
    MouseMoveEvent 216 145 0 0 0 t\n\
    RenderEvent 216 145 0 0 0 t\n\
    MouseMoveEvent 215 145 0 0 0 t\n\
    RenderEvent 215 145 0 0 0 t\n\
    MouseMoveEvent 215 145 0 0 0 t\n\
    RenderEvent 215 145 0 0 0 t\n\
    MouseMoveEvent 213 145 0 0 0 t\n\
    RenderEvent 213 145 0 0 0 t\n\
    MouseMoveEvent 211 145 0 0 0 t\n\
    RenderEvent 211 145 0 0 0 t\n\
    MouseMoveEvent 210 145 0 0 0 t\n\
    RenderEvent 210 145 0 0 0 t\n\
    MouseMoveEvent 209 145 0 0 0 t\n\
    RenderEvent 209 145 0 0 0 t\n\
    MouseMoveEvent 208 145 0 0 0 t\n\
    RenderEvent 208 145 0 0 0 t\n\
    MouseMoveEvent 207 145 0 0 0 t\n\
    RenderEvent 207 145 0 0 0 t\n\
    MouseMoveEvent 206 145 0 0 0 t\n\
    RenderEvent 206 145 0 0 0 t\n\
    MouseMoveEvent 204 145 0 0 0 t\n\
    RenderEvent 204 145 0 0 0 t\n\
    MouseMoveEvent 203 145 0 0 0 t\n\
    RenderEvent 203 145 0 0 0 t\n\
    MouseMoveEvent 202 145 0 0 0 t\n\
    RenderEvent 202 145 0 0 0 t\n\
    LeftButtonReleaseEvent 202 145 0 0 0 t\n\
    RenderEvent 202 145 0 0 0 t\n\
    KeyPressEvent 202 145 0 114 1 r\n\
    CharEvent 202 145 0 114 1 r\n\
    RenderEvent 202 145 0 114 1 r\n\
    KeyReleaseEvent 202 145 0 114 1 r\n\
    MouseMoveEvent 202 145 0 0 0 r\n\
    MouseMoveEvent 201 145 0 0 0 r\n\
    MouseMoveEvent 199 142 0 0 0 r\n\
    MouseMoveEvent 198 140 0 0 0 r\n\
    MouseMoveEvent 197 139 0 0 0 r\n\
    MouseMoveEvent 196 138 0 0 0 r\n\
    MouseMoveEvent 195 137 0 0 0 r\n\
    MouseMoveEvent 194 136 0 0 0 r\n\
    MouseMoveEvent 194 135 0 0 0 r\n\
    MouseMoveEvent 193 134 0 0 0 r\n\
    MouseMoveEvent 193 133 0 0 0 r\n\
    MouseMoveEvent 193 131 0 0 0 r\n\
    MouseMoveEvent 192 131 0 0 0 r\n\
    MouseMoveEvent 192 130 0 0 0 r\n\
    MouseMoveEvent 191 129 0 0 0 r\n\
    MouseMoveEvent 191 127 0 0 0 r\n\
    MouseMoveEvent 190 127 0 0 0 r\n\
    MouseMoveEvent 190 125 0 0 0 r\n\
    MouseMoveEvent 190 124 0 0 0 r\n\
    MouseMoveEvent 188 123 0 0 0 r\n\
    MouseMoveEvent 188 122 0 0 0 r\n\
    MouseMoveEvent 188 121 0 0 0 r\n\
    MouseMoveEvent 188 120 0 0 0 r\n\
    MouseMoveEvent 188 118 0 0 0 r\n\
    MouseMoveEvent 188 117 0 0 0 r\n\
    MouseMoveEvent 188 115 0 0 0 r\n\
    MouseMoveEvent 188 112 0 0 0 r\n\
    MouseMoveEvent 188 107 0 0 0 r\n\
    MouseMoveEvent 188 105 0 0 0 r\n\
    MouseMoveEvent 186 100 0 0 0 r\n\
    MouseMoveEvent 186 95 0 0 0 r\n\
    MouseMoveEvent 183 88 0 0 0 r\n\
    MouseMoveEvent 182 82 0 0 0 r\n\
    MouseMoveEvent 179 75 0 0 0 r\n\
    MouseMoveEvent 176 69 0 0 0 r\n\
    MouseMoveEvent 175 66 0 0 0 r\n\
    MouseMoveEvent 172 61 0 0 0 r\n\
    MouseMoveEvent 169 57 0 0 0 r\n\
    MouseMoveEvent 168 54 0 0 0 r\n\
    MouseMoveEvent 166 51 0 0 0 r\n\
    MouseMoveEvent 165 49 0 0 0 r\n\
    MouseMoveEvent 164 48 0 0 0 r\n\
    MouseMoveEvent 164 47 0 0 0 r\n\
    MouseMoveEvent 163 45 0 0 0 r\n\
    MouseMoveEvent 163 45 0 0 0 r\n\
    MouseMoveEvent 161 45 0 0 0 r\n\
    MouseMoveEvent 161 45 0 0 0 r\n\
    MouseMoveEvent 160 45 0 0 0 r\n\
    MouseMoveEvent 159 45 0 0 0 r\n\
    MouseMoveEvent 158 45 0 0 0 r\n\
    MouseMoveEvent 157 45 0 0 0 r\n\
    MouseMoveEvent 157 45 0 0 0 r\n\
    MouseMoveEvent 156 45 0 0 0 r\n\
    MouseMoveEvent 155 45 0 0 0 r\n\
    MouseMoveEvent 154 45 0 0 0 r\n\
    MouseMoveEvent 153 45 0 0 0 r\n\
    LeftButtonPressEvent 153 45 0 0 0 r\n\
    RenderEvent 153 45 0 0 0 r\n\
    MouseMoveEvent 153 45 0 0 0 r\n\
    RenderEvent 153 45 0 0 0 r\n\
    MouseMoveEvent 153 44 0 0 0 r\n\
    RenderEvent 153 44 0 0 0 r\n\
    MouseMoveEvent 153 43 0 0 0 r\n\
    RenderEvent 153 43 0 0 0 r\n\
    MouseMoveEvent 153 42 0 0 0 r\n\
    RenderEvent 153 42 0 0 0 r\n\
    MouseMoveEvent 154 41 0 0 0 r\n\
    RenderEvent 154 41 0 0 0 r\n\
    MouseMoveEvent 154 39 0 0 0 r\n\
    RenderEvent 154 39 0 0 0 r\n\
    MouseMoveEvent 154 37 0 0 0 r\n\
    RenderEvent 154 37 0 0 0 r\n\
    MouseMoveEvent 154 35 0 0 0 r\n\
    RenderEvent 154 35 0 0 0 r\n\
    MouseMoveEvent 154 34 0 0 0 r\n\
    RenderEvent 154 34 0 0 0 r\n\
    MouseMoveEvent 154 33 0 0 0 r\n\
    RenderEvent 154 33 0 0 0 r\n\
    MouseMoveEvent 154 32 0 0 0 r\n\
    RenderEvent 154 32 0 0 0 r\n\
    MouseMoveEvent 154 32 0 0 0 r\n\
    RenderEvent 154 32 0 0 0 r\n\
    MouseMoveEvent 154 31 0 0 0 r\n\
    RenderEvent 154 31 0 0 0 r\n\
    MouseMoveEvent 154 29 0 0 0 r\n\
    RenderEvent 154 29 0 0 0 r\n\
    MouseMoveEvent 154 28 0 0 0 r\n\
    RenderEvent 154 28 0 0 0 r\n\
    MouseMoveEvent 155 26 0 0 0 r\n\
    RenderEvent 155 26 0 0 0 r\n\
    MouseMoveEvent 155 25 0 0 0 r\n\
    RenderEvent 155 25 0 0 0 r\n\
    MouseMoveEvent 155 24 0 0 0 r\n\
    RenderEvent 155 24 0 0 0 r\n\
    MouseMoveEvent 155 23 0 0 0 r\n\
    RenderEvent 155 23 0 0 0 r\n\
    MouseMoveEvent 155 21 0 0 0 r\n\
    RenderEvent 155 21 0 0 0 r\n\
    MouseMoveEvent 155 19 0 0 0 r\n\
    RenderEvent 155 19 0 0 0 r\n\
    MouseMoveEvent 155 17 0 0 0 r\n\
    RenderEvent 155 17 0 0 0 r\n\
    MouseMoveEvent 155 16 0 0 0 r\n\
    RenderEvent 155 16 0 0 0 r\n\
    MouseMoveEvent 155 15 0 0 0 r\n\
    RenderEvent 155 15 0 0 0 r\n\
    MouseMoveEvent 155 14 0 0 0 r\n\
    RenderEvent 155 14 0 0 0 r\n\
    MouseMoveEvent 155 13 0 0 0 r\n\
    RenderEvent 155 13 0 0 0 r\n\
    MouseMoveEvent 156 12 0 0 0 r\n\
    RenderEvent 156 12 0 0 0 r\n\
    MouseMoveEvent 156 11 0 0 0 r\n\
    RenderEvent 156 11 0 0 0 r\n\
    MouseMoveEvent 156 10 0 0 0 r\n\
    RenderEvent 156 10 0 0 0 r\n\
    MouseMoveEvent 156 9 0 0 0 r\n\
    RenderEvent 156 9 0 0 0 r\n\
    MouseMoveEvent 156 8 0 0 0 r\n\
    RenderEvent 156 8 0 0 0 r\n\
    MouseMoveEvent 156 7 0 0 0 r\n\
    RenderEvent 156 7 0 0 0 r\n\
    MouseMoveEvent 156 6 0 0 0 r\n\
    RenderEvent 156 6 0 0 0 r\n\
    MouseMoveEvent 156 6 0 0 0 r\n\
    RenderEvent 156 6 0 0 0 r\n\
    MouseMoveEvent 156 6 0 0 0 r\n\
    RenderEvent 156 6 0 0 0 r\n\
    MouseMoveEvent 156 8 0 0 0 r\n\
    RenderEvent 156 8 0 0 0 r\n\
    MouseMoveEvent 156 10 0 0 0 r\n\
    RenderEvent 156 10 0 0 0 r\n\
    MouseMoveEvent 156 12 0 0 0 r\n\
    RenderEvent 156 12 0 0 0 r\n\
    MouseMoveEvent 156 15 0 0 0 r\n\
    RenderEvent 156 15 0 0 0 r\n\
    MouseMoveEvent 156 17 0 0 0 r\n\
    RenderEvent 156 17 0 0 0 r\n\
    MouseMoveEvent 156 19 0 0 0 r\n\
    RenderEvent 156 19 0 0 0 r\n\
    MouseMoveEvent 156 21 0 0 0 r\n\
    RenderEvent 156 21 0 0 0 r\n\
    MouseMoveEvent 156 22 0 0 0 r\n\
    RenderEvent 156 22 0 0 0 r\n\
    MouseMoveEvent 156 23 0 0 0 r\n\
    RenderEvent 156 23 0 0 0 r\n\
    LeftButtonReleaseEvent 156 23 0 0 0 r\n\
    RenderEvent 156 23 0 0 0 r\n\
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
tensor = [1,0,0, 0,2,0, 0,0,4]
pos = [1,2,3]
rep = vtk.vtkTensorRepresentation()
tensProp = rep.GetEllipsoidProperty()
tensProp.SetColor(0.4,0.4,0.8)
tensProp.SetRepresentationToWireframe()
rep.SetPlaceFactor(1)
rep.PlaceTensor(tensor,pos)

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
ren.SetBackground(0.1, 0.2, 0.4)
renWin.SetSize(300, 300)

# render and interact with data
ren.ResetCamera()
renWin.Render()

# Playack events
recorder.Play()

iRen.Start()
