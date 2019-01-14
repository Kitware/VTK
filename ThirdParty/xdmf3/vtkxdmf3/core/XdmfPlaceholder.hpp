/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfPlaceholder.hpp                                                 */
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

#ifndef XDMFPLACEHOLDER_HPP_
#define XDMFPLACEHOLDER_HPP_

// C Compatible Includes
#include "XdmfCore.hpp"
#include "XdmfHeavyDataController.hpp"

#ifdef __cplusplus

#include <map>

/**
 * @brief Couples an XdmfArray with an array structure.
 *
 * Takes the place of a heavy data set. Allows an array to define
 * its structure without having to have an associated dataset.
 */
class XDMFCORE_EXPORT XdmfPlaceholder : public XdmfHeavyDataController {

public:

  virtual ~XdmfPlaceholder();

  /**
   * Create a new placeholder to define array structure.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfPlaceholder.cpp
   * @skipline //#initialization
   * @until //#initialization
   *
   * Python
   *
   * @dontinclude XdmfExamplePlaceholder.py
   * @skipline #//initialization
   * @until #//initialization
   *
   * @param     filePath                The location of the file the data set resides in.
   * @param     type                    The data type of the dataset to read.
   * @param     start                   The offset of the starting element in each
   *                                    dimension in the data set.
   * @param     stride                  The number of elements to move in each
   *                                    dimension from the data set.
   * @param     dimensions              The number of elements to select in each
   *                                    dimension from the data set.
   *                                    (size in each dimension)
   * @param     dataspaceDimensions     The number of elements in the entire
   *                                    data set (may be larger than
   *                                    dimensions if using hyperslabs).
   *
   * @return    New Placeholder.
   */
  static shared_ptr<XdmfPlaceholder>
  New(const std::string & FilePath,
      const shared_ptr<const XdmfArrayType> type,
      const std::vector<unsigned int> & start,
      const std::vector<unsigned int> & stride,
      const std::vector<unsigned int> & dimensions,
      const std::vector<unsigned int> & dataspaceDimensions);

  virtual std::string getDescriptor() const;

  virtual std::string getName() const;

  virtual void
  getProperties(std::map<std::string, std::string> & collectedProperties) const;

  virtual void read(XdmfArray * const array);

  XdmfPlaceholder(const XdmfPlaceholder &);

protected:

  XdmfPlaceholder(const std::string & filePath,
                  const shared_ptr<const XdmfArrayType> type,
                  const std::vector<unsigned int> & start,
                  const std::vector<unsigned int> & stride,
                  const std::vector<unsigned int> & dimensions,
                  const std::vector<unsigned int> & dataspaceDimensions);

  shared_ptr<XdmfHeavyDataController>
  createSubController(const std::vector<unsigned int> & starts,
                      const std::vector<unsigned int> & strides,
                      const std::vector<unsigned int> & dimensions);

  virtual shared_ptr<XdmfHeavyDataDescription>
  getHeavyDataDescription();

  void read(XdmfArray * const array, const int fapl);

private:

  void operator=(const XdmfPlaceholder &);  // Not implemented.
};

#endif

#ifdef __cplusplus
extern "C" {
#endif

struct XDMFPLACEHOLDER; // Simply as a typedef to ensure correct typing
typedef struct XDMFPLACEHOLDER XDMFPLACEHOLDER;

XDMFCORE_EXPORT XDMFPLACEHOLDER * XdmfPlaceholderNew(char * hdf5FilePath,
                                                     int type,
                                                     unsigned int * start,
                                                     unsigned int * stride,
                                                     unsigned int * dimensions,
                                                     unsigned int * dataspaceDimensions,
                                                     unsigned int numDims,
                                                     int * status);

// C Wrappers for parent classes are generated by macros

/*
XDMFCORE_EXPORT unsigned int * XdmfPlaceholderGetDataspaceDimensions(XDMFPLACEHOLDER * controller);

XDMFCORE_EXPORT unsigned int * XdmfPlaceholderGetStart(XDMFPLACEHOLDER * controller);

XDMFCORE_EXPORT unsigned int * XdmfPlaceholderGetStride(XDMFPLACEHOLDER * controller); 
*/

XDMF_HEAVYCONTROLLER_C_CHILD_DECLARE(XdmfPlaceholder, XDMFPLACEHOLDER, XDMFCORE)

#ifdef __cplusplus
}
#endif

#endif /* XDMFPLACEHOLDER_HPP_ */
