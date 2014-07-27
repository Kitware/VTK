/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfHeavyDataController.cpp                                         */
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

#include <functional>
#include <numeric>
#include "XdmfArrayType.hpp"
#include "XdmfError.hpp"
#include "XdmfHeavyDataController.hpp"
#include "XdmfSystemUtils.hpp"

XdmfHeavyDataController::XdmfHeavyDataController(const std::string & filePath,
                                                 const shared_ptr<const XdmfArrayType> & type,
                                                 const std::vector<unsigned int> & dimensions) :
  mDimensions(dimensions),
  mFilePath(filePath),
  mArrayStartOffset(0),
  mType(type)
{
}

XdmfHeavyDataController::~XdmfHeavyDataController()
{
}

unsigned int
XdmfHeavyDataController::getArrayOffset() const
{
  return mArrayStartOffset;
}

std::vector<unsigned int> 
XdmfHeavyDataController::getDimensions() const
{
  return mDimensions;
}

std::string
XdmfHeavyDataController::getFilePath() const
{
  return mFilePath;
}

unsigned int
XdmfHeavyDataController::getSize() const
{
  return std::accumulate(mDimensions.begin(),
                         mDimensions.end(),
                         1,
                         std::multiplies<unsigned int>());
}

shared_ptr<const XdmfArrayType>
XdmfHeavyDataController::getType() const
{
  return mType;
}

void
XdmfHeavyDataController::setArrayOffset(unsigned int newOffset)
{
  mArrayStartOffset = newOffset;
}
