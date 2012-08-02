/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Format              */
/*                                                                 */
/*  Id : Id  */
/*  Date : $Date$ */
/*  Version : $Revision$ */
/*                                                                 */
/*  Author:                                                        */
/*     Kenneth Leiter                                              */
/*     kenneth.leiter@arl.army.mil                                 */
/*     US Army Research Laboratory                                 */
/*     Aberdeen Proving Ground, MD                                 */
/*                                                                 */
/*     Copyright @ 2010 US Army Research Laboratory                */
/*     All Rights Reserved                                         */
/*     See Copyright.txt or http://www.arl.hpc.mil/ice for details */
/*                                                                 */
/*     This software is distributed WITHOUT ANY WARRANTY; without  */
/*     even the implied warranty of MERCHANTABILITY or FITNESS     */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice   */
/*     for more information.                                       */
/*                                                                 */
/*******************************************************************/

#include "XdmfPartitioner.h"

#include <sstream>

#ifndef BUILD_EXE

extern "C"
{
#include <metis.h>
}

#include <map>
#include <vector>

XdmfPartitioner::XdmfPartitioner()
{
}

XdmfPartitioner::~XdmfPartitioner()
{
}

XdmfGrid * XdmfPartitioner::Partition(XdmfGrid * grid, int numPartitions, XdmfElement * parentElement)
{
  int metisElementType;  
  int numElementsPerNode;
  switch(grid->GetTopology()->GetTopologyType())
  {
    case(XDMF_TRI):
    case(XDMF_TRI_6):
      metisElementType = 1;
      numElementsPerNode = 3;
      break;
    case(XDMF_QUAD):
    case(XDMF_QUAD_8):
    case(XDMF_QUAD_9):
      metisElementType = 4;
      numElementsPerNode = 4;
      break;
    case(XDMF_TET):
    case(XDMF_TET_10):
      metisElementType = 2;
      numElementsPerNode = 4;
      break;
    case(XDMF_HEX):
    case(XDMF_HEX_20):
    case(XDMF_HEX_24):
    case(XDMF_HEX_27):
      metisElementType = 3;
      numElementsPerNode = 8;
      break;
    default:
      std::cout << "Cannot partition grid with element type: " << grid->GetTopology()->GetTopologyTypeAsString() << std::endl;
      return NULL;
  }

  int numNodes = grid->GetGeometry()->GetNumberOfPoints() / (grid->GetTopology()->GetNodesPerElement() / numElementsPerNode);
  int numElements = grid->GetTopology()->GetNumberOfElements();

  idxtype * metisConnectivity = new idxtype[numElementsPerNode * numElements];
  for(int i=0; i<numElements; ++i)
  {
    grid->GetTopology()->GetConnectivity()->GetValues(i*grid->GetTopology()->GetNodesPerElement(), &metisConnectivity[i*numElementsPerNode], numElementsPerNode); 
  }

  // Need to remap connectivity for quadratic elements so that metis handles it properly
  std::map<idxtype, idxtype> xdmfConnIdxToMetisConnIdx;
  if(numNodes != grid->GetGeometry()->GetNumberOfPoints())
  {
    int index = 0;
    for(int i=0; i<numElements * numElementsPerNode; ++i)
    {
      std::map<idxtype, idxtype>::const_iterator val = xdmfConnIdxToMetisConnIdx.find(metisConnectivity[i]);
      if(val != xdmfConnIdxToMetisConnIdx.end())
      {
        metisConnectivity[i] = val->second;
      }
      else  
      {
        // Haven't seen this id before, map to index and set to new id
        xdmfConnIdxToMetisConnIdx[metisConnectivity[i]] = index;
        metisConnectivity[i] = index;
        index++;
      }
    }
  }
 
  int startIndex = 0;
  int numCutEdges = 0;

  idxtype * elementsPartition = new idxtype[numElements];
  idxtype * nodesPartition = new idxtype[numNodes];

  //METIS_PartMeshDual(&numElements, &numNodes, metisConnectivity, &metisElementType, &startIndex, &numPartitions, &numCutEdges, elementsPartition, nodesPartition);
  std::cout << "Entered METIS" << std::endl;
  METIS_PartMeshNodal(&numElements, &numNodes, metisConnectivity, &metisElementType, &startIndex, &numPartitions, &numCutEdges, elementsPartition, nodesPartition);
  std::cout << "Exited METIS" << std::endl;

  delete [] metisConnectivity;
  delete [] nodesPartition;

  // Initialize sets to hold globalNodeIds for each partition.
  std::vector<std::map<XdmfInt32, XdmfInt32> > globalToLocalNodeIdMap;
  std::vector<std::map<XdmfInt32, XdmfInt32> > globalToLocalElementIdMap;
  for(int i=0; i<numPartitions; ++i)
  {
    std::map<XdmfInt32, XdmfInt32> nodeMap;
    globalToLocalNodeIdMap.push_back(nodeMap);
    std::map<XdmfInt32, XdmfInt32> elemMap;
    globalToLocalElementIdMap.push_back(elemMap);
  }

  // Fill in globalNodeId for each partition
  XdmfInt32 * conn = new XdmfInt32[numElements * grid->GetTopology()->GetNodesPerElement()];
  grid->GetTopology()->GetConnectivity()->GetValues(0, conn, numElements * grid->GetTopology()->GetNodesPerElement());
  int totalIndex = 0;
  for(int i=0; i<numElements; ++i)
  {
    for(int j=0; j<grid->GetTopology()->GetNodesPerElement(); ++j)
    {
      if(globalToLocalNodeIdMap[elementsPartition[i]].count(conn[totalIndex]) == 0)
      {
        // Have not seen this node, need to add to map
        globalToLocalNodeIdMap[elementsPartition[i]][conn[totalIndex]] = globalToLocalNodeIdMap[elementsPartition[i]].size() - 1;
      }
      totalIndex++;
    }
    globalToLocalElementIdMap[elementsPartition[i]][i] = globalToLocalElementIdMap[elementsPartition[i]].size() - 1;
  }

  delete [] elementsPartition;

  XdmfGrid * collection = new XdmfGrid();
  collection->SetName("Collection");
  collection->SetGridType(XDMF_GRID_COLLECTION);
  collection->SetCollectionType(XDMF_GRID_COLLECTION_SPATIAL);
  collection->SetDeleteOnGridDelete(true);

  parentElement->Insert(collection);

  for(int i=0; i<numPartitions; ++i)
  {
    std::map<XdmfInt32, XdmfInt32> currNodeMap = globalToLocalNodeIdMap[i];
    std::map<XdmfInt32, XdmfInt32> currElemMap = globalToLocalElementIdMap[i];

    if(currNodeMap.size() > 0)
    {
      std::stringstream name;
      name << grid->GetName() << "_" << i;
    
      XdmfGrid * partition = new XdmfGrid();
      partition->SetName(name.str().c_str());

      int numDimensions = 3;
      if(grid->GetGeometry()->GetGeometryType() == XDMF_GEOMETRY_XY || grid->GetGeometry()->GetGeometryType() == XDMF_GEOMETRY_X_Y)
      {
        numDimensions = 2;
      }

      XdmfGeometry * geom = partition->GetGeometry();
      geom->SetGeometryType(grid->GetGeometry()->GetGeometryType());
      geom->SetNumberOfPoints(currNodeMap.size());
      geom->SetDeleteOnGridDelete(true);
    
      XdmfArray * points = geom->GetPoints();
      points->SetNumberType(grid->GetGeometry()->GetPoints()->GetNumberType());
      points->SetNumberOfElements(currNodeMap.size() * numDimensions);
      for(std::map<XdmfInt32, XdmfInt32>::const_iterator iter = currNodeMap.begin(); iter != currNodeMap.end(); ++iter)
      {
        points->SetValues(iter->second * numDimensions, grid->GetGeometry()->GetPoints(), numDimensions, iter->first * 3);
      }
 
      XdmfTopology * top = partition->GetTopology();
      top->SetTopologyType(grid->GetTopology()->GetTopologyType());
      top->SetNumberOfElements(currElemMap.size());
      top->SetDeleteOnGridDelete(true);
    
      XdmfArray * connections = top->GetConnectivity();
      connections->SetNumberType(grid->GetTopology()->GetConnectivity()->GetNumberType());
      connections->SetNumberOfElements(currElemMap.size() * grid->GetTopology()->GetNodesPerElement());
      XdmfInt32 * tmpConn = new XdmfInt32[grid->GetTopology()->GetNodesPerElement()];
      for(std::map<XdmfInt32, XdmfInt32>::const_iterator iter = currElemMap.begin(); iter != currElemMap.end(); ++iter)
      {
        // Get global connectivity values for this element
        grid->GetTopology()->GetConnectivity()->GetValues(iter->first * grid->GetTopology()->GetNodesPerElement(), tmpConn, grid->GetTopology()->GetNodesPerElement());
        // Translate these global points to local node ids
        for(int j=0; j<grid->GetTopology()->GetNodesPerElement(); ++j)
        {
          tmpConn[j] = currNodeMap[tmpConn[j]];
        }
        connections->SetValues(iter->second * grid->GetTopology()->GetNodesPerElement(), tmpConn, grid->GetTopology()->GetNodesPerElement());
      }
      delete [] tmpConn;
      collection->Insert(partition);

      // Add GlobalNodeId Attribute
      XdmfAttribute * globalIds = new XdmfAttribute();
      globalIds->SetName("GlobalNodeId");
      globalIds->SetAttributeType(XDMF_ATTRIBUTE_TYPE_SCALAR);
      globalIds->SetAttributeCenter(XDMF_ATTRIBUTE_CENTER_NODE);
      globalIds->SetDeleteOnGridDelete(true);

      XdmfArray * globalNodeIdVals = globalIds->GetValues();
      globalNodeIdVals->SetNumberType(XDMF_INT32_TYPE);
      globalNodeIdVals->SetNumberOfElements(currNodeMap.size());
      for(std::map<XdmfInt32, XdmfInt32>::const_iterator iter = currNodeMap.begin(); iter != currNodeMap.end(); ++iter)
      {
        globalNodeIdVals->SetValues(iter->second, (XdmfInt32*)&iter->first, 1);
      }
      partition->Insert(globalIds);
 
      // Split attributes and add to grid
      for(int j=0; j<grid->GetNumberOfAttributes(); ++j)
      {
        XdmfAttribute * currAttribute = grid->GetAttribute(j);
        currAttribute->Update();
        switch(currAttribute->GetAttributeCenter())
        {
          case(XDMF_ATTRIBUTE_CENTER_GRID):
          {
            // Will continue to be true for entire collection - so insert at top level
            collection->Insert(currAttribute);
            break;
          }
          case(XDMF_ATTRIBUTE_CENTER_CELL):
          case(XDMF_ATTRIBUTE_CENTER_FACE):
          case(XDMF_ATTRIBUTE_CENTER_EDGE):
          {
            XdmfAttribute * attribute = new XdmfAttribute();
            attribute->SetName(currAttribute->GetName());
            attribute->SetAttributeType(currAttribute->GetAttributeType());
            attribute->SetAttributeCenter(currAttribute->GetAttributeCenter());
            attribute->SetDeleteOnGridDelete(true);
            XdmfArray * attributeVals = attribute->GetValues();
            attributeVals->SetNumberType(currAttribute->GetValues()->GetNumberType());
            int numValsPerComponent = currAttribute->GetValues()->GetNumberOfElements() / grid->GetTopology()->GetNumberOfElements();
            attributeVals->SetNumberOfElements(currElemMap.size() * numValsPerComponent);
            for(std::map<XdmfInt32, XdmfInt32>::const_iterator iter = currElemMap.begin(); iter != currElemMap.end(); ++iter)
            {
              attributeVals->SetValues(iter->second * numValsPerComponent, currAttribute->GetValues(), numValsPerComponent, iter->first * numValsPerComponent);
            }
            partition->Insert(attribute);
            break;
          }
          case(XDMF_ATTRIBUTE_CENTER_NODE):
          {
            XdmfAttribute * attribute = new XdmfAttribute();
            attribute->SetName(currAttribute->GetName());
            attribute->SetAttributeType(currAttribute->GetAttributeType());
            attribute->SetAttributeCenter(currAttribute->GetAttributeCenter());
            attribute->SetDeleteOnGridDelete(true);
            XdmfArray * attributeVals = attribute->GetValues();
            attributeVals->SetNumberType(currAttribute->GetValues()->GetNumberType());
            attributeVals->SetNumberOfElements(currNodeMap.size());
            for(std::map<XdmfInt32, XdmfInt32>::const_iterator iter = currNodeMap.begin(); iter != currNodeMap.end(); ++iter)
            {
              attributeVals->SetValues(iter->second, currAttribute->GetValues(), 1, iter->first);
            }
            partition->Insert(attribute);
            break;
          }
          default:
          {
            std::cout << "Unknown attribute center encountered: " << currAttribute->GetAttributeCenterAsString() << std::endl;
            break;
          }
        }
      }

      // Split sets and add to grid
      for(int j=0; j<grid->GetNumberOfSets(); ++j)
      {
        XdmfSet * currSet = grid->GetSets(j);
        currSet->Update();
        switch(currSet->GetSetType())
        {
          case(XDMF_SET_TYPE_CELL):
          case(XDMF_SET_TYPE_FACE):
          case(XDMF_SET_TYPE_EDGE):
          {
            std::vector<XdmfInt32> myIds;
            for(int j=0; j<currSet->GetIds()->GetNumberOfElements(); j++)
            {
              std::map<XdmfInt32, XdmfInt32>::const_iterator val = currElemMap.find(currSet->GetIds()->GetValueAsInt32(j));
              if(val != currNodeMap.end())
              {
                myIds.push_back(val->second);
              }
            }
            if(myIds.size() != 0)
            {
              XdmfSet * set = new XdmfSet();
              set->SetName(currSet->GetName());
              set->SetSetType(currSet->GetSetType());
              set->SetSize(myIds.size());
              set->SetDeleteOnGridDelete(true);
              XdmfArray * ids = set->GetIds();
              ids->SetNumberType(XDMF_INT32_TYPE);
              ids->SetNumberOfElements(myIds.size());
              ids->SetValues(0, &myIds[0], myIds.size());
              partition->Insert(set);
            }
            break;
          }
          case(XDMF_SET_TYPE_NODE):
          {
            std::vector<XdmfInt32> myIds;
            for(int j=0; j<currSet->GetIds()->GetNumberOfElements(); j++)
            {
              std::map<XdmfInt32, XdmfInt32>::const_iterator val = currNodeMap.find(currSet->GetIds()->GetValueAsInt32(j));
              if(val != currNodeMap.end())
              {
                myIds.push_back(val->second);
              }
            }
            if(myIds.size() != 0)
            {
              XdmfSet * set = new XdmfSet();
              set->SetName(currSet->GetName());
              set->SetSetType(currSet->GetSetType());
              set->SetSize(myIds.size());
              set->SetDeleteOnGridDelete(true);
              XdmfArray * ids = set->GetIds();
              ids->SetNumberType(XDMF_INT32_TYPE);
              ids->SetNumberOfElements(myIds.size());
              ids->SetValues(0, &myIds[0], myIds.size());
              partition->Insert(set);
            }
            break;
          }
          default:
          {
            std::cout << "Unknown set type encountered: " << currSet->GetSetTypeAsString() << std::endl;  
            break;
          }
        }
      }
      std::cout << "HERE" << std::endl;
    }
  }

  return collection;
}

#else

/**
 * XdmfPartitioner is a command line utility for partitioning Xdmf grids.
 * The XdmfPartitioner uses the metis library to partition Triangular, Quadrilateral, Tetrahedral,
 * and Hexahedral XdmfGrids.
 *
 * Usage:
 *     XdmfPartitioner <path-of-file-to-partition> <num-partitions> (Optional: <path-to-output-file>)
 *
 */
int main(int argc, char* argv[])
{
  std::string usage = "Partitions an XDMF grid using the metis library: \n \n Usage: \n \n   XdmfPartitioner <path-of-file-to-partition> <num-partitions> (Optional: <path-to-output-file>)";
  std::string meshName = "";

  if (argc < 3)
  {
    cout << usage << endl;
    return 1;
  }

  FILE * refFile = fopen(argv[1], "r");
  if (refFile)
  {
    // Success
    meshName = argv[1];
    fclose(refFile);
  }
  else
  {
    cout << "Cannot open file: " << argv[1] << endl;
    return 1;
  }

  int numPartitions = atoi(argv[2]);

  if (argc >= 4)
  {
    meshName = argv[3];
  }
  
  if(meshName.find_last_of("/\\") != std::string::npos)
  {
    meshName = meshName.substr(meshName.find_last_of("/\\")+1, meshName.length());
  }

  if (meshName.rfind(".") != std::string::npos)
  {
    meshName = meshName.substr(0, meshName.rfind("."));
  }

  if(argc < 4)
  {
    meshName = meshName + "-partitioned";
  }
  
  XdmfDOM dom;
  XdmfInt32 error = dom.Parse(argv[1]);
  if(error == XDMF_FAIL)
  {
    std::cout << "File does not appear to be a valid Xdmf file" << std::endl;
    return 1;
  }
  XdmfXmlNode gridElement = dom.FindElementByPath("/Xdmf/Domain/Grid");
  if(gridElement == NULL)
  {
    std::cout << "Cannot parse Xdmf file!" << std::endl;
    return 1;
  }

  XdmfGrid * grid = new XdmfGrid();
  grid->SetDOM(&dom);
  grid->SetElement(gridElement);
  grid->Update();

  XdmfDOM newDOM;
  XdmfRoot newRoot;
  XdmfDomain newDomain;

  newRoot.SetDOM(&newDOM);
  newRoot.Build();
  newRoot.Insert(&newDomain);

  XdmfPartitioner partitioner;
  XdmfGrid * partitionedGrid = partitioner.Partition(grid, numPartitions, &newDomain);

  for(int i=0; i<partitionedGrid->GetNumberOfChildren(); ++i)
  {
    XdmfGrid * child = partitionedGrid->GetChild(i);
    
    // Set heavy data set names for geometry and topology
    std::stringstream heavyPointName;
    heavyPointName << meshName << ".h5:/" << child->GetName() << "/XYZ";
    child->GetGeometry()->GetPoints()->SetHeavyDataSetName(heavyPointName.str().c_str());

    std::stringstream heavyConnName;
    heavyConnName << meshName << ".h5:/" << child->GetName() << "/Connections";
    child->GetTopology()->GetConnectivity()->SetHeavyDataSetName(heavyConnName.str().c_str());

    // Set heavy data set names for mesh attributes and sets
    for(int i=0; i<child->GetNumberOfAttributes(); i++)
    {
      std::stringstream heavyAttrName;
      heavyAttrName << meshName << ".h5:/" << child->GetName() << "/Attribute/" << child->GetAttribute(i)->GetAttributeCenterAsString() << "/" << child->GetAttribute(i)->GetName();
      child->GetAttribute(i)->GetValues()->SetHeavyDataSetName(heavyAttrName.str().c_str());
    }

    for(int i=0; i<child->GetNumberOfSets(); i++)
    {
      std::stringstream heavySetName;
      heavySetName << meshName << ".h5:/" << child->GetName() << "/Set/" << child->GetSets(i)->GetSetTypeAsString() << "/" << child->GetSets(i)->GetName();
      child->GetSets(i)->GetIds()->SetHeavyDataSetName(heavySetName.str().c_str());
    }
  }

  partitionedGrid->Build();
 
  std::stringstream outputFileName;
  outputFileName << meshName << ".xmf";
  
  newDOM.Write(outputFileName.str().c_str());
  
  std::cout << "Wrote: " << outputFileName.str().c_str() << std::endl;
}

#endif //BUILD_EXE
