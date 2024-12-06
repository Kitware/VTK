// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkAnnulus.h"
#include "vtkAppendPolyData.h"
#include "vtkCamera.h"
#include "vtkClipPolyData.h"
#include "vtkCommand.h"
#include "vtkImplicitAnnulusRepresentation.h"
#include "vtkImplicitAnnulusWidget.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"

namespace
{

const char eventLog[] = "# StreamVersion 1.2\n"
                        "ExposeEvent 0 299 0 0 0 0 0\n"
                        "MouseMoveEvent 147 149 0 0 0 0 0\n"
                        "RenderEvent 147 149 0 0 0 0 0\n"

                        "LeftButtonPressEvent 234 187 0 0 0 0 0\n"
                        "MouseMoveEvent 234 187 0 0 0 0 0\n"
                        "MouseMoveEvent 233 187 0 0 0 0 0\n"
                        "MouseMoveEvent 231 187 0 0 0 0 0\n"
                        "MouseMoveEvent 230 187 0 0 0 0 0\n"
                        "MouseMoveEvent 229 187 0 0 0 0 0\n"
                        "MouseMoveEvent 228 187 0 0 0 0 0\n"
                        "MouseMoveEvent 227 187 0 0 0 0 0\n"
                        "MouseMoveEvent 226 188 0 0 0 0 0\n"
                        "MouseMoveEvent 224 188 0 0 0 0 0\n"
                        "MouseMoveEvent 223 188 0 0 0 0 0\n"
                        "MouseMoveEvent 222 189 0 0 0 0 0\n"
                        "MouseMoveEvent 221 189 0 0 0 0 0\n"
                        "MouseMoveEvent 220 189 0 0 0 0 0\n"
                        "MouseMoveEvent 218 190 0 0 0 0 0\n"
                        "MouseMoveEvent 216 190 0 0 0 0 0\n"
                        "MouseMoveEvent 215 190 0 0 0 0 0\n"
                        "MouseMoveEvent 214 191 0 0 0 0 0\n"
                        "MouseMoveEvent 212 192 0 0 0 0 0\n"
                        "MouseMoveEvent 211 192 0 0 0 0 0\n"
                        "MouseMoveEvent 209 193 0 0 0 0 0\n"
                        "MouseMoveEvent 207 193 0 0 0 0 0\n"
                        "MouseMoveEvent 205 193 0 0 0 0 0\n"
                        "MouseMoveEvent 203 194 0 0 0 0 0\n"
                        "MouseMoveEvent 202 194 0 0 0 0 0\n"
                        "MouseMoveEvent 201 195 0 0 0 0 0\n"
                        "MouseMoveEvent 200 198 0 0 0 0 0\n"
                        "MouseMoveEvent 199 200 0 0 0 0 0\n"
                        "MouseMoveEvent 198 202 0 0 0 0 0\n"
                        "MouseMoveEvent 198 203 0 0 0 0 0\n"
                        "MouseMoveEvent 197 204 0 0 0 0 0\n"
                        "MouseMoveEvent 196 206 0 0 0 0 0\n"
                        "MouseMoveEvent 195 208 0 0 0 0 0\n"
                        "MouseMoveEvent 194 210 0 0 0 0 0\n"
                        "MouseMoveEvent 193 213 0 0 0 0 0\n"
                        "MouseMoveEvent 193 216 0 0 0 0 0\n"
                        "MouseMoveEvent 192 220 0 0 0 0 0\n"
                        "MouseMoveEvent 192 224 0 0 0 0 0\n"
                        "MouseMoveEvent 191 227 0 0 0 0 0\n"
                        "MouseMoveEvent 191 231 0 0 0 0 0\n"
                        "MouseMoveEvent 191 234 0 0 0 0 0\n"
                        "MouseMoveEvent 191 237 0 0 0 0 0\n"
                        "MouseMoveEvent 191 239 0 0 0 0 0\n"
                        "MouseMoveEvent 190 240 0 0 0 0 0\n"
                        "MouseMoveEvent 190 242 0 0 0 0 0\n"
                        "MouseMoveEvent 190 243 0 0 0 0 0\n"
                        "MouseMoveEvent 190 245 0 0 0 0 0\n"
                        "MouseMoveEvent 190 247 0 0 0 0 0\n"
                        "MouseMoveEvent 190 248 0 0 0 0 0\n"
                        "MouseMoveEvent 190 250 0 0 0 0 0\n"
                        "MouseMoveEvent 190 251 0 0 0 0 0\n"
                        "MouseMoveEvent 189 253 0 0 0 0 0\n"
                        "MouseMoveEvent 189 252 0 0 0 0 0\n"
                        "MouseMoveEvent 189 250 0 0 0 0 0\n"
                        "MouseMoveEvent 189 249 0 0 0 0 0\n"
                        "MouseMoveEvent 190 247 0 0 0 0 0\n"
                        "MouseMoveEvent 190 244 0 0 0 0 0\n"
                        "MouseMoveEvent 190 240 0 0 0 0 0\n"
                        "MouseMoveEvent 190 238 0 0 0 0 0\n"
                        "MouseMoveEvent 191 236 0 0 0 0 0\n"
                        "MouseMoveEvent 192 232 0 0 0 0 0\n"
                        "MouseMoveEvent 192 228 0 0 0 0 0\n"
                        "MouseMoveEvent 193 226 0 0 0 0 0\n"
                        "MouseMoveEvent 193 223 0 0 0 0 0\n"
                        "MouseMoveEvent 193 221 0 0 0 0 0\n"
                        "MouseMoveEvent 194 219 0 0 0 0 0\n"
                        "MouseMoveEvent 194 217 0 0 0 0 0\n"
                        "MouseMoveEvent 194 214 0 0 0 0 0\n"
                        "MouseMoveEvent 195 212 0 0 0 0 0\n"
                        "MouseMoveEvent 196 209 0 0 0 0 0\n"
                        "MouseMoveEvent 196 208 0 0 0 0 0\n"
                        "MouseMoveEvent 196 207 0 0 0 0 0\n"
                        "MouseMoveEvent 196 205 0 0 0 0 0\n"
                        "MouseMoveEvent 197 203 0 0 0 0 0\n"
                        "MouseMoveEvent 197 200 0 0 0 0 0\n"
                        "MouseMoveEvent 197 197 0 0 0 0 0\n"
                        "MouseMoveEvent 197 195 0 0 0 0 0\n"
                        "MouseMoveEvent 198 193 0 0 0 0 0\n"
                        "MouseMoveEvent 198 191 0 0 0 0 0\n"
                        "MouseMoveEvent 198 189 0 0 0 0 0\n"
                        "MouseMoveEvent 198 188 0 0 0 0 0\n"
                        "MouseMoveEvent 198 187 0 0 0 0 0\n"
                        "MouseMoveEvent 198 186 0 0 0 0 0\n"
                        "MouseMoveEvent 198 184 0 0 0 0 0\n"
                        "MouseMoveEvent 198 183 0 0 0 0 0\n"
                        "MouseMoveEvent 199 181 0 0 0 0 0\n"
                        "MouseMoveEvent 199 178 0 0 0 0 0\n"
                        "MouseMoveEvent 199 176 0 0 0 0 0\n"
                        "MouseMoveEvent 199 173 0 0 0 0 0\n"
                        "MouseMoveEvent 199 170 0 0 0 0 0\n"
                        "MouseMoveEvent 199 167 0 0 0 0 0\n"
                        "MouseMoveEvent 199 164 0 0 0 0 0\n"
                        "MouseMoveEvent 200 161 0 0 0 0 0\n"
                        "MouseMoveEvent 200 158 0 0 0 0 0\n"
                        "MouseMoveEvent 200 157 0 0 0 0 0\n"
                        "MouseMoveEvent 200 155 0 0 0 0 0\n"
                        "MouseMoveEvent 200 153 0 0 0 0 0\n"
                        "MouseMoveEvent 201 150 0 0 0 0 0\n"
                        "MouseMoveEvent 201 148 0 0 0 0 0\n"
                        "MouseMoveEvent 201 145 0 0 0 0 0\n"
                        "MouseMoveEvent 201 144 0 0 0 0 0\n"
                        "MouseMoveEvent 201 143 0 0 0 0 0\n"
                        "MouseMoveEvent 201 142 0 0 0 0 0\n"
                        "MouseMoveEvent 201 141 0 0 0 0 0\n"
                        "MouseMoveEvent 201 140 0 0 0 0 0\n"
                        "MouseMoveEvent 201 139 0 0 0 0 0\n"
                        "MouseMoveEvent 201 138 0 0 0 0 0\n"
                        "LeftButtonReleaseEvent 201 138 0 0 0 0 0\n"

                        "RenderEvent 218 148 0 0 0 0 0\n"

                        "LeftButtonPressEvent 218 148 0 0 0 0 0\n"
                        "MouseMoveEvent 218 147 0 0 0 0 0\n"
                        "MouseMoveEvent 216 146 0 0 0 0 0\n"
                        "MouseMoveEvent 213 144 0 0 0 0 0\n"
                        "MouseMoveEvent 211 143 0 0 0 0 0\n"
                        "MouseMoveEvent 210 142 0 0 0 0 0\n"
                        "MouseMoveEvent 210 141 0 0 0 0 0\n"
                        "MouseMoveEvent 209 141 0 0 0 0 0\n"
                        "MouseMoveEvent 208 141 0 0 0 0 0\n"
                        "MouseMoveEvent 208 140 0 0 0 0 0\n"
                        "MouseMoveEvent 207 139 0 0 0 0 0\n"
                        "MouseMoveEvent 205 137 0 0 0 0 0\n"
                        "MouseMoveEvent 201 136 0 0 0 0 0\n"
                        "MouseMoveEvent 198 134 0 0 0 0 0\n"
                        "MouseMoveEvent 195 132 0 0 0 0 0\n"
                        "MouseMoveEvent 192 130 0 0 0 0 0\n"
                        "MouseMoveEvent 189 129 0 0 0 0 0\n"
                        "MouseMoveEvent 187 127 0 0 0 0 0\n"
                        "MouseMoveEvent 184 126 0 0 0 0 0\n"
                        "MouseMoveEvent 182 125 0 0 0 0 0\n"
                        "MouseMoveEvent 180 124 0 0 0 0 0\n"
                        "MouseMoveEvent 176 123 0 0 0 0 0\n"
                        "MouseMoveEvent 174 122 0 0 0 0 0\n"
                        "MouseMoveEvent 172 121 0 0 0 0 0\n"
                        "MouseMoveEvent 170 121 0 0 0 0 0\n"
                        "MouseMoveEvent 167 120 0 0 0 0 0\n"
                        "MouseMoveEvent 165 119 0 0 0 0 0\n"
                        "MouseMoveEvent 162 117 0 0 0 0 0\n"
                        "MouseMoveEvent 158 116 0 0 0 0 0\n"
                        "MouseMoveEvent 155 115 0 0 0 0 0\n"
                        "MouseMoveEvent 150 114 0 0 0 0 0\n"
                        "MouseMoveEvent 149 113 0 0 0 0 0\n"
                        "MouseMoveEvent 147 112 0 0 0 0 0\n"
                        "MouseMoveEvent 145 112 0 0 0 0 0\n"
                        "MouseMoveEvent 143 112 0 0 0 0 0\n"
                        "MouseMoveEvent 140 111 0 0 0 0 0\n"
                        "MouseMoveEvent 137 111 0 0 0 0 0\n"
                        "MouseMoveEvent 133 110 0 0 0 0 0\n"
                        "MouseMoveEvent 130 109 0 0 0 0 0\n"
                        "MouseMoveEvent 126 108 0 0 0 0 0\n"
                        "MouseMoveEvent 123 108 0 0 0 0 0\n"
                        "MouseMoveEvent 119 108 0 0 0 0 0\n"
                        "MouseMoveEvent 115 107 0 0 0 0 0\n"
                        "MouseMoveEvent 113 107 0 0 0 0 0\n"
                        "MouseMoveEvent 111 107 0 0 0 0 0\n"
                        "MouseMoveEvent 108 106 0 0 0 0 0\n"
                        "MouseMoveEvent 106 106 0 0 0 0 0\n"
                        "MouseMoveEvent 104 106 0 0 0 0 0\n"
                        "MouseMoveEvent 101 105 0 0 0 0 0\n"
                        "MouseMoveEvent 97 105 0 0 0 0 0\n"
                        "MouseMoveEvent 93 105 0 0 0 0 0\n"
                        "MouseMoveEvent 89 104 0 0 0 0 0\n"
                        "MouseMoveEvent 88 103 0 0 0 0 0\n"
                        "MouseMoveEvent 87 103 0 0 0 0 0\n"
                        "LeftButtonReleaseEvent 87 103 0 0 0 0 0\n"

                        "RenderEvent 106 138 0 0 0 0 0\n"

                        "LeftButtonPressEvent 106 138 0 0 0 0 0\n"
                        "MouseMoveEvent 106 138 0 0 0 0 0\n"
                        "MouseMoveEvent 106 138 0 0 0 0 0\n"
                        "MouseMoveEvent 107 138 0 0 0 0 0\n"
                        "MouseMoveEvent 107 137 0 0 0 0 0\n"
                        "MouseMoveEvent 108 137 0 0 0 0 0\n"
                        "MouseMoveEvent 108 136 0 0 0 0 0\n"
                        "MouseMoveEvent 109 136 0 0 0 0 0\n"
                        "MouseMoveEvent 109 135 0 0 0 0 0\n"
                        "MouseMoveEvent 110 134 0 0 0 0 0\n"
                        "MouseMoveEvent 111 133 0 0 0 0 0\n"
                        "MouseMoveEvent 111 132 0 0 0 0 0\n"
                        "MouseMoveEvent 112 130 0 0 0 0 0\n"
                        "MouseMoveEvent 113 128 0 0 0 0 0\n"
                        "MouseMoveEvent 113 125 0 0 0 0 0\n"
                        "MouseMoveEvent 114 124 0 0 0 0 0\n"
                        "MouseMoveEvent 115 123 0 0 0 0 0\n"
                        "MouseMoveEvent 115 121 0 0 0 0 0\n"
                        "MouseMoveEvent 116 120 0 0 0 0 0\n"
                        "MouseMoveEvent 117 119 0 0 0 0 0\n"
                        "MouseMoveEvent 117 118 0 0 0 0 0\n"
                        "MouseMoveEvent 118 117 0 0 0 0 0\n"
                        "MouseMoveEvent 119 116 0 0 0 0 0\n"
                        "MouseMoveEvent 120 114 0 0 0 0 0\n"
                        "MouseMoveEvent 121 112 0 0 0 0 0\n"
                        "MouseMoveEvent 122 110 0 0 0 0 0\n"
                        "MouseMoveEvent 124 108 0 0 0 0 0\n"
                        "MouseMoveEvent 124 107 0 0 0 0 0\n"
                        "MouseMoveEvent 125 107 0 0 0 0 0\n"
                        "MouseMoveEvent 124 108 0 0 0 0 0\n"
                        "MouseMoveEvent 124 108 0 0 0 0 0\n"
                        "MouseMoveEvent 124 109 0 0 0 0 0\n"
                        "MouseMoveEvent 123 109 0 0 0 0 0\n"
                        "MouseMoveEvent 123 110 0 0 0 0 0\n"
                        "MouseMoveEvent 122 110 0 0 0 0 0\n"
                        "MouseMoveEvent 121 111 0 0 0 0 0\n"
                        "MouseMoveEvent 121 110 0 0 0 0 0\n"
                        "MouseMoveEvent 122 109 0 0 0 0 0\n"
                        "MouseMoveEvent 122 108 0 0 0 0 0\n"
                        "MouseMoveEvent 122 107 0 0 0 0 0\n"
                        "MouseMoveEvent 123 106 0 0 0 0 0\n"
                        "MouseMoveEvent 123 105 0 0 0 0 0\n"
                        "MouseMoveEvent 124 105 0 0 0 0 0\n"
                        "MouseMoveEvent 123 106 0 0 0 0 0\n"
                        "MouseMoveEvent 123 108 0 0 0 0 0\n"
                        "MouseMoveEvent 123 109 0 0 0 0 0\n"
                        "MouseMoveEvent 124 110 0 0 0 0 0\n"
                        "MouseMoveEvent 126 112 0 0 0 0 0\n"
                        "MouseMoveEvent 127 114 0 0 0 0 0\n"
                        "MouseMoveEvent 128 115 0 0 0 0 0\n"
                        "MouseMoveEvent 132 119 0 0 0 0 0\n"
                        "MouseMoveEvent 135 122 0 0 0 0 0\n"
                        "MouseMoveEvent 137 124 0 0 0 0 0\n"
                        "MouseMoveEvent 140 127 0 0 0 0 0\n"
                        "MouseMoveEvent 141 128 0 0 0 0 0\n"
                        "MouseMoveEvent 142 130 0 0 0 0 0\n"
                        "MouseMoveEvent 142 131 0 0 0 0 0\n"
                        "MouseMoveEvent 143 132 0 0 0 0 0\n"
                        "MouseMoveEvent 144 134 0 0 0 0 0\n"
                        "LeftButtonReleaseEvent 144 134 0 0 0 0 0\n"

                        "RenderEvent 137 138 0 0 0 0 0\n"

                        "LeftButtonPressEvent 139 143 0 0 0 0 0\n"
                        "MouseMoveEvent 138 143 0 0 0 0 0\n"
                        "MouseMoveEvent 137 141 0 0 0 0 0\n"
                        "MouseMoveEvent 134 137 0 0 0 0 0\n"
                        "MouseMoveEvent 131 134 0 0 0 0 0\n"
                        "MouseMoveEvent 131 133 0 0 0 0 0\n"
                        "MouseMoveEvent 130 129 0 0 0 0 0\n"
                        "MouseMoveEvent 129 128 0 0 0 0 0\n"
                        "MouseMoveEvent 129 127 0 0 0 0 0\n"
                        "MouseMoveEvent 128 126 0 0 0 0 0\n"
                        "MouseMoveEvent 128 125 0 0 0 0 0\n"
                        "MouseMoveEvent 127 123 0 0 0 0 0\n"
                        "MouseMoveEvent 126 121 0 0 0 0 0\n"
                        "MouseMoveEvent 126 120 0 0 0 0 0\n"
                        "MouseMoveEvent 126 120 0 0 0 0 0\n"
                        "MouseMoveEvent 126 119 0 0 0 0 0\n"
                        "MouseMoveEvent 126 118 0 0 0 0 0\n"
                        "MouseMoveEvent 126 117 0 0 0 0 0\n"
                        "MouseMoveEvent 126 116 0 0 0 0 0\n"
                        "MouseMoveEvent 126 115 0 0 0 0 0\n"
                        "MouseMoveEvent 126 114 0 0 0 0 0\n"
                        "MouseMoveEvent 126 114 0 0 0 0 0\n"
                        "MouseMoveEvent 126 113 0 0 0 0 0\n"
                        "MouseMoveEvent 126 112 0 0 0 0 0\n"
                        "MouseMoveEvent 126 111 0 0 0 0 0\n"
                        "MouseMoveEvent 126 108 0 0 0 0 0\n"
                        "MouseMoveEvent 126 107 0 0 0 0 0\n"
                        "MouseMoveEvent 126 106 0 0 0 0 0\n"
                        "LeftButtonReleaseEvent 126 106 0 0 0 0 0\n"

                        "RenderEvent 159 162 0 0 0 0 0\n"

                        "LeftButtonPressEvent 159 162 0 0 0 0 0\n"
                        "MouseMoveEvent 160 162 0 0 0 0 0\n"
                        "MouseMoveEvent 161 162 0 0 0 0 0\n"
                        "MouseMoveEvent 161 162 0 0 0 0 0\n"
                        "MouseMoveEvent 162 162 0 0 0 0 0\n"
                        "MouseMoveEvent 162 162 0 0 0 0 0\n"
                        "MouseMoveEvent 163 162 0 0 0 0 0\n"
                        "MouseMoveEvent 163 161 0 0 0 0 0\n"
                        "LeftButtonReleaseEvent 163 161 0 0 0 0 0\n"

                        "RenderEvent 133 237 0 0 0 0 0\n"

                        "LeftButtonPressEvent 133 237 0 0 0 0 0\n"
                        "MouseMoveEvent 135 239 0 0 0 0 0\n"
                        "MouseMoveEvent 137 242 0 0 0 0 0\n"
                        "MouseMoveEvent 138 243 0 0 0 0 0\n"
                        "MouseMoveEvent 139 244 0 0 0 0 0\n"
                        "MouseMoveEvent 139 245 0 0 0 0 0\n"
                        "MouseMoveEvent 139 246 0 0 0 0 0\n"
                        "MouseMoveEvent 140 247 0 0 0 0 0\n"
                        "MouseMoveEvent 140 248 0 0 0 0 0\n"
                        "MouseMoveEvent 140 248 0 0 0 0 0\n"
                        "MouseMoveEvent 140 249 0 0 0 0 0\n"
                        "MouseMoveEvent 140 250 0 0 0 0 0\n"
                        "MouseMoveEvent 140 251 0 0 0 0 0\n"
                        "MouseMoveEvent 140 252 0 0 0 0 0\n"
                        "MouseMoveEvent 140 253 0 0 0 0 0\n"
                        "MouseMoveEvent 140 254 0 0 0 0 0\n"
                        "MouseMoveEvent 140 256 0 0 0 0 0\n"
                        "MouseMoveEvent 140 257 0 0 0 0 0\n"
                        "MouseMoveEvent 140 259 0 0 0 0 0\n"
                        "MouseMoveEvent 140 260 0 0 0 0 0\n"
                        "MouseMoveEvent 140 261 0 0 0 0 0\n"
                        "MouseMoveEvent 140 262 0 0 0 0 0\n"
                        "MouseMoveEvent 140 262 0 0 0 0 0\n"
                        "MouseMoveEvent 140 263 0 0 0 0 0\n"
                        "MouseMoveEvent 140 265 0 0 0 0 0\n"
                        "MouseMoveEvent 140 266 0 0 0 0 0\n"
                        "MouseMoveEvent 139 267 0 0 0 0 0\n"
                        "MouseMoveEvent 139 268 0 0 0 0 0\n"
                        "MouseMoveEvent 139 269 0 0 0 0 0\n"
                        "MouseMoveEvent 139 270 0 0 0 0 0\n"
                        "MouseMoveEvent 139 271 0 0 0 0 0\n"
                        "MouseMoveEvent 139 272 0 0 0 0 0\n"
                        "MouseMoveEvent 139 273 0 0 0 0 0\n"
                        "MouseMoveEvent 139 274 0 0 0 0 0\n"
                        "MouseMoveEvent 139 275 0 0 0 0 0\n"
                        "MouseMoveEvent 139 276 0 0 0 0 0\n"
                        "MouseMoveEvent 139 277 0 0 0 0 0\n"
                        "MouseMoveEvent 139 278 0 0 0 0 0\n"
                        "MouseMoveEvent 139 279 0 0 0 0 0\n"
                        "MouseMoveEvent 140 280 0 0 0 0 0\n"
                        "MouseMoveEvent 140 281 0 0 0 0 0\n"
                        "MouseMoveEvent 140 283 0 0 0 0 0\n"
                        "MouseMoveEvent 141 284 0 0 0 0 0\n"
                        "MouseMoveEvent 141 285 0 0 0 0 0\n"
                        "MouseMoveEvent 142 287 0 0 0 0 0\n"
                        "MouseMoveEvent 142 288 0 0 0 0 0\n"
                        "MouseMoveEvent 142 289 0 0 0 0 0\n"
                        "MouseMoveEvent 142 290 0 0 0 0 0\n"
                        "MouseMoveEvent 142 290 0 0 0 0 0\n"
                        "MouseMoveEvent 142 291 0 0 0 0 0\n"
                        "LeftButtonReleaseEvent 142 291 0 0 0 0 0\n"

                        "RenderEvent 154 201 0 0 0 0 0\n"

                        "LeftButtonPressEvent 154 201 0 0 0 0 0\n"
                        "MouseMoveEvent 154 201 0 0 0 0 0\n"
                        "MouseMoveEvent 154 201 0 0 0 0 0\n"
                        "MouseMoveEvent 155 201 0 0 0 0 0\n"
                        "MouseMoveEvent 155 201 0 0 0 0 0\n"
                        "MouseMoveEvent 155 200 0 0 0 0 0\n"
                        "MouseMoveEvent 156 200 0 0 0 0 0\n"
                        "MouseMoveEvent 157 200 0 0 0 0 0\n"
                        "MouseMoveEvent 157 199 0 0 0 0 0\n"
                        "MouseMoveEvent 158 199 0 0 0 0 0\n"
                        "MouseMoveEvent 159 198 0 0 0 0 0\n"
                        "MouseMoveEvent 160 198 0 0 0 0 0\n"
                        "MouseMoveEvent 161 197 0 0 0 0 0\n"
                        "MouseMoveEvent 162 197 0 0 0 0 0\n"
                        "MouseMoveEvent 162 196 0 0 0 0 0\n"
                        "MouseMoveEvent 163 196 0 0 0 0 0\n"
                        "MouseMoveEvent 163 195 0 0 0 0 0\n"
                        "MouseMoveEvent 164 195 0 0 0 0 0\n"
                        "MouseMoveEvent 165 195 0 0 0 0 0\n"
                        "MouseMoveEvent 165 194 0 0 0 0 0\n"
                        "MouseMoveEvent 166 193 0 0 0 0 0\n"
                        "MouseMoveEvent 167 193 0 0 0 0 0\n"
                        "MouseMoveEvent 167 192 0 0 0 0 0\n"
                        "MouseMoveEvent 168 192 0 0 0 0 0\n"
                        "MouseMoveEvent 168 191 0 0 0 0 0\n"
                        "MouseMoveEvent 169 190 0 0 0 0 0\n"
                        "MouseMoveEvent 171 189 0 0 0 0 0\n"
                        "MouseMoveEvent 172 188 0 0 0 0 0\n"
                        "MouseMoveEvent 173 186 0 0 0 0 0\n"
                        "MouseMoveEvent 174 185 0 0 0 0 0\n"
                        "MouseMoveEvent 175 183 0 0 0 0 0\n"
                        "MouseMoveEvent 176 182 0 0 0 0 0\n"
                        "MouseMoveEvent 177 181 0 0 0 0 0\n"
                        "MouseMoveEvent 178 180 0 0 0 0 0\n"
                        "MouseMoveEvent 179 179 0 0 0 0 0\n"
                        "MouseMoveEvent 179 178 0 0 0 0 0\n"
                        "MouseMoveEvent 180 178 0 0 0 0 0\n"
                        "MouseMoveEvent 181 178 0 0 0 0 0\n"
                        "MouseMoveEvent 181 177 0 0 0 0 0\n"
                        "MouseMoveEvent 182 177 0 0 0 0 0\n"
                        "MouseMoveEvent 182 176 0 0 0 0 0\n"
                        "MouseMoveEvent 182 176 0 0 0 0 0\n"
                        "MouseMoveEvent 182 175 0 0 0 0 0\n"
                        "MouseMoveEvent 182 174 0 0 0 0 0\n"
                        "MouseMoveEvent 182 173 0 0 0 0 0\n"
                        "MouseMoveEvent 182 173 0 0 0 0 0\n"
                        "MouseMoveEvent 182 172 0 0 0 0 0\n"
                        "MouseMoveEvent 182 171 0 0 0 0 0\n"
                        "MouseMoveEvent 182 170 0 0 0 0 0\n"
                        "MouseMoveEvent 182 169 0 0 0 0 0\n"
                        "MouseMoveEvent 182 168 0 0 0 0 0\n"
                        "MouseMoveEvent 182 167 0 0 0 0 0\n"
                        "MouseMoveEvent 182 167 0 0 0 0 0\n"
                        "MouseMoveEvent 181 167 0 0 0 0 0\n"
                        "MouseMoveEvent 181 167 0 0 0 0 0\n"
                        "MouseMoveEvent 180 167 0 0 0 0 0\n"
                        "MouseMoveEvent 179 167 0 0 0 0 0\n"
                        "MouseMoveEvent 178 167 0 0 0 0 0\n"
                        "MouseMoveEvent 177 167 0 0 0 0 0\n"
                        "MouseMoveEvent 175 166 0 0 0 0 0\n"
                        "MouseMoveEvent 174 166 0 0 0 0 0\n"
                        "MouseMoveEvent 173 166 0 0 0 0 0\n"
                        "MouseMoveEvent 170 165 0 0 0 0 0\n"
                        "MouseMoveEvent 169 165 0 0 0 0 0\n"
                        "LeftButtonReleaseEvent 169 165 0 0 0 0 0\n"

                        "RenderEvent 150 1 0 0 0 0 0\n"
                        "LeaveEvent 151 -2 0 0 0 0 0\n";

// This does the actual work: updates the vtkAnnulus implicit function.
// This in turn causes the pipeline to update and clip the object.
// Callback for the interaction
class UpdateClipSurfaceCallback : public vtkCommand
{
public:
  static UpdateClipSurfaceCallback* New() { return new UpdateClipSurfaceCallback(); }

  void Execute(vtkObject* caller, unsigned long, void*) override
  {
    vtkImplicitAnnulusWidget* annulusWidget = reinterpret_cast<vtkImplicitAnnulusWidget*>(caller);
    vtkImplicitAnnulusRepresentation* rep =
      reinterpret_cast<vtkImplicitAnnulusRepresentation*>(annulusWidget->GetRepresentation());
    rep->GetAnnulus(this->Annulus);
    this->Actor->VisibilityOn();
  }

  vtkAnnulus* Annulus = nullptr;
  vtkActor* Actor = nullptr;
};

} // anonymous namespace

int TestImplicitAnnulusWidget(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  // Create a mace out of filters.
  vtkNew<vtkSphereSource> sphere;

  vtkNew<vtkPolyDataMapper> sphereMapper;
  sphereMapper->SetInputConnection(sphere->GetOutputPort());

  vtkNew<vtkActor> sphereActor;
  sphereActor->SetMapper(sphereMapper);

  vtkNew<vtkAnnulus> annulus;
  vtkNew<vtkClipPolyData> clipper;
  clipper->SetInputConnection(sphere->GetOutputPort());
  clipper->SetClipFunction(annulus);

  vtkNew<vtkPolyDataMapper> clipMapper;
  clipMapper->SetInputConnection(clipper->GetOutputPort());

  vtkNew<vtkActor> clipActor;
  clipActor->SetMapper(clipMapper);
  clipActor->GetProperty()->SetColor(0, 1, 0);
  clipActor->VisibilityOff();
  clipActor->SetScale(1.01, 1.01, 1.01);

  // Create the RenderWindow, Renderer and both Actors
  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(sphereActor);
  renderer->AddActor(clipActor);
  renderer->SetBackground(0.1, 0.2, 0.4);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->SetSize(300, 300);
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> interactor;
  renWin->SetInteractor(interactor);
  renWin->Render();

  vtkNew<vtkImplicitAnnulusRepresentation> rep;
  rep->SetPlaceFactor(1.25);
  rep->PlaceWidget(sphere->GetOutput()->GetBounds());

  vtkNew<vtkImplicitAnnulusWidget> annulusWidget;
  annulusWidget->SetInteractor(interactor);
  annulusWidget->SetRepresentation(rep);
  annulusWidget->SetEnabled(true);

  // Add callback to update click following annulus widget.
  vtkNew<UpdateClipSurfaceCallback> myCallback;
  myCallback->Annulus = annulus;
  myCallback->Actor = clipActor;
  annulusWidget->AddObserver(vtkCommand::InteractionEvent, myCallback);

  vtkNew<vtkInteractorEventRecorder> recorder;
  recorder->SetInteractor(interactor);

#if 0 // set to 1 if recording
  recorder->SetFileName("./record.log");
  recorder->Record();

  interactor->Initialize();
  renWin->Render();
  recorder->On();

  interactor->Start();
  recorder->Stop();

#else
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(eventLog);

  // render the image
  interactor->Initialize();
  renWin->Render();
  recorder->Play();

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();

  interactor->Start();
#endif

  return EXIT_SUCCESS;
}
