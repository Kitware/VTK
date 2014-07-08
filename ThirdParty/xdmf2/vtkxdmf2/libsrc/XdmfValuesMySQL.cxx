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
#include "XdmfValuesMySQL.h"
#include "XdmfDataStructure.h"
#include "XdmfArray.h"
#include "XdmfHDF.h"

#include "mysql.h"

namespace xdmf2
{

XdmfValuesMySQL::XdmfValuesMySQL() {
    this->Password = 0;
    this->DataBase = 0;
    this->Query = 0;
    this->Server = 0;
    this->User = 0;
    this->SetFormat(XDMF_FORMAT_MYSQL);
    this->SetServer("localhost");
    this->SetUser("root");
}

XdmfValuesMySQL::~XdmfValuesMySQL() {
}

XdmfArray *
XdmfValuesMySQL::Read(XdmfArray *anArray){
    XdmfArray       *RetArray = anArray;
    XdmfInt64       ResultLength, Fields, Rows, Index, NValues;
    XdmfInt32       FieldIndex;
    XdmfConstString Value;
    MYSQL           *Connection;
    MYSQL_RES       *Result;
    MYSQL_FIELD     *Field;
    MYSQL_ROW       Row;


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
    XdmfDebug("Accessing MySQL CDATA");
    Value = this->Get("Query");
    if(Value){
        this->SetQuery(Value);
    }else{
        this->SetQuery(this->Get("CDATA"));
    }
    Value = this->Get("Server");
    if(Value){
        this->SetServer(Value);
    }else{
        this->SetServer("localhost");
    }
    Value = this->Get("User");
    if(Value){
        this->SetUser(Value);
    }else{
        this->SetUser("root");
    }
    Value = this->Get("Password");
    if(Value){
        this->SetPassword(Value);
    }else{
        this->Password = NULL;
    }
    Value = this->Get("DataBase");
    if(Value){
        this->SetDataBase(Value);
    }else{
        this->SetDataBase("Xdmf");
    }
    if(!(Connection = mysql_init(NULL))){
        XdmfErrorMessage("Cannot Initialize MySQL");
        return(NULL);
    }
    if(!mysql_real_connect(Connection, this->Server, this->User,
            this->Password, this->DataBase, 0, NULL, 0)){
        XdmfErrorMessage("Error Making MySQL Connection : " << mysql_error(Connection));
        mysql_close(Connection);
        return(NULL);
    }
    if(mysql_query(Connection, this->Query)){
        XdmfErrorMessage("Using Query : " << this->Query);
        XdmfErrorMessage("Error Making MySQL Query : " << mysql_error(Connection));
        mysql_close(Connection);
        return(NULL);
    }
    Result = mysql_use_result(Connection);
    // Result = mysql_store_result(Connection);
    Fields = mysql_num_fields(Result);
    Rows = mysql_num_rows(Result);
    XdmfDebug("Query " << this->Query << " Returned " << Fields << "  Fields");
    XdmfDebug("Query " << this->Query << " Returned " << Rows << "  Rows");
    NValues = this->DataDesc->GetSelectionSize();
    // RetArray->SetNumberOfElements(Fields * Rows);
    Index = 0;
    while((Row = mysql_fetch_row(Result)) && (Index < NValues)){
        for(FieldIndex = 0 ; FieldIndex < Fields ; FieldIndex++){
            RetArray->SetValues(Index++, Row[FieldIndex]);
        }
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
    mysql_free_result(Result);
    mysql_close(Connection);
    return(RetArray);
}

XdmfInt32
XdmfValuesMySQL::Write(XdmfArray *anArray, XdmfConstString /*HeavyDataSetName*/){

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
    return(this->Set("CDATA", StringOutput.str()));
}

}
