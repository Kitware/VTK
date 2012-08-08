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
/*     Copyright @ 2009 US Army Research Laboratory                */
/*     All Rights Reserved                                         */
/*     See Copyright.txt or http://www.arl.hpc.mil/ice for details */
/*                                                                 */
/*     This software is distributed WITHOUT ANY WARRANTY; without  */
/*     even the implied warranty of MERCHANTABILITY or FITNESS     */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice   */
/*     for more information.                                       */
/*                                                                 */
/*******************************************************************/

#include "XdmfExodusWriter.h"

#include <exodusII.h>
#include <sstream>
#include <string>
#include <vector>

class XdmfExodusWriterNameHandler
{
  public:
    // Helper function to construct attribute names for attributes with > 1 component since exodus
    // cannot store vectors.  Also deals with MAX_STR_LENGTH limitation in exodus.
    void ConstructAttributeName(const char * attributeName, std::vector<std::string>& names, int numComponents)
    {
      std::string name = attributeName;
      if(numComponents == 1)
      {
        if(name.length() > MAX_STR_LENGTH)
        {
          name = name.substr(0, MAX_STR_LENGTH);
        }
        names.push_back(name);
      }
      else if(numComponents > 1)
      {
        int numComponentDigits = int(numComponents / 10);
        if(name.length() + numComponentDigits > MAX_STR_LENGTH)
        {
          name = name.substr(0, MAX_STR_LENGTH - numComponentDigits);
        }
        for(int j=0; j<numComponents; ++j)
        {
          std::stringstream toAdd;
          toAdd << name << "-" << j+1;
          names.push_back(toAdd.str());
        }
      }
    }
};

//
// Construct XdmfExodusWriter.
//
XdmfExodusWriter::XdmfExodusWriter()
{
  nameHandler = new XdmfExodusWriterNameHandler();
  return;
}

//
// Destroy XdmfExodusWriter
//
XdmfExodusWriter::~XdmfExodusWriter()
{
  delete nameHandler;
  return;
}

// Take Xdmf TopologyType and return Exodus Topology Type
std::string XdmfExodusWriter::DetermineExodusCellType(XdmfInt32 xdmfElementType)
{
  switch(xdmfElementType)
  {
    case(XDMF_POLYVERTEX):
    {
      return "SUP";
    }
    case(XDMF_TRI):
    {
      return "TRIANGLE";
    }
    case(XDMF_QUAD):
    {
      return "QUAD";
    }
    case(XDMF_TET):
    {
      return "TETRA";
    }
    case(XDMF_PYRAMID):
    {
      return "PYRAMID";
    }
    case(XDMF_WEDGE):
    {
      return "WEDGE";
    }
    case(XDMF_HEX):
    {
      return "HEX";
    }
    case(XDMF_EDGE_3):
    {
      return "EDGE";
    }
    case(XDMF_TRI_6):
    {
      return "TRIANGLE";
    }
    case(XDMF_QUAD_8):
    {
      return "QUAD";
    }
    case(XDMF_QUAD_9):
    {
      return "QUAD";
    }
    case(XDMF_TET_10):
    {
      return "TETRA";
    }
    case(XDMF_WEDGE_15):
    {
      return "WEDGE";
    }
    case(XDMF_HEX_20):
    {
      return "HEX";
    }
    case(XDMF_HEX_27):
    {
      return "HEX";
    }
  }
  return "";
}

//
// Write contents of the XdmfGrid to exodus file.
//
void XdmfExodusWriter::write(const char * fileName, XdmfGrid * gridToWrite)
{
  // Open Exodus File
  int wordSize = 8;
  int storeSize = 8;
  int exodusHandle = ex_create(fileName, EX_CLOBBER, &wordSize, &storeSize);

  // Initialize Exodus File
  std::string title = gridToWrite->GetName();
  if(title.length() > MAX_STR_LENGTH)
  {
    title = title.substr(0, MAX_STR_LENGTH);
  }
  int num_dim;

  switch(gridToWrite->GetGeometry()->GetGeometryType())
  {
    case(XDMF_GEOMETRY_XYZ):
      num_dim = 3;
      break;
    case(XDMF_GEOMETRY_XY):
      num_dim = 2;
      break;
    case(XDMF_GEOMETRY_X_Y_Z):
      num_dim = 3;
      break;
    case(XDMF_GEOMETRY_X_Y):
      num_dim = 2;
      break;
    default:
      std::cout << "Cannot write grid with geometry " << gridToWrite->GetGeometry()->GetGeometryTypeAsString() << " to exodus file." << std::endl;
      return;
  }

  int num_nodes = gridToWrite->GetGeometry()->GetNumberOfPoints();
  int num_elem = gridToWrite->GetTopology()->GetNumberOfElements();
  int num_elem_blk = 1;
  int num_node_sets = 0;
  int num_side_sets = 0;

  for (int i=0; i<gridToWrite->GetNumberOfSets(); ++i)
  {
    switch(gridToWrite->GetSets(i)->GetSetType())
    {
      case(XDMF_SET_TYPE_CELL):
      {
        num_side_sets++;
        break;
      }
      case(XDMF_SET_TYPE_NODE):
      {
        num_node_sets++;
        break;
      }
    }
  }

  ex_put_init(exodusHandle, title.c_str(), num_dim, num_nodes, num_elem, num_elem_blk, num_node_sets, num_side_sets);

  // Write nodal coordinate values to exodus
  double * x = new double[num_nodes];
  double * y = new double[num_nodes];
  double * z = new double[num_nodes];
  if(gridToWrite->GetGeometry()->GetGeometryType() == XDMF_GEOMETRY_XYZ || gridToWrite->GetGeometry()->GetGeometryType() == XDMF_GEOMETRY_XY)
  {
    gridToWrite->GetGeometry()->GetPoints()->GetValues(0, x, num_nodes, 3);
    gridToWrite->GetGeometry()->GetPoints()->GetValues(1, y, num_nodes, 3);
    if(gridToWrite->GetGeometry()->GetGeometryType() == XDMF_GEOMETRY_XYZ)
    {
      gridToWrite->GetGeometry()->GetPoints()->GetValues(2, z, num_nodes, 3);
    }
  }
  else if(gridToWrite->GetGeometry()->GetGeometryType() == XDMF_GEOMETRY_X_Y_Z || gridToWrite->GetGeometry()->GetGeometryType() == XDMF_GEOMETRY_X_Y)
  {
    gridToWrite->GetGeometry()->GetPoints()->GetValues(0, x, num_nodes);
    gridToWrite->GetGeometry()->GetPoints()->GetValues(num_nodes, y, num_nodes);
    if(gridToWrite->GetGeometry()->GetGeometryType() == XDMF_GEOMETRY_X_Y_Z)
    {
      gridToWrite->GetGeometry()->GetPoints()->GetValues(num_nodes*2, z, num_nodes);
    }
  }

  ex_put_coord(exodusHandle, x ,y ,z);
  delete [] x;
  delete [] y;
  delete [] z;

  // Write Element block parameters
  XdmfInt32 topType = gridToWrite->GetTopology()->GetTopologyType();
  std::string cellType = this->DetermineExodusCellType(topType);
  if (cellType.compare("") == 0)
  {
    std::cout << "Cannot write grid with topology " << gridToWrite->GetTopology()->GetTopologyTypeAsString() << std::endl;
    return;
  }
  ex_put_elem_block(exodusHandle, 10, cellType.c_str(), num_elem, gridToWrite->GetTopology()->GetNodesPerElement(), num_side_sets);

  // Write Element Connectivity
  int * elem_connectivity = new int[num_elem * gridToWrite->GetTopology()->GetNodesPerElement()];
  // Add 1 to connectivity array since exodus indices start at 1
  *gridToWrite->GetTopology()->GetConnectivity() + 1;
  gridToWrite->GetTopology()->GetConnectivity()->GetValues(0, elem_connectivity, num_elem * gridToWrite->GetTopology()->GetNodesPerElement());

  if(topType == XDMF_HEX_20 || topType == XDMF_HEX_27)
  {
    int * ptr = elem_connectivity;
    int k;
    int itmp[4];

    // Exodus Node ordering does not match Xdmf, we must convert.
    for (int i=0; i<num_elem; i++)
    {
      ptr += 12;

      for ( k = 0; k < 4; ++k, ++ptr)
      {
        itmp[k] = *ptr;
        *ptr = ptr[4];
      }

      for ( k = 0; k < 4; ++k, ++ptr )
      {
        *ptr = itmp[k];
      }

      if(topType == XDMF_HEX_27)
      {
        itmp[0] = *ptr;
        *(ptr++) = ptr[6];
        itmp[1] = *ptr;
        *(ptr++) = ptr[3];
        itmp[2] = *ptr;
        *(ptr++) = ptr[3];
        itmp[3] = *ptr;
        for ( k = 0; k < 4; ++k, ++ptr )
        {
          *ptr = itmp[k];
        }
      }
    }
  }
  else if (topType == XDMF_WEDGE_15 || topType == XDMF_WEDGE_18)
  {
    int * ptr = elem_connectivity;
    int k;
    int itmp[3];

    // Exodus Node ordering does not match Xdmf, we must convert.
    for (int i=0; i<num_elem; i++)
    {
      ptr += 9;

      for (k = 0; k < 3; ++k, ++ptr)
      {
        itmp[k] = *ptr;
        *ptr = ptr[3];
      }

      for (k = 0; k < 3; ++k, ++ptr)
      {
        *ptr = itmp[k];
      }

      if(topType == XDMF_WEDGE_18)
      {
        itmp[0] = *(ptr);
        itmp[1] = *(ptr+1);
        itmp[2] = *(ptr+2);
        *(ptr++) = itmp[2];
        *(ptr++) = itmp[0];
        *(ptr++) = itmp[1];
      }
    }
  }

  ex_put_elem_conn(exodusHandle, 10, elem_connectivity);
  delete [] elem_connectivity;

  // Write Attributes
  int numGlobalAttributes = 0;
  int numNodalAttributes = 0;
  int numElementAttributes = 0;

  std::vector<int> globalComponents;
  std::vector<int> nodalComponents;
  std::vector<int> elementComponents;
  std::vector<std::string> globalAttributeNames;
  std::vector<std::string> nodalAttributeNames;
  std::vector<std::string> elementAttributeNames;

  for(int i=0; i<gridToWrite->GetNumberOfAttributes(); ++i)
  {
    XdmfAttribute * currAttribute = gridToWrite->GetAttribute(i);
    currAttribute->Update();
    int numComponents = 0;
    switch(currAttribute->GetAttributeCenter())
    {
      case(XDMF_ATTRIBUTE_CENTER_GRID):
      {
        numComponents = currAttribute->GetValues()->GetNumberOfElements();
        globalComponents.push_back(numComponents);
        numGlobalAttributes += numComponents;
        nameHandler->ConstructAttributeName(currAttribute->GetName(), globalAttributeNames, numComponents);
        break;
      }
      case(XDMF_ATTRIBUTE_CENTER_NODE):
      {
        numComponents = currAttribute->GetValues()->GetNumberOfElements() / num_nodes;
        nodalComponents.push_back(numComponents);
        numNodalAttributes += numComponents;
        nameHandler->ConstructAttributeName(currAttribute->GetName(), nodalAttributeNames, numComponents);
        break;
      }
      case(XDMF_ATTRIBUTE_CENTER_CELL):
      {
        numComponents = currAttribute->GetValues()->GetNumberOfElements() / num_elem;
        elementComponents.push_back(numComponents);
        numElementAttributes += numComponents;
        nameHandler->ConstructAttributeName(currAttribute->GetName(), elementAttributeNames, numComponents);
        break;
      }
    }
  }

  ex_put_var_param(exodusHandle, "g", numGlobalAttributes);
  ex_put_var_param(exodusHandle, "n", numNodalAttributes);
  ex_put_var_param(exodusHandle, "e", numElementAttributes);

  char ** globalNames = new char*[numGlobalAttributes];
  char ** nodalNames = new char*[numNodalAttributes];
  char ** elementNames = new char*[numElementAttributes];

  for(int i=0; i<numGlobalAttributes; ++i)
  {
    globalNames[i] = (char*)globalAttributeNames[i].c_str();
  }

  for(int i=0; i<numNodalAttributes; ++i)
  {
    nodalNames[i] = (char*)nodalAttributeNames[i].c_str();
  }

  for(int i=0; i<numElementAttributes; ++i)
  {
    elementNames[i] = (char*)elementAttributeNames[i].c_str();
  }

  ex_put_var_names(exodusHandle, "g", numGlobalAttributes, globalNames);
  ex_put_var_names(exodusHandle, "n", numNodalAttributes, nodalNames);
  ex_put_var_names(exodusHandle, "e", numElementAttributes, elementNames);

  delete [] globalNames;
  delete [] nodalNames;
  delete [] elementNames;

  double * globalAttributeVals = new double[numGlobalAttributes];

  int globalIndex = 0;
  int globalComponentIndex = 0;
  int nodalIndex = 0;
  int nodalComponentIndex = 0;
  int elementIndex = 0;
  int elementComponentIndex = 0;

  for(int i=0; i<gridToWrite->GetNumberOfAttributes(); ++i)
  {
    XdmfAttribute * currAttribute = gridToWrite->GetAttribute(i);
    switch(currAttribute->GetAttributeCenter())
    {
      case(XDMF_ATTRIBUTE_CENTER_GRID):
      {
        for(int j=0; j<globalComponents[globalComponentIndex]; ++j)
        {
          currAttribute->GetValues()->GetValues(j, &globalAttributeVals[globalIndex], 1);
          globalIndex++;
        }
        globalComponentIndex++;
        break;
      }
      case(XDMF_ATTRIBUTE_CENTER_NODE):
      {
        for(int j=0; j<nodalComponents[nodalComponentIndex]; ++j)
        {
          double * nodalValues = new double[num_nodes];
          currAttribute->GetValues()->GetValues(j, nodalValues, num_nodes, nodalComponents[nodalComponentIndex]);
          ex_put_nodal_var(exodusHandle, 1, nodalIndex+1, num_nodes, nodalValues);
          ex_update(exodusHandle);
          delete [] nodalValues;
          nodalIndex++;
        }
        nodalComponentIndex++;
        break;
      }
      case(XDMF_ATTRIBUTE_CENTER_CELL):
      {
        for(int j=0; j<elementComponents[elementComponentIndex]; ++j)
        {
          double * elementValues = new double[num_elem];
          currAttribute->GetValues()->GetValues(j, elementValues, num_elem, elementComponents[elementComponentIndex]);
          ex_put_elem_var(exodusHandle, 1, elementIndex+1, 10, num_elem, elementValues);
          ex_update(exodusHandle);
          delete [] elementValues;
          elementIndex++;
        }
        elementComponentIndex++;
        break;
      }
    }
  }
  ex_put_glob_vars(exodusHandle, 1, numGlobalAttributes, globalAttributeVals);
  ex_update(exodusHandle);
  delete [] globalAttributeVals;

  // Write Sets
  int setId = 20;
  for (int i=0; i<gridToWrite->GetNumberOfSets(); ++i)
  {
    XdmfSet * currSet = gridToWrite->GetSets(i);
    currSet->Update();
    int numValues = currSet->GetIds()->GetNumberOfElements();
    std::string name = currSet->GetName();
    if(name.length() > MAX_STR_LENGTH)
    {
      name = name.substr(0, MAX_STR_LENGTH);
    }
    switch(currSet->GetSetType())
    {
      case(XDMF_SET_TYPE_CELL):
      {
        ex_put_side_set_param(exodusHandle, setId, numValues, 0);
        int * values = new int[numValues];
        // Add 1 to xdmf ids because exodus ids begin at 1
        *currSet->GetIds() + 1;
        currSet->GetIds()->GetValues(0, values, numValues);
        ex_put_side_set(exodusHandle, setId, values, NULL);
        ex_put_name(exodusHandle, EX_SIDE_SET, setId, name.c_str());
        setId++;
        delete [] values;
        break;
      }
      case(XDMF_SET_TYPE_NODE):
      {
        ex_put_node_set_param(exodusHandle, setId, numValues, 0);
        int * values = new int[numValues];
        // Add 1 to xdmf ids because exodus ids begin at 1
        *currSet->GetIds() + 1;
        currSet->GetIds()->GetValues(0, values, numValues);
        ex_put_node_set(exodusHandle, setId, values);
        ex_put_name(exodusHandle, EX_NODE_SET, setId, name.c_str());
        setId++;
        delete [] values;
        break;
      }
    }
  }

  // Close Exodus File
  ex_close(exodusHandle);
}
