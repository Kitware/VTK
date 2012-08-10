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
#include "XdmfLightData.h"
#include <libxml/tree.h>

XdmfLightData::XdmfLightData() {

    this->WorkingDirectory = NULL; 
    this->FileName = NULL; 
    this->Name = NULL; 
    this->StaticReturnBuffer = NULL;
    // Defaults
    this->SetFileName( "XdmfData.xmf" );
    this->SetWorkingDirectory(".");
}

XdmfLightData::~XdmfLightData() {
    if(this->StaticReturnBuffer){
        delete [] this->StaticReturnBuffer;
    }
    if(this->WorkingDirectory){
        delete [] this->WorkingDirectory;
    }
    if(this->Name){
        delete [] this->Name;
    }
    if(this->FileName){
        delete [] this->FileName;
    }
}

// Copies xmlChar * to static area then frees chars
XdmfConstString XdmfLightData::DupChars(XdmfPointer Chars){
    xmlChar *cp;

    cp = (xmlChar *)Chars;
    if(!cp) return(NULL);
    if(this->StaticReturnBuffer) delete [] this->StaticReturnBuffer;
    this->StaticReturnBuffer = new char[xmlStrlen(cp) + 1];
    strcpy(this->StaticReturnBuffer, (char *)cp);
    xmlFree(cp);
    return(this->StaticReturnBuffer);
}

// Copies bufp->content to static area then frees buffer
XdmfConstString XdmfLightData::DupBuffer(XdmfPointer Buffer){
    xmlBuffer   *bufp;

    bufp = (xmlBuffer *)Buffer;
    if(!bufp) return(NULL);
    if(this->StaticReturnBuffer) delete [] this->StaticReturnBuffer;
    this->StaticReturnBuffer = new char[xmlBufferLength(bufp) + 1];
    strcpy(this->StaticReturnBuffer, (char *)xmlBufferContent(bufp));
    xmlBufferFree(bufp);
    return(this->StaticReturnBuffer);
}

