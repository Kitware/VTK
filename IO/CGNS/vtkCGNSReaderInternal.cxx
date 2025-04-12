// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2013-2014 Mickael Philit
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCGNSReaderInternal.h"

#include "cgio_helpers.h"
#include "vtkCellType.h"
#include "vtkIdTypeArray.h"
#include "vtkMultiProcessStream.h"

#include <algorithm>

namespace CGNSRead
{
VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
int setUpRind(int cgioNum, double rindId, int* rind)
{
  CGNSRead::char_33 dataType;
  if (cgio_get_data_type(cgioNum, rindId, dataType) != CG_OK)
  {
    std::cerr << "Problem while reading Rind data type\n";
    return 1;
  }

  if (strcmp(dataType, "I4") == 0)
  {
    std::vector<vtkTypeInt32> mdata;
    CGNSRead::readNodeData<vtkTypeInt32>(cgioNum, rindId, mdata);
    for (std::size_t index = 0; index < mdata.size(); index++)
    {
      rind[index] = static_cast<int>(mdata[index]);
    }
  }
  else if (strcmp(dataType, "I8") == 0)
  {
    std::vector<vtkTypeInt64> mdata;
    CGNSRead::readNodeData<vtkTypeInt64>(cgioNum, rindId, mdata);
    for (std::size_t index = 0; index < mdata.size(); index++)
    {
      rind[index] = static_cast<int>(mdata[index]);
    }
  }
  return 0;
}

//------------------------------------------------------------------------------
int getFirstNodeId(int cgioNum, double parentId, const char* label, double* id, const char* name)
{
  int nId, n, nChildren, len;
  int ier = 0;
  char nodeLabel[CGIO_MAX_NAME_LENGTH + 1];
  char nodeName[CGIO_MAX_NAME_LENGTH + 1];

  if (cgio_number_children(cgioNum, parentId, &nChildren) != CG_OK)
  {
    return 1;
  }
  if (nChildren < 1)
  {
    return 1;
  }

  double* idList = new double[nChildren];
  cgio_children_ids(cgioNum, parentId, 1, nChildren, &len, idList);
  if (len != nChildren)
  {
    delete[] idList;
    std::cerr << "Mismatch in number of children and child IDs read" << std::endl;
    return 1;
  }

  nId = 0;
  for (n = 0; n < nChildren; n++)
  {

    if (cgio_get_label(cgioNum, idList[n], nodeLabel))
    {
      return 1;
    }

    if (name != nullptr && cgio_get_name(cgioNum, idList[n], nodeName))
    {
      return 1;
    }

    if (0 == strcmp(nodeLabel, label) && (name == nullptr || 0 == strcmp(nodeName, name)))
    {
      *id = idList[n];
      nId = 1;
    }
    else
    {
      cgio_release_id(cgioNum, idList[n]);
    }
    if (nId != 0)
    {
      break;
    }
  }
  n++;
  while (n < nChildren)
  {
    cgio_release_id(cgioNum, idList[n]);
    n++;
  }

  if (nId < 1)
  {
    *id = 0.0;
    ier = 1;
  }

  delete[] idList;
  return ier;
}

//------------------------------------------------------------------------------
int get_section_connectivity(int cgioNum, double cgioSectionId, int dim, const cgsize_t* srcStart,
  const cgsize_t* srcEnd, const cgsize_t* srcStride, const cgsize_t* memStart,
  const cgsize_t* memEnd, const cgsize_t* memStride, const cgsize_t* memDim,
  vtkIdType* localElements)
{
  const char* connectivityPath = "ElementConnectivity";
  double cgioElemConnectId;
  char dataType[3];
  std::size_t sizeOfCnt = 0;

  cgio_get_node_id(cgioNum, cgioSectionId, connectivityPath, &cgioElemConnectId);
  cgio_get_data_type(cgioNum, cgioElemConnectId, dataType);

  if (strcmp(dataType, "I4") == 0)
  {
    sizeOfCnt = sizeof(int);
  }
  else if (strcmp(dataType, "I8") == 0)
  {
    sizeOfCnt = sizeof(cglong_t);
  }
  else
  {
    std::cerr << "ElementConnectivity data_type unknown\n";
  }

  if (sizeOfCnt == sizeof(vtkIdType))
  {
    if (cgio_read_data_type(cgioNum, cgioElemConnectId, srcStart, srcEnd, srcStride, dataType, dim,
          memDim, memStart, memEnd, memStride, (void*)localElements) != CG_OK)
    {
      char message[81];
      cgio_error_message(message);
      std::cerr << "cgio_read_data_type :" << message;
      return 1;
    }
  }
  else
  {
    // Need to read into temp array to convert data
    cgsize_t nn = 1;
    for (int ii = 0; ii < dim; ii++)
    {
      nn *= memDim[ii];
    }
    if (sizeOfCnt == sizeof(int))
    {
      int* data = new int[nn];
      if (data == nullptr)
      {
        std::cerr << "Allocation failed for temporary connectivity array\n";
        return 1;
      }

      if (cgio_read_data_type(cgioNum, cgioElemConnectId, srcStart, srcEnd, srcStride, "I4", dim,
            memDim, memStart, memEnd, memStride, (void*)data) != CG_OK)
      {
        delete[] data;
        char message[81];
        cgio_error_message(message);
        std::cerr << "cgio_read_data_type :" << message;
        return 1;
      }
      for (cgsize_t n = 0; n < nn; n++)
      {
        localElements[n] = static_cast<vtkIdType>(data[n]);
      }
      delete[] data;
    }
    else if (sizeOfCnt == sizeof(cglong_t))
    {
      cglong_t* data = new cglong_t[nn];
      if (data == nullptr)
      {
        std::cerr << "Allocation failed for temporary connectivity array\n";
        return 1;
      }
      if (cgio_read_data_type(cgioNum, cgioElemConnectId, srcStart, srcEnd, srcStride, "I8", dim,
            memDim, memStart, memEnd, memStride, (void*)data) != CG_OK)
      {
        delete[] data;
        char message[81];
        cgio_error_message(message);
        std::cerr << "cgio_read_data_type :" << message;
        return 1;
      }
      for (cgsize_t n = 0; n < nn; n++)
      {
        localElements[n] = static_cast<vtkIdType>(data[n]);
      }
      delete[] data;
    }
  }
  cgio_release_id(cgioNum, cgioElemConnectId);
  return 0;
}

//------------------------------------------------------------------------------
int get_section_start_offset(int cgioNum, double cgioSectionId, int dim, const cgsize_t* srcStart,
  const cgsize_t* srcEnd, const cgsize_t* srcStride, const cgsize_t* memStart,
  const cgsize_t* memEnd, const cgsize_t* memStride, const cgsize_t* memDim,
  vtkIdType* localElementsIdx)
{
  const char* offsetPath = "ElementStartOffset";
  double cgioElemOffsetId;
  char dataType[3];
  std::size_t sizeOfCnt = 0;

  if (cgio_get_node_id(cgioNum, cgioSectionId, offsetPath, &cgioElemOffsetId) != CG_OK)
  {
    // std::cerr << "ElementStartOffset not found\n";
    return 1;
  }

  cgio_get_data_type(cgioNum, cgioElemOffsetId, dataType);

  if (strcmp(dataType, "I4") == 0)
  {
    sizeOfCnt = sizeof(int);
  }
  else if (strcmp(dataType, "I8") == 0)
  {
    sizeOfCnt = sizeof(cglong_t);
  }
  else
  {
    std::cerr << "ElementStartOffset data_type unknown\n";
  }

  if (sizeOfCnt == sizeof(vtkIdType))
  {
    if (cgio_read_data_type(cgioNum, cgioElemOffsetId, srcStart, srcEnd, srcStride, dataType, dim,
          memDim, memStart, memEnd, memStride, (void*)localElementsIdx) != CG_OK)
    {
      char message[81];
      cgio_error_message(message);
      std::cerr << "cgio_read_data_type :" << message;
      return 1;
    }
  }
  else
  {
    // Need to read into temp array to convert data
    cgsize_t nn = 1;
    for (int ii = 0; ii < dim; ii++)
    {
      nn *= memDim[ii];
    }
    if (sizeOfCnt == sizeof(int))
    {
      int* data = new int[nn];
      if (data == nullptr)
      {
        std::cerr << "Allocation failed for temporary connectivity offset array\n";
      }

      if (cgio_read_data_type(cgioNum, cgioElemOffsetId, srcStart, srcEnd, srcStride, "I4", dim,
            memDim, memStart, memEnd, memStride, (void*)data) != CG_OK)
      {
        delete[] data;
        char message[81];
        cgio_error_message(message);
        std::cerr << "cgio_read_data_type :" << message;
        return 1;
      }
      for (cgsize_t n = 0; n < nn; n++)
      {
        localElementsIdx[n] = static_cast<vtkIdType>(data[n]);
      }
      delete[] data;
    }
    else if (sizeOfCnt == sizeof(cglong_t))
    {
      cglong_t* data = new cglong_t[nn];
      if (data == nullptr)
      {
        std::cerr << "Allocation failed for temporary connectivity array\n";
        return 1;
      }
      if (cgio_read_data_type(cgioNum, cgioElemOffsetId, srcStart, srcEnd, srcStride, "I8", dim,
            memDim, memStart, memEnd, memStride, (void*)data) != CG_OK)
      {
        delete[] data;
        char message[81];
        cgio_error_message(message);
        std::cerr << "cgio_read_data_type :" << message;
        return 1;
      }
      for (cgsize_t n = 0; n < nn; n++)
      {
        localElementsIdx[n] = static_cast<vtkIdType>(data[n]);
      }
      delete[] data;
    }
  }
  cgio_release_id(cgioNum, cgioElemOffsetId);
  return 0;
}

//------------------------------------------------------------------------------
int get_section_parent_elements(const int cgioNum, const double cgioSectionId, const int dim,
  const cgsize_t* srcStart, const cgsize_t* srcEnd, const cgsize_t* srcStride,
  const cgsize_t* memStart, const cgsize_t* memEnd, const cgsize_t* memStride,
  const cgsize_t* memDim, vtkIdType* localPE)
{
  const char* PEPath = "ParentElements";
  double cgioPEId;
  char dataType[3];
  std::size_t sizeOfCnt = 0;

  if (cgio_get_node_id(cgioNum, cgioSectionId, PEPath, &cgioPEId) != CG_OK)
  {
    return 1; // ParentElements not found
  }

  cgio_get_data_type(cgioNum, cgioPEId, dataType);

  if (strcmp(dataType, "I4") == 0)
  {
    sizeOfCnt = sizeof(int);
  }
  else if (strcmp(dataType, "I8") == 0)
  {
    sizeOfCnt = sizeof(cglong_t);
  }
  else
  {
    std::cerr << "ParentElements data_type unknown\n";
  }
  if (sizeOfCnt == sizeof(vtkIdType))
  {

    if (cgio_read_data_type(cgioNum, cgioPEId, srcStart, srcEnd, srcStride, dataType, dim, memDim,
          memStart, memEnd, memStride, (void*)localPE) != CG_OK)
    {
      char message[81];
      cgio_error_message(message);
      std::cerr << "cgio_read_data_type :" << message;
      return 1;
    }
  }
  else
  {
    // Need to read into temp array to convert data
    cgsize_t nn = 1;
    for (int ii = 0; ii < dim; ii++)
    {
      nn *= memDim[ii];
    }
    if (sizeOfCnt == sizeof(int))
    {
      int* data = new int[nn];
      if (data == nullptr)
      {
        std::cerr << "Allocation failed for temporary ParentElements array\n";
      }
      if (cgio_read_data_type(cgioNum, cgioPEId, srcStart, srcEnd, srcStride, "I4", dim, memDim,
            memStart, memEnd, memStride, (void*)data) != CG_OK)
      {
        delete[] data;
        char message[81];
        cgio_error_message(message);
        std::cerr << "cgio_read_data_type :" << message;
        return 1;
      }
      for (cgsize_t n = 0; n < nn; n++)
      {
        localPE[n] = static_cast<vtkIdType>(data[n]);
      }
      delete[] data;
    }
    else if (sizeOfCnt == sizeof(cglong_t))
    {
      cglong_t* data = new cglong_t[nn];
      if (data == nullptr)
      {
        std::cerr << "Allocation failed for temporary ParentElements array\n";
        return 1;
      }
      if (cgio_read_data_type(cgioNum, cgioPEId, srcStart, srcEnd, srcStride, "I8", dim, memDim,
            memStart, memEnd, memStride, (void*)data) != CG_OK)
      {
        delete[] data;
        char message[81];
        cgio_error_message(message);
        std::cerr << "cgio_read_data_type :" << message;
        return 1;
      }
      for (cgsize_t n = 0; n < nn; n++)
      {
        localPE[n] = static_cast<vtkIdType>(data[n]);
      }
      delete[] data;
    }
  }
  cgio_release_id(cgioNum, cgioPEId);
  return 0;
}
//------------------------------------------------------------------------------
int GetVTKElemType(
  CGNS_ENUMT(ElementType_t) elemType, bool& higherOrderWarning, bool& cgnsOrderFlag)
{
  int cellType;
  higherOrderWarning = false;
  cgnsOrderFlag = false;
  //
  switch (elemType)
  {
    case CGNS_ENUMV(NODE):
      cellType = VTK_VERTEX;
      break;
    case CGNS_ENUMV(BAR_2):
      cellType = VTK_LINE;
      break;
    case CGNS_ENUMV(BAR_3):
      cellType = VTK_QUADRATIC_EDGE;
      higherOrderWarning = true;
      break;
    case CGNS_ENUMV(BAR_4):
      cellType = VTK_CUBIC_LINE;
      higherOrderWarning = true;
      break;
    case CGNS_ENUMV(TRI_3):
      cellType = VTK_TRIANGLE;
      break;
    case CGNS_ENUMV(TRI_6):
      cellType = VTK_QUADRATIC_TRIANGLE;
      higherOrderWarning = true;
      break;
    case CGNS_ENUMV(QUAD_4):
      cellType = VTK_QUAD;
      break;
    case CGNS_ENUMV(QUAD_8):
      cellType = VTK_QUADRATIC_QUAD;
      higherOrderWarning = true;
      break;
    case CGNS_ENUMV(QUAD_9):
      cellType = VTK_BIQUADRATIC_QUAD;
      higherOrderWarning = true;
      break;
    case CGNS_ENUMV(TETRA_4):
      cellType = VTK_TETRA;
      break;
    case CGNS_ENUMV(TETRA_10):
      cellType = VTK_QUADRATIC_TETRA;
      higherOrderWarning = true;
      break;
    case CGNS_ENUMV(PYRA_5):
      cellType = VTK_PYRAMID;
      break;
    case CGNS_ENUMV(PYRA_14):
      cellType = VTK_QUADRATIC_PYRAMID;
      higherOrderWarning = true;
      break;
    case CGNS_ENUMV(PENTA_6):
      cellType = VTK_WEDGE;
      break;
    case CGNS_ENUMV(PENTA_15):
      cellType = VTK_QUADRATIC_WEDGE;
      higherOrderWarning = true;
      cgnsOrderFlag = true;
      break;
    case CGNS_ENUMV(PENTA_18):
      cellType = VTK_BIQUADRATIC_QUADRATIC_WEDGE;
      higherOrderWarning = true;
      cgnsOrderFlag = true;
      break;
    case CGNS_ENUMV(HEXA_8):
      cellType = VTK_HEXAHEDRON;
      break;
    case CGNS_ENUMV(HEXA_20):
      cellType = VTK_QUADRATIC_HEXAHEDRON;
      higherOrderWarning = true;
      cgnsOrderFlag = true;
      break;
    case CGNS_ENUMV(HEXA_27):
      cellType = VTK_TRIQUADRATIC_HEXAHEDRON;
      higherOrderWarning = true;
      cgnsOrderFlag = true;
      break;
    case CGNS_ENUMV(TRI_10):
    case CGNS_ENUMV(TRI_15):
      cellType = VTK_LAGRANGE_TRIANGLE;
      higherOrderWarning = true;
      break;
    case CGNS_ENUMV(QUAD_16):
      cellType = VTK_LAGRANGE_QUADRILATERAL;
      higherOrderWarning = true;
      cgnsOrderFlag = true;
      break;
    case CGNS_ENUMV(TETRA_20):
    case CGNS_ENUMV(TETRA_35):
      cellType = VTK_LAGRANGE_TETRAHEDRON;
      higherOrderWarning = true;
      cgnsOrderFlag = true;
      break;
    case CGNS_ENUMV(PYRA_30):
      cellType = VTK_LAGRANGE_PYRAMID;
      higherOrderWarning = true;
      break;
    case CGNS_ENUMV(PENTA_40):
      cellType = VTK_LAGRANGE_WEDGE;
      higherOrderWarning = true;
      cgnsOrderFlag = true;
      break;
    case CGNS_ENUMV(HEXA_64):
    case CGNS_ENUMV(HEXA_125):
      cellType = VTK_LAGRANGE_HEXAHEDRON;
      higherOrderWarning = true;
      cgnsOrderFlag = true;
      break;
    default:
      cellType = VTK_EMPTY_CELL;
      break;
  }
  return cellType;
}
//----------------------------------------------------------------------
// static const int NULL_translate[27] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
//                                  16,17,18,19,20,21,22,23,24,25,26};

// CGNS --> VTK ordering of Elements
// static const int NODE_ToVTK[1] = { 0 };
//
// static const int BAR_2_ToVTK[2] = { 0, 1 };
//
// static const int BAR_3_ToVTK[3] = { 0, 1, 2 };
//
// static const int BAR_4_ToVTK[4] = { 0, 1, 2, 3 };
//
// static const int TRI_3_ToVTK[3] = { 0, 1, 2 };
//
// static const int TRI_6_ToVTK[6] = { 0, 1, 2, 3, 4, 5 };
//
// static const int TRI_10_ToVTK[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
//
// static const int TRI_15_ToVTK[15] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
//
// static const int QUAD_4_ToVTK[4] = { 0, 1, 2, 3 };
//
// static const int QUAD_8_ToVTK[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
//
// static const int QUAD_9_ToVTK[9] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
//
// static const int TETRA_4_ToVTK[4] = { 0, 1, 2, 3 };
//
// static const int TETRA_10_ToVTK[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
//
// static const int PYRA_5_ToVTK[5] = { 0, 1, 2, 3, 4 };
//
// static const int PYRA_14_ToVTK[14] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 };
//
// static const int PENTA_6_ToVTK[6] = { 0, 1, 2, 3, 4, 5 };

static const int PENTA_15_ToVTK[15] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 13, 14, 9, 10, 11 };

static const int PENTA_18_ToVTK[18] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 13, 14, 9, 10, 11, 15, 16,
  17 };

// static const int HEXA_8_ToVTK[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };

static const int HEXA_20_ToVTK[20] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 16, 17, 18, 19, 12, 13,
  14, 15 };

static const int HEXA_27_ToVTK[27] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 16, 17, 18, 19, 12, 13,
  14, 15, 24, 22, 21, 23, 20, 25, 26 };

static const int TETRA_20_ToVTK[20] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 17,
  18, 19, 16 };

static const int TETRA_35_ToVTK[35] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
  17, 18, 19, 20, 21, 25, 26, 27, 28, 29, 30, 31, 32, 33, 22, 23, 24 };

static const int PENTA_40_ToVTK[40] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 18, 19, 20, 21, 22,
  23, 12, 13, 14, 15, 16, 17, 24, 37, 25, 26, 28, 27, 29, 30, 32, 31, 33, 34, 36, 35, 38, 39 };

static const int HEXA_64_ToVTK[64] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 12, 15, 14, 24, 25,
  26, 27, 29, 28, 31, 30, 16, 17, 18, 19, 20, 21, 22, 23, 49, 48, 50, 51, 40, 41, 43, 42, 36, 37,
  39, 38, 45, 44, 46, 47, 32, 33, 35, 34, 52, 53, 55, 54, 56, 57, 59, 58, 60, 61, 63, 62 };

static const int HEXA_125_ToVTK[125] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 16, 15, 14,
  19, 18, 17, 32, 33, 34, 35, 36, 37, 40, 39, 38, 43, 42, 41, 20, 21, 22, 23, 24, 25, 26, 27, 28,
  29, 30, 31, 82, 81, 80, 83, 88, 87, 84, 85, 86, 62, 63, 64, 69, 70, 65, 68, 67, 66, 53, 54, 55,
  60, 61, 56, 59, 58, 57, 73, 72, 71, 74, 79, 78, 75, 76, 77, 44, 45, 46, 51, 52, 47, 50, 49, 48,
  89, 90, 91, 96, 97, 92, 95, 94, 93, 98, 99, 100, 105, 106, 101, 104, 103, 102, 107, 108, 109, 114,
  115, 110, 113, 112, 111, 116, 117, 118, 123, 124, 119, 122, 121, 120 };

static const int PYRA_30_ToVTK[30] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 9, 12, 11, 13, 14, 15, 16, 17,
  18, 19, 20, 25, 26, 27, 28, 21, 22, 24, 23, 29 };

static const int QUAD_16_ToVTK[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 9, 8, 11, 10, 12, 13, 15, 14 };

//------------------------------------------------------------------------------
inline const int* getTranslator(int cellType, int numPointsPerCell)
{
  switch (cellType)
  {
    case VTK_VERTEX:
    case VTK_LINE:
    case VTK_QUADRATIC_EDGE:
    case VTK_CUBIC_LINE:
    case VTK_TRIANGLE:
    case VTK_QUADRATIC_TRIANGLE:
    case VTK_QUAD:
    case VTK_QUADRATIC_QUAD:
    case VTK_BIQUADRATIC_QUAD:
    case VTK_TETRA:
    case VTK_QUADRATIC_TETRA:
    case VTK_PYRAMID:
    case VTK_QUADRATIC_PYRAMID:
    case VTK_WEDGE:
      return nullptr;
    case VTK_QUADRATIC_WEDGE:
      return CGNSRead::PENTA_15_ToVTK;
    case VTK_BIQUADRATIC_QUADRATIC_WEDGE:
      return CGNSRead::PENTA_18_ToVTK;
    case VTK_HEXAHEDRON:
      return nullptr;
    case VTK_QUADRATIC_HEXAHEDRON:
      return CGNSRead::HEXA_20_ToVTK;
    case VTK_TRIQUADRATIC_HEXAHEDRON:
      return CGNSRead::HEXA_27_ToVTK;
    case VTK_LAGRANGE_QUADRILATERAL:
      return CGNSRead::QUAD_16_ToVTK;
    case VTK_LAGRANGE_TETRAHEDRON:
      if (numPointsPerCell == 35)
      {
        return CGNSRead::TETRA_35_ToVTK;
      }
      else
      {
        return CGNSRead::TETRA_20_ToVTK;
      }
    case VTK_LAGRANGE_WEDGE:
      return CGNSRead::PENTA_40_ToVTK;
    case VTK_LAGRANGE_HEXAHEDRON:
      if (numPointsPerCell == 125)
      {
        return CGNSRead::HEXA_125_ToVTK;
      }
      else
      {
        return CGNSRead::HEXA_64_ToVTK;
      }
    case VTK_LAGRANGE_PYRAMID:
      return CGNSRead::PYRA_30_ToVTK;
    default:
      return nullptr;
  }
}

//------------------------------------------------------------------------------
void CGNS2VTKorder(vtkIdType size, const int* cells_types, vtkIdType* elements)
{
  const int maxPointsPerCells = 125;
  int tmp[maxPointsPerCells];
  const int* translator;
  vtkIdType pos = 0;
  for (vtkIdType icell = 0; icell < size; ++icell)
  {
    vtkIdType numPointsPerCell = elements[pos];
    translator = getTranslator(cells_types[icell], numPointsPerCell);
    pos++;
    if (translator != nullptr)
    {
      for (vtkIdType ip = 0; ip < numPointsPerCell; ++ip)
      {
        tmp[ip] = elements[translator[ip] + pos];
      }
      for (vtkIdType ip = 0; ip < numPointsPerCell; ++ip)
      {
        elements[pos + ip] = tmp[ip];
      }
    }
    pos += numPointsPerCell;
  }
}

//------------------------------------------------------------------------------
void ReorderMonoCellPointsCGNS2VTK(
  vtkIdType size, int cell_type, vtkIdType numPointsPerCell, vtkIdType* elements)
{
  vtkNew<vtkIdTypeArray> tempArray;
  tempArray->SetNumberOfTuples(numPointsPerCell);

  const int* translator;
  translator = getTranslator(cell_type, numPointsPerCell);
  if (translator == nullptr)
  {
    return;
  }

  vtkIdType pos = 0;
  for (vtkIdType icell = 0; icell < size; ++icell)
  {
    for (vtkIdType ip = 0; ip < numPointsPerCell; ++ip)
    {
      tempArray->SetValue(ip, elements[translator[ip] + pos]);
    }
    for (vtkIdType ip = 0; ip < numPointsPerCell; ++ip)
    {
      elements[pos + ip] = tempArray->GetValue(ip);
    }
    pos += numPointsPerCell;
  }
}

//------------------------------------------------------------------------------
bool testValidVector(const CGNSVector& item)
{
  // apply some logic and return true or false
  return (item.numComp == 0);
}

//------------------------------------------------------------------------------
void fillVectorsFromVars(std::vector<CGNSRead::CGNSVariable>& vars,
  std::vector<CGNSRead::CGNSVector>& vectors, int physicalDim)
{
  // get number of scalars and vectors
  const std::size_t nvar = vars.size();
  std::size_t len;
  char_33 name;

  for (std::size_t n = 0; n < nvar; ++n)
  {
    vars[n].isComponent = false;
    vars[n].xyzIndex = 0;
  }

  for (std::size_t n = 0; n < nvar; ++n)
  {
    len = strlen(vars[n].name) - 1;
    // CGNS convention uses CamelCase for vector naming (VectorX)
    // but we can also detect Vector_X or Vector_x
    switch (vars[n].name[len])
    {
      case 'X':
        if (len > 0 && vars[n].name[len - 1] == '_')
        {
          len -= 1;
        }
        vars[n].xyzIndex = 1;
        vars[n].isComponent = true;
        break;
      case 'Y':
        if (len > 0 && vars[n].name[len - 1] == '_')
        {
          len -= 1;
        }
        vars[n].xyzIndex = 2;
        vars[n].isComponent = true;
        break;
      case 'Z':
        if (len > 0 && vars[n].name[len - 1] == '_')
        {
          len -= 1;
        }
        vars[n].xyzIndex = 3;
        vars[n].isComponent = true;
        break;
      case 'x':
        if (len > 0 && vars[n].name[len - 1] == '_')
        {
          vars[n].xyzIndex = 1;
          vars[n].isComponent = true;
          len -= 1;
        }
        break;
      case 'y':
        if (len > 0 && vars[n].name[len - 1] == '_')
        {
          vars[n].xyzIndex = 2;
          vars[n].isComponent = true;
          len -= 1;
        }
        break;
      case 'z':
        if (len > 0 && vars[n].name[len - 1] == '_')
        {
          vars[n].xyzIndex = 3;
          vars[n].isComponent = true;
          len -= 1;
        }
        break;
    }
    if (vars[n].isComponent)
    {
      strcpy(name, vars[n].name);
      name[len] = '\0';
      std::vector<CGNSRead::CGNSVector>::iterator iter = CGNSRead::getVectorFromName(vectors, name);
      if (iter != vectors.end())
      {
        iter->numComp += vars[n].xyzIndex;
        iter->xyzIndex[vars[n].xyzIndex - 1] = (int)n;
      }
      else
      {
        CGNSRead::CGNSVector newVector;
        newVector.xyzIndex[0] = -1;
        newVector.xyzIndex[1] = -1;
        newVector.xyzIndex[2] = -1;
        newVector.numComp = vars[n].xyzIndex;
        newVector.xyzIndex[vars[n].xyzIndex - 1] = (int)n;
        strcpy(newVector.name, name);
        vectors.push_back(newVector);
      }
    }
  }

  // Detect and tag invalid vector :
  bool invalid = false;
  for (auto& iter : vectors)
  {
    // Check if number of component agrees with phys_dim
    if (((physicalDim == 3) && (iter.numComp != 6)) || ((physicalDim == 2) && (iter.numComp != 3)))
    {
      for (int index = 0; index < physicalDim; index++)
      {
        int nv = iter.xyzIndex[index];
        if (nv >= 0)
        {
          vars[nv].isComponent = false;
        }
      }
      iter.numComp = 0;
      invalid = true;
    }
    // Check if a variable is present with a similar
    // name as the vector being built
    if (CGNSRead::isACGNSVariable(vars, iter.name))
    {
      // vtkWarningMacro ( "Warning, vector " << iter->name
      //                  << " can't be assembled." << std::endl );
      for (int index = 0; index < physicalDim; index++)
      {
        int n = iter.xyzIndex[index];
        if (n >= 0)
        {
          vars[n].isComponent = false;
        }
      }
      iter.numComp = 0;
      invalid = true;
    }
    if (iter.numComp > 0)
    {
      // Check if DataType_t are identical for all components
      if ((vars[iter.xyzIndex[0]].dt != vars[iter.xyzIndex[1]].dt) ||
        (vars[iter.xyzIndex[0]].dt != vars[iter.xyzIndex[physicalDim - 1]].dt))
      {
        for (int index = 0; index < physicalDim; index++)
        {
          int n = iter.xyzIndex[index];
          if (n >= 0)
          {
            vars[n].isComponent = false;
          }
        }
        iter.numComp = 0;
        invalid = true;
      }
    }
  }
  // Remove invalid vectors
  if (invalid)
  {
    vectors.erase(
      std::remove_if(vectors.begin(), vectors.end(), CGNSRead::testValidVector), vectors.end());
  }
}

//------------------------------------------------------------------------------
bool vtkCGNSMetaData::Parse(const char* cgnsFileName)
{
  if (!cgnsFileName)
  {
    return false;
  }

  if (this->LastReadFilename == cgnsFileName)
  {
    return true;
  }

  int cgioNum;
  int ier;
  double rootId;
  char nodeLabel[CGIO_MAX_NAME_LENGTH + 1];

  // use cgio routine to open the file
  if (cgio_open_file(cgnsFileName, CGIO_MODE_READ, CG_FILE_NONE, &cgioNum) != CG_OK)
  {
    char message[81];
    cgio_error_message(message);
    vtkErrorWithObjectMacro(nullptr, "Error loading CGNS file with cgio_file_open: " << message);
    return false;
  }
  if (cgio_get_root_id(cgioNum, &rootId) != CG_OK)
  {
    char message[81];
    cgio_error_message(message);
    vtkErrorWithObjectMacro(
      nullptr, "Error accessing CGNS root node with cgio_get_root_id: " << message);
    return false;
  }

  // Get base id list :
  std::vector<double> baseIds;
  ier = readBaseIds(cgioNum, rootId, baseIds);
  if (ier != 0)
  {
    return false;
  }

  if (!this->baseList.empty())
  {
    this->baseList.clear();
  }
  this->baseList.resize(baseIds.size());
  // Read base list
  for (std::size_t numBase = 0; numBase < baseIds.size(); numBase++)
  {
    // base names for later selection
    readBaseCoreInfo(cgioNum, baseIds[numBase], this->baseList[numBase]);

    std::vector<double> baseChildId;

    getNodeChildrenId(cgioNum, baseIds[numBase], baseChildId);

    std::size_t nzones = 0;
    std::size_t nn;
    for (nzones = 0, nn = 0; nn < baseChildId.size(); ++nn)
    {
      if (cgio_get_label(cgioNum, baseChildId[nn], nodeLabel) != CG_OK)
      {
        return false;
      }

      if (strcmp(nodeLabel, "Zone_t") == 0)
      {
        if (nzones < nn)
        {
          baseChildId[nzones] = baseChildId[nn];
        }
        nzones++;

        this->baseList[numBase].zones.emplace_back();
        if (readZoneInfo(cgioNum, baseChildId[nn], this->baseList[numBase].zones.back()) != CG_OK)
        {
          this->baseList[numBase].zones.pop_back();
        }
      }
      else if (strcmp(nodeLabel, "Family_t") == 0)
      {
        readBaseFamily(cgioNum, baseChildId[nn], this->baseList[numBase]);
      }
      else if (strcmp(nodeLabel, "BaseIterativeData_t") == 0)
      {
        readBaseIteration(cgioNum, baseChildId[nn], this->baseList[numBase]);
      }
      else if (strcmp(nodeLabel, "ReferenceState_t") == 0)
      {
        readBaseReferenceState(cgioNum, baseChildId[nn], this->baseList[numBase]);
      }
      else
      {
        cgio_release_id(cgioNum, baseChildId[nn]);
      }
    }
    this->baseList[numBase].nzones = static_cast<int>(nzones);

    if (this->baseList[numBase].times.empty())
    {
      // If no time information were found
      // just put default values
      this->baseList[numBase].steps.clear();
      this->baseList[numBase].times.clear();
      this->baseList[numBase].steps.push_back(0);
      this->baseList[numBase].times.push_back(0.0);
    }

    // Read variable name and more from each zone
    for (nn = 0; nn < nzones; ++nn)
    {
      readZoneInfo(cgioNum, baseChildId[nn], this->baseList[numBase]);
    }
  }

  // Same Timesteps in all root nodes
  // or separated time range by root nodes
  // timesteps need to be sorted for each root node
  this->GlobalTime.clear();
  for (std::size_t numBase = 0; numBase < baseList.size(); numBase++)
  {
    if (numBase == 0)
    {
      this->GlobalTime = this->baseList[numBase].times;
      continue;
    }
    const std::vector<double>& times = this->baseList[numBase].times;
    if (times.front() > this->GlobalTime.back())
    {
      this->GlobalTime.insert(this->GlobalTime.end(), times.begin(), times.end());
    }

    if (times.back() < this->GlobalTime.front())
    {
      this->GlobalTime.insert(this->GlobalTime.begin(), times.begin(), times.end());
    }
  }

  this->LastReadFilename = cgnsFileName;
  cgio_close_file(cgioNum);
  return true;
}

//------------------------------------------------------------------------------
void vtkCGNSMetaData::PrintSelf(std::ostream& os)
{
  os << "--> vtkCGNSMetaData" << std::endl;
  os << "LastReadFileName: " << this->LastReadFilename << std::endl;
  os << "Base information:" << std::endl;
  for (std::size_t b = 0; b < this->baseList.size(); b++)
  {
    os << "  Base name: " << this->baseList[b].name << std::endl;
    os << "    number of zones: " << this->baseList[b].nzones << std::endl;
    os << "    number of time steps: " << this->baseList[b].times.size() << std::endl;
    os << "    use unsteady grid: " << this->baseList[b].useGridPointers << std::endl;
    os << "    use unsteady flow: " << this->baseList[b].useFlowPointers << std::endl;

    for (int i = 0; i < this->baseList[b].PointDataArraySelection.GetNumberOfArrays(); ++i)
    {
      os << "      Vertex :: ";
      os << this->baseList[b].PointDataArraySelection.GetArrayName(i) << std::endl;
    }
    for (int i = 0; i < this->baseList[b].CellDataArraySelection.GetNumberOfArrays(); ++i)
    {
      os << "      Cell :: ";
      os << this->baseList[b].CellDataArraySelection.GetArrayName(i) << std::endl;
    }
    for (int i = 0; i < this->baseList[b].FaceDataArraySelection.GetNumberOfArrays(); ++i)
    {
      os << "      Face :: ";
      os << this->baseList[b].FaceDataArraySelection.GetArrayName(i) << std::endl;
    }

    os << "    Family Number: " << this->baseList[b].family.size() << std::endl;
    for (const auto& fam : this->baseList[b].family)
    {
      os << "      Family: " << fam.name << " is BC: " << fam.isBC << std::endl;
    }

    os << "    Reference State:" << std::endl;
    for (const auto& iter : this->baseList[b].referenceState)
    {
      os << "  Variable: " << iter.first;
      os << "  Value: " << iter.second << std::endl;
    }
  }
}

//------------------------------------------------------------------------------
static void BroadcastCGNSString(vtkMultiProcessController* ctrl, CGNSRead::char_33& str)
{
  int len = 33;
  ctrl->Broadcast(&len, 1, 0);
  ctrl->Broadcast(&str[0], len, 0);
}

//------------------------------------------------------------------------------
static void BroadcastString(vtkMultiProcessController* controller, std::string& str, int rank)
{
  unsigned long len = static_cast<unsigned long>(str.size()) + 1;
  controller->Broadcast(&len, 1, 0);
  if (len)
  {
    if (rank)
    {
      std::vector<char> tmp;
      tmp.resize(len);
      controller->Broadcast(tmp.data(), len, 0);
      str = tmp.data();
    }
    else
    {
      const char* start = str.c_str();
      std::vector<char> tmp(start, start + len);
      // NOLINTNEXTLINE(readability-container-data-pointer): needs C++17
      controller->Broadcast(&tmp[0], len, 0);
    }
  }
}
//------------------------------------------------------------------------------
static void BroadcastDoubleVector(
  vtkMultiProcessController* controller, std::vector<double>& dvec, int rank)
{
  unsigned long len = static_cast<unsigned long>(dvec.size());
  controller->Broadcast(&len, 1, 0);
  if (rank)
  {
    dvec.resize(len);
  }
  if (len)
  {
    controller->Broadcast(dvec.data(), len, 0);
  }
}
//------------------------------------------------------------------------------
static void BroadcastIntVector(
  vtkMultiProcessController* controller, std::vector<int>& ivec, int rank)
{
  unsigned long len = static_cast<unsigned long>(ivec.size());
  controller->Broadcast(&len, 1, 0);
  if (rank)
  {
    ivec.resize(len);
  }
  if (len)
  {
    controller->Broadcast(ivec.data(), len, 0);
  }
}
//------------------------------------------------------------------------------
static void BroadcastSelection(
  vtkMultiProcessController* controller, CGNSRead::vtkCGNSArraySelection& selection, int rank)
{
  unsigned long len = static_cast<unsigned long>(selection.size());
  controller->Broadcast(&len, 1, 0);
  if (rank == 0)
  {
    int tmp;
    for (auto& ite : selection)
    {
      unsigned long len2 = static_cast<unsigned long>(ite.first.size()) + 1;
      controller->Broadcast(&len2, 1, 0);
      if (len2)
      {
        const char* start = ite.first.c_str();
        std::vector<char> tmpVector(start, start + len2);
        controller->Broadcast(tmpVector.data(), len2, 0);
      }
      tmp = (int)ite.second;
      controller->Broadcast(&tmp, 1, 0);
    }
  }
  else
  {
    unsigned long i;
    for (i = 0; i < len; ++i)
    {
      std::string key;
      int tmp;
      CGNSRead::BroadcastString(controller, key, rank);
      selection[key] = false;
      controller->Broadcast(&tmp, 1, 0);
      selection[key] = (tmp != 0);
    }
  }
}

//------------------------------------------------------------------------------
static void BroadcastRefState(
  vtkMultiProcessController* controller, std::map<std::string, double>& refInfo, int rank)
{
  unsigned long len = static_cast<unsigned long>(refInfo.size());
  controller->Broadcast(&len, 1, 0);
  if (rank == 0)
  {
    for (auto& ite : refInfo)
    {
      unsigned long len2 = static_cast<unsigned long>(ite.first.size()) + 1;
      controller->Broadcast(&len2, 1, 0);
      if (len2)
      {
        const char* start = ite.first.c_str();
        std::vector<char> tmp(start, start + len2);
        controller->Broadcast(tmp.data(), len2, 0);
      }
      controller->Broadcast(&ite.second, 1, 0);
    }
  }
  else
  {
    for (unsigned long i = 0; i < len; ++i)
    {
      std::string key;
      CGNSRead::BroadcastString(controller, key, rank);
      refInfo[key] = 0.0;
      controller->Broadcast(&refInfo[key], 1, 0);
    }
  }
}

//------------------------------------------------------------------------------
static void BroadcastFamilies(vtkMultiProcessController* controller,
  std::vector<CGNSRead::FamilyInformation>& famInfo, int rank)
{
  unsigned long len = static_cast<unsigned long>(famInfo.size());
  controller->Broadcast(&len, 1, 0);
  if (rank != 0)
  {
    famInfo.resize(len);
  }
  for (auto& ite : famInfo)
  {
    BroadcastString(controller, ite.name, rank);
    int flags = 0;
    if (rank == 0)
    {
      if (ite.isBC)
      {
        flags = 1;
      }
      controller->Broadcast(&flags, 1, 0);
    }
    else
    {
      controller->Broadcast(&flags, 1, 0);
      if ((flags & 1) != 0)
      {
        ite.isBC = true;
      }
    }
  }
}

//------------------------------------------------------------------------------
static void BroadcastZones(
  vtkMultiProcessController* controller, std::vector<CGNSRead::ZoneInformation>& zoneInfo, int rank)
{
  vtkMultiProcessStream stream;
  if (rank == 0)
  {
    stream << static_cast<unsigned int>(zoneInfo.size());
    for (auto& zinfo : zoneInfo)
    {
      stream.Push(zinfo.name, 33);
      stream << zinfo.family;
      stream << static_cast<unsigned int>(zinfo.bcs.size());
      for (auto& bcinfo : zinfo.bcs)
      {
        stream.Push(bcinfo.name, 33);
        stream << bcinfo.family;
      }
    }
  }
  controller->Broadcast(stream, 0);
  if (rank != 0)
  {
    unsigned int count;
    stream >> count;
    zoneInfo.resize(count);

    for (auto& zinfo : zoneInfo)
    {
      unsigned int size = 33;
      char* cref = zinfo.name;
      stream.Pop(cref, size);
      stream >> zinfo.family;
      stream >> count;
      zinfo.bcs.resize(count);
      for (auto& bcinfo : zinfo.bcs)
      {
        cref = bcinfo.name;
        stream.Pop(cref, size);
        stream >> bcinfo.family;
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkCGNSMetaData::Broadcast(vtkMultiProcessController* controller, int rank)
{
  unsigned long len = static_cast<unsigned long>(this->baseList.size());
  controller->Broadcast(&len, 1, 0);
  if (rank != 0)
  {
    this->baseList.resize(len);
  }
  for (auto& ite : this->baseList)
  {
    CGNSRead::BroadcastCGNSString(controller, ite.name);
    controller->Broadcast(&ite.cellDim, 1, 0);
    controller->Broadcast(&ite.physicalDim, 1, 0);
    controller->Broadcast(&ite.baseNumber, 1, 0);
    controller->Broadcast(&ite.nzones, 1, 0);

    int flags = 0;
    if (rank == 0)
    {
      if (ite.useGridPointers)
      {
        flags = 1;
      }
      if (ite.useFlowPointers)
      {
        flags = (flags | 2);
      }
      controller->Broadcast(&flags, 1, 0);
    }
    else
    {
      controller->Broadcast(&flags, 1, 0);
      if ((flags & 1) != 0)
      {
        ite.useGridPointers = true;
      }
      if ((flags & 2) != 0)
      {
        ite.useFlowPointers = true;
      }
    }

    CGNSRead::BroadcastRefState(controller, ite.referenceState, rank);
    CGNSRead::BroadcastFamilies(controller, ite.family, rank);
    CGNSRead::BroadcastZones(controller, ite.zones, rank);

    CGNSRead::BroadcastSelection(controller, ite.PointDataArraySelection, rank);
    CGNSRead::BroadcastSelection(controller, ite.CellDataArraySelection, rank);
    CGNSRead::BroadcastSelection(controller, ite.FaceDataArraySelection, rank);

    BroadcastIntVector(controller, ite.steps, rank);
    BroadcastDoubleVector(controller, ite.times, rank);
  }
  CGNSRead::BroadcastString(controller, this->LastReadFilename, rank);
  BroadcastDoubleVector(controller, this->GlobalTime, rank);
}

//------------------------------------------------------------------------------
bool ReadBase(vtkCGNSReader* reader, const BaseInformation& baseInfo)
{
  auto baseSelection = reader->GetBaseSelection();
  if (!baseSelection->ArrayIsEnabled(baseInfo.name))
  {
    // base has not been enabled.
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool ReadGridForZone(
  vtkCGNSReader* reader, const BaseInformation& baseInfo, const ZoneInformation& zoneInfo)
{
  if (!reader->GetLoadMesh())
  {
    return false;
  } // mesh (aka grid) is not enabled.

  auto baseSelection = reader->GetBaseSelection();
  if (!baseSelection->ArrayIsEnabled(baseInfo.name))
  {
    // base has not been enabled.
    return false;
  }

  // check if the zone's family is enabled.
  auto familySelection = reader->GetFamilySelection();
  if (familySelection->ArrayExists(zoneInfo.family.c_str()) &&
    !familySelection->ArrayIsEnabled(zoneInfo.family.c_str()))
  {
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool ReadPatchesForBase(vtkCGNSReader* reader, const BaseInformation&)
{
  if (!reader->GetLoadBndPatch())
  {
    // patches have been globally disabled.
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool ReadSurfacesForBase(vtkCGNSReader* reader, const BaseInformation&)
{
  if (!reader->GetLoadSurfacePatch())
  {
    // surface patches have been globally disabled.
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool ReadPatch(vtkCGNSReader* reader, const BaseInformation&, const ZoneInformation&,
  const std::string& patchFamilyname)
{
  auto familySelection = reader->GetFamilySelection();

  if (!patchFamilyname.empty() && !familySelection->ArrayIsEnabled(patchFamilyname.c_str()))
  {
    return false;
  }

  return true;
}

VTK_ABI_NAMESPACE_END
} // end of namespace
