/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfAggregate.hpp                                                   */
/*                                                                           */
/*  Author:                                                                  */
/*     Andrew Burns                                                          */
/*     andrew.j.burns2@us.army.mil                                           */
/*     US Army Research Laboratory                                           */
/*     Aberdeen Proving Ground, MD                                           */
/*                                                                           */
/*     Copyright @ 2013 US Army Research Laboratory                          */
/*     All Rights Reserved                                                   */
/*     See Copyright.txt for details                                         */
/*                                                                           */
/*     This software is distributed WITHOUT ANY WARRANTY; without            */
/*     even the implied warranty of MERCHANTABILITY or FITNESS               */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice             */
/*     for more information.                                                 */
/*                                                                           */
/*****************************************************************************/

#ifndef XDMFAGGREGATE_HPP_
#define XDMFAGGREGATE_HPP_

// C Compatible Includes
#include "Xdmf.hpp"
#include "XdmfArrayReference.hpp"

#ifdef __cplusplus

// Forward Declarations
class XdmfArray;

// Includes
#include <vector>
#include "XdmfItem.hpp"
#include "XdmfSharedPtr.hpp"

/**
 * @brief Couples an XdmfArray with heavy data stored in another XdmfArray.
 *
 * This class serves to allow an array to retrieve data that is a subsection
 * of an already existing array.
 */
class XDMF_EXPORT XdmfAggregate: public XdmfArrayReference {

public:

  /**
   * Generates an XdmfAggregate object.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfAggregate.cpp
   * @skipline //#initialization
   * @until //#initialization
   *
   * Python
   *
   * @dontinclude XdmfExampleAggregate.py
   * @skipline #//initialization
   * @until #//initialization
   *
   * @return    A constructed XdmfAggregate object.
   */
  static shared_ptr<XdmfAggregate>
  New();

  virtual ~XdmfAggregate();

  LOKI_DEFINE_VISITABLE(XdmfAggregate, XdmfItem)
  XDMF_CHILDREN(XdmfAggregate, XdmfArray, Array, Name)

  static const std::string ItemTag;

  /**
   * Get the dimensions of the set referenced by this subset.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfAggregate.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getDimensions
   * @until //#getDimensions
   *
   * Python
   *
   * @dontinclude XdmfExampleAggregate.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getDimensions
   * @until #//getDimensions
   *
   * @return    A vector containing the size in each dimension of the
   *            set referenced by this subset.
   */
  std::vector<unsigned int> getDimensions() const;

  std::map<std::string, std::string> getItemProperties() const;

  std::string getItemTag() const;

  /**
   * Get the size of the set referenced by this subset.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfAggregate.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getSize
   * @until //#getSize
   *
   * Python
   *
   * @dontinclude XdmfExampleAggregate.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getSize
   * @until #//getSize
   *
   * @return    An int containing the size of the subset.
   */
  unsigned int getSize() const;

  /**
   * Read data reference by this subset and return as an XdmfArray.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfAggregate.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#read
   * @until //#read
   *
   * Python
   *
   * @dontinclude XdmfExampleAggregate.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//read
   * @until #//read
   *
   * @return    An array filled with data based on the subset's parameters.
   */
  virtual shared_ptr<XdmfArray> read() const;

  void traverse(const shared_ptr<XdmfBaseVisitor> visitor);

protected:

  XdmfAggregate();

  void
  populateItem(const std::map<std::string, std::string> & itemProperties,
               const std::vector<shared_ptr<XdmfItem> > & childItems,
               const XdmfCoreReader * const reader);

private:

  XdmfAggregate(const XdmfAggregate&);  // Not implemented.
  void operator=(const XdmfAggregate&);  // Not implemented.

};

#endif

#ifdef __cplusplus
extern "C" {
#endif

// C wrappers go here

struct XDMFAGGREGATE; // Simply as a typedef to ensure correct typing
typedef struct XDMFAGGREGATE XDMFAGGREGATE;

XDMF_EXPORT XDMFAGGREGATE * XdmfAggregateNew();

XDMF_EXPORT XDMFARRAY * XdmfAggregateGetArray(XDMFAGGREGATE * aggregate, unsigned int index);

XDMF_EXPORT XDMFARRAY * XdmfAggregateGetArrayByName(XDMFAGGREGATE * aggregate, char * name);

XDMF_EXPORT unsigned int XdmfAggregateGetNumberArrays(XDMFAGGREGATE * aggregate);

XDMF_EXPORT void XdmfAggregateInsertArray(XDMFAGGREGATE * aggregate, XDMFARRAY * array, int transferOwnership);

XDMF_EXPORT void XdmfAggregateRemoveArray(XDMFAGGREGATE * aggregate, unsigned int index);

XDMF_EXPORT void XdmfAggregateRemoveArrayByName(XDMFAGGREGATE * aggregate, char * name);

// C Wrappers for parent classes are generated by macros

XDMF_ITEM_C_CHILD_DECLARE(XdmfAggregate, XDMFAGGREGATE, XDMF)
XDMF_ARRAYREFERENCE_C_CHILD_DECLARE(XdmfAggregate, XDMFAGGREGATE, XDMF)

#ifdef __cplusplus
}
#endif

#endif /* XDMFAGGREGATE_HPP_ */
