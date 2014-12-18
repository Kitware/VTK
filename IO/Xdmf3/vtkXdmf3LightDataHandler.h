/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXdmf3LightDataHandler.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXdmf3LightDataHandler - internal helper for vtkXdmf3Reader
// .SECTION Description
// vtkXdmf3Reader uses this class to inspect the light data in the XDMF
// file(s) and determine meta-information about the vtkDataObjects it
// needs to produce.
//
// This file is a helper for the vtkXdmf3Reader and not intended to be
// part of VTK public API
// VTK-HeaderTest-Exclude: vtkXdmf3LightDataHandler.h

#ifndef vtkXdmf3LightDataHandler_h
#define vtkXdmf3LightDataHandler_h

#include "vtkIOXdmf3Module.h" // For export macro
#include <set>
#include "XdmfItem.hpp"
#include "vtkType.h"

class vtkXdmf3SILBuilder;
class vtkXdmf3ArraySelection;
class XdmfItem;
class XdmfGraph;
class XdmfGrid;

class VTKIOXDMF3_EXPORT vtkXdmf3LightDataHandler
{
public:
  //Description:
  //factory constructor
  static shared_ptr<vtkXdmf3LightDataHandler> New(
      vtkXdmf3SILBuilder *sb,
      vtkXdmf3ArraySelection* f,
      vtkXdmf3ArraySelection* ce,
      vtkXdmf3ArraySelection* pn,
      vtkXdmf3ArraySelection* gc,
      vtkXdmf3ArraySelection* sc,
      unsigned int processor,
      unsigned int nprocessors);

  //Description:
  //destructor
  ~vtkXdmf3LightDataHandler();

  //Description:
  // recursively inspect XDMF data hierarchy to determine
  // times that we can provide data at
  // name of arrays to select from
  // name and hierarchical relationship of blocks to select from
  void InspectXDMF(shared_ptr<XdmfItem> item, vtkIdType parentVertex,
                   unsigned int depth=0);

  //Description:
  // called to make sure overflown SIL doesn't give nonsensical results
  void ClearGridsIfNeeded(shared_ptr<XdmfItem> domain);

  //Description:
  //return the list of times that the xdmf file can provide data at
  //only valid after InspectXDMF
  std::set<double> getTimes();

private:
  //Description:
  //constructor
  vtkXdmf3LightDataHandler();

  //Description:
  //remembers array names from the item
  void InspectArrays(shared_ptr<XdmfItem> item);

  //Description:
  //Used in SIL creation.
  bool TooDeep(unsigned int depth);

  //Description:
  //Used in SIL creation.
  std::string UniqueName(std::string name, bool ForGrid);

  //Description:
  //Used in SIL creation.
  void AddNamedBlock(vtkIdType parentVertex,
                     std::string originalName, std::string uniqueName);

  //Description:
  //Used in SIL creation.
  void AddNamedSet(std::string uniqueName);

  //Description:
  //records times that xdmf grids supply data at
  //if timespecs are only implied we add them to make things simpler later on
  void InspectTime(shared_ptr<XdmfItem> item);
  void GetSetTime(shared_ptr<XdmfGrid> child, unsigned int &cnt);
  void GetSetTime(shared_ptr<XdmfGraph> child, unsigned int &cnt);

  //Description:
  //for parallel partitioning
  bool ShouldRead(unsigned int piece, unsigned int npieces);

  vtkXdmf3SILBuilder *SILBuilder;
  vtkXdmf3ArraySelection *FieldArrays;
  vtkXdmf3ArraySelection *CellArrays; //ie EdgeArrays for Graphs
  vtkXdmf3ArraySelection *PointArrays; //ie NodeArrays for Graphs
  vtkXdmf3ArraySelection *GridsCache;
  vtkXdmf3ArraySelection *SetsCache;
  unsigned int MaxDepth;
  unsigned int Rank;
  unsigned int NumProcs;
  std::set<double> times; //relying on implicit sort from set<double>
};

#endif //vtkXdmf3LightDataHandler_h
