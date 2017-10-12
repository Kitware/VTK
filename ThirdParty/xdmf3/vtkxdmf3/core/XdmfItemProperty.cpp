/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfItemProperty.cpp                                                */
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

#include "XdmfItemProperty.hpp"
#include <boost/assign.hpp>

const std::map<const char, const char> XdmfItemProperty::UpperConversionMap =
        boost::assign::map_list_of ('a', 'A')
                                   ('b', 'B')
                                   ('c', 'C')
                                   ('d', 'D')
                                   ('e', 'E')
                                   ('f', 'F')
                                   ('g', 'G')
                                   ('h', 'H')
                                   ('i', 'I')
                                   ('j', 'J')
                                   ('k', 'K')
                                   ('l', 'L')
                                   ('m', 'M')
                                   ('n', 'N')
                                   ('o', 'O')
                                   ('p', 'P')
                                   ('q', 'Q')
                                   ('r', 'R')
                                   ('s', 'S')
                                   ('t', 'T')
                                   ('u', 'U')
                                   ('v', 'V')
                                   ('w', 'W')
                                   ('x', 'X')
                                   ('y', 'Y')
                                   ('z', 'Z');

// Using this method because ANSI and std transform aren't guarenteed
std::string
XdmfItemProperty::ConvertToUpper(const std::string & converted)
{
  std::string returnstring;
  returnstring.resize(converted.size());
  std::map<const char, const char>::const_iterator characterConversion;
  for (unsigned int i = 0; i < converted.size(); ++i)
  {
    characterConversion = UpperConversionMap.find(converted[i]);
    if (characterConversion != UpperConversionMap.end())
    {
      returnstring[i] = characterConversion->second;
    }
    else
    {
      returnstring[i] = converted[i];
    }
  }
  return returnstring;
}

XdmfItemProperty::XdmfItemProperty()
{
}

XdmfItemProperty::~XdmfItemProperty()
{
}
