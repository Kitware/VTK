/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfCoreReader.hpp                                                  */
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

#ifndef XDMFCOREREADER_HPP_
#define XDMFCOREREADER_HPP_

// Forward Declarations
class XdmfCoreItemFactory;
class XdmfItem;

// Includes
#include <string>
#include <vector>
#include "XdmfCore.hpp"
#include "XdmfSharedPtr.hpp"

/**
 * @brief Reads an Xdmf file stored on disk into memory.
 *
 * Reads an Xdmf file stored on disk into an Xdmf structure in memory.
 * All light data is parsed in order to create appropriate Xdmf
 * objects.  Heavy data controllers are created and attached to
 * XdmfArrays but no heavy data is read into memory.
 *
 * XdmfCoreReader is an abstract base class.
 */
class XDMFCORE_EXPORT XdmfCoreReader {

public:

  virtual ~XdmfCoreReader() = 0;

  /**
   * Parse a string containing light data into an Xdmf structure in
   * memory.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfCoreReader.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#parse
   * @until //#parse
   *
   * Python
   *
   * @dontinclude XdmfExampleCoreReader.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//parse
   * @until #//parse
   *
   * @param     lightData       A string containing light data description of an
   *                            Xdmf file.
   *
   * @return                    An XdmfItem at the root of the Xdmf tree.
   */  
  virtual shared_ptr<XdmfItem> parse(const std::string & lightData) const;

  /**
   * Read an Xdmf file from disk into memory.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfCoreReader.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#readpath
   * @until //#readpath
   * @skipline //#readroot
   * @until //#readroot
   *
   * Python
   *
   * @dontinclude XdmfExampleCoreReader.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//readpath
   * @until #//readpath
   * @skipline #//readroot
   * @until #//readroot
   *
   * @param     filePath        The path of the Xdmf file to read in from disk.
   *
   * @return                    An XdmfItem at the root of the Xdmf tree.
   */
  virtual shared_ptr<XdmfItem> read(const std::string & filePath) const;

  /**
   * Read part of an Xdmf file from disk into memory.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfCoreReader.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#readpath
   * @until //#readpath
   * @skipline //#readXPath
   * @until //#readXPath
   *
   * Python
   *
   * @dontinclude XdmfExampleCoreReader.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//readpath
   * @until #//readpath
   * @skipline #//readXPath
   * @until #//readXPath
   *
   * @param     filePath        The path of the Xdmf file to read in from disk.
   * @param     xPath           An XPath corresponding to the portion of the
   *                            file to read.
   *
   * @return                    A vector of XdmfItems that are included
   *                            in the XPath.
   */
  virtual std::vector<shared_ptr<XdmfItem> >
  read(const std::string & filePath,
       const std::string & xPath) const;

  /**
   * Read an Xdmf file from disk into memory.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfCoreReader.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#readpath
   * @until //#readpath
   * @skipline //#readItems
   * @until //#readItems
   *
   * Python
   *
   * @dontinclude XdmfExampleCoreReader.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//readpath
   * @until #//readpath
   * @skipline #//readItems
   * @until #//readItems
   *
   * @param     filePath        The path of the Xdmf file to read in from disk.
   *
   * @return                    A vector of XdmfItems at the root of the Xdmf tree.
   */
  virtual std::vector<shared_ptr<XdmfItem> >
  readItems(const std::string & filePath) const;

  /**
   * Used by the other functions to read items from an open file.
   *
   * Since files are closed between reads, this does nothing by itself.
   *
   * @param     xPath   An XPath corresponding to the portion of the file to read.
   *
   * @return            A vector of items at the X path provided.
   */
  std::vector<shared_ptr<XdmfItem> >
  readPathObjects(const std::string & xPath) const;

protected:

  /**
   * Constructor
   *
   * @param itemFactory an XdmfCoreItemFactory to construct XdmfItems
   * for a specific language.
   */
  XdmfCoreReader(const shared_ptr<const XdmfCoreItemFactory> itemFactory);

private:

  /**
   * PIMPL
   */
  class XdmfCoreReaderImpl;

  XdmfCoreReader(const XdmfCoreReader &);  // Not implemented.
  void operator=(const XdmfCoreReader &);  // Not implemented.

  XdmfCoreReaderImpl * const mImpl;

};

#ifdef _WIN32
XDMFCORE_TEMPLATE template class XDMFCORE_EXPORT
std::allocator<shared_ptr<XdmfItem> >;
XDMFCORE_TEMPLATE template class XDMFCORE_EXPORT
std::vector<shared_ptr<XdmfItem>,
            std::allocator<shared_ptr<XdmfItem> > >;
#endif

#endif /* XDMFCOREREADER_HPP_ */
