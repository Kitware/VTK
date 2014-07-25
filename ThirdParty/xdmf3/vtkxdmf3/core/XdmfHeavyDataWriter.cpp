/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfHeavyDataWriter.cpp                                             */
/*                                                                           */
/*  Author:                                                                  */
/*     Kenneth Leiter                                                        */
/*     kenneth.leiter@arl.army.mil                                           */
/*     US Army Research Laboratory                                           */
/*     Aberdeen Proving Ground, MD                                           */
/*                                                                           */
/*     Copyright @ 2011 US Army Research Laboratory                          */
/*     All Rights Reserved                                                   */
/*     See Copyright.txt for details                                         */
/*                                                                           */
/*     This software is distributed WITHOUT ANY WARRANTY; without            */
/*     even the implied warranty of MERCHANTABILITY or FITNESS               */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice             */
/*     for more information.                                                 */
/*                                                                           */
/*****************************************************************************/

#include "XdmfHeavyDataWriter.hpp"
#include "XdmfHeavyDataController.hpp"
#include "XdmfArray.hpp"
#include "XdmfArrayType.hpp"
#include "XdmfError.hpp"
#include "XdmfSystemUtils.hpp"
#include <cstdio>
#include <list>
#include <vector>

XdmfHeavyDataWriter::XdmfHeavyDataWriter(const double compression,
                                         const unsigned int overhead) :
  mAllowSplitDataSets(false),
  mDataSetId(0),
  mFileIndex(0),
  mFilePath(""),
  mFileSizeLimit(0),
  mMode(Default),
  mCompressionRatio(compression),
  mFileOverhead(overhead)
{
}

XdmfHeavyDataWriter::XdmfHeavyDataWriter(const std::string & filePath,
                                         const double compression,
                                         const unsigned int overhead) :
  mAllowSplitDataSets(false),
  mDataSetId(0),
  mFileIndex(0),
  mFilePath(XdmfSystemUtils::getRealPath(filePath)),
  mFileSizeLimit(0),
  mMode(Default),
  mReleaseData(false),
  mCompressionRatio(compression),
  mFileOverhead(overhead)
{
}

XdmfHeavyDataWriter::~XdmfHeavyDataWriter()
{
}

int
XdmfHeavyDataWriter::getAllowSetSplitting()
{
  return mAllowSplitDataSets;
}

int
XdmfHeavyDataWriter::getFileIndex()
{
  return mFileIndex;
}

unsigned int
XdmfHeavyDataWriter::getFileOverhead()
{
  return mFileOverhead;
}

std::string
XdmfHeavyDataWriter::getFilePath() const
{
  if (mFilePath.c_str() == NULL) {
    return "";
  }
  else {
    return mFilePath;
  }
}

int
XdmfHeavyDataWriter::getFileSizeLimit()
{
  return mFileSizeLimit;
}

XdmfHeavyDataWriter::Mode
XdmfHeavyDataWriter::getMode() const
{
  return mMode;
}

bool 
XdmfHeavyDataWriter::getReleaseData() const
{
  return mReleaseData;
}

void
XdmfHeavyDataWriter::setAllowSetSplitting(bool newAllow)
{
  mAllowSplitDataSets = newAllow;
}

void
XdmfHeavyDataWriter::setFileIndex(int newSize)
{
  mFileIndex = newSize;
}

void
XdmfHeavyDataWriter::setFileSizeLimit(int newSize)
{
  mFileSizeLimit = newSize;
}

void
XdmfHeavyDataWriter::setMode(const Mode mode)
{
  mMode = mode;
}

void
XdmfHeavyDataWriter::setReleaseData(const bool releaseData)
{
  mReleaseData = releaseData;
}
