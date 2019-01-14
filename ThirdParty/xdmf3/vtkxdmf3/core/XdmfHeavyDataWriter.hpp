/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfHeavyDataWriter.hpp                                             */
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

#ifndef XDMFHEAVYDATAWRITER_HPP_
#define XDMFHEAVYDATAWRITER_HPP_

#include "XdmfCore.hpp"
#include "XdmfArrayType.hpp"
#include "XdmfHeavyDataController.hpp"
#include "XdmfVisitor.hpp"

#ifdef __cplusplus

// Forward Declarations
class XdmfArray;

// Includes
#include <string>
#include <list>

/**
 * @brief Traverses the Xdmf graph and writes heavy data stored in
 * XdmfArrays to heavy data files on disk.
 *
 * This is an abstract base class to support the writing of different
 * heavy data formats.
 *
 * XdmfHeavyDataWriter traverses an Xdmf graph structure and writes
 * data stored in XdmfArrays to heavy data files on disk. Writing
 * begins by calling the accept() operation on any XdmfItem and
 * supplying this writer as the parameter. The writer will write all
 * XdmfArrays under the XdmfItem to a heavy data file on disk. It will
 * also attach an XdmfHeavyDataController to all XdmfArrays that it
 * writes to disk.
 *
 * There are three modes of operation for this writer:
 *   Default - All initialized XdmfArrays are written to new heavy datasets
 *             regardless of whether they are attached to another heavy
 *             dataset on disk via an XdmfHeavyDataController.
 *   Overwrite - If an initialized XdmfArray is attached to an heavy dataset
 *               via an XdmfHeavyDataController the writer will write values
 *               to that location, overwriting all previous written values.
 *               The dataset on disk will be resized appropriately.
 *   Append - If an initialized XdmfArray is attached to an heavy dataset via
 *            an XdmfHeavyDataController the writer will append the values to
 *            the end of the dataset on disk.
 *   Hyperslab - If an initialized XdmfArray is attached to a heavy dataset
 *               via an XdmfHeavyDataController the writer will write to a
 *               hyperslab in the dataset based on the start, stride, and
 *               dimensions of the XdmfHeavyDataController.
 */
class XDMFCORE_EXPORT XdmfHeavyDataWriter : public XdmfVisitor,
                                            public Loki::Visitor<XdmfArray> {

public:

  enum Mode {
    Default,
    Overwrite,
    Append,
    Hyperslab
  };

  virtual ~XdmfHeavyDataWriter() = 0;

  /**
   * Close file. This is only needed when the file is opened manually
   * through openFile().
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfHeavyDataWriter.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#closeFile
   * @until //#closeFile
   *
   * Python
   *
   * @dontinclude XdmfExampleHeavyDataWriter.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//closeFile
   * @until #//closeFile
   */
  virtual void closeFile() = 0;

  /**
   * Gets whether the HDF5 Writer is allowed to split data sets when writing to hdf5.
   * Splitting should only occur for massive data sets.
   * Setting to false assures compatibility with previous editions.
   * Default setting is false.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfHeavyDataWriter.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getAllowSetSplitting
   * @until //#getAllowSetSplitting
   *
   * Python
   *
   * @dontinclude XdmfExampleHeavyDataWriter.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getAllowSetSplitting
   * @until #//getAllowSetSplitting
   *
   * @return    Whether to allow data sets to be split across hdf5 files
   */
  int getAllowSetSplitting();

  /**
   * Gets the file index. Used when file splitting and incremented whent he current file is full.
   *
   * C++
   *
   * @dontinclude ExampleXdmfHeavyDataWriter.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getFileIndex
   * @until //#getFileIndex
   *
   * Python
   *
   * @dontinclude XdmfExampleHeavyDataWriter.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getFileIndex
   * @until #//getFileIndex
   *
   * @return    The current file index.
   */
  int getFileIndex();

  /**
   * Gets the amount of bytes that the heavy data writer uses as overhead for the data type.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfHeavyDataWriter.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getFileOverhead
   * @until //#getFileOverhead
   *
   * Python
   *
   * @dontinclude XdmfExampleHeavyDataWriter.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getFileOverhead
   * @until #//getFileOverhead
   *
   * @return    Amount of bytes used as overhead
   */
  unsigned int getFileOverhead();

  /**
   * Get the path to the heavy data file on disk this writer is writing to.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfHeavyDataWriter.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getFilePath
   * @until //#getFilePath
   *
   * Python
   *
   * @dontinclude XdmfExampleHeavyDataWriter.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getFilePath
   * @until #//getFilePath
   *
   * @return    A std::string containing the path to the heavy file on disk this
   *            writer is writing to.
   */
  std::string getFilePath() const;

  /**
   * Gets the file size limit of the HDF5 files produced by the writer in MB. Overflow is pushed to a new HDF5 file.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfHeavyDataWriter.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getFileSizeLimit
   * @until //#getFileSizeLimit
   *
   * Python
   *
   * @dontinclude XdmfExampleHeavyDataWriter.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getFileSizeLimit
   * @until #//getFileSizeLimit
   *
   * @return    The size limit in MB
   */
  int getFileSizeLimit();

  /**
   * Get the Mode of operation for this writer.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfHeavyDataWriter.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getMode
   * @until //#getMode
   *
   * Python
   *
   * @dontinclude XdmfExampleHeavyDataWriter.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getMode
   * @until #//getMode
   *
   * @return    The Mode of operation for this writer.
   */
  Mode getMode() const;

  /**
   * Get whether to release data from memory after writing to disk.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfHeavyDataWriter.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getReleaseData
   * @until //#getReleaseData
   *
   * Python
   *
   * @dontinclude XdmfExampleHeavyDataWriter.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getReleaseData
   * @until #//getReleaseData
   *
   * @return    True if data is freed after writing
   */
  bool getReleaseData() const;

  /**
   * Open file for writing. This is an optional command that can
   * improve performance for some writers when writing many datasets
   * to a single file. User must call closeFile() after completing
   * output.
   *
   * By default, heavy data files are open and closed before and after
   * writing each dataset to ensure that other writers have access to
   * the file (we never know whether we will be writing to the file
   * again). This is expensive in some cases, but is always
   * safe. Opening the file once and writing many datasets may result
   * in improved performance, but the user must tell the writer when
   * to open and close the file.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfHeavyDataWriter.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#openFile
   * @until //#openFile
   *
   * Python
   *
   * @dontinclude XdmfExampleHeavyDataWriter.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//openFile
   * @until #//openFile
   */
  virtual void openFile() = 0;

  /**
   * Sets whether to allow the HDF5 writer to split data sets when writing to hdf5.
   * Splitting should only occur for massive data sets.
   * Setting to false assures compatibility with previous editions.
   * Default setting is false
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfHeavyDataWriter.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#setAllowSetSplitting
   * @until //#setAllowSetSplitting
   *
   * Python
   *
   * @dontinclude XdmfExampleHeavyDataWriter.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//setAllowSetSplitting
   * @until #//setAllowSetSplitting
   *
   * @param     newAllow        Whether to allow data sets to be split across hdf5 files
   */
  void setAllowSetSplitting(bool newAllow);

  /**
   * Sets the file index. Used when file splitting and incremented when the current file is full. Set to 0 before using hyperslab or overwrite.
   *
   * C++
   *
   * @dontinclude ExampleXdmfHeavyDataWriter.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getFileIndex
   * @until //#getFileIndex
   *
   * Python
   *
   * @dontinclude XdmfExampleHeavyDataWriter.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getFileIndex
   * @until #//getFileIndex
   *
   * @param     newIndex        The index that the writer will append to the file name when incorperating file splitting
   */
  void setFileIndex(int newIndex);

  /**
   * Sets the file size limit of the HDF5 files produced by the writer in MB. Overflow is pushed to a new HDF5 file.
   * Using with arrays of string type may reduce performance.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfHeavyDataWriter.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#setFileSizeLimit
   * @until //#setFileSizeLimit
   *
   * Python
   *
   * @dontinclude XdmfExampleHeavyDataWriter.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//setFileSizeLimit
   * @until #//setFileSizeLimit
   *
   * @param     newSize         The size limit in MB
   */
  void setFileSizeLimit(int newSize);

  /**
   * Set the mode of operation for this writer.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfHeavyDataWriter.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#setMode
   * @until //#setMode
   *
   * Python
   *
   * @dontinclude XdmfExampleHeavyDataWriter.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//setMode
   * @until #//setMode
   *
   * @param     mode    The Mode of operation for this writer.
   */
  void setMode(const Mode mode);

  /**
   * Set whether to release data from memory after writing to disk.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfHeavyDataWriter.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#setReleaseData
   * @until //#setReleaseData
   *
   * Python
   *
   * @dontinclude XdmfExampleHeavyDataWriter.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//setReleaseData
   * @until #//setReleaseData
   *
   * @param     releaseData     True if data should be freed after writing
   */
  void setReleaseData(const bool releaseData = true);

  /**
   * Write an XdmfArray to heavy data file on disk.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfHeavyDataWriter.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#visit
   * @until //#visit
   *
   * Python
   *
   * @dontinclude XdmfExampleHeavyDataWriter.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//visit
   * @until #//visit
   *
   * @param     array           An XdmfArray to write to heavy data.
   * @param     visitor         A smart pointer to this visitor --- aids in grid traversal.
   */
  using XdmfVisitor::visit;
  virtual void visit(XdmfArray & array,
                     const shared_ptr<XdmfBaseVisitor> visitor) = 0;

protected:

  XdmfHeavyDataWriter(const double compression = 1, const unsigned int overhead = 0);
  XdmfHeavyDataWriter(const std::string & filePath, const double compression = 1, const unsigned int overhead = 0);

  virtual shared_ptr<XdmfHeavyDataController>
  createController(const std::string & filePath,
                   const std::string & descriptor,
                   const shared_ptr<const XdmfArrayType> type,
                   const std::vector<unsigned int> & start,
                   const std::vector<unsigned int> & stride,
                   const std::vector<unsigned int> & dimensions,
                   const std::vector<unsigned int> & dataspaceDimensions) = 0;

  virtual int getDataSetSize(shared_ptr<XdmfHeavyDataController> descriptionController) = 0;

  bool mAllowSplitDataSets;
  int mDataSetId;
  int mFileIndex;
  std::string mFilePath;
  unsigned int mFileSizeLimit;
  Mode mMode;
  bool mReleaseData;
  double  mCompressionRatio;
  unsigned int mFileOverhead;

private:

  XdmfHeavyDataWriter(const XdmfHeavyDataWriter &); // Not implemented.
  void operator=(const XdmfHeavyDataWriter &);  // Not implemented.

};

#endif

#ifdef __cplusplus
extern "C" {
#endif

struct XDMFHEAVYDATAWRITER; // Simply as a typedef to ensure correct typing
typedef struct XDMFHEAVYDATAWRITER XDMFHEAVYDATAWRITER;

#define XDMF_HEAVY_WRITER_MODE_DEFAULT   20
#define XDMF_HEAVY_WRITER_MODE_OVERWRITE 21
#define XDMF_HEAVY_WRITER_MODE_APPEND    22
#define XDMF_HEAVY_WRITER_MODE_HYPERSLAB 23

// C wrappers go here

XDMFCORE_EXPORT void XdmfHeavyDataWriterFree(XDMFHEAVYDATAWRITER * item);

XDMFCORE_EXPORT int XdmfHeavyDataWriterGetAllowSetSplitting(XDMFHEAVYDATAWRITER * writer);

XDMFCORE_EXPORT int XdmfHeavyDataWriterGetFileIndex(XDMFHEAVYDATAWRITER * writer);

XDMFCORE_EXPORT unsigned int XdmfHeavyDataWriterGetFileOverhead(XDMFHEAVYDATAWRITER * writer);

XDMFCORE_EXPORT char * XdmfHeavyDataWriterGetFilePath(XDMFHEAVYDATAWRITER * writer);

XDMFCORE_EXPORT int XdmfHeavyDataWriterGetFileSizeLimit(XDMFHEAVYDATAWRITER * writer);

XDMFCORE_EXPORT int XdmfHeavyDataWriterGetMode(XDMFHEAVYDATAWRITER * writer);

XDMFCORE_EXPORT int XdmfHeavyDataWriterGetReleaseData(XDMFHEAVYDATAWRITER * writer);

XDMFCORE_EXPORT void XdmfHeavyDataWriterSetAllowSetSplitting(XDMFHEAVYDATAWRITER * writer, int newAllow);

XDMFCORE_EXPORT void XdmfHeavyDataWriterSetFileIndex(XDMFHEAVYDATAWRITER * writer, int newIndex);

XDMFCORE_EXPORT void XdmfHeavyDataWriterSetFileSizeLimit(XDMFHEAVYDATAWRITER * writer, int newSize);

XDMFCORE_EXPORT void XdmfHeavyDataWriterSetMode(XDMFHEAVYDATAWRITER * writer, int mode, int * status);

XDMFCORE_EXPORT void XdmfHeavyDataWriterSetReleaseData(XDMFHEAVYDATAWRITER * writer, int releaseData);

#define XDMF_HEAVYWRITER_C_CHILD_DECLARE(ClassName, CClassName, Level)                        \
                                                                                              \
Level##_EXPORT void ClassName##Free( CClassName * item);                                      \
Level##_EXPORT int ClassName##GetAllowSetSplitting( CClassName * writer);                     \
Level##_EXPORT int ClassName##GetFileIndex( CClassName * writer);                             \
Level##_EXPORT unsigned int ClassName##GetFileOverhead( CClassName * writer);                 \
Level##_EXPORT char * ClassName##GetFilePath( CClassName * writer);                           \
Level##_EXPORT int ClassName##GetFileSizeLimit( CClassName * writer);                         \
Level##_EXPORT int ClassName##GetMode( CClassName * writer);                                  \
Level##_EXPORT int ClassName##GetReleaseData( CClassName * writer);                           \
Level##_EXPORT void ClassName##SetAllowSetSplitting( CClassName * writer, int newAllow);      \
Level##_EXPORT void ClassName##SetFileIndex( CClassName * writer, int newIndex);              \
Level##_EXPORT void ClassName##SetFileSizeLimit( CClassName * writer, int newSize);           \
Level##_EXPORT void ClassName##SetMode( CClassName * writer, int mode, int * status);         \
Level##_EXPORT void ClassName##SetReleaseData( CClassName * writer, int releaseData);



#define XDMF_HEAVYWRITER_C_CHILD_WRAPPER(ClassName, CClassName)                               \
                                                                                              \
void  ClassName##Free( CClassName * item)                                                     \
{                                                                                             \
  XdmfHeavyDataWriterFree((XDMFHEAVYDATAWRITER *)((void *)item));                             \
}                                                                                             \
                                                                                              \
int ClassName##GetAllowSetSplitting( CClassName * writer)                                     \
{                                                                                             \
  return XdmfHeavyDataWriterGetAllowSetSplitting((XDMFHEAVYDATAWRITER *)((void *)writer));    \
}                                                                                             \
                                                                                              \
int ClassName##GetFileIndex( CClassName * writer)                                             \
{                                                                                             \
  return XdmfHeavyDataWriterGetFileIndex((XDMFHEAVYDATAWRITER *)((void *)writer));            \
}                                                                                             \
                                                                                              \
unsigned int ClassName##GetFileOverhead( CClassName * writer)                                 \
{                                                                                             \
  return XdmfHeavyDataWriterGetFileOverhead((XDMFHEAVYDATAWRITER *)((void *)writer));         \
}                                                                                             \
                                                                                              \
char * ClassName##GetFilePath( CClassName * writer)                                           \
{                                                                                             \
  return XdmfHeavyDataWriterGetFilePath((XDMFHEAVYDATAWRITER *)((void *)writer));             \
}                                                                                             \
                                                                                              \
int ClassName##GetFileSizeLimit( CClassName * writer)                                         \
{                                                                                             \
  return XdmfHeavyDataWriterGetFileSizeLimit((XDMFHEAVYDATAWRITER *)((void *)writer));        \
}                                                                                             \
                                                                                              \
int ClassName##GetMode( CClassName * writer)                                                  \
{                                                                                             \
  return XdmfHeavyDataWriterGetMode((XDMFHEAVYDATAWRITER *)((void *)writer));                 \
}                                                                                             \
                                                                                              \
int ClassName##GetReleaseData( CClassName * writer)                                           \
{                                                                                             \
  return XdmfHeavyDataWriterGetReleaseData((XDMFHEAVYDATAWRITER *)((void *)writer));          \
}                                                                                             \
                                                                                              \
void ClassName##SetAllowSetSplitting( CClassName * writer, int newAllow)                      \
{                                                                                             \
  XdmfHeavyDataWriterSetAllowSetSplitting((XDMFHEAVYDATAWRITER *)((void *)writer), newAllow); \
}                                                                                             \
                                                                                              \
void ClassName##SetFileIndex( CClassName * writer, int newIndex)                              \
{                                                                                             \
  XdmfHeavyDataWriterSetFileIndex((XDMFHEAVYDATAWRITER *)((void *)writer), newIndex);         \
}                                                                                             \
                                                                                              \
void ClassName##SetFileSizeLimit( CClassName * writer, int newSize)                           \
{                                                                                             \
  XdmfHeavyDataWriterSetFileSizeLimit((XDMFHEAVYDATAWRITER *)((void *)writer), newSize);      \
}                                                                                             \
                                                                                              \
void ClassName##SetMode( CClassName * writer, int mode, int * status)                         \
{                                                                                             \
  XdmfHeavyDataWriterSetMode((XDMFHEAVYDATAWRITER *)((void *)writer), mode, status);          \
}                                                                                             \
                                                                                              \
void ClassName##SetReleaseData( CClassName * writer, int releaseData)                         \
{                                                                                             \
  XdmfHeavyDataWriterSetReleaseData((XDMFHEAVYDATAWRITER *)((void *)writer), releaseData);    \
}

#ifdef __cplusplus
}
#endif

#endif /* XDMFHEAVYDATAWRITER_HPP_ */
