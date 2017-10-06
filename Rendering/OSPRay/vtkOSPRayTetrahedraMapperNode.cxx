/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOSPRayTetrahedraMapperNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOSPRayTetrahedraMapperNode.h"

#include "vtkArrayDispatch.h"
#include "vtkCellArray.h"
#include "vtkCellCenterDepthSort.h"
#include "vtkCellData.h"
#include "vtkColorTransferFunction.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkGarbageCollector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVisibilitySort.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

#include "vtkVolumeNode.h"
#include "vtkAbstractVolumeMapper.h"
#include "vtkUnstructuredGridVolumeMapper.h"
#include "vtkOSPRayRendererNode.h"
#include "vtkUnstructuredGridVolumeRayCastMapper.h"

#include <cmath>
#include <algorithm>

#include "ospray/ospray.h"

vtkStandardNewMacro(vtkOSPRayTetrahedraMapperNode);

//-----------------------------------------------------------------------------
vtkOSPRayTetrahedraMapperNode::vtkOSPRayTetrahedraMapperNode()
{
  this->SamplingRate=0.0f;
  this->NumColors = 128;
  this->OSPRayVolume = NULL;
  this->TransferFunction = NULL;
}

vtkOSPRayTetrahedraMapperNode::~vtkOSPRayTetrahedraMapperNode()
{
  delete this->OSPRayVolume;
  ospRelease(this->TransferFunction);
}

void vtkOSPRayTetrahedraMapperNode::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkAbstractArray *vtkOSPRayTetrahedraMapperNode::GetArrayToProcess(vtkDataSet* input, int& cellFlag)
{
  cellFlag = -1;
  vtkAbstractVolumeMapper* mapper = vtkAbstractVolumeMapper::SafeDownCast(this->GetRenderable());
  if (!mapper)
  {
    return NULL;
  }
  vtkAbstractArray *scalars;
  int scalarMode = mapper->GetScalarMode();
  if ( scalarMode == VTK_SCALAR_MODE_DEFAULT )
  {
    scalars = input->GetPointData()->GetScalars();
    cellFlag = 0;
    if (!scalars)
    {
      scalars = input->GetCellData()->GetScalars();
      cellFlag = 1;
    }
    return scalars;
  }
  if ( scalarMode == VTK_SCALAR_MODE_USE_POINT_DATA )
  {
    scalars = input->GetPointData()->GetScalars();
    cellFlag = 0;
    return scalars;
  }
  if ( scalarMode == VTK_SCALAR_MODE_USE_CELL_DATA )
  {
    scalars = input->GetCellData()->GetScalars();
    cellFlag = 1;
    return scalars;
  }

  int arrayAccessMode = mapper->GetArrayAccessMode();
  const char *arrayName = mapper->GetArrayName();
  int arrayId = mapper->GetArrayId();
  vtkPointData *pd;
  vtkCellData *cd;
  vtkFieldData *fd;
  if ( scalarMode == VTK_SCALAR_MODE_USE_POINT_FIELD_DATA )
  {
    pd = input->GetPointData();
    if (arrayAccessMode == VTK_GET_ARRAY_BY_ID)
    {
      scalars = pd->GetAbstractArray(arrayId);
    }
    else
    {
      scalars = pd->GetAbstractArray(arrayName);
    }
    cellFlag = 0;
    return scalars;
  }

  if ( scalarMode == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA )
  {
    cd = input->GetCellData();
    if (arrayAccessMode == VTK_GET_ARRAY_BY_ID)
    {
      scalars = cd->GetAbstractArray(arrayId);
    }
    else
    {
      scalars = cd->GetAbstractArray(arrayName);
    }
    cellFlag = 1;
    return scalars;
  }

  if ( scalarMode == VTK_SCALAR_MODE_USE_FIELD_DATA )
  {
    fd = input->GetFieldData();
    if (arrayAccessMode == VTK_GET_ARRAY_BY_ID)
    {
      scalars = fd->GetAbstractArray(arrayId);
    }
    else
    {
      scalars = fd->GetAbstractArray(arrayName);
    }
    cellFlag = 2;
    return scalars;
  }

  return NULL;
}

//----------------------------------------------------------------------------
void vtkOSPRayTetrahedraMapperNode::Render(bool prepass)
{
  if (prepass) {
    vtkUnstructuredGridVolumeRayCastMapper *mapper =
    vtkUnstructuredGridVolumeRayCastMapper::SafeDownCast(this->GetRenderable());

    if (!mapper) {
      vtkErrorMacro("invalid mapper");
      return;
    }

    vtkVolumeNode *volNode = vtkVolumeNode::SafeDownCast(this->Parent);
    if (!volNode) {
      vtkErrorMacro("invalid volumeNode");
      return;
    }

    vtkVolume* vol = vtkVolume::SafeDownCast(volNode->GetRenderable());
    if (vol->GetVisibility() == false) {
      return;
    }

    if (!vol->GetProperty()) {
      // this is OK, happens in paraview client side for instance
      return;
    }

    mapper->GetInputAlgorithm()->UpdateInformation();
    mapper->GetInputAlgorithm()->Update();

    vtkOSPRayRendererNode *orn = static_cast<vtkOSPRayRendererNode *>(
                         this->GetFirstAncestorOfType("vtkOSPRayRendererNode"));

    osp::Model* OSPRayModel = orn->GetOModel();
    if (!OSPRayModel) {
      return;
    }

    vtkDataSet *dataSet = mapper->GetDataSetInput();
    if (!dataSet) {
      return;
    }

    int numberOfCells = dataSet->GetNumberOfCells();
    int numberOfPoints = dataSet->GetNumberOfPoints();

    double point[3];
    for (int i=0; i<numberOfPoints; i++) {
      dataSet->GetPoint(i,point);
      osp::vec3f v;
      v.x = point[0];
      v.y = point[1];
      v.z = point[2];
      vertices.push_back(v);
    }

    for (int i=0; i<numberOfCells; i++) {
      vtkCell *cell = dataSet->GetCell(i);

      if (cell->GetCellType() == 10) {//vtkTetra
        for (int j=0; j<4; j++) {
          cells.push_back(cell->GetPointId(j));
        }
      }
    }

    // Now check for point data
    vtkPointData *pd = dataSet->GetPointData();
    if (pd) {
      for (int i = 0; i < pd->GetNumberOfArrays(); i++) {
        vtkAbstractArray *ad = pd->GetAbstractArray(i);
        int nDataPoints = ad->GetNumberOfValues();
        int arrayType = ad->GetArrayType();

        vtkSmartPointer<vtkFloatArray> array = vtkFloatArray::SafeDownCast(ad);
        //VTK_FLOAT only, need to add support for other data types.
        if (!array)
          vtkErrorMacro("unsupported array type");

        for(int j=0; j<nDataPoints; j++) {
          float val = array->GetValue(j);
          field.push_back(val);
        }
      }
    }

    if (!this->TransferFunction)
    {
      this->TransferFunction = ospNewTransferFunction("piecewise_linear");
    }

    // when input data is modified
    if (mapper->GetDataSetInput()->GetMTime() > this->BuildTime)
    {
      if (this->OSPRayVolume) {
        delete this->OSPRayVolume;
        vertices.clear();
        cells.clear();
        field.clear();
      }
      this->OSPRayVolume = ospNewVolume("tetrahedral_volume");
      assert(this->OSPRayVolume);

      OSPData verticesData = ospNewData(vertices.size(),OSP_FLOAT3,
                vertices.data(),0);
      assert(verticesData);
      ospSetData(this->OSPRayVolume, "vertices", verticesData);

      OSPData fieldData = ospNewData(field.size(),OSP_FLOAT,field.data(),0);
      assert(fieldData);
      ospSetData(this->OSPRayVolume, "field", fieldData);

      OSPData tetrahedraData = ospNewData(cells.size()/4,OSP_INT4,cells.data(),0);
      assert(tetrahedraData);
      ospSetData(this->OSPRayVolume, "tetrahedra", tetrahedraData);

      ospSet1i(this->OSPRayVolume, "nVertices", vertices.size());
      ospSet1i(this->OSPRayVolume, "nTetrahedra", cells.size()/4);
    }

    double* dim = mapper->GetBounds();
    int minBound = std::min(std::min(dim[1]-dim[0],dim[3]-dim[2]),dim[5]-dim[4]);
    float samplingStep = minBound*0.01f;

    // test for modifications to volume properties
    vtkVolumeProperty* volProperty = vol->GetProperty();
    if (vol->GetProperty()->GetMTime() > this->PropertyTime
        || mapper->GetDataSetInput()->GetMTime() > this->BuildTime)
    {
      // Get transfer function. ++++++++++++++++++++++++++++++++++++++++++++++++++

      int fieldAssociation;
      vtkDataArray *sa = vtkDataArray::SafeDownCast(
            this->GetArrayToProcess(dataSet, fieldAssociation));
      if (!sa) {
        vtkErrorMacro("VolumeMapper's Input has no scalar array!");
        return;
      }

      vtkColorTransferFunction* colorTF = volProperty->GetRGBTransferFunction(0);
      vtkPiecewiseFunction *scalarTF = volProperty->GetScalarOpacity(0);

      this->TFVals.resize(this->NumColors*3);
      this->TFOVals.resize(this->NumColors);
      scalarTF->GetTable(sa->GetRange()[0],
             sa->GetRange()[1],
             this->NumColors,
             &TFOVals[0]);
      colorTF->GetTable(sa->GetRange()[0],
            sa->GetRange()[1],
            this->NumColors,
            &this->TFVals[0]);

      float scalarOpacityUnitDistance = volProperty->GetScalarOpacityUnitDistance();
      if (scalarOpacityUnitDistance < 1e-29) //avoid div by 0
      {
        scalarOpacityUnitDistance = 1e-29;
      }
      for(int i=0; i < this->NumColors; i++)
        this->TFOVals[i] = this->TFOVals[i]/scalarOpacityUnitDistance*samplingStep;

      OSPData colorData = ospNewData(this->NumColors,
             OSP_FLOAT3,
             &this->TFVals[0]);
      ospSetData(this->TransferFunction, "colors", colorData);

      OSPData tfAlphaData = ospNewData(NumColors, OSP_FLOAT, &TFOVals[0]);
      ospSetData(this->TransferFunction, "opacities", tfAlphaData);

      ospSet2f(this->TransferFunction, "valueRange", sa->GetRange()[0],
         sa->GetRange()[1]);

      ospSet1i(this->OSPRayVolume, "gradientShadingEnabled",
         volProperty->GetShade());

      ospCommit(this->TransferFunction);
      ospRelease(colorData);
      ospRelease(tfAlphaData);

      ospSetObject(this->OSPRayVolume, "transferFunction",
       this->TransferFunction);

      this->PropertyTime.Modified();
    }

    ospSet1f(OSPRayVolume, "samplingStep", samplingStep);
    ospSet1f(OSPRayVolume, "adaptiveMaxSamplingRate", 2.0f);
    ospSet1f(OSPRayVolume, "adaptiveBacktrack", 0.01f);
    ospSet1i(OSPRayVolume, "adaptiveSampling", 1);
    if (this->SamplingRate == 0.0f)  // 0 means automatic sampling rate
    {
      //automatically determine sampling rate
      float minSamplingRate = 0.075f; // lower for min adaptive sampling step
      if (minBound < 100)
      {
        float s = (100.0f - minBound)/100.0f;
        ospSet1f(this->OSPRayVolume, "samplingRate", s*2.0f + 0.2f);
      }
      else
      {
        ospSet1f(this->OSPRayVolume, "samplingRate", 0.2f);
      }
    }
    else
    {
      ospSet1f(this->OSPRayVolume, "samplingRate", this->SamplingRate);
    }
    ospSet1f(this->OSPRayVolume, "adaptiveScalar", 15.f);
    float rs = static_cast<float>(volProperty->GetSpecular(0));
    float gs = static_cast<float>(volProperty->GetSpecular(1));
    float bs = static_cast<float>(volProperty->GetSpecular(2));
    ospSet3f(this->OSPRayVolume, "specular", rs,gs,bs);
    ospSet1i(this->OSPRayVolume, "preIntegration", 0); //turn off preIntegration

    ospCommit(this->OSPRayVolume);
    ospAddVolume(OSPRayModel, this->OSPRayVolume);
    this->RenderTime = volNode->GetMTime();
    this->BuildTime.Modified();
  }
}
