/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWebGLExporter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkWebGLExporter.h"

#include "vtkObjectFactory.h"
#include "vtkActor2D.h"
#include "vtkActorCollection.h"
#include "vtkBase64Utilities.h"
#include "vtkDiscretizableColorTransferFunction.h"
#include "vtkFollower.h"
#include "vtkRenderer.h"
#include "vtkMapper.h"
#include "vtkSmartPointer.h"
#include "vtkTriangleFilter.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositeDataGeometryFilter.h"
#include "vtkPolyDataNormals.h"
#include "vtkCellArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkProperty.h"
#include "vtkCamera.h"
#include "vtkViewport.h"
#include "vtkMatrix4x4.h"
#include "vtkMapper2D.h"
#include "vtkCellData.h"
#include "vtkAbstractMapper.h"
#include "vtkGenericCell.h"
#include "vtkPointData.h"
#include "vtkProperty.h"
#include "vtkDataSet.h"
#include "vtkProperty2D.h"
#include "vtkRenderWindow.h"
#include "vtkRendererCollection.h"
#include "vtkScalarBarRepresentation.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkScalarBarActor.h"
#include "vtkWidgetRepresentation.h"

#include "vtkWebGLObject.h"
#include "vtkWebGLPolyData.h"
#include "vtkWebGLWidget.h"

#include <iostream>
#include <sstream>
#include <map>
#include <string>
#include <vector>
#include <algorithm>

#include "glMatrix.h"
#include "webglRenderer.h"

#include "vtksys/MD5.h"

//*****************************************************************************
class vtkWebGLExporter::vtkInternal
{
public:
  std::string LastMetaData;
  std::map<unsigned long,unsigned long> ActorTimestamp;
  std::map<unsigned long,unsigned long> OldActorTimestamp;
  std::vector<vtkWebGLObject*> Objects;
  std::vector<vtkWebGLObject*> tempObj;
};
//*****************************************************************************

vtkStandardNewMacro(vtkWebGLExporter);

vtkWebGLExporter::vtkWebGLExporter()
{
  this->meshObjMaxSize = 65532/3;
  this->lineObjMaxSize = 65534/2;
  this->Internal = new vtkInternal();
  this->TriangleFilter = NULL;
  this->GradientBackground = false;
  this->SetCenterOfRotation(0.0, 0.0, 0.0);
  this->renderersMetaData = "";
  this->SceneSize[0] = 0;
  this->SceneSize[1] = 0;
  this->SceneSize[2] = 0;
  this->hasWidget = false;
}

vtkWebGLExporter::~vtkWebGLExporter()
{
  while(this->Internal->Objects.size() != 0)
    {
    vtkWebGLObject* obj = this->Internal->Objects.back();
    obj->Delete();
    this->Internal->Objects.pop_back();
    }
  delete this->Internal;
  if (this->TriangleFilter)
    {
    this->TriangleFilter->Delete();
    }
}

void vtkWebGLExporter::SetMaxAllowedSize(int mesh, int lines)
  {
  this->meshObjMaxSize = mesh;
  this->lineObjMaxSize = lines;
  if (this->meshObjMaxSize*3 > 65532) this->meshObjMaxSize = 65532/3;
  if (this->lineObjMaxSize*2 > 65534) this->lineObjMaxSize = 65534/2;
  if (this->meshObjMaxSize < 10) this->meshObjMaxSize = 10;
  if (this->lineObjMaxSize < 10) this->lineObjMaxSize = 10;
  for(size_t i=0; i<this->Internal->Objects.size(); i++) this->Internal->Objects[i]->GenerateBinaryData();
  }

void vtkWebGLExporter::SetMaxAllowedSize(int size)
  {
  this->SetMaxAllowedSize(size, size);
  }

void vtkWebGLExporter::SetCenterOfRotation(float a1, float a2, float a3)
  {
  this->CenterOfRotation[0] = a1;
  this->CenterOfRotation[1] = a2;
  this->CenterOfRotation[2] = a3;
  }


void vtkWebGLExporter::parseRenderer(vtkRenderer *renderer, const char* vtkNotUsed(viewId), bool onlyWidget, void* vtkNotUsed(mapTime))
  {
  vtkPropCollection* propCollection = renderer->GetViewProps();
  for (int i=0; i<propCollection->GetNumberOfItems(); i++)
    {
    vtkProp* prop = (vtkProp*)propCollection->GetItemAsObject(i);
    vtkWidgetRepresentation* trt = vtkWidgetRepresentation::SafeDownCast(prop);
    if (trt!=NULL) this->hasWidget = true;
    if ((onlyWidget==false || trt!=NULL) && prop->GetVisibility())
      {
      vtkPropCollection* allactors = vtkPropCollection::New();
      prop->GetActors(allactors);
      for (int j=0; j<allactors->GetNumberOfItems(); j++)
        {
        vtkActor* actor = vtkActor::SafeDownCast(allactors->GetItemAsObject(j));
        unsigned long key = (unsigned long)actor;
        unsigned long previousValue = this->Internal->OldActorTimestamp[key];
        this->parseActor(actor, previousValue, (long)renderer, renderer->GetLayer(), trt != NULL);
        }
      allactors->Delete();
      }
    if (onlyWidget==false && prop->GetVisibility())
      {
      vtkPropCollection* all2dactors = vtkPropCollection::New();
      prop->GetActors2D(all2dactors);
      for (int k=0; k<all2dactors->GetNumberOfItems(); k++)
        {
        vtkActor2D* actor = vtkActor2D::SafeDownCast(all2dactors->GetItemAsObject(k));
        unsigned long key = (unsigned long)actor;
        unsigned long previousValue = this->Internal->OldActorTimestamp[key];
        this->parseActor2D(actor, previousValue, (long)renderer, renderer->GetLayer(), trt != NULL);
        }
      all2dactors->Delete();
      }
    }
  }

void vtkWebGLExporter::parseActor2D(vtkActor2D *actor, long actorTime, long renderId, int layer, bool isWidget)
  {
  unsigned long key = (unsigned long)actor;
  vtkScalarBarActor* scalarbar = vtkScalarBarActor::SafeDownCast(actor);

  long dataMTime = actor->GetMTime() + actor->GetRedrawMTime() + actor->GetProperty()->GetMTime();
  dataMTime += (long)actor->GetMapper();
  if (scalarbar) dataMTime += scalarbar->GetLookupTable()->GetMTime();
  if (dataMTime != actorTime && actor->GetVisibility())
    {
    this->Internal->ActorTimestamp[key] = dataMTime;

    if (actor->GetMapper())
      {
      std::string name = actor->GetMapper()->GetClassName();
      if (vtkPolyDataMapper2D::SafeDownCast(actor->GetMapper()))
        {
        }
      }
    else
      {
      if (scalarbar)
        {
        vtkWebGLWidget* obj = vtkWebGLWidget::New();
        obj->GetDataFromColorMap(actor);

        std::stringstream ss;
        ss << (long)actor;
        obj->SetId(ss.str());
        obj->SetRendererId(renderId);
        this->Internal->Objects.push_back(obj);
        obj->SetLayer(layer);
        obj->SetVisibility(actor->GetVisibility() != 0);
        obj->SetIsWidget(isWidget);
        obj->SetInteractAtServer(false);
        obj->GenerateBinaryData();
        }
      }
    }
  else
    {
    this->Internal->ActorTimestamp[key] = dataMTime;
    std::stringstream ss;
    ss << (long)actor;
    for (size_t i=0; i<this->Internal->tempObj.size(); i++)
      {
      if (this->Internal->tempObj[i]->GetId().compare(ss.str()) == 0)
        {
        vtkWebGLObject* obj = this->Internal->tempObj[i];
        this->Internal->tempObj.erase(this->Internal->tempObj.begin()+i);
        obj->SetVisibility(actor->GetVisibility() != 0);
        this->Internal->Objects.push_back(obj);
        }
      }
    }
  }

void vtkWebGLExporter::parseActor(vtkActor* actor, unsigned long actorTime, long rendererId, int layer, bool isWidget)
  {
  vtkMapper* mapper = actor->GetMapper();
  if (mapper)
    {
    unsigned long dataMTime;
    vtkTriangleFilter* polydata = this->GetPolyData(mapper, dataMTime);
    unsigned long key = (unsigned long)actor;
    dataMTime = actor->GetMTime() + mapper->GetLookupTable()->GetMTime();
    dataMTime += actor->GetProperty()->GetMTime() + mapper->GetMTime() + actor->GetRedrawMTime();
    dataMTime += polydata->GetOutput()->GetNumberOfLines() + polydata->GetOutput()->GetNumberOfPolys();
    dataMTime += actor->GetProperty()->GetRepresentation() + mapper->GetScalarMode() + actor->GetVisibility();
    dataMTime += polydata->GetInput()->GetMTime();
    if (vtkFollower::SafeDownCast(actor)) dataMTime += vtkFollower::SafeDownCast(actor)->GetCamera()->GetMTime();
    if(dataMTime != actorTime && actor->GetVisibility())
      {
      double bb[6];
      actor->GetBounds(bb);
      double m1 = std::max(bb[1]-bb[0], bb[3]-bb[2]); m1 = std::max(m1, bb[5]-bb[4]);
      double m2 = std::max(this->SceneSize[0], this->SceneSize[1]); m2 = std::max(m2, this->SceneSize[2]);
      if (m1 > m2)
        {
        this->SceneSize[0] = bb[1]-bb[0];
        this->SceneSize[1] = bb[3]-bb[2];
        this->SceneSize[2] = bb[5]-bb[4];
        }

      this->Internal->ActorTimestamp[key] = dataMTime;
      vtkWebGLObject* obj = NULL;
      std::stringstream ss;
      ss << (long)actor;
      for (size_t i=0; i<this->Internal->tempObj.size(); i++)
        {
        if (this->Internal->tempObj[i]->GetId().compare(ss.str()) == 0)
          {
          obj = this->Internal->tempObj[i];
          this->Internal->tempObj.erase(this->Internal->tempObj.begin()+i);
          }
        }
      if (obj == NULL) obj = vtkWebGLPolyData::New();

      if (polydata->GetOutput()->GetNumberOfPolys() != 0)
        {
        if (actor->GetProperty()->GetRepresentation() == VTK_WIREFRAME)
          {
          ((vtkWebGLPolyData*)obj)->GetLinesFromPolygon(mapper, actor, this->lineObjMaxSize, NULL);
          }
        else
          {

          if (actor->GetProperty()->GetEdgeVisibility())
            {
            vtkWebGLPolyData* newobj = vtkWebGLPolyData::New();
            double ccc[3]; actor->GetProperty()->GetEdgeColor(&ccc[0]);
            ((vtkWebGLPolyData*)newobj)->GetLinesFromPolygon(mapper, actor, this->lineObjMaxSize, ccc);
            newobj->SetId(ss.str() + "1");
            newobj->SetRendererId(rendererId);
            this->Internal->Objects.push_back(newobj);
            newobj->SetLayer(layer);
            newobj->SetTransformationMatrix(actor->GetMatrix());
            newobj->SetVisibility(actor->GetVisibility() != 0);
            newobj->SetHasTransparency(actor->HasTranslucentPolygonalGeometry() != 0);
            newobj->SetIsWidget(isWidget);
            newobj->SetInteractAtServer(isWidget);
            newobj->GenerateBinaryData();
            }


          switch(mapper->GetScalarMode())
            {
            case VTK_SCALAR_MODE_USE_POINT_FIELD_DATA:
               ((vtkWebGLPolyData*)obj)->GetPolygonsFromPointData(polydata, actor, this->meshObjMaxSize);
              break;
            case VTK_SCALAR_MODE_USE_CELL_FIELD_DATA:
              ((vtkWebGLPolyData*)obj)->GetPolygonsFromCellData(polydata, actor, this->meshObjMaxSize);
              break;
            default:
               ((vtkWebGLPolyData*)obj)->GetPolygonsFromPointData(polydata, actor, this->meshObjMaxSize);
              break;
            }
          }
        obj->SetId(ss.str());
        obj->SetRendererId(rendererId);
        this->Internal->Objects.push_back(obj);
        obj->SetLayer(layer);
        obj->SetTransformationMatrix(actor->GetMatrix());
        obj->SetVisibility(actor->GetVisibility() != 0);
        obj->SetHasTransparency(actor->HasTranslucentPolygonalGeometry() != 0);
        obj->SetIsWidget(isWidget);
        obj->SetInteractAtServer(isWidget);
        obj->GenerateBinaryData();
        }
      else if (polydata->GetOutput()->GetNumberOfLines() != 0)
        {
        ((vtkWebGLPolyData*)obj)->GetLines(polydata, actor, this->lineObjMaxSize);
        obj->SetId(ss.str());
        obj->SetRendererId(rendererId);
        this->Internal->Objects.push_back(obj);
        obj->SetLayer(layer);
        obj->SetTransformationMatrix(actor->GetMatrix());
        obj->SetVisibility(actor->GetVisibility() != 0);
        obj->SetHasTransparency(actor->HasTranslucentPolygonalGeometry() != 0);
        obj->SetIsWidget(isWidget);
        obj->SetInteractAtServer(isWidget);
        obj->GenerateBinaryData();
        }
      else if (polydata->GetOutput()->GetNumberOfPoints() != 0)
        {
        ((vtkWebGLPolyData*)obj)->GetPoints(polydata, actor, 65534);//Wendel
        obj->SetId(ss.str());
        obj->SetRendererId(rendererId);
        this->Internal->Objects.push_back(obj);
        obj->SetLayer(layer);
        obj->SetTransformationMatrix(actor->GetMatrix());
        obj->SetVisibility(actor->GetVisibility() != 0);
        obj->SetHasTransparency(actor->HasTranslucentPolygonalGeometry() != 0);
        obj->SetIsWidget(false);
        obj->SetInteractAtServer(false);
        obj->GenerateBinaryData();
        }

      if (polydata->GetOutput()->GetNumberOfPolys() != 0 && polydata->GetOutput()->GetNumberOfLines() != 0)
        {
        obj = vtkWebGLPolyData::New();
        ((vtkWebGLPolyData*)obj)->GetLines(polydata, actor, this->lineObjMaxSize);
        ss << "1";
        obj->SetId(ss.str());
        obj->SetRendererId(rendererId);
        this->Internal->Objects.push_back(obj);
        obj->SetLayer(layer);
        obj->SetTransformationMatrix(actor->GetMatrix());
        obj->SetVisibility(actor->GetVisibility() != 0);
        obj->SetHasTransparency(actor->HasTranslucentPolygonalGeometry() != 0);
        obj->SetIsWidget(isWidget);
        obj->SetInteractAtServer(isWidget);
        obj->GenerateBinaryData();
        }

      if (polydata->GetOutput()->GetNumberOfLines() == 0 && polydata->GetOutput()->GetNumberOfPolys() == 0 && polydata->GetOutput()->GetNumberOfPoints() == 0)
        {
        obj->Delete();
        }
      }
    else
      {
      this->Internal->ActorTimestamp[key] = actorTime;
      std::stringstream ss;
      ss << (long)actor;
      for (size_t i=0; i<this->Internal->tempObj.size(); i++)
        {
        if (this->Internal->tempObj[i]->GetId().compare(ss.str()) == 0)
          {
          vtkWebGLObject* obj = this->Internal->tempObj[i];
          this->Internal->tempObj.erase(this->Internal->tempObj.begin()+i);
          obj->SetVisibility(actor->GetVisibility() != 0);
          this->Internal->Objects.push_back(obj);
          }
        }
      }
    }
  }

void vtkWebGLExporter::parseScene(vtkRendererCollection* renderers, const char* viewId, int parseType)
{
  if (!renderers) return;

  bool onlyWidget = parseType == VTK_ONLYWIDGET;
  bool cameraOnly = onlyWidget && !this->hasWidget;

  this->SceneId = viewId? viewId : "";
  if (cameraOnly)
    {
    this->generateRendererData(renderers, viewId);
    return;
    }

  if (onlyWidget)
    {
    for (int i= static_cast<int>(this->Internal->Objects.size())-1; i>=0; i--)
      {
      vtkWebGLObject* obj = this->Internal->Objects[i];
      if (obj->InteractAtServer())
        {
        this->Internal->tempObj.push_back(obj);
        this->Internal->Objects.erase(this->Internal->Objects.begin()+i);
        }
      }
    }
  else
    {
    while(this->Internal->Objects.size() != 0)
      {
      this->Internal->tempObj.push_back(this->Internal->Objects.back());
      this->Internal->Objects.pop_back();
      }
    }

  this->Internal->OldActorTimestamp = this->Internal->ActorTimestamp;
  if (!onlyWidget) this->Internal->ActorTimestamp.clear();
  this->hasWidget = false;
  for(int i=0; i<renderers->GetNumberOfItems(); i++)
    {
    vtkRenderer* renderer = vtkRenderer::SafeDownCast(renderers->GetItemAsObject(i));
    if (renderer->GetDraw()) this->parseRenderer(renderer, viewId, onlyWidget, NULL);
    }
  while(this->Internal->tempObj.size() != 0)
    {
    vtkWebGLObject* obj = this->Internal->tempObj.back();
    this->Internal->tempObj.pop_back();
    obj->Delete();
    }

  this->generateRendererData(renderers, viewId);
}

bool sortLayer(vtkRenderer* i,vtkRenderer* j)
  {
  return (i->GetLayer() < j->GetLayer());
  }

void vtkWebGLExporter::generateRendererData(vtkRendererCollection* renderers, const char* vtkNotUsed(viewId))
  {
  std::stringstream ss;
  ss << "\"Renderers\": [";

  std::vector<vtkRenderer*> orderedList;
  for(int i=0; i<renderers->GetNumberOfItems(); i++) orderedList.push_back(vtkRenderer::SafeDownCast(renderers->GetItemAsObject(i)));
  std::sort(orderedList.begin(), orderedList.begin()+orderedList.size(), sortLayer);

  int *fullSize = NULL;
  for(size_t i=0; i<orderedList.size(); i++)
    {
    vtkRenderer* renderer = orderedList[i];

    if (i == 0)
      {
      fullSize = renderer->GetSize();
      }

    double cam[10]; cam[0] = renderer->GetActiveCamera()->GetViewAngle();
    renderer->GetActiveCamera()->GetFocalPoint(&cam[1]);
    renderer->GetActiveCamera()->GetViewUp(&cam[4]);
    renderer->GetActiveCamera()->GetPosition(&cam[7]);
    int* s, *o; s = renderer->GetSize(); o = renderer->GetOrigin();
    ss << "{\"layer\":" << renderer->GetLayer() << ",";     //Render Layer
    if (renderer->GetLayer() == 0)                          //Render Background
      {
      double back[3]; renderer->GetBackground(back);
      ss << "\"Background1\":[" << back[0] << "," << back[1] << "," << back[2] << "],";
      if (renderer->GetGradientBackground())
        {
        renderer->GetBackground2(back);
        ss << "\"Background2\":[" << back[0] << "," << back[1] << "," << back[2] << "],";
        }
      }
    ss << "\"LookAt\":[";                                  //Render Camera
    for (int j=0; j<9; j++) ss << cam[j] << ",";
    ss << cam[9] << "], ";
    ss << "\"size\": [" << (float)(s[0]/(float)fullSize[0]) << "," << (float)(s[1]/(float)fullSize[1]) << "],";    //Render Size
    ss << "\"origin\": [" << (float)(o[0]/(float)fullSize[0]) << "," << (float)(o[1]/(float)fullSize[1]) << "]";   //Render Position
    ss << "}";
    if (static_cast<int>(i+1) != renderers->GetNumberOfItems()) ss << ", ";
    }
  ss << "]";
  this->renderersMetaData = ss.str();
  }

vtkTriangleFilter* vtkWebGLExporter::GetPolyData(vtkMapper* mapper, unsigned long& dataMTime)
{
  vtkDataSet* dataset = NULL;
  vtkSmartPointer<vtkDataSet> tempDS;
  vtkDataObject* dObj = mapper->GetInputDataObject(0, 0);
  vtkCompositeDataSet* cd = vtkCompositeDataSet::SafeDownCast(dObj);
  if (cd)
    {
    dataMTime = cd->GetMTime();
    vtkCompositeDataGeometryFilter* gf = vtkCompositeDataGeometryFilter::New();
    gf->SetInputData(cd);
    gf->Update();
    tempDS = gf->GetOutput();
    gf->Delete();
    dataset = tempDS;
    }
  else
    {
    dataset = mapper->GetInput();
    dataMTime = dataset->GetMTime();
    }

  // Converting to triangles. WebGL only support triangles.
  if (this->TriangleFilter) this->TriangleFilter->Delete();
  this->TriangleFilter = vtkTriangleFilter::New();
  this->TriangleFilter->SetInputData(dataset);
  this->TriangleFilter->Update();
  return this->TriangleFilter;
}

/*
  Function: GenerateMetaData
  Description:
  - Generates the metadata of the scene in JSON format
  Ex.:
    { "id": ,"LookAt": ,"Background1": ,"Background2":
    "Objects": [{"id": ,"md5": ,"parts": },  {"id": ,"md5": ,"parts": }] }
*/
const char* vtkWebGLExporter::GenerateMetadata()
{
  double max = std::max(this->SceneSize[0], this->SceneSize[1]); max = std::max(max, this->SceneSize[2]);
  std::stringstream ss;

  ss << "{\"id\":" << this->SceneId.c_str() << ",";
  ss << "\"MaxSize\":" << max << ",";
  ss << "\"Center\":[";
  for (int i=0; i<2; i++) ss << this->CenterOfRotation[i] << ", ";
  ss << this->CenterOfRotation[2] << "],";

  ss << this->renderersMetaData << ",";

  ss << " \"Objects\":[";
  bool first = true;
  for (size_t i=0; i<this->Internal->Objects.size(); i++)
    {
    vtkWebGLObject* obj = this->Internal->Objects[i];
    if (obj->isVisible())
      {
      if (first) first = false;
      else ss << ", ";
      ss << "{\"id\":" << obj->GetId() << ", \"md5\":\"" << obj->GetMD5() << "\""
         << ", \"parts\":" << obj->GetNumberOfParts()
         << ", \"interactAtServer\":" << obj->InteractAtServer()
         << ", \"transparency\":" << obj->HasTransparency()
         << ", \"layer\":" << obj->GetLayer()
         << ", \"wireframe\":" << obj->isWireframeMode() << "}";
      }
    }
  ss << "]}";

  this->Internal->LastMetaData = ss.str();
  return this->Internal->LastMetaData.c_str();
}

const char* vtkWebGLExporter::GenerateExportMetadata()
{
  double max = std::max(this->SceneSize[0], this->SceneSize[1]); max = std::max(max, this->SceneSize[2]);
  std::stringstream ss;

  ss << "{\"id\":" << this->SceneId << ",";
  ss << "\"MaxSize\":" << max << ",";
  ss << "\"Center\":[";
  for (int i=0; i<2; i++) ss << this->CenterOfRotation[i] << ", ";
  ss << this->CenterOfRotation[2] << "],";

  ss << this->renderersMetaData << ",";

  ss << " \"Objects\":[";
  bool first = true;
  for (size_t i=0; i<this->Internal->Objects.size(); i++)
    {
    vtkWebGLObject* obj = this->Internal->Objects[i];
    if (obj->isVisible())
      {
      for(int j=0; j<obj->GetNumberOfParts(); j++)
        {
        if (first) first = false;
        else ss << ", ";
        ss << "{\"id\":" << obj->GetId() << ", \"md5\":\"" << obj->GetMD5() << "\""
           << ", \"parts\":" << 1
           << ", \"interactAtServer\":" << obj->InteractAtServer()
           << ", \"transparency\":" << obj->HasTransparency()
           << ", \"layer\":" << obj->GetLayer()
           << ", \"wireframe\":" << obj->isWireframeMode() << "}";
        }
      }
    }
  ss << "]}";

  this->Internal->LastMetaData = ss.str();
  return this->Internal->LastMetaData.c_str();
}

vtkWebGLObject* vtkWebGLExporter::GetWebGLObject(int index)
  {
  return this->Internal->Objects[index];
  }

int vtkWebGLExporter::GetNumberOfObjects()
  {
  return static_cast<int>(this->Internal->Objects.size());
  }

void vtkWebGLExporter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

const char* vtkWebGLExporter::GetId()
{
  return this->SceneId.c_str();
}

bool vtkWebGLExporter::hasChanged()
{
  for (size_t i=0; i<this->Internal->Objects.size(); i++) if (this->Internal->Objects[i]->HasChanged()) return true;
  return false;
}

void vtkWebGLExporter::exportStaticScene(vtkRendererCollection *renderers, int width, int height, std::string path)
  {
  std::stringstream ss;
  ss << width << "," << height;
  std::string resultHTML = "<html><head></head><body onload='loadStaticScene();' style='margin: 0px; padding: 0px; position: absolute; overflow: hidden; top:0px; left:0px;'>";
  resultHTML += "<div id='container' onclick='consumeEvent(event);' style='margin: 0px; padding: 0px; position: absolute; overflow: hidden; top:0px; left:0px;'></div></body>\n";
  resultHTML += "<script type='text/javascript'> var rendererWebGL = null;";
  resultHTML += "function reresize(event){ if (rendererWebGL != null) rendererWebGL.setSize(window.innerWidth, window.innerHeight); }";
  resultHTML += "function loadStaticScene(){ ";
  resultHTML += "  var objs=[];";
  resultHTML += "  for(i=0; i<object.length; i++){";
  resultHTML += "  objs[i] = decode64(object[i]);";
  resultHTML += "  }\n object = [];";
  resultHTML += "  rendererWebGL = new WebGLRenderer('webglRenderer-1', '');";
  resultHTML += "  rendererWebGL.init('', '');";
  resultHTML += "  rendererWebGL.bindToElementId('container');";
  resultHTML += "  //rendererWebGL.setSize(" + ss.str() + ");\n";
  resultHTML += "  rendererWebGL.setSize(window.innerWidth, window.innerHeight);";
  resultHTML += "  rendererWebGL.start(metadata, objs);";
  resultHTML += "  window.onresize = reresize;";
  resultHTML += "}\n";
  resultHTML += "function consumeEvent(event) { if (event.preventDefault) { event.preventDefault();} else { event.returnValue= false;} return false;}";

  resultHTML += "function ntos(n){ n=n.toString(16); if (n.length == 1) n='0'+n; n='%'+n; return unescape(n); }";
  resultHTML += "var END_OF_INPUT = -1; var base64Chars = new Array(";
  resultHTML += "'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X',";
  resultHTML += "'Y','Z','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v',";
  resultHTML += "'w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/');";
  resultHTML += "var base64Str; var base64Count;";
  resultHTML += "var reverseBase64Chars = new Array();";
  resultHTML += "for (var i=0; i < base64Chars.length; i++){ reverseBase64Chars[base64Chars[i]] = i; }";
  resultHTML += "function readReverseBase64(){ if (!base64Str) return END_OF_INPUT;";
  resultHTML += "while (true){ if (base64Count >= base64Str.length) return END_OF_INPUT;";
  resultHTML += "var nextCharacter = base64Str.charAt(base64Count); base64Count++;";
  resultHTML += "if (reverseBase64Chars[nextCharacter]){ return reverseBase64Chars[nextCharacter]; }";
  resultHTML += "if (nextCharacter == 'A') return 0; } return END_OF_INPUT; }";
  resultHTML += "function decode64(str){";
  resultHTML += "base64Str = str; base64Count = 0; var result = ''; var inBuffer = new Array(4); var done = false;";
  resultHTML += "while (!done && (inBuffer[0] = readReverseBase64()) != END_OF_INPUT";
  resultHTML += "&& (inBuffer[1] = readReverseBase64()) != END_OF_INPUT){";
  resultHTML += "inBuffer[2] = readReverseBase64();";
  resultHTML += "inBuffer[3] = readReverseBase64();";
  resultHTML += "result += ntos((((inBuffer[0] << 2) & 0xff)| inBuffer[1] >> 4));";
  resultHTML += "if (inBuffer[2] != END_OF_INPUT){";
  resultHTML += "result +=  ntos((((inBuffer[1] << 4) & 0xff)| inBuffer[2] >> 2));";
  resultHTML += "if (inBuffer[3] != END_OF_INPUT){";
  resultHTML += "result +=  ntos((((inBuffer[2] << 6)  & 0xff) | inBuffer[3]));";
  resultHTML += "} else { done = true; }";
  resultHTML += "} else { done = true; } }";
  resultHTML += "return result; }";

  vtkBase64Utilities* base64 = vtkBase64Utilities::New();

  this->parseScene(renderers, "1234567890", VTK_PARSEALL);
  const char * metadata = this->GenerateExportMetadata();
  resultHTML += "var metadata = '" + std::string(metadata) + "';";
  resultHTML += "var object = [";
  for(int i=0; i<this->GetNumberOfObjects(); i++)
    {
    std::string test;
    int size = 0;

    vtkWebGLObject* obj = this->GetWebGLObject(i);
    if (obj->isVisible())
      {
      for(int j=0; j<obj->GetNumberOfParts(); j++)
        {
        unsigned char* output = new unsigned char[obj->GetBinarySize(j)*2];
        size = base64->Encode(obj->GetBinaryData(j), obj->GetBinarySize(j), output, false);
        test = std::string((const char *)output, size);
        resultHTML += "'" + test + "',\n";
        delete[] output;
        }
      }
    }
  resultHTML += "''];";

  resultHTML += webglRenderer;
  resultHTML += glMatrix;

  resultHTML += "</script></html>";

  ofstream file;
  file.open(path.c_str());
  file << resultHTML;
  file.close();

  base64->Delete();
  }

//-----------------------------------------------------------------------------
void vtkWebGLExporter::ComputeMD5(const unsigned char* content, int size, std::string &hash)
{
  unsigned char digest[16];
  char md5Hash[33];
  md5Hash[32] = '\0';

  vtksysMD5* md5 = vtksysMD5_New();
  vtksysMD5_Initialize(md5);
  vtksysMD5_Append(md5, content, size);
  vtksysMD5_Finalize(md5, digest);
  vtksysMD5_DigestToHex(digest, md5Hash);
  vtksysMD5_Delete(md5);

  hash = md5Hash;
}
