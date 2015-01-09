/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfSubset.hpp                                                      */
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

#ifndef XDMFSUBSET_HPP_
#define XDMFSUBSET_HPP_

// Forward Declarations
class XdmfArray;

// Includes
#include <vector>
#include "XdmfCore.hpp"
#include "XdmfItem.hpp"
#include "XdmfArrayReference.hpp"
#include "XdmfSharedPtr.hpp"

/**
 * @brief Couples an XdmfArray with heavy data stored in another XdmfArray.
 *
 * This class serves to allow an array to retrieve data that is a subsection
 * of an already existing array.
 */
class XDMFCORE_EXPORT XdmfSubset: public XdmfArrayReference {

public:

  /**
   * Generates an XdmfSubset object based on the parameters provided
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfSubset.cpp
   * @skipline //#initialization
   * @until //#initialization 
   *
   * Python
   *
   * @dontinclude XdmfExampleSubset.py
   * @skipline #//initialization
   * @until #//initialization
   *
   * @param     referenceArray  The array that the subset is generated from.
   * @param     start           A vector of the starting points for each
   *                            dimension.
   * @param     stride          A vector of the strides for each dimension.
   * @param     dimensions      A vector of the amount of values read from each
   *                            dimension.
   * @return                    A constructed XdmfSubset
   */
  static shared_ptr<XdmfSubset>
  New(shared_ptr<XdmfArray> referenceArray,
      std::vector<unsigned int> & start,
      std::vector<unsigned int> & stride,
      std::vector<unsigned int> & dimensions);

  virtual ~XdmfSubset();

  LOKI_DEFINE_VISITABLE(XdmfSubset, XdmfItem)
  static const std::string ItemTag;

  /**
   * Get the dimensions of the set referenced by this subset.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfSubset.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getDimensions
   * @until //#getDimensions
   *
   * Python
   *
   * @dontinclude XdmfExampleSubset.py
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
   * Gets the array that the subset pulls data from.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfSubset.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getReferenceArray
   * @until //#getReferenceArray
   *
   * Python
   *
   * @dontinclude XdmfExampleSubset.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getReferenceArray
   * @until #//getReferenceArray
   *
   * @return    The array that the subset pulls data from
   */
  shared_ptr<XdmfArray> getReferenceArray();

  /**
   * Get the size of the set referenced by this subset.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfSubset.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getSize
   * @until //#getSize
   *
   * Python
   *
   * @dontinclude XdmfExampleSubset.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getSize
   * @until #//getSize
   *
   * @return    An int containing the size of the subset.
   */
  unsigned int getSize() const;

  /**
   * Get the start index of the set referenced by this subset.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfSubset.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getStart
   * @until //#getStart
   *
   * Python
   *
   * @dontinclude XdmfExampleSubset.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getStart
   * @until #//getStart
   *
   * @return    A vector containing the start index in each dimension of
   *            the set referenced by this subset.
   */
  std::vector<unsigned int> getStart() const;

  /**
   * Get the stride of the set referenced by this subset.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfSubset.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getStride
   * @until //#getStride
   *
   * Python
   *
   * @dontinclude XdmfExampleSubset.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getStride
   * @until #//getStride
   *
   * @return    A vector containing the stride in each dimension of the
   *            heavy data set owned by this controller.
   */
  std::vector<unsigned int> getStride() const;

  /**
   * Read data reference by this subset and return as an XdmfArray.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfSubset.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#read
   * @until //#read
   *
   * Python
   *
   * @dontinclude XdmfExampleSubset.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//read
   * @until #//read
   *
   * @return    An array filled with data based on the subset's parameters.
   */
  virtual shared_ptr<XdmfArray> read() const;

  /**
   * Set the dimensions of the set referenced by this subset.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfSubset.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getDimensions
   * @until //#getDimensions
   * @skipline //#setDimensions
   * @until //#setDimensions
   *
   * Python
   *
   * @dontinclude XdmfExampleSubset.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getDimensions
   * @until #//getDimensions
   * @skipline #//setDimensions
   * @until #//setDimensions
   *
   * @param     newDimensions   A vector containing the size in each dimension
   *                            of the set to be referenced by this subset.
   */
  void setDimensions(std::vector<unsigned int> newDimensions);

  /**
   * Set the Array that the subset is generated from.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfSubset.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getReferenceArray
   * @until //#getReferenceArray
   * @skipline //#setReferenceArray
   * @until //#setReferenceArray
   *
   * Python
   *
   * @dontinclude XdmfExampleSubset.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getReferenceArray
   * @until #//getReferenceArray
   * @skipline #//setReferenceArray
   * @until #//setReferenceArray
   *
   * @param     newReference    A shared pointer to the array that the subset
   *                            will be generated from
   */
  void setReferenceArray(shared_ptr<XdmfArray> newReference);

  /**
   * Set the start index of the set referenced by this subset.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfSubset.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getStart
   * @until //#getStart
   * @skipline //#setStart
   * @until //#setStart
   *
   * Python
   *
   * @dontinclude XdmfExampleSubset.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getStart
   * @until #//getStart
   * @skipline #//setStart
   * @until #//setStart
   *
   * @param     newStarts       A vector containing the start index in each
   *                            dimension of the set to be referenced by this
   *                            subset.
   */
  void setStart(std::vector<unsigned int> newStarts);

  /**
   * Set the stride of the heavy data set owned by this controller.
   *
   * Example of use:
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfSubset.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getStride
   * @until //#getStride
   * @skipline //#setStride
   * @until //#setStride
   *
   * Python
   *
   * @dontinclude XdmfExampleSubset.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getStride
   * @until #//getStride
   * @skipline #//setStride
   * @until #//setStride
   *
   * @param     newStrides      A vector containing the stride in each
   *                            dimension of the set referenced by this subset.
   */
  void setStride(std::vector<unsigned int> newStrides);

  void traverse(const shared_ptr<XdmfBaseVisitor> visitor);

protected:

  XdmfSubset(shared_ptr<XdmfArray> referenceArray,
             std::vector<unsigned int> & start,
             std::vector<unsigned int> & stride,
             std::vector<unsigned int> & dimensions);

  void
  populateItem(const std::map<std::string, std::string> & itemProperties,
               const std::vector<shared_ptr<XdmfItem> > & childItems,
               const XdmfCoreReader * const reader);

  shared_ptr<XdmfArray> mParent;
  std::vector<unsigned int> mDimensions;
  std::vector<unsigned int> mStart;
  std::vector<unsigned int> mStride;

private:

  XdmfSubset(const XdmfSubset&);  // Not implemented.
  void operator=(const XdmfSubset &);  // Not implemented.

};

#endif /* XDMFSUBSET_HPP_ */
