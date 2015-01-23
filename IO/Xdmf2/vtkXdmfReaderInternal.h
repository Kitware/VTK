/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXdmfReaderInternal.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXdmfReaderInternal -- private class(es) used by vtkXdmfReader
// .SECTION Description

// VTK-HeaderTest-Exclude: vtkXdmfReaderInternal.h

#ifndef vtkXdmfReaderInternal_h
#define vtkXdmfReaderInternal_h

// NAMING CONVENTION *********************************************************
// * all member variables of the type XdmfXml* begin with XML eg. XMLNode
// * all non-member variables of the type XdmfXml* begin with xml eg. xmlNode
// * all member variables of the type XdmfElement (and subclasses) begin with
//   XMF eg. XMFGrid
// * all non-member variables of the type XdmfElement (and subclasses) begin
//   with xmf eg. xmfGrid
// ***************************************************************************

#include "vtkMutableDirectedGraph.h"
#include "vtkSILBuilder.h"

#include "XdmfArray.h"
#include "XdmfAttribute.h"
#include "XdmfDOM.h"
//?
#include "XdmfDataDesc.h"
//?
#include "XdmfDataItem.h"
#include "XdmfGrid.h"
//?
#include "XdmfTopology.h"
//?
#include "XdmfGeometry.h"
//?
#include "XdmfTime.h"
//?
#include "XdmfSet.h"

#include <string>
#include <vector>
#include <set>
#include <map>
#include <vtksys/SystemTools.hxx>
#include <cassert>
#include <functional>
#include <algorithm>
#include <vtksys/ios/sstream>

class vtkXdmfDomain;
class VTKIOXDMF2_EXPORT vtkXdmfDocument
{
public:
  //---------------------------------------------------------------------------
  // Description:
  // Parse an xmf file (or string). Both these methods use caching hence calling
  // these methods repeatedly with the same argument will NOT result in
  // re-parsing of the xmf.
  bool Parse(const char*xmffilename);
  bool ParseString(const char* xmfdata, size_t length);

  //---------------------------------------------------------------------------
  // Description:
  // Returns the names for available domains.
  const std::vector<std::string>& GetDomains()
    { return this->Domains; }

  //---------------------------------------------------------------------------
  // Description:
  // Set the active domain. This will result in processing of the domain xmf if
  // the selected domain is different from the active one.
  bool SetActiveDomain(const char* domainname);
  bool SetActiveDomain(int index);

  //---------------------------------------------------------------------------
  // Description:
  // Returns the active domain.
  vtkXdmfDomain* GetActiveDomain()
    { return this->ActiveDomain; }

  //---------------------------------------------------------------------------
  // Description:
  // Constructor/Destructor
  vtkXdmfDocument();
  ~vtkXdmfDocument();

private:
  // Populates the list of domains.
  void UpdateDomains();

private:
  int ActiveDomainIndex;
  xdmf2::XdmfDOM XMLDOM;
  vtkXdmfDomain* ActiveDomain;
  std::vector<std::string> Domains;

  char* LastReadContents;
  size_t LastReadContentsLength;
  std::string LastReadFilename;
};

// I don't use vtkDataArraySelection since it's very slow when it comes to large
// number of arrays.
class vtkXdmfArraySelection : public std::map<std::string, bool>
{
public:
  void Merge(const vtkXdmfArraySelection& other)
    {
    vtkXdmfArraySelection::const_iterator iter = other.begin();
    for (; iter != other.end(); ++iter)
      {
      (*this)[iter->first] = iter->second;
      }
    }

  void AddArray(const char* name, bool status=true)
    {
    (*this)[name] = status;
    }

  bool ArrayIsEnabled(const char* name)
    {
    vtkXdmfArraySelection::iterator iter = this->find(name);
    if (iter != this->end())
      {
      return iter->second;
      }

    // don't know anything about this array, enable it by default.
    return true;
    }

  bool HasArray(const char* name)
    {
    vtkXdmfArraySelection::iterator iter = this->find(name);
    return (iter != this->end());
    }

  int GetArraySetting(const char* name)
    {
    return this->ArrayIsEnabled(name)? 1 : 0;
    }

  void SetArrayStatus(const char* name, bool status)
    {
    this->AddArray(name, status);
    }

  const char* GetArrayName(int index)
    {
    int cc=0;
    for (vtkXdmfArraySelection::iterator iter = this->begin();
      iter != this->end(); ++iter)
      {

      if (cc==index)
        {
        return iter->first.c_str();
        }
      cc++;
      }
    return NULL;
    }

  int GetNumberOfArrays()
    {
    return static_cast<int>(this->size());
    }
};

//***************************************************************************
class VTKIOXDMF2_EXPORT vtkXdmfDomain
{
private:
  XdmfInt64 NumberOfGrids;
  xdmf2::XdmfGrid* XMFGrids;

  XdmfXmlNode XMLDomain;
  xdmf2::XdmfDOM* XMLDOM;

  unsigned int GridsOverflowCounter;
  // these are node indices used when building the SIL.
  vtkIdType SILBlocksRoot;
  std::map<std::string, vtkIdType> GridCenteredAttrbuteRoots;
  std::map<vtkIdType,
    std::map<XdmfInt64, vtkIdType> > GridCenteredAttrbuteValues;

  vtkSILBuilder* SILBuilder;
  vtkMutableDirectedGraph* SIL;
  vtkXdmfArraySelection* PointArrays;
  vtkXdmfArraySelection* CellArrays;
  vtkXdmfArraySelection* Grids;
  vtkXdmfArraySelection* Sets;
  std::set<XdmfFloat64> TimeSteps; //< Only discrete timesteps are currently
                                 //  supported.

public:
  //---------------------------------------------------------------------------
  // does not take ownership of the DOM, however the xmlDom must exist as long
  // as the instance is in use.
  vtkXdmfDomain(xdmf2::XdmfDOM* xmlDom, int domain_index);

  //---------------------------------------------------------------------------
  // Description:
  // After instantiating, check that the domain is valid. If this returns false,
  // it means that the specified domain could not be located.
  bool IsValid()
    { return (this->XMLDomain != 0); }

  //---------------------------------------------------------------------------
  vtkGraph* GetSIL()
    { return this->SIL; }

  //---------------------------------------------------------------------------
  // Description:
  // Returns the number of top-level grids present in this domain.
  XdmfInt64 GetNumberOfGrids() { return this->NumberOfGrids; }

  //---------------------------------------------------------------------------
  // Description:
  // Provides access to a top-level grid from this domain.
  xdmf2::XdmfGrid* GetGrid(XdmfInt64 cc);

  //---------------------------------------------------------------------------
  // Description:
  // Returns the VTK data type need for this domain. If the domain has only 1
  // grid, then a vtkDataSet-type is returned, otherwise a vtkMultiBlockDataSet
  // is required.
  // Returns -1 on error.
  int GetVTKDataType();

  //---------------------------------------------------------------------------
  // Description:
  // Returns the timesteps.
  const std::set<XdmfFloat64>& GetTimeSteps()
    { return this->TimeSteps; }

  //---------------------------------------------------------------------------
  // Description:
  // Given a time value, returns the index.
  int GetIndexForTime(double time);

  //---------------------------------------------------------------------------
  // Description:
  // Returns the time value at the given index.
  XdmfFloat64 GetTimeForIndex(int index)
    {
    std::set<XdmfFloat64>::iterator iter;
    int cc=0;
    for (iter = this->TimeSteps.begin(); iter != this->TimeSteps.end();
      iter++, cc++)
      {
      if (cc == index)
        {
        return *iter;
        }
      }
    // invalid index.
    return 0.0;
    }

  //---------------------------------------------------------------------------
  // Description:
  // If xmfGrid is a temporal collection, returns the child-grid matching the
  // requested time.
  xdmf2::XdmfGrid* GetGrid(xdmf2::XdmfGrid* xmfGrid, double time);

  //---------------------------------------------------------------------------
  // Description:
  // Returns true if the grids is a structured dataset.
  bool IsStructured(xdmf2::XdmfGrid*);

  //---------------------------------------------------------------------------
  // Description:
  // Returns the whole extents for the dataset if the grid if IsStructured()
  // returns true for the given grid. Returns true if the extents are valid.
  // NOTE: returned extents are always (0, dimx-1, 0, dimy-1, 0, dimz-1).
  bool GetWholeExtent(xdmf2::XdmfGrid*, int extents[6]);

  //---------------------------------------------------------------------------
  // Description:
  // Returns the spacing and origin for the grid if the grid topology ==
  // XDMF_2DCORECTMESH or XDMF_3DCORECTMESH i.e. image data.  Returns true if
  // the extents are valid.
  bool GetOriginAndSpacing(xdmf2::XdmfGrid*, double origin[3], double spacing[3]);

  //---------------------------------------------------------------------------
  ~vtkXdmfDomain();

  // Returns VTK data type based on grid type and topology.
  // Returns -1 on error.
  int GetVTKDataType(xdmf2::XdmfGrid* xmfGrid);

  // Returns the dimensionality (or rank) of the topology for the given grid.
  // Returns -1 is the xmfGrid is not a uniform i.e. is a collection or a tree.
  static int GetDataDimensionality(xdmf2::XdmfGrid* xmfGrid);

  vtkXdmfArraySelection* GetPointArraySelection()
    { return this->PointArrays; }
  vtkXdmfArraySelection* GetCellArraySelection()
    { return this->CellArrays; }
  vtkXdmfArraySelection* GetGridSelection()
    { return this->Grids; }
  vtkXdmfArraySelection* GetSetsSelection()
    { return this->Sets; }

private:
  // Description:
  // There are a few meta-information that we need to collect from the domain
  // * number of data-arrays so that the user can choose which to load.
  // * grid-structure so that the user can choose the hierarchy
  // * time information so that reader can report the number of timesteps
  //   available.
  // This does another book-keeping task of ensuring that all grids have valid
  // names. If a grid is not named, then we make up a name.
  // TODO: We can use GRID centered attributes to create hierarchies in the SIL.
  void CollectMetaData();

  // Used by CollectMetaData().
  void CollectMetaData(xdmf2::XdmfGrid* xmfGrid, vtkIdType silParent);

  // Used by CollectMetaData().
  void CollectNonLeafMetaData(xdmf2::XdmfGrid* xmfGrid, vtkIdType silParent);

  // Used by CollectMetaData().
  void CollectLeafMetaData(xdmf2::XdmfGrid* xmfGrid, vtkIdType silParent);

  // Description:
  // Use this to add an association with the grid attribute with the node for
  // the grid in the SIL if applicable. Returns true if the attribute was added.
  bool UpdateGridAttributeInSIL(
    xdmf2::XdmfAttribute* xmfAttribute, vtkIdType gridSILId);
};

#endif
