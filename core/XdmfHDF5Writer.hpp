/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfHDF5Writer.hpp                                                  */
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

#ifndef XDMFHDF5WRITER_HPP_
#define XDMFHDF5WRITER_HPP_

// C Compatible includes
#include "XdmfCore.hpp"
#include "XdmfHeavyDataWriter.hpp"
#include "XdmfHeavyDataController.hpp"

// So that hdf5 does not need to be included in the header files
// It would add a dependancy to programs that use Xdmf
#ifndef _H5Ipublic_H
  #ifndef XDMF_HID_T
  #define XDMF_HID_T
    typedef int hid_t;
  #endif
#endif

#ifdef __cplusplus

// Forward Declarations
class XdmfArray;
class XdmfArrayType;
class XdmfHDF5Controller;

// Includes
#include <list>
#include <set>

/**
 * @brief Traverse the Xdmf graph and write heavy data stored in
 * XdmfArrays to HDF5 on disk.
 *
 * XdmfHDF5Writer traverses an Xdmf graph structure and writes data
 * stored in XdmfArrays to HDF5. Writing begins by calling the
 * accept() operation on any XdmfItem and supplying this writer as the
 * parameter. The writer will write all XdmfArrays under the XdmfItem
 * to an hdf5 file on disk. It will also attach an XdmfHDF5Controller
 * to all XdmfArrays that it writes to disk.
 *
 * This writer supports all heavy data writing modes listed in
 * XdmfHeavyDataWriter.
 */
class XDMFCORE_EXPORT XdmfHDF5Writer : public XdmfHeavyDataWriter {

public:

  /**
   * Construct XdmfHDF5Writer.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfHDF5Writer.cpp
   * @skipline //#initialization
   * @until //#initialization
   *
   * Python
   *
   * @dontinclude XdmfExampleHDF5Writer.py
   * @skipline #//initialization
   * @until #//initialization
   *
   * @param     filePath        The location of the hdf5 file to output to on disk.
   * @param     clobberFile     Whether to overwrite the previous file if it exists.
   *
   * @return                    New XdmfHDF5Writer.
   */
  static shared_ptr<XdmfHDF5Writer> New(const std::string & filePath,
                                        const bool clobberFile = false);

  virtual ~XdmfHDF5Writer();


  virtual void closeFile();

  /**
   * Get the chunk size used to output datasets to hdf5.
   *
   * C++
   *
   * @dontinclude ExampleXdmfHDF5Writer.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getChunkSize
   * @until //#getChunkSize
   *
   * Python
   *
   * @dontinclude XdmfExampleHDF5Writer.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getChunkSize
   * @until #//getChunkSize
   *
   * @return    Chunk size used to output datasets to hdf5.
   */
  unsigned int getChunkSize() const;

  virtual int getDataSetSize(const std::string & fileName,
                             const std::string & dataSetName);

  /**
   * Gets the factor that Deflate uses to compress data.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfHDF5Writer.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getDeflateFactor
   * @until //#getDeflateFactor
   *
   * Python
   *
   * @dontinclude XdmfExampleHDF5Writer.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getDeflateFactor
   * @until #//getDeflateFactor
   *
   * @return    The factor Deflate uses.
   */
  int getDeflateFactor() const;

  /**
   * Gets whether Deflate is enabled.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfHDF5Writer.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getUseDeflate
   * @until //#getUseDeflate
   *
   * Python
   *
   * @dontinclude XdmfExampleHDF5Writer.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getUseDeflate
   * @until #//getUseDeflate
   *
   * @return    Whether Deflate is in use.
   */
  bool getUseDeflate() const;

  virtual void openFile();

  /**
   * Set the chunk size used to output datasets to hdf5. For
   * multidimensional datasets the chunk size is the total number of
   * elements in the chunk.
   *
   * C++
   *
   * @dontinclude ExampleXdmfHDF5Writer.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#setChunkSize
   * @until //#setChunkSize
   *
   * Python
   *
   * @dontinclude XdmfExampleHDF5Writer.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//setChunkSize
   * @until #//setChunkSize
   *
   * @param     chunkSize       The number of elements per chunk.
   */
  void setChunkSize(const unsigned int chunkSize);

  /**
   * Sets the factor that Deflate will use to compress data.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfHDF5Writer.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#setDeflateFactor
   * @until //#setDeflateFactor
   *
   * Python
   *
   * @dontinclude XdmfExampleHDF5Writer.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//setDeflateFactor
   * @until #//setDeflateFactor
   *
   * @param     factor  The factor Deflate will use.
   */
  void setDeflateFactor(int factor);

  /**
   * Sets whether HDF5 will use Deflate compression
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfHDF5Writer.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#setUseDeflate
   * @until //#setUseDeflate
   *
   * Python
   *
   * @dontinclude XdmfExampleHDF5Writer.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//setUseDeflate
   * @until #//setUseDeflate
   *
   * @param     status  Whether Deflate will be used.
   */
  void setUseDeflate(bool status);

  using XdmfHeavyDataWriter::visit;
  virtual void visit(XdmfArray & array,
                     const shared_ptr<XdmfBaseVisitor> visitor);

  virtual void visit(XdmfItem & item,
                     const shared_ptr<XdmfBaseVisitor> visitor);

  XdmfHDF5Writer(const XdmfHDF5Writer &);

protected:

  XdmfHDF5Writer(const std::string & filePath);

  /**
   * Create a new HDF5 Controller that is able to read in after being
   * written by this writer.
   *
   * @param hdf5FilePath the location of the hdf5 file the data set resides in.
   * @param dataSetPath the location of the dataset within the hdf5 file.
   * @param type the data type of the dataset to read.
   * @param start the offset of the starting element in each dimension in
   * the hdf5 data set.
   * @param stride the number of elements to move in each dimension from the
   * hdf5 data set.
   * @param dimensions the number of elements to select in each
   * dimension from the hdf5 data set. (size in each dimension)
   * @param dataspaceDimensions the number of elements in the entire
   * hdf5 data set (may be larger that dimensions if using
   * hyperslabs).
   *
   * @return    new HDF5 Controller.
   */
  virtual shared_ptr<XdmfHeavyDataController>
  createController(const std::string & hdf5FilePath,
                   const std::string & descriptor,
                   const shared_ptr<const XdmfArrayType> type,
                   const std::vector<unsigned int> & start,
                   const std::vector<unsigned int> & stride,
                   const std::vector<unsigned int> & dimensions,
                   const std::vector<unsigned int> & dataspaceDimensions);

  virtual int getDataSetSize(shared_ptr<XdmfHeavyDataController> descriptionController);

  /**
   * Write the XdmfArray to a hdf5 file.
   *
   * @param     array   An XdmfArray to write to hdf5.
   */
  virtual void write(XdmfArray & array);

  /**
   * PIMPL
   */
  class XdmfHDF5WriterImpl
  {
  public:

    XdmfHDF5WriterImpl();

    virtual ~XdmfHDF5WriterImpl();

    virtual void
    closeFile();

    virtual int
    openFile(const std::string & filePath,
             const int mDataSetId);

    hid_t mHDF5Handle;
    int mFapl;
    unsigned int mChunkSize;
    std::string mOpenFile;
    int mDepth;
    std::set<const XdmfItem *> mWrittenItems;
  };

  XdmfHDF5WriterImpl * mImpl;

  bool mUseDeflate;
  int mDeflateFactor;

private:

  void operator=(const XdmfHDF5Writer &);  // Not implemented.

  virtual void controllerSplitting(XdmfArray & array,
                                   int & controllerIndexOffset,
                                   shared_ptr<XdmfHeavyDataController> heavyDataController,
                                   const std::string & checkFileName,
                                   const std::string & checkFileExt,
                                   const std::string & dataSetPath,
                                   int dataSetId,
                                   const std::vector<unsigned int> & dimensions,
                                   const std::vector<unsigned int> & dataspaceDimensions,
                                   const std::vector<unsigned int> & start,
                                   const std::vector<unsigned int> & stride,
                                   std::list<std::string> & filesWritten,
                                   std::list<std::string> & datasetsWritten,
                                   std::list<int> & datasetIdsWritten,
                                   std::list<void *> & arraysWritten,
                                   std::list<std::vector<unsigned int> > & startsWritten,
                                   std::list<std::vector<unsigned int> > & stridesWritten,
                                   std::list<std::vector<unsigned int> > & dimensionsWritten,
                                   std::list<std::vector<unsigned int> > & dataSizesWritten,
                                   std::list<unsigned int> & arrayOffsetsWritten);

};

#endif

#ifdef __cplusplus
extern "C" {
#endif

// C wrappers go here

struct XDMFHDF5WRITER; // Simply as a typedef to ensure correct typing
typedef struct XDMFHDF5WRITER XDMFHDF5WRITER;

XDMFCORE_EXPORT XDMFHDF5WRITER * XdmfHDF5WriterNew(char * fileName, int clobberFile);

XDMFCORE_EXPORT void XdmfHDF5WriterCloseFile(XDMFHDF5WRITER * writer, int * status);

XDMFCORE_EXPORT unsigned int XdmfHDF5WriterGetChunkSize(XDMFHDF5WRITER * writer, int * status);

XDMFCORE_EXPORT void XdmfHDF5WriterOpenFile(XDMFHDF5WRITER * writer, int * status);

XDMFCORE_EXPORT void XdmfHDF5WriterSetChunkSize(XDMFHDF5WRITER * writer, unsigned int chunkSize, int * status);

#define XDMF_HDF5WRITER_C_CHILD_DECLARE(ClassName, CClassName, Level)                                    \
                                                                                                         \
Level##_EXPORT void ClassName##CloseFile( CClassName * writer, int * status);                            \
Level##_EXPORT unsigned int ClassName##GetChunkSize( CClassName * writer, int * status);                 \
Level##_EXPORT void ClassName##OpenFile( CClassName * writer, int * status);                             \
Level##_EXPORT void ClassName##SetChunkSize( CClassName * writer, unsigned int chunkSize, int * status);

#define XDMF_HDF5WRITER_C_CHILD_WRAPPER(ClassName, CClassName)                                           \
                                                                                                         \
void ClassName##CloseFile( CClassName * writer, int * status)                                            \
{                                                                                                        \
  XdmfHDF5WriterCloseFile((XDMFHDF5WRITER *)((void *)writer), status);                                   \
}                                                                                                        \
                                                                                                         \
unsigned int ClassName##GetChunkSize( CClassName * writer, int * status)                                 \
{                                                                                                        \
  return XdmfHDF5WriterGetChunkSize((XDMFHDF5WRITER *)((void *)writer), status);                         \
}                                                                                                        \
                                                                                                         \
void ClassName##OpenFile( CClassName * writer, int * status)                                             \
{                                                                                                        \
  XdmfHDF5WriterOpenFile((XDMFHDF5WRITER *)((void *)writer), status);                                    \
}                                                                                                        \
                                                                                                         \
void ClassName##SetChunkSize( CClassName * writer, unsigned int chunkSize, int * status)                 \
{                                                                                                        \
  XdmfHDF5WriterSetChunkSize((XDMFHDF5WRITER *)((void *)writer), chunkSize, status);                     \
}


// C Wrappers for parent classes are generated by macros

XDMF_HEAVYWRITER_C_CHILD_DECLARE(XdmfHDF5Writer, XDMFHDF5WRITER, XDMFCORE)

#ifdef __cplusplus
}
#endif

#endif /* XDMFHDF5WRITER_HPP_ */
