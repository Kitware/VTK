/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Format              */
/*                                                                 */
/*  Id : Id */ 
/*  Date : $Date$ */
/*  Version : $Revision$  */
/*                                                                 */
/*  Author:Kenji Takizawa (Team for Advanced Flow Simulation and Modeling) */
/*                                                                 */
/*     Copyright @ 2008 US Army Research Laboratory                */
/*     All Rights Reserved                                         */
/*     See Copyright.txt or http://www.arl.hpc.mil/ice for details */
/*                                                                 */
/*     This software is distributed WITHOUT ANY WARRANTY; without  */
/*     even the implied warranty of MERCHANTABILITY or FITNESS     */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice   */
/*     for more information.                                       */
/*                                                                 */
/*******************************************************************/
#ifndef __XdmfValuesBinaryL_h
#define __XdmfValuesBinaryL_h


#include "XdmfValues.h"

//!  Parent Class for handeling I/O of actual data for an XdmfDataItem
/*!
This is the class for access of values from a raw binary.


A Binary XdmfDataItem Node Looks like :

\verbatim
<DataItem
  Dimensions="3 3 3"
  Precision="4"
  DataType="Float"
  Format="Binary"
  Seek="2"
  Compression="BZip2"
  Endian="Little"
  >
</DataItem>
\endverbatim
Endian: Little, Big, otherwise Native
Seak: unit is byte
Putting "<" in the CDATA, it may cause an error in the XML parser. 
Here's an example of using "<" in the CDATA :

\verbatim
<![CDATA[SELECT X FROM Locations WHERE Time < 0.21]]>
\endverbatim

That is you have ti start the CDATA with "<![CDATA[" and end
it with "]]>".
*/


class XDMF_EXPORT XdmfValuesBinary : public XdmfValues {

public :

  XdmfValuesBinary();
  virtual ~XdmfValuesBinary();

  XdmfConstString GetClassName() { return("XdmfValuesBinary"); } ;
  //! Read the Array from the External Representation
  XdmfArray *Read(XdmfArray *Array=NULL);
  //! Write the Array to the External Representation
  XdmfInt32 Write(XdmfArray *Array, XdmfConstString HeavyDataSetName=NULL);

  //! Get Endian Name
  XdmfGetStringMacro(Endian);
  //! Set Endian Name
  XdmfSetStringMacro(Endian);

  //! Get Seek
  XdmfGetStringMacro(Seek);
  //! Set Seek
  XdmfSetStringMacro(Seek);

  //! Get Compression
  XdmfGetStringMacro(Compression);
  //! Set Compression
  XdmfSetStringMacro(Compression);


protected :
  bool needByteSwap();
  XdmfString  Endian;
  XdmfString  Seek;
  XdmfString  Compression;
private:
  void byteSwap(XdmfArray * RetArray);
  size_t getSeek();
  enum CompressionType {Raw, Zlib, BZip2};
  enum CompressionType getCompressionType();
};

#endif
