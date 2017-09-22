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
#include "string.h"

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

// C Wrappers

void XdmfHeavyDataWriterFree(XDMFHEAVYDATAWRITER * item)
{
  if (item != NULL) {
    delete (shared_ptr<XdmfHeavyDataWriter> *) item;
    item = NULL;
  }
}

int XdmfHeavyDataWriterGetAllowSetSplitting(XDMFHEAVYDATAWRITER * writer)
{
  shared_ptr<XdmfHeavyDataWriter> & refWriter = *(shared_ptr<XdmfHeavyDataWriter> *)(writer);
  return refWriter->getAllowSetSplitting();
}

int XdmfHeavyDataWriterGetFileIndex(XDMFHEAVYDATAWRITER * writer)
{
  shared_ptr<XdmfHeavyDataWriter> & refWriter = *(shared_ptr<XdmfHeavyDataWriter> *)(writer);
  return refWriter->getFileIndex();
}

unsigned int XdmfHeavyDataWriterGetFileOverhead(XDMFHEAVYDATAWRITER * writer)
{
  shared_ptr<XdmfHeavyDataWriter> & refWriter = *(shared_ptr<XdmfHeavyDataWriter> *)(writer);
  return refWriter->getFileOverhead();
}

char * XdmfHeavyDataWriterGetFilePath(XDMFHEAVYDATAWRITER * writer)
{
  shared_ptr<XdmfHeavyDataWriter> & refWriter = *(shared_ptr<XdmfHeavyDataWriter> *)(writer);
  char * returnPointer = strdup(refWriter->getFilePath().c_str());
  return returnPointer;
}

int XdmfHeavyDataWriterGetFileSizeLimit(XDMFHEAVYDATAWRITER * writer)
{
  shared_ptr<XdmfHeavyDataWriter> & refWriter = *(shared_ptr<XdmfHeavyDataWriter> *)(writer);
  return refWriter->getFileSizeLimit();
}

int XdmfHeavyDataWriterGetMode(XDMFHEAVYDATAWRITER * writer)
{
  shared_ptr<XdmfHeavyDataWriter> & refWriter = *(shared_ptr<XdmfHeavyDataWriter> *)(writer);
  XdmfHeavyDataWriter::Mode checkMode = refWriter->getMode();
  if (checkMode == XdmfHeavyDataWriter::Default) {
    return XDMF_HEAVY_WRITER_MODE_DEFAULT;
  }
  else if (checkMode == XdmfHeavyDataWriter::Overwrite) {
    return XDMF_HEAVY_WRITER_MODE_OVERWRITE;
  }
  else if (checkMode == XdmfHeavyDataWriter::Append) {
    return XDMF_HEAVY_WRITER_MODE_APPEND;
  }
  else if (checkMode == XdmfHeavyDataWriter::Hyperslab) {
    return XDMF_HEAVY_WRITER_MODE_HYPERSLAB;
  }
  return -1;
}

int XdmfHeavyDataWriterGetReleaseData(XDMFHEAVYDATAWRITER * writer)
{
  shared_ptr<XdmfHeavyDataWriter> & refWriter = *(shared_ptr<XdmfHeavyDataWriter> *)(writer);
  return refWriter->getReleaseData();
}

void XdmfHeavyDataWriterSetAllowSetSplitting(XDMFHEAVYDATAWRITER * writer, int newAllow)
{
  shared_ptr<XdmfHeavyDataWriter> & refWriter = *(shared_ptr<XdmfHeavyDataWriter> *)(writer);
  refWriter->setAllowSetSplitting(newAllow);
}

void XdmfHeavyDataWriterSetFileIndex(XDMFHEAVYDATAWRITER * writer, int newIndex)
{
  shared_ptr<XdmfHeavyDataWriter> & refWriter = *(shared_ptr<XdmfHeavyDataWriter> *)(writer);
  refWriter->setFileIndex(newIndex);
}

void XdmfHeavyDataWriterSetFileSizeLimit(XDMFHEAVYDATAWRITER * writer, int newSize)
{
  shared_ptr<XdmfHeavyDataWriter> & refWriter = *(shared_ptr<XdmfHeavyDataWriter> *)(writer);
  refWriter->setFileSizeLimit(newSize);
}

void XdmfHeavyDataWriterSetMode(XDMFHEAVYDATAWRITER * writer, int mode, int * status)
{
  shared_ptr<XdmfHeavyDataWriter> & refWriter = *(shared_ptr<XdmfHeavyDataWriter> *)(writer);
  XDMF_ERROR_WRAP_START(status)
  XdmfHeavyDataWriter::Mode newMode;
  switch (mode) {
    case XDMF_HEAVY_WRITER_MODE_DEFAULT:
      newMode = XdmfHeavyDataWriter::Default;
      break;
    case XDMF_HEAVY_WRITER_MODE_OVERWRITE:
      newMode = XdmfHeavyDataWriter::Overwrite;
      break;
    case XDMF_HEAVY_WRITER_MODE_APPEND:
      newMode = XdmfHeavyDataWriter::Append;
      break;
    case XDMF_HEAVY_WRITER_MODE_HYPERSLAB:
      newMode = XdmfHeavyDataWriter::Hyperslab;
      break;
    default:
      XdmfError::message(XdmfError::FATAL,
                         "Error: Invalid heavy writer mode.");
  }
  refWriter->setMode(newMode);
  XDMF_ERROR_WRAP_END(status)
}

void XdmfHeavyDataWriterSetReleaseData(XDMFHEAVYDATAWRITER * writer, int releaseData)
{
  shared_ptr<XdmfHeavyDataWriter> & refWriter = *(shared_ptr<XdmfHeavyDataWriter> *)(writer);
  refWriter->setReleaseData(releaseData);
}
