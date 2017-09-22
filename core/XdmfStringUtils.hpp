/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfStringUtils.hpp                                                 */
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

#ifndef XDMFSTRINGUTILS_HPP_
#define XDMFSTRINGUTILS_HPP_

#include "XdmfCore.hpp"

#ifdef __cplusplus

#include <algorithm>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

/**
 * @brief String parsing utilities
 *
 * Collects all string parsing utilities used by Xdmf
 */
class XDMFCORE_EXPORT XdmfStringUtils {

 public:

  /**
   * Split a string by whitespace and insert into container.
   *
   * @param stringToSplit a string to split by spaces.
   * @param containerToInsert a container to insert the split string into.
   */
  template <typename T>
  static void split(const std::string & stringToSplit,
		    std::vector<T> & containerToInsert);

  /**
   * Capitalize a string.
   *
   * @param string to capitalize
   *
   * @return capitalized string
   */
  static std::string toUpper(const std::string & string);

 protected:

  XdmfStringUtils();
  ~XdmfStringUtils();

 private:

  XdmfStringUtils(const XdmfStringUtils &);  // Not implemented.
  void operator=(const XdmfStringUtils &);  // Not implemented.

};

template <typename T>
void 
XdmfStringUtils::split(const std::string & stringToSplit,
		       std::vector<T> & containerToInsert)
{
  std::istringstream stream(stringToSplit);
  std::copy(std::istream_iterator<T>(stream),
	    std::istream_iterator<T>(),
	    std::back_inserter(containerToInsert));
}

#endif

#endif /* XDMFSTRINGUTILS_HPP_ */
