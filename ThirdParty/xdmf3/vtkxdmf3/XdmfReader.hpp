/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfReader.hpp                                                      */
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

#ifndef XDMFREADER_HPP_
#define XDMFREADER_HPP_

// Includes
#include "Xdmf.hpp"
#include "XdmfCoreReader.hpp"

/**
 * @brief Reads an Xdmf file stored on disk into memory.
 *
 * Reads an Xdmf file stored on disk into an Xdmf structure in
 * memory. All light data is parsed in order to create appropriate
 * Xdmf objects. Heavy data controllers are created and attached to
 * XdmfArrays but no heavy data is read into memory.
 */
class XDMF_EXPORT XdmfReader : public XdmfCoreReader {

public:

  /**
   * Create a new XdmfReader.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfReader.cpp
   * @skipline //#initialization
   * @until //#initialization
   *
   * Python
   *
   * @dontinclude XdmfExampleReader.py
   * @skipline #//initialization
   * @until #//initialization
   *
   * @return    Constructed XdmfReader.
   */
  static shared_ptr<XdmfReader> New();

  virtual ~XdmfReader();

  shared_ptr<XdmfItem> read(const std::string & filePath) const;

  std::vector<shared_ptr<XdmfItem> >
  read(const std::string & filePath,
       const std::string & xPath) const;

protected:

  XdmfReader();

private:

  XdmfReader(const XdmfReader &);  // Not implemented.
  void operator=(const XdmfReader &);  // Not implemented.
};

#endif /* XDMFREADER_HPP_ */
