#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Test the vtkMagnifierWidget and vtkMagnifierRepresentation classes

from vtkmodules.vtkFiltersCore import (
    vtkFeatureEdges,
    vtkGlyph3D,
)
from vtkmodules.vtkFiltersSources import (
    vtkConeSource,
    vtkSphereSource,
)
from vtkmodules.vtkInteractionWidgets import (
    vtkMagnifierRepresentation,
    vtkMagnifierWidget,
)
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkInteractorEventRecorder,
    vtkPolyDataMapper,
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

# Control test resolution
res = 32

# These are the pre-recorded events
Recording = \
    "# StreamVersion 1.1\n\
    MouseMoveEvent 146 150 0 0 0 0\n\
    KeyPressEvent 146 150 0 116 1 t\n\
    CharEvent 146 150 0 116 1 t\n\
    KeyReleaseEvent 146 150 0 116 1 t\n\
    LeftButtonPressEvent 146 150 0 0 0 t\n\
    StartInteractionEvent 146 150 0 0 0 t\n\
    MouseMoveEvent 147 149 0 0 0 t\n\
    RenderEvent 147 149 0 0 0 t\n\
    InteractionEvent 147 149 0 0 0 t\n\
    MouseMoveEvent 148 148 0 0 0 t\n\
    RenderEvent 148 148 0 0 0 t\n\
    InteractionEvent 148 148 0 0 0 t\n\
    MouseMoveEvent 149 148 0 0 0 t\n\
    RenderEvent 149 148 0 0 0 t\n\
    InteractionEvent 149 148 0 0 0 t\n\
    MouseMoveEvent 149 148 0 0 0 t\n\
    RenderEvent 149 148 0 0 0 t\n\
    InteractionEvent 149 148 0 0 0 t\n\
    MouseMoveEvent 149 147 0 0 0 t\n\
    RenderEvent 149 147 0 0 0 t\n\
    InteractionEvent 149 147 0 0 0 t\n\
    MouseMoveEvent 150 147 0 0 0 t\n\
    RenderEvent 150 147 0 0 0 t\n\
    InteractionEvent 150 147 0 0 0 t\n\
    MouseMoveEvent 150 146 0 0 0 t\n\
    RenderEvent 150 146 0 0 0 t\n\
    InteractionEvent 150 146 0 0 0 t\n\
    MouseMoveEvent 151 145 0 0 0 t\n\
    RenderEvent 151 145 0 0 0 t\n\
    InteractionEvent 151 145 0 0 0 t\n\
    MouseMoveEvent 152 143 0 0 0 t\n\
    RenderEvent 152 143 0 0 0 t\n\
    InteractionEvent 152 143 0 0 0 t\n\
    MouseMoveEvent 153 142 0 0 0 t\n\
    RenderEvent 153 142 0 0 0 t\n\
    InteractionEvent 153 142 0 0 0 t\n\
    MouseMoveEvent 154 141 0 0 0 t\n\
    RenderEvent 154 141 0 0 0 t\n\
    InteractionEvent 154 141 0 0 0 t\n\
    MouseMoveEvent 154 140 0 0 0 t\n\
    RenderEvent 154 140 0 0 0 t\n\
    InteractionEvent 154 140 0 0 0 t\n\
    MouseMoveEvent 155 139 0 0 0 t\n\
    RenderEvent 155 139 0 0 0 t\n\
    InteractionEvent 155 139 0 0 0 t\n\
    MouseMoveEvent 155 138 0 0 0 t\n\
    RenderEvent 155 138 0 0 0 t\n\
    InteractionEvent 155 138 0 0 0 t\n\
    MouseMoveEvent 156 137 0 0 0 t\n\
    RenderEvent 156 137 0 0 0 t\n\
    InteractionEvent 156 137 0 0 0 t\n\
    MouseMoveEvent 156 136 0 0 0 t\n\
    RenderEvent 156 136 0 0 0 t\n\
    InteractionEvent 156 136 0 0 0 t\n\
    MouseMoveEvent 156 134 0 0 0 t\n\
    RenderEvent 156 134 0 0 0 t\n\
    InteractionEvent 156 134 0 0 0 t\n\
    MouseMoveEvent 157 132 0 0 0 t\n\
    RenderEvent 157 132 0 0 0 t\n\
    InteractionEvent 157 132 0 0 0 t\n\
    MouseMoveEvent 157 131 0 0 0 t\n\
    RenderEvent 157 131 0 0 0 t\n\
    InteractionEvent 157 131 0 0 0 t\n\
    MouseMoveEvent 158 130 0 0 0 t\n\
    RenderEvent 158 130 0 0 0 t\n\
    InteractionEvent 158 130 0 0 0 t\n\
    MouseMoveEvent 158 129 0 0 0 t\n\
    RenderEvent 158 129 0 0 0 t\n\
    InteractionEvent 158 129 0 0 0 t\n\
    MouseMoveEvent 159 127 0 0 0 t\n\
    RenderEvent 159 127 0 0 0 t\n\
    InteractionEvent 159 127 0 0 0 t\n\
    MouseMoveEvent 159 126 0 0 0 t\n\
    RenderEvent 159 126 0 0 0 t\n\
    InteractionEvent 159 126 0 0 0 t\n\
    MouseMoveEvent 159 125 0 0 0 t\n\
    RenderEvent 159 125 0 0 0 t\n\
    InteractionEvent 159 125 0 0 0 t\n\
    MouseMoveEvent 159 124 0 0 0 t\n\
    RenderEvent 159 124 0 0 0 t\n\
    InteractionEvent 159 124 0 0 0 t\n\
    MouseMoveEvent 160 124 0 0 0 t\n\
    RenderEvent 160 124 0 0 0 t\n\
    InteractionEvent 160 124 0 0 0 t\n\
    MouseMoveEvent 161 123 0 0 0 t\n\
    RenderEvent 161 123 0 0 0 t\n\
    InteractionEvent 161 123 0 0 0 t\n\
    MouseMoveEvent 162 122 0 0 0 t\n\
    RenderEvent 162 122 0 0 0 t\n\
    InteractionEvent 162 122 0 0 0 t\n\
    MouseMoveEvent 163 122 0 0 0 t\n\
    RenderEvent 163 122 0 0 0 t\n\
    InteractionEvent 163 122 0 0 0 t\n\
    MouseMoveEvent 164 121 0 0 0 t\n\
    RenderEvent 164 121 0 0 0 t\n\
    InteractionEvent 164 121 0 0 0 t\n\
    MouseMoveEvent 168 119 0 0 0 t\n\
    RenderEvent 168 119 0 0 0 t\n\
    InteractionEvent 168 119 0 0 0 t\n\
    MouseMoveEvent 172 119 0 0 0 t\n\
    RenderEvent 172 119 0 0 0 t\n\
    InteractionEvent 172 119 0 0 0 t\n\
    MouseMoveEvent 174 119 0 0 0 t\n\
    RenderEvent 174 119 0 0 0 t\n\
    InteractionEvent 174 119 0 0 0 t\n\
    MouseMoveEvent 177 118 0 0 0 t\n\
    RenderEvent 177 118 0 0 0 t\n\
    InteractionEvent 177 118 0 0 0 t\n\
    MouseMoveEvent 178 118 0 0 0 t\n\
    RenderEvent 178 118 0 0 0 t\n\
    InteractionEvent 178 118 0 0 0 t\n\
    MouseMoveEvent 179 118 0 0 0 t\n\
    RenderEvent 179 118 0 0 0 t\n\
    InteractionEvent 179 118 0 0 0 t\n\
    MouseMoveEvent 179 119 0 0 0 t\n\
    RenderEvent 179 119 0 0 0 t\n\
    InteractionEvent 179 119 0 0 0 t\n\
    MouseMoveEvent 179 120 0 0 0 t\n\
    RenderEvent 179 120 0 0 0 t\n\
    InteractionEvent 179 120 0 0 0 t\n\
    MouseMoveEvent 180 120 0 0 0 t\n\
    RenderEvent 180 120 0 0 0 t\n\
    InteractionEvent 180 120 0 0 0 t\n\
    MouseMoveEvent 181 122 0 0 0 t\n\
    RenderEvent 181 122 0 0 0 t\n\
    InteractionEvent 181 122 0 0 0 t\n\
    MouseMoveEvent 182 123 0 0 0 t\n\
    RenderEvent 182 123 0 0 0 t\n\
    InteractionEvent 182 123 0 0 0 t\n\
    MouseMoveEvent 184 125 0 0 0 t\n\
    RenderEvent 184 125 0 0 0 t\n\
    InteractionEvent 184 125 0 0 0 t\n\
    MouseMoveEvent 186 129 0 0 0 t\n\
    RenderEvent 186 129 0 0 0 t\n\
    InteractionEvent 186 129 0 0 0 t\n\
    MouseMoveEvent 187 129 0 0 0 t\n\
    RenderEvent 187 129 0 0 0 t\n\
    InteractionEvent 187 129 0 0 0 t\n\
    MouseMoveEvent 191 132 0 0 0 t\n\
    RenderEvent 191 132 0 0 0 t\n\
    InteractionEvent 191 132 0 0 0 t\n\
    MouseMoveEvent 192 133 0 0 0 t\n\
    RenderEvent 192 133 0 0 0 t\n\
    InteractionEvent 192 133 0 0 0 t\n\
    MouseMoveEvent 194 134 0 0 0 t\n\
    RenderEvent 194 134 0 0 0 t\n\
    InteractionEvent 194 134 0 0 0 t\n\
    MouseMoveEvent 196 135 0 0 0 t\n\
    RenderEvent 196 135 0 0 0 t\n\
    InteractionEvent 196 135 0 0 0 t\n\
    MouseMoveEvent 198 135 0 0 0 t\n\
    RenderEvent 198 135 0 0 0 t\n\
    InteractionEvent 198 135 0 0 0 t\n\
    MouseMoveEvent 199 135 0 0 0 t\n\
    RenderEvent 199 135 0 0 0 t\n\
    InteractionEvent 199 135 0 0 0 t\n\
    MouseMoveEvent 200 134 0 0 0 t\n\
    RenderEvent 200 134 0 0 0 t\n\
    InteractionEvent 200 134 0 0 0 t\n\
    MouseMoveEvent 202 133 0 0 0 t\n\
    RenderEvent 202 133 0 0 0 t\n\
    InteractionEvent 202 133 0 0 0 t\n\
    MouseMoveEvent 203 132 0 0 0 t\n\
    RenderEvent 203 132 0 0 0 t\n\
    InteractionEvent 203 132 0 0 0 t\n\
    MouseMoveEvent 204 132 0 0 0 t\n\
    RenderEvent 204 132 0 0 0 t\n\
    InteractionEvent 204 132 0 0 0 t\n\
    MouseMoveEvent 205 131 0 0 0 t\n\
    RenderEvent 205 131 0 0 0 t\n\
    InteractionEvent 205 131 0 0 0 t\n\
    MouseMoveEvent 206 131 0 0 0 t\n\
    RenderEvent 206 131 0 0 0 t\n\
    InteractionEvent 206 131 0 0 0 t\n\
    MouseMoveEvent 208 130 0 0 0 t\n\
    RenderEvent 208 130 0 0 0 t\n\
    InteractionEvent 208 130 0 0 0 t\n\
    MouseMoveEvent 210 129 0 0 0 t\n\
    RenderEvent 210 129 0 0 0 t\n\
    InteractionEvent 210 129 0 0 0 t\n\
    MouseMoveEvent 211 129 0 0 0 t\n\
    RenderEvent 211 129 0 0 0 t\n\
    InteractionEvent 211 129 0 0 0 t\n\
    MouseMoveEvent 212 128 0 0 0 t\n\
    RenderEvent 212 128 0 0 0 t\n\
    InteractionEvent 212 128 0 0 0 t\n\
    MouseMoveEvent 213 128 0 0 0 t\n\
    RenderEvent 213 128 0 0 0 t\n\
    InteractionEvent 213 128 0 0 0 t\n\
    MouseMoveEvent 215 127 0 0 0 t\n\
    RenderEvent 215 127 0 0 0 t\n\
    InteractionEvent 215 127 0 0 0 t\n\
    MouseMoveEvent 215 126 0 0 0 t\n\
    RenderEvent 215 126 0 0 0 t\n\
    InteractionEvent 215 126 0 0 0 t\n\
    MouseMoveEvent 217 125 0 0 0 t\n\
    RenderEvent 217 125 0 0 0 t\n\
    InteractionEvent 217 125 0 0 0 t\n\
    MouseMoveEvent 217 124 0 0 0 t\n\
    RenderEvent 217 124 0 0 0 t\n\
    InteractionEvent 217 124 0 0 0 t\n\
    MouseMoveEvent 220 122 0 0 0 t\n\
    RenderEvent 220 122 0 0 0 t\n\
    InteractionEvent 220 122 0 0 0 t\n\
    MouseMoveEvent 223 120 0 0 0 t\n\
    RenderEvent 223 120 0 0 0 t\n\
    InteractionEvent 223 120 0 0 0 t\n\
    MouseMoveEvent 226 118 0 0 0 t\n\
    RenderEvent 226 118 0 0 0 t\n\
    InteractionEvent 226 118 0 0 0 t\n\
    MouseMoveEvent 228 118 0 0 0 t\n\
    RenderEvent 228 118 0 0 0 t\n\
    InteractionEvent 228 118 0 0 0 t\n\
    MouseMoveEvent 229 116 0 0 0 t\n\
    RenderEvent 229 116 0 0 0 t\n\
    InteractionEvent 229 116 0 0 0 t\n\
    MouseMoveEvent 230 117 0 0 0 t\n\
    RenderEvent 230 117 0 0 0 t\n\
    InteractionEvent 230 117 0 0 0 t\n\
    MouseMoveEvent 230 117 0 0 0 t\n\
    RenderEvent 230 117 0 0 0 t\n\
    InteractionEvent 230 117 0 0 0 t\n\
    MouseMoveEvent 231 118 0 0 0 t\n\
    RenderEvent 231 118 0 0 0 t\n\
    InteractionEvent 231 118 0 0 0 t\n\
    MouseMoveEvent 232 119 0 0 0 t\n\
    RenderEvent 232 119 0 0 0 t\n\
    InteractionEvent 232 119 0 0 0 t\n\
    MouseMoveEvent 234 120 0 0 0 t\n\
    RenderEvent 234 120 0 0 0 t\n\
    InteractionEvent 234 120 0 0 0 t\n\
    MouseMoveEvent 235 122 0 0 0 t\n\
    RenderEvent 235 122 0 0 0 t\n\
    InteractionEvent 235 122 0 0 0 t\n\
    MouseMoveEvent 237 124 0 0 0 t\n\
    RenderEvent 237 124 0 0 0 t\n\
    InteractionEvent 237 124 0 0 0 t\n\
    MouseMoveEvent 239 126 0 0 0 t\n\
    RenderEvent 239 126 0 0 0 t\n\
    InteractionEvent 239 126 0 0 0 t\n\
    MouseMoveEvent 240 127 0 0 0 t\n\
    RenderEvent 240 127 0 0 0 t\n\
    InteractionEvent 240 127 0 0 0 t\n\
    MouseMoveEvent 240 128 0 0 0 t\n\
    RenderEvent 240 128 0 0 0 t\n\
    InteractionEvent 240 128 0 0 0 t\n\
    MouseMoveEvent 241 129 0 0 0 t\n\
    RenderEvent 241 129 0 0 0 t\n\
    InteractionEvent 241 129 0 0 0 t\n\
    MouseMoveEvent 241 131 0 0 0 t\n\
    RenderEvent 241 131 0 0 0 t\n\
    InteractionEvent 241 131 0 0 0 t\n\
    MouseMoveEvent 241 131 0 0 0 t\n\
    RenderEvent 241 131 0 0 0 t\n\
    InteractionEvent 241 131 0 0 0 t\n\
    MouseMoveEvent 242 132 0 0 0 t\n\
    RenderEvent 242 132 0 0 0 t\n\
    InteractionEvent 242 132 0 0 0 t\n\
    MouseMoveEvent 242 133 0 0 0 t\n\
    RenderEvent 242 133 0 0 0 t\n\
    InteractionEvent 242 133 0 0 0 t\n\
    MouseMoveEvent 242 134 0 0 0 t\n\
    RenderEvent 242 134 0 0 0 t\n\
    InteractionEvent 242 134 0 0 0 t\n\
    MouseMoveEvent 242 135 0 0 0 t\n\
    RenderEvent 242 135 0 0 0 t\n\
    InteractionEvent 242 135 0 0 0 t\n\
    MouseMoveEvent 242 136 0 0 0 t\n\
    RenderEvent 242 136 0 0 0 t\n\
    InteractionEvent 242 136 0 0 0 t\n\
    MouseMoveEvent 242 137 0 0 0 t\n\
    RenderEvent 242 137 0 0 0 t\n\
    InteractionEvent 242 137 0 0 0 t\n\
    MouseMoveEvent 242 138 0 0 0 t\n\
    RenderEvent 242 138 0 0 0 t\n\
    InteractionEvent 242 138 0 0 0 t\n\
    MouseMoveEvent 243 139 0 0 0 t\n\
    RenderEvent 243 139 0 0 0 t\n\
    InteractionEvent 243 139 0 0 0 t\n\
    MouseMoveEvent 244 139 0 0 0 t\n\
    RenderEvent 244 139 0 0 0 t\n\
    InteractionEvent 244 139 0 0 0 t\n\
    MouseMoveEvent 245 139 0 0 0 t\n\
    RenderEvent 245 139 0 0 0 t\n\
    InteractionEvent 245 139 0 0 0 t\n\
    MouseMoveEvent 246 139 0 0 0 t\n\
    RenderEvent 246 139 0 0 0 t\n\
    InteractionEvent 246 139 0 0 0 t\n\
    MouseMoveEvent 247 139 0 0 0 t\n\
    RenderEvent 247 139 0 0 0 t\n\
    InteractionEvent 247 139 0 0 0 t\n\
    MouseMoveEvent 248 138 0 0 0 t\n\
    RenderEvent 248 138 0 0 0 t\n\
    InteractionEvent 248 138 0 0 0 t\n\
    MouseMoveEvent 250 137 0 0 0 t\n\
    RenderEvent 250 137 0 0 0 t\n\
    InteractionEvent 250 137 0 0 0 t\n\
    MouseMoveEvent 253 136 0 0 0 t\n\
    RenderEvent 253 136 0 0 0 t\n\
    InteractionEvent 253 136 0 0 0 t\n\
    MouseMoveEvent 255 134 0 0 0 t\n\
    RenderEvent 255 134 0 0 0 t\n\
    InteractionEvent 255 134 0 0 0 t\n\
    MouseMoveEvent 256 133 0 0 0 t\n\
    RenderEvent 256 133 0 0 0 t\n\
    InteractionEvent 256 133 0 0 0 t\n\
    LeftButtonReleaseEvent 256 133 0 0 0 t\n\
    EndInteractionEvent 256 133 0 0 0 t\n\
    RenderEvent 256 133 0 0 0 t\n\
    MouseMoveEvent 255 132 0 0 0 t\n\
    MouseMoveEvent 254 131 0 0 0 t\n\
    MouseMoveEvent 253 131 0 0 0 t\n\
    MouseMoveEvent 251 129 0 0 0 t\n\
    MouseMoveEvent 249 128 0 0 0 t\n\
    MouseMoveEvent 247 127 0 0 0 t\n\
    MouseMoveEvent 246 126 0 0 0 t\n\
    MouseMoveEvent 242 125 0 0 0 t\n\
    MouseMoveEvent 240 124 0 0 0 t\n\
    MouseMoveEvent 238 124 0 0 0 t\n\
    MouseMoveEvent 236 123 0 0 0 t\n\
    MouseMoveEvent 233 123 0 0 0 t\n\
    MouseMoveEvent 232 123 0 0 0 t\n\
    MouseMoveEvent 231 123 0 0 0 t\n\
    MouseMoveEvent 230 123 0 0 0 t\n\
    MouseMoveEvent 229 122 0 0 0 t\n\
    MouseMoveEvent 228 122 0 0 0 t\n\
    MouseMoveEvent 227 122 0 0 0 t\n\
    MouseMoveEvent 226 122 0 0 0 t\n\
    MouseMoveEvent 225 122 0 0 0 t\n\
    MouseMoveEvent 224 122 0 0 0 t\n\
    MouseMoveEvent 223 122 0 0 0 t\n\
    MouseMoveEvent 221 122 0 0 0 t\n\
    MouseMoveEvent 220 122 0 0 0 t\n\
    MouseMoveEvent 219 123 0 0 0 t\n\
    MouseMoveEvent 217 123 0 0 0 t\n\
    MouseMoveEvent 215 123 0 0 0 t\n\
    MouseMoveEvent 214 123 0 0 0 t\n\
    MouseMoveEvent 213 124 0 0 0 t\n\
    MouseMoveEvent 212 124 0 0 0 t\n\
    MouseMoveEvent 211 124 0 0 0 t\n\
    MouseMoveEvent 210 125 0 0 0 t\n\
    MouseMoveEvent 209 125 0 0 0 t\n\
    MouseMoveEvent 207 125 0 0 0 t\n\
    MouseMoveEvent 206 126 0 0 0 t\n\
    MouseMoveEvent 205 126 0 0 0 t\n\
    MouseMoveEvent 204 127 0 0 0 t\n\
    MouseMoveEvent 204 128 0 0 0 t\n\
    MouseMoveEvent 203 129 0 0 0 t\n\
    KeyPressEvent 203 129 0 109 1 m\n\
    CharEvent 203 129 0 109 1 m\n\
    RenderEvent 203 129 0 109 1 m\n\
    KeyReleaseEvent 203 129 0 109 1 m\n\
    MouseMoveEvent 203 129 0 0 0 m\n\
    RenderEvent 203 129 0 0 0 m\n\
    MouseMoveEvent 202 129 0 0 0 m\n\
    RenderEvent 202 129 0 0 0 m\n\
    MouseMoveEvent 201 128 0 0 0 m\n\
    RenderEvent 201 128 0 0 0 m\n\
    MouseMoveEvent 200 128 0 0 0 m\n\
    RenderEvent 200 128 0 0 0 m\n\
    MouseMoveEvent 199 128 0 0 0 m\n\
    RenderEvent 199 128 0 0 0 m\n\
    MouseMoveEvent 199 127 0 0 0 m\n\
    RenderEvent 199 127 0 0 0 m\n\
    MouseMoveEvent 199 126 0 0 0 m\n\
    RenderEvent 199 126 0 0 0 m\n\
    MouseMoveEvent 199 127 0 0 0 m\n\
    RenderEvent 199 127 0 0 0 m\n\
    MouseMoveEvent 199 127 0 0 0 m\n\
    RenderEvent 199 127 0 0 0 m\n\
    MouseMoveEvent 199 128 0 0 0 m\n\
    RenderEvent 199 128 0 0 0 m\n\
    MouseMoveEvent 199 129 0 0 0 m\n\
    RenderEvent 199 129 0 0 0 m\n\
    MouseMoveEvent 199 130 0 0 0 m\n\
    RenderEvent 199 130 0 0 0 m\n\
    MouseMoveEvent 199 131 0 0 0 m\n\
    RenderEvent 199 131 0 0 0 m\n\
    KeyPressEvent 199 131 0 0 1 Shift_L\n\
    CharEvent 199 131 0 0 1 Shift_L\n\
    KeyPressEvent 199 131 1 43 1 plus\n\
    CharEvent 199 131 1 43 1 plus\n\
    RenderEvent 199 131 1 43 1 plus\n\
    KeyReleaseEvent 199 131 1 43 1 plus\n\
    KeyPressEvent 199 131 1 43 1 plus\n\
    CharEvent 199 131 1 43 1 plus\n\
    RenderEvent 199 131 1 43 1 plus\n\
    KeyReleaseEvent 199 131 1 43 1 plus\n\
    KeyReleaseEvent 199 131 1 0 1 Shift_L\n\
    KeyPressEvent 199 131 0 45 1 minus\n\
    CharEvent 199 131 0 45 1 minus\n\
    RenderEvent 199 131 0 45 1 minus\n\
    KeyReleaseEvent 199 131 0 45 1 minus\n\
    KeyPressEvent 199 131 0 45 1 minus\n\
    CharEvent 199 131 0 45 1 minus\n\
    RenderEvent 199 131 0 45 1 minus\n\
    KeyReleaseEvent 199 131 0 45 1 minus\n\
    KeyPressEvent 199 131 0 45 1 minus\n\
    CharEvent 199 131 0 45 1 minus\n\
    RenderEvent 199 131 0 45 1 minus\n\
    KeyReleaseEvent 199 131 0 45 1 minus\n\
"

# Create a simple geometry: a mace
sphere = vtkSphereSource()
sphere.SetThetaResolution(res)
sphere.SetPhiResolution(int(res/2))

cone = vtkConeSource()
cone.SetResolution(int(res/4))

normals = vtkGlyph3D()
normals.SetInputConnection(sphere.GetOutputPort())
normals.SetSourceConnection(cone.GetOutputPort())
normals.SetVectorModeToUseNormal()
normals.SetScaleFactor(0.1)

sphereMapper = vtkPolyDataMapper()
sphereMapper.SetInputConnection(sphere.GetOutputPort())

spikeMapper = vtkPolyDataMapper()
spikeMapper.SetInputConnection(normals.GetOutputPort())

sphereActor = vtkActor()
sphereActor.SetMapper(sphereMapper)

spikeActor = vtkActor()
spikeActor.SetMapper(spikeMapper)

# Special effect to see edges
edges = vtkFeatureEdges()
edges.SetInputConnection(sphere.GetOutputPort())
edges.ExtractAllEdgeTypesOff()
edges.ManifoldEdgesOn()

edgeMapper = vtkPolyDataMapper()
edgeMapper.SetInputConnection(edges.GetOutputPort())
edgeMapper.ScalarVisibilityOff()

edgeActor = vtkActor()
edgeActor.SetMapper(edgeMapper)
edgeActor.GetProperty().SetColor(1,0,0)

# Create the RenderWindow, Renderer and both Actors
#
ren0 = vtkRenderer()
ren0.SetViewport(0,0,0.5,1.0)
ren1 = vtkRenderer()
ren1.SetViewport(0.5,0,1.0,1.0)
renWin = vtkRenderWindow()
renWin.AddRenderer(ren0)
renWin.AddRenderer(ren1)

iRen = vtkRenderWindowInteractor()
iRen.SetRenderWindow(renWin)

# Observe events for magnification factor change
def ChangeMag(widget, event_string):
    if iRen.GetKeyCode() == '+' :
        magF = magRep.GetMagnificationFactor() + 1
    else:
        magF = magRep.GetMagnificationFactor() - 1
    magRep.SetMagnificationFactor(magF)

# Add the magnifier widget
magRep = vtkMagnifierRepresentation()
magRep.SetRenderer(ren0)
magRep.GetMagnificationRenderer().SetBackground(0.8,0.8,0.8)
magRep.BorderOn()
magRep.GetBorderProperty().SetColor(0,1,0)
magRep.AddViewProp(sphereActor)
magRep.AddViewProp(edgeActor)

magW = vtkMagnifierWidget()
magW.SetInteractor(iRen)
magW.SetRepresentation(magRep)
magW.AddObserver("WidgetValueChangedEvent",ChangeMag)

# Handle playback of events
recorder = vtkInteractorEventRecorder()
recorder.SetInteractor(iRen)
#recorder.SetFileName("record.log")
#recorder.On()
#recorder.Record()
recorder.ReadFromInputStringOn()
recorder.SetInputString(Recording)

# Add the actors to the renderer, set the background and size
#
ren0.AddActor(sphereActor)
ren0.AddActor(spikeActor)
ren0.SetBackground(0,0,0)
ren0.ResetCamera()

ren1.AddActor(sphereActor)
ren1.AddActor(spikeActor)
ren1.SetBackground(0,0,0)
ren1.ResetCamera()

renWin.SetSize(600, 300)

iRen.Initialize()
renWin.Render()

# Playack events
recorder.Play()

# Interact with the data
iRen.Start()
