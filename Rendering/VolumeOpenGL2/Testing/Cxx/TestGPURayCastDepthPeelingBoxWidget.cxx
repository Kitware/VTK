/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastDepthPeeling.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 *  Tests interactive clipping with volume peeling.
 */

#include <vtkBoxWidget.h>
#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkCommand.h>
#include <vtkDataArray.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkImageData.h>
#include <vtkImageReader.h>
#include <vtkImageShiftScale.h>
#include <vtkInteractorEventRecorder.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkNew.h>
#include <vtkOpenGLRenderer.h>
#include <vtkOutlineFilter.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPlaneCollection.h>
#include <vtkPlanes.h>
#include <vtkPointData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderTimerLog.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkTestUtilities.h>
#include <vtkTestingObjectFactory.h>
#include <vtkTimerLog.h>
#include <vtkVolumeProperty.h>
#include <vtkXMLImageDataReader.h>

#include <cassert>

namespace
{

// Callback for the interaction
class vtkBWCallback : public vtkCommand
{
public:
  vtkSmartPointer<vtkVolume> Volume;

  static vtkBWCallback* New() { return new vtkBWCallback; }

  void Execute(vtkObject* caller, unsigned long, void*) override
  {
    vtkBoxWidget* boxWidget = reinterpret_cast<vtkBoxWidget*>(caller);

    vtkNew<vtkPlanes> widgetPlanes;
    boxWidget->GetPlanes(widgetPlanes);
    this->Volume->GetMapper()->SetClippingPlanes(widgetPlanes);
  }
};

class SamplingDistanceCallback : public vtkCommand
{
public:
  static SamplingDistanceCallback* New() { return new SamplingDistanceCallback; }

  void Execute(vtkObject* vtkNotUsed(caller), unsigned long event, void* vtkNotUsed(data)) override
  {
    switch (event)
    {
      case vtkCommand::StartInteractionEvent:
      {
        // Higher ImageSampleDistance to make the volume-rendered image's
        // resolution visibly lower during interaction.
        this->Mapper->SetImageSampleDistance(6.5);
      }
      break;

      case vtkCommand::EndInteractionEvent:
      {
        // Default ImageSampleDistance
        this->Mapper->SetImageSampleDistance(1.0);
      }
    }
  }

  vtkGPUVolumeRayCastMapper* Mapper = nullptr;
};

const std::string EventStream =
R"eventStream(
#StreamVersion 1.1
LeftButtonPressEvent 198 296 0 0 0 0
RenderEvent 198 296 0 0 0 0
MouseMoveEvent 198 295 0 0 0 0
RenderEvent 198 295 0 0 0 0
MouseMoveEvent 198 295 0 0 0 0
RenderEvent 198 295 0 0 0 0
MouseMoveEvent 198 295 0 0 0 0
RenderEvent 198 295 0 0 0 0
MouseMoveEvent 198 294 0 0 0 0
RenderEvent 198 294 0 0 0 0
MouseMoveEvent 198 294 0 0 0 0
RenderEvent 198 294 0 0 0 0
MouseMoveEvent 198 292 0 0 0 0
RenderEvent 198 292 0 0 0 0
MouseMoveEvent 198 291 0 0 0 0
RenderEvent 198 291 0 0 0 0
MouseMoveEvent 198 289 0 0 0 0
RenderEvent 198 289 0 0 0 0
MouseMoveEvent 198 285 0 0 0 0
RenderEvent 198 285 0 0 0 0
MouseMoveEvent 198 283 0 0 0 0
RenderEvent 198 283 0 0 0 0
MouseMoveEvent 198 281 0 0 0 0
RenderEvent 198 281 0 0 0 0
MouseMoveEvent 198 279 0 0 0 0
RenderEvent 198 279 0 0 0 0
MouseMoveEvent 197 276 0 0 0 0
RenderEvent 197 276 0 0 0 0
MouseMoveEvent 197 275 0 0 0 0
RenderEvent 197 275 0 0 0 0
MouseMoveEvent 197 274 0 0 0 0
RenderEvent 197 274 0 0 0 0
MouseMoveEvent 197 273 0 0 0 0
RenderEvent 197 273 0 0 0 0
MouseMoveEvent 197 271 0 0 0 0
RenderEvent 197 271 0 0 0 0
MouseMoveEvent 197 266 0 0 0 0
RenderEvent 197 266 0 0 0 0
MouseMoveEvent 197 260 0 0 0 0
RenderEvent 197 260 0 0 0 0
MouseMoveEvent 197 255 0 0 0 0
RenderEvent 197 255 0 0 0 0
MouseMoveEvent 197 247 0 0 0 0
RenderEvent 197 247 0 0 0 0
MouseMoveEvent 197 243 0 0 0 0
RenderEvent 197 243 0 0 0 0
MouseMoveEvent 197 237 0 0 0 0
RenderEvent 197 237 0 0 0 0
MouseMoveEvent 197 231 0 0 0 0
RenderEvent 197 231 0 0 0 0
MouseMoveEvent 197 227 0 0 0 0
RenderEvent 197 227 0 0 0 0
MouseMoveEvent 197 224 0 0 0 0
RenderEvent 197 224 0 0 0 0
MouseMoveEvent 197 221 0 0 0 0
RenderEvent 197 221 0 0 0 0
MouseMoveEvent 197 220 0 0 0 0
RenderEvent 197 220 0 0 0 0
MouseMoveEvent 197 218 0 0 0 0
RenderEvent 197 218 0 0 0 0
MouseMoveEvent 197 217 0 0 0 0
RenderEvent 197 217 0 0 0 0
MouseMoveEvent 197 216 0 0 0 0
RenderEvent 197 216 0 0 0 0
MouseMoveEvent 197 215 0 0 0 0
RenderEvent 197 215 0 0 0 0
MouseMoveEvent 197 215 0 0 0 0
RenderEvent 197 215 0 0 0 0
MouseMoveEvent 197 214 0 0 0 0
RenderEvent 197 214 0 0 0 0
MouseMoveEvent 197 214 0 0 0 0
RenderEvent 197 214 0 0 0 0
MouseMoveEvent 197 213 0 0 0 0
RenderEvent 197 213 0 0 0 0
MouseMoveEvent 197 213 0 0 0 0
RenderEvent 197 213 0 0 0 0
MouseMoveEvent 197 212 0 0 0 0
RenderEvent 197 212 0 0 0 0
MouseMoveEvent 197 212 0 0 0 0
RenderEvent 197 212 0 0 0 0
MouseMoveEvent 197 212 0 0 0 0
RenderEvent 197 212 0 0 0 0
MouseMoveEvent 197 211 0 0 0 0
RenderEvent 197 211 0 0 0 0
MouseMoveEvent 197 211 0 0 0 0
RenderEvent 197 211 0 0 0 0
MouseMoveEvent 197 210 0 0 0 0
RenderEvent 197 210 0 0 0 0
MouseMoveEvent 197 209 0 0 0 0
RenderEvent 197 209 0 0 0 0
MouseMoveEvent 197 208 0 0 0 0
RenderEvent 197 208 0 0 0 0
MouseMoveEvent 197 206 0 0 0 0
RenderEvent 197 206 0 0 0 0
MouseMoveEvent 196 205 0 0 0 0
RenderEvent 196 205 0 0 0 0
MouseMoveEvent 196 205 0 0 0 0
RenderEvent 196 205 0 0 0 0
MouseMoveEvent 196 204 0 0 0 0
RenderEvent 196 204 0 0 0 0
MouseMoveEvent 196 204 0 0 0 0
RenderEvent 196 204 0 0 0 0
MouseMoveEvent 196 201 0 0 0 0
RenderEvent 196 201 0 0 0 0
MouseMoveEvent 196 200 0 0 0 0
RenderEvent 196 200 0 0 0 0
MouseMoveEvent 196 199 0 0 0 0
RenderEvent 196 199 0 0 0 0
MouseMoveEvent 196 197 0 0 0 0
RenderEvent 196 197 0 0 0 0
MouseMoveEvent 196 195 0 0 0 0
RenderEvent 196 195 0 0 0 0
MouseMoveEvent 195 193 0 0 0 0
RenderEvent 195 193 0 0 0 0
MouseMoveEvent 195 192 0 0 0 0
RenderEvent 195 192 0 0 0 0
MouseMoveEvent 195 191 0 0 0 0
RenderEvent 195 191 0 0 0 0
MouseMoveEvent 195 190 0 0 0 0
RenderEvent 195 190 0 0 0 0
LeftButtonReleaseEvent 195 190 0 0 0 0
RenderEvent 195 190 0 0 0 0
MouseMoveEvent 196 189 0 0 0 0
MouseMoveEvent 196 189 0 0 0 0
MouseMoveEvent 197 188 0 0 0 0
MouseMoveEvent 197 187 0 0 0 0
MouseMoveEvent 198 187 0 0 0 0
MouseMoveEvent 199 186 0 0 0 0
MouseMoveEvent 200 185 0 0 0 0
MouseMoveEvent 202 185 0 0 0 0
MouseMoveEvent 204 185 0 0 0 0
MouseMoveEvent 205 183 0 0 0 0
MouseMoveEvent 206 182 0 0 0 0
MouseMoveEvent 208 181 0 0 0 0
MouseMoveEvent 210 180 0 0 0 0
MouseMoveEvent 212 177 0 0 0 0
MouseMoveEvent 214 176 0 0 0 0
MouseMoveEvent 216 175 0 0 0 0
MouseMoveEvent 218 172 0 0 0 0
MouseMoveEvent 219 170 0 0 0 0
MouseMoveEvent 222 168 0 0 0 0
MouseMoveEvent 225 166 0 0 0 0
MouseMoveEvent 226 163 0 0 0 0
MouseMoveEvent 229 160 0 0 0 0
MouseMoveEvent 231 158 0 0 0 0
MouseMoveEvent 233 155 0 0 0 0
MouseMoveEvent 236 152 0 0 0 0
MouseMoveEvent 237 151 0 0 0 0
MouseMoveEvent 240 148 0 0 0 0
MouseMoveEvent 241 145 0 0 0 0
MouseMoveEvent 244 144 0 0 0 0
MouseMoveEvent 245 141 0 0 0 0
MouseMoveEvent 247 140 0 0 0 0
MouseMoveEvent 248 138 0 0 0 0
MouseMoveEvent 251 137 0 0 0 0
MouseMoveEvent 252 134 0 0 0 0
MouseMoveEvent 254 133 0 0 0 0
MouseMoveEvent 255 131 0 0 0 0
MouseMoveEvent 256 130 0 0 0 0
MouseMoveEvent 258 129 0 0 0 0
MouseMoveEvent 259 128 0 0 0 0
MouseMoveEvent 260 126 0 0 0 0
MouseMoveEvent 261 125 0 0 0 0
MouseMoveEvent 261 126 0 0 0 0
MouseMoveEvent 261 127 0 0 0 0
MouseMoveEvent 260 127 0 0 0 0
MouseMoveEvent 260 128 0 0 0 0
MouseMoveEvent 260 129 0 0 0 0
MouseMoveEvent 260 130 0 0 0 0
MouseMoveEvent 261 131 0 0 0 0
MouseMoveEvent 261 131 0 0 0 0
MouseMoveEvent 262 131 0 0 0 0
MouseMoveEvent 262 132 0 0 0 0
MouseMoveEvent 262 132 0 0 0 0
MouseMoveEvent 262 133 0 0 0 0
MouseMoveEvent 263 133 0 0 0 0
MouseMoveEvent 264 133 0 0 0 0
MouseMoveEvent 264 133 0 0 0 0
MouseMoveEvent 265 134 0 0 0 0
MouseMoveEvent 265 134 0 0 0 0
MouseMoveEvent 266 134 0 0 0 0
MouseMoveEvent 266 134 0 0 0 0
MouseMoveEvent 266 134 0 0 0 0
LeftButtonPressEvent 266 134 0 0 0 0
RenderEvent 266 134 0 0 0 0
MouseMoveEvent 266 134 0 0 0 0
RenderEvent 266 134 0 0 0 0
MouseMoveEvent 265 135 0 0 0 0
RenderEvent 265 135 0 0 0 0
)eventStream"
    // We have to break this string up, otherwise MSVC whines about it being
    // too long (error C2026)
R"eventStream(
MouseMoveEvent 262 137 0 0 0 0
RenderEvent 262 137 0 0 0 0
MouseMoveEvent 260 138 0 0 0 0
RenderEvent 260 138 0 0 0 0
MouseMoveEvent 258 140 0 0 0 0
RenderEvent 258 140 0 0 0 0
MouseMoveEvent 257 141 0 0 0 0
RenderEvent 257 141 0 0 0 0
MouseMoveEvent 255 142 0 0 0 0
RenderEvent 255 142 0 0 0 0
MouseMoveEvent 253 143 0 0 0 0
RenderEvent 253 143 0 0 0 0
MouseMoveEvent 250 145 0 0 0 0
RenderEvent 250 145 0 0 0 0
MouseMoveEvent 248 145 0 0 0 0
RenderEvent 248 145 0 0 0 0
MouseMoveEvent 248 146 0 0 0 0
RenderEvent 248 146 0 0 0 0
MouseMoveEvent 246 146 0 0 0 0
RenderEvent 246 146 0 0 0 0
MouseMoveEvent 244 147 0 0 0 0
RenderEvent 244 147 0 0 0 0
MouseMoveEvent 243 148 0 0 0 0
RenderEvent 243 148 0 0 0 0
MouseMoveEvent 242 148 0 0 0 0
RenderEvent 242 148 0 0 0 0
MouseMoveEvent 241 149 0 0 0 0
RenderEvent 241 149 0 0 0 0
MouseMoveEvent 240 149 0 0 0 0
RenderEvent 240 149 0 0 0 0
MouseMoveEvent 239 150 0 0 0 0
RenderEvent 239 150 0 0 0 0
MouseMoveEvent 238 151 0 0 0 0
RenderEvent 238 151 0 0 0 0
MouseMoveEvent 237 152 0 0 0 0
RenderEvent 237 152 0 0 0 0
MouseMoveEvent 236 152 0 0 0 0
RenderEvent 236 152 0 0 0 0
MouseMoveEvent 236 152 0 0 0 0
RenderEvent 236 152 0 0 0 0
MouseMoveEvent 236 153 0 0 0 0
RenderEvent 236 153 0 0 0 0
MouseMoveEvent 235 153 0 0 0 0
RenderEvent 235 153 0 0 0 0
MouseMoveEvent 234 153 0 0 0 0
RenderEvent 234 153 0 0 0 0
MouseMoveEvent 234 153 0 0 0 0
RenderEvent 234 153 0 0 0 0
MouseMoveEvent 233 154 0 0 0 0
RenderEvent 233 154 0 0 0 0
MouseMoveEvent 232 154 0 0 0 0
RenderEvent 232 154 0 0 0 0
MouseMoveEvent 231 154 0 0 0 0
RenderEvent 231 154 0 0 0 0
MouseMoveEvent 231 155 0 0 0 0
RenderEvent 231 155 0 0 0 0
MouseMoveEvent 231 155 0 0 0 0
RenderEvent 231 155 0 0 0 0
MouseMoveEvent 230 155 0 0 0 0
RenderEvent 230 155 0 0 0 0
MouseMoveEvent 230 156 0 0 0 0
RenderEvent 230 156 0 0 0 0
MouseMoveEvent 229 156 0 0 0 0
RenderEvent 229 156 0 0 0 0
MouseMoveEvent 229 157 0 0 0 0
RenderEvent 229 157 0 0 0 0
MouseMoveEvent 228 157 0 0 0 0
RenderEvent 228 157 0 0 0 0
MouseMoveEvent 228 157 0 0 0 0
RenderEvent 228 157 0 0 0 0
MouseMoveEvent 228 158 0 0 0 0
RenderEvent 228 158 0 0 0 0
MouseMoveEvent 227 158 0 0 0 0
RenderEvent 227 158 0 0 0 0
MouseMoveEvent 227 158 0 0 0 0
RenderEvent 227 158 0 0 0 0
MouseMoveEvent 226 158 0 0 0 0
RenderEvent 226 158 0 0 0 0
MouseMoveEvent 226 158 0 0 0 0
RenderEvent 226 158 0 0 0 0
MouseMoveEvent 226 159 0 0 0 0
RenderEvent 226 159 0 0 0 0
MouseMoveEvent 225 159 0 0 0 0
RenderEvent 225 159 0 0 0 0
MouseMoveEvent 225 159 0 0 0 0
RenderEvent 225 159 0 0 0 0
MouseMoveEvent 225 159 0 0 0 0
RenderEvent 225 159 0 0 0 0
MouseMoveEvent 225 160 0 0 0 0
RenderEvent 225 160 0 0 0 0
MouseMoveEvent 224 160 0 0 0 0
RenderEvent 224 160 0 0 0 0
MouseMoveEvent 223 160 0 0 0 0
RenderEvent 223 160 0 0 0 0
MouseMoveEvent 223 160 0 0 0 0
RenderEvent 223 160 0 0 0 0
MouseMoveEvent 223 160 0 0 0 0
RenderEvent 223 160 0 0 0 0
MouseMoveEvent 222 160 0 0 0 0
RenderEvent 222 160 0 0 0 0
MouseMoveEvent 222 161 0 0 0 0
RenderEvent 222 161 0 0 0 0
MouseMoveEvent 222 161 0 0 0 0
RenderEvent 222 161 0 0 0 0
MouseMoveEvent 221 161 0 0 0 0
RenderEvent 221 161 0 0 0 0
MouseMoveEvent 221 162 0 0 0 0
RenderEvent 221 162 0 0 0 0
MouseMoveEvent 220 162 0 0 0 0
RenderEvent 220 162 0 0 0 0
MouseMoveEvent 220 162 0 0 0 0
RenderEvent 220 162 0 0 0 0
MouseMoveEvent 220 162 0 0 0 0
RenderEvent 220 162 0 0 0 0
MouseMoveEvent 219 162 0 0 0 0
RenderEvent 219 162 0 0 0 0
MouseMoveEvent 218 162 0 0 0 0
RenderEvent 218 162 0 0 0 0
MouseMoveEvent 218 163 0 0 0 0
RenderEvent 218 163 0 0 0 0
LeftButtonReleaseEvent 218 163 0 0 0 0
RenderEvent 218 163 0 0 0 0
MouseMoveEvent 218 163 0 0 0 0
MouseMoveEvent 219 163 0 0 0 0
MouseMoveEvent 219 162 0 0 0 0
MouseMoveEvent 220 162 0 0 0 0
MouseMoveEvent 220 161 0 0 0 0
MouseMoveEvent 221 160 0 0 0 0
MouseMoveEvent 222 159 0 0 0 0
MouseMoveEvent 222 158 0 0 0 0
MouseMoveEvent 223 158 0 0 0 0
MouseMoveEvent 223 157 0 0 0 0
MouseMoveEvent 224 156 0 0 0 0
MouseMoveEvent 225 154 0 0 0 0
MouseMoveEvent 226 153 0 0 0 0
MouseMoveEvent 226 150 0 0 0 0
MouseMoveEvent 227 149 0 0 0 0
MouseMoveEvent 228 147 0 0 0 0
MouseMoveEvent 229 146 0 0 0 0
MouseMoveEvent 230 143 0 0 0 0
MouseMoveEvent 232 141 0 0 0 0
MouseMoveEvent 233 140 0 0 0 0
MouseMoveEvent 234 137 0 0 0 0
MouseMoveEvent 235 135 0 0 0 0
MouseMoveEvent 235 132 0 0 0 0
MouseMoveEvent 236 130 0 0 0 0
MouseMoveEvent 238 127 0 0 0 0
MouseMoveEvent 238 126 0 0 0 0
MouseMoveEvent 239 124 0 0 0 0
MouseMoveEvent 240 121 0 0 0 0
MouseMoveEvent 240 120 0 0 0 0
MouseMoveEvent 241 118 0 0 0 0
MouseMoveEvent 241 116 0 0 0 0
MouseMoveEvent 241 115 0 0 0 0
MouseMoveEvent 241 113 0 0 0 0
MouseMoveEvent 241 111 0 0 0 0
MouseMoveEvent 243 110 0 0 0 0
MouseMoveEvent 243 108 0 0 0 0
MouseMoveEvent 243 107 0 0 0 0
MouseMoveEvent 243 106 0 0 0 0
MouseMoveEvent 243 105 0 0 0 0
MouseMoveEvent 243 103 0 0 0 0
MouseMoveEvent 243 102 0 0 0 0
MouseMoveEvent 243 101 0 0 0 0
MouseMoveEvent 243 100 0 0 0 0
MouseMoveEvent 243 99 0 0 0 0
MouseMoveEvent 243 98 0 0 0 0
MouseMoveEvent 243 97 0 0 0 0
MouseMoveEvent 243 96 0 0 0 0
MouseMoveEvent 243 95 0 0 0 0
MouseMoveEvent 243 94 0 0 0 0
MouseMoveEvent 243 93 0 0 0 0
MouseMoveEvent 243 92 0 0 0 0
MouseMoveEvent 243 91 0 0 0 0
MouseMoveEvent 243 90 0 0 0 0
MouseMoveEvent 243 89 0 0 0 0
MouseMoveEvent 243 88 0 0 0 0
MouseMoveEvent 243 86 0 0 0 0
MouseMoveEvent 243 85 0 0 0 0
MouseMoveEvent 243 83 0 0 0 0
MouseMoveEvent 243 81 0 0 0 0
MouseMoveEvent 243 80 0 0 0 0
MouseMoveEvent 243 78 0 0 0 0
MouseMoveEvent 243 76 0 0 0 0
MouseMoveEvent 243 75 0 0 0 0
MouseMoveEvent 243 73 0 0 0 0
MouseMoveEvent 243 72 0 0 0 0
MouseMoveEvent 243 70 0 0 0 0
MouseMoveEvent 243 69 0 0 0 0
MouseMoveEvent 243 68 0 0 0 0
MouseMoveEvent 243 67 0 0 0 0
MouseMoveEvent 243 66 0 0 0 0
MouseMoveEvent 243 65 0 0 0 0
MouseMoveEvent 243 64 0 0 0 0
MouseMoveEvent 243 62 0 0 0 0
MouseMoveEvent 243 61 0 0 0 0
MouseMoveEvent 244 60 0 0 0 0
MouseMoveEvent 244 59 0 0 0 0
MouseMoveEvent 244 58 0 0 0 0
MouseMoveEvent 244 57 0 0 0 0
MouseMoveEvent 244 56 0 0 0 0
MouseMoveEvent 244 55 0 0 0 0
MouseMoveEvent 244 54 0 0 0 0
LeftButtonPressEvent 244 54 0 0 0 0
StartInteractionEvent 244 54 0 0 0 0
MouseMoveEvent 245 54 0 0 0 0
RenderEvent 245 54 0 0 0 0
InteractionEvent 245 54 0 0 0 0
MouseMoveEvent 248 54 0 0 0 0
RenderEvent 248 54 0 0 0 0
InteractionEvent 248 54 0 0 0 0
MouseMoveEvent 255 57 0 0 0 0
RenderEvent 255 57 0 0 0 0
InteractionEvent 255 57 0 0 0 0
MouseMoveEvent 259 60 0 0 0 0
RenderEvent 259 60 0 0 0 0
InteractionEvent 259 60 0 0 0 0
MouseMoveEvent 264 63 0 0 0 0
RenderEvent 264 63 0 0 0 0
InteractionEvent 264 63 0 0 0 0
MouseMoveEvent 267 65 0 0 0 0
RenderEvent 267 65 0 0 0 0
InteractionEvent 267 65 0 0 0 0
MouseMoveEvent 269 67 0 0 0 0
RenderEvent 269 67 0 0 0 0
InteractionEvent 269 67 0 0 0 0
MouseMoveEvent 272 69 0 0 0 0
RenderEvent 272 69 0 0 0 0
InteractionEvent 272 69 0 0 0 0
MouseMoveEvent 274 70 0 0 0 0
RenderEvent 274 70 0 0 0 0
InteractionEvent 274 70 0 0 0 0
MouseMoveEvent 276 71 0 0 0 0
RenderEvent 276 71 0 0 0 0
InteractionEvent 276 71 0 0 0 0
MouseMoveEvent 277 73 0 0 0 0
RenderEvent 277 73 0 0 0 0
InteractionEvent 277 73 0 0 0 0
MouseMoveEvent 278 73 0 0 0 0
RenderEvent 278 73 0 0 0 0
InteractionEvent 278 73 0 0 0 0
MouseMoveEvent 278 74 0 0 0 0
RenderEvent 278 74 0 0 0 0
InteractionEvent 278 74 0 0 0 0
MouseMoveEvent 279 74 0 0 0 0
RenderEvent 279 74 0 0 0 0
InteractionEvent 279 74 0 0 0 0
MouseMoveEvent 280 75 0 0 0 0
RenderEvent 280 75 0 0 0 0
InteractionEvent 280 75 0 0 0 0
MouseMoveEvent 281 75 0 0 0 0
RenderEvent 281 75 0 0 0 0
InteractionEvent 281 75 0 0 0 0
MouseMoveEvent 282 77 0 0 0 0
RenderEvent 282 77 0 0 0 0
InteractionEvent 282 77 0 0 0 0
MouseMoveEvent 284 77 0 0 0 0
RenderEvent 284 77 0 0 0 0
InteractionEvent 284 77 0 0 0 0
MouseMoveEvent 286 78 0 0 0 0
RenderEvent 286 78 0 0 0 0
InteractionEvent 286 78 0 0 0 0
MouseMoveEvent 288 80 0 0 0 0
RenderEvent 288 80 0 0 0 0
InteractionEvent 288 80 0 0 0 0
MouseMoveEvent 290 81 0 0 0 0
RenderEvent 290 81 0 0 0 0
InteractionEvent 290 81 0 0 0 0
MouseMoveEvent 292 82 0 0 0 0
RenderEvent 292 82 0 0 0 0
InteractionEvent 292 82 0 0 0 0
MouseMoveEvent 293 83 0 0 0 0
RenderEvent 293 83 0 0 0 0
InteractionEvent 293 83 0 0 0 0
MouseMoveEvent 295 84 0 0 0 0
RenderEvent 295 84 0 0 0 0
InteractionEvent 295 84 0 0 0 0
MouseMoveEvent 297 86 0 0 0 0
RenderEvent 297 86 0 0 0 0
InteractionEvent 297 86 0 0 0 0
MouseMoveEvent 298 86 0 0 0 0
RenderEvent 298 86 0 0 0 0
InteractionEvent 298 86 0 0 0 0
MouseMoveEvent 299 87 0 0 0 0
RenderEvent 299 87 0 0 0 0
InteractionEvent 299 87 0 0 0 0
MouseMoveEvent 300 88 0 0 0 0
RenderEvent 300 88 0 0 0 0
InteractionEvent 300 88 0 0 0 0
MouseMoveEvent 301 88 0 0 0 0
RenderEvent 301 88 0 0 0 0
InteractionEvent 301 88 0 0 0 0
MouseMoveEvent 303 90 0 0 0 0
RenderEvent 303 90 0 0 0 0
InteractionEvent 303 90 0 0 0 0
MouseMoveEvent 304 90 0 0 0 0
RenderEvent 304 90 0 0 0 0
InteractionEvent 304 90 0 0 0 0
MouseMoveEvent 305 91 0 0 0 0
RenderEvent 305 91 0 0 0 0
InteractionEvent 305 91 0 0 0 0
MouseMoveEvent 307 92 0 0 0 0
RenderEvent 307 92 0 0 0 0
InteractionEvent 307 92 0 0 0 0
MouseMoveEvent 308 93 0 0 0 0
RenderEvent 308 93 0 0 0 0
InteractionEvent 308 93 0 0 0 0
MouseMoveEvent 309 94 0 0 0 0
RenderEvent 309 94 0 0 0 0
InteractionEvent 309 94 0 0 0 0
MouseMoveEvent 311 94 0 0 0 0
RenderEvent 311 94 0 0 0 0
InteractionEvent 311 94 0 0 0 0
MouseMoveEvent 314 95 0 0 0 0
RenderEvent 314 95 0 0 0 0
InteractionEvent 314 95 0 0 0 0
MouseMoveEvent 319 97 0 0 0 0
RenderEvent 319 97 0 0 0 0
InteractionEvent 319 97 0 0 0 0
MouseMoveEvent 322 98 0 0 0 0
RenderEvent 322 98 0 0 0 0
InteractionEvent 322 98 0 0 0 0
MouseMoveEvent 324 99 0 0 0 0
RenderEvent 324 99 0 0 0 0
InteractionEvent 324 99 0 0 0 0
MouseMoveEvent 325 99 0 0 0 0
RenderEvent 325 99 0 0 0 0
InteractionEvent 325 99 0 0 0 0
MouseMoveEvent 325 100 0 0 0 0
RenderEvent 325 100 0 0 0 0
InteractionEvent 325 100 0 0 0 0
MouseMoveEvent 325 101 0 0 0 0
)eventStream"
    // We have to break this string up, otherwise MSVC whines about it being
    // too long (error C2026)
R"eventStream(
RenderEvent 325 101 0 0 0 0
InteractionEvent 325 101 0 0 0 0
MouseMoveEvent 325 102 0 0 0 0
RenderEvent 325 102 0 0 0 0
InteractionEvent 325 102 0 0 0 0
MouseMoveEvent 324 102 0 0 0 0
RenderEvent 324 102 0 0 0 0
InteractionEvent 324 102 0 0 0 0
MouseMoveEvent 324 103 0 0 0 0
RenderEvent 324 103 0 0 0 0
InteractionEvent 324 103 0 0 0 0
MouseMoveEvent 324 103 0 0 0 0
RenderEvent 324 103 0 0 0 0
InteractionEvent 324 103 0 0 0 0
MouseMoveEvent 324 104 0 0 0 0
RenderEvent 324 104 0 0 0 0
InteractionEvent 324 104 0 0 0 0
MouseMoveEvent 324 104 0 0 0 0
RenderEvent 324 104 0 0 0 0
InteractionEvent 324 104 0 0 0 0
MouseMoveEvent 324 105 0 0 0 0
RenderEvent 324 105 0 0 0 0
InteractionEvent 324 105 0 0 0 0
MouseMoveEvent 325 105 0 0 0 0
RenderEvent 325 105 0 0 0 0
InteractionEvent 325 105 0 0 0 0
MouseMoveEvent 325 106 0 0 0 0
RenderEvent 325 106 0 0 0 0
InteractionEvent 325 106 0 0 0 0
MouseMoveEvent 325 106 0 0 0 0
RenderEvent 325 106 0 0 0 0
InteractionEvent 325 106 0 0 0 0
MouseMoveEvent 325 107 0 0 0 0
RenderEvent 325 107 0 0 0 0
InteractionEvent 325 107 0 0 0 0
MouseMoveEvent 327 108 0 0 0 0
RenderEvent 327 108 0 0 0 0
InteractionEvent 327 108 0 0 0 0
MouseMoveEvent 327 109 0 0 0 0
RenderEvent 327 109 0 0 0 0
InteractionEvent 327 109 0 0 0 0
MouseMoveEvent 328 109 0 0 0 0
RenderEvent 328 109 0 0 0 0
InteractionEvent 328 109 0 0 0 0
MouseMoveEvent 328 110 0 0 0 0
RenderEvent 328 110 0 0 0 0
InteractionEvent 328 110 0 0 0 0
MouseMoveEvent 328 111 0 0 0 0
RenderEvent 328 111 0 0 0 0
InteractionEvent 328 111 0 0 0 0
MouseMoveEvent 329 111 0 0 0 0
RenderEvent 329 111 0 0 0 0
InteractionEvent 329 111 0 0 0 0
MouseMoveEvent 329 112 0 0 0 0
RenderEvent 329 112 0 0 0 0
InteractionEvent 329 112 0 0 0 0
MouseMoveEvent 330 113 0 0 0 0
RenderEvent 330 113 0 0 0 0
InteractionEvent 330 113 0 0 0 0
MouseMoveEvent 330 114 0 0 0 0
RenderEvent 330 114 0 0 0 0
InteractionEvent 330 114 0 0 0 0
MouseMoveEvent 331 115 0 0 0 0
RenderEvent 331 115 0 0 0 0
InteractionEvent 331 115 0 0 0 0
MouseMoveEvent 331 115 0 0 0 0
RenderEvent 331 115 0 0 0 0
InteractionEvent 331 115 0 0 0 0
MouseMoveEvent 331 116 0 0 0 0
RenderEvent 331 116 0 0 0 0
InteractionEvent 331 116 0 0 0 0
MouseMoveEvent 331 116 0 0 0 0
RenderEvent 331 116 0 0 0 0
InteractionEvent 331 116 0 0 0 0
MouseMoveEvent 331 117 0 0 0 0
RenderEvent 331 117 0 0 0 0
InteractionEvent 331 117 0 0 0 0
MouseMoveEvent 331 118 0 0 0 0
RenderEvent 331 118 0 0 0 0
InteractionEvent 331 118 0 0 0 0
MouseMoveEvent 331 118 0 0 0 0
RenderEvent 331 118 0 0 0 0
InteractionEvent 331 118 0 0 0 0
MouseMoveEvent 331 119 0 0 0 0
RenderEvent 331 119 0 0 0 0
InteractionEvent 331 119 0 0 0 0
MouseMoveEvent 331 120 0 0 0 0
RenderEvent 331 120 0 0 0 0
InteractionEvent 331 120 0 0 0 0
MouseMoveEvent 332 120 0 0 0 0
RenderEvent 332 120 0 0 0 0
InteractionEvent 332 120 0 0 0 0
MouseMoveEvent 332 120 0 0 0 0
RenderEvent 332 120 0 0 0 0
InteractionEvent 332 120 0 0 0 0
LeftButtonReleaseEvent 332 120 0 0 0 0
EndInteractionEvent 332 120 0 0 0 0
RenderEvent 332 120 0 0 0 0
MouseMoveEvent 332 120 0 0 0 0
MouseMoveEvent 331 120 0 0 0 0
MouseMoveEvent 331 120 0 0 0 0
MouseMoveEvent 330 120 0 0 0 0
MouseMoveEvent 330 120 0 0 0 0
MouseMoveEvent 330 120 0 0 0 0
MouseMoveEvent 330 120 0 0 0 0
MouseMoveEvent 329 120 0 0 0 0
MouseMoveEvent 328 120 0 0 0 0
MouseMoveEvent 327 119 0 0 0 0
MouseMoveEvent 326 119 0 0 0 0
MouseMoveEvent 325 119 0 0 0 0
MouseMoveEvent 324 119 0 0 0 0
MouseMoveEvent 323 119 0 0 0 0
MouseMoveEvent 322 119 0 0 0 0
MouseMoveEvent 321 118 0 0 0 0
MouseMoveEvent 320 118 0 0 0 0
MouseMoveEvent 319 118 0 0 0 0
MouseMoveEvent 318 118 0 0 0 0
MouseMoveEvent 317 117 0 0 0 0
MouseMoveEvent 316 117 0 0 0 0
MouseMoveEvent 315 117 0 0 0 0
MouseMoveEvent 314 117 0 0 0 0
MouseMoveEvent 314 116 0 0 0 0
MouseMoveEvent 313 115 0 0 0 0
MouseMoveEvent 312 114 0 0 0 0
MouseMoveEvent 312 113 0 0 0 0
MouseMoveEvent 311 113 0 0 0 0
MouseMoveEvent 310 112 0 0 0 0
MouseMoveEvent 309 111 0 0 0 0
MouseMoveEvent 308 111 0 0 0 0
MouseMoveEvent 307 110 0 0 0 0
MouseMoveEvent 306 109 0 0 0 0
MouseMoveEvent 305 109 0 0 0 0
MouseMoveEvent 304 108 0 0 0 0
MouseMoveEvent 303 108 0 0 0 0
MouseMoveEvent 302 107 0 0 0 0
MouseMoveEvent 301 107 0 0 0 0
MouseMoveEvent 300 106 0 0 0 0
MouseMoveEvent 299 106 0 0 0 0
MouseMoveEvent 298 105 0 0 0 0
MouseMoveEvent 297 105 0 0 0 0
MouseMoveEvent 296 105 0 0 0 0
MouseMoveEvent 295 104 0 0 0 0
MouseMoveEvent 294 104 0 0 0 0
MouseMoveEvent 293 103 0 0 0 0
MouseMoveEvent 292 103 0 0 0 0
MouseMoveEvent 291 103 0 0 0 0
MouseMoveEvent 290 102 0 0 0 0
MouseMoveEvent 289 102 0 0 0 0
MouseMoveEvent 288 102 0 0 0 0
MouseMoveEvent 287 101 0 0 0 0
MouseMoveEvent 286 101 0 0 0 0
MouseMoveEvent 285 101 0 0 0 0
MouseMoveEvent 284 101 0 0 0 0
MouseMoveEvent 283 100 0 0 0 0
MouseMoveEvent 282 100 0 0 0 0
MouseMoveEvent 281 100 0 0 0 0
MouseMoveEvent 280 99 0 0 0 0
MouseMoveEvent 279 99 0 0 0 0
MouseMoveEvent 278 99 0 0 0 0
MouseMoveEvent 277 99 0 0 0 0
MouseMoveEvent 276 99 0 0 0 0
MouseMoveEvent 276 98 0 0 0 0
MouseMoveEvent 275 98 0 0 0 0
MouseMoveEvent 274 98 0 0 0 0
MouseWheelForwardEvent 274 98 0 0 0 0
StartInteractionEvent 274 98 0 0 0 0
RenderEvent 274 98 0 0 0 0
EndInteractionEvent 274 98 0 0 0 0
RenderEvent 274 98 0 0 0 0
MouseWheelForwardEvent 274 98 0 0 1 0
StartInteractionEvent 274 98 0 0 1 0
RenderEvent 274 98 0 0 1 0
EndInteractionEvent 274 98 0 0 1 0
RenderEvent 274 98 0 0 1 0
MouseWheelForwardEvent 274 98 0 0 0 0
StartInteractionEvent 274 98 0 0 0 0
RenderEvent 274 98 0 0 0 0
EndInteractionEvent 274 98 0 0 0 0
RenderEvent 274 98 0 0 0 0
MouseWheelForwardEvent 274 98 0 0 1 0
StartInteractionEvent 274 98 0 0 1 0
RenderEvent 274 98 0 0 1 0
EndInteractionEvent 274 98 0 0 1 0
RenderEvent 274 98 0 0 1 0
MouseMoveEvent 274 98 0 0 0 0
MouseMoveEvent 275 98 0 0 0 0
MouseMoveEvent 276 98 0 0 0 0
MouseMoveEvent 277 98 0 0 0 0
MouseMoveEvent 278 99 0 0 0 0
MouseMoveEvent 279 99 0 0 0 0
MouseMoveEvent 280 99 0 0 0 0
MouseMoveEvent 281 99 0 0 0 0
MouseMoveEvent 283 100 0 0 0 0
MouseMoveEvent 284 100 0 0 0 0
MouseMoveEvent 285 100 0 0 0 0
MouseMoveEvent 286 100 0 0 0 0
MouseMoveEvent 287 100 0 0 0 0
MouseMoveEvent 289 100 0 0 0 0
MouseMoveEvent 290 100 0 0 0 0
MouseMoveEvent 291 100 0 0 0 0
MouseMoveEvent 293 100 0 0 0 0
MouseMoveEvent 294 100 0 0 0 0
MouseMoveEvent 296 100 0 0 0 0
MouseMoveEvent 297 100 0 0 0 0
MouseMoveEvent 298 99 0 0 0 0
MouseMoveEvent 300 99 0 0 0 0
MouseMoveEvent 302 99 0 0 0 0
MouseMoveEvent 303 99 0 0 0 0
MouseMoveEvent 303 98 0 0 0 0
MouseMoveEvent 303 98 0 0 0 0
MouseMoveEvent 304 98 0 0 0 0
MouseMoveEvent 304 97 0 0 0 0
MouseMoveEvent 305 97 0 0 0 0
MouseMoveEvent 306 97 0 0 0 0
MouseMoveEvent 306 97 0 0 0 0
MouseMoveEvent 307 96 0 0 0 0
MouseMoveEvent 308 96 0 0 0 0
MouseMoveEvent 309 96 0 0 0 0
MouseMoveEvent 309 95 0 0 0 0
MouseMoveEvent 310 95 0 0 0 0
MouseMoveEvent 311 94 0 0 0 0
MouseMoveEvent 312 94 0 0 0 0
MouseMoveEvent 314 93 0 0 0 0
MouseMoveEvent 315 93 0 0 0 0
MouseMoveEvent 316 92 0 0 0 0
MouseMoveEvent 317 91 0 0 0 0
MouseMoveEvent 318 91 0 0 0 0
MouseMoveEvent 319 90 0 0 0 0
MouseMoveEvent 320 89 0 0 0 0
MouseMoveEvent 321 88 0 0 0 0
MouseMoveEvent 322 87 0 0 0 0
MouseMoveEvent 323 86 0 0 0 0
MouseMoveEvent 324 85 0 0 0 0
MouseMoveEvent 324 84 0 0 0 0
MouseMoveEvent 325 84 0 0 0 0
MouseMoveEvent 325 83 0 0 0 0
MouseMoveEvent 326 82 0 0 0 0
MouseMoveEvent 326 81 0 0 0 0
MouseMoveEvent 326 80 0 0 0 0
MouseMoveEvent 327 80 0 0 0 0
MouseMoveEvent 327 79 0 0 0 0
MouseMoveEvent 327 78 0 0 0 0
MouseMoveEvent 327 77 0 0 0 0
MouseMoveEvent 327 76 0 0 0 0
MouseMoveEvent 327 75 0 0 0 0
MouseMoveEvent 328 75 0 0 0 0
MouseMoveEvent 328 74 0 0 0 0
MouseMoveEvent 329 74 0 0 0 0
MouseMoveEvent 329 73 0 0 0 0
MouseMoveEvent 330 72 0 0 0 0
MouseMoveEvent 331 71 0 0 0 0
MouseMoveEvent 331 70 0 0 0 0
MouseMoveEvent 332 70 0 0 0 0
MouseMoveEvent 332 69 0 0 0 0
MouseMoveEvent 333 68 0 0 0 0
MouseMoveEvent 334 68 0 0 0 0
MouseMoveEvent 334 67 0 0 0 0
MouseMoveEvent 335 66 0 0 0 0
MouseMoveEvent 335 65 0 0 0 0
MouseMoveEvent 336 64 0 0 0 0
MouseMoveEvent 337 63 0 0 0 0
MouseMoveEvent 338 63 0 0 0 0
MouseMoveEvent 339 62 0 0 0 0
MouseMoveEvent 341 61 0 0 0 0
MouseMoveEvent 342 60 0 0 0 0
MouseMoveEvent 344 59 0 0 0 0
MouseMoveEvent 346 57 0 0 0 0
MouseMoveEvent 348 56 0 0 0 0
MouseMoveEvent 351 55 0 0 0 0
MouseMoveEvent 353 54 0 0 0 0
MouseMoveEvent 355 52 0 0 0 0
MouseMoveEvent 358 51 0 0 0 0
MouseMoveEvent 360 49 0 0 0 0
MouseMoveEvent 361 48 0 0 0 0
MouseMoveEvent 365 48 0 0 0 0
MouseMoveEvent 368 47 0 0 0 0
MouseMoveEvent 369 45 0 0 0 0
MouseMoveEvent 372 44 0 0 0 0
MouseMoveEvent 373 44 0 0 0 0
MouseMoveEvent 376 43 0 0 0 0
MouseMoveEvent 377 42 0 0 0 0
MouseMoveEvent 379 42 0 0 0 0
MouseMoveEvent 381 40 0 0 0 0
MouseMoveEvent 382 40 0 0 0 0
)eventStream"
    // We have to break this string up, otherwise MSVC whines about it being
    // too long (error C2026)
R"eventStream(
MouseMoveEvent 383 39 0 0 0 0
MouseMoveEvent 384 39 0 0 0 0
MouseMoveEvent 385 38 0 0 0 0
MouseMoveEvent 385 37 0 0 0 0
MouseMoveEvent 384 37 0 0 0 0
MouseMoveEvent 384 38 0 0 0 0
MouseMoveEvent 382 38 0 0 0 0
MouseMoveEvent 381 38 0 0 0 0
MouseMoveEvent 380 38 0 0 0 0
MouseMoveEvent 380 39 0 0 0 0
MouseMoveEvent 379 39 0 0 0 0
MouseMoveEvent 378 39 0 0 0 0
MouseMoveEvent 377 39 0 0 0 0
MouseMoveEvent 376 39 0 0 0 0
MouseMoveEvent 376 38 0 0 0 0
MiddleButtonPressEvent 376 38 0 0 0 0
StartInteractionEvent 376 38 0 0 0 0
MouseMoveEvent 376 38 0 0 0 0
RenderEvent 376 38 0 0 0 0
InteractionEvent 376 38 0 0 0 0
MouseMoveEvent 376 38 0 0 0 0
RenderEvent 376 38 0 0 0 0
InteractionEvent 376 38 0 0 0 0
MouseMoveEvent 376 39 0 0 0 0
RenderEvent 376 39 0 0 0 0
InteractionEvent 376 39 0 0 0 0
MouseMoveEvent 376 39 0 0 0 0
RenderEvent 376 39 0 0 0 0
InteractionEvent 376 39 0 0 0 0
MouseMoveEvent 375 39 0 0 0 0
RenderEvent 375 39 0 0 0 0
InteractionEvent 375 39 0 0 0 0
MouseMoveEvent 375 40 0 0 0 0
RenderEvent 375 40 0 0 0 0
InteractionEvent 375 40 0 0 0 0
MouseMoveEvent 375 40 0 0 0 0
RenderEvent 375 40 0 0 0 0
InteractionEvent 375 40 0 0 0 0
MouseMoveEvent 375 41 0 0 0 0
RenderEvent 375 41 0 0 0 0
InteractionEvent 375 41 0 0 0 0
MouseMoveEvent 375 42 0 0 0 0
RenderEvent 375 42 0 0 0 0
InteractionEvent 375 42 0 0 0 0
MouseMoveEvent 375 42 0 0 0 0
RenderEvent 375 42 0 0 0 0
InteractionEvent 375 42 0 0 0 0
MouseMoveEvent 375 43 0 0 0 0
RenderEvent 375 43 0 0 0 0
InteractionEvent 375 43 0 0 0 0
MouseMoveEvent 374 44 0 0 0 0
RenderEvent 374 44 0 0 0 0
InteractionEvent 374 44 0 0 0 0
MouseMoveEvent 374 45 0 0 0 0
RenderEvent 374 45 0 0 0 0
InteractionEvent 374 45 0 0 0 0
MouseMoveEvent 374 46 0 0 0 0
RenderEvent 374 46 0 0 0 0
InteractionEvent 374 46 0 0 0 0
MouseMoveEvent 374 48 0 0 0 0
RenderEvent 374 48 0 0 0 0
InteractionEvent 374 48 0 0 0 0
MouseMoveEvent 374 50 0 0 0 0
RenderEvent 374 50 0 0 0 0
InteractionEvent 374 50 0 0 0 0
MouseMoveEvent 375 52 0 0 0 0
RenderEvent 375 52 0 0 0 0
InteractionEvent 375 52 0 0 0 0
MouseMoveEvent 375 54 0 0 0 0
RenderEvent 375 54 0 0 0 0
InteractionEvent 375 54 0 0 0 0
MouseMoveEvent 376 56 0 0 0 0
RenderEvent 376 56 0 0 0 0
InteractionEvent 376 56 0 0 0 0
MouseMoveEvent 376 58 0 0 0 0
RenderEvent 376 58 0 0 0 0
InteractionEvent 376 58 0 0 0 0
MouseMoveEvent 376 60 0 0 0 0
RenderEvent 376 60 0 0 0 0
InteractionEvent 376 60 0 0 0 0
MouseMoveEvent 376 61 0 0 0 0
RenderEvent 376 61 0 0 0 0
InteractionEvent 376 61 0 0 0 0
MouseMoveEvent 376 63 0 0 0 0
RenderEvent 376 63 0 0 0 0
InteractionEvent 376 63 0 0 0 0
MouseMoveEvent 377 64 0 0 0 0
RenderEvent 377 64 0 0 0 0
InteractionEvent 377 64 0 0 0 0
MouseMoveEvent 377 66 0 0 0 0
RenderEvent 377 66 0 0 0 0
InteractionEvent 377 66 0 0 0 0
MouseMoveEvent 377 68 0 0 0 0
RenderEvent 377 68 0 0 0 0
InteractionEvent 377 68 0 0 0 0
MouseMoveEvent 377 69 0 0 0 0
RenderEvent 377 69 0 0 0 0
InteractionEvent 377 69 0 0 0 0
MouseMoveEvent 377 71 0 0 0 0
RenderEvent 377 71 0 0 0 0
InteractionEvent 377 71 0 0 0 0
MouseMoveEvent 377 72 0 0 0 0
RenderEvent 377 72 0 0 0 0
InteractionEvent 377 72 0 0 0 0
MouseMoveEvent 377 73 0 0 0 0
RenderEvent 377 73 0 0 0 0
InteractionEvent 377 73 0 0 0 0
MouseMoveEvent 377 74 0 0 0 0
RenderEvent 377 74 0 0 0 0
InteractionEvent 377 74 0 0 0 0
MouseMoveEvent 378 75 0 0 0 0
RenderEvent 378 75 0 0 0 0
InteractionEvent 378 75 0 0 0 0
MouseMoveEvent 378 76 0 0 0 0
RenderEvent 378 76 0 0 0 0
InteractionEvent 378 76 0 0 0 0
MouseMoveEvent 378 78 0 0 0 0
RenderEvent 378 78 0 0 0 0
InteractionEvent 378 78 0 0 0 0
MouseMoveEvent 378 79 0 0 0 0
RenderEvent 378 79 0 0 0 0
InteractionEvent 378 79 0 0 0 0
MouseMoveEvent 378 80 0 0 0 0
RenderEvent 378 80 0 0 0 0
InteractionEvent 378 80 0 0 0 0
MouseMoveEvent 378 81 0 0 0 0
RenderEvent 378 81 0 0 0 0
InteractionEvent 378 81 0 0 0 0
MouseMoveEvent 378 82 0 0 0 0
RenderEvent 378 82 0 0 0 0
InteractionEvent 378 82 0 0 0 0
MouseMoveEvent 379 83 0 0 0 0
RenderEvent 379 83 0 0 0 0
InteractionEvent 379 83 0 0 0 0
MouseMoveEvent 379 84 0 0 0 0
RenderEvent 379 84 0 0 0 0
InteractionEvent 379 84 0 0 0 0
MouseMoveEvent 379 85 0 0 0 0
RenderEvent 379 85 0 0 0 0
InteractionEvent 379 85 0 0 0 0
MouseMoveEvent 379 86 0 0 0 0
RenderEvent 379 86 0 0 0 0
InteractionEvent 379 86 0 0 0 0
MouseMoveEvent 379 88 0 0 0 0
RenderEvent 379 88 0 0 0 0
InteractionEvent 379 88 0 0 0 0
MouseMoveEvent 379 89 0 0 0 0
RenderEvent 379 89 0 0 0 0
InteractionEvent 379 89 0 0 0 0
MouseMoveEvent 379 90 0 0 0 0
RenderEvent 379 90 0 0 0 0
InteractionEvent 379 90 0 0 0 0
MouseMoveEvent 379 91 0 0 0 0
RenderEvent 379 91 0 0 0 0
InteractionEvent 379 91 0 0 0 0
MouseMoveEvent 379 92 0 0 0 0
RenderEvent 379 92 0 0 0 0
InteractionEvent 379 92 0 0 0 0
MouseMoveEvent 380 93 0 0 0 0
RenderEvent 380 93 0 0 0 0
InteractionEvent 380 93 0 0 0 0
MouseMoveEvent 380 95 0 0 0 0
RenderEvent 380 95 0 0 0 0
InteractionEvent 380 95 0 0 0 0
MouseMoveEvent 380 96 0 0 0 0
RenderEvent 380 96 0 0 0 0
InteractionEvent 380 96 0 0 0 0
MouseMoveEvent 380 97 0 0 0 0
RenderEvent 380 97 0 0 0 0
InteractionEvent 380 97 0 0 0 0
MouseMoveEvent 380 98 0 0 0 0
RenderEvent 380 98 0 0 0 0
InteractionEvent 380 98 0 0 0 0
MouseMoveEvent 380 99 0 0 0 0
RenderEvent 380 99 0 0 0 0
InteractionEvent 380 99 0 0 0 0
MouseMoveEvent 381 101 0 0 0 0
RenderEvent 381 101 0 0 0 0
InteractionEvent 381 101 0 0 0 0
MouseMoveEvent 381 103 0 0 0 0
RenderEvent 381 103 0 0 0 0
InteractionEvent 381 103 0 0 0 0
MouseMoveEvent 381 104 0 0 0 0
RenderEvent 381 104 0 0 0 0
InteractionEvent 381 104 0 0 0 0
MouseMoveEvent 381 105 0 0 0 0
RenderEvent 381 105 0 0 0 0
InteractionEvent 381 105 0 0 0 0
MouseMoveEvent 381 106 0 0 0 0
RenderEvent 381 106 0 0 0 0
InteractionEvent 381 106 0 0 0 0
MouseMoveEvent 381 107 0 0 0 0
RenderEvent 381 107 0 0 0 0
InteractionEvent 381 107 0 0 0 0
MouseMoveEvent 381 108 0 0 0 0
RenderEvent 381 108 0 0 0 0
InteractionEvent 381 108 0 0 0 0
MouseMoveEvent 382 109 0 0 0 0
RenderEvent 382 109 0 0 0 0
InteractionEvent 382 109 0 0 0 0
MouseMoveEvent 382 110 0 0 0 0
RenderEvent 382 110 0 0 0 0
InteractionEvent 382 110 0 0 0 0
MouseMoveEvent 382 111 0 0 0 0
RenderEvent 382 111 0 0 0 0
InteractionEvent 382 111 0 0 0 0
MouseMoveEvent 382 112 0 0 0 0
RenderEvent 382 112 0 0 0 0
InteractionEvent 382 112 0 0 0 0
MouseMoveEvent 382 112 0 0 0 0
RenderEvent 382 112 0 0 0 0
InteractionEvent 382 112 0 0 0 0
MouseMoveEvent 382 113 0 0 0 0
RenderEvent 382 113 0 0 0 0
InteractionEvent 382 113 0 0 0 0
MouseMoveEvent 382 113 0 0 0 0
RenderEvent 382 113 0 0 0 0
InteractionEvent 382 113 0 0 0 0
MiddleButtonReleaseEvent 382 113 0 0 0 0
EndInteractionEvent 382 113 0 0 0 0
RenderEvent 382 113 0 0 0 0
MouseMoveEvent 382 114 0 0 0 0
MouseMoveEvent 382 115 0 0 0 0
MouseMoveEvent 381 115 0 0 0 0
MouseMoveEvent 381 115 0 0 0 0
MouseMoveEvent 380 115 0 0 0 0
MouseMoveEvent 380 115 0 0 0 0
MouseMoveEvent 379 115 0 0 0 0
MouseMoveEvent 379 114 0 0 0 0
MouseMoveEvent 379 113 0 0 0 0
MouseMoveEvent 378 112 0 0 0 0
MouseMoveEvent 378 111 0 0 0 0
MouseMoveEvent 377 110 0 0 0 0
MouseMoveEvent 377 109 0 0 0 0
MouseMoveEvent 376 108 0 0 0 0
MouseMoveEvent 375 107 0 0 0 0
MouseMoveEvent 375 106 0 0 0 0
MouseMoveEvent 375 105 0 0 0 0
MouseMoveEvent 374 105 0 0 0 0
MouseMoveEvent 374 104 0 0 0 0
MouseMoveEvent 373 103 0 0 0 0
MouseMoveEvent 372 102 0 0 0 0
MouseMoveEvent 371 102 0 0 0 0
MouseMoveEvent 371 101 0 0 0 0
MouseMoveEvent 370 100 0 0 0 0
MouseMoveEvent 370 99 0 0 0 0
MouseMoveEvent 369 99 0 0 0 0
MouseMoveEvent 368 99 0 0 0 0
MouseMoveEvent 367 98 0 0 0 0
MouseMoveEvent 366 97 0 0 0 0
MouseMoveEvent 365 97 0 0 0 0
MouseMoveEvent 364 96 0 0 0 0
MouseMoveEvent 363 95 0 0 0 0
MouseMoveEvent 362 95 0 0 0 0
MouseMoveEvent 361 94 0 0 0 0
MouseMoveEvent 360 94 0 0 0 0
MouseMoveEvent 359 93 0 0 0 0
MouseMoveEvent 358 93 0 0 0 0
MouseMoveEvent 357 93 0 0 0 0
MouseMoveEvent 357 92 0 0 0 0
MouseMoveEvent 356 92 0 0 0 0
MouseMoveEvent 355 92 0 0 0 0
MouseMoveEvent 355 91 0 0 0 0
MouseWheelForwardEvent 355 91 0 0 0 0
StartInteractionEvent 355 91 0 0 0 0
RenderEvent 355 91 0 0 0 0
EndInteractionEvent 355 91 0 0 0 0
RenderEvent 355 91 0 0 0 0
MouseWheelForwardEvent 355 91 0 0 1 0
StartInteractionEvent 355 91 0 0 1 0
RenderEvent 355 91 0 0 1 0
EndInteractionEvent 355 91 0 0 1 0
RenderEvent 355 91 0 0 1 0
MouseMoveEvent 354 91 0 0 0 0
MouseMoveEvent 355 91 0 0 0 0
MouseMoveEvent 356 91 0 0 0 0
MouseMoveEvent 357 91 0 0 0 0
MouseMoveEvent 358 91 0 0 0 0
MouseMoveEvent 359 91 0 0 0 0
MouseMoveEvent 359 90 0 0 0 0
MouseMoveEvent 360 90 0 0 0 0
MouseMoveEvent 361 90 0 0 0 0
MouseMoveEvent 361 89 0 0 0 0
MouseMoveEvent 362 89 0 0 0 0
MouseMoveEvent 363 89 0 0 0 0
MouseMoveEvent 363 88 0 0 0 0
MouseMoveEvent 364 88 0 0 0 0
MouseMoveEvent 364 87 0 0 0 0
MouseMoveEvent 365 87 0 0 0 0
MouseMoveEvent 365 86 0 0 0 0
MouseMoveEvent 365 85 0 0 0 0
MouseMoveEvent 366 85 0 0 0 0
MouseMoveEvent 366 85 0 0 0 0
MouseMoveEvent 367 85 0 0 0 0
MouseMoveEvent 368 85 0 0 0 0
MouseMoveEvent 368 84 0 0 0 0
MouseMoveEvent 369 84 0 0 0 0
MouseMoveEvent 369 84 0 0 0 0
MouseMoveEvent 369 83 0 0 0 0
MouseMoveEvent 370 83 0 0 0 0
MouseMoveEvent 371 82 0 0 0 0
MouseMoveEvent 372 82 0 0 0 0
MouseMoveEvent 372 82 0 0 0 0
MouseMoveEvent 372 81 0 0 0 0
MouseMoveEvent 373 81 0 0 0 0
MouseMoveEvent 374 80 0 0 0 0
MouseMoveEvent 375 80 0 0 0 0
MouseMoveEvent 375 79 0 0 0 0
MouseMoveEvent 376 79 0 0 0 0
MouseMoveEvent 376 78 0 0 0 0
MouseMoveEvent 377 78 0 0 0 0
MouseMoveEvent 378 77 0 0 0 0
MouseMoveEvent 379 77 0 0 0 0
MouseMoveEvent 379 76 0 0 0 0
MouseMoveEvent 380 76 0 0 0 0
MouseMoveEvent 380 75 0 0 0 0
MouseMoveEvent 381 74 0 0 0 0
MouseMoveEvent 381 73 0 0 0 0
MouseMoveEvent 382 72 0 0 0 0
MouseMoveEvent 382 71 0 0 0 0
MouseMoveEvent 382 70 0 0 0 0
MouseMoveEvent 382 69 0 0 0 0
MouseMoveEvent 382 68 0 0 0 0
MouseMoveEvent 381 68 0 0 0 0
MouseMoveEvent 381 67 0 0 0 0
MouseMoveEvent 380 67 0 0 0 0
MouseMoveEvent 379 67 0 0 0 0
MouseMoveEvent 379 66 0 0 0 0
MouseMoveEvent 378 66 0 0 0 0
MouseMoveEvent 377 65 0 0 0 0
MouseMoveEvent 376 65 0 0 0 0
MouseMoveEvent 375 64 0 0 0 0
MouseMoveEvent 374 64 0 0 0 0
MouseMoveEvent 373 64 0 0 0 0
MouseMoveEvent 372 63 0 0 0 0
MouseMoveEvent 371 63 0 0 0 0
MouseMoveEvent 370 63 0 0 0 0
MouseMoveEvent 369 63 0 0 0 0
MouseMoveEvent 368 63 0 0 0 0
MouseMoveEvent 367 63 0 0 0 0
MouseMoveEvent 367 64 0 0 0 0
MouseMoveEvent 366 64 0 0 0 0
MouseMoveEvent 365 64 0 0 0 0
MouseMoveEvent 365 64 0 0 0 0
MouseMoveEvent 364 64 0 0 0 0
MouseWheelBackwardEvent 364 64 0 0 0 0
StartInteractionEvent 364 64 0 0 0 0
RenderEvent 364 64 0 0 0 0
EndInteractionEvent 364 64 0 0 0 0
RenderEvent 364 64 0 0 0 0
)eventStream";

} // end anon namespace

int TestGPURayCastDepthPeelingBoxWidget(int argc, char* argv[])
{
  // Volume peeling is only supported through the dual depth peeling algorithm.
  // If the current system only supports the legacy peeler, skip this test:
  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkRenderer> ren;
  renWin->Render(); // Create the context
  renWin->AddRenderer(ren);
  vtkOpenGLRenderer* oglRen = vtkOpenGLRenderer::SafeDownCast(ren);
  assert(oglRen); // This test should only be enabled for OGL2 backend.
  // This will print details about why depth peeling is unsupported:
  oglRen->SetDebug(1);
  bool supported = oglRen->IsDualDepthPeelingSupported();
  oglRen->SetDebug(0);
  if (!supported)
  {
    std::cerr << "Skipping test; volume peeling not supported.\n";
    return VTK_SKIP_RETURN_CODE;
  }

  double scalarRange[2];

  vtkNew<vtkActor> outlineActor;
  vtkNew<vtkPolyDataMapper> outlineMapper;
  vtkNew<vtkGPUVolumeRayCastMapper> volumeMapper;

  vtkNew<vtkXMLImageDataReader> reader;
  const char* volumeFile = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vase_1comp.vti");
  reader->SetFileName(volumeFile);
  delete[] volumeFile;
  volumeMapper->SetInputConnection(reader->GetOutputPort());

  // Add outline filter
  vtkNew<vtkOutlineFilter> outlineFilter;
  outlineFilter->SetInputConnection(reader->GetOutputPort());
  outlineMapper->SetInputConnection(outlineFilter->GetOutputPort());
  outlineActor->SetMapper(outlineMapper);

  volumeMapper->GetInput()->GetScalarRange(scalarRange);
  volumeMapper->SetSampleDistance(0.1);
  volumeMapper->SetAutoAdjustSampleDistances(0);
  volumeMapper->SetBlendModeToComposite();

  renWin->SetMultiSamples(0);
  renWin->SetSize(400, 400);
  ren->SetBackground(0.0, 0.0, 0.0);

  vtkNew<vtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(50, 0.0);
  scalarOpacity->AddPoint(75, 1.0);

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOn();
  volumeProperty->SetInterpolationType(VTK_LINEAR_INTERPOLATION);
  volumeProperty->SetScalarOpacity(scalarOpacity);

  vtkSmartPointer<vtkColorTransferFunction> colorTransferFunction =
    volumeProperty->GetRGBTransferFunction(0);
  colorTransferFunction->RemoveAllPoints();
  colorTransferFunction->AddRGBPoint(scalarRange[0], 0.6, 0.6, 0.6);

  vtkSmartPointer<vtkVolume> volume = vtkSmartPointer<vtkVolume>::New();
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  int dims[3];
  double spacing[3], center[3], origin[3];
  reader->Update();
  vtkSmartPointer<vtkImageData> im = reader->GetOutput();
  im->GetDimensions(dims);
  im->GetOrigin(origin);
  im->GetSpacing(spacing);

  // Add sphere 1
  center[0] = origin[0] + spacing[0] * dims[0] / 2.0;
  center[1] = origin[1] + spacing[1] * dims[1] / 2.0;
  center[2] = origin[2] + spacing[2] * dims[2] / 2.0;

  vtkNew<vtkSphereSource> sphereSource;
  sphereSource->SetCenter(center);
  sphereSource->SetRadius(dims[1] / 3.0);
  vtkNew<vtkActor> sphereActor;
  vtkProperty* sphereProperty = sphereActor->GetProperty();
  sphereProperty->SetColor(0.5, 0.9, 0.7);
  sphereProperty->SetOpacity(0.3);
  vtkNew<vtkPolyDataMapper> sphereMapper;
  sphereMapper->SetInputConnection(sphereSource->GetOutputPort());
  sphereActor->SetMapper(sphereMapper);

  // Add sphere 2
  center[0] += 15.0;
  center[1] += 15.0;
  center[2] += 15.0;

  vtkNew<vtkSphereSource> sphereSource2;
  sphereSource2->SetCenter(center);
  sphereSource2->SetRadius(dims[1] / 3.0);
  vtkNew<vtkActor> sphereActor2;
  sphereProperty = sphereActor2->GetProperty();
  sphereProperty->SetColor(0.9, 0.4, 0.1);
  sphereProperty->SetOpacity(0.3);
  vtkNew<vtkPolyDataMapper> sphereMapper2;
  sphereMapper2->SetInputConnection(sphereSource2->GetOutputPort());
  sphereActor2->SetMapper(sphereMapper2);

  // Add actors
  ren->AddVolume(volume);
  ren->AddActor(outlineActor);
  ren->AddActor(sphereActor);
  ren->AddActor(sphereActor2);

  // Configure depth peeling
  ren->SetUseDepthPeeling(1);
  ren->SetOcclusionRatio(0.0);
  ren->SetMaximumNumberOfPeels(17);
  ren->SetUseDepthPeelingForVolumes(true);

  // Create box widget
  vtkNew<vtkBoxWidget> boxWidget;
  boxWidget->SetInteractor(iren);
  boxWidget->SetPlaceFactor(1.25);
  vtkNew<vtkBWCallback> bwCb;
  bwCb->Volume = volume;
  boxWidget->AddObserver(vtkCommand::InteractionEvent, bwCb);
  boxWidget->SetProp3D(volume);
  boxWidget->PlaceWidget();
  boxWidget->On();
  boxWidget->InsideOutOn();
  bwCb->Execute(boxWidget, 0, nullptr); // Initialize clip planes

  // Prep the command stream
  vtkNew<vtkInteractorEventRecorder> recorder;
  recorder->SetInteractor(iren);
  recorder->SetEnabled(1);

  vtkNew<vtkInteractorStyleTrackballCamera> style;
  renWin->GetInteractor()->SetInteractorStyle(style);

  vtkNew<SamplingDistanceCallback> callback;
  callback->Mapper = volumeMapper;
  style->AddObserver(vtkCommand::StartInteractionEvent, callback);
  style->AddObserver(vtkCommand::EndInteractionEvent, callback);

  ren->ResetCamera();
  ren->GetActiveCamera()->Azimuth(-55);
  ren->GetActiveCamera()->Elevation(35);
  ren->GetActiveCamera()->OrthogonalizeViewUp();

  iren->Initialize();
  renWin->Render();

//#define RECORD
#ifdef RECORD
  recorder->SetFileName("/tmp/events.log");
  recorder->Record();
#else // playback
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(EventStream.c_str());
  recorder->Play();

  // Disable recorder before starting interactor during playback
  recorder->Off();
#endif

  iren->Start();

  recorder->Off();

  return EXIT_SUCCESS;
}
