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

/**
 * Determines whether two XDMF files contain equivalent data.
 *
 * Intended to be used as both a command line utility and a framework for code
 * testing purposes.
 *
 * The XdmfDiff utility can be accessed via python or executed as a compiled command line utility.
 *
 * Command Line:
 *   Write a script to access the swig wrapped diff utility
 *
 *   There are two ways to run the command line utility:
 *
 *     XdmfDiff referenceFile newFile
 *       Compares all information contained in referenceFile to newFile .  Extra grids
 *       contained in newFile that are not in referenceFile are ignored.
 *
 *     XdmfDiff referenceFile newFile settingsFile
 *       Compares information contained in referenceFile to newFile according to settings
 *       specified in the settingsFile.  All settings options are outlined below:
 *
 * Settings Options:
 *   RELATIVE_ERROR .15
 *   ABSOLUTE_ERROR 1
 *   INCLUDE_GRID grid1 grid2
 *   IGNORE_GRID grid1 grid2
 *   IGNORE_GEOMETRY
 *   IGNORE_TOPOLOGY
 *   INCLUDE_ATTRIBUTE attr1 attr2
 *   IGNORE_ATTRIBUTE attr1 attr2
 *   IGNORE_ALL_ATTRIBUTES
 *   DISPLAY_FAILURES_ONLY
 *   VERBOSE_OUTPUT
 *
 * Settings can be commented out with #
 *
 * For code testing purposes run XdmfDiff::AreEquivalent().
 */

#include "XdmfDiff.h"

#ifndef BUILD_EXE

#include <XdmfAttribute.h>
#include <XdmfGrid.h>
#include <XdmfGeometry.h>
#include <XdmfTime.h>
#include <XdmfTopology.h>

#include <cmath>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <map>

class XdmfDiffInternal
{
public:
  /*
   * Collects XdmfDiffEntrys for during a comparison (i.e. stores all differences
   *   between two XdmfGeometry values)
   */
  class XdmfDiffReport
  {
  public:
    /*
     * Error description for a single pair of values
     */
    class XdmfDiffEntry
    {
    public:
      XdmfDiffEntry(std::string errorDescription, XdmfInt64 loc,
          std::string refVals, std::string newVals)
      {
        description = errorDescription;
        location = loc;
        refValues = refVals;
        newValues = newVals;
      }

      ~XdmfDiffEntry()
      {
      }

      friend std::ostream &
      operator<<(std::ostream & toReturn, const XdmfDiffEntry &diffEntry)
      {
        if (diffEntry.location == -1)
        {
          toReturn << "For " << diffEntry.description << " | Expected : "
              << diffEntry.refValues << " | Got : " << diffEntry.newValues;
        }
        else
        {
          toReturn << "For " << diffEntry.description << " | At Tuple "
              << diffEntry.location << " | Expected : " << diffEntry.refValues
              << " | Got : " << diffEntry.newValues;
        }
        return toReturn;
      }

    private:
      XdmfInt64 location;
      std::string refValues;
      std::string newValues;
      std::string description;
    };

    XdmfDiffReport(std::string type)
    {
      valType = type;
    }

    ~XdmfDiffReport()
    {
    }

    void
    AddError(std::string errorDescription, std::string refVals,
        std::string newVals)
    {
      this->AddError(errorDescription, -1, refVals, newVals);
    }

    void
    AddError(std::string errorDescription, XdmfInt64 loc, std::string refVals,
        std::string newVals)
    {
      errors.push_back(XdmfDiffEntry(errorDescription, loc, refVals, newVals));
    }

    void
    AddError(std::string warning)
    {
      warnings.push_back(warning);
    }

    XdmfInt64
    GetNumberOfErrors()
    {
      return errors.size() + warnings.size();
    }

    friend std::ostream &
    operator<<(std::ostream & toReturn, const XdmfDiffReport &diffReport)
    {
      toReturn << diffReport.valType << "\n";
      for (unsigned int i = 0; i < diffReport.warnings.size(); i++)
      {
        toReturn << "\t\t" << diffReport.warnings[i] << "\n";
      }
      for (unsigned int i = 0; i < diffReport.errors.size(); i++)
      {
        toReturn << "\t\t" << diffReport.errors[i] << "\n";
      }
      return toReturn;
    }

  private:
    std::vector<XdmfDiffEntry> errors;
    std::vector<std::string> warnings;
    std::string valType;
  };

  /*
   * Collects XdmfDiffReports for an entire file-wide comparison.  Provides
   *   methods for outputting the comparisons in text
   */
  class XdmfDiffReportCollection
  {
  public:
    XdmfDiffReportCollection(XdmfBoolean failuresOnly, XdmfBoolean verbose)
    {
      displayFailuresOnly = failuresOnly;
      verboseOutput = verbose;
    }

    ~XdmfDiffReportCollection()
    {
    }

    void
    AddReport(std::string gridName, XdmfDiffReport report)
    {
      reports[gridName].push_back(report);
    }

    XdmfInt64
    GetNumberOfErrors()
    {
      int numErrors = 0;
      for (std::map<std::string, std::vector<XdmfDiffReport> >::const_iterator
          iter = reports.begin(); iter != reports.end(); iter++)
      {
        for (unsigned int i = 0; i < iter->second.size(); i++)
        {
          std::vector<XdmfDiffReport> report = iter->second;
          numErrors += report[i].GetNumberOfErrors();
        }
      }
      return numErrors;
    }

    friend std::ostream &
    operator<<(std::ostream & toReturn,
        const XdmfDiffReportCollection &diffCollection)
    {
      for (std::map<std::string, std::vector<XdmfDiffReport> >::const_iterator
          iter = diffCollection.reports.begin(); iter
          != diffCollection.reports.end(); iter++)
      {
        int numGridErrors = 0;
        for (unsigned int i = 0; i < iter->second.size(); i++)
        {
          std::vector<XdmfDiffReport> report = iter->second;
          if (report[i].GetNumberOfErrors() > 0)
          {
            if (numGridErrors == 0 || diffCollection.verboseOutput)
            {
              toReturn << "|FAIL|  Grid Name: " << iter->first << "\n";
            }
            toReturn << "\t" << report[i];
            numGridErrors += report[i].GetNumberOfErrors();
          }
          else if (diffCollection.verboseOutput
              && !diffCollection.displayFailuresOnly)
          {
            toReturn << "|PASS|  Grid Name: " << iter->first;
            toReturn << "\t" << report[i];
          }
        }
        if (numGridErrors == 0 && !diffCollection.displayFailuresOnly
            && !diffCollection.verboseOutput)
        {
          toReturn << "|PASS|  Grid Name: " << iter->first << "\n";
        }
      }
      return toReturn;
    }

  private:
    std::map<std::string, std::vector<XdmfDiffReport> > reports;
    XdmfBoolean displayFailuresOnly;
    XdmfBoolean verboseOutput;
  };

  XdmfDiffInternal(XdmfConstString refFileName, XdmfConstString newFileName);
  XdmfDiffInternal(XdmfDOM * refDOM, XdmfDOM * newDOM);
  void Init();
  ~XdmfDiffInternal();
  std::string GetDiffs();
  std::string GetDiffs(XdmfConstString gridName);
  XdmfInt32 SetRelativeError(XdmfFloat64 & relativeError);
  XdmfInt32 SetAbsoluteError(XdmfFloat64 & absoluteError);
  XdmfInt32 SetDiffFileName(XdmfString name);
  XdmfString GetDiffFileName();
  XdmfBoolean AreEquivalent();
  XdmfInt32 IncludeGrid(XdmfString gridName);
  XdmfInt32 IgnoreGrid(XdmfString gridName);
  XdmfInt32 IncludeAttribute(XdmfString attributeName);
  XdmfInt32 IgnoreAttribute(XdmfString attributeName);
  XdmfInt32 ParseSettingsFile(XdmfConstString settingsFile);

  XdmfSetValueMacro(IgnoreTime, XdmfBoolean);
  XdmfGetValueMacro(IgnoreTime, XdmfBoolean);
  XdmfSetValueMacro(IgnoreGeometry, XdmfBoolean);
  XdmfGetValueMacro(IgnoreGeometry, XdmfBoolean);
  XdmfSetValueMacro(IgnoreTopology, XdmfBoolean);
  XdmfGetValueMacro(IgnoreTopology, XdmfBoolean);
  XdmfSetValueMacro(IgnoreAllAttributes, XdmfBoolean);
  XdmfGetValueMacro(IgnoreAllAttributes, XdmfBoolean);
  XdmfSetValueMacro(DisplayFailuresOnly, XdmfBoolean);
  XdmfGetValueMacro(DisplayFailuresOnly, XdmfBoolean);
  XdmfSetValueMacro(VerboseOutput, XdmfBoolean);
  XdmfGetValueMacro(VerboseOutput, XdmfBoolean);
  XdmfSetValueMacro(CreateDiffFile, XdmfBoolean);
  XdmfGetValueMacro(CreateDiffFile, XdmfBoolean);
  XdmfGetValueMacro(AbsoluteError, XdmfFloat64);
  XdmfGetValueMacro(RelativeError, XdmfFloat64);

private:

  /*
   * Returns the differences between the two XDMF files.  Compares each grid in the
   * reference file to the grid of the same name in the second file.  Stuffs the results
   * of the comparison in the error report
   *
   * @param errorReports an XdmfDiffReportCollection that collects all difference reports during the comparison
   *
   */
  void
  GetDiffs(XdmfDiffReportCollection & errorReports);

  /*
   * Returns the differences between two grids.
   *
   * @param refGrid the reference grid to compare (new grid is searched for using the reference grid's name)
   * @param errorReports an XdmfDiffReportCollection that collects all difference reports during the comparison
   *
   */
  void
  GetDiffs(XdmfGrid & refGrid, XdmfDiffReportCollection & errorReports);

  /*
   * Returns the differences in values between two XdmfGeometries
   *
   * @param refGeometry an XdmfGeometry to compare
   * @param newGeometry an XdmfGeometry to compare
   *
   * @return an XdmfDiffReport containing differences in values
   *
   */
  XdmfDiffReport
  GetGeometryDiffs(XdmfGeometry * refGeometry, XdmfGeometry * newGeometry);

  /*
   * Returns the differences in values between two XdmfTopologies
   *
   * @param refTopology an XdmfTopology to compare
   * @param newTopology an XdmfTopology to compare
   *
   * @return an XdmfDiffReport containing differences in values
   *
   */
  XdmfDiffReport
  GetTopologyDiffs(XdmfTopology * refTopology, XdmfTopology * newTopology);

  /*
   * Returns the differences in values between two XdmfAttributes
   *
   * @param refAttribute an XdmfAttribute to compare
   * @param newAttribute an XdmfAttribute to compare
   *
   * @return an XdmfDiffReport containing differences in values
   *
   */
  XdmfDiffReport
  GetAttributeDiffs(XdmfAttribute * refAttribute, XdmfAttribute * newAttribute);

  /*
   * Entry point for array comparisons ---> passes to templated private function based on number type to do actual value comparisons
   *
   * @param errorReport an XdmfDiffReport to add comparison results to
   * @param refArray an XdmfArray containing values to compare
   * @param newArray an XdmfArray containing values to compare
   * @param startIndex an index to start comparison at
   * @param numValues the number of values to compare
   * @param groupLength how many values are contained together.
   * 		  Useful for reporting changes in multiple values i.e. XYZ geometry (default: 1)
   *
   */
  XdmfArray *
  CompareValues(XdmfDiffReport & errorReport, XdmfArray * refArray,
      XdmfArray * newArray, XdmfInt64 startIndex, XdmfInt64 numValues,
      XdmfInt64 groupLength = 1);

  /*
   * Compares values between two XdmfArrays
   *
   * @param errorReport an XdmfDiffReport to add comparison results to
   * @param refArray an XdmfArray containing values to compare
   * @param newArray an XdmfArray containing values to compare
   * @param startIndex an index to start comparison at
   * @param numValues the number of values to compare
   * @param groupLength how many values are contained together.
   * 		  Useful for reporting changes in multiple values i.e. XYZ geometry (default: 1)
   *
   */
  template<class XdmfType>
    XdmfArray *
    CompareValuesPriv(XdmfDiffReport & errorReport, XdmfArray * refArray,
        XdmfArray * newArray, XdmfInt64 startIndex, XdmfInt64 numValues,
        XdmfInt64 groupLength = 1);

  std::set<std::string> includedGrids;
  std::set<std::string> ignoredGrids;
  std::set<std::string> includedAttributes;
  std::set<std::string> ignoredAttributes;
  XdmfDOM * myRefDOM;
  XdmfDOM * myNewDOM;
  XdmfFloat64 RelativeError;
  XdmfFloat64 AbsoluteError;
  XdmfBoolean IgnoreTime;
  XdmfBoolean IgnoreGeometry;
  XdmfBoolean IgnoreTopology;
  XdmfBoolean IgnoreAllAttributes;
  XdmfBoolean DisplayFailuresOnly;
  XdmfBoolean VerboseOutput;
  XdmfBoolean refDOMIsMine;
  XdmfBoolean newDOMIsMine;
  XdmfBoolean CreateDiffFile;
  XdmfGrid * diffGrid;
  XdmfElement * diffGridParent;
  std::string diffName;
  std::string diffHeavyName;
};

XdmfDiff::XdmfDiff(XdmfConstString refFileName, XdmfConstString newFileName)
{
  myInternal = new XdmfDiffInternal(refFileName, newFileName);
}

XdmfDiff::XdmfDiff(XdmfDOM * refDOM, XdmfDOM * newDOM)
{
  myInternal = new XdmfDiffInternal(refDOM, newDOM);
}

XdmfDiff::~XdmfDiff()
{
  delete myInternal;
}

std::string
XdmfDiff::GetDiffs()
{
  return myInternal->GetDiffs();
}

std::string
XdmfDiff::GetDiffs(XdmfConstString gridName)
{
  return myInternal->GetDiffs(gridName);
}

XdmfInt32
XdmfDiff::SetIgnoreTime(XdmfBoolean value)
{
  return myInternal->SetIgnoreTime(value);
}

XdmfInt32
XdmfDiff::GetIgnoreTime()
{
  return myInternal->GetIgnoreTime();
}

XdmfInt32
XdmfDiff::SetIgnoreGeometry(XdmfBoolean value)
{
  return myInternal->SetIgnoreGeometry(value);
}

XdmfInt32
XdmfDiff::GetIgnoreGeometry()
{
  return myInternal->GetIgnoreGeometry();
}

XdmfInt32
XdmfDiff::SetIgnoreTopology(XdmfBoolean value)
{
  return myInternal->SetIgnoreTopology(value);
}

XdmfInt32
XdmfDiff::GetIgnoreTopology()
{
  return myInternal->GetIgnoreTopology();
}

XdmfInt32
XdmfDiff::SetIgnoreAllAttributes(XdmfBoolean value)
{
  return myInternal->SetIgnoreAllAttributes(value);
}

XdmfInt32
XdmfDiff::GetIgnoreAllAttributes()
{
  return myInternal->GetIgnoreAllAttributes();
}

XdmfInt32
XdmfDiff::SetDisplayFailuresOnly(XdmfBoolean value)
{
  return myInternal->SetDisplayFailuresOnly(value);
}

XdmfInt32
XdmfDiff::GetDisplayFailuresOnly()
{
  return myInternal->GetDisplayFailuresOnly();
}

XdmfInt32
XdmfDiff::SetVerboseOutput(XdmfBoolean value)
{
  return myInternal->SetVerboseOutput(value);
}

XdmfInt32
XdmfDiff::GetVerboseOutput()
{
  return myInternal->GetVerboseOutput();
}

XdmfInt32
XdmfDiff::SetCreateDiffFile(XdmfBoolean value)
{
  return myInternal->SetCreateDiffFile(value);
}

XdmfInt32
XdmfDiff::GetCreateDiffFile()
{
  return myInternal->GetCreateDiffFile();
}

XdmfInt32
XdmfDiff::SetDiffFileName(XdmfString value)
{
  return myInternal->SetDiffFileName(value);
}

XdmfString
XdmfDiff::GetDiffFileName()
{
  return myInternal->GetDiffFileName();
}

XdmfInt32
XdmfDiff::SetRelativeError(XdmfFloat64 relativeError)
{
  return myInternal->SetRelativeError(relativeError);
}

XdmfFloat64
XdmfDiff::GetRelativeError()
{
  return myInternal->GetRelativeError();
}

XdmfInt32
XdmfDiff::SetAbsoluteError(XdmfFloat64 absoluteError)
{
  return myInternal->SetAbsoluteError(absoluteError);
}

XdmfFloat64
XdmfDiff::GetAbsoluteError()
{
  return myInternal->GetAbsoluteError();
}

XdmfInt32
XdmfDiff::IncludeGrid(XdmfString gridName)
{
  return myInternal->IncludeGrid(gridName);
}

XdmfInt32
XdmfDiff::IgnoreGrid(XdmfString gridName)
{
  return myInternal->IgnoreGrid(gridName);
}

XdmfInt32
XdmfDiff::IncludeAttribute(XdmfString attributeName)
{
  return myInternal->IncludeAttribute(attributeName);
}

XdmfInt32
XdmfDiff::IgnoreAttribute(XdmfString attributeName)
{
  return myInternal->IgnoreAttribute(attributeName);
}

XdmfBoolean
XdmfDiff::AreEquivalent()
{
  return myInternal->AreEquivalent();
}

XdmfInt32
XdmfDiff::ParseSettingsFile(XdmfConstString settingsFile)
{
  return myInternal->ParseSettingsFile(settingsFile);
}

XdmfDiffInternal::XdmfDiffInternal(XdmfConstString refFileName,
    XdmfConstString newFileName)
{
  myRefDOM = new XdmfDOM();
  myNewDOM = new XdmfDOM();

  std::string refstr = refFileName;
  size_t reffound = refstr.find_last_of("/\\");
  if (reffound != std::string::npos)
  {
    myRefDOM->SetWorkingDirectory(refstr.substr(0, reffound).substr().c_str());
  }

  std::string newstr = newFileName;
  size_t newfound = newstr.find_last_of("/\\");
  if (newfound != std::string::npos)
  {
    myNewDOM->SetWorkingDirectory(newstr.substr(0, newfound).substr().c_str());
  }

  myRefDOM->Parse(refFileName);
  myNewDOM->Parse(newFileName);

  refDOMIsMine = true;
  newDOMIsMine = true;

  this->Init();
}

XdmfDiffInternal::XdmfDiffInternal(XdmfDOM * refDOM, XdmfDOM * newDOM)
{
  myRefDOM = refDOM;
  myNewDOM = newDOM;

  refDOMIsMine = false;
  newDOMIsMine = false;

  this->Init();
}

void
XdmfDiffInternal::Init()
{
  RelativeError = 0;
  AbsoluteError = 0;
  IgnoreTime = false;
  IgnoreGeometry = false;
  IgnoreTopology = false;
  IgnoreAllAttributes = false;
  DisplayFailuresOnly = false;
  VerboseOutput = false;
  CreateDiffFile = false;
  diffGrid - NULL;
  diffGridParent = NULL;
  std::string path = myRefDOM->GetFileName();
  int endPath = path.find_last_of("/\\") + 1;
  int begSuffix = path.find_last_of(".");
  diffHeavyName = path.substr(endPath, begSuffix - endPath) + "-diff.h5";
  diffName = path.substr(endPath, begSuffix - endPath) + "-diff.xmf";
}

XdmfDiffInternal::~XdmfDiffInternal()
{
  if (this->refDOMIsMine)
    delete myRefDOM;
  if (this->newDOMIsMine)
    delete myNewDOM;
}

XdmfInt32
XdmfDiffInternal::SetRelativeError(XdmfFloat64 & relativeError)
{
  RelativeError = relativeError;
  AbsoluteError = 0;
  return (XDMF_SUCCESS);
}

XdmfInt32
XdmfDiffInternal::SetAbsoluteError(XdmfFloat64 & absoluteError)
{
  AbsoluteError = absoluteError;
  RelativeError = 0;
  return (XDMF_SUCCESS);
}

XdmfInt32
XdmfDiffInternal::SetDiffFileName(XdmfString name)
{
  if (name)
  {
    diffName = name;
    diffHeavyName = diffName.substr(0, diffName.find_last_of(".")) + ".h5";
    return XDMF_SUCCESS;
  }
  return XDMF_FAIL;
}

XdmfString
XdmfDiffInternal::GetDiffFileName()
{
  return (XdmfString) diffName.c_str();
}

XdmfInt32
XdmfDiffInternal::IncludeGrid(XdmfString gridName)
{
  ignoredGrids.erase(gridName);
  includedGrids.insert(gridName);
  return XDMF_SUCCESS;
}

XdmfInt32
XdmfDiffInternal::IgnoreGrid(XdmfString gridName)
{
  includedGrids.erase(gridName);
  ignoredGrids.insert(gridName);
  return XDMF_SUCCESS;
}

XdmfInt32
XdmfDiffInternal::IncludeAttribute(XdmfString attributeName)
{
  ignoredAttributes.erase(attributeName);
  includedAttributes.insert(attributeName);
  return XDMF_SUCCESS;
}

XdmfInt32
XdmfDiffInternal::IgnoreAttribute(XdmfString attributeName)
{
  includedAttributes.erase(attributeName);
  ignoredAttributes.insert(attributeName);
  return XDMF_SUCCESS;
}

std::string
XdmfDiffInternal::GetDiffs()
{
  std::stringstream toReturn;
  XdmfDiffReportCollection myErrors = XdmfDiffReportCollection(
      DisplayFailuresOnly, VerboseOutput);
  this->GetDiffs(myErrors);
  toReturn << myErrors;
  return toReturn.str();
}

std::string
XdmfDiffInternal::GetDiffs(XdmfConstString gridName)
{
  std::stringstream toReturn;
  XdmfXmlNode currDomain = myRefDOM->FindElement("Domain");
  for (int i = 0; i < myRefDOM->FindNumberOfElements("Grid", currDomain); i++)
  {
    XdmfGrid grid = XdmfGrid();
    grid.SetDOM(myRefDOM);
    grid.SetElement(myRefDOM->FindElement("Grid", i, currDomain));
    grid.Update();
    if (strcmp(grid.GetName(), gridName) == 0)
    {
      XdmfDiffInternal::XdmfDiffReportCollection errorReports =
          XdmfDiffInternal::XdmfDiffReportCollection(DisplayFailuresOnly,
              VerboseOutput);
      this->GetDiffs(errorReports);
      toReturn << errorReports;
      return toReturn.str();
    }
  }
  toReturn << "FAIL: Cannot Find Grid Named " << gridName;
  return toReturn.str();
}

void
XdmfDiffInternal::GetDiffs(
    XdmfDiffInternal::XdmfDiffReportCollection & errorReports)
{
  XdmfRoot * diffRoot;
  XdmfDomain * diffDomain;
  XdmfDOM * diffDOM;

  if (CreateDiffFile)
  {
    diffRoot = new XdmfRoot();
    diffDomain = new XdmfDomain();
    diffDOM = new XdmfDOM();

    diffRoot->SetDOM(diffDOM);
    diffRoot->Build();
    diffRoot->Insert(diffDomain);

    diffGridParent = diffDomain;
  }

  XdmfXmlNode currDomain = myRefDOM->FindElement("Domain");
  for (int i = 0; i < myRefDOM->FindNumberOfElements("Grid", currDomain); i++)
  {
    XdmfGrid grid = XdmfGrid();
    grid.SetDOM(myRefDOM);
    grid.SetElement(myRefDOM->FindElement("Grid", i, currDomain));
    grid.Update();
    //grid.Build();
    // Make sure we cleanup well
    for (int j = 0; j < grid.GetNumberOfAttributes(); j++)
    {
      grid.GetAttribute(j)->SetDeleteOnGridDelete(true);
    }
    this->GetDiffs(grid, errorReports);
  }

  if (CreateDiffFile)
  {
    diffDOM->Write(diffName.c_str());
    delete diffRoot;
    delete diffDomain;
    delete diffDOM;
  }
}

void
XdmfDiffInternal::GetDiffs(XdmfGrid & refGrid,
    XdmfDiffInternal::XdmfDiffReportCollection & errorReports)
{
  // Check for user specified grid includes / excludes
  if (includedGrids.size() != 0)
  {
    if (includedGrids.find(refGrid.GetName()) == includedGrids.end())
    {
      return;
    }
  }

  if (ignoredGrids.size() != 0)
  {
    if (ignoredGrids.find(refGrid.GetName()) != ignoredGrids.end())
    {
      return;
    }
  }

  XdmfGrid newGrid = XdmfGrid();
  newGrid.SetDOM(myNewDOM);

  std::string refPath = myRefDOM->GetPath(refGrid.GetElement());
  std::string parentPath = refPath.substr(0, refPath.find_last_of("/"));

  XdmfXmlNode newNode;
  if (refGrid.GetGridType() == XDMF_GRID_COLLECTION)
  {
    newNode = myNewDOM->FindElementByPath(refPath.c_str());
  }
  else
  {
    newNode = myNewDOM->FindElementByAttribute("Name", refGrid.GetName(), 0,
        myNewDOM->FindElementByPath(parentPath.c_str()));
  }

  if (newNode == NULL || newGrid.SetElement(newNode) != XDMF_SUCCESS)
  {
    XdmfDiffReport diffReport = XdmfDiffReport("Grid Name");
    std::stringstream warning;
    warning << "Could Not Find Grid: " << refGrid.GetName();
    diffReport.AddError(warning.str());
    errorReports.AddReport(refGrid.GetName(), diffReport);
    return;
  }
  newGrid.Update();

  XdmfDiffReport diffReport = XdmfDiffReport("Grid Type");
  if (refGrid.GetGridType() != newGrid.GetGridType())
  {
    diffReport.AddError("Grid Type", refGrid.GetGridTypeAsString(),
        newGrid.GetGridTypeAsString());
  }
  errorReports.AddReport(refGrid.GetName(), diffReport);

  if (CreateDiffFile && diffGridParent)
  {
    diffGrid = new XdmfGrid();
    diffGrid->SetGridType(refGrid.GetGridType());
    diffGrid->SetCollectionType(refGrid.GetCollectionType());
    diffGrid->SetName(refGrid.GetName());
    XdmfGeometry * geom = new XdmfGeometry();
    geom->SetLightDataLimit(0);
    geom->SetGeometryType(refGrid.GetGeometry()->GetGeometryType());
    geom->SetNumberOfPoints(refGrid.GetGeometry()->GetNumberOfPoints());
    geom->SetPoints(refGrid.GetGeometry()->GetPoints());
    geom->SetDeleteOnGridDelete(true);
    if (refGrid.GetGeometry()->GetPoints()->GetHeavyDataSetName())
    {
      std::string currHeavyName =
          refGrid.GetGeometry()->GetPoints()->GetHeavyDataSetName();
      currHeavyName = diffHeavyName + currHeavyName.substr(currHeavyName.find(
          ":/"), currHeavyName.size() - currHeavyName.find(":/"));
      geom->GetPoints()->SetHeavyDataSetName(currHeavyName.c_str());
    }
    diffGrid->SetGeometry(geom);
    XdmfTopology * top = new XdmfTopology();
    top->SetLightDataLimit(0);
    top->SetTopologyType(refGrid.GetTopology()->GetTopologyType());
    top->SetNodesPerElement(refGrid.GetTopology()->GetNodesPerElement());
    top->SetNumberOfElements(refGrid.GetTopology()->GetNumberOfElements());
    top->SetConnectivity(refGrid.GetTopology()->GetConnectivity());
    top->SetDeleteOnGridDelete(true);
    if (refGrid.GetTopology()->GetConnectivity()->GetHeavyDataSetName())
    {
      std::string currHeavyName =
          refGrid.GetTopology()->GetConnectivity()->GetHeavyDataSetName();
      currHeavyName = diffHeavyName + currHeavyName.substr(currHeavyName.find(
          ":/"), currHeavyName.size() - currHeavyName.find(":/"));
      top->GetConnectivity()->SetHeavyDataSetName(currHeavyName.c_str());
    }
    diffGrid->SetTopology(top);
    diffGridParent->Insert(diffGrid);
    diffGrid->Build();
    if (refGrid.GetTime()->GetTimeType() != XDMF_TIME_UNSET)
    {
      diffGrid->Insert(refGrid.GetTime());
    }
  }

  if (refGrid.GetGridType() == XDMF_GRID_COLLECTION)
  {
    if (newGrid.GetGridType() == XDMF_GRID_COLLECTION)
    {
      XdmfDiffReport diffReport = XdmfDiffReport("Collection Type");
      if (refGrid.GetCollectionType() != newGrid.GetCollectionType())
      {
        diffReport.AddError(0, refGrid.GetCollectionTypeAsString(),
            newGrid.GetCollectionTypeAsString());
      }
      errorReports.AddReport(refGrid.GetName(), diffReport);
    }
  }
  else
  {
    if (!IgnoreGeometry)
    {
      errorReports.AddReport(refGrid.GetName(), this->GetGeometryDiffs(
          refGrid.GetGeometry(), newGrid.GetGeometry()));
    }

    if (!IgnoreTopology)
    {
      errorReports.AddReport(refGrid.GetName(), this->GetTopologyDiffs(
          refGrid.GetTopology(), newGrid.GetTopology()));
    }

    if (!IgnoreAllAttributes)
    {
      for (int i = 0; i < refGrid.GetNumberOfAttributes(); i++)
      {
        refGrid.GetAttribute(i)->Update();
        XdmfAttribute * newAttribute = NULL;
        for (int j = 0; j < newGrid.GetNumberOfAttributes(); j++)
        {
          if (strcmp(newGrid.GetAttribute(j)->GetName(),
              refGrid.GetAttribute(i)->GetName()) == 0)
          {
            newAttribute = newGrid.GetAttribute(j);
            newAttribute->Update();
          }
        }
        if (newAttribute != NULL)
        {
          if (includedAttributes.size() != 0)
          {
            if (includedAttributes.find(refGrid.GetAttribute(i)->GetName())
                != includedAttributes.end())
            {
              errorReports.AddReport(
                  refGrid.GetName(),
                  this->GetAttributeDiffs(refGrid.GetAttribute(i), newAttribute));
            }
          }
          else
          {
            if (ignoredAttributes.size() != 0)
            {
              if (ignoredAttributes.find(refGrid.GetAttribute(i)->GetName())
                  == ignoredAttributes.end())
              {
                errorReports.AddReport(refGrid.GetName(),
                    this->GetAttributeDiffs(refGrid.GetAttribute(i),
                        newAttribute));
              }
            }
            else
            {
              errorReports.AddReport(
                  refGrid.GetName(),
                  this->GetAttributeDiffs(refGrid.GetAttribute(i), newAttribute));
            }
          }
        }
        else
        {
          std::stringstream heading;
          heading << "Attribute " << refGrid.GetAttribute(i)->GetName();
          XdmfDiffReport diffReport = XdmfDiffReport(heading.str());
          std::stringstream warning;
          warning << "Could Not Find " << "Attribute: "
              << refGrid.GetAttribute(i)->GetName();
          diffReport.AddError(warning.str());
          errorReports.AddReport(refGrid.GetName(), diffReport);
        }
      }
    }
  }

  if (!IgnoreTime)
  {
    if (refGrid.GetTime()->GetValue() != newGrid.GetTime()->GetValue())
    {
      XdmfDiffReport diffReport = XdmfDiffReport("Time");
      std::stringstream refTime;
      std::stringstream newTime;
      refTime << refGrid.GetTime()->GetValue();
      newTime << newGrid.GetTime()->GetValue();
      diffReport.AddError(0, refTime.str(), newTime.str());
      errorReports.AddReport(refGrid.GetName(), diffReport);
    }
  }

  if (CreateDiffFile && diffGridParent)
  {
    diffGrid->Build();
  }

  if (refGrid.GetNumberOfChildren() > 0)
  {
    if (CreateDiffFile && diffGridParent)
    {
      diffGridParent = diffGrid;
    }
    for (int i = 0; i < refGrid.GetNumberOfChildren(); i++)
    {
      XdmfGrid grid = XdmfGrid();
      grid.SetDOM(myRefDOM);
      grid.SetElement(refGrid.GetChild(i)->GetElement());
      grid.Update();
      this->GetDiffs(grid, errorReports);
    }
  }
}

XdmfDiffInternal::XdmfDiffReport
XdmfDiffInternal::GetGeometryDiffs(XdmfGeometry * refGeometry,
    XdmfGeometry * newGeometry)
{
  XdmfDiffReport diffReport = XdmfDiffReport("Geometry");

  if (refGeometry->GetGeometryType() != newGeometry->GetGeometryType())
  {
    diffReport.AddError("Geometry Type",
        refGeometry->GetGeometryTypeAsString(),
        newGeometry->GetGeometryTypeAsString());
  }

  switch (refGeometry->GetGeometryType())
    {
  case XDMF_GEOMETRY_XYZ:
    this->CompareValues(diffReport, refGeometry->GetPoints(),
        newGeometry->GetPoints(), 0,
        refGeometry->GetPoints()->GetNumberOfElements(), 3);
    break;
  case XDMF_GEOMETRY_XY:
    this->CompareValues(diffReport, refGeometry->GetPoints(),
        newGeometry->GetPoints(), 0,
        refGeometry->GetPoints()->GetNumberOfElements(), 2);
    break;
  default:
    this->CompareValues(diffReport, refGeometry->GetPoints(),
        newGeometry->GetPoints(), 0,
        refGeometry->GetPoints()->GetNumberOfElements());
    }

  return diffReport;
}

XdmfDiffInternal::XdmfDiffReport
XdmfDiffInternal::GetTopologyDiffs(XdmfTopology * refTopology,
    XdmfTopology * newTopology)
{
  XdmfDiffReport diffReport = XdmfDiffReport("Topology");

  if (refTopology->GetTopologyType() != newTopology->GetTopologyType())
  {
    diffReport.AddError("Topology Type",
        refTopology->GetTopologyTypeAsString(),
        newTopology->GetTopologyTypeAsString());
  }

  this->CompareValues(diffReport, refTopology->GetConnectivity(),
      newTopology->GetConnectivity(), 0, refTopology->GetNumberOfElements(),
      refTopology->GetNodesPerElement());

  return diffReport;
}

XdmfDiffInternal::XdmfDiffReport
XdmfDiffInternal::GetAttributeDiffs(XdmfAttribute * refAttribute,
    XdmfAttribute * newAttribute)
{
  std::stringstream valType;
  valType << "Attribute " << refAttribute->GetName();
  XdmfDiffReport diffReport = XdmfDiffReport(valType.str());

  int numValsPerNode = 1;
  switch (refAttribute->GetAttributeType())
    {
  case XDMF_ATTRIBUTE_TYPE_VECTOR:
    numValsPerNode = 3;
    break;
  case XDMF_ATTRIBUTE_TYPE_TENSOR6:
    numValsPerNode = 6;
    break;
  case XDMF_ATTRIBUTE_TYPE_TENSOR:
    numValsPerNode = 9;
    break;
  default:
    numValsPerNode = 1;
    break;
    }

  if (refAttribute->GetAttributeCenter() != newAttribute->GetAttributeCenter())
  {
    diffReport.AddError("Attribute Center",
        refAttribute->GetAttributeCenterAsString(),
        newAttribute->GetAttributeCenterAsString());
  }

  if (refAttribute->GetAttributeType() != newAttribute->GetAttributeType())
  {
    numValsPerNode = 1;
    diffReport.AddError("Attribute Type",
        refAttribute->GetAttributeTypeAsString(),
        newAttribute->GetAttributeTypeAsString());
  }

  XdmfArray * diffs = this->CompareValues(diffReport,
      refAttribute->GetValues(), newAttribute->GetValues(), 0,
      refAttribute->GetValues()->GetNumberOfElements(), numValsPerNode);

  if (CreateDiffFile && diffs && diffGrid)
  {
    XdmfAttribute * attr = new XdmfAttribute();
    attr->SetLightDataLimit(0);
    attr->SetName(refAttribute->GetName());
    attr->SetAttributeType(refAttribute->GetAttributeType());
    attr->SetAttributeCenter(refAttribute->GetAttributeCenter());
    attr->SetValues(diffs);
    attr->SetDeleteOnGridDelete(true);
    diffGrid->Insert(attr);
  }

  return diffReport;
}

XdmfArray *
XdmfDiffInternal::CompareValues(XdmfDiffReport & errorReport,
    XdmfArray * refArray, XdmfArray * newArray, XdmfInt64 startIndex,
    XdmfInt64 numValues, XdmfInt64 groupLength)
{
  switch (refArray->GetNumberType())
    {
  case XDMF_FLOAT64_TYPE:
    return this->CompareValuesPriv<XdmfFloat64> (errorReport, refArray,
        newArray, startIndex, numValues, groupLength);
  case XDMF_FLOAT32_TYPE:
    return this->CompareValuesPriv<XdmfFloat32> (errorReport, refArray,
        newArray, startIndex, numValues, groupLength);
  case XDMF_INT64_TYPE:
    return this->CompareValuesPriv<XdmfInt64> (errorReport, refArray, newArray,
        startIndex, numValues, groupLength);
  case XDMF_INT32_TYPE:
    return this->CompareValuesPriv<XdmfInt32> (errorReport, refArray, newArray,
        startIndex, numValues, groupLength);
  case XDMF_INT16_TYPE:
    return this->CompareValuesPriv<XdmfInt16> (errorReport, refArray, newArray,
        startIndex, numValues, groupLength);
  case XDMF_INT8_TYPE:
    return this->CompareValuesPriv<XdmfInt8> (errorReport, refArray, newArray,
        startIndex, numValues, groupLength);
  case XDMF_UINT32_TYPE:
    return this->CompareValuesPriv<XdmfUInt32> (errorReport, refArray,
        newArray, startIndex, numValues, groupLength);
  case XDMF_UINT16_TYPE:
    return this->CompareValuesPriv<XdmfUInt16> (errorReport, refArray,
        newArray, startIndex, numValues, groupLength);
  case XDMF_UINT8_TYPE:
    return this->CompareValuesPriv<XdmfUInt8> (errorReport, refArray, newArray,
        startIndex, numValues, groupLength);
  default:
    return this->CompareValuesPriv<XdmfFloat64> (errorReport, refArray,
        newArray, startIndex, numValues, groupLength);
    }
  return NULL;
}

template<class XdmfType>
  XdmfArray *
  XdmfDiffInternal::CompareValuesPriv(XdmfDiffReport & errorReport,
      XdmfArray * refArray, XdmfArray * newArray, XdmfInt64 startIndex,
      XdmfInt64 numValues, XdmfInt64 groupLength)
  {

    if (groupLength < 1)
    {
      return NULL;
    }

    if (refArray->GetNumberOfElements() != newArray->GetNumberOfElements())
    {
      std::stringstream refArrayElem;
      std::stringstream newArrayElem;
      refArrayElem << refArray->GetNumberOfElements();
      newArrayElem << newArray->GetNumberOfElements();
      errorReport.AddError("Number of Elements", refArrayElem.str(),
          newArrayElem.str());
    }

    if (strcmp(refArray->GetShapeAsString(), newArray->GetShapeAsString()) != 0)
    {
      errorReport.AddError("Shape", refArray->GetShapeAsString(),
          newArray->GetShapeAsString());
    }

    if (refArray->GetNumberType() != newArray->GetNumberType())
    {
      errorReport.AddError("Number Type", refArray->GetNumberTypeAsString(),
          newArray->GetNumberTypeAsString());
    }

    XdmfType * refVals = (XdmfType*) refArray->GetDataPointer(startIndex);
    XdmfType * newVals = (XdmfType*) newArray->GetDataPointer(startIndex);

    XdmfArray * toReturn = new XdmfArray();
    if (refArray->GetHeavyDataSetName())
    {
      std::string currHeavyName = refArray->GetHeavyDataSetName();
      currHeavyName = diffHeavyName + currHeavyName.substr(currHeavyName.find(
          ":/"), currHeavyName.size() - currHeavyName.find(":/"));
      toReturn->SetHeavyDataSetName(currHeavyName.c_str());
    }
    toReturn->SetNumberType(refArray->GetNumberType());
    toReturn->SetNumberOfElements(refArray->GetNumberOfElements());
    XdmfType * toReturnArray = (XdmfType*) toReturn->GetDataPointer(startIndex);

    for (int i = 0; i < numValues; i++)
    {
      double acceptableError = fabs(AbsoluteError);
      if (acceptableError == 0)
      {
        acceptableError = fabs(refVals[startIndex + i] * RelativeError);
      }
      XdmfType diff = newVals[startIndex + i] - refVals[startIndex + i];
      toReturnArray[startIndex + i] = diff;
      if (fabs((float) diff) > acceptableError)
      {
        XdmfInt64 groupIndex = (int) ((startIndex + i) / groupLength);
        std::stringstream refValsReturn;
        std::stringstream newValsReturn;
        for (int j = 0; j < groupLength; j++)
        {
          refValsReturn << refVals[startIndex + i + j];
          newValsReturn << newVals[startIndex + i + j];
          if (j > 0)
          {
            toReturnArray[startIndex + i + j] = newVals[startIndex + i + j]
                - refVals[startIndex + i + j];
          }
          if (j + 1 < groupLength)
          {
            refValsReturn << ", ";
            newValsReturn << ", ";
          }
        }
        errorReport.AddError("Values", groupIndex, refValsReturn.str(),
            newValsReturn.str());
        i = startIndex + i + groupLength - 1;
      }
    }
    return toReturn;
  }

XdmfBoolean
XdmfDiffInternal::AreEquivalent()
{
  XdmfDiffReportCollection myErrors = XdmfDiffReportCollection(
      DisplayFailuresOnly, VerboseOutput);
  this->GetDiffs(myErrors);
  if (myErrors.GetNumberOfErrors() == 0)
  {
    return true;
  }
  return false;
}

/**
 *
 * Parses a file containing settings for the comparison.  Settings are outlined at the top
 * of this file.  Commented lines starting with '#' are ignored.
 *
 * @param settingsFile the path to the settings file
 *
 */

XdmfInt32
XdmfDiffInternal::ParseSettingsFile(XdmfConstString settingsFile)
{
  std::string str;
  ifstream ifs(settingsFile, ifstream::in);
  while (std::getline(ifs, str))
  {
    if (str != "")
    {
      std::string buf;
      std::stringstream ss(str);

      std::vector<std::string> tokens;
      while (ss >> buf)
      {
        tokens.push_back(buf);
      }

      if (tokens[0].compare("RELATIVE_ERROR") == 0)
      {
        std::istringstream stm;
        stm.str(tokens[1]);
        double d;
        stm >> d;
        this->SetRelativeError(d);
      }

      if (tokens[0].compare("ABSOLUTE_ERROR") == 0)
      {
        std::istringstream stm;
        stm.str(tokens[1]);
        double d;
        stm >> d;
        this->SetAbsoluteError(d);
      }

      if (tokens[0].compare("INCLUDE_GRID") == 0)
      {
        for (unsigned int i = 1; i < tokens.size(); i++)
        {
          includedGrids.insert(tokens[i]);
        }
      }

      if (tokens[0].compare("IGNORE_GRID") == 0)
      {
        for (unsigned int i = 1; i < tokens.size(); i++)
        {
          ignoredGrids.insert(tokens[i]);
        }
      }

      if (tokens[0].compare("IGNORE_TIME") == 0)
      {
        this->SetIgnoreTime(true);
      }

      if (tokens[0].compare("IGNORE_GEOMETRY") == 0)
      {
        this->SetIgnoreGeometry(true);
      }

      if (tokens[0].compare("IGNORE_TOPOLOGY") == 0)
      {
        this->SetIgnoreTopology(true);
      }

      if (tokens[0].compare("INCLUDE_ATTRIBUTE") == 0)
      {
        for (unsigned int i = 1; i < tokens.size(); i++)
        {
          includedAttributes.insert(tokens[i]);
        }
      }

      if (tokens[0].compare("IGNORE_ATTRIBUTE") == 0)
      {
        for (unsigned int i = 1; i < tokens.size(); i++)
        {
          ignoredAttributes.insert(tokens[i]);
        }
      }

      if (tokens[0].compare("IGNORE_ALL_ATTRIBUTES") == 0)
      {
        this->SetIgnoreAllAttributes(true);
      }

      if (tokens[0].compare("DISPLAY_FAILURES_ONLY") == 0)
      {
        this->SetDisplayFailuresOnly(true);
      }

      if (tokens[0].compare("VERBOSE_OUTPUT") == 0)
      {
        this->SetVerboseOutput(true);
      }
    }
  }
  ifs.close();
  return XDMF_SUCCESS;
}

#else

/**
 * Entry point for command line utility
 *
 */
int
main(int argc, char* argv[])
{

  XdmfDiff * diffFramework;

  std::string usage = "Compares Xdmf files for equality: \n \n Usage: \n \n   XdmfDiff <path-to-reference-xdmf-file> <path-to-xdmf-file> (Optional: <path-to-settings-file>)";

  if (argc < 3)
  {
    cout << usage << endl;
    return 1;
  }

  if (argc >= 3)
  {
    FILE * refFile = fopen(argv[1], "r");
    if (refFile)
    {
      // Success
      fclose(refFile);
    }
    else
    {
      cout << "Cannot open: " << argv[1] << endl;
      return 1;
    }

    FILE * newFile = fopen(argv[2], "r");
    if (newFile)
    {
      // Success
      fclose(newFile);
    }
    else
    {
      cout << "Cannot open: " << argv[2] << endl;
      return 1;
    }

    diffFramework = new XdmfDiff(argv[1], argv[2]);

    if (argc >= 4)
    {
      FILE * defFile = fopen(argv[3], "r");
      if (defFile)
      {
        // Success
        fclose(defFile);
      }
      else
      {
        cout << "Cannot open: " << argv[3] << endl;
        delete diffFramework;
        return 1;
      }
      diffFramework->ParseSettingsFile(argv[3]);
    }
  }

  //diffFramework.SetCreateDiffFile(true);
  std::string output = diffFramework->GetDiffs();
  cout << output << endl;
  delete diffFramework;
  return 0;
}

#endif // BUILD_EXE
