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
/*     Copyright @ 2002 US Army Research Laboratory                */
/*     All Rights Reserved                                         */
/*     See Copyright.txt or http://www.arl.hpc.mil/ice for details */
/*                                                                 */
/*     This software is distributed WITHOUT ANY WARRANTY; without  */
/*     even the implied warranty of MERCHANTABILITY or FITNESS     */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice   */
/*     for more information.                                       */
/*                                                                 */
/*******************************************************************/
#ifndef __XdmfLightData_h
#define __XdmfLightData_h

#include "XdmfObject.h"

#ifndef SWIG
#include <string.h> // strcmp, strlen, strcpy
#endif

// typedef XdmfPointer XdmfXmlNode;
struct _xmlNode;
typedef _xmlNode *XdmfXmlNode;
struct _xmlDoc;
typedef _xmlDoc *XdmfXmlDoc;

//! Base object for Light Data (XML)
/*!
This is an abstract convenience object for reading and writing
LightData Files. LightData "points" to HeavyData ; the giga-terabytes of HPC simulations.

A XdmfLightData Object is not used by itself. Rather one of the derived
classes like XdmfGrid or XdmfFormatMulti is used and these derived methods
are used from that class.
*/

class XDMF_EXPORT XdmfLightData : public XdmfObject {

public:
  XdmfLightData();
  ~XdmfLightData();

  XdmfConstString GetClassName() { return ( "XdmfLightData" ) ; };

//! Set the current name
    XdmfSetStringMacro(Name);
//! Get the current name
    XdmfGetValueMacro(Name, XdmfConstString);

//! Set the current filename
    XdmfSetStringMacro(FileName);
//! Get the current filename
    XdmfGetValueMacro(FileName, XdmfConstString);

/*! Set the current WorkingDirectory
        This alleviates the need to hard code pathnames in the
        light data. i.e. the heavy and light data can be in 
        one directory and accessed from another.
*/
    XdmfSetStringMacro(WorkingDirectory);
//! Get the current WorkingDirectory
    XdmfGetValueMacro(WorkingDirectory, XdmfConstString);

//! Has Object been properly initialized
    XdmfGetValueMacro(Initialized, XdmfInt32);
    XdmfSetValueMacro(Initialized, XdmfInt32);

/*! To avoid memory leaks, string return values from methods point to a static buffer.
    If the calling function wishes to retain this value, it must be copied.
    This method copies the value into the static return buffer, allocating space if necessary.
        \param ReturnValue A zero terminated string
*/

/*! Get the char * to the string portion of the return buffer.
*/
    XdmfConstString GetReturnBuffer() {return(this->StaticReturnBuffer);}; 


protected:
    XdmfConstString DupChars(XdmfPointer Chars);   
    XdmfConstString DupBuffer(XdmfPointer Buffer);   
    XdmfString      WorkingDirectory;
    XdmfString      FileName;
    XdmfString      Name;
    XdmfString      StaticReturnBuffer;
    XdmfInt32       Initialized;
};

#endif // __XdmfLightData_h
