/*=========================================================================

  Program:   Visualization Toolkit
  Module:    cgio_helpers.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//  Copyright 2013-2014 Mickael Philit.

#include "cgio_helpers.h"

namespace CGNSRead
{
//----------------------------------------------------------------------------
int readNodeStringData(int cgioNum, double nodeId, std::string& data)
{
  int n;
  cgsize_t size = 1;
  cgsize_t dimVals[12];
  int ndim;

  if (cgio_get_dimensions(cgioNum, nodeId, &ndim, dimVals) != CG_OK)
  {
    cgio_error_exit("cgio_get_dimensions");
    return 1;
  }

  // allocate data
  for (n = 0; n < ndim; n++)
  {
    size *= dimVals[n];
  }
  if (size <= 0)
  {
    return 1;
  }

  data.resize(size);
  // read data
  if (cgio_read_all_data_type(cgioNum, nodeId, "C1", (void*)data.c_str()) != CG_OK)
  {
    return 1;
  }

  return 0;
}
//------------------------------------------------------------------------------
// Specialize char array
template <>
int readNodeData<char>(int cgioNum, double nodeId, std::vector<char>& data)
{
  int n;
  cgsize_t size = 1;
  cgsize_t dimVals[12];
  int ndim;

  if (cgio_get_dimensions(cgioNum, nodeId, &ndim, dimVals) != CG_OK)
  {
    cgio_error_exit("cgio_get_dimensions");
    return 1;
  }

  // allocate data
  for (n = 0; n < ndim; n++)
  {
    size *= dimVals[n];
  }
  if (size <= 0)
  {
    return 1;
  }

  data.resize(size + 1);

  // read data
  if (cgio_read_all_data_type(cgioNum, nodeId, "C1", data.data()) != CG_OK)
  {
    return 1;
  }
  data[size] = '\0';

  return 0;
}

//------------------------------------------------------------------------------
int getNodeChildrenId(int cgioNum, double fatherId, std::vector<double>& childrenIds)
{
  int nchildren;
  int len;

  cgio_number_children(cgioNum, fatherId, &nchildren);

  childrenIds.resize(nchildren);
  double* idList = new double[nchildren];

  cgio_children_ids(cgioNum, fatherId, 1, nchildren, &len, idList);

  if (len != nchildren)
  {
    delete[] idList;
    std::cerr << "Mismatch in number of children and child IDs read" << std::endl;
    return 1;
  }

  for (int child = 0; child < nchildren; child++)
  {
    childrenIds[child] = idList[child];
  }

  delete[] idList;
  return 0;
}

//------------------------------------------------------------------------------
int readBaseIds(int cgioNum, double rootId, std::vector<double>& baseIds)
{
  CGNSRead::char_33 nodeLabel;
  std::size_t nbases = 0;
  std::size_t nc;

  baseIds.clear();
  getNodeChildrenId(cgioNum, rootId, baseIds);
  if (baseIds.empty())
  {
    std::cerr << "Error: Not enough nodes under the root description file." << std::endl;
    return 1;
  }

  for (nbases = 0, nc = 0; nc < baseIds.size(); nc++)
  {
    if (cgio_get_label(cgioNum, baseIds[nc], nodeLabel) != CG_OK)
    {
      return 1;
    }
    if (strcmp(nodeLabel, "CGNSBase_t") == 0)
    {
      if (nbases < nc)
      {
        baseIds[nbases] = baseIds[nc];
      }
      nbases++;
    }
    else
    {
      cgio_release_id(cgioNum, baseIds[nc]);
    }
  }
  baseIds.resize(nbases);

  if (nbases < 1)
  {
    std::cerr << "Error: Not enough bases in the file." << std::endl;
    return 1;
  }

  return 0;
}

//------------------------------------------------------------------------------
int readBaseCoreInfo(int cgioNum, double baseId, CGNSRead::BaseInformation& baseInfo)
{
  CGNSRead::char_33 dataType;
  std::vector<int32_t> mdata;

  if (cgio_get_name(cgioNum, baseId, baseInfo.name) != CG_OK)
  {
    std::cerr << "cgio_get_name" << std::endl;
    return 1;
  }

  // read node data type
  if (cgio_get_data_type(cgioNum, baseId, dataType) != CG_OK)
  {
    return 1;
  }

  if (strcmp(dataType, "I4") != 0)
  {
    std::cerr << "Unexpected data type for dimension data of base" << std::endl;
    return 1;
  }

  if (CGNSRead::readNodeData<int32_t>(cgioNum, baseId, mdata) != 0)
  {
    std::cerr << "error while reading base dimension" << std::endl;
    return 1;
  }

  baseInfo.cellDim = mdata[0];
  baseInfo.physicalDim = mdata[1];

  return 0;
}

//------------------------------------------------------------------------------
int readBaseIteration(int cgioNum, double nodeId, CGNSRead::BaseInformation& baseInfo)
{
  CGNSRead::char_33 nodeLabel;
  CGNSRead::char_33 nodeName;
  CGNSRead::char_33 dataType;

  bool createTimeStates = true;
  bool createIterStates = true;

  std::vector<int32_t> ndata;
  // read node data type
  if (cgio_get_data_type(cgioNum, nodeId, dataType) != CG_OK)
  {
    return 1;
  }

  if (strcmp(dataType, "I4") != 0)
  {
    std::cerr << "Unexpected data type for iteration number of steps" << std::endl;
    return 1;
  }

  if (CGNSRead::readNodeData<int32_t>(cgioNum, nodeId, ndata) != 0)
  {
    std::cerr << "error while reading number of state in base" << std::endl;
    return 1;
  }
  int nstates = ndata[0];
  std::vector<double> childrenIterative;

  getNodeChildrenId(cgioNum, nodeId, childrenIterative);

  for (std::size_t nc = 0; nc < childrenIterative.size(); ++nc)
  {
    if (cgio_get_label(cgioNum, childrenIterative[nc], nodeLabel) != CG_OK)
    {
      return 1;
    }

    if (cgio_get_name(cgioNum, childrenIterative[nc], nodeName) != CG_OK)
    {
      return 1;
    }

    bool isDataArray = (strcmp(nodeLabel, "DataArray_t") == 0);
    if (isDataArray && (strcmp(nodeName, "TimeValues") == 0))
    {
      // Read time values
      // read node data type
      if (cgio_get_data_type(cgioNum, childrenIterative[nc], dataType) != CG_OK)
      {
        return 1;
      }
      baseInfo.times.clear();
      if (strcmp(dataType, "R8") == 0)
      {
        CGNSRead::readNodeData<double>(cgioNum, childrenIterative[nc], baseInfo.times);
      }
      else if (strcmp(dataType, "R4") == 0)
      {
        std::vector<float> iteData;
        CGNSRead::readNodeData<float>(cgioNum, childrenIterative[nc], iteData);
        baseInfo.times.resize(iteData.size());
        for (std::size_t ii = 0; ii < iteData.size(); ii++)
        {
          baseInfo.times[ii] = (double)iteData[ii];
        }
      }
      else
      {
        std::cerr << "Unexpected data type for iterative data" << std::endl;
        return 1;
      }

      if (static_cast<int>(baseInfo.times.size()) != nstates)
      {
        std::cerr << "Error reading times node";
        return 1;
      }

      createTimeStates = false;
    }
    else if (isDataArray && (strcmp(nodeName, "IterationValues") == 0))
    {
      // Read time steps
      // read node data type
      if (cgio_get_data_type(cgioNum, childrenIterative[nc], dataType) != CG_OK)
      {
        return 1;
      }

      if (strcmp(dataType, "I4") != 0)
      {
        std::cerr << "Unexpected data type for iterative data" << std::endl;
        return 1;
      }

      baseInfo.steps.clear();
      CGNSRead::readNodeData<int32_t>(cgioNum, childrenIterative[nc], baseInfo.steps);
      if (static_cast<int>(baseInfo.steps.size()) != nstates)
      {
        std::cerr << "Error reading steps node";
        return 1;
      }
      createIterStates = false;
    }
    else
    {
      cgio_release_id(cgioNum, childrenIterative[nc]);
    }
  }

  if (createIterStates)
  {
    for (int i = 0; i < nstates; ++i)
    {
      baseInfo.steps.push_back(i);
    }
  }
  if (createTimeStates)
  {
    for (int i = 0; i < nstates; ++i)
    {
      baseInfo.times.push_back((double)baseInfo.steps[i]);
    }
  }
  return 0;
}

//------------------------------------------------------------------------------
int readZoneIterInfo(int cgioNum, double nodeId, CGNSRead::BaseInformation& baseInfo)
{
  CGNSRead::char_33 nodeLabel;
  CGNSRead::char_33 nodeName;
  std::vector<double> iterChildId;

  getNodeChildrenId(cgioNum, nodeId, iterChildId);

  for (std::size_t nn = 0; nn < iterChildId.size(); nn++)
  {

    if (cgio_get_name(cgioNum, iterChildId[nn], nodeName) != CG_OK)
    {
      return 1;
    }
    if (cgio_get_label(cgioNum, iterChildId[nn], nodeLabel) != CG_OK)
    {
      return 1;
    }
    bool isDataArray = (strcmp(nodeLabel, "DataArray_t") == 0);
    if (isDataArray && (strcmp(nodeName, "GridCoordinatesPointers") == 0))
    {
      baseInfo.useGridPointers = true;
    }
    else if (isDataArray && (strcmp(nodeName, "FlowSolutionPointers") == 0))
    {
      baseInfo.useFlowPointers = true;
      // Maybe load FlowSolutionPointers once and for all
    }
    cgio_release_id(cgioNum, iterChildId[nn]);
  }
  return 0;
}

//------------------------------------------------------------------------------
int readSolInfo(int cgioNum, double nodeId, CGNSRead::BaseInformation& baseInfo)
{
  CGNS_ENUMT(GridLocation_t) varCentering = CGNS_ENUMV(Vertex);
  CGNSRead::char_33 nodeLabel;
  std::vector<double> solChildId;

  getNodeChildrenId(cgioNum, nodeId, solChildId);

  std::vector<CGNSRead::CGNSVariable> cgnsVars;
  std::vector<CGNSRead::CGNSVector> cgnsVectors;

  std::size_t nn;
  std::size_t nvars = 0;

  for (nvars = 0, nn = 0; nn < solChildId.size(); nn++)
  {
    if (cgio_get_label(cgioNum, solChildId[nn], nodeLabel) != CG_OK)
    {
      std::cerr << "Error while reading nodelabel" << std::endl;
      return 1;
    }

    if (strcmp(nodeLabel, "DataArray_t") == 0)
    {
      CGNSRead::CGNSVariable curVar;

      if (cgio_get_name(cgioNum, solChildId[nn], curVar.name) != CG_OK)
      {
        return 1;
      }
      curVar.isComponent = false;
      curVar.xyzIndex = 0;

      // read node data type
      CGNSRead::char_33 dataType;
      if (cgio_get_data_type(cgioNum, solChildId[nn], dataType))
      {
        continue;
      }
      if (strcmp(dataType, "R8") == 0)
      {
        curVar.dt = CGNS_ENUMV(RealDouble);
      }
      else if (strcmp(dataType, "R4") == 0)
      {
        curVar.dt = CGNS_ENUMV(RealSingle);
      }
      else if (strcmp(dataType, "I4") == 0)
      {
        curVar.dt = CGNS_ENUMV(Integer);
      }
      else if (strcmp(dataType, "I8") == 0)
      {
        curVar.dt = CGNS_ENUMV(LongInteger);
      }
      else
      {
        continue;
      }

      cgnsVars.push_back(curVar);
      cgio_release_id(cgioNum, solChildId[nn]);
      nvars++;
    }
    else if (strcmp(nodeLabel, "GridLocation_t") == 0)
    {
      CGNSRead::char_33 dataType;

      if (cgio_get_data_type(cgioNum, solChildId[nn], dataType) != CG_OK)
      {
        return 1;
      }

      if (strcmp(dataType, "C1") != 0)
      {
        std::cerr << "Unexpected data type for GridLocation_t node" << std::endl;
        return 1;
      }

      std::string location;
      CGNSRead::readNodeStringData(cgioNum, solChildId[nn], location);

      if (location == "Vertex")
      {
        varCentering = CGNS_ENUMV(Vertex);
      }
      else if (location == "CellCenter")
      {
        varCentering = CGNS_ENUMV(CellCenter);
      }
      else if (location == "FaceCenter")
      {
        varCentering = CGNS_ENUMV(FaceCenter);
      }
      else
      {
        varCentering = CGNS_ENUMV(GridLocationNull);
      }
    }
    else
    {
      cgio_release_id(cgioNum, solChildId[nn]);
    }
  }

  if (nvars == 0)
  {
    return 1;
  }

  if (varCentering != CGNS_ENUMV(Vertex) && varCentering != CGNS_ENUMV(CellCenter) &&
    varCentering != CGNS_ENUMV(FaceCenter))
  {
    std::cerr << "Unsupported centering type encountered! Only Vertex, CellCenter and "
                 "FaceCenter are supported."
              << std::endl;
    return 1;
  }

  CGNSRead::fillVectorsFromVars(cgnsVars, cgnsVectors, baseInfo.physicalDim);

  for (std::size_t ii = 0; ii < cgnsVars.size(); ++ii)
  {
    if (cgnsVars[ii].isComponent)
    {
      continue;
    }
    switch (varCentering)
    {
      case CGNS_ENUMV(Vertex):
        if (!baseInfo.PointDataArraySelection.HasArray(cgnsVars[ii].name))
        {
          baseInfo.PointDataArraySelection.AddArray(cgnsVars[ii].name, false);
        }
        break;

      case CGNS_ENUMV(CellCenter):
        if (!baseInfo.CellDataArraySelection.HasArray(cgnsVars[ii].name))
        {
          baseInfo.CellDataArraySelection.AddArray(cgnsVars[ii].name, false);
        }
        break;

      case CGNS_ENUMV(FaceCenter):
        if (!baseInfo.FaceDataArraySelection.HasArray(cgnsVars[ii].name))
        {
          baseInfo.FaceDataArraySelection.AddArray(cgnsVars[ii].name, false);
        }
        break;

      default:
        break;
    }
  }
  for (std::size_t jj = 0; jj < cgnsVectors.size(); ++jj)
  {
    switch (varCentering)
    {
      case CGNS_ENUMV(Vertex):
        if (!baseInfo.PointDataArraySelection.HasArray(cgnsVectors[jj].name))
        {
          baseInfo.PointDataArraySelection.AddArray(cgnsVectors[jj].name, false);
        }
        break;

      case CGNS_ENUMV(CellCenter):
        if (!baseInfo.CellDataArraySelection.HasArray(cgnsVectors[jj].name))
        {
          baseInfo.CellDataArraySelection.AddArray(cgnsVectors[jj].name, false);
        }
        break;

      case CGNS_ENUMV(FaceCenter):
        if (!baseInfo.FaceDataArraySelection.HasArray(cgnsVectors[jj].name))
        {
          baseInfo.FaceDataArraySelection.AddArray(cgnsVectors[jj].name, false);
        }
        break;

      default:
        break;
    }
  }
  return 0;
}

//------------------------------------------------------------------------------
int readBaseFamily(
  int cgioNum, double nodeId, CGNSRead::BaseInformation& baseInfo, const std::string& parentPath)
{
  CGNSRead::FamilyInformation curFamily;
  CGNSRead::char_33 nodeLabel;
  CGNSRead::char_33 nodeName;
  std::vector<double> famChildId;

  if (cgio_get_name(cgioNum, nodeId, nodeName) != CG_OK)
  {
    return 1;
  }
  curFamily.isBC = false;

  // use a path relative to base to select Family_t part of Family_t tree
  std::string curPath = std::string(nodeName);
  if (!parentPath.empty())
  {
    curPath = parentPath + "/" + curPath;
  }
  curFamily.name = curPath;

  getNodeChildrenId(cgioNum, nodeId, famChildId);

  for (std::size_t nn = 0; nn < famChildId.size(); nn++)
  {
    if (cgio_get_label(cgioNum, famChildId[nn], nodeLabel) != CG_OK)
    {
      return 1;
    }
    if (strcmp(nodeLabel, "FamilyBC_t") == 0)
    {
      curFamily.isBC = true;
    }
    else if (strcmp(nodeLabel, "Family_t") == 0)
    {
      readBaseFamily(cgioNum, famChildId[nn], baseInfo, curPath);
    }
  }
  CGNSRead::releaseIds(cgioNum, famChildId);
  baseInfo.family.push_back(curFamily);

  return 0;
}

//------------------------------------------------------------------------------
int readBaseReferenceState(int cgioNum, double nodeId, CGNSRead::BaseInformation& baseInfo)
{
  CGNSRead::char_33 nodeLabel;
  CGNSRead::char_33 curName;

  std::vector<double> children;
  getNodeChildrenId(cgioNum, nodeId, children);

  std::size_t nn;
  for (nn = 0; nn < children.size(); nn++)
  {
    if (cgio_get_label(cgioNum, children[nn], nodeLabel) != CG_OK)
    {
      std::cerr << "Error while reading nodelabel" << std::endl;
      return 1;
    }
    if (cgio_get_name(cgioNum, children[nn], curName) != CG_OK)
    {
      return 1;
    }
    bool isDataArray = strcmp(nodeLabel, "DataArray_t") == 0;
    if (isDataArray &&
      ((strcmp(curName, "Mach") == 0) || (strcmp(curName, "SpecificHeatRatio") == 0) ||
        (strcmp(curName, "IdealGasConstant") == 0) ||
        (strcmp(curName, "SpecificHeatVolume") == 0) ||
        (strcmp(curName, "SpecificHeatPressure") == 0)))
    {
      // read node data type
      CGNSRead::char_33 dataType;
      if (cgio_get_data_type(cgioNum, children[nn], dataType))
      {
        return 1;
      }

      if (strcmp(dataType, "R8") == 0)
      {
        std::vector<double> bdata;
        if (CGNSRead::readNodeData<double>(cgioNum, children[nn], bdata) == 0)
        {
          baseInfo.referenceState[curName] = (double)bdata[0];
        }
      }
      else if (strcmp(dataType, "R4") == 0)
      {
        std::vector<float> bdata;
        if (CGNSRead::readNodeData<float>(cgioNum, children[nn], bdata) == 0)
        {
          baseInfo.referenceState[curName] = (double)bdata[0];
        }
      }
      else
      {
        std::cerr << "Unexpected data in ReferenceState_t" << std::endl;
        return 1;
      }
    }
    cgio_release_id(cgioNum, children[nn]);
  }
  return 0;
}

//------------------------------------------------------------------------------
int readZoneInfo(int cgioNum, double nodeId, CGNSRead::BaseInformation& baseInfo)
{
  CGNSRead::char_33 nodeLabel;
  std::vector<double> zoneChildId;

  getNodeChildrenId(cgioNum, nodeId, zoneChildId);

  int nflows = 0;
  std::size_t nn;
  for (nflows = 0, nn = 0; nn < zoneChildId.size(); nn++)
  {

    if (cgio_get_label(cgioNum, zoneChildId[nn], nodeLabel) != CG_OK)
    {
      std::cerr << "Error while reading node label" << std::endl;
      return 1;
    }

    if ((nflows < 3) && (strcmp(nodeLabel, "FlowSolution_t") == 0))
    {
      // Read only 3 Flow Solution to have a chance
      // to get Cell and Vertex variables
      // C=Cell V=Vertex
      // Layout sample :
      //    1. C init state (this one may be not processed due
      //                     to FlowSolutionPointers but we still
      //                     want some information about the two next node)
      //    2. C time 1s
      //    3. V time 1s

      if (readSolInfo(cgioNum, zoneChildId[nn], baseInfo) == 0)
      {
        nflows++;
      }
    }
    else if (strcmp(nodeLabel, "ZoneIterativeData_t") == 0)
    {
      // get time information
      readZoneIterInfo(cgioNum, zoneChildId[nn], baseInfo);
    }
    cgio_release_id(cgioNum, zoneChildId[nn]);
  }
  return 0;
}

//------------------------------------------------------------------------------
int readZoneInfo(int cgioNum, double zoneId, CGNSRead::ZoneInformation& zoneInfo)
{
  if (cgio_get_name(cgioNum, zoneId, zoneInfo.name) != CG_OK)
  {
    return 1;
  }

  std::vector<double> zoneChildren;
  getNodeChildrenId(cgioNum, zoneId, zoneChildren);
  for (double zoneChildId : zoneChildren)
  {
    CGNSRead::char_33 nodeLabel;
    if (cgio_get_label(cgioNum, zoneChildId, nodeLabel) == CG_OK)
    {
      if (strcmp(nodeLabel, "FamilyName_t") == 0)
      {
        CGNSRead::readNodeStringData(cgioNum, zoneChildId, zoneInfo.family);
        if (!zoneInfo.family.empty() && zoneInfo.family[0] == '/')
        {
          // This is a family path
          std::string::size_type pos = zoneInfo.family.find('/', 1);
          if (pos == std::string::npos)
          {
            // Invalid family path
            return 1;
          }
          // Remove /Base prefix of path for backward compatibility
          zoneInfo.family = zoneInfo.family.substr(pos + 1);
        }
      }
      else if (strcmp(nodeLabel, "ZoneBC_t") == 0)
      {
        std::vector<double> zoneBCChildren;
        getNodeChildrenId(cgioNum, zoneChildId, zoneBCChildren);
        for (double zoneBCChildId : zoneBCChildren)
        {
          if (cgio_get_label(cgioNum, zoneBCChildId, nodeLabel) == CG_OK &&
            strcmp(nodeLabel, "BC_t") == 0)
          {
            CGNSRead::ZoneBCInformation bcinfo;
            cgio_get_name(cgioNum, zoneBCChildId, bcinfo.name);

            // now read family info for this BC_t.
            std::vector<double> bcChildren;
            getNodeChildrenId(cgioNum, zoneBCChildId, bcChildren);
            for (double bcChildId : bcChildren)
            {
              if (cgio_get_label(cgioNum, bcChildId, nodeLabel) == CG_OK &&
                strcmp(nodeLabel, "FamilyName_t") == 0)
              {
                CGNSRead::readNodeStringData(cgioNum, bcChildId, bcinfo.family);
                if (!bcinfo.family.empty() && bcinfo.family[0] == '/')
                {
                  // This is a family path
                  std::string::size_type pos = bcinfo.family.find('/', 1);
                  if (pos == std::string::npos)
                  {
                    // Invalid family path
                    return 1;
                  }
                  bcinfo.family = bcinfo.family.substr(pos + 1);
                }
                break;
              }
            }
            releaseIds(cgioNum, bcChildren);
            zoneInfo.bcs.push_back(bcinfo);
          }
        }
        releaseIds(cgioNum, zoneBCChildren);
      }
    }
  }
  releaseIds(cgioNum, zoneChildren);
  return 0;
}

//------------------------------------------------------------------------------
void releaseIds(int cgioNum, const std::vector<double>& ids)
{
  for (auto iter = ids.begin(); iter != ids.end(); ++iter)
  {
    cgio_release_id(cgioNum, *iter);
  }
}
}
