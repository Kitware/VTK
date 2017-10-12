/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfBinaryController.hpp                                            */
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

#ifndef XDMFBinaryCONTROLLER_HPP_
#define XDMFBinaryCONTROLLER_HPP_

// C Compatible Includes
#include "XdmfCore.hpp"
#include "XdmfHeavyDataController.hpp"

#ifdef __cplusplus

/**
 * @brief Couples an XdmfArray with Binary data stored on disk.
 *
 * Serves as an interface between data stored in XdmfArrays and data
 * stored in binary files on disk. When an Xdmf file is read from or
 * written to disk an XdmfBinaryController is attached to
 * XdmfArrays. This allows data to be released from memory but still
 * be accessible or have its location written to light data.
 */
class XDMFCORE_EXPORT XdmfBinaryController : public XdmfHeavyDataController {

public:

  typedef enum Endian {
    BIG,
    LITTLE,
    NATIVE
  } Endian;

  virtual ~XdmfBinaryController();

  /**
   * Create a new controller for an binary data set on disk.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfBinaryController.cpp
   * @skipline //#initializationsimplified
   * @until //#initializationsimplified
   *
   * Python
   *
   * @dontinclude XdmfExampleBinaryController.py
   * @skipline #//initializationsimplified
   * @until #//initializationsimplified
   *
   * @param     filePath        Location of the binary file.
   * @param     type            Data type of the dataset to read.
   * @param     endian          Endianness of the data.
   * @param     seek            Distance in bytes to begin reading in file.
   * @param     dimensions      Number of elements to select in each from the total
   *
   * @return New Binary Controller.
   */
  static shared_ptr<XdmfBinaryController>
  New(const std::string & filePath,
      const shared_ptr<const XdmfArrayType> & type,
      const Endian & endian,
      const unsigned int seek,
      const std::vector<unsigned int> & dimensions);

  /**
   * Create a new controller for an binary data set on disk.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfBinaryController.cpp
   * @skipline //#initialization
   * @until //#initialization
   *
   * Python
   *
   * @dontinclude XdmfExampleBinaryController.py
   * @skipline #//initialization
   * @until #//initialization
   *
   * @param     filePath        Location of the binary file.
   * @param     type            Data type of the dataset to read.
   * @param     endian          Endianness of the data.
   * @param     seek            Distance in bytes to begin reading in file.
   * @param     starts          Starting index for each dimension
   * @param     strides         Distance between read values across the dataspace
   * @param     dimensions      Number of elements to select in each from the total
   * @param     dataspaces      Total number of elements to select in each
   *
   * @return New Binary Controller.
   */
  static shared_ptr<XdmfBinaryController>
  New(const std::string & filePath,
      const shared_ptr<const XdmfArrayType> & type,
      const Endian & endian,
      const unsigned int seek,
      const std::vector<unsigned int> & starts,
      const std::vector<unsigned int> & strides,
      const std::vector<unsigned int> & dimensions,
      const std::vector<unsigned int> & dataspaces);

  virtual std::string getDataspaceDescription() const;

  /**
   * Gets the endianness of the dataset that the controller points to.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfBinaryController.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getEndian
   * @until //#getEndian
   *
   * Python
   *
   * @dontinclude XdmfExampleBinaryController.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getEndian
   * @until #//getEndian
   *
   * @return    The endianness of the dataset.
   */
  virtual Endian getEndian() const;

  virtual std::string getName() const;

  virtual void 
  getProperties(std::map<std::string, std::string> & collectedProperties) const;

  /**
   * Gets the offset in bytes of the dataset that the controller points to in the file.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfBinaryController.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getSeek
   * @until //#getSeek
   *
   * Python
   *
   * @dontinclude XdmfExampleBinaryController.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getSeek
   * @until #//getSeek
   *
   * @return    The offset in bytes.
   */
  virtual unsigned int getSeek() const;

  virtual void read(XdmfArray * const array);

  XdmfBinaryController(const XdmfBinaryController &);

protected:

  XdmfBinaryController(const std::string & filePath,
                       const shared_ptr<const XdmfArrayType> & type,
                       const Endian & endian,
                       const unsigned int seek,
                       const std::vector<unsigned int> & starts,
                       const std::vector<unsigned int> & strides,
                       const std::vector<unsigned int> & dimensions,
                       const std::vector<unsigned int> & dataspaces);

private:

  void operator=(const XdmfBinaryController &);  // Not implemented.

  const Endian mEndian;
  const unsigned int mSeek;

};

#endif

#ifdef __cplusplus
extern "C" {
#endif

#define XDMF_BINARY_CONTROLLER_ENDIAN_BIG    50
#define XDMF_BINARY_CONTROLLER_ENDIAN_LITTLE 51
#define XDMF_BINARY_CONTROLLER_ENDIAN_NATIVE 52

struct XDMFBINARYCONTROLLER; // Simply as a typedef to ensure correct typing
typedef struct XDMFBINARYCONTROLLER XDMFBINARYCONTROLLER;

XDMFCORE_EXPORT XDMFBINARYCONTROLLER * XdmfBinaryControllerNew(char * filePath,
                                                               int type,
                                                               int endian,
                                                               unsigned int seek,
                                                               unsigned int * dimensions,
                                                               unsigned int numDims,
                                                               int * status);

XDMFCORE_EXPORT XDMFBINARYCONTROLLER * XdmfBinaryControllerNewHyperslab(char * filePath,
                                                                        int type,
                                                                        int endian,
                                                                        unsigned int seek,
                                                                        unsigned int * starts,
                                                                        unsigned int * strides,
                                                                        unsigned int * dimensions,
                                                                        unsigned int * dataspaces,
                                                                        unsigned int numDims,
                                                                        int * status);

XDMFCORE_EXPORT int XdmfBinaryControllerGetEndian(XDMFBINARYCONTROLLER * controller);

XDMFCORE_EXPORT unsigned int XdmfBinaryControllerGetSeek(XDMFBINARYCONTROLLER * controller);

XDMF_HEAVYCONTROLLER_C_CHILD_DECLARE(XdmfBinaryController, XDMFBINARYCONTROLLER, XDMFCORE)

#ifdef __cplusplus
}
#endif

#endif /* XDMFBinaryCONTROLLER_HPP_ */
