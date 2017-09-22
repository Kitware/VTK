/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfWriter.hpp                                                      */
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

#ifndef XDMFWRITER_HPP_
#define XDMFWRITER_HPP_

// C Compatible Includes
#include "XdmfCore.hpp"
#include "XdmfHeavyDataWriter.hpp"
#include "XdmfVisitor.hpp"

#ifdef __cplusplus

// Forward Declarations
class XdmfArray;
class XdmfInformation;
class XdmfHeavyDataWriter;

/**
 * @brief Traverse the Xdmf graph and write light and heavy data
 * stored to disk.
 *
 * XdmfWriter visits each node of an Xdmf graph structure and writes
 * data to disk. Writing begins by calling the accept() operation on
 * any XdmfItem and supplying this writer as the parameter. The
 * XdmfItem as well as all children attached to the XdmfItem are
 * written to disk. Heavy data is written to a heavy data format using
 * an XdmfHeavyDataWriter and light data is written to XML.
 *
 * An infinite loop is possible if an XdmfItem somehow ends up as its
 * own child, either directly or by way of another Xdmf Item.
 *
 * By default, the XdmfWriter writes all heavy data to a single heavy
 * data file specified by the XdmfHeavyDataWriter. If a dataset is
 * encountered that resides in a different heavy data file on disk,
 * the dataset is read from disk and written to the new heavy data
 * file. If this is undesired, the XdmfWriter can be set to
 * DistributedHeavyData mode in which the writer will automatically
 * reference any heavy dataset even if it resides in a different file
 * than the one currently being written to.
 */
class XDMFCORE_EXPORT XdmfWriter : public XdmfVisitor,
                                   public Loki::Visitor<XdmfArray> {

public:

  enum Mode {
    Default,
    DistributedHeavyData
  };

  /**
   * Create a new XdmfWriter to write Xdmf data to disk. This will
   * create its own hdf5 writer based on the xmlFileName. For example,
   * if supplied "output.xmf" the created hdf5 writer would write to
   * file "output.h5".
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfWriter.cpp
   * @skipline //#initialization
   * @until //#initialization
   *
   * Python
   *
   * @dontinclude XdmfExampleWriter.py
   * @skipline #//initialization
   * @until #//initialization
   *
   * @param     xmlFilePath     The path to the xml file to write to.
   *
   * @return                    The new XdmfWriter.
   */
  static shared_ptr<XdmfWriter> New(const std::string & xmlFilePath);

  /**
   * Create a new XdmfWriter to write Xdmf data to disk. This will
   * utilize the passed heavy data writer to write any heavy data to
   * disk.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfWriter.cpp
   * @skipline //#heavyinitialization
   * @until //#heavyinitialization
   *
   * Python
   *
   * @dontinclude XdmfExampleWriter.py
   * @skipline #//heavyinitialization
   * @until #//heavyinitialization
   *
   * @param     xmlFilePath             The path to the xml file to write to.
   * @param     heavyDataWriter         The heavy data writer to use when writing.
   *
   * @return                            The new XdmfWriter.
   */
  static shared_ptr<XdmfWriter> New(const std::string & xmlFilePath, 
                                    const shared_ptr<XdmfHeavyDataWriter> heavyDataWriter);

  /**
   * Create a new XdmfWriter to write Xdmf data to disk. This will
   * write heavy data to disk using the passed heavy data writer and
   * will add xml output to the stream.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfWriter.cpp
   * @skipline #//bufferinitialization
   * @until #//bufferinitialization
   *
   * Python: does not curretnly support this version of New
   *
   * @param     stream                  The output stream to write light data to.
   * @param     heavyDataWriter         The heavy data writer to use when writing.
   *
   * @return                            The new XdmfWriter.
   */
  static shared_ptr<XdmfWriter> New(std::ostream & stream,
                                    const shared_ptr<XdmfHeavyDataWriter> heavyDataWriter);

  virtual ~XdmfWriter();

  /**
   * Get the absolute path to the XML file on disk this writer is
   * writing to.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfWriter.cpp
   * @skipline //#heavyinitialization
   * @until //#heavyinitialization
   * @skipline //#getFilePath
   * @until //#getFilePath
   *
   * Python
   *
   * @dontinclude XdmfExampleWriter.py
   * @skipline //#heavyinitialization
   * @until //#heavyinitialization
   * @skipline //#getFilePath
   * @until //#getFilePath
   *
   * @return    A std::string containing the path to the XML file on disk this
   *            writer is writing to.
   */
  std::string getFilePath() const;

  /**
   * Get the heavy data writer that this XdmfWriter uses to write
   * heavy data to disk.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfWriter.cpp
   * @skipline //#heavyinitialization
   * @until //#heavyinitialization
   * @skipline //#getHeavyDataWriter
   * @until //#getHeavyDataWriter
   *
   * Python
   *
   * @dontinclude XdmfExampleWriter.py
   * @skipline #//heavyinitialization
   * @until #//heavyinitialization
   * @skipline #//getHeavyDataWriter
   * @until #//getHeavyDataWriter
   *
   * @return    The requested heavy data writer.
   */
  shared_ptr<XdmfHeavyDataWriter> getHeavyDataWriter();

  /**
   * Get the heavy data writer that this XdmfWriter uses to write
   * heavy data to disk (const version).
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfWriter.cpp
   * @skipline //#heavyinitialization
   * @until //#heavyinitialization
   * @skipline //#getHeavyDataWriterconst
   * @until //#getHeavyDataWriterconst
   *
   * Python: Does not support a contant version of this function
   *
   * @return    The requested heavy data writer.
   */
  shared_ptr<const XdmfHeavyDataWriter> getHeavyDataWriter() const;

  /**
   * Get the number of values that this writer writes to light data
   * (XML) before switching to a heavy data format.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfWriter.cpp
   * @skipline //#heavyinitialization
   * @until //#heavyinitialization
   * @skipline //#getLightDataLimit
   * @until //#getLightDataLimit
   *
   * Python
   *
   * @dontinclude XdmfExampleWriter.py
   * @skipline #//heavyinitialization
   * @until #//heavyinitialization
   * @skipline #//getLightDataLimit
   * @until #//getLightDataLimit
   *
   * @return    An unsigned int containing the number of values.
   */
  unsigned int getLightDataLimit() const;

  /**
   * Get the Mode of operation for this writer.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfWriter.cpp
   * @skipline //#heavyinitialization
   * @until //#heavyinitialization
   * @skipline //#getMode
   * @until //#getMode
   *
   * Python
   *
   * @dontinclude XdmfExampleWriter.py
   * @skipline #//heavyinitialization
   * @until #//heavyinitialization
   * @skipline #//getMode
   * @until #//getMode
   *
   * @return    The Mode of operation for this writer.
   */
  Mode getMode() const;

  /**
   * Gets whether XML is rebuilt with each write.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfWriter.cpp
   * @skipline //#heavyinitialization
   * @until //#heavyinitialization
   * @skipline //#getRebuildXML
   * @until //#getRebuildXML
   *
   * Python
   *
   * @dontinclude XdmfExampleWriter.py
   * @skipline #//heavyinitialization
   * @until #//heavyinitialization
   * @skipline #//getRebuildXML
   * @until #//getRebuildXML
   *
   * @return    Whether XML will be rebuilt.
   */
  bool getRebuildXML();

  /**
   * Get whether this writer is set to write xpaths.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfWriter.cpp
   * @skipline //#heavyinitialization
   * @until //#heavyinitialization
   * @skipline //#getWriteXPaths
   * @until //#getWriteXPaths
   *
   * Python
   *
   * @dontinclude XdmfExampleWriter.py
   * @skipline #//heavyinitialization
   * @until #//heavyinitialization
   * @skipline #//getWriteXPaths
   * @until #//getWriteXPaths
   *
   * @return    bool whether this writer is set to write xpaths.
   */
  bool getWriteXPaths() const;

  /**
   * Get whether this writer is set to parse xpaths from information.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfWriter.cpp
   * @skipline //#heavyinitialization
   * @until //#heavyinitialization
   * @skipline //#getXPathParse
   * @until //#getXPathParse
   *
   * Python
   *
   * @dontinclude XdmfExampleWriter.py
   * @skipline #//heavyinitialization
   * @until #//heavyinitialization
   * @skipline #//getXPathParse
   * @until #//getXPathParse
   *
   * @return    bool whether this writer is set to write xpaths.
   */
  bool getXPathParse() const;

  /**
   * Set the heavy data writer that this XdmfWriter uses to write
   * heavy data to disk.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfWriter.cpp
   * @skipline //#heavyinitialization
   * @until //#heavyinitialization
   * @skipline //#getHeavyDataWriter
   * @until //#getHeavyDataWriter
   * @skipline //#setHeavyDataWriter
   * @until //#setHeavyDataWriter
   *
   * Python
   *
   * @dontinclude XdmfExampleWriter.py
   * @skipline #//heavyinitialization
   * @until #//heavyinitialization
   * @skipline #//getHeavyDataWriter
   * @until #//getHeavyDataWriter
   * @skipline #//setHeavyDataWriter
   * @until #//setHeavyDataWriter
   *
   * @param     heavyDataWriter         The heavy data writer to set.
   */
  void setHeavyDataWriter(shared_ptr<XdmfHeavyDataWriter> heavyDataWriter);

  /**
   * Set the number of values that this writer writes to light data
   * (XML) before switching to a heavy data format.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfWriter.cpp
   * @skipline //#heavyinitialization
   * @until //#heavyinitialization
   * @skipline //#setLightDataLimit
   * @until //#setLightDataLimit
   *
   * Python
   *
   * @dontinclude XdmfExampleWriter.py
   * @skipline #//heavyinitialization
   * @until #//heavyinitialization
   * @skipline #//setLightDataLimit
   * @until #//setLightDataLimit
   *
   * @param     numValues       An unsigned int containing the number of values.
   */
  void setLightDataLimit(const unsigned int numValues);

  /**
   * Set the mode of operation for this writer.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfWriter.cpp
   * @skipline //#heavyinitialization
   * @until //#heavyinitialization
   * @skipline //#setMode
   * @until //#setMode
   *
   * Python
   *
   * @dontinclude XdmfExampleWriter.py
   * @skipline #//heavyinitialization
   * @until #//heavyinitialization
   * @skipline #//setMode
   * @until #//setMode
   *
   * @param     mode    The Mode of operation for this writer.
   */
  void setMode(const Mode mode);

  /**
   * Sets whether XML will be rebuilt with each write. This
   * functionality is mostly broken, so use at your own peril. It
   * seems to work when additional grids are added to a existing
   * domain during repeated rewrites (such as during
   * timestepping). The XML description of the unmodified grids is
   * correctly saved and reused. However, beware if making changes to
   * old grids as the functionality to mark changes to the grids is
   * not fully implemented.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfWriter.cpp
   * @skipline //#heavyinitialization
   * @until //#heavyinitialization
   * @skipline //#setRebuildXML
   * @until //#setRebuildXML
   *
   * Python
   *
   * @dontinclude XdmfExampleWriter.py
   * @skipline #//heavyinitialization
   * @until #//heavyinitialization
   * @skipline #//setRebuildXML
   * @until #//setRebuildXML
   *
   * @param     newStatus       Whether to rebuild XML.
   */
  void setRebuildXML(bool newStatus);

  /**
   * Set whether to write xpaths for this writer.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfWriter.cpp
   * @skipline //#heavyinitialization
   * @until //#heavyinitialization
   * @skipline //#setWriteXPaths
   * @until //#setWriteXPaths
   *
   * Python
   *
   * @dontinclude XdmfExampleWriter.py
   * @skipline #//heavyinitialization
   * @until #//heavyinitialization
   * @skipline #//setWriteXPaths
   * @until #//setWriteXPaths
   *
   * @param     writeXPaths     Whether to write xpaths for this writer.
   */
  void setWriteXPaths(const bool writeXPaths = true);

  /**
   * Set whether to parse xpaths from infomation for this writer.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfWriter.cpp
   * @skipline //#heavyinitialization
   * @until //#heavyinitialization
   * @skipline //#setXPathParse
   * @until //#setXPathParse
   *
   * Python
   *
   * @dontinclude XdmfExampleWriter.py
   * @skipline #//heavyinitialization
   * @until #//heavyinitialization
   * @skipline #//setXPathParse
   * @until #//setXPathParse
   *
   * @param     xPathParse      Whether to write xpaths for this writer.
   */
  void setXPathParse(const bool xPathParse = true);

  /**
   * Write an XdmfArray to disk
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfWriter.cpp
   * @skipline //#visitarray
   * @until //#visitarray
   *
   * Python
   *
   * @dontinclude XdmfExampleWriter.py
   * @skipline #//visitarray
   * @until #//visitarray
   *
   * @param     array           An XdmfArray to write to disk.
   * @param     visitor         A smart pointer to this visitor --- aids in grid traversal.
   */
  virtual void visit(XdmfArray & array,
                     const shared_ptr<XdmfBaseVisitor> visitor);

  /**
   * Write an XdmfItem to disk
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfWriter.cpp
   * @skipline //#visititem
   * @until //#visititem
   *
   * Python
   *
   * @dontinclude XdmfExampleWriter.py
   * @skipline #//visititem
   * @until #//visititem
   *
   * @param     item            An XdmfItem to write to disk.
   * @param     visitor         A smart pointer to this visitor --- aids in grid traversal.
   */
  virtual void visit(XdmfItem & item,
                     const shared_ptr<XdmfBaseVisitor> visitor);

protected:

  XdmfWriter(const std::string & xmlFilePath,
             shared_ptr<XdmfHeavyDataWriter> heavyDataWriter,
             std::ostream * stream = NULL);

  void setDocumentTitle(std::string title);
  void setVersionString(std::string version);

  bool mRebuildAlreadyVisited;

private:

  /**
   * PIMPL
   */
  class XdmfWriterImpl;

  XdmfWriter(const XdmfWriter &);  // Not implemented.
  void operator=(const XdmfWriter &);  // Not implemented.

  XdmfWriterImpl * mImpl;

};

#endif

#ifdef __cplusplus
extern "C" {
#endif

#define XDMF_WRITER_MODE_DEFAULT                30
#define XDMF_WRITER_MODE_DISTRIBUTED_HEAVY_DATA 31

// C wrappers go here

struct XDMFWRITER; // Simply as a typedef to ensure correct typing
typedef struct XDMFWRITER XDMFWRITER;

XDMFCORE_EXPORT XDMFWRITER * XdmfWriterNew(char * fileName);

XDMFCORE_EXPORT XDMFWRITER * XdmfWriterNewSpecifyHeavyDataWriter(char * fileName, XDMFHEAVYDATAWRITER * heavyDataWriter);

XDMFCORE_EXPORT void XdmfWriterFree(XDMFWRITER * item);

XDMFCORE_EXPORT char * XdmfWriterGetFilePath(XDMFWRITER * writer, int * status);

XDMFCORE_EXPORT XDMFHEAVYDATAWRITER * XdmfWriterGetHeavyDataWriter(XDMFWRITER * writer, int * status);

XDMFCORE_EXPORT unsigned int XdmfWriterGetLightDataLimit(XDMFWRITER * writer, int * status);

XDMFCORE_EXPORT int XdmfWriterGetMode(XDMFWRITER * writer, int * status);

XDMFCORE_EXPORT int XdmfWriterGetWriteXPaths(XDMFWRITER * writer, int * status);

XDMFCORE_EXPORT int XdmfWriterGetXPathParse(XDMFWRITER * writer, int * status);

XDMFCORE_EXPORT void XdmfWriterSetHeavyDataWriter(XDMFWRITER * writer, XDMFHEAVYDATAWRITER * heavyDataWriter, int transferOwnership, int * status);

XDMFCORE_EXPORT void XdmfWriterSetLightDataLimit(XDMFWRITER * writer, unsigned int numValues, int * status);

XDMFCORE_EXPORT void XdmfWriterSetMode(XDMFWRITER * writer, int mode, int * status);

XDMFCORE_EXPORT void XdmfWriterSetWriteXPaths(XDMFWRITER * writer, int writeXPaths, int * status);

XDMFCORE_EXPORT void XdmfWriterSetXPathParse(XDMFWRITER * writer, int xPathParse, int * status);

#ifdef __cplusplus
}
#endif

#endif /* XDMFWRITER_HPP_ */
