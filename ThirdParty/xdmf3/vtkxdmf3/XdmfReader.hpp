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

// C Compatible Includes
#include "Xdmf.hpp"
#include "XdmfCoreReader.hpp"

#ifdef __cplusplus

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

  /**
   * Uses the internal item factory to create a copy of the internal pointer
   * of the provided shared pointer. Primarily used for C wrapping.
   *
   * @param     original        The source shared pointer that the pointer will be pulled from.
   * @return                    A duplicate of the object contained in the pointer.
   */
  virtual XdmfItem * DuplicatePointer(shared_ptr<XdmfItem> original) const;

  shared_ptr<XdmfItem> read(const std::string & filePath) const;

  std::vector<shared_ptr<XdmfItem> >
  read(const std::string & filePath,
       const std::string & xPath) const;

  XdmfReader(const XdmfReader &);

protected:

  XdmfReader();

private:

  void operator=(const XdmfReader &);  // Not implemented.
};

#endif

#ifdef __cplusplus
extern "C" {
#endif

// C wrappers go here

struct XDMFREADER; // Simply as a typedef to ensure correct typing
typedef struct XDMFREADER XDMFREADER;

XDMF_EXPORT XDMFREADER * XdmfReaderNew();

XDMF_EXPORT void XdmfReaderFree(XDMFREADER * item);

XDMF_CORE_READER_C_CHILD_DECLARE(XdmfReader, XDMFREADER, XDMF)

#ifdef __cplusplus
}
#endif

#endif /* XDMFREADER_HPP_ */
