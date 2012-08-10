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

#ifndef XDMFDIFF_H_
#define XDMFDIFF_H_

#include <XdmfArray.h>
#include <XdmfGrid.h>
#include <XdmfDOM.h>
#include <XdmfDomain.h>
#include <XdmfRoot.h>

class XdmfDiffInternal;

#if defined(WIN32) && !defined(XDMFSTATIC)

// Windows and DLL configuration
#if defined(XdmfUtils_EXPORTS)
#define XDMF_UTILS_DLL __declspec(dllexport)
#else
#define XDMF_UTILS_DLL __declspec(dllimport)
#endif

#else

// Linux or static configuration
#define XDMF_UTILS_DLL

#endif

class XDMF_UTILS_DLL XdmfDiff
{
public:

  /*
   * Constructs an XdmfDiff object to compare two Xdmf Files
   *
   * @param refFileName the path to an Xdmf file to compare
   * @param newFileName the path to an Xdmf file to compare
   *
   */
  XdmfDiff(XdmfConstString refFileName, XdmfConstString newFileName);

  /*
   * Constructs an XdmfDiff object to compare two Xdmf Files
   *
   * @param refDOM an XdmfDOM to compare
   * @param newDOM an XdmfDOM to compare
   *
   */
  XdmfDiff(XdmfDOM * refDOM, XdmfDOM * newDOM);

  /*
   * Destructor
   *
   */
  ~XdmfDiff();

  /*
   * Get the differences between two Xdmf files
   *
   * @return an XdmfConstString of differences between the files
   */
  std::string
  GetDiffs();

  /*
   * Get the differences between grids in two Xdmf files
   *
   * @param gridName the name of the grid to compare
   *
   * @return an XdmfConstString of differences between the grids
   */
  std::string GetDiffs(XdmfConstString gridName);

  XdmfInt32 SetIgnoreTime(XdmfBoolean value = true);
  XdmfInt32 GetIgnoreTime();
  XdmfInt32 SetIgnoreGeometry(XdmfBoolean value = true);
  XdmfInt32 GetIgnoreGeometry();
  XdmfInt32 SetIgnoreTopology(XdmfBoolean value = true);
  XdmfInt32 GetIgnoreTopology();
  XdmfInt32 SetIgnoreAllAttributes(XdmfBoolean value = true);
  XdmfInt32 GetIgnoreAllAttributes();
  XdmfInt32 SetDisplayFailuresOnly(XdmfBoolean value = true);
  XdmfInt32 GetDisplayFailuresOnly();
  XdmfInt32 SetVerboseOutput(XdmfBoolean value = true);
  XdmfInt32 GetVerboseOutput();
  XdmfInt32 SetCreateDiffFile(XdmfBoolean value = true);
  XdmfInt32 GetCreateDiffFile();
  XdmfInt32 SetDiffFileName(XdmfString value);
  XdmfString GetDiffFileName();

  /*
   * Sets the acceptable relative error between values.  Relative Errors and Absolute Errors
   * can not be used at the same time.
   *
   * @param relativeError the acceptable relative error in decimal form
   *
   */
  XdmfInt32 SetRelativeError(XdmfFloat64 relativeError);
  XdmfFloat64 GetRelativeError();

  /*
   * Sets the acceptable absolute error between values.  Relative Errors and Absolute Errors
   * can not be used at the same time.
   *
   * @param absoluteError the acceptable absolute error
   *
   */
  XdmfInt32 SetAbsoluteError(XdmfFloat64 absoluteError);
  XdmfFloat64 GetAbsoluteError();
  XdmfInt32 IncludeGrid(XdmfString gridName);
  XdmfInt32 IgnoreGrid(XdmfString gridName);
  XdmfInt32 IncludeAttribute(XdmfString attributeName);
  XdmfInt32 IgnoreAttribute(XdmfString attributeName);

  XdmfInt32 ParseSettingsFile(XdmfConstString settingsFile);

  /*
   * Determines whether the two files are equivalent.
   *
   * @return an XdmfBoolean true = equivalent, false = nonequivalent
   *
   */
  XdmfBoolean AreEquivalent();

private:
  XdmfDiffInternal * myInternal;
};

#endif /* XDMFDIFF_H_ */
