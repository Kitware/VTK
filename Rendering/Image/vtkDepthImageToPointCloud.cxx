/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDepthImageToPointCloud.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDepthImageToPointCloud.h"

#include "vtkCommand.h"
#include "vtkFloatArray.h"
#include "vtkCellArray.h"
#include "vtkPolyData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkCamera.h"
#include "vtkMatrix4x4.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkCoordinate.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkSMPTools.h"
#include "vtkArrayListTemplate.h" // For processing attribute data

vtkStandardNewMacro(vtkDepthImageToPointCloud);
vtkCxxSetObjectMacro(vtkDepthImageToPointCloud,Camera,vtkCamera);

//----------------------------------------------------------------------------
// Helper classes to support efficient computing, and threaded execution.
namespace {

  // Map input point id to output point id. This map is needed because of the
  // optionally capability to cull near and far points.
  template <typename T>
  void MapPoints(vtkIdType numPts, T *depths, bool cullNear, bool cullFar,
                 vtkIdType *map, vtkIdType &numOutPts)
  {
    numOutPts = 0;
    float d;
    for (vtkIdType ptId=0; ptId < numPts; ++depths, ++map, ++ptId)
    {
      d = static_cast<float>(*depths);
      if ( (cullNear && d <= 0.0) || (cullFar && d >= 1.0) )
      {
        *map = (-1);
      }
      else
      {
        *map = numOutPts++;
      }
    }
  }

  // This class performs point by point transformation. The view matrix is
  // used to transform each pixel. IMPORTANT NOTE: The transformation occurs
  // by normalizing the image pixels into the (-1,1) view space (depth values
  // are passed thru). The process follows the vtkCoordinate class which is
  // the standard for VTK rendering transformations. Subtle differences in
  // whether the lower left pixel origin are at the center of the pixel
  // versus the lower-left corner of the pixel will make slight differences
  // in how pixels are transformed. (Similarly for the upper right pixel as
  // well). This half pixel difference can cause transformation issues. Here
  // we've played around with the scaling below to produce the best results
  // in the current version of VTK.
  template <typename TD, typename TP>
  struct MapDepthImage
  {
    const TD *Depths;
    TP *Pts;
    const int *Dims;
    const double *Matrix;
    const vtkIdType *PtMap;

    MapDepthImage(TD *depths, TP *pts, int dims[2], double *m, vtkIdType *ptMap) :
      Depths(depths), Pts(pts), Dims(dims), Matrix(m), PtMap(ptMap)
    {
    }

    void  operator()(vtkIdType row, vtkIdType end)
    {
        double drow, result[4];
        vtkIdType offset = row*this->Dims[0];
        const TD *dptr = this->Depths + offset;
        const vtkIdType *mptr = this->PtMap + offset;
        TP *pptr;
        for ( ; row < end; ++row )
        {
          drow = -1.0 + (2.0*static_cast<double>(row) / static_cast<double>(this->Dims[1]-1));
          // if pixel origin is pixel center use the two lines below
          // drow = -1.0 + 2.0*((static_cast<double>(row)+0.5) /
          //                    static_cast<double>(this->Dims[1]));
          for ( vtkIdType i=0; i < this->Dims[0]; ++dptr, ++mptr, ++i )
          {
            if ( *mptr > (-1) ) //if not masked
            {
              pptr = this->Pts + *mptr * 3;
              result[0] = -1.0 + 2.0*static_cast<double>(i) /
                static_cast<double>(this->Dims[0]-1);
              // if pixel origin is pixel center use the two lines below
              // result[0] = -1.0 + 2.0*((static_cast<double>(i)+0.5) /
              //                         static_cast<double>(this->Dims[0]));
              result[1] = drow;
              result[2] = *dptr;
              result[3] = 1.0;
              vtkMatrix4x4::MultiplyPoint(this->Matrix,result,result);
              *pptr++ = result[0] / result[3]; //x
              *pptr++ = result[1] / result[3]; //y
              *pptr   = result[2] / result[3]; //z
            }//transform this point
          }
        }
    }
  };

  // Interface to vtkSMPTools. Threading over image rows. Also perform
  // one time calculation/initialization for more efficient processing.
  template <typename TD, typename TP>
  void XFormPoints(TD *depths, vtkIdType *ptMap, TP *pts,
                   int dims[2], vtkCamera *cam)
  {
    double m[16], aspect=static_cast<double>(dims[0]) / static_cast<double>(dims[1]);
    vtkMatrix4x4 *matrix = cam->GetCompositeProjectionTransformMatrix(aspect,0,1);
    vtkMatrix4x4::Invert(*matrix->Element, m);

    MapDepthImage<TD,TP> mapDepths(depths,pts,dims,m,ptMap);
    vtkSMPTools::For(0,dims[1], mapDepths);
  }

  // Process the color scalars. It would be pretty easy to process all
  // attribute types if this was ever desired.
  struct MapScalars
  {
    vtkIdType NumColors;
    vtkDataArray *InColors;
    ArrayList Colors;
    const vtkIdType *PtMap;
    vtkDataArray *OutColors;

    MapScalars(vtkIdType num, vtkDataArray *colors, vtkIdType *ptMap) :
      NumColors(num), InColors(colors), PtMap(ptMap), OutColors(NULL)
    {
        vtkStdString outName = "DepthColors";
        this->OutColors = Colors.AddArrayPair(this->NumColors, this->InColors,
                                              outName, 0.0, false);
    }

    void  operator()(vtkIdType id, vtkIdType end)
    {
        vtkIdType outId;
        for ( ; id < end; ++id)
        {
          if ( (outId=this->PtMap[id]) > (-1) )
          {
            this->Colors.Copy(id,outId);
          }
        }
    }
  };

} //anonymous namespace


//================= Begin class proper =======================================
//----------------------------------------------------------------------------
vtkDepthImageToPointCloud::vtkDepthImageToPointCloud()
{
  this->Camera = NULL;
  this->CullNearPoints = false;
  this->CullFarPoints = true;
  this->ProduceColorScalars = true;
  this->ProduceVertexCellArray = true;
  this->OutputPointsPrecision = vtkAlgorithm::DEFAULT_PRECISION;

  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkDepthImageToPointCloud::~vtkDepthImageToPointCloud()
{
  if (this->Camera)
  {
    this->Camera->UnRegister(this);
    this->Camera = NULL;
  }
}

//----------------------------------------------------------------------------
vtkMTimeType vtkDepthImageToPointCloud::GetMTime()
{
  vtkCamera *cam = this->GetCamera();
  vtkMTimeType t1 = this->MTime.GetMTime();
  vtkMTimeType t2;

  if (!cam)
  {
    return t1;
  }

  // Check the camera
  t2 = cam->GetMTime();
  if (t2 > t1)
  {
    t1 = t2;
  }

  return t1;
}

//----------------------------------------------------------------------------
int vtkDepthImageToPointCloud::
FillInputPortInformation(int port, vtkInformation *info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  }
  else if (port == 1)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkDepthImageToPointCloud::
FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  return 1;
}


//----------------------------------------------------------------------------
int vtkDepthImageToPointCloud::
RequestInformation (vtkInformation* vtkNotUsed(request),
                    vtkInformationVector** vtkNotUsed(inputVector),
                    vtkInformationVector* vtkNotUsed(outputVector))
{
  return 1;
}

//----------------------------------------------------------------------------
int vtkDepthImageToPointCloud::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *vtkNotUsed(outputVector))
{
  int inExt[6];
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), inExt);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inExt, 6);

  // need to set the stencil update extent to the input extent
  if (this->GetNumberOfInputConnections(1) > 0)
  {
    vtkInformation *in2Info = inputVector[1]->GetInformationObject(0);
    in2Info->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inExt, 6);
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkDepthImageToPointCloud::
RequestData(vtkInformation*,
            vtkInformationVector** inputVector,
            vtkInformationVector* outputVector)
{
  // Get the input, make sure that it is valid
  int numInputs=0;
  vtkInformation* info = inputVector[0]->GetInformationObject(0);
  vtkImageData *inData = vtkImageData::SafeDownCast(
    info->Get(vtkDataObject::DATA_OBJECT()));
  if ( inData == NULL )
  {
    vtkErrorMacro("At least one input image is required");
    return 0;
  }
  ++numInputs;

  vtkInformation* info2 = inputVector[1]->GetInformationObject(0);
  vtkImageData *inData2=NULL;
  if ( info2 )
  {
    inData2 = vtkImageData::SafeDownCast(
      info2->Get(vtkDataObject::DATA_OBJECT()));
    if ( inData2 != NULL )
    {
      ++numInputs;
    }
  }

  vtkCamera *cam = this->Camera;
  if ( cam == NULL )
  {
    vtkErrorMacro("Input camera required");
    return 0;
  }

  // At this point we have at least one input, possibly two. If one input, we
  // assume we either have 1) depth values or 2) color scalars + depth values
  // (if depth values are in an array called "ZBuffer".) If two inputs, then the
  // depth values are in input0 and the color scalars are in input1.
  vtkDataArray *depths=NULL;
  vtkDataArray *colors=NULL;
  if ( numInputs == 2 )
  {
    depths = inData->GetPointData()->GetScalars();
    colors = inData2->GetPointData()->GetScalars();
  }
  else if (numInputs == 1)
  {
    if ( (depths = inData->GetPointData()->GetArray("ZBuffer")) != NULL )
    {
      colors = inData->GetPointData()->GetScalars();
    }
    else
    {
      depths = inData->GetPointData()->GetScalars();
    }
  }
  else
  {
    vtkErrorMacro("Wrong number of inputs");
    return 0;
  }

  // Extract relevant information to generate output
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkPolyData *outData = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Determine the image extents
  const int *ext = inData->GetExtent();
  int dims[2];
  dims[0] = ext[1]-ext[0]+1;
  dims[1] = ext[3]-ext[2]+1;
  vtkIdType numPts = dims[0] * dims[1];

  // Estimate the total number of output points. Note that if we are culling
  // near and.or far points, then the number of output points is not known,
  // so a point mask is created.
  vtkIdType numOutPts=0;
  vtkIdType *ptMap = new vtkIdType[numPts];
  void *depthPtr = depths->GetVoidPointer(0);
  switch (depths->GetDataType())
  {
    vtkTemplateMacro(MapPoints(numPts,(VTK_TT*)depthPtr,this->CullNearPoints,
                               this->CullFarPoints,ptMap,numOutPts));
  }

  // Manage the requested output point precision
  int pointsType = VTK_DOUBLE;
  if (this->OutputPointsPrecision == vtkAlgorithm::SINGLE_PRECISION)
  {
    pointsType = VTK_FLOAT;
  }

  // Create the points array which represents the point cloud
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->SetDataType(pointsType);
  points->SetNumberOfPoints(numOutPts);
  outData->SetPoints(points);

  // Threaded over x-edges (rows). Each depth value is transformed into a
  // world point. Below there is a double allocation based on the depth type
  // and output point type.
  if ( pointsType == VTK_FLOAT )
  {
    float *ptsPtr = static_cast<float*>(points->GetVoidPointer(0));
    switch (depths->GetDataType())
    {
      vtkTemplateMacro(XFormPoints((VTK_TT*)depthPtr, ptMap,
                                   static_cast<float*>(ptsPtr), dims, cam));
    }
  }
  else
  {
    double *ptsPtr = static_cast<double*>(points->GetVoidPointer(0));
    switch (depths->GetDataType())
    {
      vtkTemplateMacro(XFormPoints((VTK_TT*)depthPtr, ptMap,
                                   static_cast<double*>(ptsPtr), dims, cam));
    }
  }

  // Produce the output colors if requested. Another templated, threaded loop.
  if ( colors && this->ProduceColorScalars )
  {
    vtkPointData *outPD = outData->GetPointData();
    MapScalars mapScalars(numOutPts,colors,ptMap);
    vtkSMPTools::For(0,numPts, mapScalars);
    outPD->SetScalars(mapScalars.OutColors);
  }

  // Clean up
  delete [] ptMap;

  // If requested, create an output vertex array
  if ( this->ProduceVertexCellArray )
  {
    vtkSmartPointer<vtkCellArray> verts = vtkSmartPointer<vtkCellArray>::New();
    vtkIdType npts = points->GetNumberOfPoints();
    verts->InsertNextCell(npts);
    for (vtkIdType i=0; i < npts; ++i)
    {
      verts->InsertCellPoint(i);
    }
    outData->SetVerts(verts);
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkDepthImageToPointCloud::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if ( this->Camera )
  {
    os << indent << "Camera:\n";
    this->Camera->PrintSelf(os,indent.GetNextIndent());
  }
  else
  {
    os << indent << "Camera: (none)\n";
  }

  os << indent << "Cull Near Points: "
     << (this->CullNearPoints ? "On\n" : "Off\n");

  os << indent << "Cull Far Points: "
     << (this->CullFarPoints ? "On\n" : "Off\n");

  os << indent << "Produce Color Scalars: "
     << (this->ProduceColorScalars ? "On\n" : "Off\n");

  os << indent << "Produce Vertex Cell Array: "
     << (this->ProduceVertexCellArray ? "On\n" : "Off\n");

  os << indent << "OutputPointsPrecision: "
     << this->OutputPointsPrecision << "\n";

}
