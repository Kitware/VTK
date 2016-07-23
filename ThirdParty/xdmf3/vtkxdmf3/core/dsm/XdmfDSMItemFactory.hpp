/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfDSMItemFactory.hpp                                              */
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

#ifndef XDMFDSMITEMFACTORY_HPP_
#define XDMFDSMITEMFACTORY_HPP_

// C Compatible Includes
#include "Xdmf.hpp"
#include "XdmfCoreItemFactory.hpp"
#include "XdmfDSMBuffer.hpp"

#ifdef __cplusplus

// Forward Declarations
class XdmfItem;

// Includes

/**
 * @brief Factory for constructing XdmfItems from their ItemTag and
 * ItemProperties.
 */
class XDMFDSM_EXPORT XdmfDSMItemFactory : public XdmfCoreItemFactory {

public:

  /**
   * Create a new XdmfItemFactory.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfItemFactory.cpp
   * @skipline //#initialization
   * @until //#initialization
   *
   * Python
   *
   * @dontinclude XdmfExampleItemFactory.py
   * @skipline #//initialization
   * @until #//initialization
   *
   * @return    Constructed XdmfItemFactory.
   */
  static shared_ptr<XdmfDSMItemFactory> New();

  virtual ~XdmfDSMItemFactory();

  virtual shared_ptr<XdmfItem>
  createItem(const std::string & itemTag,
             const std::map<std::string, std::string> & itemProperties,
             const std::vector<shared_ptr<XdmfItem> > & childItems) const;

  virtual std::vector<shared_ptr<XdmfHeavyDataController> >
  generateHeavyDataControllers(const std::map<std::string, std::string> & itemProperties,
                               const std::vector<unsigned int> & passedDimensions,
                               shared_ptr<const XdmfArrayType> passedArrayType,
                               const std::string & passedFormat) const;


  virtual shared_ptr<XdmfHeavyDataWriter>
  generateHeavyDataWriter(std::string typeName, std::string path) const;

  XdmfDSMBuffer *
  getDSMBuffer();

  virtual bool isArrayTag(char * tag) const;

  void
  setDSMBuffer(XdmfDSMBuffer * newBuffer);

  virtual XdmfItem *
  DuplicatePointer(shared_ptr<XdmfItem> original) const;

protected:

  XdmfDSMItemFactory();

private:

  XdmfDSMItemFactory(const XdmfDSMItemFactory &);  // Not implemented.
  void operator=(const XdmfDSMItemFactory &);  // Not implemented.

  XdmfDSMBuffer * mDSMBuffer;
};

#ifdef _WIN32
XDMF_TEMPLATE template class XDMF_EXPORT shared_ptr<XdmfDSMItemFactory>;
#endif

#endif

#endif /* XDMFDSMITEMFACTORY_HPP_ */
