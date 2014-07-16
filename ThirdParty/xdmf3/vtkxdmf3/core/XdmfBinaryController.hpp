/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfBinaryController.hpp                                              */
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

// Includes
#include "XdmfCore.hpp"
#include "XdmfHeavyDataController.hpp"

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
   * @param filePath the location of the binary file.
   * @param type the data type of the dataset to read.
   * @param endian the endianness of the data.
   * @param seek in bytes to begin reading in file.
   * @param dimensions the number of elements to select in each
   * dimension from the hdf5 data set. (size in each dimension)
   *
   * @return New Binary Controller.
   */
  static shared_ptr<XdmfBinaryController>
  New(const std::string & filePath,
      const shared_ptr<const XdmfArrayType> & type,
      const Endian & endian,
      const unsigned int seek,
      const std::vector<unsigned int> & dimensions);

  virtual std::string getDescriptor() const;

  virtual Endian getEndian() const;

  virtual std::string getName() const;

  virtual void 
  getProperties(std::map<std::string, std::string> & collectedProperties) const;

  virtual unsigned int getSeek() const;

  virtual void read(XdmfArray * const array);

protected:

  XdmfBinaryController(const std::string & filePath,
                       const shared_ptr<const XdmfArrayType> & type,
                       const Endian & endian,
                       const unsigned int seek,
                       const std::vector<unsigned int> & dimensions);

private:

  XdmfBinaryController(const XdmfBinaryController &);  // Not implemented.
  void operator=(const XdmfBinaryController &);  // Not implemented.

  const Endian mEndian;
  const unsigned int mSeek;

};

#endif /* XDMFBinaryCONTROLLER_HPP_ */
