/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWebApplication.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkWebApplication.h"

#include "vtkBase64Utilities.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkImageData.h"
#include "vtkJPEGWriter.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkObjectIdMap.h"
#include "vtkPNGWriter.h"
#include "vtkDataEncoder.h"
#include "vtkWebInteractionEvent.h"
#include "vtkPointData.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRendererCollection.h"
#include "vtkSmartPointer.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"
#include "vtkWebGLExporter.h"
#include "vtkWebGLObject.h"
#include "vtkWindowToImageFilter.h"

#include <cassert>
#include <cmath>
#include <map>
#include <sstream>

class vtkWebApplication::vtkInternals
{
public:
  struct ImageCacheValueType
    {
  public:
    vtkSmartPointer<vtkUnsignedCharArray> Data;
    bool NeedsRender;
    bool HasImagesBeingProcessed;
    vtkObject* ViewPointer;
    unsigned long ObserverId;
    ImageCacheValueType() : NeedsRender(true), HasImagesBeingProcessed(false), ViewPointer(NULL), ObserverId(0) { }

    void SetListener(vtkObject* view)
    {
      if(this->ViewPointer == view)
        {
        return;
        }

      if(this->ViewPointer && this->ObserverId)
        {
        this->ViewPointer->RemoveObserver(this->ObserverId);
        this->ObserverId = 0;
        }
      this->ViewPointer = view;
      if(this->ViewPointer)
        {
        this->ObserverId = this->ViewPointer->AddObserver(vtkCommand::AnyEvent, this, &ImageCacheValueType::ViewEventListener);
        }
    }

    void RemoveListener(vtkObject* view)
    {
      if(this->ViewPointer && this->ViewPointer == view && this->ObserverId)
        {
        this->ViewPointer->RemoveObserver(this->ObserverId);
        this->ObserverId = 0;
        this->ViewPointer = NULL;
        }
    }

    void ViewEventListener(vtkObject*, unsigned long, void*)
    {
      this->NeedsRender = true;
    }
    };
  typedef std::map<void*, ImageCacheValueType> ImageCacheType;
  ImageCacheType ImageCache;

  typedef std::map<void*, unsigned int > ButtonStatesType;
  ButtonStatesType ButtonStates;

  vtkNew<vtkDataEncoder> Encoder;

  // WebGL related struct
  struct WebGLObjCacheValue
    {
    public:
      int ObjIndex;
      std::map<int, std::string> BinaryParts;
    };
  // map for <vtkWebGLExporter, <webgl-objID, WebGLObjCacheValue> >
  typedef std::map<std::string, WebGLObjCacheValue> WebGLObjId2IndexMap;
  std::map<vtkWebGLExporter*, WebGLObjId2IndexMap> WebGLExporterObjIdMap;
  // map for <vtkRenderWindow, vtkWebGLExporter>
  std::map<vtkRenderWindow*, vtkSmartPointer<vtkWebGLExporter> > ViewWebGLMap;
  std::string LastAllWebGLBinaryObjects;
  vtkNew<vtkObjectIdMap> ObjectIdMap;
};

vtkStandardNewMacro(vtkWebApplication);
//----------------------------------------------------------------------------
vtkWebApplication::vtkWebApplication():
  ImageEncoding(ENCODING_BASE64),
  ImageCompression(COMPRESSION_JPEG),
  Internals(new vtkWebApplication::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkWebApplication::~vtkWebApplication()
{
  delete this->Internals;
  this->Internals = NULL;
}

//----------------------------------------------------------------------------
bool vtkWebApplication::GetHasImagesBeingProcessed(vtkRenderWindow* view)
{
  const vtkInternals::ImageCacheValueType& value = this->Internals->ImageCache[view];
  return value.HasImagesBeingProcessed;
}

//----------------------------------------------------------------------------
vtkUnsignedCharArray* vtkWebApplication::InteractiveRender(vtkRenderWindow* view, int quality)
{
  // for now, just do the same as StillRender().
  return this->StillRender(view, quality);
}

//----------------------------------------------------------------------------
void vtkWebApplication::InvalidateCache(vtkRenderWindow* view)
{
  this->Internals->ImageCache[view].NeedsRender = true;
}

//----------------------------------------------------------------------------
vtkUnsignedCharArray* vtkWebApplication::StillRender(vtkRenderWindow* view, int quality)
{
  if (!view)
    {
    vtkErrorMacro("No view specified.");
    return NULL;
    }

  vtkInternals::ImageCacheValueType& value = this->Internals->ImageCache[view];
  value.SetListener(view);

  if (value.NeedsRender == false &&
    value.Data != NULL /* FIXME SEB &&
    view->HasDirtyRepresentation() == false */)
    {
    //cout <<  "Reusing cache" << endl;
    bool latest = this->Internals->Encoder->GetLatestOutput(this->Internals->ObjectIdMap->GetGlobalId(view), value.Data);
    value.HasImagesBeingProcessed = !latest;
    return value.Data;
    }

  //cout <<  "Regenerating " << endl;
  //vtkTimerLog::ResetLog();
  //vtkTimerLog::CleanupLog();
  //vtkTimerLog::MarkStartEvent("StillRenderToString");
  //vtkTimerLog::MarkStartEvent("CaptureWindow");

  view->Render();

  // TODO: We should add logic to check if a new rendering needs to be done and
  // then alone do a new rendering otherwise use the cached image.
  vtkNew<vtkWindowToImageFilter> w2i;
  w2i->SetInput(view);
  w2i->SetMagnification(1);
  w2i->ReadFrontBufferOff();
  w2i->ShouldRerenderOff();
  w2i->FixBoundaryOn();
  w2i->Update();

  vtkImageData* image = vtkImageData::New();
  image->ShallowCopy(w2i->GetOutput());

  //vtkTimerLog::MarkEndEvent("CaptureWindow");

  //vtkTimerLog::MarkEndEvent("StillRenderToString");
  //vtkTimerLog::DumpLogWithIndents(&cout, 0.0);

  this->Internals->Encoder->PushAndTakeReference(this->Internals->ObjectIdMap->GetGlobalId(view), image, quality);
  assert(image == NULL);

  if (value.Data == NULL)
    {
    // we need to wait till output is processed.
    //cout << "Flushing" << endl;
    this->Internals->Encoder->Flush(this->Internals->ObjectIdMap->GetGlobalId(view));
    //cout << "Done Flushing" << endl;
    }

  bool latest = this->Internals->Encoder->GetLatestOutput(this->Internals->ObjectIdMap->GetGlobalId(view), value.Data);
  value.HasImagesBeingProcessed = !latest;
  value.NeedsRender = false;
  return value.Data;
}

//----------------------------------------------------------------------------
const char* vtkWebApplication::StillRenderToString(vtkRenderWindow* view, unsigned long time, int quality)
{
  vtkUnsignedCharArray* array = this->StillRender(view, quality);
  if (array && array->GetMTime() != time)
    {
    this->LastStillRenderToStringMTime = array->GetMTime();
    //cout << "Image size: " << array->GetNumberOfTuples() << endl;
    return reinterpret_cast<char*>(array->GetPointer(0));
    }
  return NULL;
}

//----------------------------------------------------------------------------
bool vtkWebApplication::HandleInteractionEvent(
  vtkRenderWindow* view, vtkWebInteractionEvent* event)
{
  vtkRenderWindowInteractor *iren = NULL;

  if (view)
    {
    iren = view->GetInteractor();
    }
  else
    {
    vtkErrorMacro("Interaction not supported for view : " << view);
    return false;
    }

  int ctrlKey =
    (event->GetModifiers() & vtkWebInteractionEvent::CTRL_KEY) != 0?  1: 0;
  int shiftKey =
    (event->GetModifiers() & vtkWebInteractionEvent::SHIFT_KEY) != 0?  1: 0;

  // Handle scroll action if any
  if(event->GetScroll()) {
    iren->SetEventInformation(0, 0, ctrlKey, shiftKey, event->GetKeyCode(), 0);
    iren->MouseMoveEvent();
    iren->RightButtonPressEvent();
    iren->SetEventInformation(0, event->GetScroll()*10, ctrlKey, shiftKey, event->GetKeyCode(), 0);
    iren->MouseMoveEvent();
    iren->RightButtonReleaseEvent();
    this->Internals->ImageCache[view].NeedsRender = true;
    return true;
  }

  int *viewSize = view->GetSize();
  int posX = std::floor(viewSize[0] * event->GetX() + 0.5);
  int posY = std::floor(viewSize[1] * event->GetY() + 0.5);

  iren->SetEventInformation(posX, posY, ctrlKey, shiftKey, event->GetKeyCode(), event->GetRepeatCount());

  unsigned int prev_buttons = this->Internals->ButtonStates[view];
  unsigned int changed_buttons = (event->GetButtons() ^ prev_buttons);
  iren->MouseMoveEvent();
  if ( (changed_buttons & vtkWebInteractionEvent::LEFT_BUTTON) != 0 )
    {
    if ( (event->GetButtons() & vtkWebInteractionEvent::LEFT_BUTTON) != 0)
      {
      iren->LeftButtonPressEvent();
      if(event->GetRepeatCount() > 0)
        {
        iren->LeftButtonReleaseEvent();
        }
      }
    else
      {
      iren->LeftButtonReleaseEvent();
      }
    }

  if ( (changed_buttons & vtkWebInteractionEvent::RIGHT_BUTTON) != 0 )
    {
    if ( (event->GetButtons() & vtkWebInteractionEvent::RIGHT_BUTTON) != 0)
      {
      iren->RightButtonPressEvent();
      if(event->GetRepeatCount() > 0)
        {
        iren->RightButtonPressEvent();
        }
      }
    else
      {
      iren->RightButtonReleaseEvent();
      }
    }
  if ( (changed_buttons & vtkWebInteractionEvent::MIDDLE_BUTTON) != 0 )
    {
    if ( (event->GetButtons() & vtkWebInteractionEvent::MIDDLE_BUTTON) != 0)
      {
      iren->MiddleButtonPressEvent();
      if(event->GetRepeatCount() > 0)
        {
        iren->MiddleButtonPressEvent();
        }
      }
    else
      {
      iren->MiddleButtonReleaseEvent();
      }
    }

  this->Internals->ButtonStates[view] = event->GetButtons();

  bool needs_render = (changed_buttons != 0 || event->GetButtons());
  this->Internals->ImageCache[view].NeedsRender = needs_render;
  return needs_render;
}

// ---------------------------------------------------------------------------
const char* vtkWebApplication::GetWebGLSceneMetaData(vtkRenderWindow* view)
{
  if (!view)
    {
    vtkErrorMacro("No view specified.");
    return NULL;
    }

  // We use the camera focal point to be the center of rotation
  double centerOfRotation[3];
  vtkCamera *cam = view->GetRenderers()->GetFirstRenderer()->GetActiveCamera();
  cam->GetFocalPoint(centerOfRotation);

  if(this->Internals->ViewWebGLMap.find(view) ==
    this->Internals->ViewWebGLMap.end())
    {
    this->Internals->ViewWebGLMap[view] =
      vtkSmartPointer<vtkWebGLExporter>::New();
    }

  std::stringstream globalIdAsString;
  globalIdAsString << this->Internals->ObjectIdMap->GetGlobalId(view);

  vtkWebGLExporter* webglExporter = this->Internals->ViewWebGLMap[view];
  webglExporter->parseScene(
        view->GetRenderers(), globalIdAsString.str().c_str(), VTK_PARSEALL);

  vtkInternals::WebGLObjId2IndexMap webglMap;
  for(int i=0; i<webglExporter->GetNumberOfObjects(); ++i)
    {
    vtkWebGLObject* wObj = webglExporter->GetWebGLObject(i);
    if(wObj && wObj->isVisible())
      {
      vtkInternals::WebGLObjCacheValue val;
      val.ObjIndex = i;
      for(int j=0; j<wObj->GetNumberOfParts(); ++j)
        {
        val.BinaryParts[j] = "";
        }
      webglMap[wObj->GetId()] = val;
      }
    }
  this->Internals->WebGLExporterObjIdMap[webglExporter] = webglMap;
  webglExporter->SetCenterOfRotation(
        static_cast<float>(centerOfRotation[0]),
        static_cast<float>(centerOfRotation[1]),
        static_cast<float>(centerOfRotation[2]));
  return webglExporter->GenerateMetadata();
}

//----------------------------------------------------------------------------
const char* vtkWebApplication::GetWebGLBinaryData(
  vtkRenderWindow* view, const char* id, int part)
{
  if (!view)
    {
    vtkErrorMacro("No view specified.");
    return NULL;
    }
  if(this->Internals->ViewWebGLMap.find(view) ==
    this->Internals->ViewWebGLMap.end())
    {
    if(this->GetWebGLSceneMetaData(view) == NULL)
      {
      vtkErrorMacro("Failed to generate WebGL MetaData for: " << view);
      return NULL;
      }
    }

  vtkWebGLExporter* webglExporter = this->Internals->ViewWebGLMap[view];
  if(webglExporter == NULL)
    {
    vtkErrorMacro("There is no cached WebGL Exporter for: " << view);
    return NULL;
    }

  if(this->Internals->WebGLExporterObjIdMap[webglExporter].size() > 0 &&
    this->Internals->WebGLExporterObjIdMap[webglExporter].find(id) !=
    this->Internals->WebGLExporterObjIdMap[webglExporter].end())
    {
    vtkInternals::WebGLObjCacheValue* cachedVal =
      &(this->Internals->WebGLExporterObjIdMap[webglExporter][id]);
    if(cachedVal->BinaryParts.find(part) != cachedVal->BinaryParts.end())
      {
      if(cachedVal->BinaryParts[part].empty())
        {
        vtkWebGLObject* obj = webglExporter->GetWebGLObject(cachedVal->ObjIndex);
        if(obj && obj->isVisible())
          {
          // Manage Base64
          vtkNew<vtkBase64Utilities> base64;
          unsigned char* output = new unsigned char[obj->GetBinarySize(part)*2];
          int size = base64->Encode(
            obj->GetBinaryData(part), obj->GetBinarySize(part), output, false);
          cachedVal->BinaryParts[part] = std::string((const char *)output, size);
          delete[] output;
          }
        }
      return cachedVal->BinaryParts[part].c_str();
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
void vtkWebApplication::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ImageEncoding: " << this->ImageEncoding << endl;
  os << indent << "ImageCompression: " << this->ImageCompression << endl;
}

//----------------------------------------------------------------------------
vtkObjectIdMap* vtkWebApplication::GetObjectIdMap()
{
  return this->Internals->ObjectIdMap.GetPointer();
}
