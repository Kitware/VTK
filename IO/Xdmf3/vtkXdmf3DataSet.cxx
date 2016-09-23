/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXdmf3DataSet.cxx
  Language:  C++

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkXdmf3DataSet.h"
#include "vtkXdmf3ArrayKeeper.h"
#include "vtkXdmf3ArraySelection.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkExtractSelection.h"
#include "vtkMergePoints.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkOutEdgeIterator.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkStructuredGrid.h"
#include "vtkType.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVertexListIterator.h"

#include "XdmfArrayType.hpp"
#include "XdmfAttribute.hpp"
#include "XdmfAttributeCenter.hpp"
#include "XdmfAttributeType.hpp"
#include "XdmfCurvilinearGrid.hpp"
#include "XdmfDomain.hpp"
#include "XdmfGeometry.hpp"
#include "XdmfGeometryType.hpp"
#include "XdmfGraph.hpp"
#include "XdmfRectilinearGrid.hpp"
#include "XdmfRegularGrid.hpp"
#include "XdmfSet.hpp"
#include "XdmfSetType.hpp"
#include "XdmfUnstructuredGrid.hpp"
#include "XdmfTime.hpp"
#include "XdmfTopology.hpp"
#include "XdmfTopologyType.hpp"

//==============================================================================
bool vtkXdmf3DataSet_ReadIfNeeded(XdmfArray *array, bool dbg=false)
{
  if (!array->isInitialized())
  {
    if (dbg)
    {
      cerr << "READ " << array << endl;
    }
    array->read();
    return true;
  }
  return false;
}

void vtkXdmf3DataSet_ReleaseIfNeeded(XdmfArray *array, bool MyInit,
                                     bool dbg=false)
{
  if (MyInit)
  {
    if (dbg)
    {
      cerr << "RELEASE " << array << endl;
    }
    //array->release(); //reader level uses vtkXdmfArrayKeeper to aggregate now
  }
}

//==============================================================================
vtkDataArray *vtkXdmf3DataSet::XdmfToVTKArray(
  XdmfArray* xArray,
  std::string attrName, //TODO: passing in attrName here, because
                        //XdmfArray::getName() is oddly not virtual
  unsigned int preferredComponents,
  vtkXdmf3ArrayKeeper *keeper
)
{
  //TODO: verify the 32/64 choices are correct in all configurations
  shared_ptr<const XdmfArrayType> arrayType = xArray->getArrayType();
  vtkDataArray* vArray = NULL;
  int vtk_type = -1;
  if (arrayType == XdmfArrayType::Int8())
  {
    vtk_type = VTK_CHAR;
  }
  else if (arrayType == XdmfArrayType::Int16())
  {
    vtk_type = VTK_SHORT;
  }
  else if (arrayType == XdmfArrayType::Int32())
  {
    vtk_type = VTK_INT;
  }
  else if (arrayType == XdmfArrayType::Int64())
  {
    vtk_type = VTK_LONG;
  }
  //else if (arrayType == XdmfArrayType::UInt64()) UInt64 does not exist
    //{
    //vtk_type = VTK_LONG;
    //}
  else if (arrayType == XdmfArrayType::Float32())
  {
    vtk_type = VTK_FLOAT;
  }
  else if (arrayType == XdmfArrayType::Float64())
  {
    vtk_type = VTK_DOUBLE;
  }
  else if (arrayType == XdmfArrayType::UInt8())
  {
    vtk_type = VTK_UNSIGNED_CHAR;
  }
  else if (arrayType == XdmfArrayType::UInt16())
  {
    vtk_type = VTK_UNSIGNED_SHORT;
  }
  else if (arrayType == XdmfArrayType::UInt32())
  {
    vtk_type = VTK_UNSIGNED_INT;
  }
  else if (arrayType == XdmfArrayType::String())
  {
    vtk_type = VTK_STRING;
  }
  else
  {
    cerr << "Skipping unrecognized array type ["
         << arrayType->getName() << "]" << endl;
    return NULL;
  }
  vArray = vtkDataArray::CreateDataArray(vtk_type);
  if (vArray)
  {
    vArray->SetName(attrName.c_str());

    std::vector<unsigned int> dims = xArray->getDimensions();
    unsigned int ndims = static_cast<unsigned int>(dims.size());
    unsigned int ncomp = preferredComponents;
    if (preferredComponents == 0) //caller doesn't know what to expect,
    {
      ncomp = 1; //1 is a safe bet
      if (ndims>1)
      {
        //use last xdmf dim
        ncomp = dims[ndims-1];
      }
    }
    unsigned int ntuples = xArray->getSize() / ncomp;

    vArray->SetNumberOfComponents(static_cast<int>(ncomp));
    vArray->SetNumberOfTuples(ntuples);
    bool freeMe = vtkXdmf3DataSet_ReadIfNeeded(xArray);
#define DO_DEEPREAD 0
#if DO_DEEPREAD
    //deepcopy
    switch(vArray->GetDataType())
    {
      vtkTemplateMacro(
         xArray->getValues(0,
                           static_cast<VTK_TT*>(vArray->GetVoidPointer(0)),
                           ntuples*ncomp);
        );
      default:
        cerr << "UNKNOWN" << endl;
    }
#else
    //shallowcopy
    vArray->SetVoidArray(xArray->getValuesInternal(), ntuples*ncomp, 1);
    if (keeper && freeMe)
    {
      keeper->Insert(xArray);
    }
#endif

/*
    cerr
      << xArray << " " << xArray->getValuesInternal() << " "
      << vArray->GetVoidPointer(0) << " " << ntuples << " "
      << vArray << " " << vArray->GetName() << endl;
*/
    vtkXdmf3DataSet_ReleaseIfNeeded(xArray, freeMe);
  }
  return vArray;
}

//--------------------------------------------------------------------------
bool vtkXdmf3DataSet::VTKToXdmfArray(
  vtkDataArray *vArray,
  XdmfArray* xArray,
  unsigned int rank, unsigned int *dims
)
{
  std::vector<unsigned int> xdims;
  if (rank == 0)
  {
    xdims.push_back(vArray->GetNumberOfTuples());
  }
  else
  {
    for (unsigned int i = 0; i < rank; i++)
    {
      xdims.push_back(dims[i]);
    }
  }
  //add additional dimension to the xdmf array to match the vtk array's width,
  //ex coordinate arrays have xyz, so add [3]
  unsigned int ncomp =
    static_cast<unsigned int>(vArray->GetNumberOfComponents());
  if (ncomp != 1)
  {
    xdims.push_back(ncomp);
  }

  if (vArray->GetName())
  {
    xArray->setName(vArray->GetName());
  }

#define DO_DEEPWRITE 1
  //TODO: verify the 32/64 choices are correct in all configurations
  switch(vArray->GetDataType())
  {
    case VTK_VOID:
      return false;
    case VTK_BIT:
      return false;
    case VTK_CHAR:
    case VTK_SIGNED_CHAR:
      xArray->initialize(XdmfArrayType::Int8(), xdims);
#if DO_DEEPWRITE
      //deepcopy
      xArray->insert(0,
        static_cast<char *>(vArray->GetVoidPointer(0)),
        vArray->GetDataSize());
#else
      //shallowcopy
      xArray->setValuesInternal(
        static_cast<char *>(vArray->GetVoidPointer(0)),
        vArray->GetDataSize(),
        false);
#endif
      break;
    case VTK_UNSIGNED_CHAR:
      xArray->initialize(XdmfArrayType::UInt8(), xdims);
#if DO_DEEPWRITE
      xArray->insert(0,
        static_cast<unsigned char *>(vArray->GetVoidPointer(0)),
        vArray->GetDataSize());
#else
      xArray->setValuesInternal(
        static_cast<unsigned char *>(vArray->GetVoidPointer(0)),
        vArray->GetDataSize(),
        false);
#endif
      break;
    case VTK_SHORT:
      xArray->initialize(XdmfArrayType::Int16(), xdims);
#if DO_DEEPWRITE
      xArray->insert(0,
        static_cast<short *>(vArray->GetVoidPointer(0)),
        vArray->GetDataSize());
#else
      xArray->setValuesInternal(
        static_cast<short *>(vArray->GetVoidPointer(0)),
        vArray->GetDataSize(),
        false);
#endif
      break;
    case VTK_UNSIGNED_SHORT:
      xArray->initialize(XdmfArrayType::UInt16(), xdims);
#if DO_DEEPWRITE
      xArray->insert(0,
        static_cast<unsigned short *>(vArray->GetVoidPointer(0)),
        vArray->GetDataSize());
#else
      xArray->setValuesInternal(
        static_cast<unsigned short *>(vArray->GetVoidPointer(0)),
        vArray->GetDataSize(),
        false);
#endif
      break;
    case VTK_INT:
      xArray->initialize(XdmfArrayType::Int32(), xdims);
#if DO_DEEPWRITE
      xArray->insert(0,
        static_cast<int *>(vArray->GetVoidPointer(0)),
        vArray->GetDataSize());
#else
      xArray->setValuesInternal(
        static_cast<int *>(vArray->GetVoidPointer(0)),
        vArray->GetDataSize(),
        false);
#endif
      break;
    case VTK_UNSIGNED_INT:
      xArray->initialize(XdmfArrayType::UInt32(), xdims);
#if DO_DEEPWRITE
      xArray->insert(0,
        static_cast<unsigned int *>(vArray->GetVoidPointer(0)),
        vArray->GetDataSize());
#else
      xArray->setValuesInternal(
        static_cast<unsigned int *>(vArray->GetVoidPointer(0)),
        vArray->GetDataSize(),
        false);
#endif
      break;
    case VTK_LONG:
      xArray->initialize(XdmfArrayType::Int64(), xdims);
#if DO_DEEPWRITE
      xArray->insert(0,
        static_cast<long *>(vArray->GetVoidPointer(0)),
        vArray->GetDataSize());
#else
      xArray->setValuesInternal(
        static_cast<long *>(vArray->GetVoidPointer(0)),
        vArray->GetDataSize(),
        false);
#endif
      break;
    case VTK_UNSIGNED_LONG:
      //  arrayType = XdmfArrayType::UInt64(); UInt64 does not exist
      return false;
    case VTK_FLOAT:
      xArray->initialize(XdmfArrayType::Float32(), xdims);
#if DO_DEEPWRITE
      xArray->insert(0,
        static_cast<float *>(vArray->GetVoidPointer(0)),
        vArray->GetDataSize());
#else
      xArray->setValuesInternal(
        static_cast<float *>(vArray->GetVoidPointer(0)),
        vArray->GetDataSize(),
        false);
#endif
      break;
    case VTK_DOUBLE:
      xArray->initialize(XdmfArrayType::Float64(), xdims);
#if DO_DEEPWRITE
      xArray->insert(0,
        static_cast<double *>(vArray->GetVoidPointer(0)),
        vArray->GetDataSize());
#else
      xArray->setValuesInternal(
        static_cast<double *>(vArray->GetVoidPointer(0)),
        vArray->GetDataSize(),
        false);
#endif
      break;
    case VTK_ID_TYPE:
      if (VTK_SIZEOF_ID_TYPE == XdmfArrayType::Int64()->getElementSize())
      {
        xArray->initialize(XdmfArrayType::Int64(), xdims);
#if DO_DEEPWRITE
      xArray->insert(0,
          static_cast<long *>(vArray->GetVoidPointer(0)),
        vArray->GetDataSize());
#else
        xArray->setValuesInternal(
          static_cast<long *>(vArray->GetVoidPointer(0)),
          vArray->GetDataSize(),
          false);
#endif
      }
      else
      {
        xArray->initialize(XdmfArrayType::Int32(), xdims);
#if DO_DEEPWRITE
      xArray->insert(0,
          static_cast<int *>(vArray->GetVoidPointer(0)),
        vArray->GetDataSize());
#else
        xArray->setValuesInternal(
          static_cast<int *>(vArray->GetVoidPointer(0)),
          vArray->GetDataSize(),
          false);
#endif
      }
      break;
    case VTK_STRING:
      return false;
      //TODO: what is correct syntax here?
      //xArray->initialize(XdmfArrayType::String(), xdims);
      //xArray->setValuesInternal(
      //  static_cast<std::string>(vArray->GetVoidPointer(0)),
      //  vArray->GetDataSize(),
      //  false);
      //break;
    case VTK_OPAQUE:
    case VTK_LONG_LONG:
    case VTK_UNSIGNED_LONG_LONG:
    case VTK___INT64:
    case VTK_UNSIGNED___INT64:
    case VTK_VARIANT:
    case VTK_OBJECT:
    case VTK_UNICODE_STRING:
      return false;
    default:
      cerr << "Unrecognized vtk_type";
      return false;
  }

  return true;
}

//--------------------------------------------------------------------------
void vtkXdmf3DataSet::XdmfToVTKAttributes(
  vtkXdmf3ArraySelection *fselection,
  vtkXdmf3ArraySelection *cselection,
  vtkXdmf3ArraySelection *pselection,
  XdmfGrid *grid, vtkDataObject *dObject,
  vtkXdmf3ArrayKeeper *keeper)
{
  vtkDataSet *dataSet = vtkDataSet::SafeDownCast(dObject);
  if (!dataSet)
  {
    return;
  }
  unsigned int numCells = dataSet->GetNumberOfCells();
  unsigned int numPoints = dataSet->GetNumberOfPoints();
  unsigned int numAttributes = grid->getNumberAttributes();
  for (unsigned int cc=0; cc < numAttributes; cc++)
  {
    shared_ptr<XdmfAttribute> xmfAttribute = grid->getAttribute(cc);
    std::string attrName = xmfAttribute->getName();
    if (attrName.length() == 0)
    {
      cerr << "Skipping unnamed array." << endl;
      continue;
    }

    //figure out how many components in this array
    std::vector<unsigned int> dims = xmfAttribute->getDimensions();
    unsigned int ndims = static_cast<unsigned int>(dims.size());
    unsigned int nvals = 1;
    for (unsigned int i = 0; i < dims.size(); i++)
    {
      nvals = nvals * dims[i];
    }

    unsigned int ncomp = 1;

    vtkFieldData * fieldData = 0;

    shared_ptr<const XdmfAttributeCenter> attrCenter = xmfAttribute->getCenter();
    if (attrCenter == XdmfAttributeCenter::Grid())
    {
      if (!fselection->ArrayIsEnabled(attrName.c_str()))
      {
        continue;
      }
      fieldData = dataSet->GetFieldData();
      ncomp = dims[ndims-1];
    }
    else if (attrCenter == XdmfAttributeCenter::Cell())
    {
      if (!cselection->ArrayIsEnabled(attrName.c_str()))
      {
        continue;
      }
      if (numCells == 0)
      {
        continue;
      }
      fieldData = dataSet->GetCellData();
      ncomp = nvals/numCells;
    }
    else if (attrCenter == XdmfAttributeCenter::Node())
    {
      if (!pselection->ArrayIsEnabled(attrName.c_str()))
      {
        continue;
      }
      if (numPoints == 0)
      {
        continue;
      }
      fieldData = dataSet->GetPointData();
      ncomp = nvals/numPoints;
    }
    else
    {
      cerr << "skipping " << attrName << " unrecognized association" << endl;
      continue; // unhandled.
    }
    vtkDataSetAttributes *fdAsDSA = vtkDataSetAttributes::SafeDownCast(
      fieldData);

    shared_ptr<const XdmfAttributeType> attrType = xmfAttribute->getType();
    enum vAttType {NONE, SCALAR, VECTOR, TENSOR, MATRIX, TENSOR6, GLOBALID};
    int atype = NONE;
    if (attrType == XdmfAttributeType::Scalar() && ncomp==1)
    {
      atype = SCALAR;
    }
    else if (attrType == XdmfAttributeType::Vector() && ncomp==1)
    {
      atype = VECTOR;
    }
    else if (attrType == XdmfAttributeType::Tensor() && ncomp==9)
    {
      atype = TENSOR;
    }
    else if (attrType == XdmfAttributeType::Matrix())
    {
      atype = MATRIX;
    }
    else if (attrType == XdmfAttributeType::Tensor6())
    {
      atype = TENSOR6;
    }
    else if (attrType == XdmfAttributeType::GlobalId() && ncomp==1)
    {
      atype = GLOBALID;
    }

    vtkDataArray *array = XdmfToVTKArray(xmfAttribute.get(), attrName,
                                         ncomp, keeper);
    if (array)
    {
      fieldData->AddArray(array);
      if (fdAsDSA)
      {
        switch (atype)
        {
          case SCALAR:
            if (!fdAsDSA->GetScalars())
            {
              fdAsDSA->SetScalars(array);
            }
            break;
          case VECTOR:
            if (!fdAsDSA->GetVectors())
            {
              fdAsDSA->SetVectors(array);
            }
            break;
          case TENSOR:
            if (!fdAsDSA->GetTensors())
            {
              fdAsDSA->SetTensors(array);
            }
            break;
          case GLOBALID:
            if (!fdAsDSA->GetGlobalIds())
            {
              fdAsDSA->SetGlobalIds(array);
            }
            break;
        }
      }
      array->Delete();
    }
  }
}

//--------------------------------------------------------------------------
void vtkXdmf3DataSet::VTKToXdmfAttributes(
  vtkDataObject *dObject, XdmfGrid *grid)
{
  vtkDataSet *dataSet = vtkDataSet::SafeDownCast(dObject);
  if (!dataSet)
  {
    return;
  }

  unsigned int FDims[1];
  FDims[0] = dataSet->GetFieldData()->GetNumberOfTuples();
  unsigned int CRank = 3;
  unsigned int CDims[3];
  unsigned int PRank = 3;
  unsigned int PDims[3];
  unsigned int Dims[3];
  int wExtent[6];
  wExtent[0] = 0;
  wExtent[1] = -1;
  vtkImageData *dsAsID = vtkImageData::SafeDownCast(dataSet);
  if (dsAsID)
  {
    dsAsID->GetExtent(wExtent);
  }
  else
  {
    vtkRectilinearGrid *dsAsRG = vtkRectilinearGrid::SafeDownCast(dataSet);
    if (dsAsRG)
    {
      dsAsRG->GetExtent(wExtent);
    }
    else
    {
      vtkStructuredGrid *dsAsSG = vtkStructuredGrid::SafeDownCast(dataSet);
      if (dsAsSG)
      {
        dsAsSG->GetExtent(wExtent);
      }
    }
  }
  if (wExtent[1] > wExtent[0])
  {
    Dims[2] = static_cast<unsigned int>(wExtent[1] - wExtent[0] + 1);
    Dims[1] = static_cast<unsigned int>(wExtent[3] - wExtent[2] + 1);
    Dims[0] = static_cast<unsigned int>(wExtent[5] - wExtent[4] + 1);
    PDims[0] = Dims[0];
    PDims[1] = Dims[1];
    PDims[2] = Dims[2];
    CDims[0] = Dims[0] - 1;
    CDims[1] = Dims[1] - 1;
    CDims[2] = Dims[2] - 1;
  }
  else
  {
    PRank = 1;
    PDims[0] = dataSet->GetNumberOfPoints();
    CRank = 1;
    CDims[0] = dataSet->GetNumberOfCells();
  }

  shared_ptr<const XdmfAttributeCenter> center;
  vtkFieldData *fieldData;
  for (int fa = 0; fa < 3; fa++)
  {
    switch (fa)
    {
    case 0:
      fieldData = dataSet->GetFieldData();
      center = XdmfAttributeCenter::Grid();
      break;
    case 1:
      fieldData = dataSet->GetPointData();
      center = XdmfAttributeCenter::Node();
      break;
    default:
      fieldData = dataSet->GetCellData();
      center = XdmfAttributeCenter::Cell();
    }

    vtkDataSetAttributes *fdAsDSA = vtkDataSetAttributes::SafeDownCast(
      fieldData);
    int numArrays = fieldData->GetNumberOfArrays();
    for (int cc=0; cc < numArrays; cc++)
    {
      vtkDataArray *vArray = fieldData->GetArray(cc);
      if (!vArray)
      {
        // We're skipping non-numerical arrays for now because
        // we do not support their serialization in the heavy data file.
        continue;
      }
      std::string attrName = vArray->GetName();
      if (attrName.length() == 0)
      {
        cerr << "Skipping unnamed array." << endl;
        continue;
      }
      shared_ptr<XdmfAttribute> xmfAttribute = XdmfAttribute::New();
      xmfAttribute->setName(attrName);
      xmfAttribute->setCenter(center);
      //TODO: Also use ncomponents to tell xdmf about the other vectors etc
      if (fdAsDSA)
      {
        if (vArray == fdAsDSA->GetScalars())
        {
          xmfAttribute->setType(XdmfAttributeType::Scalar());
        }
        else if (vArray == fdAsDSA->GetVectors())
        {
          xmfAttribute->setType(XdmfAttributeType::Vector());
        }
        else if (vArray == fdAsDSA->GetTensors())
        {
          xmfAttribute->setType(XdmfAttributeType::Tensor());
        }
        else if (vArray == fdAsDSA->GetGlobalIds())
        {
          xmfAttribute->setType(XdmfAttributeType::GlobalId());
        }
      }

      unsigned int rank = 1;
      unsigned int *dims = FDims;
      if (fa == 1)
      {
        rank = PRank;
        dims = PDims;
      }
      else if (fa == 2)
      {
        rank = CRank;
        dims = CDims;
      }
      bool OK = VTKToXdmfArray(vArray, xmfAttribute.get(), rank, dims);
      if (OK)
      {
        grid->insert(xmfAttribute);
      }
    }
  }
}

//----------------------------------------------------------------------------
unsigned int vtkXdmf3DataSet::GetNumberOfPointsPerCell(int vtk_cell_type,
                                                       bool &fail)
{
  fail = false;
  switch (vtk_cell_type)
  {
  case VTK_POLY_VERTEX:
    return 0;
  case VTK_POLY_LINE:
    return 0;
  case VTK_POLYGON:
    return 0;
  case VTK_TRIANGLE:
    return 3;
  case VTK_QUAD:
    return 4;
  case VTK_TETRA:
    return 4;
  case VTK_PYRAMID:
    return 5;
  case VTK_WEDGE:
    return 6;
  case VTK_HEXAHEDRON:
    return 8;
  case VTK_QUADRATIC_EDGE:
    return 3;
  case VTK_QUADRATIC_TRIANGLE:
    return 6;
  case VTK_QUADRATIC_QUAD:
    return 8;
  case VTK_BIQUADRATIC_QUAD:
    return 9;
  case VTK_QUADRATIC_TETRA:
    return 10;
  case VTK_QUADRATIC_PYRAMID:
    return 13;
  case VTK_QUADRATIC_WEDGE:
    return 15;
  case VTK_BIQUADRATIC_QUADRATIC_WEDGE:
    return 18;
  case VTK_QUADRATIC_HEXAHEDRON:
    return 20;
  case VTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON:
    return 24;
  case VTK_TRIQUADRATIC_HEXAHEDRON:
    return 24;
  }
  fail = true;
  return 0;
}

//----------------------------------------------------------------------------
int vtkXdmf3DataSet::GetXdmfCellType(int vtkType)
{
  switch(vtkType)
  {
    case VTK_EMPTY_CELL :
      return 0x0; //XdmfTopologyType::NoTopologyType()
    case VTK_VERTEX :
    case VTK_POLY_VERTEX :
      return 0x1; // XdmfTopologyType::Polyvertex()
    case VTK_LINE :
    case VTK_POLY_LINE :
      return 0x2; //XdmfTopologyType::Polyline()
    case VTK_TRIANGLE :
    case VTK_TRIANGLE_STRIP :
      return 0x4; //XdmfTopologyType::Triangle()
    case VTK_POLYGON :
      return 0x3;// XdmfTopologyType::Polygon()
    case VTK_PIXEL :
    case VTK_QUAD :
      return 0x5; //XdmfTopologyType::Quadrilateral()
    case VTK_TETRA :
      return 0x6; //XdmfTopologyType::Tetrahedron()
    case VTK_VOXEL :
    case VTK_HEXAHEDRON :
      return 0x9; //XdmfTopologyType::Hexahedron()
    case VTK_WEDGE :
      return 0x8; //XdmfTopologyType::Wedge()
    case VTK_PYRAMID :
      return 0x7; //XdmfTopologyType::Pyramid()
    case VTK_POLYHEDRON :
      return 0x10; //XdmfTopologyType::Polyhedron()
    case VTK_PENTAGONAL_PRISM :
    case VTK_HEXAGONAL_PRISM :
    case VTK_QUADRATIC_EDGE :
    case VTK_QUADRATIC_TRIANGLE :
    case VTK_QUADRATIC_QUAD :
    case VTK_QUADRATIC_TETRA :
    case VTK_QUADRATIC_HEXAHEDRON :
    case VTK_QUADRATIC_WEDGE :
    case VTK_QUADRATIC_PYRAMID :
    case VTK_BIQUADRATIC_QUAD :
    case VTK_TRIQUADRATIC_HEXAHEDRON :
    case VTK_QUADRATIC_LINEAR_QUAD :
    case VTK_QUADRATIC_LINEAR_WEDGE :
    case VTK_BIQUADRATIC_QUADRATIC_WEDGE :
    case VTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON :
    case VTK_BIQUADRATIC_TRIANGLE :
    case VTK_CUBIC_LINE :
    case VTK_CONVEX_POINT_SET :
    case VTK_PARAMETRIC_CURVE :
    case VTK_PARAMETRIC_SURFACE :
    case VTK_PARAMETRIC_TRI_SURFACE :
    case VTK_PARAMETRIC_QUAD_SURFACE :
    case VTK_PARAMETRIC_TETRA_REGION :
    case VTK_PARAMETRIC_HEX_REGION :
    case VTK_HIGHER_ORDER_EDGE :
    case VTK_HIGHER_ORDER_TRIANGLE :
    case VTK_HIGHER_ORDER_QUAD :
    case VTK_HIGHER_ORDER_POLYGON :
    case VTK_HIGHER_ORDER_TETRAHEDRON :
    case VTK_HIGHER_ORDER_WEDGE :
    case VTK_HIGHER_ORDER_PYRAMID :
    case VTK_HIGHER_ORDER_HEXAHEDRON :
      cerr << "I do not know how to make that xdmf cell type" << endl;
      //TODO: see list below
      return -1;
    default :
      cerr << "Unknown vtk cell type" << endl;
      return -1;
  }

  /*
  0x22; //XdmfTopologyType::Edge_3() ));
  0x24; //XdmfTopologyType::Triangle_6() ));
  0x25; //XdmfTopologyType::Quadrilateral_8() 0x25));
  0x23; //XdmfTopologyType::Quadrilateral_9() 0x23));
  0x25; //XdmfTopologyType::Tetrahedron_10() 0x26));
  0x27; //XdmfTopologyType::Pyramid_13() 0x27));
  0x28; //XdmfTopologyType::Wedge_15() ));
  0x29; //XdmfTopologyType::Wedge_18() 0x29));
  0x30; //XdmfTopologyType::Hexahedron_20() 0x30));
  0x31; //XdmfTopologyType::Hexahedron_24() 0x31));
  0x32; //XdmfTopologyType::Hexahedron_27() 0x32));
  0x33; //XdmfTopologyType::Hexahedron_64() 0x33));
  0x34; //XdmfTopologyType::Hexahedron_125() 0x34));
  0x35; //XdmfTopologyType::Hexahedron_216()
  0x36; //XdmfTopologyType::Hexahedron_343()
  0x37; //XdmfTopologyType::Hexahedron_512()
  0x38; //XdmfTopologyType::Hexahedron_729()
  0x39; //XdmfTopologyType::Hexahedron_1000()
  0x40; //XdmfTopologyType::Hexahedron_1331()
  0x41; //XdmfTopologyType::Hexahedron_Spectral_64()
  0x42; //XdmfTopologyType::Hexahedron_Spectral_125()
  0x43; //XdmfTopologyType::Hexahedron_Spectral_216()
  0x44; //XdmfTopologyType::Hexahedron_Spectral_343()
  0x45; //XdmfTopologyType::Hexahedron_Spectral_512()
  0x46; //XdmfTopologyType::Hexahedron_Spectral_729()
  0x47; //XdmfTopologyType::Hexahedron_Spectral_1000()
  0x48; //XdmfTopologyType::Hexahedron_Spectral_1331()
  0x70; //XdmfTopologyType::Mixed()
  */
}

//----------------------------------------------------------------------------
int vtkXdmf3DataSet::GetVTKCellType(
  shared_ptr<const XdmfTopologyType> topologyType)
{
  //TODO: examples to test/demonstrate each of these
  if (topologyType == XdmfTopologyType::Polyvertex())
  {
    return VTK_POLY_VERTEX;
  }
  if (topologyType->getName() == XdmfTopologyType::Polyline(0)->getName())
  {
    return VTK_POLY_LINE;
  }
  if (topologyType->getName() == XdmfTopologyType::Polygon(0)->getName())
  {
    return VTK_POLYGON; // FIXME: should this not be treated as mixed?
  }
  if (topologyType == XdmfTopologyType::Triangle())
  {
    return VTK_TRIANGLE;
  }
  if (topologyType == XdmfTopologyType::Quadrilateral())
  {
    return VTK_QUAD;
  }
  if (topologyType == XdmfTopologyType::Tetrahedron())
  {
    return VTK_TETRA;
  }
  if (topologyType == XdmfTopologyType::Pyramid())
  {
    return VTK_PYRAMID;
  }
  if (topologyType == XdmfTopologyType::Wedge())
  {
    return VTK_WEDGE;
  }
  if (topologyType == XdmfTopologyType::Hexahedron())
  {
    return VTK_HEXAHEDRON;
  }
  if (topologyType == XdmfTopologyType::Edge_3())
  {
    return VTK_QUADRATIC_EDGE;
  }
  if (topologyType == XdmfTopologyType::Triangle_6())
  {
    return VTK_QUADRATIC_TRIANGLE;
  }
  if (topologyType == XdmfTopologyType::Quadrilateral_8())
  {
    return VTK_QUADRATIC_QUAD;
  }
  if (topologyType == XdmfTopologyType::Quadrilateral_9())
  {
    return VTK_BIQUADRATIC_QUAD;
  }
  if (topologyType == XdmfTopologyType::Tetrahedron_10())
  {
    return VTK_QUADRATIC_TETRA;
  }
  if (topologyType == XdmfTopologyType::Pyramid_13())
  {
    return VTK_QUADRATIC_PYRAMID;
  }
  if (topologyType == XdmfTopologyType::Wedge_15())
  {
    return VTK_QUADRATIC_WEDGE;
  }
  if (topologyType == XdmfTopologyType::Wedge_18())
  {
    return VTK_BIQUADRATIC_QUADRATIC_WEDGE;
  }
  if (topologyType == XdmfTopologyType::Hexahedron_20())
  {
    return VTK_QUADRATIC_HEXAHEDRON;
  }
  if (topologyType == XdmfTopologyType::Hexahedron_24())
  {
    return VTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON;
  }
  if (topologyType == XdmfTopologyType::Hexahedron_27())
  {
    return VTK_TRIQUADRATIC_HEXAHEDRON;
  }
  if (topologyType == XdmfTopologyType::Polyhedron())
  {
    return VTK_POLYHEDRON;
  }
  /* TODO: Translate these new XDMF cell types
  static shared_ptr<const XdmfTopologyType> Hexahedron_64();
  static shared_ptr<const XdmfTopologyType> Hexahedron_125();
  static shared_ptr<const XdmfTopologyType> Hexahedron_216();
  static shared_ptr<const XdmfTopologyType> Hexahedron_343();
  static shared_ptr<const XdmfTopologyType> Hexahedron_512();
  static shared_ptr<const XdmfTopologyType> Hexahedron_729();
  static shared_ptr<const XdmfTopologyType> Hexahedron_1000();
  static shared_ptr<const XdmfTopologyType> Hexahedron_1331();
  static shared_ptr<const XdmfTopologyType> Hexahedron_Spectral_64();
  static shared_ptr<const XdmfTopologyType> Hexahedron_Spectral_125();
  static shared_ptr<const XdmfTopologyType> Hexahedron_Spectral_216();
  static shared_ptr<const XdmfTopologyType> Hexahedron_Spectral_343();
  static shared_ptr<const XdmfTopologyType> Hexahedron_Spectral_512();
  static shared_ptr<const XdmfTopologyType> Hexahedron_Spectral_729();
  static shared_ptr<const XdmfTopologyType> Hexahedron_Spectral_1000();
  static shared_ptr<const XdmfTopologyType> Hexahedron_Spectral_1331();
  */
  if (topologyType == XdmfTopologyType::Mixed())
  {
    return VTK_NUMBER_OF_CELL_TYPES;
  }

  return VTK_EMPTY_CELL;
}

//==========================================================================
void vtkXdmf3DataSet::SetTime(XdmfGrid *grid, double hasTime, double time)
{
  if (hasTime)
  {
    grid->setTime(XdmfTime::New(time));
  }
}
//==========================================================================
void vtkXdmf3DataSet::SetTime(XdmfGraph *graph, double hasTime, double time)
{
  if (hasTime)
  {
    graph->setTime(XdmfTime::New(time));
  }
}

//==========================================================================

void vtkXdmf3DataSet::XdmfToVTK(
  vtkXdmf3ArraySelection *fselection,
  vtkXdmf3ArraySelection *cselection,
  vtkXdmf3ArraySelection *pselection,
  XdmfRegularGrid *grid,
  vtkImageData *dataSet,
  vtkXdmf3ArrayKeeper *keeper)
{
  vtkXdmf3DataSet::CopyShape(grid, dataSet, keeper);
  vtkXdmf3DataSet::XdmfToVTKAttributes(fselection, cselection, pselection,
                                       grid, dataSet, keeper);
}

//--------------------------------------------------------------------------
void vtkXdmf3DataSet::CopyShape(
  XdmfRegularGrid *grid,
  vtkImageData *dataSet,
  vtkXdmf3ArrayKeeper *vtkNotUsed(keeper))
{
  if (!dataSet)
  {
    return;
  }

  int whole_extent[6];
  whole_extent[0] = 0;
  whole_extent[1] = -1;
  whole_extent[2] = 0;
  whole_extent[3] = -1;
  whole_extent[4] = 0;
  whole_extent[5] = -1;

  shared_ptr<XdmfArray> xdims = grid->getDimensions();
  if (xdims)
  {
    bool freeMe = vtkXdmf3DataSet_ReadIfNeeded(xdims.get());
    for (unsigned int i = 0; (i < 3 && i < xdims->getSize()); i++)
    {
      whole_extent[(2-i)*2+1] = xdims->getValue<int>(i)-1;
    }
  if (xdims->getSize() == 2)
  {
    whole_extent[1] = whole_extent[0];
  }
    vtkXdmf3DataSet_ReleaseIfNeeded(xdims.get(), freeMe);
  }
  dataSet->SetExtent(whole_extent);

  double origin[3];
  origin[0] = 0.0;
  origin[1] = 0.0;
  origin[2] = 0.0;
  shared_ptr<XdmfArray> xorigin = grid->getOrigin();
  if (xorigin)
  {
    bool freeMe = vtkXdmf3DataSet_ReadIfNeeded(xorigin.get());
    for (unsigned int i = 0; (i< 3 && i < xorigin->getSize()); i++)
    {
      origin[2-i] = xorigin->getValue<double>(i);
    }
    vtkXdmf3DataSet_ReleaseIfNeeded(xorigin.get(), freeMe);
  }
  dataSet->SetOrigin(origin);

  double spacing[3];
  spacing[0] = 1.0;
  spacing[1] = 1.0;
  spacing[2] = 1.0;
  shared_ptr<XdmfArray> xspacing;
  xspacing = grid->getBrickSize();
  if (xspacing)
  {
    bool freeMe = vtkXdmf3DataSet_ReadIfNeeded(xspacing.get());
    for (unsigned int i = 0; (i < 3 && i < xspacing->getSize()); i++)
    {
      spacing[2-i] = xspacing->getValue<double>(i);
    }
    vtkXdmf3DataSet_ReleaseIfNeeded(xspacing.get(), freeMe);
  }

  dataSet->SetSpacing(spacing);
}

//--------------------------------------------------------------------------
void vtkXdmf3DataSet::VTKToXdmf(
  vtkImageData *dataSet,
  XdmfDomain *domain,
  bool hasTime, double time,
  const char* name)
{
  int whole_extent[6];
  dataSet->GetExtent(whole_extent);
  double origin[3];
  dataSet->GetOrigin(origin);
  double spacing[3];
  dataSet->GetSpacing(spacing);
  unsigned int dims[3];
  dims[0] = static_cast<unsigned int>(whole_extent[1] - whole_extent[0] + 1);
  dims[1] = static_cast<unsigned int>(whole_extent[3] - whole_extent[2] + 1);
  dims[2] = static_cast<unsigned int>(whole_extent[5] - whole_extent[4] + 1);
  shared_ptr<XdmfRegularGrid> grid = XdmfRegularGrid::New(
    spacing[2], spacing[1], spacing[0],
    dims[2], dims[1], dims[0],
    origin[2], origin[1], origin[0]);
  if (name)
  {
    grid->setName(std::string(name));
  }

  vtkXdmf3DataSet::VTKToXdmfAttributes(dataSet, grid.get());
  vtkXdmf3DataSet::SetTime(grid.get(), hasTime, time);

  domain->insert(grid);
}


//==========================================================================

void vtkXdmf3DataSet::XdmfToVTK(
  vtkXdmf3ArraySelection *fselection,
  vtkXdmf3ArraySelection *cselection,
  vtkXdmf3ArraySelection *pselection,
  XdmfRectilinearGrid *grid,
  vtkRectilinearGrid *dataSet,
  vtkXdmf3ArrayKeeper *keeper)
{
  vtkXdmf3DataSet::CopyShape(grid, dataSet, keeper);
  vtkXdmf3DataSet::XdmfToVTKAttributes(fselection, cselection, pselection,
                                       grid, dataSet, keeper);
}

//--------------------------------------------------------------------------
void vtkXdmf3DataSet::CopyShape(
  XdmfRectilinearGrid *grid,
  vtkRectilinearGrid *dataSet,
  vtkXdmf3ArrayKeeper *keeper)
{
  if (!dataSet)
  {
    return;
  }

  int whole_extent[6];
  whole_extent[0] = 0;
  whole_extent[1] = -1;
  whole_extent[2] = 0;
  whole_extent[3] = -1;
  whole_extent[4] = 0;
  whole_extent[5] = -1;

  shared_ptr<XdmfArray> xdims;
  xdims = grid->getDimensions();
  //Note: XDMF standard for RECTMESH is inconsistent with SMESH and CORECTMESH
  //it is ijk in VTK terms and they are kji.
  if (xdims)
  {
    bool freeMe = vtkXdmf3DataSet_ReadIfNeeded(xdims.get());
    for (unsigned int i = 0; (i < 3 && i < xdims->getSize()); i++)
    {
      whole_extent[i*2+1] = xdims->getValue<int>(i)-1;
    }
  if (xdims->getSize() == 2)
  {
    whole_extent[5] = whole_extent[4];
  }
    vtkXdmf3DataSet_ReleaseIfNeeded(xdims.get(), freeMe);
  }
  dataSet->SetExtent(whole_extent);

  vtkDataArray *vCoords = NULL;
  shared_ptr<XdmfArray> xCoords;

  xCoords = grid->getCoordinates(0);
  vCoords = vtkXdmf3DataSet::XdmfToVTKArray
    (xCoords.get(), xCoords->getName(), 1, keeper);
  dataSet->SetXCoordinates(vCoords);
  if (vCoords)
  {
    vCoords->Delete();
  }

  xCoords = grid->getCoordinates(1);
  vCoords = vtkXdmf3DataSet::XdmfToVTKArray
    (xCoords.get(), xCoords->getName(), 1, keeper);
  dataSet->SetYCoordinates(vCoords);
  if (vCoords)
  {
    vCoords->Delete();
  }

  if (xdims->getSize() > 2)
  {
    xCoords = grid->getCoordinates(2);
    vCoords = vtkXdmf3DataSet::XdmfToVTKArray
      (xCoords.get(), xCoords->getName(), 1, keeper);
    dataSet->SetZCoordinates(vCoords);
    if (vCoords)
    {
      vCoords->Delete();
    }
  }
}

//--------------------------------------------------------------------------
void vtkXdmf3DataSet::VTKToXdmf(
  vtkRectilinearGrid *dataSet,
  XdmfDomain *domain,
  bool hasTime, double time,
  const char* name)
{
  vtkDataArray *vCoords = NULL;
  shared_ptr<XdmfArray> xXCoords = XdmfArray::New();
  shared_ptr<XdmfArray> xYCoords = XdmfArray::New();
  shared_ptr<XdmfArray> xZCoords = XdmfArray::New();

  bool OK = true;
  vCoords = dataSet->GetXCoordinates();
  OK &= vtkXdmf3DataSet::VTKToXdmfArray(vCoords, xZCoords.get());
  if (OK)
  {
    vCoords = dataSet->GetYCoordinates();
    OK &= vtkXdmf3DataSet::VTKToXdmfArray(vCoords, xYCoords.get());
    if (OK)
    {
      vCoords = dataSet->GetZCoordinates();
      OK &= vtkXdmf3DataSet::VTKToXdmfArray(vCoords, xXCoords.get());
    }
  }

  if (!OK)
  {
    return;
  }

  shared_ptr<XdmfRectilinearGrid> grid = XdmfRectilinearGrid::New(
    xXCoords, xYCoords, xZCoords);

  if (name)
  {
    grid->setName(std::string(name));
  }

  vtkXdmf3DataSet::VTKToXdmfAttributes(dataSet, grid.get());
  vtkXdmf3DataSet::SetTime(grid.get(), hasTime, time);

  domain->insert(grid);
}

//==========================================================================

void vtkXdmf3DataSet::XdmfToVTK(
  vtkXdmf3ArraySelection *fselection,
  vtkXdmf3ArraySelection *cselection,
  vtkXdmf3ArraySelection *pselection,
  XdmfCurvilinearGrid *grid,
  vtkStructuredGrid *dataSet,
  vtkXdmf3ArrayKeeper *keeper)
{
  vtkXdmf3DataSet::CopyShape(grid, dataSet, keeper);
  vtkXdmf3DataSet::XdmfToVTKAttributes(fselection, cselection, pselection,
                                       grid, dataSet, keeper);
}

//--------------------------------------------------------------------------
void vtkXdmf3DataSet::CopyShape(
  XdmfCurvilinearGrid *grid,
  vtkStructuredGrid *dataSet,
  vtkXdmf3ArrayKeeper *keeper)
{
  if (!dataSet)
  {
    return;
  }

  int whole_extent[6];
  whole_extent[0] = 0;
  whole_extent[1] = -1;
  whole_extent[2] = 0;
  whole_extent[3] = -1;
  whole_extent[4] = 0;
  whole_extent[5] = -1;
  shared_ptr<XdmfArray> xdims;
  xdims = grid->getDimensions();
  if (xdims)
  {
    for (unsigned int i = 0; (i < 3 && i < xdims->getSize()); i++)
    {
      whole_extent[(2-i)*2+1] = xdims->getValue<int>(i)-1;
    }
  }
  if (xdims->getSize() == 2)
  {
    whole_extent[1] = whole_extent[0];
  }
  dataSet->SetExtent(whole_extent);

  vtkDataArray *vPoints = NULL;
  shared_ptr<XdmfGeometry> geom = grid->getGeometry();
  if (geom->getType() == XdmfGeometryType::XY())
  {
    vPoints = vtkXdmf3DataSet::XdmfToVTKArray(geom.get(), "", 2, keeper);
    vtkDataArray *vPoints3 = vPoints->NewInstance();
    vPoints3->SetNumberOfComponents(3);
    vPoints3->SetNumberOfTuples(vPoints->GetNumberOfTuples());
    vPoints3->SetName("");
    vPoints3->CopyComponent(0, vPoints, 0);
    vPoints3->CopyComponent(1, vPoints, 1);
    vPoints3->FillComponent(2, 0.0);
    vPoints->Delete();
    vPoints = vPoints3;
  }
  else if (geom->getType() == XdmfGeometryType::XYZ())
  {
    vPoints = vtkXdmf3DataSet::XdmfToVTKArray(geom.get(), "", 3, keeper);
  }
  else
  {
    //TODO: No X_Y or X_Y_Z in xdmf anymore
    return;
  }
  vtkPoints *p = vtkPoints::New();
  p->SetData(vPoints);
  dataSet->SetPoints(p);
  p->Delete();
  if (vPoints)
  {
    vPoints->Delete();
  }
}

//--------------------------------------------------------------------------
void vtkXdmf3DataSet::VTKToXdmf(
  vtkStructuredGrid *dataSet,
  XdmfDomain *domain,
  bool hasTime, double time,
  const char* name)
{
  int whole_extent[6];
  whole_extent[0] = 0;
  whole_extent[1] = -1;
  whole_extent[2] = 0;
  whole_extent[3] = -1;
  whole_extent[4] = 0;
  whole_extent[5] = -1;
  dataSet->GetExtent(whole_extent);
  shared_ptr<XdmfArray> xdims = XdmfArray::New();
  xdims->initialize(XdmfArrayType::Int32());
  if (xdims)
  {
    for (unsigned int i = 0; i < 3; i++)
    {
      int extent = whole_extent[(2-i)*2+1]-whole_extent[(2-i)*2]+1;
      xdims->pushBack<int>(extent);
    }
  }

  vtkDataArray *vCoords = dataSet->GetPoints()->GetData();
  shared_ptr<XdmfGeometry> xCoords = XdmfGeometry::New();
  bool OK = vtkXdmf3DataSet::VTKToXdmfArray(vCoords, xCoords.get());
  if (!OK)
  {
    return;
  }
  xCoords->setType(XdmfGeometryType::XYZ());

  shared_ptr<XdmfCurvilinearGrid> grid = XdmfCurvilinearGrid::New(xdims);
  grid->setGeometry(xCoords);

  if (name)
  {
    grid->setName(std::string(name));
  }

  vtkXdmf3DataSet::VTKToXdmfAttributes(dataSet, grid.get());
  vtkXdmf3DataSet::SetTime(grid.get(), hasTime, time);

  domain->insert(grid);
}

//==========================================================================

void vtkXdmf3DataSet::XdmfToVTK(
  vtkXdmf3ArraySelection *fselection,
  vtkXdmf3ArraySelection *cselection,
  vtkXdmf3ArraySelection *pselection,
  XdmfUnstructuredGrid *grid,
  vtkUnstructuredGrid *dataSet,
  vtkXdmf3ArrayKeeper *keeper)
{
  vtkXdmf3DataSet::CopyShape(grid, dataSet, keeper);
  vtkXdmf3DataSet::XdmfToVTKAttributes(fselection, cselection, pselection,
                                       grid, dataSet, keeper);
}

//--------------------------------------------------------------------------
void vtkXdmf3DataSet::CopyShape(
  XdmfUnstructuredGrid *grid,
  vtkUnstructuredGrid *dataSet,
  vtkXdmf3ArrayKeeper *keeper)
{
  if (!dataSet)
  {
    return;
  }

  shared_ptr<XdmfTopology> xTopology = grid->getTopology();
  shared_ptr<const XdmfTopologyType> xCellType = xTopology->getType();
  int vCellType = vtkXdmf3DataSet::GetVTKCellType(xCellType);
  if (vCellType == VTK_EMPTY_CELL)
  {
    return;
  }

  bool freeMe = vtkXdmf3DataSet_ReadIfNeeded(xTopology.get());

  if (xCellType != XdmfTopologyType::Mixed())
  {
    // all cells are of the same type.
    unsigned int numPointsPerCell= xCellType->getNodesPerElement();

    // translate cell array
    unsigned int numCells = xTopology->getNumberElements();

    int *cell_types = new int[numCells];

    vtkCellArray* vCells = vtkCellArray::New();
    // Get the pointer
    vtkIdType* cells_ptr = vCells->WritePointer(
      numCells, numCells * (1 + numPointsPerCell));

    // xmfConnections: N p1 p2 ... pN
    // i.e. Triangles : 3 0 1 2    3 3 4 5   3 6 7 8
    vtkIdType index = 0;
    for(vtkIdType cc = 0 ; cc < static_cast<vtkIdType>(numCells); cc++ )
    {
      cell_types[cc] = vCellType;
      *cells_ptr++ = numPointsPerCell;
      for (vtkIdType i = 0 ; i < static_cast<vtkIdType>(numPointsPerCell); i++ )
      {
        *cells_ptr++ = xTopology->getValue<vtkIdType>(index++);
      }
    }
    dataSet->SetCells(cell_types, vCells);
    vCells->Delete();
    vtkXdmf3DataSet_ReleaseIfNeeded(xTopology.get(), freeMe);
    delete [] cell_types;
  }
  else
  {
    // mixed cell types
    unsigned int conn_length = xTopology->getSize();
    vtkIdType numCells = xTopology->getNumberElements();

    int *cell_types = new int[numCells];

    /* Create Cell Array */
    vtkCellArray* vCells = vtkCellArray::New();

    /* Get the pointer. Make it Big enough ... too big for now */
    vtkIdType* cells_ptr = vCells->WritePointer(numCells, conn_length);

    /* xmfConnections : N p1 p2 ... pN */
    /* i.e. Triangles : 3 0 1 2    3 3 4 5   3 6 7 8 */
    vtkIdType index = 0;
    int sub = 0;
    for(vtkIdType cc = 0 ; cc < numCells; cc++ )
    {
      shared_ptr<const XdmfTopologyType> nextCellType =
        XdmfTopologyType::New(xTopology->getValue<vtkIdType>(index++));
      int vtk_cell_typeI = vtkXdmf3DataSet::GetVTKCellType(nextCellType);

      if (vtk_cell_typeI != VTK_POLYHEDRON)
      {
        bool unknownCell;
        unsigned int numPointsPerCell =
          vtkXdmf3DataSet::GetNumberOfPointsPerCell(vtk_cell_typeI, unknownCell);

        if (unknownCell)
        {
          // encountered an unknown cell.
          cerr << "Unkown cell type." << endl;
          vCells->Delete();
          delete [] cell_types;
          vtkXdmf3DataSet_ReleaseIfNeeded(xTopology.get(), freeMe);
          return;
        }

        if (numPointsPerCell==0)
        {
          // cell type does not have a fixed number of points in which case the
          // next entry in xmfConnections tells us the number of points.
          numPointsPerCell = xTopology->getValue<unsigned int>(index++);
          sub++; // used to shrink the cells array at the end.
        }

        cell_types[cc] = vtk_cell_typeI;
        *cells_ptr++ = numPointsPerCell;
        for(vtkIdType i = 0 ; i < static_cast<vtkIdType>(numPointsPerCell); i++ )
        {
          *cells_ptr++ = xTopology->getValue<vtkIdType>(index++);
        }
      }
      else
      {
        // polyhedrons do not have a fixed number of faces in which case the
        // next entry in xmfConnections tells us the number of faces.
        const unsigned int numFacesPerCell =
          xTopology->getValue<unsigned int>(index++);

        // polyhedrons do not have a fixed number of points in which case the
        // the number of points needs to be obtained from the data.
        unsigned int numPointsPerCell = 0;
        for(vtkIdType i = 0 ; i < static_cast<vtkIdType>(numFacesPerCell); i++ )
        {
          // faces do not have a fixed number of points in which case the next
          // entry in xmfConnections tells us the number of points.
          numPointsPerCell +=
            xTopology->getValue<unsigned int>(index + numPointsPerCell + i);
        }

        // add cell entry to the array, which for polyhedrons is in the format:
        // [cellLength, nCellFaces, nFace0Pts, id0_0, id0_1, ...,
        //                          nFace1Pts, id1_0, id1_1, ...,
        //                          ...]
        cell_types[cc] = vtk_cell_typeI;
        *cells_ptr++ = numPointsPerCell + numFacesPerCell + 1;
        sub++; // used to shrink the cells array at the end.
        *cells_ptr++ = numFacesPerCell;
        for(vtkIdType i = 0 ;
            i < static_cast<vtkIdType>(numPointsPerCell + numFacesPerCell); i++ )
        {
          *cells_ptr++ = xTopology->getValue<vtkIdType>(index++);
        }
      }
    }
    // Resize the Array to the Proper Size
    vCells->GetData()->Resize(index-sub);
    dataSet->SetCells(cell_types, vCells);
    vCells->Delete();
    delete [] cell_types;
    vtkXdmf3DataSet_ReleaseIfNeeded(xTopology.get(), freeMe);
  }

  //copy geometry
  vtkDataArray *vPoints = NULL;
  shared_ptr<XdmfGeometry> geom = grid->getGeometry();
  if (geom->getType() == XdmfGeometryType::XY())
  {
    vPoints = vtkXdmf3DataSet::XdmfToVTKArray(geom.get(), "", 2, keeper);
    vtkDataArray *vPoints3 = vPoints->NewInstance();
    vPoints3->SetNumberOfComponents(3);
    vPoints3->SetNumberOfTuples(vPoints->GetNumberOfTuples());
    vPoints3->SetName("");
    vPoints3->CopyComponent(0, vPoints, 0);
    vPoints3->CopyComponent(1, vPoints, 1);
    vPoints3->FillComponent(2, 0.0);
    vPoints->Delete();
    vPoints = vPoints3;
  }
  else if (geom->getType() == XdmfGeometryType::XYZ())
  {
    vPoints = vtkXdmf3DataSet::XdmfToVTKArray(geom.get(), "", 3, keeper);
  }
  else
  {
    //TODO: No X_Y or X_Y_Z in xdmf anymore
    return;
  }

  vtkPoints *p = vtkPoints::New();
  p->SetData(vPoints);
  dataSet->SetPoints(p);
  p->Delete();
  if (vPoints)
  {
    vPoints->Delete();
  }
}

//--------------------------------------------------------------------------
void vtkXdmf3DataSet::VTKToXdmf(
  vtkPointSet *dataSet,
  XdmfDomain *domain,
  bool hasTime, double time,
  const char* name)
{
  vtkDataArray *vCoords = dataSet->GetPoints()->GetData();
  shared_ptr<XdmfGeometry> xCoords = XdmfGeometry::New();
  bool OK = vtkXdmf3DataSet::VTKToXdmfArray(vCoords, xCoords.get());
  if (!OK)
  {
    return;
  }
  xCoords->setType(XdmfGeometryType::XYZ());

  shared_ptr<XdmfUnstructuredGrid> grid = XdmfUnstructuredGrid::New();
  if (name)
  {
    grid->setName(std::string(name));
  }
  grid->setGeometry(xCoords);

  shared_ptr<XdmfTopology> xTopology = XdmfTopology::New();
  grid->setTopology(xTopology);


  //TODO: homogeneous case in old reader _might_ be faster
  //for simplicity I am treating all dataSets as having mixed cell types
  xTopology->setType(XdmfTopologyType::Mixed());
  vtkIdType numCells = dataSet->GetNumberOfCells();

  //reserve some space
  /*4 = celltype+numids+id0+id1 or celltype+id0+id1+id2*/
  const int PER_CELL_ESTIMATE=4;
  unsigned int total_estimate = numCells*PER_CELL_ESTIMATE;
  if (VTK_SIZEOF_ID_TYPE == XdmfArrayType::Int64()->getElementSize())
  {
    xTopology->initialize(XdmfArrayType::Int64(), total_estimate);
  }
  else
  {
    xTopology->initialize(XdmfArrayType::Int32(), total_estimate);
  }

  unsigned int tcount = 0;
  vtkIdType cntr = 0;
  for (vtkIdType cid=0 ; cid < numCells; cid++)
  {
    vtkCell *cell = dataSet->GetCell(cid);
    vtkIdType cellType = dataSet->GetCellType(cid);
    vtkIdType numPts = cell->GetNumberOfPoints();
    int xType = vtkXdmf3DataSet::GetXdmfCellType(cellType);
    if (xType != -1)
    {
      xTopology->insert(cntr++, xType);
    }
    tcount +=1;
    switch(cellType)
    {
      case VTK_VERTEX :
      case VTK_POLY_VERTEX :
      case VTK_LINE :
      case VTK_POLY_LINE :
      case VTK_POLYGON :
        xTopology->insert(cntr++,static_cast<long>(numPts));
        tcount +=1;
        break;
      default:
        break;
    }
    if ( cellType == VTK_VOXEL )
    {
      // Reinterpret to xdmf's order
      xTopology->insert(cntr++, (int)cell->GetPointId(0));
      xTopology->insert(cntr++, (int)cell->GetPointId(1));
      xTopology->insert(cntr++, (int)cell->GetPointId(3));
      xTopology->insert(cntr++, (int)cell->GetPointId(2));
      xTopology->insert(cntr++, (int)cell->GetPointId(4));
      xTopology->insert(cntr++, (int)cell->GetPointId(5));
      xTopology->insert(cntr++, (int)cell->GetPointId(7));
      xTopology->insert(cntr++, (int)cell->GetPointId(6));
      tcount +=8;
    }
    else if ( cellType == VTK_PIXEL )
    {
      // Reinterpret to xdmf's order
      xTopology->insert(cntr++, (int)cell->GetPointId(0));
      xTopology->insert(cntr++, (int)cell->GetPointId(1));
      xTopology->insert(cntr++, (int)cell->GetPointId(3));
      xTopology->insert(cntr++, (int)cell->GetPointId(2));
      tcount +=4;
    }
    else if ( cellType == VTK_POLYHEDRON )
    {
      // Convert polyhedron to format:
      // [nCellFaces, nFace0Pts, i, j, k, nFace1Pts, i, j, k, ...]
      const vtkIdType numFaces = cell->GetNumberOfFaces();
      xTopology->insert(cntr++,static_cast<long>(numFaces));
      tcount +=1;

      vtkIdType fid, pid;
      vtkCell *face;
      for (fid=0; fid < numFaces; fid++)
      {
        face = cell->GetFace(fid);
        numPts = face->GetNumberOfPoints();
        xTopology->insert(cntr++,static_cast<long>(numPts));
        tcount +=1;
        for (pid=0; pid < numPts; pid++)
        {
          xTopology->insert(cntr++, (int)face->GetPointId(pid));
        }
        tcount +=numPts;
      }
    }
    else
    {
      for (vtkIdType pid=0; pid < numPts; pid++)
      {
        xTopology->insert(cntr++, (int)cell->GetPointId(pid));
      }
      tcount +=numPts;
    }
  }
  xTopology->resize(tcount,0); //release unused reserved space

  vtkXdmf3DataSet::VTKToXdmfAttributes(dataSet, grid.get());
  vtkXdmf3DataSet::SetTime(grid.get(), hasTime, time);

  domain->insert(grid);
}

//==========================================================================

void vtkXdmf3DataSet::XdmfToVTK(
  vtkXdmf3ArraySelection *fselection,
  vtkXdmf3ArraySelection *cselection,
  vtkXdmf3ArraySelection *pselection,
  XdmfGraph *grid,
  vtkMutableDirectedGraph *dataSet,
  vtkXdmf3ArrayKeeper *keeper)
{
  if (!dataSet)
  {
    return;
  }

  unsigned int numNodes = grid->getNumberNodes();
  shared_ptr<XdmfArray> mRowPointer = grid->getRowPointer();
  shared_ptr<XdmfArray> mColumnIndex = grid->getColumnIndex();
  shared_ptr<XdmfArray> mValues = grid->getValues();
  bool freeRow = vtkXdmf3DataSet_ReadIfNeeded(mRowPointer.get());
  bool freeColumn = vtkXdmf3DataSet_ReadIfNeeded(mColumnIndex.get());
  bool freeValues = vtkXdmf3DataSet_ReadIfNeeded(mValues.get());
  //unpack the compressed row storage format graph into nodes and edges

  vtkSmartPointer<vtkDoubleArray> wA =
    vtkSmartPointer<vtkDoubleArray>::New();
  wA->SetName("Edge Weights");
  wA->SetNumberOfComponents(1);

  //Nodes
  for(unsigned int i=0; i<numNodes; ++i)
  {
    dataSet->AddVertex();
  }

  //Edges
  unsigned int index = 0;
  for(unsigned int i=0; i<numNodes; ++i)
  {
    for(unsigned int j=mRowPointer->getValue<unsigned int>(i);
        j<mRowPointer->getValue<unsigned int>(i+1);
        ++j)
    {
      const unsigned int k = mColumnIndex->getValue<unsigned int>(j);
      dataSet->AddEdge(i,k);

      double value = mValues->getValue<double>(index++);
      wA->InsertNextValue(value);
    }
  }

  vtkXdmf3DataSet_ReleaseIfNeeded(mRowPointer.get(), freeRow);
  vtkXdmf3DataSet_ReleaseIfNeeded(mColumnIndex.get(), freeColumn);
  vtkXdmf3DataSet_ReleaseIfNeeded(mValues.get(), freeValues);

  //Copy over arrays
  vtkDataSetAttributes *edgeData = dataSet->GetEdgeData();
  edgeData->AddArray(wA);

  //Next the optional arrays
  unsigned int numAttributes = grid->getNumberAttributes();
  for (unsigned int cc=0; cc < numAttributes; cc++)
  {
    shared_ptr<XdmfAttribute> xmfAttribute = grid->getAttribute(cc);
    std::string attrName = xmfAttribute->getName();
    if (attrName.length() == 0)
    {
      cerr << "Skipping unnamed array." << endl;
      continue;
    }

    vtkFieldData * fieldData = 0;
    shared_ptr<const XdmfAttributeCenter> attrCenter =
      xmfAttribute->getCenter();
    if (attrCenter == XdmfAttributeCenter::Grid())
    {
      if (!fselection->ArrayIsEnabled(attrName.c_str()))
      {
        continue;
      }
      fieldData = dataSet->GetFieldData();
    }
    else if (attrCenter == XdmfAttributeCenter::Edge())
    {
      if (!cselection->ArrayIsEnabled(attrName.c_str()))
      {
        continue;
      }
      fieldData = dataSet->GetEdgeData();
    }
    else if (attrCenter == XdmfAttributeCenter::Node())
    {
      if (!pselection->ArrayIsEnabled(attrName.c_str()))
      {
        continue;
      }
      fieldData = dataSet->GetVertexData();
    }
    else
    {
      cerr << "Skipping " << attrName << " unrecognized association"
           << endl;
      continue; // unhandled.
    }

    vtkDataArray *array =
      vtkXdmf3DataSet::XdmfToVTKArray(xmfAttribute.get(), attrName,
                                      0, keeper);
    if (array)
    {
      fieldData->AddArray(array);
      array->Delete();
    }
  }
}

//--------------------------------------------------------------------------
void vtkXdmf3DataSet::VTKToXdmf(
  vtkDirectedGraph *dataSet,
  XdmfDomain *domain,
  bool hasTime, double time,
  const char* name)
{
  //get list of vertices
  vtkSmartPointer<vtkVertexListIterator> vit =
    vtkSmartPointer<vtkVertexListIterator>::New();
  dataSet->GetVertices(vit);

  vtkIdType numNodes = dataSet->GetNumberOfVertices();
  shared_ptr<XdmfArray> mRowPointer = XdmfArray::New();
  shared_ptr<XdmfArray> mColumnIndex = XdmfArray::New();
  shared_ptr<XdmfArray> mValues = XdmfArray::New();
  mValues->initialize(XdmfArrayType::Float32());
  if (VTK_SIZEOF_ID_TYPE == XdmfArrayType::Int64()->getElementSize())
  {
    mRowPointer->initialize(XdmfArrayType::Int64());
    mColumnIndex->initialize(XdmfArrayType::Int64());
  }
  else
  {
    mRowPointer->initialize(XdmfArrayType::Int32());
    mColumnIndex->initialize(XdmfArrayType::Int32());
  }

  vtkDoubleArray *wA = vtkArrayDownCast<vtkDoubleArray>(
    dataSet->GetEdgeData()->GetArray("Edge Weights"));

  while (vit->HasNext())
  {
      vtkIdType sVertex = vit->Next();

      mRowPointer->pushBack(mColumnIndex->getSize());

      vtkSmartPointer<vtkOutEdgeIterator> eit =
          vtkSmartPointer<vtkOutEdgeIterator>::New();
      dataSet->GetOutEdges(sVertex, eit);

      while (eit->HasNext())
      {
        vtkOutEdgeType e = eit->Next();
        vtkIdType dVertex = e.Target;
        mColumnIndex->pushBack((int)dVertex);
        double eW = 1.0;
        if (wA)
        {
          eW = wA->GetValue(e.Id);
        }
        mValues->pushBack(eW);
      }
  }
  mRowPointer->pushBack(mValues->getSize());

  shared_ptr<XdmfGraph> grid = XdmfGraph::New(numNodes);
  grid->setValues(mValues);
  grid->setColumnIndex(mColumnIndex);
  grid->setRowPointer(mRowPointer);
  if (name)
  {
    grid->setName(std::string(name));
  }

  vtkFieldData *fd;
  shared_ptr<const XdmfAttributeCenter> center;
  for (int i = 0; i < 3; i++)
  {
    switch (i)
    {
    case 0:
      fd = dataSet->GetFieldData();
      center = XdmfAttributeCenter::Grid();
      break;
    case 1:
      fd = dataSet->GetVertexData();
      center = XdmfAttributeCenter::Node();
      break;
    default:
      fd = dataSet->GetEdgeData();
      center = XdmfAttributeCenter::Edge();
    }

    for (vtkIdType j = 0; j < fd->GetNumberOfArrays(); j++)
    {
      vtkDataArray *vArray = fd->GetArray(j);
      if (vArray == wA)
      {
        continue;
      }
      shared_ptr<XdmfAttribute> xmfAttribute = XdmfAttribute::New();
      if (!vArray->GetName())
      {
        continue;
      }
      xmfAttribute->setName(vArray->GetName());
      xmfAttribute->setCenter(center);
      bool OK = vtkXdmf3DataSet::VTKToXdmfArray(vArray, xmfAttribute.get());
      if (OK)
      {
        grid->insert(xmfAttribute);
      }
    }
  }

  vtkXdmf3DataSet::SetTime(grid.get(), hasTime, time);

  domain->insert(grid);
}


//==========================================================================
//TODO: meld this with Grid XdmfToVTKAttributes
//TODO: enable set atribute selections
void vtkXdmf3DataSet::XdmfToVTKAttributes(
/*
  vtkXdmf3ArraySelection *fselection,
  vtkXdmf3ArraySelection *cselection,
  vtkXdmf3ArraySelection *pselection,
*/
  XdmfSet *grid, vtkDataObject *dObject,
  vtkXdmf3ArrayKeeper *keeper)
{
  vtkDataSet *dataSet = vtkDataSet::SafeDownCast(dObject);
  if (!dataSet)
  {
    return;
  }
  unsigned int numCells = dataSet->GetNumberOfCells();
  unsigned int numPoints = dataSet->GetNumberOfPoints();
  unsigned int numAttributes = grid->getNumberAttributes();
  for (unsigned int cc=0; cc < numAttributes; cc++)
  {
    shared_ptr<XdmfAttribute> xmfAttribute = grid->getAttribute(cc);
    std::string attrName = xmfAttribute->getName();
    if (attrName.length() == 0)
    {
      cerr << "Skipping unnamed array." << endl;
      continue;
    }

    //figure out how many components in this array
    std::vector<unsigned int> dims = xmfAttribute->getDimensions();
    unsigned int ndims = static_cast<unsigned int>(dims.size());
    unsigned int nvals = 1;
    for (unsigned int i = 0; i < dims.size(); i++)
    {
      nvals = nvals * dims[i];
    }

    unsigned int ncomp = 1;

    vtkFieldData * fieldData = 0;

    shared_ptr<const XdmfAttributeCenter> attrCenter =
      xmfAttribute->getCenter();
    if (attrCenter == XdmfAttributeCenter::Grid())
    {
      /*
      if (!fselection->ArrayIsEnabled(attrName.c_str()))
        {
        continue;
        }
      */
      fieldData = dataSet->GetFieldData();
      ncomp = dims[ndims-1];
    }
    else if (attrCenter == XdmfAttributeCenter::Cell())
    {
      /*
      if (!cselection->ArrayIsEnabled(attrName.c_str()))
        {
        continue;
        }
      */
      if (numCells == 0)
      {
        continue;
      }
      fieldData = dataSet->GetCellData();
      ncomp = nvals/numCells;
    }
    else if (attrCenter == XdmfAttributeCenter::Node())
    {
      /*
      if (!pselection->ArrayIsEnabled(attrName.c_str()))
        {
        continue;
        }
      */
      if (numPoints == 0)
      {
        continue;
      }
      fieldData = dataSet->GetPointData();
      ncomp = nvals/numPoints;
    }
    else
    {
      cerr << "skipping " << attrName << " unrecognized association" << endl;
      continue; // unhandled.
    }
    vtkDataSetAttributes *fdAsDSA = vtkDataSetAttributes::SafeDownCast(
      fieldData);

    shared_ptr<const XdmfAttributeType> attrType = xmfAttribute->getType();
    enum vAttType {NONE, SCALAR, VECTOR, TENSOR, MATRIX, TENSOR6, GLOBALID};
    int atype = NONE;
    if (attrType == XdmfAttributeType::Scalar() && ncomp==1)
    {
      atype = SCALAR;
    }
    else if (attrType == XdmfAttributeType::Vector() && ncomp==1)
    {
      atype = VECTOR;
    }
    else if (attrType == XdmfAttributeType::Tensor() && ncomp==9)
    {
      atype = TENSOR;
    }
    else if (attrType == XdmfAttributeType::Matrix())
    {
      atype = MATRIX;
    }
    else if (attrType == XdmfAttributeType::Tensor6())
    {
      atype = TENSOR6;
    }
    else if (attrType == XdmfAttributeType::GlobalId() && ncomp==1)
    {
      atype = GLOBALID;
    }

    vtkDataArray *array = XdmfToVTKArray
      (xmfAttribute.get(), attrName, ncomp, keeper);
    if (array)
    {
      fieldData->AddArray(array);
      if (fdAsDSA)
      {
        switch (atype)
        {
          case SCALAR:
            if (!fdAsDSA->GetScalars())
            {
              fdAsDSA->SetScalars(array);
            }
            break;
          case VECTOR:
            if (!fdAsDSA->GetVectors())
            {
              fdAsDSA->SetVectors(array);
            }
            break;
          case TENSOR:
            if (!fdAsDSA->GetTensors())
            {
              fdAsDSA->SetTensors(array);
            }
            break;
          case GLOBALID:
            if (!fdAsDSA->GetGlobalIds())
            {
              fdAsDSA->SetGlobalIds(array);
            }
            break;
        }
      }
      array->Delete();
    }
  }
}

//--------------------------------------------------------------------------
void vtkXdmf3DataSet::XdmfSubsetToVTK(
  XdmfGrid *grid,
  unsigned int setnum,
  vtkDataSet *dataSet,
  vtkUnstructuredGrid *subSet,
  vtkXdmf3ArrayKeeper *keeper)
{
  shared_ptr<XdmfSet> set = grid->getSet(setnum);
  bool releaseMe = vtkXdmf3DataSet_ReadIfNeeded(set.get());
  /*
  if (set->getType() == XdmfSetType::NoSetType())
    {
    }
  */
  if (set->getType() == XdmfSetType::Node())
  {
    vtkDataArray *ids =
      vtkXdmf3DataSet::XdmfToVTKArray(set.get(), set->getName(), 1, keeper);

    vtkSmartPointer<vtkSelectionNode> selectionNode =
      vtkSmartPointer<vtkSelectionNode>::New();
    selectionNode->SetFieldType(vtkSelectionNode::POINT);
    selectionNode->SetContentType(vtkSelectionNode::INDICES);
    selectionNode->SetSelectionList(ids);

    vtkSmartPointer<vtkSelection> selection =
      vtkSmartPointer<vtkSelection>::New();
    selection->AddNode(selectionNode);

    vtkSmartPointer<vtkExtractSelection> extractSelection =
      vtkSmartPointer<vtkExtractSelection>::New();
    extractSelection->SetInputData(0, dataSet);
    extractSelection->SetInputData(1, selection);
    extractSelection->Update();

    //remove arrays from grid, only care about subsets own arrays
    vtkUnstructuredGrid *dso =
      vtkUnstructuredGrid::SafeDownCast(extractSelection->GetOutput());
    dso->GetPointData()->Initialize();
    dso->GetCellData()->Initialize();
    dso->GetFieldData()->Initialize();
    subSet->ShallowCopy(dso);

    vtkXdmf3DataSet::XdmfToVTKAttributes(set.get(), subSet, keeper);
    ids->Delete();
  }

  if (set->getType() == XdmfSetType::Cell())
  {
    vtkDataArray *ids =
      vtkXdmf3DataSet::XdmfToVTKArray(set.get(), set->getName(), 1, keeper);

    vtkSmartPointer<vtkSelectionNode> selectionNode =
      vtkSmartPointer<vtkSelectionNode>::New();
    selectionNode->SetFieldType(vtkSelectionNode::CELL);
    selectionNode->SetContentType(vtkSelectionNode::INDICES);
    selectionNode->SetSelectionList(ids);

    vtkSmartPointer<vtkSelection> selection =
      vtkSmartPointer<vtkSelection>::New();
    selection->AddNode(selectionNode);

    vtkSmartPointer<vtkExtractSelection> extractSelection =
      vtkSmartPointer<vtkExtractSelection>::New();
    extractSelection->SetInputData(0, dataSet);
    extractSelection->SetInputData(1, selection);
    extractSelection->Update();

    //remove arrays from grid, only care about subsets own arrays
    vtkUnstructuredGrid *dso =
      vtkUnstructuredGrid::SafeDownCast(extractSelection->GetOutput());
    dso->GetPointData()->Initialize();
    dso->GetCellData()->Initialize();
    dso->GetFieldData()->Initialize();
    subSet->ShallowCopy(dso);

    vtkXdmf3DataSet::XdmfToVTKAttributes(set.get(), subSet, keeper);
    ids->Delete();
  }

  if (set->getType() == XdmfSetType::Face())
  {
    vtkPoints *pts = vtkPoints::New();
    subSet->SetPoints(pts);
    vtkSmartPointer<vtkMergePoints> mergePts =
      vtkSmartPointer<vtkMergePoints>::New();
    mergePts->InitPointInsertion(pts, dataSet->GetBounds());

    vtkDataArray *ids =
      vtkXdmf3DataSet::XdmfToVTKArray(set.get(), set->getName(), 2, keeper);
    // ids is a 2 component array were each tuple is (cell-id, face-id).

    vtkIdType numFaces = ids->GetNumberOfTuples();
    for (vtkIdType cc=0; cc < numFaces; cc++)
    {
      vtkIdType cellId = ids->GetComponent(cc,0);
      vtkIdType faceId = ids->GetComponent(cc,1);
      vtkCell* cell = dataSet->GetCell(cellId);
      if (!cell)
      {
        continue;
      }
      vtkCell* face = cell->GetFace(faceId);
      if (!face)
      {
        continue;
      }

      // Now insert this face a new cell in the output dataset.
      vtkIdType numPoints = face->GetNumberOfPoints();
      vtkPoints* facePoints = face->GetPoints();
      vtkIdType* outputPts = new vtkIdType[numPoints+1];
      vtkIdType* cPt = outputPts;

      double ptCoord[3];
      for (vtkIdType pt = 0; pt < facePoints->GetNumberOfPoints(); pt++)
      {
        facePoints->GetPoint(pt, ptCoord);
        mergePts->InsertUniquePoint(ptCoord, cPt[pt]);
      }
      subSet->InsertNextCell(face->GetCellType(), numPoints, outputPts);
      delete [] outputPts;
    }

    vtkXdmf3DataSet::XdmfToVTKAttributes(set.get(), subSet, keeper);

    ids->Delete();
    pts->Delete();
  }

  if (set->getType() == XdmfSetType::Edge())
  {
    vtkPoints *pts = vtkPoints::New();
    subSet->SetPoints(pts);
    vtkSmartPointer<vtkMergePoints> mergePts =
      vtkSmartPointer<vtkMergePoints>::New();
    mergePts->InitPointInsertion(pts, dataSet->GetBounds());

    vtkDataArray *ids =
      vtkXdmf3DataSet::XdmfToVTKArray(set.get(), set->getName(), 3, keeper);
    // ids is a 3 component array were each tuple is (cell-id, face-id, edge-id).

    vtkIdType numEdges = ids->GetNumberOfTuples();
    for (vtkIdType cc=0; cc < numEdges; cc++)
    {
      vtkIdType cellId = ids->GetComponent(cc,0);
      vtkIdType faceId = ids->GetComponent(cc,1);
      vtkIdType edgeId = ids->GetComponent(cc,2);
      vtkCell* cell = dataSet->GetCell(cellId);
      if (!cell)
      {
        continue;
      }
      vtkCell* face = cell->GetFace(faceId);
      if (!face)
      {
        continue;
      }
      vtkCell* edge = face->GetEdge(edgeId);
      if (!edge)
      {
        continue;
      }

      // Now insert this edge a new cell in the output dataset.
      vtkIdType numPoints = edge->GetNumberOfPoints();
      vtkPoints* edgePoints = edge->GetPoints();
      vtkIdType* outputPts = new vtkIdType[numPoints+1];
      vtkIdType* cPt = outputPts;

      double ptCoord[3];
      for (vtkIdType pt = 0; pt < edgePoints->GetNumberOfPoints(); pt++)
      {
        edgePoints->GetPoint(pt, ptCoord);
        mergePts->InsertUniquePoint(ptCoord, cPt[pt]);
      }
      subSet->InsertNextCell(edge->GetCellType(), numPoints, outputPts);
      delete [] outputPts;
    }

    vtkXdmf3DataSet::XdmfToVTKAttributes(set.get(), subSet, keeper);

    ids->Delete();
    pts->Delete();
  }

  vtkXdmf3DataSet_ReleaseIfNeeded(set.get(), releaseMe);
  return;
}
