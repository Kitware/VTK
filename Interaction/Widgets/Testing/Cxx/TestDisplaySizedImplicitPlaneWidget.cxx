// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAppendPolyData.h"
#include "vtkClipPolyData.h"
#include "vtkCommand.h"
#include "vtkConeSource.h"
#include "vtkDisplaySizedImplicitPlaneRepresentation.h"
#include "vtkDisplaySizedImplicitPlaneWidget.h"
#include "vtkGlyph3D.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkLODActor.h"
#include "vtkNew.h"
#include "vtkPlane.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"

using Representation = vtkDisplaySizedImplicitPlaneRepresentation;
using Widget = vtkDisplaySizedImplicitPlaneWidget;

const char eventLog[] = "# StreamVersion 1.1\n"
                        "ExposeEvent 0 299 0 0 0 0\n"
                        "RenderEvent 0 299 0 0 0 0\n"
                        "EnterEvent 299 170 0 0 0 0\n"
                        "MouseMoveEvent 299 170 0 0 0 0\n"
                        "MouseMoveEvent 235 176 0 0 0 0\n"
                        "RenderEvent 235 176 0 0 0 0\n"
                        "MouseMoveEvent 235 176 0 0 0 0\n"
                        "MouseMoveEvent 234 176 0 0 0 0\n"
                        "LeftButtonPressEvent 234 176 0 0 0 0\n"
                        "RenderEvent 234 176 0 0 0 0\n"
                        "MouseMoveEvent 234 176 0 0 0 0\n"
                        "RenderEvent 234 176 0 0 0 0\n"
                        "MouseMoveEvent 233 176 0 0 0 0\n"
                        "RenderEvent 233 176 0 0 0 0\n"
                        "MouseMoveEvent 233 176 0 0 0 0\n"
                        "RenderEvent 233 176 0 0 0 0\n"
                        "MouseMoveEvent 232 175 0 0 0 0\n"
                        "RenderEvent 232 175 0 0 0 0\n"
                        "MouseMoveEvent 231 175 0 0 0 0\n"
                        "RenderEvent 231 175 0 0 0 0\n"
                        "MouseMoveEvent 230 175 0 0 0 0\n"
                        "RenderEvent 230 175 0 0 0 0\n"
                        "MouseMoveEvent 228 174 0 0 0 0\n"
                        "RenderEvent 228 174 0 0 0 0\n"
                        "MouseMoveEvent 227 174 0 0 0 0\n"
                        "RenderEvent 227 174 0 0 0 0\n"
                        "MouseMoveEvent 226 174 0 0 0 0\n"
                        "RenderEvent 226 174 0 0 0 0\n"
                        "MouseMoveEvent 225 174 0 0 0 0\n"
                        "RenderEvent 225 174 0 0 0 0\n"
                        "MouseMoveEvent 224 174 0 0 0 0\n"
                        "RenderEvent 224 174 0 0 0 0\n"
                        "MouseMoveEvent 223 173 0 0 0 0\n"
                        "RenderEvent 223 173 0 0 0 0\n"
                        "MouseMoveEvent 222 173 0 0 0 0\n"
                        "RenderEvent 222 173 0 0 0 0\n"
                        "MouseMoveEvent 221 173 0 0 0 0\n"
                        "RenderEvent 221 173 0 0 0 0\n"
                        "MouseMoveEvent 220 173 0 0 0 0\n"
                        "RenderEvent 220 173 0 0 0 0\n"
                        "MouseMoveEvent 218 172 0 0 0 0\n"
                        "RenderEvent 218 172 0 0 0 0\n"
                        "MouseMoveEvent 216 172 0 0 0 0\n"
                        "RenderEvent 216 172 0 0 0 0\n"
                        "MouseMoveEvent 215 172 0 0 0 0\n"
                        "RenderEvent 215 172 0 0 0 0\n"
                        "MouseMoveEvent 214 171 0 0 0 0\n"
                        "RenderEvent 214 171 0 0 0 0\n"
                        "MouseMoveEvent 213 171 0 0 0 0\n"
                        "RenderEvent 213 171 0 0 0 0\n"
                        "MouseMoveEvent 212 171 0 0 0 0\n"
                        "RenderEvent 212 171 0 0 0 0\n"
                        "MouseMoveEvent 211 171 0 0 0 0\n"
                        "RenderEvent 211 171 0 0 0 0\n"
                        "MouseMoveEvent 209 170 0 0 0 0\n"
                        "RenderEvent 209 170 0 0 0 0\n"
                        "MouseMoveEvent 207 170 0 0 0 0\n"
                        "RenderEvent 207 170 0 0 0 0\n"
                        "MouseMoveEvent 206 169 0 0 0 0\n"
                        "RenderEvent 206 169 0 0 0 0\n"
                        "MouseMoveEvent 204 169 0 0 0 0\n"
                        "RenderEvent 204 169 0 0 0 0\n"
                        "MouseMoveEvent 202 169 0 0 0 0\n"
                        "RenderEvent 202 169 0 0 0 0\n"
                        "MouseMoveEvent 201 168 0 0 0 0\n"
                        "RenderEvent 201 168 0 0 0 0\n"
                        "MouseMoveEvent 201 168 0 0 0 0\n"
                        "RenderEvent 201 168 0 0 0 0\n"
                        "MouseMoveEvent 200 168 0 0 0 0\n"
                        "RenderEvent 200 168 0 0 0 0\n"
                        "MouseMoveEvent 199 168 0 0 0 0\n"
                        "RenderEvent 199 168 0 0 0 0\n"
                        "MouseMoveEvent 198 167 0 0 0 0\n"
                        "RenderEvent 198 167 0 0 0 0\n"
                        "MouseMoveEvent 197 167 0 0 0 0\n"
                        "RenderEvent 197 167 0 0 0 0\n"
                        "MouseMoveEvent 196 167 0 0 0 0\n"
                        "RenderEvent 196 167 0 0 0 0\n"
                        "MouseMoveEvent 196 166 0 0 0 0\n"
                        "RenderEvent 196 166 0 0 0 0\n"
                        "MouseMoveEvent 194 166 0 0 0 0\n"
                        "RenderEvent 194 166 0 0 0 0\n"
                        "MouseMoveEvent 193 166 0 0 0 0\n"
                        "RenderEvent 193 166 0 0 0 0\n"
                        "MouseMoveEvent 191 165 0 0 0 0\n"
                        "RenderEvent 191 165 0 0 0 0\n"
                        "MouseMoveEvent 190 165 0 0 0 0\n"
                        "RenderEvent 190 165 0 0 0 0\n"
                        "MouseMoveEvent 188 164 0 0 0 0\n"
                        "RenderEvent 188 164 0 0 0 0\n"
                        "MouseMoveEvent 187 164 0 0 0 0\n"
                        "RenderEvent 187 164 0 0 0 0\n"
                        "MouseMoveEvent 187 164 0 0 0 0\n"
                        "RenderEvent 187 164 0 0 0 0\n"
                        "MouseMoveEvent 186 164 0 0 0 0\n"
                        "RenderEvent 186 164 0 0 0 0\n"
                        "MouseMoveEvent 186 164 0 0 0 0\n"
                        "RenderEvent 186 164 0 0 0 0\n"
                        "MouseMoveEvent 185 163 0 0 0 0\n"
                        "RenderEvent 185 163 0 0 0 0\n"
                        "MouseMoveEvent 185 163 0 0 0 0\n"
                        "RenderEvent 185 163 0 0 0 0\n"
                        "MouseMoveEvent 185 163 0 0 0 0\n"
                        "RenderEvent 185 163 0 0 0 0\n"
                        "MouseMoveEvent 185 163 0 0 0 0\n"
                        "RenderEvent 185 163 0 0 0 0\n"
                        "LeftButtonReleaseEvent 185 163 0 0 0 0\n"
                        "RenderEvent 185 163 0 0 0 0\n"
                        "MouseMoveEvent 185 163 0 0 0 0\n"
                        "RenderEvent 185 163 0 0 0 0\n"
                        "MouseMoveEvent 185 163 0 0 0 0\n"
                        "MouseMoveEvent 190 164 0 0 0 0\n"
                        "RenderEvent 190 164 0 0 0 0\n"
                        "MouseMoveEvent 191 164 0 0 0 0\n"
                        "MouseMoveEvent 265 156 0 0 0 0\n"
                        "LeftButtonPressEvent 265 156 0 0 0 0\n"
                        "StartInteractionEvent 265 156 0 0 0 0\n"
                        "MouseMoveEvent 265 155 0 0 0 0\n"
                        "InteractionEvent 265 155 0 0 0 0\n"
                        "MouseMoveEvent 265 155 0 0 0 0\n"
                        "InteractionEvent 265 155 0 0 0 0\n"
                        "TimerEvent 265 155 0 0 0 0\n"
                        "RenderEvent 265 155 0 0 0 0\n"
                        "TimerEvent 265 155 0 0 0 0\n"
                        "RenderEvent 265 155 0 0 0 0\n"
                        "TimerEvent 265 155 0 0 0 0\n"
                        "RenderEvent 265 155 0 0 0 0\n"
                        "TimerEvent 265 155 0 0 0 0\n"
                        "RenderEvent 265 155 0 0 0 0\n"
                        "TimerEvent 265 155 0 0 0 0\n"
                        "RenderEvent 265 155 0 0 0 0\n"
                        "MouseMoveEvent 266 155 0 0 0 0\n"
                        "InteractionEvent 266 155 0 0 0 0\n"
                        "TimerEvent 266 155 0 0 0 0\n"
                        "RenderEvent 266 155 0 0 0 0\n"
                        "MouseMoveEvent 267 154 0 0 0 0\n"
                        "InteractionEvent 267 154 0 0 0 0\n"
                        "TimerEvent 267 154 0 0 0 0\n"
                        "RenderEvent 267 154 0 0 0 0\n"
                        "MouseMoveEvent 270 152 0 0 0 0\n"
                        "InteractionEvent 270 152 0 0 0 0\n"
                        "TimerEvent 270 152 0 0 0 0\n"
                        "RenderEvent 270 152 0 0 0 0\n"
                        "LeftButtonReleaseEvent 269 152 0 0 0 0\n"
                        "EndInteractionEvent 269 152 0 0 0 0\n"
                        "RenderEvent 269 152 0 0 0 0\n"
                        "MouseMoveEvent 270 152 0 0 0 0\n"
                        "MouseMoveEvent 180 65 0 0 0 0\n"
                        "KeyPressEvent 180 65 0 111 1 p\n"
                        "RenderEvent 180 65 0 111 1 p\n"
                        "CharEvent 180 65 0 111 1 p\n"
                        "KeyReleaseEvent 180 65 0 111 1 p\n"
                        "MouseMoveEvent 180 65 0 0 0 p\n"
                        "RenderEvent 180 65 0 0 0 p\n"
                        "MouseMoveEvent 179 65 0 0 0 p\n"
                        "MouseMoveEvent 175 66 0 0 0 p\n"
                        "RenderEvent 175 66 0 0 0 p\n"
                        "MouseMoveEvent 175 66 0 0 0 p\n"
                        "MouseMoveEvent 169 67 0 0 0 p\n"
                        "RenderEvent 169 67 0 0 0 p\n"
                        "MouseMoveEvent 167 68 0 0 0 p\n"
                        "MouseMoveEvent 158 70 0 0 0 p\n"
                        "RenderEvent 158 70 0 0 0 p\n"
                        "MouseMoveEvent 157 70 0 0 0 p\n"
                        "MouseMoveEvent 139 117 0 0 0 p\n"
                        "KeyPressEvent 139 117 0 110 1 n\n"
                        "RenderEvent 139 117 0 110 1 n\n"
                        "CharEvent 139 117 0 110 1 n\n"
                        "KeyReleaseEvent 139 117 0 110 1 n\n"
                        "MouseMoveEvent 139 117 0 0 0 n\n"
                        "MouseMoveEvent 178 102 0 0 0 n\n"
                        "RenderEvent 178 102 0 0 0 n\n"
                        "MouseMoveEvent 179 101 0 0 0 n\n"
                        "MouseMoveEvent 195 80 0 0 0 n\n"
                        "RenderEvent 195 80 0 0 0 n\n"
                        "LeftButtonPressEvent 195 80 0 0 0 n\n"
                        "RenderEvent 195 80 0 0 0 n\n"
                        "MouseMoveEvent 195 80 0 0 0 n\n"
                        "RenderEvent 195 80 0 0 0 n\n"
                        "MouseMoveEvent 196 80 0 0 0 n\n"
                        "RenderEvent 196 80 0 0 0 n\n"
                        "MouseMoveEvent 197 80 0 0 0 n\n"
                        "RenderEvent 197 80 0 0 0 n\n"
                        "MouseMoveEvent 198 80 0 0 0 n\n"
                        "RenderEvent 198 80 0 0 0 n\n"
                        "MouseMoveEvent 201 81 0 0 0 n\n"
                        "RenderEvent 201 81 0 0 0 n\n"
                        "MouseMoveEvent 204 81 0 0 0 n\n"
                        "RenderEvent 204 81 0 0 0 n\n"
                        "MouseMoveEvent 208 81 0 0 0 n\n"
                        "RenderEvent 208 81 0 0 0 n\n"
                        "MouseMoveEvent 213 81 0 0 0 n\n"
                        "RenderEvent 213 81 0 0 0 n\n"
                        "MouseMoveEvent 218 81 0 0 0 n\n"
                        "RenderEvent 218 81 0 0 0 n\n"
                        "MouseMoveEvent 223 81 0 0 0 n\n"
                        "RenderEvent 223 81 0 0 0 n\n"
                        "MouseMoveEvent 228 82 0 0 0 n\n"
                        "RenderEvent 228 82 0 0 0 n\n"
                        "MouseMoveEvent 234 82 0 0 0 n\n"
                        "RenderEvent 234 82 0 0 0 n\n"
                        "MouseMoveEvent 238 82 0 0 0 n\n"
                        "RenderEvent 238 82 0 0 0 n\n"
                        "MouseMoveEvent 242 82 0 0 0 n\n"
                        "RenderEvent 242 82 0 0 0 n\n"
                        "MouseMoveEvent 244 82 0 0 0 n\n"
                        "RenderEvent 244 82 0 0 0 n\n"
                        "MouseMoveEvent 246 82 0 0 0 n\n"
                        "RenderEvent 246 82 0 0 0 n\n"
                        "MouseMoveEvent 249 82 0 0 0 n\n"
                        "RenderEvent 249 82 0 0 0 n\n"
                        "MouseMoveEvent 251 82 0 0 0 n\n"
                        "RenderEvent 251 82 0 0 0 n\n"
                        "MouseMoveEvent 251 82 0 0 0 n\n"
                        "RenderEvent 251 82 0 0 0 n\n"
                        "MouseMoveEvent 251 82 0 0 0 n\n"
                        "RenderEvent 251 82 0 0 0 n\n"
                        "MouseMoveEvent 252 82 0 0 0 n\n"
                        "RenderEvent 252 82 0 0 0 n\n"
                        "MouseMoveEvent 252 82 0 0 0 n\n"
                        "RenderEvent 252 82 0 0 0 n\n"
                        "MouseMoveEvent 252 82 0 0 0 n\n"
                        "RenderEvent 252 82 0 0 0 n\n"
                        "MouseMoveEvent 253 82 0 0 0 n\n"
                        "RenderEvent 253 82 0 0 0 n\n"
                        "MouseMoveEvent 253 82 0 0 0 n\n"
                        "RenderEvent 253 82 0 0 0 n\n"
                        "MouseMoveEvent 255 82 0 0 0 n\n"
                        "RenderEvent 255 82 0 0 0 n\n"
                        "MouseMoveEvent 256 83 0 0 0 n\n"
                        "RenderEvent 256 83 0 0 0 n\n"
                        "MouseMoveEvent 257 83 0 0 0 n\n"
                        "RenderEvent 257 83 0 0 0 n\n"
                        "MouseMoveEvent 258 83 0 0 0 n\n"
                        "RenderEvent 258 83 0 0 0 n\n"
                        "MouseMoveEvent 259 83 0 0 0 n\n"
                        "RenderEvent 259 83 0 0 0 n\n"
                        "MouseMoveEvent 259 83 0 0 0 n\n"
                        "RenderEvent 259 83 0 0 0 n\n"
                        "MouseMoveEvent 259 84 0 0 0 n\n"
                        "RenderEvent 259 84 0 0 0 n\n"
                        "MouseMoveEvent 261 84 0 0 0 n\n"
                        "RenderEvent 261 84 0 0 0 n\n"
                        "MouseMoveEvent 263 84 0 0 0 n\n"
                        "RenderEvent 263 84 0 0 0 n\n"
                        "MouseMoveEvent 266 84 0 0 0 n\n"
                        "RenderEvent 266 84 0 0 0 n\n"
                        "MouseMoveEvent 269 85 0 0 0 n\n"
                        "RenderEvent 269 85 0 0 0 n\n"
                        "MouseMoveEvent 272 86 0 0 0 n\n"
                        "RenderEvent 272 86 0 0 0 n\n"
                        "MouseMoveEvent 274 86 0 0 0 n\n"
                        "RenderEvent 274 86 0 0 0 n\n"
                        "MouseMoveEvent 277 87 0 0 0 n\n"
                        "RenderEvent 277 87 0 0 0 n\n"
                        "MouseMoveEvent 280 87 0 0 0 n\n"
                        "RenderEvent 280 87 0 0 0 n\n"
                        "MouseMoveEvent 282 88 0 0 0 n\n"
                        "RenderEvent 282 88 0 0 0 n\n"
                        "MouseMoveEvent 284 89 0 0 0 n\n"
                        "RenderEvent 284 89 0 0 0 n\n"
                        "MouseMoveEvent 286 89 0 0 0 n\n"
                        "RenderEvent 286 89 0 0 0 n\n"
                        "MouseMoveEvent 289 90 0 0 0 n\n"
                        "RenderEvent 289 90 0 0 0 n\n"
                        "MouseMoveEvent 291 90 0 0 0 n\n"
                        "RenderEvent 291 90 0 0 0 n\n"
                        "MouseMoveEvent 292 91 0 0 0 n\n"
                        "RenderEvent 292 91 0 0 0 n\n"
                        "MouseMoveEvent 293 91 0 0 0 n\n"
                        "RenderEvent 293 91 0 0 0 n\n"
                        "MouseMoveEvent 294 91 0 0 0 n\n"
                        "RenderEvent 294 91 0 0 0 n\n"
                        "MouseMoveEvent 294 91 0 0 0 n\n"
                        "RenderEvent 294 91 0 0 0 n\n"
                        "MouseMoveEvent 295 92 0 0 0 n\n"
                        "RenderEvent 295 92 0 0 0 n\n"
                        "MouseMoveEvent 296 92 0 0 0 n\n"
                        "RenderEvent 296 92 0 0 0 n\n"
                        "MouseMoveEvent 297 92 0 0 0 n\n"
                        "RenderEvent 297 92 0 0 0 n\n"
                        "MouseMoveEvent 297 92 0 0 0 n\n"
                        "RenderEvent 297 92 0 0 0 n\n"
                        "LeftButtonReleaseEvent 297 92 0 0 0 n\n"
                        "RenderEvent 297 92 0 0 0 n\n"
                        "MouseMoveEvent 297 92 0 0 0 n\n"
                        "MouseMoveEvent 296 92 0 0 0 n\n"
                        "MouseMoveEvent 295 92 0 0 0 n\n"
                        "MouseMoveEvent 294 92 0 0 0 n\n"
                        "MouseMoveEvent 293 92 0 0 0 n\n"
                        "MouseMoveEvent 292 92 0 0 0 n\n"
                        "MouseMoveEvent 291 92 0 0 0 n\n"
                        "MouseMoveEvent 290 92 0 0 0 n\n"
                        "MouseMoveEvent 289 92 0 0 0 n\n"
                        "MouseMoveEvent 288 92 0 0 0 n\n"
                        "MouseMoveEvent 287 92 0 0 0 n\n"
                        "MouseMoveEvent 286 92 0 0 0 n\n"
                        "MouseMoveEvent 285 92 0 0 0 n\n"
                        "MouseMoveEvent 284 92 0 0 0 n\n"
                        "MouseMoveEvent 283 92 0 0 0 n\n"
                        "MouseMoveEvent 282 92 0 0 0 n\n"
                        "MouseMoveEvent 281 92 0 0 0 n\n"
                        "MouseMoveEvent 280 92 0 0 0 n\n"
                        "MouseMoveEvent 279 92 0 0 0 n\n"
                        "MouseMoveEvent 278 92 0 0 0 n\n"
                        "MouseMoveEvent 277 92 0 0 0 n\n"
                        "MouseMoveEvent 276 92 0 0 0 n\n"
                        "MouseMoveEvent 276 91 0 0 0 n\n"
                        "MouseMoveEvent 275 91 0 0 0 n\n"
                        "MouseMoveEvent 274 91 0 0 0 n\n"
                        "MouseMoveEvent 273 91 0 0 0 n\n"
                        "MouseMoveEvent 272 91 0 0 0 n\n"
                        "MouseMoveEvent 271 91 0 0 0 n\n"
                        "MouseMoveEvent 270 91 0 0 0 n\n"
                        "MouseMoveEvent 269 91 0 0 0 n\n"
                        "MouseMoveEvent 268 91 0 0 0 n\n"
                        "MouseMoveEvent 267 91 0 0 0 n\n"
                        "MouseMoveEvent 266 91 0 0 0 n\n"
                        "MouseMoveEvent 265 91 0 0 0 n\n"
                        "RenderEvent 265 91 0 0 0 n\n"
                        "MouseMoveEvent 265 91 0 0 0 n\n"
                        "MouseMoveEvent 253 91 0 0 0 n\n"
                        "RenderEvent 253 91 0 0 0 n\n"
                        "MouseMoveEvent 253 91 0 0 0 n\n"
                        "MouseMoveEvent 239 91 0 0 0 n\n"
                        "RenderEvent 239 91 0 0 0 n\n"
                        "MouseMoveEvent 239 91 0 0 0 n\n"
                        "MouseMoveEvent 235 91 0 0 0 n\n"
                        "LeftButtonPressEvent 235 91 0 0 0 n\n"
                        "RenderEvent 235 91 0 0 0 n\n"
                        "MouseMoveEvent 235 92 0 0 0 n\n"
                        "RenderEvent 235 92 0 0 0 n\n"
                        "MouseMoveEvent 235 92 0 0 0 n\n"
                        "RenderEvent 235 92 0 0 0 n\n"
                        "MouseMoveEvent 234 93 0 0 0 n\n"
                        "RenderEvent 234 93 0 0 0 n\n"
                        "MouseMoveEvent 231 96 0 0 0 n\n"
                        "RenderEvent 231 96 0 0 0 n\n"
                        "MouseMoveEvent 227 100 0 0 0 n\n"
                        "RenderEvent 227 100 0 0 0 n\n"
                        "MouseMoveEvent 222 106 0 0 0 n\n"
                        "RenderEvent 222 106 0 0 0 n\n"
                        "MouseMoveEvent 216 111 0 0 0 n\n"
                        "RenderEvent 216 111 0 0 0 n\n"
                        "MouseMoveEvent 211 115 0 0 0 n\n"
                        "RenderEvent 211 115 0 0 0 n\n"
                        "MouseMoveEvent 207 119 0 0 0 n\n"
                        "RenderEvent 207 119 0 0 0 n\n"
                        "MouseMoveEvent 204 123 0 0 0 n\n"
                        "RenderEvent 204 123 0 0 0 n\n"
                        "MouseMoveEvent 200 126 0 0 0 n\n"
                        "RenderEvent 200 126 0 0 0 n\n"
                        "MouseMoveEvent 197 129 0 0 0 n\n"
                        "RenderEvent 197 129 0 0 0 n\n"
                        "MouseMoveEvent 195 132 0 0 0 n\n"
                        "RenderEvent 195 132 0 0 0 n\n"
                        "MouseMoveEvent 193 134 0 0 0 n\n"
                        "RenderEvent 193 134 0 0 0 n\n"
                        "MouseMoveEvent 192 135 0 0 0 n\n"
                        "RenderEvent 192 135 0 0 0 n\n"
                        "MouseMoveEvent 190 136 0 0 0 n\n"
                        "RenderEvent 190 136 0 0 0 n\n"
                        "MouseMoveEvent 188 138 0 0 0 n\n"
                        "RenderEvent 188 138 0 0 0 n\n"
                        "MouseMoveEvent 186 139 0 0 0 n\n"
                        "RenderEvent 186 139 0 0 0 n\n"
                        "MouseMoveEvent 186 139 0 0 0 n\n"
                        "RenderEvent 186 139 0 0 0 n\n"
                        "MouseMoveEvent 185 139 0 0 0 n\n"
                        "RenderEvent 185 139 0 0 0 n\n"
                        "MouseMoveEvent 185 140 0 0 0 n\n"
                        "RenderEvent 185 140 0 0 0 n\n"
                        "MouseMoveEvent 185 140 0 0 0 n\n"
                        "RenderEvent 185 140 0 0 0 n\n"
                        "LeftButtonReleaseEvent 185 140 0 0 0 n\n"
                        "RenderEvent 185 140 0 0 0 n\n"
                        "MouseMoveEvent 185 140 0 0 0 n\n"
                        "RenderEvent 185 140 0 0 0 n\n"
                        "MouseMoveEvent 185 140 0 0 0 n\n"
                        "MouseMoveEvent 219 134 0 0 0 n\n"
                        "RenderEvent 219 134 0 0 0 n\n"
                        "MouseMoveEvent 219 134 0 0 0 n\n"
                        "MouseMoveEvent 224 134 0 0 0 n\n"
                        "LeftButtonPressEvent 224 134 0 0 0 n\n"
                        "RenderEvent 224 134 0 0 0 n\n"
                        "MouseMoveEvent 224 134 0 0 0 n\n"
                        "RenderEvent 224 134 0 0 0 n\n"
                        "MouseMoveEvent 224 134 0 0 0 n\n"
                        "RenderEvent 224 134 0 0 0 n\n"
                        "MouseMoveEvent 224 134 0 0 0 n\n"
                        "RenderEvent 224 134 0 0 0 n\n"
                        "MouseMoveEvent 225 133 0 0 0 n\n"
                        "RenderEvent 225 133 0 0 0 n\n"
                        "MouseMoveEvent 226 132 0 0 0 n\n"
                        "RenderEvent 226 132 0 0 0 n\n"
                        "MouseMoveEvent 229 130 0 0 0 n\n"
                        "RenderEvent 229 130 0 0 0 n\n"
                        "MouseMoveEvent 234 127 0 0 0 n\n"
                        "RenderEvent 234 127 0 0 0 n\n"
                        "MouseMoveEvent 240 123 0 0 0 n\n"
                        "RenderEvent 240 123 0 0 0 n\n"
                        "MouseMoveEvent 246 119 0 0 0 n\n"
                        "RenderEvent 246 119 0 0 0 n\n"
                        "MouseMoveEvent 253 115 0 0 0 n\n"
                        "RenderEvent 253 115 0 0 0 n\n"
                        "MouseMoveEvent 259 111 0 0 0 n\n"
                        "RenderEvent 259 111 0 0 0 n\n"
                        "MouseMoveEvent 264 108 0 0 0 n\n"
                        "RenderEvent 264 108 0 0 0 n\n"
                        "MouseMoveEvent 267 106 0 0 0 n\n"
                        "RenderEvent 267 106 0 0 0 n\n"
                        "MouseMoveEvent 270 105 0 0 0 n\n"
                        "RenderEvent 270 105 0 0 0 n\n"
                        "MouseMoveEvent 271 103 0 0 0 n\n"
                        "RenderEvent 271 103 0 0 0 n\n"
                        "MouseMoveEvent 272 103 0 0 0 n\n"
                        "RenderEvent 272 103 0 0 0 n\n"
                        "MouseMoveEvent 273 102 0 0 0 n\n"
                        "RenderEvent 273 102 0 0 0 n\n"
                        "MouseMoveEvent 273 102 0 0 0 n\n"
                        "RenderEvent 273 102 0 0 0 n\n"
                        "MouseMoveEvent 274 101 0 0 0 n\n"
                        "RenderEvent 274 101 0 0 0 n\n"
                        "MouseMoveEvent 274 101 0 0 0 n\n"
                        "RenderEvent 274 101 0 0 0 n\n"
                        "MouseMoveEvent 274 101 0 0 0 n\n"
                        "RenderEvent 274 101 0 0 0 n\n"
                        "MouseMoveEvent 274 101 0 0 0 n\n"
                        "RenderEvent 274 101 0 0 0 n\n"
                        "MouseMoveEvent 274 101 0 0 0 n\n"
                        "RenderEvent 274 101 0 0 0 n\n"
                        "MouseMoveEvent 275 101 0 0 0 n\n"
                        "RenderEvent 275 101 0 0 0 n\n"
                        "MouseMoveEvent 275 101 0 0 0 n\n"
                        "RenderEvent 275 101 0 0 0 n\n"
                        "LeftButtonReleaseEvent 275 101 0 0 0 n\n"
                        "RenderEvent 275 101 0 0 0 n\n"
                        "MouseMoveEvent 275 101 0 0 0 n\n"
                        "MouseMoveEvent 272 101 0 0 0 n\n"
                        "RenderEvent 272 101 0 0 0 n\n"
                        "MouseMoveEvent 272 100 0 0 0 n\n"
                        "MouseMoveEvent 259 98 0 0 0 n\n"
                        "RenderEvent 259 98 0 0 0 n\n"
                        "MouseMoveEvent 258 98 0 0 0 n\n"
                        "MouseMoveEvent 254 96 0 0 0 n\n"
                        "MouseWheelBackwardEvent 254 96 0 0 0 n\n"
                        "StartInteractionEvent 254 96 0 0 0 n\n"
                        "RenderEvent 254 96 0 0 0 n\n"
                        "EndInteractionEvent 254 96 0 0 0 n\n"
                        "RenderEvent 254 96 0 0 0 n\n"
                        "MouseMoveEvent 254 96 0 0 0 n\n"
                        "RenderEvent 254 96 0 0 0 n\n"
                        "MouseMoveEvent 254 96 0 0 0 n\n"
                        "MouseWheelBackwardEvent 254 96 0 0 0 n\n"
                        "StartInteractionEvent 254 96 0 0 0 n\n"
                        "RenderEvent 254 96 0 0 0 n\n"
                        "EndInteractionEvent 254 96 0 0 0 n\n"
                        "RenderEvent 254 96 0 0 0 n\n"
                        "MouseMoveEvent 254 96 0 0 0 n\n"
                        "MouseMoveEvent 255 96 0 0 0 n\n"
                        "MouseWheelBackwardEvent 255 96 0 0 0 n\n"
                        "StartInteractionEvent 255 96 0 0 0 n\n"
                        "RenderEvent 255 96 0 0 0 n\n"
                        "EndInteractionEvent 255 96 0 0 0 n\n"
                        "RenderEvent 255 96 0 0 0 n\n"
                        "MouseMoveEvent 255 96 0 0 0 n\n"
                        "RenderEvent 255 96 0 0 0 n\n"
                        "MouseMoveEvent 255 96 0 0 0 n\n"
                        "MouseMoveEvent 299 177 0 0 0 n\n"
                        "LeaveEvent 300 178 0 0 0 n\n"
                        "EnterEvent 299 226 0 0 0 n\n"
                        "MouseMoveEvent 299 226 0 0 0 n\n"
                        "MouseMoveEvent 286 299 0 0 0 n\n"
                        "LeaveEvent 286 300 0 0 0 n";

// This does the actual work: updates the vtkPlane implicit function.
// This in turn causes the pipeline to update and clip the object.
// Callback for the interaction
class vtkTDSIPW2Callback : public vtkCommand
{
public:
  static vtkTDSIPW2Callback* New() { return new vtkTDSIPW2Callback; }

  void Execute(vtkObject* caller, unsigned long, void*) override
  {
    auto* planeWidget = reinterpret_cast<Widget*>(caller);
    auto* rep = reinterpret_cast<Representation*>(planeWidget->GetRepresentation());
    rep->GetPlane(this->Plane);
    this->Actor->VisibilityOn();
  }

  vtkTDSIPW2Callback()
    : Plane(nullptr)
    , Actor(nullptr)
  {
  }

  vtkPlane* Plane;
  vtkActor* Actor;
};

int TestDisplaySizedImplicitPlaneWidget(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  // Create a mace out of filters.
  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkConeSource> cone;
  vtkNew<vtkGlyph3D> glyph;
  glyph->SetInputConnection(sphere->GetOutputPort());
  glyph->SetSourceConnection(cone->GetOutputPort());
  glyph->SetVectorModeToUseNormal();
  glyph->SetScaleModeToScaleByVector();
  glyph->SetScaleFactor(0.25);
  glyph->Update();

  // The sphere and spikes are appended into a single polydata.
  // This just makes things simpler to manage.
  vtkNew<vtkAppendPolyData> apd;
  apd->AddInputConnection(glyph->GetOutputPort());
  apd->AddInputConnection(sphere->GetOutputPort());

  // This portion of the code clips the mace with the vtkPlanes
  // implicit function. The clipped region is colored green.
  vtkNew<vtkPlane> plane;
  vtkNew<vtkClipPolyData> clipper;
  clipper->SetInputConnection(apd->GetOutputPort());
  clipper->SetClipFunction(plane);
  clipper->InsideOutOn();

  vtkNew<vtkPolyDataMapper> selectMapper;
  selectMapper->SetInputConnection(clipper->GetOutputPort());

  vtkNew<vtkLODActor> selectActor;
  selectActor->SetMapper(selectMapper);

  // Create the RenderWindow, Renderer and both Actors
  vtkNew<vtkRenderer> ren1;
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(ren1);
  ren1->AddActor(selectActor);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // The SetInteractor method is how 3D widgets are associated with the render
  // window interactor. Internally, SetInteractor sets up a bunch of callbacks
  // using the Command/Observer mechanism (AddObserver()).
  vtkNew<vtkTDSIPW2Callback> myCallback;
  myCallback->Plane = plane;
  myCallback->Actor = selectActor;

  vtkNew<Representation> rep;
  rep->ScaleEnabledOn();
  rep->SetPlaceFactor(1.25); // This must be set prior to placing the widget
  rep->PlaceWidget(selectActor->GetBounds());
  rep->SetNormal(plane->GetNormal());
  // Some color variation for a white background
  // rep->SetSelectedWidgetColor(1, 0, 1);
  // rep->SetUnselectedWidgetColor(0, 1, 0);
  // rep->SetForegroundWidgetColor(0, 0, 1);
  rep->DrawOutlineOn();
  rep->DrawIntersectionEdgesOn();

  // ren1->SetBackground(1, 1, 1);

  vtkNew<Widget> planeWidget;
  planeWidget->SetInteractor(iren);
  planeWidget->SetRepresentation(rep);
  planeWidget->AddObserver(vtkCommand::InteractionEvent, myCallback);

  // Render
  iren->Initialize();
  renWin->Render();
  planeWidget->On();

  vtkNew<vtkInteractorEventRecorder> recorder;
  recorder->SetInteractor(iren);
#if 0 // uncomment if recording
  recorder->SetFileName("record.log");
  recorder->Record();
  recorder->On();

  iren->Initialize();
  renWin->Render();
  iren->Start();
#else
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(eventLog);

  // render the image
  iren->Initialize();
  renWin->Render();
  recorder->Play();

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();

  iren->Start();
#endif

  return EXIT_SUCCESS;
}
