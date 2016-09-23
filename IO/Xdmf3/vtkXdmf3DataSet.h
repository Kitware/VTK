/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXdmf3DataSet.h
  Language:  C++

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
#include "XdmfSharedPtr.hpp"
#include <string> //Needed only for XdmfArray::getName :(

class vtkXdmf3ArraySelection;
class vtkXdmf3ArrayKeeper;
class XdmfArray;
class vtkDataArray;
class XdmfGrid;
class vtkDataObject;
class XdmfSet;
class vtkDataSet;
class XdmfTopologyType;
class XdmfRegularGrid;
class vtkImageData;
class XdmfRectilinearGrid;
class vtkRectilinearGrid;
class XdmfCurvilinearGrid;
class vtkStructuredGrid;
class XdmfUnstructuredGrid;
class vtkUnstructuredGrid;
class vtkPointSet;
class XdmfGraph;
class vtkMutableDirectedGraph;
class vtkDirectedGraph;
class XdmfDomain;


class VTKIOXDMF3_EXPORT vtkXdmf3DataSet
{
public:

  // Common

  /**
   * Returns a VTK array corresponding to the Xdmf array it is given.
   */
  static vtkDataArray *XdmfToVTKArray(
    XdmfArray* xArray,
    std::string attrName,//TODO: needed because XdmfArray::getName() misbehaves
    unsigned int preferredComponents = 0,
    vtkXdmf3ArrayKeeper *keeper=NULL);

  /**
   * Populates and Xdmf array corresponding to the VTK array it is given
   */
  static bool VTKToXdmfArray(
    vtkDataArray *vArray,
    XdmfArray* xArray,
    unsigned int rank = 0, unsigned int *dims = NULL);

  /**
   * Populates the given VTK DataObject's attribute arrays with the selected
   * arrays from the Xdmf Grid
   */
  static void XdmfToVTKAttributes(
    vtkXdmf3ArraySelection *fselection,
    vtkXdmf3ArraySelection *cselection,
    vtkXdmf3ArraySelection *pselection,
    XdmfGrid *grid, vtkDataObject *dObject,
    vtkXdmf3ArrayKeeper *keeper=NULL);

  /**
   * Populates the given Xdmf Grid's attribute arrays with the selected
   * arrays from the VTK DataObject
   */
  static void VTKToXdmfAttributes(vtkDataObject *dObject, XdmfGrid *grid);

  //@{
  /**
   * Helpers for Unstructured Grid translation
   */
  static unsigned int GetNumberOfPointsPerCell(int vtk_cell_type, bool &fail);
  static int GetVTKCellType(shared_ptr<const XdmfTopologyType> topologyType);
  static int GetXdmfCellType(int vtkType);
  //@}

  //@{
  /**
   * Helper used in VTKToXdmf to set the time in a Xdmf grid
   */
  static void SetTime(XdmfGrid *grid, double hasTime, double time);
  static void SetTime(XdmfGraph *graph, double hasTime, double time);
  //@}

  //vtkXdmf3RegularGrid

  /**
   * Populates the VTK data set with the contents of the Xdmf grid
   */
  static void XdmfToVTK(
    vtkXdmf3ArraySelection *fselection,
    vtkXdmf3ArraySelection *cselection,
    vtkXdmf3ArraySelection *pselection,
    XdmfRegularGrid *grid,
    vtkImageData *dataSet,
    vtkXdmf3ArrayKeeper *keeper=NULL);

  /**
   * Helper that does topology for XdmfToVTK
   */
  static void CopyShape(
    XdmfRegularGrid *grid,
    vtkImageData *dataSet,
    vtkXdmf3ArrayKeeper *keeper=NULL);

  /**
   * Populates the Xdmf Grid with the contents of the VTK data set
   */
  static void VTKToXdmf(
    vtkImageData *dataSet,
    XdmfDomain *domain,
    bool hasTime, double time,
    const char* name = 0);

  //vtkXdmf3RectilinearGrid
  /**
   * Populates the VTK data set with the contents of the Xdmf grid
   */
  static void XdmfToVTK(
    vtkXdmf3ArraySelection *fselection,
    vtkXdmf3ArraySelection *cselection,
    vtkXdmf3ArraySelection *pselection,
    XdmfRectilinearGrid *grid,
    vtkRectilinearGrid *dataSet,
    vtkXdmf3ArrayKeeper *keeper=NULL);

  /**
   * Helper that does topology for XdmfToVTK
   */
  static void CopyShape(
    XdmfRectilinearGrid *grid,
    vtkRectilinearGrid *dataSet,
    vtkXdmf3ArrayKeeper *keeper=NULL);

  /**
   * Populates the Xdmf Grid with the contents of the VTK data set
   */
  static void VTKToXdmf(
    vtkRectilinearGrid *dataSet,
    XdmfDomain *domain,
    bool hasTime, double time,
    const char* name = 0);

  //vtkXdmf3CurvilinearGrid
  /**
   * Populates the VTK data set with the contents of the Xdmf grid
   */
  static void XdmfToVTK(
    vtkXdmf3ArraySelection *fselection,
    vtkXdmf3ArraySelection *cselection,
    vtkXdmf3ArraySelection *pselection,
    XdmfCurvilinearGrid *grid,
    vtkStructuredGrid *dataSet,
    vtkXdmf3ArrayKeeper *keeper=NULL);

  /**
   * Helper that does topology for XdmfToVTK
   */
  static void CopyShape(
    XdmfCurvilinearGrid *grid,
    vtkStructuredGrid *dataSet,
    vtkXdmf3ArrayKeeper *keeper=NULL);

  /**
   * Populates the Xdmf Grid with the contents of the VTK data set
   */
  static void VTKToXdmf(
    vtkStructuredGrid *dataSet,
    XdmfDomain *domain,
    bool hasTime, double time,
    const char* name = 0);

  //vtkXdmf3UnstructuredGrid
  /**
   * Populates the VTK data set with the contents of the Xdmf grid
   */
  static void XdmfToVTK(
    vtkXdmf3ArraySelection *fselection,
    vtkXdmf3ArraySelection *cselection,
    vtkXdmf3ArraySelection *pselection,
    XdmfUnstructuredGrid *grid,
    vtkUnstructuredGrid *dataSet,
    vtkXdmf3ArrayKeeper *keeper=NULL);

  /**
   * Helper that does topology for XdmfToVTK
   */
  static void CopyShape(
    XdmfUnstructuredGrid *grid,
    vtkUnstructuredGrid *dataSet,
    vtkXdmf3ArrayKeeper *keeper=NULL);

  /**
   * Populates the Xdmf Grid with the contents of the VTK data set
   */
  static void VTKToXdmf(
    vtkPointSet *dataSet,
    XdmfDomain *domain,
    bool hasTime, double time,
    const char* name = 0);

  //vtkXdmf3Graph
  /**
   * Populates the VTK graph with the contents of the Xdmf grid
   */
  static void XdmfToVTK(
    vtkXdmf3ArraySelection *fselection,
    vtkXdmf3ArraySelection *cselection,
    vtkXdmf3ArraySelection *pselection,
    XdmfGraph *grid,
    vtkMutableDirectedGraph *dataSet,
    vtkXdmf3ArrayKeeper *keeper=NULL);

  /**
   * Populates the Xdmf Grid with the contents of the VTK data set
   */
  static void VTKToXdmf(
    vtkDirectedGraph *dataSet,
    XdmfDomain *domain,
    bool hasTime, double time,
    const char* name = 0);


  //Side Sets

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
    XdmfSet *grid, vtkDataObject *dObject,
    vtkXdmf3ArrayKeeper *keeper=NULL);

  /**
   * Extracts numbered subset out of grid (grid corresponds to dataSet),
   * and fills in subSet with it.
   */
  static void XdmfSubsetToVTK(
    XdmfGrid *grid,
    unsigned int setnum,
    vtkDataSet *dataSet,
    vtkUnstructuredGrid *subSet,
    vtkXdmf3ArrayKeeper *keeper=NULL);

};

#endif
// VTK-HeaderTest-Exclude: vtkXdmf3DataSet.h
