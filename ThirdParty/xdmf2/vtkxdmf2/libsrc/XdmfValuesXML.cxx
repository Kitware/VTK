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
#include "XdmfValuesXML.h"
#include "XdmfDataStructure.h"
#include "XdmfArray.h"
#include "XdmfHDF.h"

namespace xdmf2
{

XdmfValuesXML::XdmfValuesXML() {
    this->SetFormat(XDMF_FORMAT_XML);
}

XdmfValuesXML::~XdmfValuesXML() {
}

XdmfArray *
XdmfValuesXML::Read(XdmfArray *anArray){
    XdmfArray   *RetArray = anArray;

    if(!this->DataDesc){
        XdmfErrorMessage("DataDesc has not been set");
        return(NULL);
    }
    // Allocate anArray if Necessary
    if(!RetArray){
        RetArray = new XdmfArray();
        RetArray->CopyType(this->DataDesc);
        RetArray->CopyShape(this->DataDesc);
        // RetArray->CopySelection(this->DataDesc);
    }
    XdmfDebug("Accessing XML CDATA");
    if(RetArray->SetValues(0, this->Get("CDATA")) != XDMF_SUCCESS){
        XdmfErrorMessage("Error Accessing Actual Data Values");
        if(!anArray) delete RetArray;
        RetArray = NULL;
    }
    if(this->DataDesc->GetSelectionSize() != RetArray->GetNumberOfElements() ){
        // Only Want Portion of anArray
        XdmfArray *SrcArray;
        XdmfInt64  SelectionSize = this->DataDesc->GetSelectionSize();

        XdmfDebug("Selecting " << SelectionSize << " elements of XML CDATA");
        SrcArray = RetArray->Clone();
        RetArray->SetShape(1, &SelectionSize);
        RetArray->SelectAll();
        SrcArray->CopySelection(this->DataDesc);
        XdmfDebug("Original Values = " << SrcArray->GetValues());
        CopyArray(SrcArray, RetArray);
        XdmfDebug("New Values = " << RetArray->GetValues());
        delete SrcArray;
    }
    return(RetArray);
}

XdmfInt32
XdmfValuesXML::Write(XdmfArray *anArray, XdmfConstString /*HeavyDataSetName*/){

    XdmfConstString DataValues;
    ostrstream   StringOutput;
    XdmfInt32   rank, r;
    XdmfInt64   i, index, nelements, len, idims[XDMF_MAX_DIMENSION], dims[XDMF_MAX_DIMENSION];

    if(!this->DataDesc ){
        XdmfErrorMessage("DataDesc has not been set");
        return(XDMF_FAIL);
    }
    if(!anArray){
        XdmfErrorMessage("Array to Write is NULL");
        return(XDMF_FAIL);
    }
    rank = this->DataDesc->GetShape(dims);
    for(i = 0 ; i < rank ; i++){
        idims[i] = dims[i];
    }
    // At most 10 values per line
    len = MIN(dims[rank - 1], 10);
    nelements = this->DataDesc->GetNumberOfElements();
    index = 0;
    StringOutput << endl;
    while(nelements){
        r = rank - 1;
        len = MIN(len, nelements);
        DataValues = anArray->GetValues(index, len);
        StringOutput << DataValues << endl;
        index += len;
        nelements -= len;
        dims[r] -= len;
        // End of Smallest dimension ?
        if(nelements && r && (dims[r] <= 0)){
            // Reset
            dims[r] = idims[r];
            // Go Backwards thru dimensions
            while(r){
                r--;
                dims[r]--;
                // Is dim now 0
                if(dims[r] <= 0){
                    // Add an Endl and keep going
                    StringOutput << endl;
                    dims[r] = idims[r];
                }else{
                    // Still some left
                    break;
                }
            }
        }
    }
    StringOutput << ends;
    XdmfString toReturn = StringOutput.str();
    StringOutput.rdbuf()->freeze(0);
    return(this->Set("CDATA", toReturn));
}

}
