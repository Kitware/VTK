//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
// Copyright (c) 2018, The Regents of the University of California, through
// Lawrence Berkeley National Laboratory (subject to receipt of any required approvals
// from the U.S. Dept. of Energy).  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// (1) Redistributions of source code must retain the above copyright notice, this
//     list of conditions and the following disclaimer.
//
// (2) Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
// (3) Neither the name of the University of California, Lawrence Berkeley National
//     Laboratory, U.S. Dept. of Energy nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//
//=======================================================================================
//
//	Parallel Peak Pruning v. 2.0
//
//	Started June 15, 2017
//
// Copyright Hamish Carr, University of Leeds
//
// PrintGraph.h - routines for outputting dot files for debug purposes
//
//=======================================================================================
//
// COMMENTS:	We have dot output scattered among multiple files, when it is primarily
//				used for debug. Moreover, our principal data classes are public so that
//				the algorithmic construction can be separated into factory classes.
//				So we can collect all of the code for dot output into one place and be
//				consistent about how we implement it
//
//				We will want dot output for the following classes:
//					BoundaryTree
//					ContourTree
//					ContourTreeMesh
//					HierarchicalContourTree
//					InteriorForest / InteriorForest
//
//				In addition, it may be useful to be able to print out:
//					MergeTree
//
//				but as of the time of writing (30/07/2020) it is not priority for debug
//
//				Factory classes will typically have multiple temporary arrays
//				and will therefore primarily be debugged by printing the arrays
//
//				However, since these classes are often indexed on our the main data classes
//				we should also have a version of the graph output that shows a designated
//				piece of text on each node
//
//				The general design (initially) will be to produce all-purpose illustrations
//				While these will probably be overkill, experience shows that confusion between
//				the different indices is common, so it is better to be absolutely explicit
//				and print out every	piece of information possible in the graph.
//				However, for the ContourTree and HierarchicalContourTree, we also want to have
//				the option to show regular / super / hyper structure separately.
//
//				NOTE ON CONST USAGE:
//				I had originally planned to use NULL vectors for parameters not being passed
//				but the compiler doesn't like the possibility of non-const temporary parameters
//				so the flags arrays must be const.
//				If I do the same with Mesh & ContourTree, I need to set functions elsewhere to
//				be const.  I will not do that unilaterally.
//				All parameters of these print functions should on principal be const, except
//				the stream parameter
//
//=======================================================================================

#ifndef viskores_worklet_contourtree_distributed_print_graph_h
#define viskores_worklet_contourtree_distributed_print_graph_h

#include <viskores/Types.h>
#include <viskores/cont/Algorithm.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/ContourTree.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/meshtypes/ContourTreeMesh.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/BoundaryTree.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/HierarchicalContourTree.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/InteriorForest.h>

namespace viskores
{
namespace worklet
{
namespace contourtree_distributed
{

/// \brief Routines for printing various tree data structures in graphviz .dot format
///
/// These routines are primarily for debug at the moment, and share a number of constants and
/// software patterns.
/// They are therefore collected in a single unit rather than distributed in each clase

//	Routines to print out a contour tree in regular and super/hyper format
//	The older code ended up with separate versions depending on mesh type, but there are now
//	accessor functions, and I will try to avoid this. However, this means templating everything
//	to do with contour trees

// Routines for contour trees:
// 1.	Routine to print super structure
//		generic, with colour set according to iteration
//		with option to print grey / white for boundary / interior nodes (regular tree only)
//		with option to show BoundaryTree / Forest as different styles (super tree only)
//		with option to show an arbitrary additional value passed as an array

// Routines for ContourTreeMesh:
// 2.	Simple routine to dump out nodes / edges for cross-checking
// 		no options should be needed

// Routines for BoundaryTree:
// 3.	All purpose routine to dump out the contents for comparison with contour tree
//		Includes option to show how the InteriorForest connects to it

// Routines for HierarchicalContourTree:
// 4.	Routine to print regular/super/hyper structure with similar options to contour tree

// Routines for contour trees:
// 1.	Routine to print super structure
//		generic, with colour set according to iteration
//		with option to print grey / white for boundary / interior nodes (regular tree only)
//		with option to show BoundaryTree / Forest as different styles (super tree only)
//		with option to show an arbitrary additional value passed as an array

constexpr viskores::Id INDEX_WIDTH = 6;

constexpr viskores::Id NO_PER_NODE_VALUES = 0;
constexpr viskores::Id PER_REGULAR_NODE_VALUES = 1;
constexpr viskores::Id PER_REGULAR_NODE_BOUNDARY_FLAGS = 2;
constexpr viskores::Id PER_SUPER_NODE_VALUES = 3;
constexpr viskores::Id PER_SUPER_NODE_BOUNDARY_FLAGS = 4;
constexpr viskores::Id PER_HYPER_NODE_VALUES = 5;
constexpr viskores::Id BAD_PER_NODE_VALUES = 6;

constexpr viskores::Id NODE_TYPE_REGULAR = 0;
constexpr viskores::Id NODE_TYPE_SUPER = 1;
constexpr viskores::Id NODE_TYPE_HYPER = 2;

// bitflags for various components
constexpr viskores::Id SHOW_REGULAR_STRUCTURE = 0x00000001;
constexpr viskores::Id SHOW_SUPER_STRUCTURE = 0x00000002;
constexpr viskores::Id SHOW_HYPER_STRUCTURE = 0x00000004;

constexpr viskores::Id SHOW_BOUNDARY_NODES = 0x00000010;
constexpr viskores::Id SHOW_CRITICAL_BOUNDARY_NODES = 0x00000020;
constexpr viskores::Id SHOW_NECESSARY_SUPERNODES = 0x00000040;

constexpr viskores::Id SHOW_GLOBAL_ID = 0x00000100;
constexpr viskores::Id SHOW_DATA_VALUE = 0x00000200;
constexpr viskores::Id SHOW_MESH_REGULAR_ID = 0x00000400;
constexpr viskores::Id SHOW_MESH_SORT_ID = 0x00000800;

constexpr viskores::Id SHOW_NODE_ID = 0x00001000;
constexpr viskores::Id SHOW_SUPERPARENT = 0x00002000;
constexpr viskores::Id SHOW_ARC_ID = 0x00004000;
constexpr viskores::Id SHOW_EXTRA_DATA = 0x00008000;

constexpr viskores::Id SHOW_SUPERNODE_ID = 0x00010000;
constexpr viskores::Id SHOW_HYPERPARENT = 0x00020000;
constexpr viskores::Id SHOW_SUPERARC_ID = 0x0004000;
constexpr viskores::Id SHOW_ITERATION = 0x00080000;

constexpr viskores::Id SHOW_HYPERNODE_ID = 0x00100000;
constexpr viskores::Id SHOW_HYPERARC_ID = 0x00200000;

// bit flags used for structures other than the contour tree
// the BoundaryTree has a vertex index, but doesn't have the contour tree's nodes, so we will reuse that bit flag
constexpr viskores::Id SHOW_BOUNDARY_TREE_VERTEX_ID = SHOW_NODE_ID;
// others are just relabelling the same ID flag
constexpr viskores::Id SHOW_BOUNDARY_TREE_GLOBAL_ID = SHOW_GLOBAL_ID;
constexpr viskores::Id SHOW_BOUNDARY_TREE_DATA_VALUE = SHOW_DATA_VALUE;
constexpr viskores::Id SHOW_BOUNDARY_TREE_MESH_REGULAR_ID = SHOW_MESH_REGULAR_ID;
constexpr viskores::Id SHOW_BOUNDARY_TREE_MESH_SORT_ID = SHOW_MESH_SORT_ID;
constexpr viskores::Id SHOW_BOUNDARY_TREE_ARC_ID = SHOW_ARC_ID;
constexpr viskores::Id SHOW_BOUNDARY_TREE_ALL =
  (SHOW_BOUNDARY_TREE_VERTEX_ID | SHOW_BOUNDARY_TREE_GLOBAL_ID | SHOW_BOUNDARY_TREE_DATA_VALUE |
   SHOW_BOUNDARY_TREE_MESH_REGULAR_ID | SHOW_BOUNDARY_TREE_MESH_SORT_ID |
   SHOW_BOUNDARY_TREE_ARC_ID);

// Similarly, relabel the IDs for use with contour tree meshes
constexpr viskores::Id SHOW_CONTOUR_TREE_MESH_VERTEX_ID = SHOW_NODE_ID;
// others are just relabelling the same ID flag
constexpr viskores::Id SHOW_CONTOUR_TREE_MESH_GLOBAL_ID = SHOW_GLOBAL_ID;
constexpr viskores::Id SHOW_CONTOUR_TREE_MESH_DATA_VALUE = SHOW_DATA_VALUE;
constexpr viskores::Id SHOW_CONTOUR_TREE_MESH_ALL =
  (SHOW_CONTOUR_TREE_MESH_VERTEX_ID | SHOW_CONTOUR_TREE_MESH_GLOBAL_ID |
   SHOW_CONTOUR_TREE_MESH_DATA_VALUE);

// InteriorForest is slightly tricky, but wants to be use the same set as BoundaryTree
constexpr viskores::Id SHOW_INTERIOR_FOREST_VERTEX_ID = SHOW_SUPERNODE_ID;
constexpr viskores::Id SHOW_INTERIOR_FOREST_GLOBAL_ID = SHOW_BOUNDARY_TREE_GLOBAL_ID;
constexpr viskores::Id SHOW_INTERIOR_FOREST_DATA_VALUE = SHOW_BOUNDARY_TREE_DATA_VALUE;
constexpr viskores::Id SHOW_INTERIOR_FOREST_MESH_REGULAR_ID = SHOW_BOUNDARY_TREE_MESH_REGULAR_ID;
constexpr viskores::Id SHOW_INTERIOR_FOREST_MESH_SORT_ID = SHOW_BOUNDARY_TREE_MESH_SORT_ID;
constexpr viskores::Id SHOW_INTERIOR_FOREST_ALL =
  (SHOW_INTERIOR_FOREST_VERTEX_ID | SHOW_INTERIOR_FOREST_GLOBAL_ID |
   SHOW_INTERIOR_FOREST_DATA_VALUE | SHOW_INTERIOR_FOREST_MESH_REGULAR_ID |
   SHOW_INTERIOR_FOREST_MESH_SORT_ID);

constexpr viskores::Id SHOW_ALL_STRUCTURE =
  (SHOW_REGULAR_STRUCTURE | SHOW_SUPER_STRUCTURE | SHOW_HYPER_STRUCTURE);
constexpr viskores::Id SHOW_BASIC_IDS = (SHOW_DATA_VALUE | SHOW_MESH_SORT_ID);
constexpr viskores::Id SHOW_ALL_IDS =
  (SHOW_GLOBAL_ID | SHOW_DATA_VALUE | SHOW_MESH_REGULAR_ID | SHOW_MESH_SORT_ID | SHOW_NODE_ID |
   SHOW_SUPERPARENT | SHOW_ARC_ID);
constexpr viskores::Id SHOW_BASIC_SUPERIDS = (SHOW_SUPERNODE_ID | SHOW_ITERATION);
constexpr viskores::Id SHOW_ALL_SUPERIDS =
  (SHOW_SUPERNODE_ID | SHOW_HYPERPARENT | SHOW_ITERATION | SHOW_SUPERARC_ID);
constexpr viskores::Id SHOW_BASIC_HYPERIDS = SHOW_HYPERNODE_ID;
constexpr viskores::Id SHOW_ALL_HYPERIDS = (SHOW_HYPERNODE_ID | SHOW_HYPERARC_ID);

constexpr viskores::Id SHOW_REGULAR_SIMPLE = (SHOW_REGULAR_STRUCTURE | SHOW_BASIC_IDS);
constexpr viskores::Id SHOW_REGULAR_BOUNDARY =
  (SHOW_REGULAR_STRUCTURE | SHOW_BASIC_IDS | SHOW_BOUNDARY_NODES);
constexpr viskores::Id SHOW_REGULAR_CRITICAL_BOUNDARY =
  (SHOW_REGULAR_STRUCTURE | SHOW_BASIC_IDS | SHOW_CRITICAL_BOUNDARY_NODES);

constexpr viskores::Id SHOW_SUPER_SIMPLE =
  (SHOW_SUPER_STRUCTURE | SHOW_BASIC_IDS | SHOW_BASIC_SUPERIDS);
constexpr viskores::Id SHOW_BOUNDARY_INTERIOR_DIVISION =
  (SHOW_SUPER_STRUCTURE | SHOW_BASIC_IDS | SHOW_BASIC_SUPERIDS | SHOW_NECESSARY_SUPERNODES);

constexpr viskores::Id SHOW_SUPER_AND_HYPER_SIMPLE =
  (SHOW_SUPER_SIMPLE | SHOW_HYPER_STRUCTURE | SHOW_HYPERNODE_ID);

constexpr viskores::Id SHOW_ALL_STANDARD =
  (SHOW_ALL_STRUCTURE | SHOW_ALL_IDS | SHOW_ALL_SUPERIDS | SHOW_ALL_HYPERIDS);

constexpr viskores::Id SHOW_HIERARCHICAL_STANDARD =
  (SHOW_SUPER_STRUCTURE | SHOW_HYPER_STRUCTURE | SHOW_ALL_IDS | SHOW_ALL_SUPERIDS |
   SHOW_ALL_HYPERIDS);

// 1.	Routine for printing dot for contour tree regular / super / hyper structure
VISKORES_CONT
template <typename T, typename StorageType, typename MeshType, typename VectorType>
std::string ContourTreeDotGraphPrint(
  const std::string& label, // the label to use as title for the graph
  MeshType& mesh,           // the underlying mesh for the contour tree
  const viskores::worklet::contourtree_augmented::mesh_dem::IdRelabeler*
    localToGlobalIdRelabeler, // relabler needed to compute global ids
  const viskores::cont::ArrayHandle<T, StorageType>& field,
  viskores::worklet::contourtree_augmented::ContourTree& contourTree, // the contour tree itself
  const viskores::Id showMask = SHOW_ALL_STANDARD, // mask with flags for what elements to show
  // const viskores::worklet::contourtree_augmented::IdArrayType &necessaryFlags = viskores::worklet::contourtree_augmented::IdArrayType(),
  // array with flags for "necessary"
  const VectorType& perNodeValues = VectorType()) // an arbitrary vector of values
{                                                 // ContourTreeSuperDotGraphPrint()
  // initialise a string stream to capture the output
  std::stringstream outStream;

  // now grab portals to all the variables we will need
  auto nodesPortal = contourTree.Nodes.ReadPortal();
  auto arcsPortal = contourTree.Arcs.ReadPortal();
  auto superparentsPortal = contourTree.Superparents.ReadPortal();
  auto supernodesPortal = contourTree.Supernodes.ReadPortal();
  auto superarcsPortal = contourTree.Superarcs.ReadPortal();
  auto hyperparentsPortal = contourTree.Hyperparents.ReadPortal();
  auto whenTransferredPortal = contourTree.WhenTransferred.ReadPortal();
  auto hypernodesPortal = contourTree.Hypernodes.ReadPortal();
  auto hyperarcsPortal = contourTree.Hyperarcs.ReadPortal();
  // auto necessaryFlagsPortal	= necessaryFlags.ReadPortal();
  auto perNodeValuesPortal = perNodeValues.ReadPortal();

  // work out how long the computed value is
  int nodeValueType = viskores::worklet::contourtree_distributed::BAD_PER_NODE_VALUES;
  viskores::Id perNodeSize = perNodeValues.GetNumberOfValues();
  if (perNodeSize == 0)
    nodeValueType = viskores::worklet::contourtree_distributed::NO_PER_NODE_VALUES;
  else if (perNodeSize == contourTree.Nodes.GetNumberOfValues())
    nodeValueType = viskores::worklet::contourtree_distributed::PER_REGULAR_NODE_VALUES;
  else if (perNodeSize == contourTree.Supernodes.GetNumberOfValues())
    nodeValueType = viskores::worklet::contourtree_distributed::PER_SUPER_NODE_VALUES;
  else if (perNodeSize == contourTree.Hypernodes.GetNumberOfValues())
    nodeValueType = viskores::worklet::contourtree_distributed::PER_HYPER_NODE_VALUES;
  else
  { // error message
    outStream << "ERROR in ContourTreeDotGraphPrint().\n";
    outStream << "Per node values array must be empty, or\n";
    outStream << "Same length as regular nodes ("
              << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH)
              << contourTree.Nodes.GetNumberOfValues() << "), or\n";
    outStream << "Same length as super nodes   ("
              << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH)
              << contourTree.Supernodes.GetNumberOfValues() << "), or\n";
    outStream << "Same length as hyper nodes   ("
              << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH)
              << contourTree.Hypernodes.GetNumberOfValues() << ")\n";
    outStream << "Actual length was            ("
              << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH)
              << perNodeValues.GetNumberOfValues() << ")\n";
  } // error message

  // print the header information
  outStream << "digraph ContourTree\n\t{\n";
  outStream << "\tlabel=\"" << std::setw(1) << label << "\"\n\tlabelloc=t\n\tfontsize=30\n";
  outStream << "\t// Nodes" << std::endl;

  auto meshSortOrderPortal = mesh.SortOrder.ReadPortal();
  auto globalIds = mesh.GetGlobalIdsFromSortIndices(contourTree.Nodes, localToGlobalIdRelabeler);
  auto globalIdsPortal = globalIds.ReadPortal();
  auto dataValuesPortal = field.ReadPortal();

  // loop through all of the nodes in the regular list
  for (viskores::Id node = 0; node < contourTree.Nodes.GetNumberOfValues(); node++)
  { // per node
    // the nodes array is actually sorted by superarc, but the superarcs array is not
    // so we ignore the nodes array and work directly with the node #
    viskores::Id sortID = nodesPortal.Get(node);

    // retrieve the regular ID
    viskores::Id regularID = meshSortOrderPortal.Get(sortID);

    // retrieve the global ID
    viskores::Id globalID = globalIdsPortal.Get(sortID);

    // retrieve the values
    auto dataValue = dataValuesPortal.Get(regularID);

    // retrieve the superparent
    viskores::Id superparent = superparentsPortal.Get(sortID);

    // and retrieve the iteration #
    viskores::Id iteration =
      viskores::worklet::contourtree_augmented::MaskedIndex(whenTransferredPortal.Get(superparent));

    // work out the super ID & hyper ID
    viskores::Id superID = viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT;
    viskores::Id hyperparent = viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT;
    viskores::Id hyperID = viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT;
    viskores::Id nodeType = viskores::worklet::contourtree_distributed::NODE_TYPE_REGULAR;

    // test for super
    if (supernodesPortal.Get(superparent) == sortID)
    { // at least super
      // set super ID
      superID = superparent;
      // set hyperparent
      hyperparent = hyperparentsPortal.Get(superID);
      // set nodetype
      nodeType = NODE_TYPE_SUPER;
      // test for hyper
      if (hypernodesPortal.Get(hyperparent) == superID)
      { // hyper node
        nodeType = NODE_TYPE_HYPER;
        hyperID = hyperparent;
      } // hyper node
    }   // at least super

    // now, if we don't want the regular nodes, we want to skip them entirely, so
    bool showNode = false;
    // regular structure always shows all nodes
    if (showMask & viskores::worklet::contourtree_distributed::SHOW_REGULAR_STRUCTURE)
      showNode = true;
    // super structure shows super & hyper nodes only
    else if (showMask & viskores::worklet::contourtree_distributed::SHOW_SUPER_STRUCTURE)
      showNode = (nodeType != viskores::worklet::contourtree_distributed::NODE_TYPE_REGULAR);
    else if (showMask & viskores::worklet::contourtree_distributed::SHOW_HYPER_STRUCTURE)
      showNode = (nodeType == viskores::worklet::contourtree_distributed::NODE_TYPE_HYPER);

    // if we didn't set the flag, skip the node
    if (!showNode)
      continue;

    // print the vertex ID, which should be the sort ID & needs to be left-justified to work
    outStream << "\ts" << std::setw(1) << sortID;

    // print the style characteristics - node is filled and fixed size
    outStream << " [style=filled,fixedsize=true,fontname=\"Courier\",margin=\"0.02,0.02\"";
    // specify the style based on the type of node
    if (nodeType == viskores::worklet::contourtree_distributed::NODE_TYPE_REGULAR)
      outStream << ",height=\"1.7in\",width=\"1.7in\",penwidth=5";
    else if (nodeType == viskores::worklet::contourtree_distributed::NODE_TYPE_SUPER)
      outStream << ",height=\"2.5in\",width=\"2.5in\",penwidth=10";
    else if (nodeType == viskores::worklet::contourtree_distributed::NODE_TYPE_HYPER)
      outStream << ",height=\"2.5in\",width=\"2.5in\",penwidth=15";

    // shape should always be circular.
    outStream << ",shape=circle";

    // fill colour is grey for boundary or necessary, if these are passed in
    bool isGrey = false;
    // TODO: Add liesOnBoundary and isNecessary so we can define the gray value
    /*
    if (showMask & viskores::worklet::contourtree_distributed::SHOW_BOUNDARY_NODES)
      isGrey = (showMask & viskores::worklet::contourtree_distributed::SHOW_REGULAR_STRUCTURE) && mesh.liesOnBoundary(regularID);
    else if (showMask & viskores::worklet::contourtree_distributed::SHOW_CRITICAL_BOUNDARY_NODES)
      isGrey = (showMask & viskores::worklet::contourtree_distributed::SHOW_REGULAR_STRUCTURE) && mesh.isNecessary(regularID);
    else if (showMask & viskores::worklet::contourtree_distributed::SHOW_NECESSARY_SUPERNODES)
      isGrey = 	(showMask & viskores::worklet::contourtree_distributed::SHOW_SUPER_STRUCTURE)			// skip if superstructure not shown
          &&	!viskores::worklet::contourtree_augmented::NoSuchElement(superID)						// ignore non-super nodes
          && 	(necessaryFlags.GetNumberOfValues() == contourTree.Supernodes.GetNumberOfValues())	// skip if necessary flags array is wrong size
          &&	necessaryFlagsPortal.Get(superID);
    */
    // after setting the flag, its easy
    outStream << (isGrey ? ",fillcolor=grey" : ",fillcolor=white");

    // stroke colour depends on iteration
    outStream << ",color="
              << viskores::worklet::contourtree_augmented::NODE_COLORS
                   [iteration % viskores::worklet::contourtree_augmented::N_NODE_COLORS];

    // start printing the label
    outStream << ",label=\"";
    // print the global ID
    if (showMask & viskores::worklet::contourtree_distributed::SHOW_GLOBAL_ID)
      outStream << "g " << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH)
                << globalID << "\\n";
    // print the value
    if (showMask & viskores::worklet::contourtree_distributed::SHOW_DATA_VALUE)
      outStream << "v " << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH)
                << dataValue << "\\n";
    // print the regular & sort IDs
    if (showMask & viskores::worklet::contourtree_distributed::SHOW_MESH_REGULAR_ID)
      outStream << "r " << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH)
                << regularID << "\\n";
    if (showMask & viskores::worklet::contourtree_distributed::SHOW_MESH_SORT_ID)
      outStream << "s " << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH)
                << sortID << "\\n";
    // and the node ID
    if (showMask & viskores::worklet::contourtree_distributed::SHOW_NODE_ID)
      outStream << "n " << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH)
                << node << "\\n";
    // print the superparent
    if (showMask & viskores::worklet::contourtree_distributed::SHOW_SUPERPARENT)
      outStream << "sp" << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH)
                << superparent << "\\n";

    // add arbitrary per node value if it is regular in nature
    if ((showMask & viskores::worklet::contourtree_distributed::SHOW_EXTRA_DATA) &&
        (nodeValueType == viskores::worklet::contourtree_distributed::PER_REGULAR_NODE_VALUES))
      outStream << "x " << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH)
                << perNodeValuesPortal.Get(regularID) << "\\n";

    // we now want to add labelling information specific to supernodes, but also present in hypernodes
    if (nodeType != viskores::worklet::contourtree_distributed::NODE_TYPE_REGULAR)
    { // at least super

      // print the super node ID
      if (showMask & viskores::worklet::contourtree_distributed::SHOW_SUPERNODE_ID)
        outStream << "SN" << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH)
                  << superID << "\\n";

      // print the hyperparent as well
      if (showMask & viskores::worklet::contourtree_distributed::SHOW_HYPERPARENT)
        outStream << "HP" << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH)
                  << hyperparent << "\\n";
      if (showMask & viskores::worklet::contourtree_distributed::SHOW_ITERATION)
        outStream << "IT" << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH)
                  << iteration << "\\n";

      // add arbitrary per node value if it is super in nature
      if ((showMask & viskores::worklet::contourtree_distributed::SHOW_EXTRA_DATA) &&
          (nodeValueType == viskores::worklet::contourtree_distributed::PER_SUPER_NODE_VALUES))
        outStream << "X " << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH)
                  << perNodeValuesPortal.Get(superID) << "\\n";
    } // at least super

    // now add even more for hypernodes
    if (nodeType == viskores::worklet::contourtree_distributed::NODE_TYPE_HYPER)
    { // hyper node
      if (showMask & viskores::worklet::contourtree_distributed::SHOW_HYPERNODE_ID)
        outStream << "HN" << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH)
                  << hyperID << "\\n";

      // add arbitrary per node value if it is hyper in nature
      if ((showMask & viskores::worklet::contourtree_distributed::SHOW_EXTRA_DATA) &&
          (nodeValueType == viskores::worklet::contourtree_distributed::PER_HYPER_NODE_VALUES))
        outStream << "X " << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH)
                  << perNodeValuesPortal.Get(hyperID) << "\\n";
    } // hyper node

    outStream << "\"]" << std::endl;
  } // per node

  // always show the null node
  outStream << "\t// Null Node" << std::endl;
  outStream
    << "\tNULL "
       "[style=filled,fixedsize=true,fontname=\"Courier\",margin=\"0.02,0.02\",height=\"0.5in\","
       "width=\"0.5in\",penwidth=1,shape=circle,fillcolor=white,color=black,label=\"NULL\"]"
    << std::endl;

  // start the arcs
  outStream << "\t// Arcs" << std::endl;

  // now add regular arcs (if requested)
  if (showMask & viskores::worklet::contourtree_distributed::SHOW_REGULAR_STRUCTURE)
    for (viskores::Id node = 0; node < contourTree.Nodes.GetNumberOfValues(); node++)
    { // per node
      // retrieve the "to" end
      viskores::Id to = arcsPortal.Get(node);

      // if "to" is NSE, it's the root node
      if (viskores::worklet::contourtree_augmented::NoSuchElement(to))
        outStream << "\ts" << std::setw(1) << node << " -> NULL [penwidth=2";
      else
      { // actual node
        // mask out the flags to get the target node
        to = viskores::worklet::contourtree_augmented::MaskedIndex(to);

        // since we're using sort IDs, we compare them
        if (node < to)
          outStream << "\ts" << std::setw(1) << to << " -> s" << std::setw(1) << node
                    << " [dir=back,penwidth=3";
        else
          outStream << "\ts" << std::setw(1) << node << " -> s" << std::setw(1) << to
                    << " [penwidth=3";
      } // actual node

      // set the color based on the from vertex
      // retrieve the superparent
      viskores::Id superparent = superparentsPortal.Get(node);
      viskores::Id iteration = viskores::worklet::contourtree_augmented::MaskedIndex(
        whenTransferredPortal.Get(superparent));
      outStream << ",color="
                << viskores::worklet::contourtree_augmented::NODE_COLORS
                     [iteration % viskores::worklet::contourtree_augmented::N_NODE_COLORS];
      if (showMask & SHOW_ARC_ID)
        outStream << ",label=\"A" << node << "\"";
      outStream << "]" << std::endl;
    } // per node

  // show superarcs if requested
  if (showMask & viskores::worklet::contourtree_distributed::SHOW_SUPER_STRUCTURE)
    for (viskores::Id supernode = 0; supernode < contourTree.Supernodes.GetNumberOfValues();
         supernode++)
    { // per supernode
      // retrieve the sort ID
      viskores::Id from = supernodesPortal.Get(supernode);

      // retrieve the "to" end
      viskores::Id toSuper = superarcsPortal.Get(supernode);

      // test for "NSE"
      if (viskores::worklet::contourtree_augmented::NoSuchElement(toSuper))
        outStream << "\ts" << std::setw(1) << from << " -> NULL [penwidth=4";
      else
      { // supernode
        // mask out the ascending flag & convert to sort ID
        viskores::Id to =
          supernodesPortal.Get(viskores::worklet::contourtree_augmented::MaskedIndex(toSuper));

        // now test for ascending with sort IDs as before
        if (from < to)
          outStream << "\ts" << std::setw(1) << to << " -> s" << std::setw(1) << from
                    << " [dir=back,penwidth=7";
        else
          outStream << "\ts" << std::setw(1) << from << " -> s" << std::setw(1) << to
                    << " [penwidth=7";
      } // supernode

      // set the color based on the from vertex
      viskores::Id iteration =
        viskores::worklet::contourtree_augmented::MaskedIndex(whenTransferredPortal.Get(supernode));
      outStream << ",color="
                << viskores::worklet::contourtree_augmented::NODE_COLORS
                     [iteration % viskores::worklet::contourtree_augmented::N_NODE_COLORS];
      if (showMask & viskores::worklet::contourtree_distributed::SHOW_SUPERARC_ID)
        outStream << ",label=\"SA" << supernode << "\"";
      outStream << "]" << std::endl;
    } // per supernode

  // add hyper arcs if requested
  if (showMask & viskores::worklet::contourtree_distributed::SHOW_HYPER_STRUCTURE)
    for (viskores::Id hypernode = 0; hypernode < contourTree.Hypernodes.GetNumberOfValues();
         hypernode++)
    { // per hypernode
      // retrieve the sort ID
      viskores::Id fromSuper = hypernodesPortal.Get(hypernode);
      viskores::Id from = supernodesPortal.Get(fromSuper);

      // retrieve the "to" end
      viskores::Id toSuper = hyperarcsPortal.Get(hypernode);

      // test for "NSE"
      if (viskores::worklet::contourtree_augmented::NoSuchElement(toSuper))
        outStream << "\ts" << std::setw(1) << from << " -> NULL [penwidth=6";
      else
      { // hypernode
        // mask out the ascending flag & convert to sort ID
        viskores::Id to =
          supernodesPortal.Get(viskores::worklet::contourtree_augmented::MaskedIndex(toSuper));

        // now test for ascending with sort IDs as before
        if (from < to)
          outStream << "\ts" << std::setw(1) << to << " -> s" << std::setw(1) << from
                    << " [dir=back,penwidth=12";
        else
          outStream << "\ts" << std::setw(1) << from << " -> s" << std::setw(1) << to
                    << " [penwidth=12";
      } // hypernode

      // set the color based on the from vertex
      viskores::Id iteration =
        viskores::worklet::contourtree_augmented::MaskedIndex(whenTransferredPortal.Get(fromSuper));
      outStream << ",color="
                << viskores::worklet::contourtree_augmented::NODE_COLORS
                     [iteration % viskores::worklet::contourtree_augmented::N_NODE_COLORS];
      if (showMask & viskores::worklet::contourtree_distributed::SHOW_HYPERARC_ID)
        outStream << ",label=\"HA" << hypernode << "\"";
      outStream << "]" << std::endl;
    } // per hypernode

  // print the footer information
  outStream << "\t}\n";

  // now return the string
  return outStream.str();
} // ContourTreeSuperDotGraphPrint()


// 2.	Simple routine to dump out nodes / edges for cross-checking
VISKORES_CONT
template <typename FieldType>
std::string ContourTreeMeshDotGraphPrint(
  const std::string& label, // the label to use as title for the graph
  viskores::worklet::contourtree_augmented::ContourTreeMesh<FieldType>& mesh, // the mesh itself
  const viskores::Id showMask =
    SHOW_CONTOUR_TREE_MESH_ALL) // mask with flags for what elements to show
{                               // ContourTreeMeshDotGraphPrint()
  // initialise a string stream to capture the output
  std::stringstream outStream;

  // now grab portals to all the variables we will need
  auto globalMeshIndexPortal = mesh.GlobalMeshIndex.ReadPortal();
  auto meshSortedValuesPortal = mesh.SortedValues.ReadPortal();
  auto meshNeighborConnectivityPortal = mesh.NeighborConnectivity.ReadPortal();
  auto meshNeighborOffsetsPortal = mesh.NeighborOffsets.ReadPortal();

  // print the header information
  outStream << "digraph ContourTreeMesh\n\t{\n";
  outStream << "\tlabel=\"" << std::setw(1) << label << "\"\n\tlabelloc=t\n\tfontsize=30\n";
  outStream << "\t// Nodes" << std::endl;

  // loop through all vertices
  for (viskores::Id vertex = 0; vertex < mesh.GetNumberOfVertices(); vertex++)
  { // per vertex
    // work out the various ID's
    viskores::Id globalID = globalMeshIndexPortal.Get(vertex);
    auto dataValue = meshSortedValuesPortal.Get(vertex);

    // print the vertex
    outStream << "\tr" << std::setw(1) << vertex;
    outStream << "[style=filled,fixedsize=true,fontname=\"Courier\",margin=\"0.02,0.02\",height="
                 "\"1.7in\",width=\"1.7in\",penwidth=5,shape=circle";
    outStream << ",fillcolor=white";
    outStream << ",label=\"";
    if (showMask & viskores::worklet::contourtree_distributed::SHOW_CONTOUR_TREE_MESH_VERTEX_ID)
      outStream << "r " << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH)
                << vertex << "\\n";
    if (showMask & viskores::worklet::contourtree_distributed::SHOW_CONTOUR_TREE_MESH_GLOBAL_ID)
      outStream << "g " << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH)
                << globalID << "\\n";
    if (showMask & viskores::worklet::contourtree_distributed::SHOW_CONTOUR_TREE_MESH_DATA_VALUE)
      outStream << "v " << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH)
                << dataValue << "\\n";
    outStream << "\"];\n";
  } // per vertex

  // now print out the edges
  for (viskores::Id vertex = 0; vertex < mesh.NeighborOffsets.GetNumberOfValues(); vertex++)
  { // per vertex
    // find iterators for the block of edges for this vertex
    viskores::Id neighboursBegin = meshNeighborOffsetsPortal.Get(vertex);
    viskores::Id neighboursEnd = (vertex < mesh.GetNumberOfVertices() - 1)
      ? meshNeighborOffsetsPortal.Get(vertex + 1)
      : mesh.NeighborConnectivity.GetNumberOfValues();

    // now loop through the neighbours
    for (viskores::Id whichNbr = neighboursBegin; whichNbr != neighboursEnd; ++whichNbr)
    { // per neighbour
      viskores::Id nbrID = meshNeighborConnectivityPortal.Get(whichNbr);
      // skip if the neighbour is higher (use sim. of simp.)
      if ((meshSortedValuesPortal.Get(nbrID) > meshSortedValuesPortal.Get(vertex)) ||
          ((meshSortedValuesPortal.Get(nbrID) == meshSortedValuesPortal.Get(vertex)) &&
           (nbrID > vertex)))
        // output the edge
        outStream << "\tr" << std::setw(1) << nbrID << " -> r" << std::setw(1) << vertex
                  << " [penwidth=3]" << std::endl;
      else
        outStream << "\tr" << std::setw(1) << vertex << " -> r" << std::setw(1) << nbrID
                  << " [dir=back,penwidth=3]" << std::endl;
    } // per neighbour

  } // per vertex

  // close the graph
  outStream << "\t}" << std::endl;

  // now return the string
  return outStream.str();
} // ContourTreeMeshDotGraphPrint()


// 3.	All purpose routine to dump out the contents for comparison with contour tree
VISKORES_CONT
template <typename T, typename StorageType, typename MeshType, typename MeshBoundaryExecObjType>
std::string BoundaryTreeDotGraphPrint(
  const std::string& label, // the label to use as title for the graph
  MeshType& mesh,           // the underlying mesh for the contour tree
  MeshBoundaryExecObjType&
    meshBoundaryExecutionObject, // the boundary description need to determin if a vertex is on the boundary
  viskores::worklet::contourtree_distributed::BoundaryTree&
    boundaryTree, // the boundary tree itself
  const viskores::worklet::contourtree_augmented::mesh_dem::IdRelabeler*
    localToGlobalIdRelabeler, // relabler needed to compute global ids
  const viskores::cont::ArrayHandle<T, StorageType>& field,
  const viskores::Id showMask = viskores::worklet::contourtree_distributed::
    SHOW_BOUNDARY_TREE_ALL, // mask with flags for what elements to show
  const bool printHeaderAndFooter = true)
{ // BoundaryTreeDotGraphPrint()
  // initialise a string stream to capture the output
  std::stringstream outStream;

  // now grab portals to all the variables we will need
  auto vertexIndexPortal = boundaryTree.VertexIndex.ReadPortal();
  auto superarcsPortal = boundaryTree.Superarcs.ReadPortal();

  // if requested
  if (printHeaderAndFooter)
  { // print header
    // print the header information
    outStream << "digraph BoundaryTree\n\t{\n";
    outStream << "\tlabel=\"" << std::setw(1) << label << "\"\n\tlabelloc=t\n\tfontsize=30\n";
    outStream << "\t// Nodes" << std::endl;
  } // print header

  // prercompute the mesh boundary
  // TODO: This should be done in parallel. We have the basic code but for printing this is fine for now
  viskores::cont::ArrayHandle<bool> liesOnBoundary;
  {
    viskores::worklet::contourtree_augmented::IdArrayType boundaryVertexArray;
    viskores::worklet::contourtree_augmented::IdArrayType boundaryVertexSortIndexArray;
    mesh.GetBoundaryVertices(boundaryVertexArray,          // output
                             boundaryVertexSortIndexArray, // output
                             &meshBoundaryExecutionObject  //input
    );
    // TODO Add option for boundary critical only

    auto boundaryVertexArrayPortal = boundaryVertexArray.ReadPortal();
    // viskores::cont::ArrayHandle<viskores::Range> rangeArray = viskores::cont::ArrayRangeCompute(mesh.SortOrder);
    // viskores::Id maxId = static_cast<viskores::Id>(rangeArray.ReadPortal().Get(0).Max) + 1;
    liesOnBoundary.Allocate(mesh.SortOrder.GetNumberOfValues());
    auto liesOnBoundaryWritePortal = liesOnBoundary.WritePortal();
    viskores::cont::Algorithm::Copy(
      viskores::cont::ArrayHandleConstant<bool>(false, liesOnBoundary.GetNumberOfValues()),
      liesOnBoundary);
    for (viskores::Id i = 0; i < boundaryVertexArray.GetNumberOfValues(); ++i)
    {
      liesOnBoundaryWritePortal.Set(boundaryVertexArrayPortal.Get(i), true);
    }
  }
  auto liesOnBoundaryPortal = liesOnBoundary.ReadPortal();

  // loop through all nodes
  auto meshSortOrderPortal = mesh.SortOrder.ReadPortal();
  auto globalIds = mesh.GetGlobalIdsFromSortIndices(mesh.SortOrder, localToGlobalIdRelabeler);
  auto globalIdsPortal = globalIds.ReadPortal();
  auto dataValuesPortal = field.ReadPortal();
  for (viskores::Id node = 0; node < boundaryTree.VertexIndex.GetNumberOfValues(); node++)
  { // per node
    // work out the node and it's value
    viskores::Id sortID = vertexIndexPortal.Get(node);
    viskores::Id regularID = meshSortOrderPortal.Get(sortID);
    // NOTE: globalIdsPortal already looked up by meshSortOrder so we need to
    //       look up globalID now by node not sortID
    viskores::Id globalID = globalIdsPortal.Get(node);
    auto dataValue = dataValuesPortal.Get(regularID);

    // print the vertex (using global ID to simplify things for the residue)
    outStream << "\tg" << std::setw(1) << globalID;
    outStream << "[style=filled,fixedsize=true,fontname=\"Courier\",margin=\"0.02,0.02\",height="
                 "\"1.7in\",width=\"1.7in\",penwidth=5,shape=circle";
    outStream << ",fillcolor=" << (liesOnBoundaryPortal.Get(regularID) ? "grey" : "white");
    outStream << ",label=\"";
    if (showMask & viskores::worklet::contourtree_distributed::SHOW_BOUNDARY_TREE_VERTEX_ID)
      outStream << "b " << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH)
                << node << "\\n";
    if (showMask & viskores::worklet::contourtree_distributed::SHOW_BOUNDARY_TREE_GLOBAL_ID)
      outStream << "g " << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH)
                << globalID << "\\n";
    if (showMask & viskores::worklet::contourtree_distributed::SHOW_BOUNDARY_TREE_DATA_VALUE)
      outStream << "v " << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH)
                << dataValue << "\\n";
    if (showMask & viskores::worklet::contourtree_distributed::SHOW_BOUNDARY_TREE_MESH_REGULAR_ID)
      outStream << "r " << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH)
                << regularID << "\\n";
    if (showMask & viskores::worklet::contourtree_distributed::SHOW_BOUNDARY_TREE_MESH_SORT_ID)
      outStream << "s " << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH)
                << sortID << "\\n";
    outStream << "\"];\n";
  } // per vertex
  // always show the null node
  outStream << "\t// Null Node" << std::endl;
  outStream
    << "\tNULL "
       "[style=filled,fixedsize=true,fontname=\"Courier\",margin=\"0.02,0.02\",height=\"0.5in\","
       "width=\"0.5in\",penwidth=1,shape=circle,fillcolor=white,color=black,label=\"NULL\"]"
    << std::endl;

  // now print out the edges
  for (viskores::Id node = 0; node < boundaryTree.Superarcs.GetNumberOfValues(); node++)
  { // per node
    // retrieve global ID of node
    viskores::Id sortID = vertexIndexPortal.Get(node);
    viskores::Id globalID = globalIdsPortal.Get(sortID);

    // retrieve ID of target supernode
    viskores::Id to = superarcsPortal.Get(node);

    // if this is true, it is the last pruned vertex & is omitted
    if (viskores::worklet::contourtree_augmented::NoSuchElement(to))
      outStream << "\tg" << std::setw(1) << globalID << " -> NULL [penwidth=2";
    else
    { // actual superarc
      viskores::Id toSort = vertexIndexPortal.Get(to);
      viskores::Id toGlobal = globalIdsPortal.Get(toSort);
      if (node < to)
        outStream << "\tg" << std::setw(1) << toGlobal << " -> g" << std::setw(1) << globalID
                  << " [dir=back,penwidth=3";
      else
        outStream << "\tg" << std::setw(1) << globalID << " -> g" << std::setw(1) << toGlobal
                  << " [penwidth=3";
    } // actual superarc

    // now tidy up
    if (showMask & viskores::worklet::contourtree_distributed::SHOW_BOUNDARY_TREE_ARC_ID)
      outStream << ",label=\"BA" << node << "\"";
    outStream << "]" << std::endl;
  } // per node

  if (printHeaderAndFooter)
  { // print footer
    outStream << "\t}" << std::endl;
  } // print footer
  // now return the string
  return outStream.str();
} // BoundaryTreeDotGraphPrint()


// Routines for InteriorForest:
// 4.	All purpose routine to dump out the contents for comparison with contour tree
VISKORES_CONT
template <typename T, typename StorageType, typename MeshType, typename MeshBoundaryExecObjType>
std::string InteriorForestDotGraphPrint(
  const std::string& label, // the label to use as title for the graph
  viskores::worklet::contourtree_distributed::InteriorForest& forest, // the forest in question
  viskores::worklet::contourtree_augmented::ContourTree&
    contourTree, // the contour tree to which it belongs
  viskores::worklet::contourtree_distributed::BoundaryTree& boundaryTree, // the boundary tree
  MeshType& mesh, // the underlying mesh for the contour tree
  MeshBoundaryExecObjType&
    meshBoundaryExecutionObject, // the boundary description need to determin if a vertex is on the boundary
  const viskores::worklet::contourtree_augmented::mesh_dem::IdRelabeler*
    localToGlobalIdRelabeler, // relabler needed to compute global ids
  const viskores::cont::ArrayHandle<T, StorageType>& field,
  const viskores::Id& showMask =
    viskores::worklet::contourtree_distributed::SHOW_INTERIOR_FOREST_ALL) // mask for what to show
{ // InteriorForestDotGraphPrint()
  // initialise a string stream to capture the output
  std::stringstream outStream;

  // now grab portals to all the variables we will need
  auto supernodesPortal = contourTree.Supernodes.ReadPortal();
  auto superarcsPortal = contourTree.Superarcs.ReadPortal();
  auto forestAbovePortal = forest.Above.ReadPortal();
  auto forestBelowPortal = forest.Below.ReadPortal();
  auto forestIsNecessaryPortal = forest.IsNecessary.ReadPortal();

  // print the header information
  outStream << "digraph InteriorForest\n\t{\n";
  outStream << "\tlabel=\"" << std::setw(1) << label << "\"\n\tlabelloc=t\n\tfontsize=30\n";
  outStream << "\t// Nodes" << std::endl;

  // call the boundary tree routine first, telling it to omit the header and footer
  // note that since we define our mask in the same bits as BRACT, we can pass through the mask
  outStream << BoundaryTreeDotGraphPrint(
    label,
    mesh,
    meshBoundaryExecutionObject,
    boundaryTree,
    localToGlobalIdRelabeler,
    field,
    viskores::worklet::contourtree_distributed::SHOW_INTERIOR_FOREST_ALL,
    false);

  // now we need to show the forest and how it relates to the boundary tree
  // note - we will ignore the boundary tree Mesh Indices array for now
  auto meshSortOrderPortal = mesh.SortOrder.ReadPortal();
  auto globalIds = mesh.GetGlobalIdsFromSortIndices(mesh.SortOrder, localToGlobalIdRelabeler);
  auto globalIdsPortal = globalIds.ReadPortal();
  auto dataValuesPortal = field.ReadPortal();

  // loop through all of the supernodes in the contour tree
  for (viskores::Id supernode = 0; supernode < contourTree.Supernodes.GetNumberOfValues();
       supernode++)
  { // per supernode
    // retrieve the various IDs for the supernode
    viskores::Id sortID = supernodesPortal.Get(supernode);
    viskores::Id regularID = meshSortOrderPortal.Get(sortID);
    // NOTE: globalIdsPortal already looked up by meshSortOrder so we need to
    //       look up globalID now by supernode not sortID
    viskores::Id globalID = globalIdsPortal.Get(supernode);
    auto dataValue = dataValuesPortal.Get(regularID);

    // vertices marked "necessary" are in the interior of the BRACT, but not all are in the BRACT
    // but the ones in the BRACT always have above/below pointing to themselves, so we test that
    if (forestIsNecessaryPortal.Get(supernode) && (forestAbovePortal.Get(supernode) == globalID) &&
        (forestBelowPortal.Get(supernode) == globalID))
      continue;

    // now print out the node
    // print the vertex
    outStream << "\tg" << std::setw(1) << globalID;
    outStream << "[style=filled,fixedsize=true,fontname=\"Courier\",margin=\"0.02,0.02\",height="
                 "\"1.7in\",width=\"1.7in\",penwidth=5,shape=circle";
    outStream << ",fillcolor=white";
    outStream << ",label=\"";
    if (showMask & viskores::worklet::contourtree_distributed::SHOW_INTERIOR_FOREST_VERTEX_ID)
      outStream << "SN" << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH)
                << supernode << "\\n";
    if (showMask & viskores::worklet::contourtree_distributed::SHOW_INTERIOR_FOREST_GLOBAL_ID)
      outStream << "g " << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH)
                << globalID << "\\n";
    if (showMask & viskores::worklet::contourtree_distributed::SHOW_INTERIOR_FOREST_DATA_VALUE)
      outStream << "v " << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH)
                << dataValue << "\\n";
    if (showMask & viskores::worklet::contourtree_distributed::SHOW_INTERIOR_FOREST_MESH_REGULAR_ID)
      outStream << "r " << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH)
                << regularID << "\\n";
    if (showMask & viskores::worklet::contourtree_distributed::SHOW_INTERIOR_FOREST_MESH_SORT_ID)
      outStream << "s " << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH)
                << sortID << "\\n";
    outStream << "\"];\n";
  } // per supernode

  // now loop through the superarcs in the contour tree
  for (viskores::Id supernode = 0; supernode < contourTree.Supernodes.GetNumberOfValues();
       supernode++)
  { // per supernode / superarc
    // retrieve the various IDs for the supernode
    viskores::Id sortID = supernodesPortal.Get(supernode);
    viskores::Id globalID = globalIdsPortal.Get(sortID);

    // for nodes not necessary, show their superarc
    if (!forestIsNecessaryPortal.Get(supernode))
    { // not necessary
      // retrieve the target of its superarc
      viskores::Id superarc = superarcsPortal.Get(supernode);

      // check to see if it exists
      if (viskores::worklet::contourtree_augmented::NoSuchElement(superarc))
        continue;

      // separate out the ID
      viskores::Id superTo = viskores::worklet::contourtree_augmented::MaskedIndex(superarc);
      viskores::Id toSort = supernodesPortal.Get(superTo);
      viskores::Id toGlobal = globalIdsPortal.Get(toSort);

      // then print out the edge
      if (contourtree_augmented::IsAscending(superTo))
        outStream << "\tg" << std::setw(1) << toGlobal << " -> g" << globalID
                  << "[dir=back,penwidth=3]" << std::endl;
      else
        outStream << "\tg" << std::setw(1) << globalID << " -> g" << toGlobal << "[penwidth=3]"
                  << std::endl;
    } // not necessary
    else if ((forestAbovePortal.Get(supernode) != globalID) ||
             (forestBelowPortal.Get(supernode) != globalID))
    { // attachment point
      // all others are attachment points and have a valid above / below
      outStream << "\tg" << std::setw(1) << forestAbovePortal.Get(supernode) << " -> g"
                << std::setw(1) << globalID << "[penwidth=1,style=dotted,label=above,dir=back]"
                << std::endl;
      outStream << "\tg" << std::setw(1) << globalID << " -> g" << forestBelowPortal.Get(supernode)
                << "[penwidth=1,style=dotted,label=below]" << std::endl;
    } // attachment point

  } // per supernode / superarc

  // print the footer
  outStream << "\t}" << std::endl;

  // now return the string
  return outStream.str();
} // InteriorForestDotGraphPrint()


// 5.	Routine to print regular/super/hyper structure with similar options to contour tree
VISKORES_CONT
template <typename FieldType>
// template <typename FieldType, typename VectorType>
std::string HierarchicalContourTreeDotGraphPrint(
  const std::string& label, // the label to use as title for the graph
  const viskores::worklet::contourtree_distributed::HierarchicalContourTree<FieldType>&
    hierarchicalTree, // the hierarchical contour tree itself
  const viskores::Id showMask = viskores::worklet::contourtree_distributed::
    SHOW_HIERARCHICAL_STANDARD) // mask with flags for what elements to show
// 		const unsigned long showMask = viskores::worklet::contourtree_distributed::SHOW_HIERARCHICAL_STANDARD,		// mask with flags for what elements to show
// 		const VectorType &perNodeValues = VectorType(0))														// an arbitrary vector of values
{ // HierarchicalContourTreeDotGraphPrint()
  // initialise a string stream to capture the output
  std::stringstream outStream;

  // now grab portals to all the variables we will need
  auto regularNodeGlobalIdsPortal = hierarchicalTree.RegularNodeGlobalIds.ReadPortal();
  auto dataValuesPortal = hierarchicalTree.DataValues.ReadPortal();
  auto regularNodeSortOrderPortal = hierarchicalTree.RegularNodeSortOrder.ReadPortal();
  auto regular2supernodePortal = hierarchicalTree.Regular2Supernode.ReadPortal();
  auto superparentsPortal = hierarchicalTree.Superparents.ReadPortal();
  auto supernodesPortal = hierarchicalTree.Supernodes.ReadPortal();
  auto superarcsPortal = hierarchicalTree.Superarcs.ReadPortal();
  auto hyperparentsPortal = hierarchicalTree.Hyperparents.ReadPortal();
  auto super2hypernodePortal = hierarchicalTree.Super2Hypernode.ReadPortal();
  auto whichRoundPortal = hierarchicalTree.WhichRound.ReadPortal();
  auto whichIterationPortal = hierarchicalTree.WhichIteration.ReadPortal();
  auto hypernodesPortal = hierarchicalTree.Hypernodes.ReadPortal();
  auto hyperarcsPortal = hierarchicalTree.Hyperarcs.ReadPortal();

  // TODO:  Resolve passing conventions for per node values
  // 	auto perNodeValuesPortal			= perNodeValues.ReadPortal();

  // work out how long the computed value is
  // 	int nodeValueType = viskores::worklet::contourtree_distributed::BAD_PER_NODE_VALUES;
  // 	viskores::Id perNodeSize = perNodeValues.GetNumberOfValues();
  // 	if (perNodeSize == 0)
  // 		nodeValueType = viskores::worklet::contourtree_distributed::NO_PER_NODE_VALUES;
  // 	else if (perNodeSize == hierarchicalTree.Nodes.GetNumberOfValues())
  // 		nodeValueType = viskores::worklet::contourtree_distributed::PER_REGULAR_NODE_VALUES;
  // 	else if (perNodeSize == hierarchicalTree.Supernodes.GetNumberOfValues())
  // 		nodeValueType = viskores::worklet::contourtree_distributed::PER_SUPER_NODE_VALUES;
  // 	else if (perNodeSize == hierarchicalTree.Hypernodes.GetNumberOfValues())
  // 		nodeValueType = viskores::worklet::contourtree_distributed::PER_HYPER_NODE_VALUES;
  // 	else
  // 		{ // error message
  // 		outStream << "ERROR in HierarchicalContourTreeDotGraphPrint().\n";
  // 		outStream << "Per node values array must be empty, or\n";
  // 		outStream << "Same length as regular nodes (" << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH) << hierarchicalTree.Nodes.GetNumberOfValues() << "), or\n";
  // 		outStream << "Same length as super nodes   (" << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH) << hierarchicalTree.Supernodes.GetNumberOfValues() << "), or\n";
  // 		outStream << "Same length as hyper nodes   (" << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH) << hierarchicalTree.Hypernodes.GetNumberOfValues() << ")\n";
  // 		outStream << "Actual length was            (" << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH) << perNodeValues.GetNumberOfValues() << ")\n";
  // 		} // error message

  // print the header information
  outStream << "digraph HierarchicalContourTree\n\t{\n";
  outStream << "\tlabel=\"" << std::setw(1) << label << "\"\n\tlabelloc=t\n\tfontsize=30\n";
  outStream << "\t// Nodes" << std::endl;

  // loop through all of the nodes in the regular list
  for (viskores::Id node = 0; node < hierarchicalTree.RegularNodeGlobalIds.GetNumberOfValues();
       node++)
  { // per node
    // Since the superparent for an attachment point is set to another supernode, reset the calculation here
    //viskores::Id whichRound = maskedIndex(hierarchicalTree.WhichRound[superID]);
    //viskores::Id whichIteration = maskedIndex(hierarchicalTree.WhichIteration[superID]);

    // the regular ID in this case is the node itself
    viskores::Id regularID = node;

    // for a sort ID, we will take the sort order vector
    viskores::Id sortID = regularNodeSortOrderPortal.Get(node);

    // retrieve the global ID
    viskores::Id globalID = regularNodeGlobalIdsPortal.Get(node);

    // retrieve the values
    auto dataValue = dataValuesPortal.Get(node);

    // retrieve the superparent
    viskores::Id superparent = superparentsPortal.Get(node);

    // and retrieve the iteration #
    viskores::Id whichRound =
      viskores::worklet::contourtree_augmented::MaskedIndex(whichRoundPortal.Get(superparent));
    viskores::Id whichIteration =
      viskores::worklet::contourtree_augmented::MaskedIndex(whichIterationPortal.Get(superparent));

    // work out the super ID & hyper ID
    viskores::Id superID = regular2supernodePortal.Get(node);
    viskores::Id hyperparent = viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT;
    viskores::Id hyperID = viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT;
    viskores::Id nodeType = viskores::worklet::contourtree_distributed::NODE_TYPE_REGULAR;

    // test for super
    if (!viskores::worklet::contourtree_augmented::NoSuchElement(superID))
    { // at least super
      // set hyperparent
      hyperparent = hyperparentsPortal.Get(superID);
      // set nodetype
      nodeType = viskores::worklet::contourtree_distributed::NODE_TYPE_SUPER;
      // retrieve hyper ID
      hyperID = super2hypernodePortal.Get(superparent);
      // test it
      if (!viskores::worklet::contourtree_augmented::NoSuchElement(hyperID))
      { // hyper node
        nodeType = viskores::worklet::contourtree_distributed::NODE_TYPE_HYPER;
      } // hyper node
    }   // at least super

    // now, if we don't want the regular nodes, we want to skip them entirely, so
    bool showNode = false;
    // regular structure always shows all nodes
    if (showMask & viskores::worklet::contourtree_distributed::SHOW_REGULAR_STRUCTURE)
      showNode = true;
    // super structure shows super & hyper nodes only
    else if (showMask & viskores::worklet::contourtree_distributed::SHOW_SUPER_STRUCTURE)
      showNode = (nodeType != viskores::worklet::contourtree_distributed::NODE_TYPE_REGULAR);
    else if (showMask & viskores::worklet::contourtree_distributed::SHOW_HYPER_STRUCTURE)
      showNode = (nodeType == viskores::worklet::contourtree_distributed::NODE_TYPE_HYPER);

    // if we didn't set the flag, skip the node
    if (!showNode)
      continue;

    // print the vertex ID, which should be the sort ID & needs to be left-justified to work
    outStream << "\ts" << std::setw(1) << sortID;

    // print the style characteristics - node is filled and fixed size
    outStream << " [style=filled,fixedsize=true,fontname=\"Courier\",margin=\"0.02,0.02\"";
    // specify the style based on the type of node
    if (nodeType == viskores::worklet::contourtree_distributed::NODE_TYPE_REGULAR)
      outStream << ",height=\"1.7in\",width=\"1.7in\",penwidth=5";
    else if (nodeType == viskores::worklet::contourtree_distributed::NODE_TYPE_SUPER)
      outStream << ",height=\"2.5in\",width=\"2.5in\",penwidth=10";
    else if (nodeType == viskores::worklet::contourtree_distributed::NODE_TYPE_HYPER)
      outStream << ",height=\"2.5in\",width=\"2.5in\",penwidth=15";

    // shape should always be circular.
    outStream << ",shape=circle";

    // after setting the flag, its easy
    outStream << ",fillcolor=white";

    // stroke colour depends on which round
    outStream << ",color="
              << viskores::worklet::contourtree_augmented::NODE_COLORS
                   [whichRound % viskores::worklet::contourtree_augmented::N_NODE_COLORS];

    // start printing the label
    outStream << ",label=\"";
    // print the global ID
    if (showMask & viskores::worklet::contourtree_distributed::SHOW_GLOBAL_ID)
      outStream << "g " << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH)
                << globalID << "\\n";
    // print the value
    if (showMask & viskores::worklet::contourtree_distributed::SHOW_DATA_VALUE)
      outStream << "v " << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH)
                << dataValue << "\\n";
    // print the regular & sort IDs
    if (showMask & viskores::worklet::contourtree_distributed::SHOW_MESH_REGULAR_ID)
      outStream << "r " << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH)
                << regularID << "\\n";
    if (showMask & viskores::worklet::contourtree_distributed::SHOW_MESH_SORT_ID)
      outStream << "s " << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH)
                << sortID << "\\n";
    // print the superparent
    if (showMask & viskores::worklet::contourtree_distributed::SHOW_SUPERPARENT)
      outStream << "sp" << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH)
                << superparent << "\\n";

    // add arbitrary per node value if it is regular in nature
    // 		if ((showMask & viskores::worklet::contourtree_distributed::SHOW_EXTRA_DATA) && (nodeValueType == viskores::worklet::contourtree_distributed::PER_REGULAR_NODE_VALUES))
    // 			outStream << "x " << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH) << perNodeValues[regularID] << "\\n";

    // we now want to add labelling information specific to supernodes, but also present in hypernodes
    if (nodeType != viskores::worklet::contourtree_distributed::NODE_TYPE_REGULAR)
    { // at least super

      // print the super node ID
      if (showMask & viskores::worklet::contourtree_distributed::SHOW_SUPERNODE_ID)
        outStream << "SN" << std::setw(INDEX_WIDTH) << superID << "\\n";

      // print the hyperparent as well
      if (showMask & viskores::worklet::contourtree_distributed::SHOW_HYPERPARENT)
        outStream << "HP" << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH)
                  << hyperparent << "\\n";
      if (showMask & viskores::worklet::contourtree_distributed::SHOW_ITERATION)
        outStream << "IT" << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH)
                  << whichRound << "." << whichIteration << "\\n";

      // add arbitrary per node value if it is super in nature
      // 			if ((showMask & viskores::worklet::contourtree_distributed::SHOW_EXTRA_DATA) && (nodeValueType == PER_SUPER_NODE_VALUES))
      // 				outStream << "X " << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH) << perNodeValues[superID] << "\\n";
    } // at least super

    // now add even more for hypernodes
    if (nodeType == viskores::worklet::contourtree_distributed::NODE_TYPE_HYPER)
    { // hyper node
      if (showMask & viskores::worklet::contourtree_distributed::SHOW_HYPERNODE_ID)
        outStream << "HN" << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH)
                  << hyperID << "\\n";

      // add arbitrary per node value if it is hyper in nature
      // 			if ((showMask & viskores::worklet::contourtree_distributed::SHOW_EXTRA_DATA) && (nodeValueType == viskores::worklet::contourtree_distributed::PER_HYPER_NODE_VALUES))
      // 				outStream << "X " << std::setw(viskores::worklet::contourtree_distributed::INDEX_WIDTH) << perNodeValues[hyperID] << "\\n";
    } // hyper node

    outStream << "\"]" << std::endl;
  } // per node

  // always show the null node
  outStream << "\t// Null Node" << std::endl;
  outStream
    << "\tNULL "
       "[style=filled,fixedsize=true,fontname=\"Courier\",margin=\"0.02,0.02\",height=\"0.5in\","
       "width=\"0.5in\",penwidth=1,shape=circle,fillcolor=white,color=black,label=\"NULL\"]"
    << std::endl;

  // now show superarc nodes
  outStream << "\t// Superarc nodes\n";
  // now repeat to create nodes for the middle of each superarc (to represent the superarcs themselves)
  for (viskores::Id superarc = 0; superarc < hierarchicalTree.Superarcs.GetNumberOfValues();
       superarc++)
  { // per superarc
    // retrieve ID of target superarc
    viskores::Id superarcFrom = superarc;
    viskores::Id superarcTo = superarcsPortal.Get(superarcFrom);

    // and retrieve the iteration #
    viskores::Id whichRound =
      viskores::worklet::contourtree_augmented::MaskedIndex(whichRoundPortal.Get(superarcFrom));

    // if this is true, it is the last pruned vertex (attachment point or root) and has no superarc vertex
    if (viskores::worklet::contourtree_augmented::NoSuchElement(superarcTo))
      continue;

    // print the superarc vertex
    outStream << "\tSA" << std::setw(1) << superarc;
    outStream << "[shape=circle,color="
              << viskores::worklet::contourtree_augmented::NODE_COLORS
                   [whichRound % viskores::worklet::contourtree_augmented::N_NODE_COLORS];
    outStream << ",fillcolor=white";
    outStream << ",fixedsize=true";
    outStream << ",height=0.8,width=0.8";
    outStream << ",label=\"";
    if (showMask & viskores::worklet::contourtree_distributed::SHOW_SUPERARC_ID)
      outStream << "SA" << std::setw(1) << superarc;
    outStream << "\"];" << std::endl;
  } // per superarc

  // now show regular arcs - since we do not maintain a sort, they will all attach to the parent superarc
  if (showMask & viskores::worklet::contourtree_distributed::SHOW_REGULAR_STRUCTURE)
  { // showing regular nodes
    outStream << "\t// Superarc nodes\n";
    for (viskores::Id regularID = 0;
         regularID < hierarchicalTree.RegularNodeGlobalIds.GetNumberOfValues();
         regularID++)
    { // per regular node
      // if it has a superID, then we don't want to attach it to a superarc
      if (!viskores::worklet::contourtree_augmented::NoSuchElement(
            regular2supernodePortal.Get(regularID)))
        continue;

      // retrieve the sort ID
      viskores::Id sortID = regularNodeSortOrderPortal.Get(regularID);
      // retrieve the superparent
      viskores::Id superparent = superparentsPortal.Get(regularID);

      // and connect to the superarc
      outStream << "\ts" << sortID << " -> SA" << superparent << "[style=dotted]" << std::endl;

    } // per regular node
  }   // showing regular nodes

  if (showMask & viskores::worklet::contourtree_distributed::SHOW_SUPER_STRUCTURE)
  { // showing superstructure
    outStream << "\t// Superarc edges\n";
    // loop through all superarcs to draw them
    for (viskores::Id superarc = 0; superarc < hierarchicalTree.Superarcs.GetNumberOfValues();
         superarc++)
    { // per superarc
      // retrieve ID of target supernode
      viskores::Id superarcFrom = superarc;
      // retrieve the sort ID
      viskores::Id fromRegular = supernodesPortal.Get(superarcFrom);
      viskores::Id fromSort = regularNodeSortOrderPortal.Get(fromRegular);

      // and retrieve the destination
      viskores::Id superarcTo = superarcsPortal.Get(superarcFrom);

      // and retrieve the iteration #
      viskores::Id whichRound =
        viskores::worklet::contourtree_augmented::MaskedIndex(whichRoundPortal.Get(superarcFrom));

      // if this is true, it may be the last pruned vertex
      if (viskores::worklet::contourtree_augmented::NoSuchElement(superarcTo))
      { // no superarc
        // if it occurred on the final round, it's the global root and is shown as the NULL node
        if (whichRound == hierarchicalTree.NumRounds)
          outStream << "\ts" << fromSort << " -> NULL[label=\"SA" << superarc << "\",style=dotted]"
                    << std::endl;
        else
        { // attachment point
          // otherwise, the target is actually a superarc vertex not a supernode vertex
          // so we use the regular ID to retrieve the superparent which tells us which superarc we insert into
          viskores::Id regularFrom = supernodesPortal.Get(superarcFrom);
          superarcTo = superparentsPortal.Get(regularFrom);

          // output a suitable edge
          outStream << "\ts" << fromSort << " -> SA" << superarcTo << "[label=\"S" << superarc
                    << "\",style=dotted,color="
                    << viskores::worklet::contourtree_augmented::NODE_COLORS
                         [whichRound % viskores::worklet::contourtree_augmented::N_NODE_COLORS]
                    << "]" << std::endl;
        } // attachment point
      }   // no superarc
      else
      { // there is a superarc
        // retrieve the ascending flag
        bool ascendingSuperarc = viskores::worklet::contourtree_augmented::IsAscending(superarcTo);

        // strip out the flags
        superarcTo = viskores::worklet::contourtree_augmented::MaskedIndex(superarcTo);

        // retrieve the sort ID for the to end
        viskores::Id toRegular = supernodesPortal.Get(superarcTo);
        viskores::Id toSort = regularNodeSortOrderPortal.Get(toRegular);

        // how we print depends on whether the superarc ascends
        if (ascendingSuperarc)
        { // ascending arc
          outStream << "\ts" << toSort << " -> SA" << superarc << "[label=\"SA" << superarc
                    << "\",dir=back";
          outStream << ",penwidth=3,color="
                    << viskores::worklet::contourtree_augmented::NODE_COLORS
                         [whichRound % viskores::worklet::contourtree_augmented::N_NODE_COLORS]
                    << "]" << std::endl;
          outStream << "\tSA" << superarc << " -> s" << fromSort << "[label=\"SA" << superarc
                    << "\",dir=back";
          outStream << ",penwidth=3,color="
                    << viskores::worklet::contourtree_augmented::NODE_COLORS
                         [whichRound % viskores::worklet::contourtree_augmented::N_NODE_COLORS]
                    << "]" << std::endl;
        } // ascending arc
        else
        { // descending arc
          outStream << "\ts" << fromSort << " -> SA" << superarc << "[label=\"SA" << superarc
                    << "\"";
          outStream << ",penwidth=3,color="
                    << viskores::worklet::contourtree_augmented::NODE_COLORS
                         [whichRound % viskores::worklet::contourtree_augmented::N_NODE_COLORS]
                    << "]" << std::endl;
          outStream << "\tSA" << superarc << " -> s" << toSort << "[label=\"SA" << superarc << "\"";
          outStream << ",penwidth=3,color="
                    << viskores::worklet::contourtree_augmented::NODE_COLORS
                         [whichRound % viskores::worklet::contourtree_augmented::N_NODE_COLORS]
                    << "]" << std::endl;
        } // descending arc
      }   // there is a superarc
    }     // per superarc
  }       // showing superstructure

  if (showMask & viskores::worklet::contourtree_distributed::SHOW_HYPER_STRUCTURE)
  { // show hyperstructure
    outStream << "\t// Hyperarcs\n";
    // now loop through the hyperarcs to draw them
    for (viskores::Id hyperarc = 0; hyperarc < hierarchicalTree.Hyperarcs.GetNumberOfValues();
         hyperarc++)
    { // per hyperarc
      // down convert to a sort ID
      viskores::Id fromSuper = hypernodesPortal.Get(hyperarc);
      viskores::Id fromRegular = supernodesPortal.Get(fromSuper);
      viskores::Id fromSort = regularNodeSortOrderPortal.Get(fromRegular);

      // and retrieve the iteration #
      viskores::Id whichRound =
        viskores::worklet::contourtree_augmented::MaskedIndex(whichRoundPortal.Get(fromSuper));

      // and do the same with the to end
      viskores::Id toSuper = hyperarcsPortal.Get(hyperarc);

      // if this is true, it is the last pruned vertex & connects to NULL
      if (viskores::worklet::contourtree_augmented::NoSuchElement(toSuper))
        outStream << "\ts" << fromSort << " -> NULL[label=\"HA" << hyperarc
                  << "\",penwidth=3.0,style=dotted]" << std::endl;
      else
      { // not the last one
        // otherwise, retrieve the ascending flag
        bool ascendingHyperarc = viskores::worklet::contourtree_augmented::IsAscending(toSuper);

        // strip out the flags
        toSuper = viskores::worklet::contourtree_augmented::MaskedIndex(toSuper);

        // retrieve the sort index
        viskores::Id toRegular = supernodesPortal.Get(toSuper);
        viskores::Id toSort = regularNodeSortOrderPortal.Get(toRegular);

        // how we print depends on whether the hyperarc ascends
        if (ascendingHyperarc)
        { // ascending arc
          outStream << "\ts" << toSort << " -> s" << fromSort << "[label=\"HA" << hyperarc
                    << "\",dir=back";
          outStream << ",penwidth=5.0,color="
                    << viskores::worklet::contourtree_augmented::NODE_COLORS
                         [whichRound % viskores::worklet::contourtree_augmented::N_NODE_COLORS]
                    << "]" << std::endl;
        } // ascending arc
        else
        { // descending arc
          outStream << "\ts" << fromSort << " -> s" << toSort << "[label=\"HA" << hyperarc << "\"";
          outStream << ",penwidth=5.0,color="
                    << viskores::worklet::contourtree_augmented::NODE_COLORS
                         [whichRound % viskores::worklet::contourtree_augmented::N_NODE_COLORS]
                    << "]" << std::endl;
        } // descending arc
      }   // not the last one
    }     // per hyperarc
  }       // show hyperstructure

  // print the footer information
  outStream << "\t}\n";

  // now return the string
  return outStream.str();
} // HierarchicalContourTreeDotGraphPrint()


} // namespace contourtree_distributed
} // namespace worklet
} // namespace viskores

#endif
