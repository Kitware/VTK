/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfSystemUtils.hpp                                                 */
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

#ifndef XDMFSYSTEMUTILS_HPP_
#define XDMFSYSTEMUTILS_HPP_

// Includes
#include <string>
#include "XdmfCore.hpp"

/**
 * @brief System specific functions.
 *
 * Collects all system specific functions needed by Xdmf.
 */
class XDMFCORE_EXPORT XdmfSystemUtils {

 public:

  /**
   * Converts a filesystem path to an absolute real path (absolute
   * path with no symlinks)
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfSystemUtils.cpp
   * @skipline //#getRealPath
   * @until //#getRealPath
   *
   * Python
   *
   * @dontinclude XdmfExampleSystemUtils.py
   * @skipline #//getRealPath
   * @until #//getRealPath
   *
   * @param path a string containing the path to convert.
   *
   * @return the equivalent real path.
   */
  static std::string getRealPath(const std::string & path);

 protected:

  XdmfSystemUtils();
  ~XdmfSystemUtils();

 private:

  XdmfSystemUtils(const XdmfSystemUtils &);  // Not implemented.
  void operator=(const XdmfSystemUtils &);  // Not implemented.

};

#endif /* XDMFSYSTEMUTILS_HPP_ */
