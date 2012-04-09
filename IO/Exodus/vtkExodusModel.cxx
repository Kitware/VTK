/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExodusModel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkExodusModel.h"
#include "vtkUnstructuredGrid.h"
#include "vtkObjectFactory.h"
#include "vtkDataArray.h"
#include "vtkIntArray.h"
#include "vtkFloatArray.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include <ctype.h>
#include <set>
#include <map>
#include "vtk_exodusII.h"
#include <ctype.h>


vtkStandardNewMacro(vtkExodusModel);

vtkExodusModel::vtkExodusModel()
{
  this->ModelMetadata = NULL;
  this->GeometryCount = -1;
}

vtkExodusModel::~vtkExodusModel()
{
  this->SetModelMetadata(NULL);
}

void vtkExodusModel::Reset()
{
  if (this->ModelMetadata)
    {
    this->ModelMetadata->Reset();
    }
}

vtkModelMetadata *vtkExodusModel::GetModelMetadata()
{
  if (!this->ModelMetadata)
    {
    this->ModelMetadata = vtkModelMetadata::New();
    this->ModelMetadata->Register(this);
    }

  return this->ModelMetadata;
}

void vtkExodusModel::SetModelMetadata(vtkModelMetadata *emd)
{
  if (this->ModelMetadata == emd) return;

  if (this->ModelMetadata)
    {
    this->ModelMetadata->UnRegister(this);
    this->ModelMetadata->Delete();
    this->ModelMetadata = NULL;
    }

  if (emd)
    {
    this->ModelMetadata = emd;
    emd->Register(this);
    }
}

//---------------------------------------------------------------
// Initialize this ExodusModel object with the ExodusModel
// packed into a vtkUnstructuredGrid's field arrays.
//---------------------------------------------------------------

int vtkExodusModel::HasMetadata(vtkUnstructuredGrid *grid)
{
  int hasIt = 0;

  if (grid)
    {
    hasIt = vtkModelMetadata::HasMetadata(grid);
    }

  return hasIt;
}
int vtkExodusModel::UnpackExodusModel(vtkUnstructuredGrid *grid, int deleteIt)
{
  vtkModelMetadata *mmd = this->GetModelMetadata();

  int fail = mmd->Unpack(grid, deleteIt);

  return fail;
}

//---------------------------------------------------------------
// Pack the metadata in this ExodusModel object into the
// supplied vtkUnstructuredGrid.
//---------------------------------------------------------------

void vtkExodusModel::PackExodusModel(vtkUnstructuredGrid *grid)
{
  vtkModelMetadata *mmd = this->GetModelMetadata();

  mmd->Pack(grid);

  return;
}
//---------------------------------------------------------------
// Set all the global fields of the Exodus Model from an open
// Exodus file.
//---------------------------------------------------------------

int vtkExodusModel::SetGlobalInformation(int fid, int compute_word_size)
{
  int i;
  int use_floats = (compute_word_size == sizeof(float));

  int intVal;
  float floatVal;
  char charVal;

  vtkModelMetadata *emd = this->GetModelMetadata();

  ex_opts(0);    // turn off all error messages

  emd->FreeAllGlobalData();
  emd->FreeAllLocalData();

  // Title and dimension

  char *title = new char [MAX_LINE_LENGTH + 1];
  int dim;

  ex_inquire(fid, EX_INQ_TITLE, &intVal, &floatVal, title);
  ex_inquire(fid, EX_INQ_DIM, &dim, &floatVal, &charVal);

  emd->SetTitle(title);

  // QA records

  int nqaRecs;
  ex_inquire(fid, EX_INQ_QA, &nqaRecs, &floatVal, &charVal);

  if (nqaRecs > 0)
    {
    typedef char *p4[4];
    p4 *qarecs = new p4 [nqaRecs];

    for (i=0; i<nqaRecs; i++)
      {
      qarecs[i][0] = new char [MAX_STR_LENGTH + 1];
      qarecs[i][1] = new char [MAX_STR_LENGTH + 1];
      qarecs[i][2] = new char [MAX_STR_LENGTH + 1];
      qarecs[i][3] = new char [MAX_STR_LENGTH + 1];
      }

    ex_get_qa(fid, qarecs);
    emd->SetQARecords(nqaRecs, qarecs);
    }

  // Information lines

  int ninfoLines;
  ex_inquire(fid, EX_INQ_INFO, &ninfoLines, &floatVal, &charVal);

  if (ninfoLines > 0)
    {
    char **lines = new char * [ninfoLines];
    for (i=0; i<ninfoLines; i++)
      {
      lines[i] = new char [MAX_LINE_LENGTH + 1];
      }

    ex_get_info(fid, lines);
    emd->SetInformationLines(ninfoLines, lines);
    }

  // Coordinate names

  char **coordNames = new char * [dim];
  for (i=0; i<dim; i++)
    {
    coordNames[i] = new char [MAX_STR_LENGTH + 1];
    }

  ex_get_coord_names(fid, coordNames);
  emd->SetCoordinateNames(dim, coordNames);

  // Time steps
  //   TODO - We convert time steps to float.  We should fix this
  //   to respect the precision of the time values in the input
  //   file.

  int nTimeSteps;
  ex_inquire(fid, EX_INQ_TIME, &nTimeSteps, &floatVal, &charVal);

  if (nTimeSteps > 0)
    {
    float *ts = new float [nTimeSteps];
    if (use_floats)
      {
      ex_get_all_times(fid, ts);
      }
    else
      {
      double *dts = new double [nTimeSteps];
      ex_get_all_times(fid, dts);
      for (i=0; i<nTimeSteps; i++)
        {
        ts[i] = (float)dts[i];
        }
      delete [] dts;
      }

    emd->SetTimeSteps(nTimeSteps, ts);
    }

  // Block information

  int nblocks;
  int *bids = NULL;
  ex_inquire(fid, EX_INQ_ELEM_BLK, &nblocks, &floatVal, &charVal);
  emd->SetNumberOfBlocks(nblocks);

  if (nblocks > 0)
    {
    bids = new int [nblocks];

    ex_get_elem_blk_ids(fid, bids);

    char **types = new char * [nblocks];
    int *nodesPerElement = new int [nblocks];
    int *numAtt = new int [nblocks];

    for (i=0; i<nblocks; i++)
      {
      types[i] = new char [MAX_STR_LENGTH + 1];

      ex_get_elem_block(fid, bids[i], types[i],
             &intVal, &nodesPerElement[i], &numAtt[i]);
      }

    emd->SetBlockIds(bids);
    emd->SetBlockElementType(types);
    emd->SetBlockNodesPerElement(nodesPerElement);
    emd->SetBlockNumberOfAttributesPerElement(numAtt);
    }
  else
    {
    vtkWarningMacro(<<
      "ExodusModel finds no blocks.  We thought that couldn't happen");
    }

  // Node set and Side set global information

  int nnsets;
  int *nids = NULL;
  int nssets;
  int *sids = NULL;
  ex_inquire(fid, EX_INQ_NODE_SETS, &nnsets, &floatVal, &charVal);
  ex_inquire(fid, EX_INQ_SIDE_SETS, &nssets, &floatVal, &charVal);
  emd->SetNumberOfNodeSets(nnsets);
  emd->SetNumberOfSideSets(nssets);

  if (nnsets > 0)
    {
    nids = new int [nnsets];
    ex_get_node_set_ids(fid, nids);
    emd->SetNodeSetIds(nids);
    }

  if (nssets > 0)
    {
    sids = new int [nssets];
    ex_get_side_set_ids(fid, sids);
    emd->SetSideSetIds(sids);
    }

  // Block, Node set and Side set properties

  int nBlockProperties;
  ex_inquire(fid, EX_INQ_EB_PROP, &nBlockProperties, &floatVal, &charVal);

  if (nBlockProperties > 0)
    {
    char **names = new char * [nBlockProperties];

    for (i=0; i<nBlockProperties; i++)
      {
      names[i] = new char [MAX_STR_LENGTH + 1];
      }

    ex_get_prop_names(fid, EX_ELEM_BLOCK, names);

    int *val = new int [nBlockProperties * nblocks];
    int *v = val;

    for (i=0; i<nBlockProperties; i++)
      {
      ex_get_prop_array(fid, EX_ELEM_BLOCK, names[i], v);
      v += nblocks;
      }

    if (nBlockProperties > 0)
      {
      emd->SetBlockPropertyNames(nBlockProperties, names);
      emd->SetBlockPropertyValue(val);
      }
    else
      {
      delete [] names;
      delete [] val;
      }
    }

  int nNodeSetProperties;
  ex_inquire(fid, EX_INQ_NS_PROP, &nNodeSetProperties, &floatVal, &charVal);

  if (nNodeSetProperties > 0)
    {
    char **names = new char * [nNodeSetProperties];

    for (i=0; i<nNodeSetProperties; i++)
      {
      names[i] = new char [MAX_STR_LENGTH + 1];
      }

    ex_get_prop_names(fid, EX_NODE_SET, names);

    int *val = new int [nNodeSetProperties * nnsets];
    int *v = val;

    for (i=0; i<nNodeSetProperties; i++)
      {
      ex_get_prop_array(fid, EX_NODE_SET, names[i], v);
      v += nnsets;
      }

    if (nNodeSetProperties > 0)
      {
      emd->SetNodeSetPropertyNames(nNodeSetProperties, names);
      emd->SetNodeSetPropertyValue(val);
      }
    else
      {
      delete [] names;
      delete [] val;
      }
    }

  int nSideSetProperties;
  ex_inquire(fid, EX_INQ_SS_PROP, &nSideSetProperties, &floatVal, &charVal);

  if (nSideSetProperties > 0)
    {
    char **names = new char * [nSideSetProperties];

    for (i=0; i<nSideSetProperties; i++)
      {
      names[i] = new char [MAX_STR_LENGTH + 1];
      }

    ex_get_prop_names(fid, EX_SIDE_SET, names);

    int *val = new int [nSideSetProperties * nssets];
    int *v = val;

    for (i=0; i<nSideSetProperties; i++)
      {
      ex_get_prop_array(fid, EX_SIDE_SET, names[i], v);
      v += nssets;
      }

    if (nSideSetProperties > 0)
      {
      emd->SetSideSetPropertyNames(nSideSetProperties, names);
      emd->SetSideSetPropertyValue(val);
      }
    else
      {
      delete [] names;
      delete [] val;
      }
    }

  // Element variables and node variables:

  int nEltVars;
  int nNodeVars;

  ex_get_var_param(fid, "E", &nEltVars);
  ex_get_var_param(fid, "N", &nNodeVars);

  if (nEltVars > 0)
    {
    char **names = new char * [nEltVars];

    for (i=0; i<nEltVars; i++)
      {
      names[i] = new char [MAX_STR_LENGTH + 1];
      }

    ex_get_var_names(fid, "E", nEltVars, names);

    this->RemoveBeginningAndTrailingSpaces(names, nEltVars);

    emd->SetElementVariableInfo(nEltVars, names, 0, NULL, NULL, NULL);
    }

  if (nNodeVars > 0)
    {
    char **names = new char * [nNodeVars];

    for (i=0; i<nNodeVars; i++)
      {
      names[i] = new char [MAX_STR_LENGTH + 1];
      }

    ex_get_var_names(fid, "N", nNodeVars, names);

    this->RemoveBeginningAndTrailingSpaces(names, nNodeVars);

    emd->SetNodeVariableInfo(nNodeVars, names, 0, NULL, NULL, NULL);
    }

  // Block/element variable truth table, by block by element variable

  if ((nEltVars > 0) && (nblocks > 0))
    {
    int *tt = new int [nEltVars * nblocks];

    ex_get_elem_var_tab(fid, nblocks, nEltVars, tt);

    emd->SetElementVariableTruthTable(tt);
    }

  // Global variables

  int nvars = 0;
  ex_get_var_param(fid, "G", &nvars);

  if (nvars > 0)
    {
    char **nms = new char * [nvars];
    for (i=0; i<nvars; i++)
      {
      nms[i] = new char [MAX_STR_LENGTH + 1];
      }

    ex_get_var_names(fid, "G", nvars, nms);

    emd->SetGlobalVariableNames(nvars, nms);
    }

  ex_opts(EX_VERBOSE);    // turn error messages back on

  return 0;
}

void vtkExodusModel::CopyDoubleToFloat(float *f, double *d, int len)
{
  for (int i=0; i<len; i++)
    {
    f[i] = (float)d[i];
    }
}
// TODO - We should probably have an option to omit ghost cells
//   from the metadata.

int vtkExodusModel::SetLocalInformation(vtkUnstructuredGrid *ugrid,
                     int fid, int timeStep, int geoCount, int compute_word_size)
{
  vtkModelMetadata *emd = this->GetModelMetadata();
  int i;

  int newTimeStep = (timeStep != emd->GetTimeStepIndex());

  int newGeometry = (geoCount > this->GeometryCount);
  this->GeometryCount = geoCount;

  if (!newTimeStep && !newGeometry)
    {
    return 0;
    }

  ex_opts(0);    // turn off all error messages

  int use_floats = (compute_word_size == sizeof(float));

  if (newTimeStep)
    {
    emd->SetGlobalVariableValue(NULL);

    // GLOBAL VARIABLE VALUES AT THIS TIMESTEP

    int numGlobalVars = emd->GetNumberOfGlobalVariables();
    int ts = timeStep + 1;

    emd->SetTimeStepIndex(timeStep);

    if (numGlobalVars > 0)
      {
      float *varf = new float [numGlobalVars];

      if (use_floats)
        {
        ex_get_glob_vars(fid, ts, numGlobalVars, varf);
        }
      else
        {
        double *vard = new double [numGlobalVars];
        ex_get_glob_vars(fid, ts, numGlobalVars, vard);

        for (i=0; i<numGlobalVars; i++)
          {
          varf[i] = (float)vard[i];
          }
        delete [] vard;
        }

      emd->SetGlobalVariableValue(varf);
      }
    }

  if (ugrid->GetNumberOfCells() < 1)
    {
    return 0;
    }

  // Big assumptions - this vtkUnstructuredGrid was created with the
  //  vtkExodusReader.  If it contains any elements of a block, it
  //  contains all the elements, and they appear together and in the
  //  same order in the vtkUnstructuredGrid as they do in the Exodus file.
  //  The order of the blocks may be different in the vtkUnstructuredGrid
  //  than it is in the Exodus file.  The vtkUnstructuredGrid
  //  contains cell arrays called BlockId and
  //  GlobalElementId and a point array called GlobalNodeId.
  //
  // Another assumption is that the element number map in the Exodus file
  // matches the global element IDs in the vtkUnstructuredGrid, and the
  // node number map in the Exodus file matches the global node IDs in
  // the vtkUnstructuredGrid.  (That is, we are both using the same
  // global IDs to identify points and cells.)
  //
  // TODO - fix behavior on error

  // Check input

  int *blockIds = NULL;
  int *cellIds = NULL;
  int *pointIds = NULL;

  vtkDataArray *da = ugrid->GetCellData()->GetArray("BlockId");
  if (da)
    {
    vtkIntArray *ia = vtkIntArray::SafeDownCast(da);
    if (ia)
      {
      blockIds = ia->GetPointer(0);
      }
    }

  da = ugrid->GetCellData()->GetArray("GlobalElementId");
  if (da)
    {
    vtkIntArray *ia = vtkIntArray::SafeDownCast(da);
    if (ia)
      {
      cellIds = ia->GetPointer(0);
      }
    }

  da = ugrid->GetPointData()->GetArray("GlobalNodeId");
  if (da)
    {
    vtkIntArray *ia = vtkIntArray::SafeDownCast(da);
    if (ia)
      {
      pointIds = ia->GetPointer(0);
      }
    }

  if (!blockIds || !cellIds || !pointIds)
    {
    return 1;
    }

  int nblocks = emd->GetNumberOfBlocks();
  int ncells = ugrid->GetNumberOfCells();
  int npoints = ugrid->GetNumberOfPoints();
  if ((nblocks < 1) || (ncells < 1)) return 1;

  if (newGeometry)
    {
    // BLOCK, NODE SET, AND SIDE SET LISTS

    emd->FreeBlockDependentData();

    this->SetLocalBlockInformation(fid, use_floats, blockIds,
                                   cellIds, ncells);

    if (emd->GetNumberOfNodeSets() > 0)
      {
      this->SetLocalNodeSetInformation(fid, use_floats, pointIds, npoints);
      }

    if (emd->GetNumberOfSideSets() > 0)
      {
      this->SetLocalSideSetInformation(fid, use_floats, cellIds, ncells);
      }
    }

  ex_opts(EX_VERBOSE);    // turn error messages back on

  return 0;
}
int vtkExodusModel::SetLocalBlockInformation(
                int fid, int use_floats, int *blockIds , int *cellIds, int ncells)
{
  int i;

  vtkModelMetadata *emd = this->GetModelMetadata();

  int nblocks = emd->GetNumberOfBlocks();

  if (nblocks < 1)
    {
    return 0; // maybe this is really an error, I'm not sure
    }

  int *count = new int [nblocks];
  memset(count, 0, sizeof(int) * nblocks);
  int lastId = -1;
  int idx = 0;

  std::map<int,int> blockIdStart;
  std::map<int,int>::iterator it;

  for (i=0; i<ncells; i++)
    {
    int id = blockIds[i];

    if (id != lastId)
      {
      idx = emd->GetBlockLocalIndex(id);

      if ((idx < 0) || (count[idx] > 0))
        {
        // Bad block ID or elements are not in order by block

        delete [] count;
        return 1;
        }

      blockIdStart.insert(std::map<int,int>::value_type(idx, i));
      lastId = id;
      }

    count[idx]++;
    }

  int *GlobalBlockIds = emd->GetBlockIds();

  for (idx=0; idx<nblocks; idx++)
    {
    if (count[idx] == 0) continue;

    char type[MAX_STR_LENGTH+1];
    int numElem, numNodes, numAttr;

    ex_get_elem_block(fid, GlobalBlockIds[idx], type,
                      &numElem, &numNodes, &numAttr);

    if (numElem != count[idx])
      {
      // Ugrid does not contain all the elements for this block
      delete [] count;
      return 1;
      }
    }

  emd->SetBlockNumberOfElements(count);

  int *idList = new int [ncells];
  float *attsF = NULL;
  double *attsD = NULL;

  int natts = emd->GetSizeBlockAttributeArray();

  if (natts > 0)
    {
    attsF = new float [natts];

    if (!use_floats)
      {
      attsD = new double [natts];
      }
    }

  int *eltIdIdx = emd->GetBlockElementIdListIndex();
  int *attIdx = emd->GetBlockAttributesIndex();
  int *numAttsPerElement = emd->GetBlockNumberOfAttributesPerElement();

  for (idx=0; idx<nblocks; idx++)
    {
    if (count[idx] == 0) continue;

    int to = eltIdIdx[idx];

    it = blockIdStart.find(idx);
    int from = it->second;

    memcpy(idList + to,  cellIds + from, sizeof(int) * count[idx]);

    if (attsF == NULL) continue;

    if (numAttsPerElement[idx] == 0) continue;

    to = attIdx[idx];

    if (use_floats)
      {
      ex_get_elem_attr(fid, GlobalBlockIds[idx], attsF + to);
      }
    else
      {
      ex_get_elem_attr(fid, GlobalBlockIds[idx], attsD + to);
      }
    }

  blockIdStart.erase(blockIdStart.begin(), blockIdStart.end());

  emd->SetBlockElementIdList(idList);

  if (attsF)
    {
    if (!use_floats)
      {
      this->CopyDoubleToFloat(attsF, attsD, natts);
      delete [] attsD;
      }
    emd->SetBlockAttributes(attsF);
    }

  return 0;
}
int vtkExodusModel::SetLocalNodeSetInformation(
        int fid, int use_floats, int *pointIds, int npoints)
{
  int i, j;
  float dummyFloat;
  char dummyChar;

  vtkModelMetadata *emd = this->GetModelMetadata();

  // external node IDs in file

  int numNodesInFile = 0;
  ex_inquire(fid, EX_INQ_NODES, &numNodesInFile, &dummyFloat, &dummyChar);
  int *nodeMap = new int [numNodesInFile];

  ex_get_node_num_map(fid, nodeMap);
cerr << "node num map : ";
for (i = 0; i < numNodesInFile; i ++)
{
  cerr << nodeMap[i] << " ";
}
cerr << endl;

  // external node IDs in vtkUnstructuredGrid

  std::map<int, int> localNodeIdMap;
  std::map<int, int>::iterator it;

  for (i=0; i<npoints; i++)
    {
    localNodeIdMap.insert(std::map<int,int>::value_type(pointIds[i], i));
    }

  int nns = emd->GetNumberOfNodeSets();
  int *numDF = new int [nns];

  int *nssize = new int [nns];
  memset(nssize, 0, sizeof(int) * nns);

  vtkIntArray *nsNodeIds = vtkIntArray::New();
  nsNodeIds->SetNumberOfComponents(1);

  vtkFloatArray *nsDF = vtkFloatArray::New();
  nsDF->SetNumberOfComponents(1);

  int total = 0;

  int *nodeSetIds = emd->GetNodeSetIds();

  for (i=0; i<nns; i++)
    {
    int nnodes = 0;
    ex_get_node_set_param(fid, nodeSetIds[i], &nnodes, numDF+i);

    if (nnodes == 0) continue;

    int *nodes = new int [nnodes];

    ex_get_node_set(fid, nodeSetIds[i], nodes);

    float *dfF = NULL;
    double *dfD = NULL;

    if (numDF[i])
      {
      dfF = new float [nnodes];

      if (use_floats)
        {
        ex_get_node_set_dist_fact(fid, nodeSetIds[i], dfF);
        }
      else
        {
        dfD = new double [nnodes];
        ex_get_node_set_dist_fact(fid, nodeSetIds[i], dfD);
        this->CopyDoubleToFloat(dfF, dfD, nnodes);
        delete [] dfD;
        }
      }

    // find which of my points are in this node set

    for (j=0; j<nnodes; j++)
      {
      int lid = nodes[j] - 1;
      int gid = nodeMap[lid];

      it = localNodeIdMap.find(gid);

      if (it == localNodeIdMap.end()) continue;  // I don't have that one

      nsNodeIds->InsertNextValue(gid);

      if (dfF)
        {
        nsDF->InsertNextValue(dfF[j]);
        }

      nssize[i]++;
      total++;
      }

    delete [] nodes;
    if (dfF) delete [] dfF;
    }

  delete [] nodeMap;

  localNodeIdMap.erase(localNodeIdMap.begin(), localNodeIdMap.end());

  emd->SetNodeSetSize(nssize);

  if (total > 0)
    {
    int *nsndf = new int [nns];

    for (i=0; i<nns; i++)
      {
      if (numDF[i] > 0)
        {
        nsndf[i] = nssize[i];
        }
      else
        {
        nsndf[i] = 0;
        }
      }

    delete [] numDF;

    emd->SetNodeSetNumberOfDistributionFactors(nsndf);

    int *ids = new int [total];
    memcpy(ids, nsNodeIds->GetPointer(0), sizeof(int) * total);

    nsNodeIds->Delete();

    emd->SetNodeSetNodeIdList(ids);

    int sizeDF = nsDF->GetNumberOfTuples();

    if (sizeDF > 0)
      {
      float *df = new float [sizeDF];
      memcpy(df, nsDF->GetPointer(0), sizeof(float) * sizeDF);
      emd->SetNodeSetDistributionFactors(df);
      }
    nsDF->Delete();
    }
  else
    {
    delete [] numDF;
    nsNodeIds->Delete();
    nsDF->Delete();
    }

  return 0;
}
int vtkExodusModel::SetLocalSideSetInformation(
        int fid, int use_floats, int *cellIds, int ncells)
{
  // TODO - go over this and check it

  int i, j, k;
  float dummyFloat;
  char dummyChar;

  vtkModelMetadata *emd = this->GetModelMetadata();

  // external cell IDs in file

  int numCellsInFile = 0;
  ex_inquire(fid, EX_INQ_ELEM, &numCellsInFile, &dummyFloat, &dummyChar);
  int *cellMap = new int [numCellsInFile];

  ex_get_elem_num_map(fid, cellMap);

  // external cell IDs in vtkUnstructuredGrid

  std::map<int, int> localCellIdMap;
  std::map<int, int>::iterator it;

  for (i=0; i<ncells; i++)
    {
    localCellIdMap.insert(std::map<int,int>::value_type(cellIds[i], i));
    }

  int nss = emd->GetNumberOfSideSets();
  int *numDF = new int [nss];

  int *sssize = new int [nss];
  memset(sssize, 0, sizeof(int) * nss);

  vtkIntArray *ssCellIds = vtkIntArray::New();
  ssCellIds->SetNumberOfComponents(1);

  vtkIntArray *ssSideIds = vtkIntArray::New();
  ssSideIds->SetNumberOfComponents(1);

  vtkIntArray *ssDFPerSide = vtkIntArray::New();
  ssDFPerSide->SetNumberOfComponents(1);

  vtkFloatArray *ssDF = vtkFloatArray::New();
  ssDF->SetNumberOfComponents(1);

  int total = 0;

  int *sideSetIds = emd->GetSideSetIds();

  for (i=0; i<nss; i++)
    {
    int nsides = 0;
    ex_get_side_set_param(fid, sideSetIds[i], &nsides, numDF+i);

    if (nsides == 0) continue;

    int *elts = new int [nsides];
    int *sides = new int [nsides];

    ex_get_side_set(fid, sideSetIds[i], elts, sides);

    // find which of my cells have sides in this side set

    for (j=0; j<nsides; j++)
      {
      int lid = elts[j] - 1;
      int gid = cellMap[lid];

      it = localCellIdMap.find(gid);

      if (it == localCellIdMap.end())
        {
        elts[j] = -1;  // flag this one, I don't have it
        continue;
        }

      ssCellIds->InsertNextValue(gid);

      ssSideIds->InsertNextValue(sides[j]);

      sssize[i]++;
      total++;
      }

    delete [] sides;

    if (sssize[i] > 0)
      {
      if (numDF[i] > 0)
        {
        int *nodeCount = new int [nsides];
        int *nodeList = new int [numDF[i]];

        ex_get_side_set_node_list(fid, sideSetIds[i], nodeCount, nodeList);

        delete [] nodeList;

        float *dfF = NULL;
        double *dfD = NULL;
        int nextdf = 0;

        dfF = new float [numDF[i]];

        if (use_floats)
          {
          ex_get_side_set_dist_fact(fid, sideSetIds[i], dfF);
          }
        else
          {
          dfD = new double [numDF[i]];
          ex_get_side_set_dist_fact(fid, sideSetIds[i], dfD);
          this->CopyDoubleToFloat(dfF, dfD, numDF[i]);
          delete [] dfD;
          }

        for (j=0; j<nsides; j++)
          {
          if (elts[j] >= 0)
            {
            ssDFPerSide->InsertNextValue(nodeCount[j]);

            for (k=0; k < nodeCount[j]; k++)
              {
              ssDF->InsertNextValue(dfF[nextdf++]);
              }
            }
          else
            {
            nextdf += nodeCount[j];
            }
          }
        }
      else
        {
        for (j=0; j<sssize[i]; j++)
          {
          ssDFPerSide->InsertNextValue(0);
          }
        }
      }

    delete [] elts;
    }

  delete [] cellMap;

  localCellIdMap.erase(localCellIdMap.begin(), localCellIdMap.end());

  emd->SetSideSetSize(sssize);

  if (total == 0)
    {
    delete [] numDF;
    delete [] sssize;
    ssCellIds->Delete();
    ssSideIds->Delete();
    ssDFPerSide->Delete();
    ssDF->Delete();
    return 0;
    }

  int n = ssCellIds->GetNumberOfTuples();

  int *buf = new int [n];
  memcpy(buf, ssCellIds->GetPointer(0), sizeof(int) * n);
  ssCellIds->Delete();

  emd->SetSideSetElementList(buf);

  buf = new int [n];
  memcpy(buf, ssSideIds->GetPointer(0), sizeof(int) * n);
  ssSideIds->Delete();

  emd->SetSideSetSideList(buf);

  buf = new int [n];
  memcpy(buf, ssDFPerSide->GetPointer(0), sizeof(int) * n);
  ssDFPerSide->Delete();

  emd->SetSideSetNumDFPerSide(buf);

  int *nssdf = new int [nss];

  int *ndf = emd->GetSideSetNumDFPerSide();

  int totaldf = 0;

  for (i=0; i<nss; i++)
    {
    if ((numDF[i] > 0) && (sssize[i] > 0))
      {
      nssdf[i] = 0;

      for (j=0; j<sssize[i]; j++)
        {
        nssdf[i] += *ndf++;
        }

      totaldf += nssdf[i];
      }
    else
      {
      nssdf[i] = 0;

      ndf += sssize[i];
      }
    }

  delete [] numDF;

  emd->SetSideSetNumberOfDistributionFactors(nssdf);

  if (totaldf > 0)
    {
    float *df = new float [totaldf];
    memcpy(df, ssDF->GetPointer(0), sizeof(float) * totaldf);
    emd->SetSideSetDistributionFactors(df);
    }

  ssDF->Delete();

  return 0;
}
//-------------------------------------------------
// Merge an ExodusModel into this one
//-------------------------------------------------
int vtkExodusModel::MergeExodusModel(vtkExodusModel *em)
{
  vtkModelMetadata *myMmd = this->GetModelMetadata();

  vtkModelMetadata *newmd = em->GetModelMetadata();

  int fail = myMmd->MergeModelMetadata(newmd);

  return (fail != 0);
}
vtkExodusModel *vtkExodusModel::ExtractExodusModel(vtkIdTypeArray *globalCellIdList,
                                                   vtkUnstructuredGrid *grid)
{
  vtkExodusModel *em = vtkExodusModel::New();

  vtkModelMetadata *mmd = this->GetModelMetadata()->ExtractModelMetadata(
    globalCellIdList, grid);

  if (mmd == NULL)
    {
    em->Delete();
    em = NULL;
    }
  else
    {
    em->SetModelMetadata(mmd);
    }

  return em;
}
//-------------------------------------------------
// Element variables
//-------------------------------------------------
int vtkExodusModel::AddUGridElementVariable(char *ugridVarName,
                                    char *origName, int numComponents)
{
  vtkModelMetadata *emd = this->GetModelMetadata();

  int rc = emd->AddUGridElementVariable(ugridVarName,
                origName, numComponents);

  return rc;
}
int vtkExodusModel::RemoveUGridElementVariable(char *ugridVarName)
{
  vtkModelMetadata *emd = this->GetModelMetadata();

  int rc = emd->RemoveUGridElementVariable(ugridVarName);

  return rc;
}
void vtkExodusModel::SetElementVariableInfo(int numOrigNames, char **origNames,
            int numNames, char **names,  int *numComp, int *map)
{
  vtkModelMetadata *emd = this->GetModelMetadata();

  emd->SetElementVariableInfo(numOrigNames, origNames,
                     numNames, names, numComp, map);
}

//-------------------------------------------------
// Node variables
//-------------------------------------------------
int vtkExodusModel::AddUGridNodeVariable(char *ugridVarName,
                                    char *origName, int numComponents)
{
  vtkModelMetadata *emd = this->GetModelMetadata();

  int rc = emd->AddUGridNodeVariable(ugridVarName,
                origName, numComponents);

  return rc;
}
int vtkExodusModel::RemoveUGridNodeVariable(char *ugridVarName)
{
  vtkModelMetadata *emd = this->GetModelMetadata();

  int rc = emd->RemoveUGridNodeVariable(ugridVarName);

  return rc;
}
void vtkExodusModel::SetNodeVariableInfo(int numOrigNames, char **origNames,
            int numNames, char **names,  int *numComp, int *map)
{
  vtkModelMetadata *emd = this->GetModelMetadata();

  emd->SetNodeVariableInfo(numOrigNames, origNames,
                     numNames, names, numComp, map);
}
void vtkExodusModel::RemoveBeginningAndTrailingSpaces(char **names, int len)
{
  size_t i, j;

  for (i=0; i<static_cast<size_t>(len); i++)
    {
    char *c = names[i];
    size_t nmlen = strlen(c);

    char *cbegin = c;
    char *cend = c + nmlen - 1;

    // remove spaces or non-printing character from start and end

    for (j=0; j<nmlen; j++)
      {
      if (!isgraph(*cbegin)) cbegin++;
      else break;
      }

    for (j=0; j<nmlen; j++)
      {
      if (!isgraph(*cend)) cend--;
      else break;
      }

    if (cend < cbegin)
      {
      sprintf(names[i], "null_%u", static_cast<unsigned int>(i));
      continue;
      }

    size_t newlen = cend - cbegin + 1;

    if (newlen < nmlen)
      {
      for (j=0; j<newlen; j++)
        {
        *c++ = *cbegin++;
        }
      *c = '\0';
      }
    }
}

//-------------------------------------------------
//-------------------------------------------------

void vtkExodusModel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ModelMetadata: " << this->ModelMetadata << endl;
  os << indent << "GeometryCount: " << this->GeometryCount << endl;
}
