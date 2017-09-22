/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfStringUtils.cpp                                                 */
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

#include <cctype>
#include "XdmfStringUtils.hpp"

XdmfStringUtils::XdmfStringUtils()
{
}

XdmfStringUtils::~XdmfStringUtils()
{
}

std::string
XdmfStringUtils::toUpper(const std::string & string)
{
  std::string returnString;
  std::transform(string.begin(), 
		 string.end(), 
		 std::back_inserter(returnString),
		 [](unsigned char c) {return std::toupper(c); } );
  return returnString;
}
