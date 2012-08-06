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

#include "XdmfExodusReader.h"
#include <exodusII.h>

//
// Construct XdmfExodusReader.
//
XdmfExodusReader::XdmfExodusReader()
{
  return;
}

//
// Destroy XdmfExodusReader
//
XdmfExodusReader::~XdmfExodusReader()
{
  return;
}

//
// Translate from exodus ii to xdmf topologies
// this was taken directly from vtkExodusIIReader and modified to fit xdmf elements
//
XdmfInt32 XdmfExodusReader::DetermineXdmfCellType(char * exoElemType, int numPointsPerCell)
{
  // Make exoType uppercase
  std::string elemType = exoElemType;
  std::transform(elemType.begin(), elemType.end(), elemType.begin(), toupper);

  // Check for quadratic elements
  if (elemType.substr(0,3) == "TRI" && numPointsPerCell == 6)
  {
    return XDMF_TRI_6;
  }
  else if (elemType.substr(0,3) == "SHE" && numPointsPerCell == 8)
  {
    return XDMF_QUAD_8;
  }
  else if (elemType.substr(0,3) == "SHE" && numPointsPerCell == 9)
  {
    // VTK_QUADRATIC_QUAD with 9 points
    // Currently unsupported in Xdmf
    return XDMF_QUAD_9;
  }
  else if (elemType.substr(0,3) == "TET" && numPointsPerCell == 10)
  {
    return XDMF_TET_10;
  }
  else if (elemType.substr(0,3) == "TET" && numPointsPerCell == 11)
  {
    // VTK_QUADRATIC_TETRA with 11 points
    // Currently unsupported in Xdmf
    return XDMF_NOTOPOLOGY;
  }
  else if (elemType.substr(0,3) == "WED" && numPointsPerCell == 15)
  {
    return XDMF_WEDGE_15;
  }
  else if (elemType.substr(0,3) == "HEX" && numPointsPerCell == 20)
  {
    return XDMF_HEX_20;
  }
  else if (elemType.substr(0,3) == "HEX" && numPointsPerCell == 21)
  {
    // VTK_QUADRATIC_HEXAHEDRON with 21 points
    // Currently unsupported in Xdmf
    return XDMF_NOTOPOLOGY;
  }
  else if (elemType.substr(0,3) == "HEX" && numPointsPerCell == 27)
  {
    return XDMF_HEX_27;
  }
  else if (elemType.substr(0,3) == "QUA" && numPointsPerCell == 8)
  {
    return XDMF_QUAD_8;
  }
  else if (elemType.substr(0,3) == "QUA" && numPointsPerCell == 9)
  {
    // VTK_BIQUADRATIC_QUAD;
    // Currently unsupported in Xdmf
    return XDMF_QUAD_9;
  }
  else if (elemType.substr(0,3) == "TRU" && numPointsPerCell == 3)
  {
    return XDMF_EDGE_3;
  }
  else if (elemType.substr(0,3) == "BEA" && numPointsPerCell == 3)
  {
    return XDMF_EDGE_3;
  }
  else if (elemType.substr(0,3) == "BAR" && numPointsPerCell == 3)
  {
    return XDMF_EDGE_3;
  }
  else if (elemType.substr(0,3) == "EDG" && numPointsPerCell == 3)
  {
    return XDMF_EDGE_3;
  }
  // Check for regular elements
  else if (elemType.substr(0,3) == "CIR")
  {
    // VTK_VERTEX;
    // Currently unsupported in Xdmf
    return XDMF_NOTOPOLOGY;
  }
  else if (elemType.substr(0,3) == "SPH")
  {
    // VTK_VERTEX;
    // Currently unsupported in Xdmf
    return XDMF_NOTOPOLOGY;
  }
  else if (elemType.substr(0,3) == "BAR")
  {
    // VTK_LINE;
    // Currently unsupported in Xdmf
    return XDMF_NOTOPOLOGY;
  }
  else if (elemType.substr(0,3) == "TRU")
  {
    // VTK_LINE;
    // Currently unsupported in Xdmf
    return XDMF_NOTOPOLOGY;
  }
  else if (elemType.substr(0,3) == "BEA")
  {
    // VTK_LINE;
    // Currently unsupported in Xdmf
    return XDMF_NOTOPOLOGY;
  }
  else if (elemType.substr(0,3) == "EDG")
  {
    // VTK_LINE;
    // Currently unsupported in Xdmf
    return XDMF_NOTOPOLOGY;
  }
  else if (elemType.substr(0,3) == "TRI")
  {
    return XDMF_TRI;
  }
  else if (elemType.substr(0,3) == "QUA")
  {
    return XDMF_QUAD;
  }
  else if (elemType.substr(0,3) == "TET")
  {
    return XDMF_TET;
  }
  else if (elemType.substr(0,3) == "PYR")
  {
    return XDMF_PYRAMID;
  }
  else if (elemType.substr(0,3) == "WED")
  {
    return XDMF_WEDGE;
  }
  else if (elemType.substr(0,3) == "HEX")
  {
    return XDMF_HEX;
  }
  else if (elemType.substr(0,3) == "SHE" && numPointsPerCell == 3)
  {
    return XDMF_TRI;
  }
  else if (elemType.substr(0,3) == "SHE" && numPointsPerCell == 4)
  {
    return XDMF_QUAD;
  }
  else if (elemType.substr(0,8) == "STRAIGHT" && numPointsPerCell == 2)
  {
    // VTK_LINE;
    // Currently unsupported in Xdmf
    return XDMF_NOTOPOLOGY;
  }
  else if (elemType.substr(0,3) == "SUP")
  {
    return XDMF_POLYVERTEX;
  }
  //vtkErrorMacro("Unsupported element type: " << elemType.c_str());
  return XDMF_NOTOPOLOGY;
  }

//
// Read contents of the exodus file and fill in xdmf grids.
//
XdmfGrid * XdmfExodusReader::read(const char * fileName, XdmfElement * parentElement)
{
  XdmfGrid * grid = new XdmfGrid();
  parentElement->Insert(grid);

  // Read Exodus II file to XdmfGrid via Exodus II API

  float version;
  int CPU_word_size = sizeof(double);
  int IO_word_size = 0; // Get from file
  int exodusHandle = ex_open(fileName, EX_READ, &CPU_word_size, &IO_word_size, &version);

  char * title = new char[MAX_LINE_LENGTH+1];
  int num_dim, num_nodes, num_elem, num_elem_blk, num_node_sets, num_side_sets;
  ex_get_init (exodusHandle, title, &num_dim, &num_nodes, &num_elem, &num_elem_blk, &num_node_sets, &num_side_sets);

  /*
  cout << "Title: " << title <<
    "\nNum Dim: " << num_dim <<
    "\nNum Nodes: " << num_nodes <<
    "\nNum Elem: " << num_elem <<
    "\nNum Elem Blk: " << num_elem_blk <<
    "\nNum Node Sets: " << num_node_sets <<
    "\nNum Side Sets: " << num_side_sets << endl;
  */

  // Read geometry values
  double * x = new double[num_nodes];
  double * y = new double[num_nodes];
  double * z = new double[num_nodes];

  ex_get_coord(exodusHandle, x, y, z);

  XdmfGeometry * geom = grid->GetGeometry();
  if(num_dim < 2 || num_dim > 3)
  {
    // Xdmf does not support geometries with less than 2 dimensions
		std::cout << "Exodus File contains geometry of dimension " << num_dim << "which is unsupported by Xdmf" << std::endl;
  	return NULL;
	}

  // In the future we may want to do XDMF_GEOMETRY_X_Y_Z?
  if(num_dim == 2)
  {
    geom->SetGeometryType(XDMF_GEOMETRY_XY);
  }
  else
  {
    geom->SetGeometryType(XDMF_GEOMETRY_XYZ);
  }
  geom->SetNumberOfPoints(num_nodes);
  geom->SetDeleteOnGridDelete(true);

  XdmfArray * points = geom->GetPoints();
  points->SetNumberType(XDMF_FLOAT64_TYPE);
  points->SetNumberOfElements(num_nodes * num_dim);
  for (int j=0; j<num_nodes; j++)
  {
    points->SetValue((j * num_dim), x[j]);
    points->SetValue((j * num_dim) + 1, y[j]);
    if(num_dim == 3)
    {
   	  points->SetValue((j * num_dim) + 2, z[j]);
    }
  }
  delete [] x;
  delete [] y;
  delete [] z;

  int * blockIds = new int[num_elem_blk];
  ex_get_elem_blk_ids(exodusHandle, blockIds);

  int * numElemsInBlock = new int[num_elem_blk];
  int * numNodesPerElemInBlock = new int[num_elem_blk];
  int * numElemAttrInBlock = new int[num_elem_blk];
  XdmfInt32 * topTypeInBlock = new XdmfInt32[num_elem_blk];
  int totalNumElem = 0;
  int totalConns = 0;
  for (int j=0; j<num_elem_blk; j++)
  {
    char * elem_type = new char[MAX_STR_LENGTH+1];
    int num_nodes_per_elem, num_elem_this_blk, num_attr;
    ex_get_elem_block(exodusHandle, blockIds[j], elem_type, &num_elem_this_blk, &num_nodes_per_elem, &num_attr);

    /*
    cout << "Block Id: " << blockIds[j] <<
      "\nElem Type: " << elem_type <<
      "\nNum Elem in Blk: " << num_elem_this_blk <<
      "\nNum Nodes per Elem: " << num_nodes_per_elem <<
      "\nNum Attr: " << num_attr << endl;
    */

    numElemsInBlock[j] = num_elem_this_blk;
    numNodesPerElemInBlock[j] = num_nodes_per_elem;
    numElemAttrInBlock[j] = num_attr;
    topTypeInBlock[j] = this->DetermineXdmfCellType(elem_type, num_nodes_per_elem);
    totalNumElem = totalNumElem + num_elem_this_blk;
    totalConns = totalConns + num_elem_this_blk * num_nodes_per_elem;
    delete [] elem_type;
  }

  // Read connectivity from element blocks
  // TODO: Make this work for mixed topologies?
  XdmfInt32 topType;
  int * conn = new int[totalConns];
  int elemIndex = 0;
  for (int j=0; j<num_elem_blk; ++j)
  {
    if (topTypeInBlock[j] != XDMF_NOTOPOLOGY)
    {
      topType = topTypeInBlock[j];
      ex_get_elem_conn(exodusHandle, blockIds[j], &conn[elemIndex]);
      elemIndex = elemIndex + numElemsInBlock[j] * numNodesPerElemInBlock[j];
    }
  }

  // This is taken from VTK's vtkExodusIIReader and adapted to fit Xdmf element types, which have the same ordering as VTK.
  if(topType == XDMF_HEX_20 || topType == XDMF_HEX_27)
  {
    int * ptr = conn;
    int k;
    int itmp[4];

    // Exodus Node ordering does not match Xdmf, we must convert.
    for (int i=0; i<totalNumElem; i++)
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
        for ( k = 0; k < 4; ++k, ++ptr )
        {
          itmp[k] = *ptr;
          *ptr = ptr[3];
        }
        *(ptr++) = itmp[1];
        *(ptr++) = itmp[2];
        *(ptr++) = itmp[0];
      }
    }
  }
  else if (topType == XDMF_WEDGE_15 || topType == XDMF_WEDGE_18)
  {
    int * ptr = conn;
    int k;
    int itmp[3];

    // Exodus Node ordering does not match Xdmf, we must convert.
    for (int i=0; i<totalNumElem; i++)
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
        *(ptr++) = itmp[1];
        *(ptr++) = itmp[2];
        *(ptr++) = itmp[0];
      }
    }
  }

  XdmfTopology * topology = grid->GetTopology();
  topology->SetTopologyType(topType);
  topology->SetNumberOfElements(totalNumElem);
  topology->SetDeleteOnGridDelete(true);

  XdmfArray * connections = topology->GetConnectivity();
  connections->SetNumberType(XDMF_INT32_TYPE);
  connections->SetNumberOfElements(totalConns);
  connections->SetValues(0, conn, totalConns, 1, 1);
  // Subtract all node ids by 1 since exodus indices start at 1
  *connections - 1;
  delete [] conn;

  // Get nodal map to global ids and write global ids to xdmf
  int * node_map = new int[num_nodes];
  ex_get_node_num_map(exodusHandle, node_map);

  XdmfAttribute * globalIds = new XdmfAttribute();
  globalIds->SetName("GlobalNodeId");
  globalIds->SetAttributeType(XDMF_ATTRIBUTE_TYPE_SCALAR);
  globalIds->SetAttributeCenter(XDMF_ATTRIBUTE_CENTER_NODE);
  globalIds->SetDeleteOnGridDelete(true);

  XdmfArray * globalNodeIdVals = globalIds->GetValues();
  globalNodeIdVals->SetNumberType(XDMF_INT32_TYPE);
  globalNodeIdVals->SetNumberOfElements(num_nodes);
  globalNodeIdVals->SetValues(0, node_map, num_nodes, 1, 1);
  // Subtract all node ids by 1 since exodus indices start at 1
  *globalNodeIdVals - 1;
  grid->Insert(globalIds);
  delete [] node_map;

  // Read node sets
  int * nodeSetIds = new int[num_node_sets];
  ex_get_node_set_ids(exodusHandle, nodeSetIds);

  char * node_set_names[num_node_sets];
  for (int j=0; j<num_node_sets; j++)
  {
    node_set_names[j] = new char[MAX_STR_LENGTH+1];
  }
  ex_get_names(exodusHandle, EX_NODE_SET, node_set_names);

  for (int j=0; j<num_node_sets; j++)
  {
    int num_nodes_in_set, num_df_in_set;
    ex_get_node_set_param(exodusHandle, nodeSetIds[j], &num_nodes_in_set, &num_df_in_set);

    /*
    cout << "Node Set Id: " << nodeSetIds[j] <<
      "\nNode Set Name: " << node_set_names[j] <<
      "\nNum Nodes in Set: "<< num_nodes_in_set <<
      "\nNum Distrub Factors: " << num_df_in_set << endl;
    */

    if (num_nodes_in_set > 0)
    {
      int * node_set_node_list = new int[num_nodes_in_set];
      ex_get_node_set(exodusHandle, nodeSetIds[j], node_set_node_list);

      XdmfSet * set = new XdmfSet();
      set->SetName(node_set_names[j]);
      set->SetSetType(XDMF_SET_TYPE_NODE);
      set->SetSize(num_nodes_in_set);
      set->SetDeleteOnGridDelete(true);

      XdmfArray * ids = set->GetIds();
      ids->SetNumberType(XDMF_INT32_TYPE);
      ids->SetNumberOfElements(num_nodes_in_set);
      ids->SetValues(0, node_set_node_list, num_nodes_in_set, 1, 1);
      // Subtract all node ids by 1 since exodus indices start at 1
      *ids - 1;
      grid->Insert(set);

      /*
      if(num_df_in_set > 0)
      {
        double * node_set_distribution_factors = new double[num_df_in_set];
        ex_get_node_set_dist_fact(exodusHandle, nodeSetIds[j], node_set_distribution_factors);

        XdmfAttribute * attr = new XdmfAttribute();
        attr->SetName("SetAttribute");
        attr->SetAttributeType(XDMF_ATTRIBUTE_TYPE_SCALAR);
        attr->SetAttributeCenter(XDMF_ATTRIBUTE_CENTER_NODE);
        attr->SetDeleteOnGridDelete(true);

        XdmfArray * attrVals = attr->GetValues();
        attrVals->SetNumberType(XDMF_FLOAT32_TYPE);
        attrVals->SetNumberOfElements(num_df_in_set);
        attrVals->SetValues(0, node_set_distribution_factors, num_df_in_set, 1, 1);
        set->Insert(attr);
        delete [] node_set_distribution_factors;
      }
      */

      delete [] node_set_node_list;
    }
    delete [] node_set_names[j];
  }
  delete [] nodeSetIds;

  // Read result variables (attributes)
  int num_global_vars, num_nodal_vars, num_elem_vars;
  ex_get_var_param(exodusHandle, "g", &num_global_vars);
  ex_get_var_param(exodusHandle, "n", &num_nodal_vars);
  ex_get_var_param(exodusHandle, "e", &num_elem_vars);

  /*
  cout << "Num Global Vars: " << num_global_vars <<
    "\nNum Nodal Vars: " << num_nodal_vars <<
    "\nNum Elem Vars: " << num_elem_vars << endl;
  */

  char * global_var_names[num_global_vars];
  char * nodal_var_names[num_nodal_vars];
  char * elem_var_names[num_elem_vars];
  for (int j=0; j<num_global_vars; j++)
  {
    global_var_names[j] = new char[MAX_STR_LENGTH+1];
  }
  for (int j=0; j<num_nodal_vars; j++)
  {
    nodal_var_names[j] = new char[MAX_STR_LENGTH+1];
  }
  for (int j=0; j<num_elem_vars; j++)
  {
    elem_var_names[j] = new char[MAX_STR_LENGTH+1];
  }
  ex_get_var_names(exodusHandle, "g", num_global_vars, global_var_names);
  ex_get_var_names(exodusHandle, "n", num_nodal_vars, nodal_var_names);
  ex_get_var_names(exodusHandle, "e", num_elem_vars, elem_var_names);

  /*
  cout << "Global Vars Names: " << endl;
  for (int j=0; j<num_global_vars; j++)
  {
    cout << global_var_names[j] << endl;
  }
  cout << "Nodal Vars Names: " << endl;
  for (int j=0; j<num_nodal_vars; j++)
  {
    cout << nodal_var_names[j] << endl;
  }
  cout << "Elem Vars Names: " << endl;
  for (int j=0; j<num_elem_vars; j++)
  {
    cout << elem_var_names[j] << endl;
  }
  */

  // Get variable data
  // TODO: do this for all timesteps?

  // Global variable data
  double * global_var_vals = new double[num_global_vars];
  ex_get_glob_vars(exodusHandle, 1, num_global_vars, &global_var_vals);
  for (int j=0; j<num_global_vars; j++)
  {
    // Write global attribute to xdmf
    XdmfAttribute * attr = new XdmfAttribute();
    attr->SetName(global_var_names[j]);
    attr->SetAttributeType(XDMF_ATTRIBUTE_TYPE_SCALAR);
    attr->SetAttributeCenter(XDMF_ATTRIBUTE_CENTER_GRID);
    attr->SetDeleteOnGridDelete(true);

    XdmfArray * attrVals = attr->GetValues();
    attrVals->SetNumberType(XDMF_FLOAT64_TYPE);
    attrVals->SetNumberOfElements(1);
    attrVals->SetValues(0, &global_var_vals[j], 1, 1, 1);
    grid->Insert(attr);
    delete [] global_var_names[j];
  }
  delete [] global_var_vals;

  // Nodal variable data
  for (int j=0; j<num_nodal_vars; j++)
  {
    // The strcmp with "GlobalNodeId" is meant to prevent errors from occuring when a nodal variable is named GlobalNodeId.
    // A GlobalNodeId attribute was added before when adding the nodal map which means that this attribute should be ignored...
    // This will probably only occur when doing repeated conversions --- i.e. Xdmf to Exodus to Xdmf to Exodus...
    if (strcmp(nodal_var_names[j], "GlobalNodeId") != 0)
    {
      double * nodal_var_vals = new double[num_nodes];
      ex_get_nodal_var(exodusHandle, 1, j+1, num_nodes, nodal_var_vals);

      // Write nodal attribute to xdmf
      XdmfAttribute * attr = new XdmfAttribute();
      attr->SetName(nodal_var_names[j]);
      attr->SetAttributeType(XDMF_ATTRIBUTE_TYPE_SCALAR);
      attr->SetAttributeCenter(XDMF_ATTRIBUTE_CENTER_NODE);
      attr->SetDeleteOnGridDelete(true);

      XdmfArray * attrVals = attr->GetValues();
      attrVals->SetNumberType(XDMF_FLOAT64_TYPE);
      attrVals->SetNumberOfElements(num_nodes);
      attrVals->SetValues(0, nodal_var_vals, num_nodes, 1, 1);
      grid->Insert(attr);
      delete [] nodal_var_vals;
      delete [] nodal_var_names[j];
    }
  }

  // Element variable data
  for (int j=0; j<num_elem_vars; j++)
  {
    double * elem_var_vals = new double[totalNumElem];
    elemIndex = 0;
    for (int k=0; k<num_elem_blk; k++)
    {
      ex_get_elem_var(exodusHandle, 1, j+1, blockIds[k], numElemsInBlock[j], &elem_var_vals[elemIndex]);
      elemIndex = elemIndex + numElemsInBlock[j];
    }

    // Write element attribute to xdmf
    XdmfAttribute * attr = new XdmfAttribute();
    attr->SetName(elem_var_names[j]);
    attr->SetAttributeType(XDMF_ATTRIBUTE_TYPE_SCALAR);
    attr->SetAttributeCenter(XDMF_ATTRIBUTE_CENTER_CELL);
    attr->SetDeleteOnGridDelete(true);

    XdmfArray * attrVals = attr->GetValues();
    attrVals->SetNumberType(XDMF_FLOAT64_TYPE);
    attrVals->SetNumberOfElements(totalNumElem);
    attrVals->SetValues(0, elem_var_vals, totalNumElem, 1, 1);
    grid->Insert(attr);
    delete [] elem_var_vals;
    delete [] elem_var_names[j];
  }

  ex_close(exodusHandle);
  delete [] title;
  delete [] blockIds;
  delete [] numElemsInBlock;
  delete [] numNodesPerElemInBlock;
  delete [] numElemAttrInBlock;
  delete [] topTypeInBlock;
  return grid;
}
