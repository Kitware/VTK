/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Format              */
/*                                                                 */
/*  Id : Id  */
/*  Date : $Date$ */
/*  Version : $Revision$ */
/*                                                                 */
/*  Author:                                                        */
/*     Jerry A. Clarke                                             */
/*     clarke@arl.army.mil                                         */
/*     US Army Research Laboratory                                 */
/*     Aberdeen Proving Ground, MD                                 */
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
#ifndef __XdmfValuesMySQL_h
#define __XdmfValuesMySQL_h


#include "XdmfValues.h"

//!  Parent Class for handeling I/O of actual data for an XdmfDataItem
/*!
This is the class for access of values from a MySQL Database. By default, the
In this format (SQL) the CDATA of the DataItem is an SQL Query into a MySQL Database.


A MySQL XdmfDataItem Node Looks like :

\verbatim
<DataItem
  Rank="1"
  Dimensions="300"
  Precision="4"
  DataType="Float"
  Format="MySQL"
  DataBase="MnmiDB"
  User="Xdmf"
  Server="localhost">
  SELECT * FROM Locations WHERE Time > 0.11
</DataItem>
\endverbatim

Putting "<" in the CDATA, it may cause an error in the XML parser. 
Here's an example of using "<" in the CDATA :

\verbatim
<![CDATA[SELECT X FROM Locations WHERE Time < 0.21]]>
\endverbatim

That is you have ti start the CDATA with "<![CDATA[" and end
it with "]]>".
*/


class XDMF_EXPORT XdmfValuesMySQL : public XdmfValues {

public :

  XdmfValuesMySQL();
  virtual ~XdmfValuesMySQL();

  XdmfConstString GetClassName() { return("XdmfValuesMySQL"); } ;
  //! Read the Array from the External Representation
  XdmfArray *Read(XdmfArray *Array=NULL);
  //! Write the Array to the External Representation
  XdmfInt32 Write(XdmfArray *Array, XdmfConstString HeavyDataSetName=NULL);

  //! Get the Hostname of the MySQL Server
  XdmfGetStringMacro(Server);
  //! Get the Hostname of the MySQL Server
  XdmfSetStringMacro(Server);

  //! Get the User Name
  XdmfGetStringMacro(User);
  //! Get the User Name
  XdmfSetStringMacro(User);

  //! Get the Password
  XdmfGetStringMacro(Password);
  //! Get the Password
  XdmfSetStringMacro(Password);

  //! Get the DataBase Name
  XdmfGetStringMacro(DataBase);
  //! Get the DataBase Name
  XdmfSetStringMacro(DataBase);

  //! Get the Query
  XdmfGetStringMacro(Query);
  //! Get the Query
  XdmfSetStringMacro(Query);

protected :
    XdmfString  Server;
    XdmfString  User;
    XdmfString  Password;
    XdmfString  DataBase;
    XdmfString  Query;
};

#endif
