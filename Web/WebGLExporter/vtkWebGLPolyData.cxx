/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWebGLPolyData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkWebGLPolyData.h"

#include "vtkActor.h"
#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCompositeDataGeometryFilter.h"
#include "vtkCompositeDataSet.h"
#include "vtkGenericCell.h"
#include "vtkMapper.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyDataNormals.h"
#include "vtkProperty.h"
#include "vtkScalarsToColors.h"
#include "vtkSmartPointer.h"
#include "vtkTriangleFilter.h"
#include "vtkWebGLDataSet.h"
#include "vtkWebGLExporter.h"
#include "vtkWebGLObject.h"

#include <vector>
#include <map>
#include <string>
#include <sstream>

vtkStandardNewMacro(vtkWebGLPolyData);
//*****************************************************************************
class vtkWebGLPolyData::vtkInternal
  {
public:
  std::vector<vtkWebGLDataSet*> Parts;
  std::map<long int, short> IndexMap;
  };
//*****************************************************************************

vtkWebGLPolyData::vtkWebGLPolyData()
  {
  this->webGlType = wTRIANGLES;
  this->iswidget = false;
  this->Internal = new vtkInternal();
  }

vtkWebGLPolyData::~vtkWebGLPolyData()
  {
  vtkWebGLDataSet* obj;
  while(this->Internal->Parts.size() != 0)
    {
    obj = this->Internal->Parts.back();
    this->Internal->Parts.pop_back();
    obj->Delete();
    }
  delete this->Internal;
  }

void vtkWebGLPolyData::SetMesh(float* _vertices, int _numberOfVertices, int* _index, int _numberOfIndexes, float* _normals, unsigned char* _colors, float* _tcoords, int maxSize)
  {
  this->webGlType = wTRIANGLES;

  vtkWebGLDataSet* obj;
  while(this->Internal->Parts.size() != 0)
    {
    obj = this->Internal->Parts.back();
    this->Internal->Parts.pop_back();
    obj->Delete();
    }

  short* index;
  int div = maxSize*3;
  if (_numberOfVertices < div)
    {
    index = new short[_numberOfIndexes];
    for (int i=0; i<_numberOfIndexes; i++) index[i] = (short)_index[i];

    obj = vtkWebGLDataSet::New();
    obj->SetVertices(_vertices, _numberOfVertices);
    obj->SetIndexes(index, _numberOfIndexes);
    obj->SetNormals(_normals);
    obj->SetColors(_colors);
    obj->SetMatrix(this->Matrix);
    this->Internal->Parts.push_back(obj);
    }
  else
    {
    int total = _numberOfIndexes;
    int curr = 0;
    int size = 0;

    while(curr < total)
      {
      if (div+curr > total) size = total - curr;
      else size = div;

      float* vertices = new float[size*3];
      float* normals = new float[size*3];
      unsigned char* colors = new unsigned char[size*4];
      short* indexes = new short[size];
      float* tcoord = NULL;
      if (_tcoords) tcoord = new float[size*2];

      this->Internal->IndexMap.clear();
      int count = 0;
      for(int j=0; j<size; j++)
        {
        int ind = _index[curr+j];
        if (this->Internal->IndexMap.find(ind) == this->Internal->IndexMap.end())
          {
          vertices[count*3+0] = _vertices[ind*3+0];
          vertices[count*3+1] = _vertices[ind*3+1];
          vertices[count*3+2] = _vertices[ind*3+2];

          normals[count*3+0] = _normals[ind*3+0];
          normals[count*3+1] = _normals[ind*3+1];
          normals[count*3+2] = _normals[ind*3+2];

          colors[count*4+0] = _colors[ind*4+0];
          colors[count*4+1] = _colors[ind*4+1];
          colors[count*4+2] = _colors[ind*4+2];
          colors[count*4+3] = _colors[ind*4+3];

          if (_tcoords)
            {
            tcoord[count*2+0] = _tcoords[ind*2+0];
            tcoord[count*2+1] = _tcoords[ind*2+1];
            }
          this->Internal->IndexMap[ind] = count;
          indexes[j] = count++;
          }
        else
          {
          indexes[j] = this->Internal->IndexMap[ind];
          }
        }
      curr += size;
      float* v = new float[count*3]; memcpy(v, vertices, count*3*sizeof(float)); delete[] vertices;
      float* n = new float[count*3]; memcpy(n, normals, count*3*sizeof(float)); delete[] normals;
      unsigned char* c = new unsigned char[count*4]; memcpy(c, colors, count*4); delete[] colors;
      obj = vtkWebGLDataSet::New();
      obj->SetVertices(v, count);
      obj->SetIndexes(indexes, size);
      obj->SetNormals(n);
      obj->SetColors(c);
      if (_tcoords)
        {
        float* tc = new float[count*2]; memcpy(tc, tcoord, count*2*sizeof(float)); delete[] tcoord;
        obj->SetTCoords(tc);
        }
      obj->SetMatrix(this->Matrix);
      this->Internal->Parts.push_back(obj);
      }

    delete[] _vertices;
    delete[] _index;
    delete[] _normals;
    delete[] _colors;
    if (_tcoords) delete[] _tcoords;
    }
  }

void vtkWebGLPolyData::SetLine(float *_points, int _numberOfPoints, int *_index, int _numberOfIndex, unsigned char *_colors, int maxSize)
  {
  this->webGlType = wLINES;

  vtkWebGLDataSet* obj;
  while(this->Internal->Parts.size() != 0)
    {
    obj = this->Internal->Parts.back();
    this->Internal->Parts.pop_back();
    obj->Delete();
    }

  short* index;
  int div = maxSize*2;
  if (_numberOfPoints < div)
    {
    index = new short[_numberOfIndex];
    for (int i=0; i<_numberOfIndex; i++) index[i] = (short)((unsigned int)_index[i]);
    obj = vtkWebGLDataSet::New();
    obj->SetPoints(_points, _numberOfPoints);
    obj->SetIndexes(index, _numberOfIndex);
    obj->SetColors(_colors);
    obj->SetMatrix(this->Matrix);
    this->Internal->Parts.push_back(obj);
    }
  else
    {
    int total = _numberOfIndex;
    int curr = 0;
    int size = 0;

    while(curr < total)
      {
      if (div+curr > total) size = total - curr;
      else size = div;

      float* points = new float[size*3];
      unsigned char* colors = new unsigned char[size*4];
      short* indexes = new short[size];

      for(int j=0; j<size; j++)
        {
        indexes[j] = j;

        points[j*3+0] = _points[_index[curr+j]*3+0];
        points[j*3+1] = _points[_index[curr+j]*3+1];
        points[j*3+2] = _points[_index[curr+j]*3+2];

        colors[j*4+0] = _colors[_index[curr+j]*4+0];
        colors[j*4+1] = _colors[_index[curr+j]*4+1];
        colors[j*4+2] = _colors[_index[curr+j]*4+2];
        colors[j*4+3] = _colors[_index[curr+j]*4+3];
        }
      curr += size;
      obj = vtkWebGLDataSet::New();
      obj->SetPoints(points, size);
      obj->SetIndexes(indexes, size);
      obj->SetColors(colors);
      obj->SetMatrix(this->Matrix);
      this->Internal->Parts.push_back(obj);
      }
    delete[] _points;
    delete[] _index;
    delete[] _colors;
    }
  }

void vtkWebGLPolyData::SetTransformationMatrix(vtkMatrix4x4* m)
  {
  this->Superclass::SetTransformationMatrix(m);
  for (size_t i=0; i<this->Internal->Parts.size(); i++)
    {
    this->Internal->Parts[i]->SetMatrix(this->Matrix);
    }
  }

unsigned char* vtkWebGLPolyData::GetBinaryData(int part)
  {
  this->hasChanged = false;
  vtkWebGLDataSet* obj = this->Internal->Parts[part];
  return obj->GetBinaryData();
  }

int vtkWebGLPolyData::GetBinarySize(int part)
  {
  vtkWebGLDataSet* obj = this->Internal->Parts[part];
  return obj->GetBinarySize();
  }

void vtkWebGLPolyData::GenerateBinaryData()
  {
  vtkWebGLDataSet* obj;
  this->hasChanged = false;
  std::stringstream ss;
  for(size_t i=0; i<this->Internal->Parts.size(); i++)
    {
    obj = this->Internal->Parts[i];
    obj->GenerateBinaryData();
    ss << obj->GetMD5();
    }
  if(this->Internal->Parts.size() != 0)
    {
    std::string localMD5;
    vtkWebGLExporter::ComputeMD5((unsigned char*)ss.str().c_str(), static_cast<int>(ss.str().size()), localMD5);
    this->hasChanged = this->MD5.compare(localMD5) != 0;
    this->MD5 = localMD5;
    }
  else cout << "Warning: GenerateBinaryData() @ vtkWebGLObject: This isn\'t supposed to happen.";
  }

int vtkWebGLPolyData::GetNumberOfParts()
  {
  return static_cast<int>(this->Internal->Parts.size());
  }

void vtkWebGLPolyData::PrintSelf(ostream& os, vtkIndent indent)
  {
  this->Superclass::PrintSelf(os, indent);
  }

void vtkWebGLPolyData::GetLinesFromPolygon(vtkMapper *mapper, vtkActor *actor, int lineMaxSize, double* edgeColor)
  {
  vtkWebGLPolyData *object = this;
  vtkDataSet* dataset = NULL;
  vtkSmartPointer<vtkDataSet> tempDS;
  vtkDataObject* dObj = mapper->GetInputDataObject(0, 0);
  vtkCompositeDataSet* cd = vtkCompositeDataSet::SafeDownCast(dObj);
  if (cd)
    {
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
    }

  int np = 0;
  int size = 0;
  for(int i=0; i<dataset->GetNumberOfCells(); i++) size += dataset->GetCell(i)->GetNumberOfPoints();

  float* points = new float[size*3];
  unsigned char* color = new unsigned char[size*4];
  int* index = new int[size*2];
  double* point;
  int pos = 0;

  vtkScalarsToColors* table = mapper->GetLookupTable();
  vtkDataArray* array;
  if (mapper->GetScalarMode() == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA)
    {
    vtkCellData* celldata = dataset->GetCellData();
    if (actor->GetMapper()->GetArrayAccessMode() == VTK_GET_ARRAY_BY_ID)
      array = celldata->GetArray(actor->GetMapper()->GetArrayId());
    else
      array = celldata->GetArray(actor->GetMapper()->GetArrayName());
    }
  else
    {
    vtkPointData* pointdata = dataset->GetPointData();
    if (actor->GetMapper()->GetArrayAccessMode() == VTK_GET_ARRAY_BY_ID)
      array = pointdata->GetArray(actor->GetMapper()->GetArrayId());
    else
      array = pointdata->GetArray(actor->GetMapper()->GetArrayName());
    }

  int colorComponent = table->GetVectorComponent();
  int numberOfComponents = 0;
  if (array != NULL) numberOfComponents = array->GetNumberOfComponents();
  int mode = table->GetVectorMode();
  double mag=0, rgb[3];
  int curr=0;
  for(int i=0; i<dataset->GetNumberOfCells(); i++)
    {
    vtkCell* cell = dataset->GetCell(i);
    int b = pos;
    np = dataset->GetCell(i)->GetNumberOfPoints();
    for(int j=0; j<np; j++)
      {
      point = cell->GetPoints()->GetPoint(j);
      points[curr*3 + j*3 + 0] = point[0];
      points[curr*3 + j*3 + 1] = point[1];
      points[curr*3 + j*3 + 2] = point[2];

      index[curr*2 + j*2 + 0] = pos++;
      index[curr*2 + j*2 + 1] = pos;
      if(j==np-1) index[curr*2 + j*2 + 1] = b;

      vtkIdType pointId = cell->GetPointIds()->GetId(j);
      if (numberOfComponents == 0)
        {
        actor->GetProperty()->GetColor(rgb);
        }
      else
        {
        switch(mode)
          {
        case vtkScalarsToColors::MAGNITUDE:
            mag = 0; for (int w=0; w<numberOfComponents; w++) mag += (double)array->GetComponent(pointId, w)*(double)array->GetComponent(pointId, w);
            mag = sqrt(mag);
            table->GetColor(mag, &rgb[0]);
            break;
          case vtkScalarsToColors::COMPONENT:
            mag = array->GetComponent(pointId, colorComponent);
            table->GetColor(mag, &rgb[0]);
            break;
          case vtkScalarsToColors::RGBCOLORS:
            array->GetTuple(pointId, &rgb[0]);
            break;
          }
        }
      if (edgeColor != NULL) memcpy(rgb, edgeColor, sizeof(double)*3);
      color[curr*4 + j*4 + 0] = (unsigned char)((int)(rgb[0]*255));
      color[curr*4 + j*4 + 1] = (unsigned char)((int)(rgb[1]*255));
      color[curr*4 + j*4 + 2] = (unsigned char)((int)(rgb[2]*255));
      color[curr*4 + j*4 + 3] = (unsigned char)255;

      }
    curr += np;
    }
  object->SetLine(points, size, index, size*2, color, lineMaxSize);
  }

void vtkWebGLPolyData::GetLines(vtkTriangleFilter* polydata, vtkActor* actor, int lineMaxSize)
  {
  vtkWebGLPolyData* object = this;
  vtkCellArray* lines = polydata->GetOutput(0)->GetLines();

  // Index
  //Array of 3 Values. [#number of index, i1, i2]
  //Discarting the first value
  int* index = new int[lines->GetData()->GetSize()*2/3];
  for (int i=0; i< lines->GetData()->GetSize(); i++) if (i%3 != 0) index[i*2/3] = lines->GetData()->GetValue(i);
  // Point
  double point[3];
  float* points = new float[polydata->GetOutput(0)->GetNumberOfPoints()*3];
  for (int i=0; i<polydata->GetOutput(0)->GetNumberOfPoints(); i++)
    {
    polydata->GetOutput(0)->GetPoint(i, point);
    points[i*3+0] = point[0];
    points[i*3+1] = point[1];
    points[i*3+2] = point[2];
    }
  // Colors
  unsigned char* color = new unsigned char[polydata->GetOutput(0)->GetNumberOfPoints()*4];
  this->GetColorsFromPolyData(color, polydata->GetOutput(0), actor);

  object->SetLine(points, polydata->GetOutput(0)->GetNumberOfPoints(), index, lines->GetData()->GetSize()*2/3, color, lineMaxSize);
  }

void vtkWebGLPolyData::SetPoints(float *points, int numberOfPoints, unsigned char *colors, int maxSize)
  {
  this->webGlType = wPOINTS;

  // Delete Old Objects
  vtkWebGLDataSet* obj;
  while(this->Internal->Parts.size() != 0)
    {
    obj = this->Internal->Parts.back();
    this->Internal->Parts.pop_back();
    obj->Delete();
    }

  // Create new objs
  int numObjs = (numberOfPoints/maxSize)+1;
  int offset = 0;
  int size = 0;
  for(int i=0; i<numObjs; i++)
    {
    size = numberOfPoints - offset;
    if (size > maxSize) size = maxSize;

    float* _points = new float[size*3];
    unsigned char* _colors = new unsigned char[size*4];
    memcpy(_points, &points[offset*3], size*3*sizeof(float));
    memcpy(_colors, &colors[offset*4], size*4*sizeof(unsigned char));

    obj = vtkWebGLDataSet::New();
    obj->SetPoints(_points, size);
    obj->SetColors(_colors);
    obj->SetType(wPOINTS);
    obj->SetMatrix(this->Matrix);
    this->Internal->Parts.push_back(obj);

    offset += size;
    }

  delete[] points;
  delete[] colors;
  }

void vtkWebGLPolyData::GetPoints(vtkTriangleFilter *polydata, vtkActor *actor, int maxSize)
  {
  vtkWebGLPolyData* object = this;

  // Points
  double point[3];
  float* points = new float[polydata->GetOutput(0)->GetNumberOfPoints()*3];
  for (int i=0; i<polydata->GetOutput(0)->GetNumberOfPoints(); i++)
    {
    polydata->GetOutput(0)->GetPoint(i, point);
    points[i*3+0] = point[0];
    points[i*3+1] = point[1];
    points[i*3+2] = point[2];
    }
  // Colors
  unsigned char* colors = new unsigned char[polydata->GetOutput(0)->GetNumberOfPoints()*4];
  this->GetColorsFromPolyData(colors, polydata->GetOutput(0), actor);

  object->SetPoints(points, polydata->GetOutput(0)->GetNumberOfPoints(), colors, maxSize);
  }

void vtkWebGLPolyData::GetColorsFromPolyData(unsigned char* color, vtkPolyData* polydata, vtkActor* actor)
  {
  int celldata;
  vtkDataArray* array = vtkAbstractMapper::GetScalars(polydata, actor->GetMapper()->GetScalarMode(),
                                                      actor->GetMapper()->GetArrayAccessMode(), actor->GetMapper()->GetArrayId(),
                                                      actor->GetMapper()->GetArrayName(), celldata);
  if (actor->GetMapper()->GetScalarVisibility() && array != NULL)
    {
    vtkScalarsToColors* table = actor->GetMapper()->GetLookupTable();

    vtkUnsignedCharArray* cor = table->MapScalars(array, table->GetVectorMode(), table->GetVectorComponent());
    memcpy(color, cor->GetPointer(0), polydata->GetNumberOfPoints()*4);
    cor->Delete();
    }
  else
    {
    for (int i=0; i<polydata->GetNumberOfPoints(); i++)
      {
      color[i*4+0] = (unsigned char)255;
      color[i*4+1] = (unsigned char)255;
      color[i*4+2] = (unsigned char)255;
      color[i*4+3] = (unsigned char)255;
      }
    }
  }

void vtkWebGLPolyData::GetPolygonsFromPointData(vtkTriangleFilter* polydata, vtkActor* actor, int maxSize)
  {
  vtkWebGLPolyData* object = this;

  vtkPolyDataNormals* polynormals = vtkPolyDataNormals::New();
  polynormals->SetInputConnection(polydata->GetOutputPort(0));
  polynormals->Update();

  vtkPolyData* data = polynormals->GetOutput();

  vtkCellArray* poly = data->GetPolys();
  vtkPointData* point = data->GetPointData();
  vtkIdTypeArray* ndata = poly->GetData();
  vtkDataSetAttributes* attr = (vtkDataSetAttributes*)point;

  //Vertices
  float* vertices = new float[data->GetNumberOfPoints()*3];
  for (int i=0; i<data->GetNumberOfPoints()*3; i++) vertices[i] = data->GetPoint(i/3)[i%3];
  //Index
  // ndata contain 4 values for the normal: [number of values per index, index[3]]
  // We dont need the first value
  int* indexes = new int[ndata->GetSize()*3/4];
  for (int i=0; i<ndata->GetSize(); i++)
    if (i%4 != 0) indexes[i*3/4] = ndata->GetValue(i);
  //Normal
  float* normal = new float[attr->GetNormals()->GetSize()];
  for (int i=0; i< attr->GetNormals()->GetSize(); i++) normal[i] = attr->GetNormals()->GetComponent(0, i);
  //Colors
  unsigned char* color = new unsigned char[data->GetNumberOfPoints()*4];
  this->GetColorsFromPointData(color, point, data, actor);
  //TCoord
  float* tcoord = NULL;
  if (attr->GetTCoords())
    {
    tcoord = new float[attr->GetTCoords()->GetSize()];
    for (int i=0; i<attr->GetTCoords()->GetSize(); i++) tcoord[i] = attr->GetTCoords()->GetComponent(0, i);
    }

  object->SetMesh(vertices, data->GetNumberOfPoints(), indexes, ndata->GetSize()*3/4, normal, color, tcoord, maxSize);
  polynormals->Delete();
  }

void vtkWebGLPolyData::GetPolygonsFromCellData(vtkTriangleFilter* polydata, vtkActor* actor, int maxSize)
  {
  vtkWebGLPolyData* object = this;

  vtkPolyDataNormals* polynormals = vtkPolyDataNormals::New();
  polynormals->SetInputConnection(polydata->GetOutputPort(0));
  polynormals->Update();

  vtkPolyData* data = polynormals->GetOutput();
  vtkCellData* celldata = data->GetCellData();

  vtkDataArray* array;
  if (actor->GetMapper()->GetArrayAccessMode() == VTK_GET_ARRAY_BY_ID)
    array = celldata->GetArray(actor->GetMapper()->GetArrayId());
  else
    array = celldata->GetArray(actor->GetMapper()->GetArrayName());
  vtkScalarsToColors* table = actor->GetMapper()->GetLookupTable();
  int colorComponent = table->GetVectorComponent();
  int mode = table->GetVectorMode();


  float* vertices = new float[data->GetNumberOfCells()*3*3];
  float* normals  = new float[data->GetNumberOfCells()*3*3];
  unsigned char* colors   = new unsigned char[data->GetNumberOfCells()*3*4];
  int* indexes  = new int[data->GetNumberOfCells()*3*3];

  vtkGenericCell* cell = vtkGenericCell::New();
  double tuple[3], normal[3], color[3];
  color[0] = 1.0; color[1] = 1.0; color[2] = 1.0;
  vtkPoints* points;
  int aux;
  double mag, alpha=1.0;
  int numberOfComponents = 0;
  if (array) numberOfComponents = array->GetNumberOfComponents();
  else mode = -1;
  for(int i=0; i<data->GetNumberOfCells(); i++)
    {
    data->GetCell(i, cell);
    points = cell->GetPoints();

    //getColors
    alpha = 1.0;
    switch(mode)
      {
    case -1:
        actor->GetProperty()->GetColor(color);
        alpha = actor->GetProperty()->GetOpacity();
        break;
      case vtkScalarsToColors::MAGNITUDE:
        mag = 0; for (int w=0; w<numberOfComponents; w++) mag += (double)array->GetComponent(i, w)*(double)array->GetComponent(i, w);
        mag = sqrt(mag);
        table->GetColor(mag, &color[0]);
        alpha = table->GetOpacity(mag);
        break;
      case vtkScalarsToColors::COMPONENT:
        mag = array->GetComponent(i, colorComponent);
        table->GetColor(mag, &color[0]);
        alpha = table->GetOpacity(mag);
        break;
      case vtkScalarsToColors::RGBCOLORS:
        array->GetTuple(i, &color[0]);
        break;
      }
    //getNormals
    celldata->GetNormals()->GetTuple(i, &normal[0]);
    for(int j=0; j<3; j++)
      {
      aux = i*9+j*3;
      //Normals
      normals[aux+0] = normal[0]; normals[aux+1] = normal[1]; normals[aux+2] = normal[2];
      //getVertices
      points->GetPoint(j, &tuple[0]);
      vertices[aux+0] = tuple[0]; vertices[aux+1] = tuple[1]; vertices[aux+2] = tuple[2];
      //Colors
      colors[4*(3*i+j)+0] = (unsigned char)((int)(color[0]*255)); colors[4*(3*i+j)+1] = (unsigned char)((int)(color[1]*255));
      colors[4*(3*i+j)+2] = (unsigned char)((int)(color[2]*255)); colors[4*(3*i+j)+3] = (unsigned char)((int)(alpha*255));
      //getIndexes
      indexes[aux+0] = aux+0; indexes[aux+1] = aux+1; indexes[aux+2] = aux+2;
      }
    }
  object->SetMesh(vertices, data->GetNumberOfCells()*3, indexes, data->GetNumberOfCells()*3, normals, colors, NULL, maxSize);
  cell->Delete();
  polynormals->Delete();
  }

void vtkWebGLPolyData::GetColorsFromPointData(unsigned char* color, vtkPointData* pointdata, vtkPolyData* polydata, vtkActor* actor)
  {
  vtkDataSetAttributes* attr = (vtkDataSetAttributes*)pointdata;

  int colorSize = attr->GetNormals()->GetSize()*4/3;

  vtkDataArray* array;
  if (actor->GetMapper()->GetArrayAccessMode() == VTK_GET_ARRAY_BY_ID)
    array = pointdata->GetArray(actor->GetMapper()->GetArrayId());
  else
    array = pointdata->GetArray(actor->GetMapper()->GetArrayName());

  if (array && actor->GetMapper()->GetScalarVisibility() && actor->GetMapper()->GetArrayName() != NULL && actor->GetMapper()->GetArrayName()[0] != '\0')
    {
    vtkScalarsToColors* table = actor->GetMapper()->GetLookupTable();
    int colorComponent = table->GetVectorComponent(), numberOfComponents = array->GetNumberOfComponents();
    int mode = table->GetVectorMode();
    double mag=0, rgb[3];
    double alpha = 1.0;

    if (numberOfComponents == 1 && mode == vtkScalarsToColors::MAGNITUDE)
      {
      mode = vtkScalarsToColors::COMPONENT;
      colorComponent = 0;
      }
    for (int i=0; i<colorSize/4; i++)
      {
      switch(mode)
        {
      case vtkScalarsToColors::MAGNITUDE:
          mag = 0; for (int w=0; w<numberOfComponents; w++) mag += (double)array->GetComponent(i, w)*(double)array->GetComponent(i, w);
          mag = sqrt(mag);
          table->GetColor(mag, &rgb[0]);
          alpha = table->GetOpacity(mag);
          break;
        case vtkScalarsToColors::COMPONENT:
          mag = array->GetComponent(i, colorComponent);
          table->GetColor(mag, &rgb[0]);
          alpha = table->GetOpacity(mag);
          break;
        case vtkScalarsToColors::RGBCOLORS:
          array->GetTuple(i, &rgb[0]);
          alpha = actor->GetProperty()->GetOpacity();
          break;
        }
      color[i*4+0] = (unsigned char)((int)(rgb[0]*255));
      color[i*4+1] = (unsigned char)((int)(rgb[1]*255));
      color[i*4+2] = (unsigned char)((int)(rgb[2]*255));
      color[i*4+3] = (unsigned char)((int)(alpha*255));
      }
    }
  else
    {
    double rgb[3];
    double alpha = 0;
    int celldata;
    array = vtkAbstractMapper::GetScalars(polydata, actor->GetMapper()->GetScalarMode(),
                                          actor->GetMapper()->GetArrayAccessMode(), actor->GetMapper()->GetArrayId(),
                                          actor->GetMapper()->GetArrayName(), celldata);
    if (actor->GetMapper()->GetScalarVisibility() &&
        (actor->GetMapper()->GetColorMode() == VTK_COLOR_MODE_DEFAULT ||
         actor->GetMapper()->GetColorMode() == VTK_COLOR_MODE_DIRECT_SCALARS) &&
        array)
      {
      vtkScalarsToColors* table = actor->GetMapper()->GetLookupTable();
      vtkUnsignedCharArray* cor = table->MapScalars(array, table->GetVectorMode(), table->GetVectorComponent());
      memcpy(color, cor->GetPointer(0), polydata->GetNumberOfPoints()*4);
      cor->Delete();
      }
    else
      {
      actor->GetProperty()->GetColor(rgb);
      alpha = actor->GetProperty()->GetOpacity();
      for (int i=0; i<colorSize/4; i++)
        {
        color[i*4+0] = (unsigned char)((int)(rgb[0]*255));
        color[i*4+1] = (unsigned char)((int)(rgb[1]*255));
        color[i*4+2] = (unsigned char)((int)(rgb[2]*255));
        color[i*4+3] = (unsigned char)((int)(alpha*255));
        }
      }
    }
  }
