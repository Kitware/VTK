/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfTIFFController.hpp                                              */
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

#ifndef XDMFTIFFCONTROLLER_HPP_
#define XDMFTIFFCONTROLLER_HPP_

// C Compatible Includes
#include "XdmfCore.hpp"
#include "XdmfHeavyDataController.hpp"

#ifdef __cplusplus

/**
 * @brief Couples an XdmfArray with TIFF data stored on disk.
 *
 * Serves as an interface between data stored in XdmfArrays and data
 * stored in tiff files on disk. When an Xdmf file is read from or
 * written to disk an XdmfTIFFController is attached to
 * XdmfArrays. This allows data to be released from memory but still
 * be accessible or have its location written to light data.
 */
class XDMFCORE_EXPORT XdmfTIFFController : public XdmfHeavyDataController {

public:

  virtual ~XdmfTIFFController();

  /**
   * Create a new controller for an TIFF file on disk.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfTIFFController.cpp
   * @skipline //#initializationsimplified
   * @until //#initializationsimplified
   *
   * Python
   *
   * @dontinclude XdmfExampleTIFFController.py
   * @skipline #//initializationsimplified
   * @until #//initializationsimplified
   *
   * @param     filePath                The location of the tiff file the data
   *                                    set resides in.
   * @param     type                    The data type of the dataset to read.
   * @param     dimensions              The number of elements to select in each
   *                                    dimension from the data set.
   *                                    (size in each dimension)
   *
   * @return    New TIFF Controller.
   */
  static shared_ptr<XdmfTIFFController>
  New(const std::string & filePath,
      const shared_ptr<const XdmfArrayType> & type,
      const std::vector<unsigned int> & dimensions);

  /**
   * Create a new controller for an TIFF file on disk.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfTIFFController.cpp
   * @skipline //#initialization
   * @until //#initialization
   *
   * Python
   *
   * @dontinclude XdmfExampleTIFFController.py
   * @skipline #//initialization
   * @until #//initialization
   *
   * @param     filePath                The location of the tiff file the data set resides in.
   * @param     type                    The data type of the dataset to read.
   * @param     starts                  The offset of the starting element in each
   *                                    dimension in the data set.
   * @param     strides                 The number of elements to move in each
   *                                    dimension from the data set.
   * @param     dimensions              The number of elements to select in each
   *                                    dimension from the data set.
   *                                    (size in each dimension)
   * @param     dataspaces              The number of elements in the entire
   *                                    data set (may be larger than
   *                                    dimensions if using hyperslabs).
   *
   * @return    New TIFF Controller.
   */
  static shared_ptr<XdmfTIFFController>
  New(const std::string & filePath,
      const shared_ptr<const XdmfArrayType> & type,
      const std::vector<unsigned int> & starts,
      const std::vector<unsigned int> & strides,
      const std::vector<unsigned int> & dimensions,
      const std::vector<unsigned int> & dataspaces);

  virtual std::string getName() const;

  virtual void 
  getProperties(std::map<std::string, std::string> & collectedProperties) const;

  virtual void read(XdmfArray * const array);

  XdmfTIFFController(const XdmfTIFFController &);

protected:

  XdmfTIFFController(const std::string & filePath,
                     const shared_ptr<const XdmfArrayType> & type,
                     const std::vector<unsigned int> & starts,
                     const std::vector<unsigned int> & strides,
                     const std::vector<unsigned int> & dimensions,
                     const std::vector<unsigned int> & dataspaces);

  virtual shared_ptr<XdmfHeavyDataController>
  createSubController(const std::vector<unsigned int> & starts,
                      const std::vector<unsigned int> & strides,
                      const std::vector<unsigned int> & dimensions);

  unsigned int getNumberDirectories() const;

  void readToArray(XdmfArray * const array,
                   void * pointer,
                   unsigned int offset,
                   unsigned int start,
                   unsigned int stride,
                   unsigned int amount,
                   shared_ptr<const XdmfArrayType> type);

private:

  void operator=(const XdmfTIFFController &);  // Not implemented.
};

#endif

#ifdef __cplusplus
extern "C" {
#endif

struct XDMFTIFFCONTROLLER; // Simply as a typedef to ensure correct typing
typedef struct XDMFTIFFCONTROLLER XDMFTIFFCONTROLLER;

XDMFCORE_EXPORT XDMFTIFFCONTROLLER * XdmfTIFFControllerNew(char * filePath,
                                                           int type,
                                                           unsigned int * dimensions,
                                                           unsigned int numDims,
                                                           int * status);

XDMFCORE_EXPORT XDMFTIFFCONTROLLER * XdmfTIFFControllerNewHyperslab(char * filePath,
                                                                    int type,
                                                                    unsigned int * starts,
                                                                    unsigned int * strides,
                                                                    unsigned int * dimensions,
                                                                    unsigned int * dataspaces,
                                                                    unsigned int numDims,
                                                                    int * status);

XDMF_HEAVYCONTROLLER_C_CHILD_DECLARE(XdmfTIFFController, XDMFTIFFCONTROLLER, XDMFCORE)

#ifdef __cplusplus
}
#endif

#endif /* XDMFTIFFCONTROLLER_HPP_ */
