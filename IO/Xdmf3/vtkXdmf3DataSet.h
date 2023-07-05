// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXdmf3DataSet
 * @brief   dataset level translation between xdmf3 and vtk
 *
 * This class holds static methods that translate the five atomic data
 * types between vtk and xdmf3.
 *
 * This file is a helper for the vtkXdmf3Reader and vtkXdmf3Writer and
 * not intended to be part of VTK public API
 */

#ifndef vtkXdmf3DataSet_h
#define vtkXdmf3DataSet_h

#include "vtkIOXdmf3Module.h" // For export macro

// clang-format off
#include "vtk_xdmf3.h"
#include VTKXDMF3_HEADER(core/XdmfSharedPtr.hpp)
// clang-format on

#include <string> //Needed only for XdmfArray::getName :(

class XdmfArray;
class XdmfAttribute;
class XdmfGrid;
class XdmfSet;
class XdmfTopologyType;
class XdmfRegularGrid;
class XdmfRectilinearGrid;
class XdmfCurvilinearGrid;
class XdmfUnstructuredGrid;
class XdmfGraph;
class XdmfDomain;

VTK_ABI_NAMESPACE_BEGIN
class vtkXdmf3ArraySelection;
class vtkXdmf3ArrayKeeper;
class vtkDataArray;
class vtkDataObject;
class vtkDataSet;
class vtkImageData;
class vtkRectilinearGrid;
class vtkStructuredGrid;
class vtkUnstructuredGrid;
class vtkPointSet;
class vtkMutableDirectedGraph;
class vtkDirectedGraph;

class VTKIOXDMF3_EXPORT vtkXdmf3DataSet
{
public:
  // Common

  /**
   * Returns a VTK array corresponding to the Xdmf array it is given.
   */
  static vtkDataArray* XdmfToVTKArray(XdmfArray* xArray,
    std::string attrName, // TODO: needed because XdmfArray::getName() misbehaves
    unsigned int preferredComponents = 0, vtkXdmf3ArrayKeeper* keeper = nullptr);

  /**
   * Populates and Xdmf array corresponding to the VTK array it is given
   */
  static bool VTKToXdmfArray(
    vtkDataArray* vArray, XdmfArray* xArray, unsigned int rank = 0, unsigned int* dims = nullptr);

  /**
   * Populates the given VTK DataObject's attribute arrays with the selected
   * arrays from the Xdmf Grid
   */
  static void XdmfToVTKAttributes(vtkXdmf3ArraySelection* fselection,
    vtkXdmf3ArraySelection* cselection, vtkXdmf3ArraySelection* pselection, XdmfGrid* grid,
    vtkDataObject* dObject, vtkXdmf3ArrayKeeper* keeper = nullptr);

  /**
   * Populates the given Xdmf Grid's attribute arrays with the selected
   * arrays from the VTK DataObject
   */
  static void VTKToXdmfAttributes(vtkDataObject* dObject, XdmfGrid* grid);

  ///@{
  /**
   * Helpers for Unstructured Grid translation
   */
  static unsigned int GetNumberOfPointsPerCell(int vtk_cell_type, bool& fail);
  static int GetVTKCellType(shared_ptr<const XdmfTopologyType> topologyType);
  static int GetXdmfCellType(int vtkType);
  ///@}

  ///@{
  /**
   * Helper used in VTKToXdmf to set the time in a Xdmf grid
   */
  static void SetTime(XdmfGrid* grid, double hasTime, double time);
  static void SetTime(XdmfGraph* graph, double hasTime, double time);
  ///@}

  // vtkXdmf3RegularGrid

  /**
   * Populates the VTK data set with the contents of the Xdmf grid
   */
  static void XdmfToVTK(vtkXdmf3ArraySelection* fselection, vtkXdmf3ArraySelection* cselection,
    vtkXdmf3ArraySelection* pselection, XdmfRegularGrid* grid, vtkImageData* dataSet,
    vtkXdmf3ArrayKeeper* keeper = nullptr);

  /**
   * Helper that does topology for XdmfToVTK
   */
  static void CopyShape(
    XdmfRegularGrid* grid, vtkImageData* dataSet, vtkXdmf3ArrayKeeper* keeper = nullptr);

  /**
   * Populates the Xdmf Grid with the contents of the VTK data set
   */
  static void VTKToXdmf(vtkImageData* dataSet, XdmfDomain* domain, bool hasTime, double time,
    const char* name = nullptr);

  // vtkXdmf3RectilinearGrid
  /**
   * Populates the VTK data set with the contents of the Xdmf grid
   */
  static void XdmfToVTK(vtkXdmf3ArraySelection* fselection, vtkXdmf3ArraySelection* cselection,
    vtkXdmf3ArraySelection* pselection, XdmfRectilinearGrid* grid, vtkRectilinearGrid* dataSet,
    vtkXdmf3ArrayKeeper* keeper = nullptr);

  /**
   * Helper that does topology for XdmfToVTK
   */
  static void CopyShape(
    XdmfRectilinearGrid* grid, vtkRectilinearGrid* dataSet, vtkXdmf3ArrayKeeper* keeper = nullptr);

  /**
   * Populates the Xdmf Grid with the contents of the VTK data set
   */
  static void VTKToXdmf(vtkRectilinearGrid* dataSet, XdmfDomain* domain, bool hasTime, double time,
    const char* name = nullptr);

  // vtkXdmf3CurvilinearGrid
  /**
   * Populates the VTK data set with the contents of the Xdmf grid
   */
  static void XdmfToVTK(vtkXdmf3ArraySelection* fselection, vtkXdmf3ArraySelection* cselection,
    vtkXdmf3ArraySelection* pselection, XdmfCurvilinearGrid* grid, vtkStructuredGrid* dataSet,
    vtkXdmf3ArrayKeeper* keeper = nullptr);

  /**
   * Helper that does topology for XdmfToVTK
   */
  static void CopyShape(
    XdmfCurvilinearGrid* grid, vtkStructuredGrid* dataSet, vtkXdmf3ArrayKeeper* keeper = nullptr);

  /**
   * Populates the Xdmf Grid with the contents of the VTK data set
   */
  static void VTKToXdmf(vtkStructuredGrid* dataSet, XdmfDomain* domain, bool hasTime, double time,
    const char* name = nullptr);

  // vtkXdmf3UnstructuredGrid
  /**
   * Populates the VTK data set with the contents of the Xdmf grid
   */
  static void XdmfToVTK(vtkXdmf3ArraySelection* fselection, vtkXdmf3ArraySelection* cselection,
    vtkXdmf3ArraySelection* pselection, XdmfUnstructuredGrid* grid, vtkUnstructuredGrid* dataSet,
    vtkXdmf3ArrayKeeper* keeper = nullptr);

  /**
   * Helper that does topology for XdmfToVTK
   */
  static void CopyShape(XdmfUnstructuredGrid* grid, vtkUnstructuredGrid* dataSet,
    vtkXdmf3ArrayKeeper* keeper = nullptr);

  /**
   * Populates the Xdmf Grid with the contents of the VTK data set
   */
  static void VTKToXdmf(vtkPointSet* dataSet, XdmfDomain* domain, bool hasTime, double time,
    const char* name = nullptr);

  // vtkXdmf3Graph
  /**
   * Populates the VTK graph with the contents of the Xdmf grid
   */
  static void XdmfToVTK(vtkXdmf3ArraySelection* fselection, vtkXdmf3ArraySelection* cselection,
    vtkXdmf3ArraySelection* pselection, XdmfGraph* grid, vtkMutableDirectedGraph* dataSet,
    vtkXdmf3ArrayKeeper* keeper = nullptr);

  /**
   * Populates the Xdmf Grid with the contents of the VTK data set
   */
  static void VTKToXdmf(vtkDirectedGraph* dataSet, XdmfDomain* domain, bool hasTime, double time,
    const char* name = nullptr);

  // Side Sets

  /**
   * Populates the given VTK DataObject's attribute arrays with the selected
   * arrays from the Xdmf Grid
   */
  static void XdmfToVTKAttributes(
    /*
        vtkXdmf3ArraySelection *fselection,
        vtkXdmf3ArraySelection *cselection,
        vtkXdmf3ArraySelection *pselection,
    */
    XdmfSet* grid, vtkDataObject* dObject, vtkXdmf3ArrayKeeper* keeper = nullptr);

  /**
   * Extracts numbered subset out of grid (grid corresponds to dataSet),
   * and fills in subSet with it.
   */
  static void XdmfSubsetToVTK(XdmfGrid* grid, unsigned int setnum, vtkDataSet* dataSet,
    vtkUnstructuredGrid* subSet, vtkXdmf3ArrayKeeper* keeper = nullptr);

  /**
   * Converts XDMF topology type, finite element family and degree
   * into an equivalent (or approximative) representation via VTK cell
   * type.
   */
  static int GetVTKFiniteElementCellType(unsigned int element_degree,
    const std::string& element_family, shared_ptr<const XdmfTopologyType> topologyType);

  /**
   * Parses finite element function defined in Attribute.
   *
   * This method changes geometry stored in vtkDataObject
   * and adds Point/Cell data field.
   *
   * XdmfAttribute must contain 2 arrays - one is the XdmfAttribute itself and
   * remaining one the auxiliary array. Interpretation of the arrays is
   * described in XDMF wiki page http://www.xdmf.org/index.php/XDMF_Model_and_Format#Attribute
   *
   */
  static void ParseFiniteElementFunction(vtkDataObject* dObject,
    shared_ptr<XdmfAttribute> xmfAttribute, vtkDataArray* array, XdmfGrid* grid,
    vtkXdmf3ArrayKeeper* keeper = nullptr);
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkXdmf3DataSet.h
