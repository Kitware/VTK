/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfReader.cpp                                                      */
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

#include "XdmfItemFactory.hpp"
#include "XdmfReader.hpp"
#include "XdmfError.hpp"

shared_ptr<XdmfReader>
XdmfReader::New()
{
  shared_ptr<XdmfReader> p(new XdmfReader());
  return p;
}

XdmfReader::XdmfReader() :
  XdmfCoreReader(XdmfItemFactory::New())
{
}

XdmfReader::XdmfReader(const XdmfReader &) :
  XdmfCoreReader(XdmfItemFactory::New())
{
}

XdmfReader::~XdmfReader()
{
}

XdmfItem *
XdmfReader::DuplicatePointer(shared_ptr<XdmfItem> original) const
{
  return XdmfCoreReader::DuplicatePointer(original);
}

// Implemented to make SWIG wrapping work correctly
// (typemaps to return specific subclass instances of XdmfItems)
shared_ptr<XdmfItem>
XdmfReader::read(const std::string & filePath) const
{
  return XdmfCoreReader::read(filePath);
}

std::vector<shared_ptr<XdmfItem> >
XdmfReader::read(const std::string & filePath,
                 const std::string & xPath) const
{
  return XdmfCoreReader::read(filePath, xPath);
}

// C Wrappers

XDMFREADER * XdmfReaderNew()
{
  shared_ptr<XdmfReader> returnReader = XdmfReader::New();
  return (XDMFREADER *)((void *)(new XdmfReader(*returnReader.get())));
}

void XdmfReaderFree(XDMFREADER * item)
{
  delete ((XdmfReader *)item);
}

XDMF_CORE_READER_C_CHILD_WRAPPER(XdmfReader, XDMFREADER)
