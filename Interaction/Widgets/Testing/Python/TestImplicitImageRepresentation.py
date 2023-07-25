#!/usr/bin/env python
# -*- coding: utf-8 -*-



from vtkmodules.vtkCommonDataModel import (
    vtkPlane,
    vtkSphere,
)
from vtkmodules.vtkImagingHybrid import vtkSampleFunction
from vtkmodules.vtkInteractionWidgets import (
    vtkImplicitImageRepresentation,
    vtkImplicitPlaneWidget2,
)
from vtkmodules.vtkRenderingCore import (
    vtkInteractorEventRecorder,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
import vtkmodules.test.Testing
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# These are the pre-recorded events
Recording = \
    "# StreamVersion 1.1\n\
    ExposeEvent 0 299 0 0 0 0\n\
    RenderEvent 0 299 0 0 0 0\n\
    EnterEvent 299 216 0 0 0 0\n\
    MouseMoveEvent 299 216 0 0 0 0\n\
    MouseMoveEvent 297 216 0 0 0 0\n\
    MouseMoveEvent 296 218 0 0 0 0\n\
    MouseMoveEvent 295 218 0 0 0 0\n\
    MouseMoveEvent 294 219 0 0 0 0\n\
    MouseMoveEvent 293 219 0 0 0 0\n\
    MouseMoveEvent 292 220 0 0 0 0\n\
    MouseMoveEvent 291 221 0 0 0 0\n\
    MouseMoveEvent 289 221 0 0 0 0\n\
    MouseMoveEvent 288 222 0 0 0 0\n\
    MouseMoveEvent 286 222 0 0 0 0\n\
    MouseMoveEvent 285 222 0 0 0 0\n\
    MouseMoveEvent 284 223 0 0 0 0\n\
    MouseMoveEvent 282 223 0 0 0 0\n\
    MouseMoveEvent 281 223 0 0 0 0\n\
    MouseMoveEvent 279 223 0 0 0 0\n\
    MouseMoveEvent 278 224 0 0 0 0\n\
    MouseMoveEvent 276 224 0 0 0 0\n\
    MouseMoveEvent 274 224 0 0 0 0\n\
    MouseMoveEvent 272 225 0 0 0 0\n\
    MouseMoveEvent 271 225 0 0 0 0\n\
    MouseMoveEvent 270 225 0 0 0 0\n\
    MouseMoveEvent 268 226 0 0 0 0\n\
    MouseMoveEvent 266 226 0 0 0 0\n\
    MouseMoveEvent 264 227 0 0 0 0\n\
    MouseMoveEvent 262 227 0 0 0 0\n\
    MouseMoveEvent 262 228 0 0 0 0\n\
    MouseMoveEvent 260 228 0 0 0 0\n\
    MouseMoveEvent 259 229 0 0 0 0\n\
    MouseMoveEvent 258 229 0 0 0 0\n\
    MouseMoveEvent 257 230 0 0 0 0\n\
    MouseMoveEvent 256 230 0 0 0 0\n\
    MouseMoveEvent 255 230 0 0 0 0\n\
    MouseMoveEvent 254 231 0 0 0 0\n\
    MouseMoveEvent 252 231 0 0 0 0\n\
    MouseMoveEvent 251 232 0 0 0 0\n\
    MouseMoveEvent 250 232 0 0 0 0\n\
    MouseMoveEvent 249 233 0 0 0 0\n\
    MouseMoveEvent 248 233 0 0 0 0\n\
    MouseMoveEvent 248 234 0 0 0 0\n\
    MouseMoveEvent 247 235 0 0 0 0\n\
    MouseMoveEvent 247 236 0 0 0 0\n\
    MouseMoveEvent 246 237 0 0 0 0\n\
    RenderEvent 246 237 0 0 0 0\n\
    MouseMoveEvent 245 237 0 0 0 0\n\
    RenderEvent 245 237 0 0 0 0\n\
    MouseMoveEvent 245 238 0 0 0 0\n\
    RenderEvent 245 238 0 0 0 0\n\
    LeftButtonPressEvent 245 238 0 0 0 0\n\
    RenderEvent 245 238 0 0 0 0\n\
    MouseMoveEvent 244 236 0 0 0 0\n\
    RenderEvent 244 236 0 0 0 0\n\
    MouseMoveEvent 243 234 0 0 0 0\n\
    RenderEvent 243 234 0 0 0 0\n\
    MouseMoveEvent 241 232 0 0 0 0\n\
    RenderEvent 241 232 0 0 0 0\n\
    MouseMoveEvent 239 230 0 0 0 0\n\
    RenderEvent 239 230 0 0 0 0\n\
    MouseMoveEvent 237 228 0 0 0 0\n\
    RenderEvent 237 228 0 0 0 0\n\
    MouseMoveEvent 236 227 0 0 0 0\n\
    RenderEvent 236 227 0 0 0 0\n\
    MouseMoveEvent 235 226 0 0 0 0\n\
    RenderEvent 235 226 0 0 0 0\n\
    MouseMoveEvent 233 225 0 0 0 0\n\
    RenderEvent 233 225 0 0 0 0\n\
    MouseMoveEvent 231 223 0 0 0 0\n\
    RenderEvent 231 223 0 0 0 0\n\
    MouseMoveEvent 230 222 0 0 0 0\n\
    RenderEvent 230 222 0 0 0 0\n\
    MouseMoveEvent 227 220 0 0 0 0\n\
    RenderEvent 227 220 0 0 0 0\n\
    MouseMoveEvent 226 219 0 0 0 0\n\
    RenderEvent 226 219 0 0 0 0\n\
    MouseMoveEvent 224 218 0 0 0 0\n\
    RenderEvent 224 218 0 0 0 0\n\
    MouseMoveEvent 223 218 0 0 0 0\n\
    RenderEvent 223 218 0 0 0 0\n\
    MouseMoveEvent 222 218 0 0 0 0\n\
    RenderEvent 222 218 0 0 0 0\n\
    MouseMoveEvent 218 215 0 0 0 0\n\
    RenderEvent 218 215 0 0 0 0\n\
    MouseMoveEvent 216 213 0 0 0 0\n\
    RenderEvent 216 213 0 0 0 0\n\
    MouseMoveEvent 213 211 0 0 0 0\n\
    RenderEvent 213 211 0 0 0 0\n\
    MouseMoveEvent 211 209 0 0 0 0\n\
    RenderEvent 211 209 0 0 0 0\n\
    MouseMoveEvent 207 206 0 0 0 0\n\
    RenderEvent 207 206 0 0 0 0\n\
    MouseMoveEvent 205 206 0 0 0 0\n\
    RenderEvent 205 206 0 0 0 0\n\
    MouseMoveEvent 203 206 0 0 0 0\n\
    RenderEvent 203 206 0 0 0 0\n\
    MouseMoveEvent 203 205 0 0 0 0\n\
    RenderEvent 203 205 0 0 0 0\n\
    MouseMoveEvent 202 205 0 0 0 0\n\
    RenderEvent 202 205 0 0 0 0\n\
    MouseMoveEvent 201 205 0 0 0 0\n\
    RenderEvent 201 205 0 0 0 0\n\
    MouseMoveEvent 199 205 0 0 0 0\n\
    RenderEvent 199 205 0 0 0 0\n\
    MouseMoveEvent 197 204 0 0 0 0\n\
    RenderEvent 197 204 0 0 0 0\n\
    MouseMoveEvent 195 204 0 0 0 0\n\
    RenderEvent 195 204 0 0 0 0\n\
    MouseMoveEvent 193 204 0 0 0 0\n\
    RenderEvent 193 204 0 0 0 0\n\
    MouseMoveEvent 189 205 0 0 0 0\n\
    RenderEvent 189 205 0 0 0 0\n\
    MouseMoveEvent 185 205 0 0 0 0\n\
    RenderEvent 185 205 0 0 0 0\n\
    MouseMoveEvent 183 205 0 0 0 0\n\
    RenderEvent 183 205 0 0 0 0\n\
    MouseMoveEvent 181 205 0 0 0 0\n\
    RenderEvent 181 205 0 0 0 0\n\
    MouseMoveEvent 180 205 0 0 0 0\n\
    RenderEvent 180 205 0 0 0 0\n\
    MouseMoveEvent 179 206 0 0 0 0\n\
    RenderEvent 179 206 0 0 0 0\n\
    MouseMoveEvent 177 208 0 0 0 0\n\
    RenderEvent 177 208 0 0 0 0\n\
    MouseMoveEvent 176 208 0 0 0 0\n\
    RenderEvent 176 208 0 0 0 0\n\
    MouseMoveEvent 173 210 0 0 0 0\n\
    RenderEvent 173 210 0 0 0 0\n\
    MouseMoveEvent 172 212 0 0 0 0\n\
    RenderEvent 172 212 0 0 0 0\n\
    MouseMoveEvent 170 213 0 0 0 0\n\
    RenderEvent 170 213 0 0 0 0\n\
    MouseMoveEvent 169 214 0 0 0 0\n\
    RenderEvent 169 214 0 0 0 0\n\
    MouseMoveEvent 167 215 0 0 0 0\n\
    RenderEvent 167 215 0 0 0 0\n\
    MouseMoveEvent 165 217 0 0 0 0\n\
    RenderEvent 165 217 0 0 0 0\n\
    MouseMoveEvent 165 219 0 0 0 0\n\
    RenderEvent 165 219 0 0 0 0\n\
    MouseMoveEvent 164 219 0 0 0 0\n\
    RenderEvent 164 219 0 0 0 0\n\
    MouseMoveEvent 163 220 0 0 0 0\n\
    RenderEvent 163 220 0 0 0 0\n\
    MouseMoveEvent 161 222 0 0 0 0\n\
    RenderEvent 161 222 0 0 0 0\n\
    MouseMoveEvent 161 224 0 0 0 0\n\
    RenderEvent 161 224 0 0 0 0\n\
    MouseMoveEvent 160 225 0 0 0 0\n\
    RenderEvent 160 225 0 0 0 0\n\
    MouseMoveEvent 159 227 0 0 0 0\n\
    RenderEvent 159 227 0 0 0 0\n\
    MouseMoveEvent 159 228 0 0 0 0\n\
    RenderEvent 159 228 0 0 0 0\n\
    MouseMoveEvent 158 228 0 0 0 0\n\
    RenderEvent 158 228 0 0 0 0\n\
    MouseMoveEvent 158 228 0 0 0 0\n\
    RenderEvent 158 228 0 0 0 0\n\
    MouseMoveEvent 157 229 0 0 0 0\n\
    RenderEvent 157 229 0 0 0 0\n\
    MouseMoveEvent 157 231 0 0 0 0\n\
    RenderEvent 157 231 0 0 0 0\n\
    MouseMoveEvent 156 233 0 0 0 0\n\
    RenderEvent 156 233 0 0 0 0\n\
    MouseMoveEvent 156 235 0 0 0 0\n\
    RenderEvent 156 235 0 0 0 0\n\
    MouseMoveEvent 156 236 0 0 0 0\n\
    RenderEvent 156 236 0 0 0 0\n\
    MouseMoveEvent 156 238 0 0 0 0\n\
    RenderEvent 156 238 0 0 0 0\n\
    LeftButtonReleaseEvent 156 238 0 0 0 0\n\
    RenderEvent 156 238 0 0 0 0\n\
    MouseMoveEvent 156 237 0 0 0 0\n\
    RenderEvent 156 237 0 0 0 0\n\
    MouseMoveEvent 156 236 0 0 0 0\n\
    RenderEvent 156 236 0 0 0 0\n\
    MouseMoveEvent 156 236 0 0 0 0\n\
    RenderEvent 156 236 0 0 0 0\n\
    MouseMoveEvent 156 235 0 0 0 0\n\
    RenderEvent 156 235 0 0 0 0\n\
    MouseMoveEvent 156 234 0 0 0 0\n\
    RenderEvent 156 234 0 0 0 0\n\
    MouseMoveEvent 155 233 0 0 0 0\n\
    RenderEvent 155 233 0 0 0 0\n\
    MouseMoveEvent 155 232 0 0 0 0\n\
    RenderEvent 155 232 0 0 0 0\n\
    MouseMoveEvent 155 230 0 0 0 0\n\
    RenderEvent 155 230 0 0 0 0\n\
    MouseMoveEvent 153 226 0 0 0 0\n\
    RenderEvent 153 226 0 0 0 0\n\
    MouseMoveEvent 153 221 0 0 0 0\n\
    RenderEvent 153 221 0 0 0 0\n\
    MouseMoveEvent 152 216 0 0 0 0\n\
    RenderEvent 152 216 0 0 0 0\n\
    MouseMoveEvent 151 213 0 0 0 0\n\
    RenderEvent 151 213 0 0 0 0\n\
    MouseMoveEvent 148 207 0 0 0 0\n\
    RenderEvent 148 207 0 0 0 0\n\
    MouseMoveEvent 148 205 0 0 0 0\n\
    RenderEvent 148 205 0 0 0 0\n\
    MouseMoveEvent 147 203 0 0 0 0\n\
    RenderEvent 147 203 0 0 0 0\n\
    MouseMoveEvent 147 200 0 0 0 0\n\
    RenderEvent 147 200 0 0 0 0\n\
    MouseMoveEvent 147 193 0 0 0 0\n\
    RenderEvent 147 193 0 0 0 0\n\
    MouseMoveEvent 147 189 0 0 0 0\n\
    RenderEvent 147 189 0 0 0 0\n\
    MouseMoveEvent 147 185 0 0 0 0\n\
    RenderEvent 147 185 0 0 0 0\n\
    MouseMoveEvent 147 180 0 0 0 0\n\
    RenderEvent 147 180 0 0 0 0\n\
    MouseMoveEvent 150 177 0 0 0 0\n\
    RenderEvent 150 177 0 0 0 0\n\
    MouseMoveEvent 152 174 0 0 0 0\n\
    RenderEvent 152 174 0 0 0 0\n\
    MouseMoveEvent 154 172 0 0 0 0\n\
    RenderEvent 154 172 0 0 0 0\n\
    MouseMoveEvent 154 166 0 0 0 0\n\
    RenderEvent 154 166 0 0 0 0\n\
    MouseMoveEvent 156 164 0 0 0 0\n\
    RenderEvent 156 164 0 0 0 0\n\
    MouseMoveEvent 156 161 0 0 0 0\n\
    RenderEvent 156 161 0 0 0 0\n\
    MouseMoveEvent 157 158 0 0 0 0\n\
    RenderEvent 157 158 0 0 0 0\n\
    MouseMoveEvent 157 155 0 0 0 0\n\
    RenderEvent 157 155 0 0 0 0\n\
    MouseMoveEvent 157 152 0 0 0 0\n\
    RenderEvent 157 152 0 0 0 0\n\
    MouseMoveEvent 157 150 0 0 0 0\n\
    RenderEvent 157 150 0 0 0 0\n\
    MouseMoveEvent 157 148 0 0 0 0\n\
    RenderEvent 157 148 0 0 0 0\n\
    MouseMoveEvent 156 148 0 0 0 0\n\
    RenderEvent 156 148 0 0 0 0\n\
    MouseMoveEvent 155 148 0 0 0 0\n\
    RenderEvent 155 148 0 0 0 0\n\
    LeftButtonPressEvent 155 148 0 0 0 0\n\
    RenderEvent 155 148 0 0 0 0\n\
    MouseMoveEvent 155 148 0 0 0 0\n\
    RenderEvent 155 148 0 0 0 0\n\
    MouseMoveEvent 155 148 0 0 0 0\n\
    RenderEvent 155 148 0 0 0 0\n\
    MouseMoveEvent 154 148 0 0 0 0\n\
    RenderEvent 154 148 0 0 0 0\n\
    MouseMoveEvent 152 150 0 0 0 0\n\
    RenderEvent 152 150 0 0 0 0\n\
    MouseMoveEvent 149 152 0 0 0 0\n\
    RenderEvent 149 152 0 0 0 0\n\
    MouseMoveEvent 147 154 0 0 0 0\n\
    RenderEvent 147 154 0 0 0 0\n\
    MouseMoveEvent 146 155 0 0 0 0\n\
    RenderEvent 146 155 0 0 0 0\n\
    MouseMoveEvent 144 157 0 0 0 0\n\
    RenderEvent 144 157 0 0 0 0\n\
    MouseMoveEvent 141 159 0 0 0 0\n\
    RenderEvent 141 159 0 0 0 0\n\
    MouseMoveEvent 139 161 0 0 0 0\n\
    RenderEvent 139 161 0 0 0 0\n\
    MouseMoveEvent 138 162 0 0 0 0\n\
    RenderEvent 138 162 0 0 0 0\n\
    MouseMoveEvent 136 162 0 0 0 0\n\
    RenderEvent 136 162 0 0 0 0\n\
    MouseMoveEvent 134 164 0 0 0 0\n\
    RenderEvent 134 164 0 0 0 0\n\
    MouseMoveEvent 132 165 0 0 0 0\n\
    RenderEvent 132 165 0 0 0 0\n\
    MouseMoveEvent 130 165 0 0 0 0\n\
    RenderEvent 130 165 0 0 0 0\n\
    MouseMoveEvent 128 166 0 0 0 0\n\
    RenderEvent 128 166 0 0 0 0\n\
    MouseMoveEvent 127 167 0 0 0 0\n\
    RenderEvent 127 167 0 0 0 0\n\
    MouseMoveEvent 126 168 0 0 0 0\n\
    RenderEvent 126 168 0 0 0 0\n\
    MouseMoveEvent 127 168 0 0 0 0\n\
    RenderEvent 127 168 0 0 0 0\n\
    MouseMoveEvent 127 167 0 0 0 0\n\
    RenderEvent 127 167 0 0 0 0\n\
    MouseMoveEvent 130 166 0 0 0 0\n\
    RenderEvent 130 166 0 0 0 0\n\
    MouseMoveEvent 131 165 0 0 0 0\n\
    RenderEvent 131 165 0 0 0 0\n\
    MouseMoveEvent 134 163 0 0 0 0\n\
    RenderEvent 134 163 0 0 0 0\n\
    MouseMoveEvent 136 162 0 0 0 0\n\
    RenderEvent 136 162 0 0 0 0\n\
    MouseMoveEvent 137 160 0 0 0 0\n\
    RenderEvent 137 160 0 0 0 0\n\
    MouseMoveEvent 138 159 0 0 0 0\n\
    RenderEvent 138 159 0 0 0 0\n\
    MouseMoveEvent 139 158 0 0 0 0\n\
    RenderEvent 139 158 0 0 0 0\n\
    MouseMoveEvent 140 157 0 0 0 0\n\
    RenderEvent 140 157 0 0 0 0\n\
    MouseMoveEvent 141 157 0 0 0 0\n\
    RenderEvent 141 157 0 0 0 0\n\
    MouseMoveEvent 142 156 0 0 0 0\n\
    RenderEvent 142 156 0 0 0 0\n\
    MouseMoveEvent 143 156 0 0 0 0\n\
    RenderEvent 143 156 0 0 0 0\n\
    MouseMoveEvent 144 155 0 0 0 0\n\
    RenderEvent 144 155 0 0 0 0\n\
    MouseMoveEvent 145 154 0 0 0 0\n\
    RenderEvent 145 154 0 0 0 0\n\
    MouseMoveEvent 146 154 0 0 0 0\n\
    RenderEvent 146 154 0 0 0 0\n\
    MouseMoveEvent 147 153 0 0 0 0\n\
    RenderEvent 147 153 0 0 0 0\n\
    MouseMoveEvent 147 153 0 0 0 0\n\
    RenderEvent 147 153 0 0 0 0\n\
    MouseMoveEvent 148 152 0 0 0 0\n\
    RenderEvent 148 152 0 0 0 0\n\
    MouseMoveEvent 149 152 0 0 0 0\n\
    RenderEvent 149 152 0 0 0 0\n\
    MouseMoveEvent 151 152 0 0 0 0\n\
    RenderEvent 151 152 0 0 0 0\n\
    MouseMoveEvent 152 151 0 0 0 0\n\
    RenderEvent 152 151 0 0 0 0\n\
    MouseMoveEvent 153 151 0 0 0 0\n\
    RenderEvent 153 151 0 0 0 0\n\
    MouseMoveEvent 154 150 0 0 0 0\n\
    RenderEvent 154 150 0 0 0 0\n\
    MouseMoveEvent 157 149 0 0 0 0\n\
    RenderEvent 157 149 0 0 0 0\n\
    MouseMoveEvent 158 147 0 0 0 0\n\
    RenderEvent 158 147 0 0 0 0\n\
    MouseMoveEvent 160 146 0 0 0 0\n\
    RenderEvent 160 146 0 0 0 0\n\
    MouseMoveEvent 161 145 0 0 0 0\n\
    RenderEvent 161 145 0 0 0 0\n\
    MouseMoveEvent 162 144 0 0 0 0\n\
    RenderEvent 162 144 0 0 0 0\n\
    MouseMoveEvent 163 143 0 0 0 0\n\
    RenderEvent 163 143 0 0 0 0\n\
    MouseMoveEvent 164 143 0 0 0 0\n\
    RenderEvent 164 143 0 0 0 0\n\
    MouseMoveEvent 165 143 0 0 0 0\n\
    RenderEvent 165 143 0 0 0 0\n\
    MouseMoveEvent 167 141 0 0 0 0\n\
    RenderEvent 167 141 0 0 0 0\n\
    MouseMoveEvent 168 140 0 0 0 0\n\
    RenderEvent 168 140 0 0 0 0\n\
    MouseMoveEvent 170 139 0 0 0 0\n\
    RenderEvent 170 139 0 0 0 0\n\
    MouseMoveEvent 171 139 0 0 0 0\n\
    RenderEvent 171 139 0 0 0 0\n\
    MouseMoveEvent 172 138 0 0 0 0\n\
    RenderEvent 172 138 0 0 0 0\n\
    LeftButtonReleaseEvent 172 138 0 0 0 0\n\
    RenderEvent 172 138 0 0 0 0\n\
    MouseMoveEvent 172 139 0 0 0 0\n\
    RenderEvent 172 139 0 0 0 0\n\
    MouseMoveEvent 171 139 0 0 0 0\n\
    MouseMoveEvent 170 140 0 0 0 0\n\
    MouseMoveEvent 170 141 0 0 0 0\n\
    MouseMoveEvent 169 142 0 0 0 0\n\
    MouseMoveEvent 168 143 0 0 0 0\n\
    MouseMoveEvent 167 144 0 0 0 0\n\
    MouseMoveEvent 166 145 0 0 0 0\n\
    MouseMoveEvent 165 146 0 0 0 0\n\
    MouseMoveEvent 165 147 0 0 0 0\n\
    MouseMoveEvent 164 147 0 0 0 0\n\
    MouseMoveEvent 162 147 0 0 0 0\n\
    RenderEvent 162 147 0 0 0 0\n\
    MouseMoveEvent 160 149 0 0 0 0\n\
    MouseMoveEvent 158 149 0 0 0 0\n\
    MouseMoveEvent 158 150 0 0 0 0\n\
    MouseMoveEvent 156 151 0 0 0 0\n\
    MouseMoveEvent 155 152 0 0 0 0\n\
    MouseMoveEvent 154 152 0 0 0 0\n\
    MouseMoveEvent 153 153 0 0 0 0\n\
    MouseMoveEvent 151 153 0 0 0 0\n\
    RenderEvent 151 153 0 0 0 0\n\
    MouseMoveEvent 149 155 0 0 0 0\n\
    RenderEvent 149 155 0 0 0 0\n\
    MouseMoveEvent 144 157 0 0 0 0\n\
    RenderEvent 144 157 0 0 0 0\n\
    MouseMoveEvent 141 159 0 0 0 0\n\
    RenderEvent 141 159 0 0 0 0\n\
    MouseMoveEvent 136 162 0 0 0 0\n\
    RenderEvent 136 162 0 0 0 0\n\
    MouseMoveEvent 133 164 0 0 0 0\n\
    MouseMoveEvent 132 165 0 0 0 0\n\
    MouseMoveEvent 131 166 0 0 0 0\n\
    MouseMoveEvent 129 166 0 0 0 0\n\
    MouseMoveEvent 128 167 0 0 0 0\n\
    MouseMoveEvent 127 168 0 0 0 0\n\
    MouseMoveEvent 126 169 0 0 0 0\n\
    MouseMoveEvent 125 170 0 0 0 0\n\
    MouseMoveEvent 124 170 0 0 0 0\n\
    MouseMoveEvent 123 172 0 0 0 0\n\
    MouseMoveEvent 122 172 0 0 0 0\n\
    MouseMoveEvent 121 172 0 0 0 0\n\
    MouseMoveEvent 120 172 0 0 0 0\n\
    RenderEvent 120 172 0 0 0 0\n\
    MouseMoveEvent 118 174 0 0 0 0\n\
    RenderEvent 118 174 0 0 0 0\n\
    MouseMoveEvent 117 175 0 0 0 0\n\
    RenderEvent 117 175 0 0 0 0\n\
    MouseMoveEvent 115 177 0 0 0 0\n\
    RenderEvent 115 177 0 0 0 0\n\
    MouseMoveEvent 113 178 0 0 0 0\n\
    RenderEvent 113 178 0 0 0 0\n\
    MouseMoveEvent 113 179 0 0 0 0\n\
    RenderEvent 113 179 0 0 0 0\n\
    MouseMoveEvent 111 181 0 0 0 0\n\
    RenderEvent 111 181 0 0 0 0\n\
    MouseMoveEvent 109 184 0 0 0 0\n\
    RenderEvent 109 184 0 0 0 0\n\
    MouseMoveEvent 109 186 0 0 0 0\n\
    RenderEvent 109 186 0 0 0 0\n\
    MouseMoveEvent 108 188 0 0 0 0\n\
    RenderEvent 108 188 0 0 0 0\n\
    MouseMoveEvent 107 189 0 0 0 0\n\
    RenderEvent 107 189 0 0 0 0\n\
    MouseMoveEvent 107 190 0 0 0 0\n\
    RenderEvent 107 190 0 0 0 0\n\
    MouseMoveEvent 107 191 0 0 0 0\n\
    RenderEvent 107 191 0 0 0 0\n\
    MouseMoveEvent 107 191 0 0 0 0\n\
    RenderEvent 107 191 0 0 0 0\n\
    LeftButtonPressEvent 107 191 0 0 0 0\n\
    RenderEvent 107 191 0 0 0 0\n\
    MouseMoveEvent 107 191 0 0 0 0\n\
    RenderEvent 107 191 0 0 0 0\n\
    MouseMoveEvent 108 191 0 0 0 0\n\
    RenderEvent 108 191 0 0 0 0\n\
    MouseMoveEvent 110 191 0 0 0 0\n\
    RenderEvent 110 191 0 0 0 0\n\
    MouseMoveEvent 111 190 0 0 0 0\n\
    RenderEvent 111 190 0 0 0 0\n\
    MouseMoveEvent 113 190 0 0 0 0\n\
    RenderEvent 113 190 0 0 0 0\n\
    MouseMoveEvent 116 190 0 0 0 0\n\
    RenderEvent 116 190 0 0 0 0\n\
    MouseMoveEvent 117 190 0 0 0 0\n\
    RenderEvent 117 190 0 0 0 0\n\
    MouseMoveEvent 119 190 0 0 0 0\n\
    RenderEvent 119 190 0 0 0 0\n\
    MouseMoveEvent 121 190 0 0 0 0\n\
    RenderEvent 121 190 0 0 0 0\n\
    MouseMoveEvent 128 190 0 0 0 0\n\
    RenderEvent 128 190 0 0 0 0\n\
    MouseMoveEvent 131 190 0 0 0 0\n\
    RenderEvent 131 190 0 0 0 0\n\
    MouseMoveEvent 136 190 0 0 0 0\n\
    RenderEvent 136 190 0 0 0 0\n\
    MouseMoveEvent 139 189 0 0 0 0\n\
    RenderEvent 139 189 0 0 0 0\n\
    MouseMoveEvent 142 189 0 0 0 0\n\
    RenderEvent 142 189 0 0 0 0\n\
    MouseMoveEvent 146 190 0 0 0 0\n\
    RenderEvent 146 190 0 0 0 0\n\
    MouseMoveEvent 148 188 0 0 0 0\n\
    RenderEvent 148 188 0 0 0 0\n\
    MouseMoveEvent 150 188 0 0 0 0\n\
    RenderEvent 150 188 0 0 0 0\n\
    MouseMoveEvent 151 189 0 0 0 0\n\
    RenderEvent 151 189 0 0 0 0\n\
    MouseMoveEvent 153 188 0 0 0 0\n\
    RenderEvent 153 188 0 0 0 0\n\
    MouseMoveEvent 153 187 0 0 0 0\n\
    RenderEvent 153 187 0 0 0 0\n\
    MouseMoveEvent 154 187 0 0 0 0\n\
    RenderEvent 154 187 0 0 0 0\n\
    MouseMoveEvent 155 187 0 0 0 0\n\
    RenderEvent 155 187 0 0 0 0\n\
    MouseMoveEvent 156 188 0 0 0 0\n\
    RenderEvent 156 188 0 0 0 0\n\
    MouseMoveEvent 158 188 0 0 0 0\n\
    RenderEvent 158 188 0 0 0 0\n\
    MouseMoveEvent 160 188 0 0 0 0\n\
    RenderEvent 160 188 0 0 0 0\n\
    MouseMoveEvent 161 187 0 0 0 0\n\
    RenderEvent 161 187 0 0 0 0\n\
    MouseMoveEvent 162 187 0 0 0 0\n\
    RenderEvent 162 187 0 0 0 0\n\
    MouseMoveEvent 164 185 0 0 0 0\n\
    RenderEvent 164 185 0 0 0 0\n\
    MouseMoveEvent 166 185 0 0 0 0\n\
    RenderEvent 166 185 0 0 0 0\n\
    MouseMoveEvent 167 184 0 0 0 0\n\
    RenderEvent 167 184 0 0 0 0\n\
    MouseMoveEvent 170 181 0 0 0 0\n\
    RenderEvent 170 181 0 0 0 0\n\
    MouseMoveEvent 171 180 0 0 0 0\n\
    RenderEvent 171 180 0 0 0 0\n\
    MouseMoveEvent 173 178 0 0 0 0\n\
    RenderEvent 173 178 0 0 0 0\n\
    MouseMoveEvent 174 176 0 0 0 0\n\
    RenderEvent 174 176 0 0 0 0\n\
    MouseMoveEvent 175 175 0 0 0 0\n\
    RenderEvent 175 175 0 0 0 0\n\
    MouseMoveEvent 175 173 0 0 0 0\n\
    RenderEvent 175 173 0 0 0 0\n\
    MouseMoveEvent 177 171 0 0 0 0\n\
    RenderEvent 177 171 0 0 0 0\n\
    MouseMoveEvent 178 169 0 0 0 0\n\
    RenderEvent 178 169 0 0 0 0\n\
    MouseMoveEvent 179 167 0 0 0 0\n\
    RenderEvent 179 167 0 0 0 0\n\
    MouseMoveEvent 180 164 0 0 0 0\n\
    RenderEvent 180 164 0 0 0 0\n\
    MouseMoveEvent 181 164 0 0 0 0\n\
    RenderEvent 181 164 0 0 0 0\n\
    MouseMoveEvent 182 162 0 0 0 0\n\
    RenderEvent 182 162 0 0 0 0\n\
    MouseMoveEvent 183 160 0 0 0 0\n\
    RenderEvent 183 160 0 0 0 0\n\
    MouseMoveEvent 184 159 0 0 0 0\n\
    RenderEvent 184 159 0 0 0 0\n\
    MouseMoveEvent 184 158 0 0 0 0\n\
    RenderEvent 184 158 0 0 0 0\n\
    MouseMoveEvent 186 158 0 0 0 0\n\
    RenderEvent 186 158 0 0 0 0\n\
    MouseMoveEvent 187 157 0 0 0 0\n\
    RenderEvent 187 157 0 0 0 0\n\
    MouseMoveEvent 190 157 0 0 0 0\n\
    RenderEvent 190 157 0 0 0 0\n\
    MouseMoveEvent 191 157 0 0 0 0\n\
    RenderEvent 191 157 0 0 0 0\n\
    MouseMoveEvent 192 157 0 0 0 0\n\
    RenderEvent 192 157 0 0 0 0\n\
    MouseMoveEvent 193 157 0 0 0 0\n\
    RenderEvent 193 157 0 0 0 0\n\
    MouseMoveEvent 193 157 0 0 0 0\n\
    RenderEvent 193 157 0 0 0 0\n\
    MouseMoveEvent 194 157 0 0 0 0\n\
    RenderEvent 194 157 0 0 0 0\n\
    MouseMoveEvent 195 157 0 0 0 0\n\
    RenderEvent 195 157 0 0 0 0\n\
    MouseMoveEvent 198 157 0 0 0 0\n\
    RenderEvent 198 157 0 0 0 0\n\
    MouseMoveEvent 199 157 0 0 0 0\n\
    RenderEvent 199 157 0 0 0 0\n\
    MouseMoveEvent 200 157 0 0 0 0\n\
    RenderEvent 200 157 0 0 0 0\n\
    MouseMoveEvent 201 157 0 0 0 0\n\
    RenderEvent 201 157 0 0 0 0\n\
    MouseMoveEvent 201 157 0 0 0 0\n\
    RenderEvent 201 157 0 0 0 0\n\
    MouseMoveEvent 202 157 0 0 0 0\n\
    RenderEvent 202 157 0 0 0 0\n\
    MouseMoveEvent 203 157 0 0 0 0\n\
    RenderEvent 203 157 0 0 0 0\n\
    MouseMoveEvent 204 157 0 0 0 0\n\
    RenderEvent 204 157 0 0 0 0\n\
    MouseMoveEvent 206 159 0 0 0 0\n\
    RenderEvent 206 159 0 0 0 0\n\
    MouseMoveEvent 208 161 0 0 0 0\n\
    RenderEvent 208 161 0 0 0 0\n\
    MouseMoveEvent 209 163 0 0 0 0\n\
    RenderEvent 209 163 0 0 0 0\n\
    MouseMoveEvent 210 164 0 0 0 0\n\
    RenderEvent 210 164 0 0 0 0\n\
    MouseMoveEvent 211 166 0 0 0 0\n\
    RenderEvent 211 166 0 0 0 0\n\
    MouseMoveEvent 212 168 0 0 0 0\n\
    RenderEvent 212 168 0 0 0 0\n\
    MouseMoveEvent 213 170 0 0 0 0\n\
    RenderEvent 213 170 0 0 0 0\n\
    MouseMoveEvent 214 172 0 0 0 0\n\
    RenderEvent 214 172 0 0 0 0\n\
    MouseMoveEvent 214 174 0 0 0 0\n\
    RenderEvent 214 174 0 0 0 0\n\
    MouseMoveEvent 214 176 0 0 0 0\n\
    RenderEvent 214 176 0 0 0 0\n\
    MouseMoveEvent 214 178 0 0 0 0\n\
    RenderEvent 214 178 0 0 0 0\n\
    MouseMoveEvent 214 180 0 0 0 0\n\
    RenderEvent 214 180 0 0 0 0\n\
    MouseMoveEvent 214 181 0 0 0 0\n\
    RenderEvent 214 181 0 0 0 0\n\
    MouseMoveEvent 214 182 0 0 0 0\n\
    RenderEvent 214 182 0 0 0 0\n\
    MouseMoveEvent 214 183 0 0 0 0\n\
    RenderEvent 214 183 0 0 0 0\n\
    MouseMoveEvent 214 184 0 0 0 0\n\
    RenderEvent 214 184 0 0 0 0\n\
    MouseMoveEvent 214 185 0 0 0 0\n\
    RenderEvent 214 185 0 0 0 0\n\
    MouseMoveEvent 213 186 0 0 0 0\n\
    RenderEvent 213 186 0 0 0 0\n\
    MouseMoveEvent 213 187 0 0 0 0\n\
    RenderEvent 213 187 0 0 0 0\n\
    MouseMoveEvent 213 188 0 0 0 0\n\
    RenderEvent 213 188 0 0 0 0\n\
    MouseMoveEvent 213 190 0 0 0 0\n\
    RenderEvent 213 190 0 0 0 0\n\
    MouseMoveEvent 212 190 0 0 0 0\n\
    RenderEvent 212 190 0 0 0 0\n\
    MouseMoveEvent 212 192 0 0 0 0\n\
    RenderEvent 212 192 0 0 0 0\n\
    MouseMoveEvent 212 193 0 0 0 0\n\
    RenderEvent 212 193 0 0 0 0\n\
    MouseMoveEvent 212 194 0 0 0 0\n\
    RenderEvent 212 194 0 0 0 0\n\
    MouseMoveEvent 212 195 0 0 0 0\n\
    RenderEvent 212 195 0 0 0 0\n\
    MouseMoveEvent 211 196 0 0 0 0\n\
    RenderEvent 211 196 0 0 0 0\n\
    MouseMoveEvent 210 197 0 0 0 0\n\
    RenderEvent 210 197 0 0 0 0\n\
    MouseMoveEvent 210 198 0 0 0 0\n\
    RenderEvent 210 198 0 0 0 0\n\
    MouseMoveEvent 210 200 0 0 0 0\n\
    RenderEvent 210 200 0 0 0 0\n\
    MouseMoveEvent 209 200 0 0 0 0\n\
    RenderEvent 209 200 0 0 0 0\n\
    MouseMoveEvent 209 200 0 0 0 0\n\
    RenderEvent 209 200 0 0 0 0\n\
    MouseMoveEvent 208 200 0 0 0 0\n\
    RenderEvent 208 200 0 0 0 0\n\
    MouseMoveEvent 206 201 0 0 0 0\n\
    RenderEvent 206 201 0 0 0 0\n\
    MouseMoveEvent 205 202 0 0 0 0\n\
    RenderEvent 205 202 0 0 0 0\n\
    MouseMoveEvent 204 202 0 0 0 0\n\
    RenderEvent 204 202 0 0 0 0\n\
    MouseMoveEvent 202 204 0 0 0 0\n\
    RenderEvent 202 204 0 0 0 0\n\
    MouseMoveEvent 201 204 0 0 0 0\n\
    RenderEvent 201 204 0 0 0 0\n\
    MouseMoveEvent 200 205 0 0 0 0\n\
    RenderEvent 200 205 0 0 0 0\n\
    MouseMoveEvent 199 206 0 0 0 0\n\
    RenderEvent 199 206 0 0 0 0\n\
    MouseMoveEvent 198 206 0 0 0 0\n\
    RenderEvent 198 206 0 0 0 0\n\
    MouseMoveEvent 198 207 0 0 0 0\n\
    RenderEvent 198 207 0 0 0 0\n\
    MouseMoveEvent 197 207 0 0 0 0\n\
    RenderEvent 197 207 0 0 0 0\n\
    MouseMoveEvent 196 207 0 0 0 0\n\
    RenderEvent 196 207 0 0 0 0\n\
    MouseMoveEvent 195 209 0 0 0 0\n\
    RenderEvent 195 209 0 0 0 0\n\
    MouseMoveEvent 193 210 0 0 0 0\n\
    RenderEvent 193 210 0 0 0 0\n\
    MouseMoveEvent 192 211 0 0 0 0\n\
    RenderEvent 192 211 0 0 0 0\n\
    MouseMoveEvent 191 212 0 0 0 0\n\
    RenderEvent 191 212 0 0 0 0\n\
    MouseMoveEvent 190 213 0 0 0 0\n\
    RenderEvent 190 213 0 0 0 0\n\
    LeftButtonReleaseEvent 190 213 0 0 0 0\n\
    RenderEvent 190 213 0 0 0 0\n\
    "

# Create some synthetic data
#
# Create a synthetic source: sample a sphere across a volume
sphere = vtkSphere()
sphere.SetCenter( 0.0,0.0,0.0)
sphere.SetRadius(0.25)

res = 200
sample = vtkSampleFunction()
sample.SetImplicitFunction(sphere)
sample.SetModelBounds(-0.5,0.5, -0.5,0.5, -0.5,0.5)
sample.SetSampleDimensions(res,res,res)
sample.SetOutputScalarTypeToFloat()
sample.Update()

# The cut plane
plane = vtkPlane()
plane.SetOrigin(0,0,0)
plane.SetNormal(1,1,1)

# Create the RenderWindow, Renderer and both Actors
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren)
iRen = vtkRenderWindowInteractor()
iRen.SetRenderWindow(renWin)

# Create the widget, its representation, and callback
def MovePlane(widget, event_string):
    rep.GetPlane(plane)

rep = vtkImplicitImageRepresentation()
rep.SetPlaceFactor(1.0);
rep.PlaceImage(sample.GetOutputPort())
rep.SetPlane(plane)

planeWidget = vtkImplicitPlaneWidget2()
planeWidget.SetInteractor(iRen)
planeWidget.SetRepresentation(rep);
planeWidget.AddObserver("InteractionEvent",MovePlane);

recorder = vtkInteractorEventRecorder()
recorder.SetInteractor(iRen)
#recorder.SetFileName("record.log")
#recorder.On()
#recorder.Record()
recorder.ReadFromInputStringOn()
recorder.SetInputString(Recording)

# Add the actors to the renderer, set the background and size
#
ren.SetBackground(1, 1, 1)
renWin.SetSize(300, 300)
ren.SetBackground(0.1, 0.2, 0.4)
planeWidget.On()
ren.ResetCamera()

iRen.Initialize()
renWin.Render()

# Actually cut the data
recorder.Play()
renWin.Render()
iRen.Start()
