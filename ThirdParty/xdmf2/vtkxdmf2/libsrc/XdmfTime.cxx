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
#include "XdmfTime.h"

#include "XdmfDOM.h"
#include "XdmfArray.h"
#include "XdmfDataItem.h"
#include "XdmfGrid.h"

XdmfTime::XdmfTime() {
    this->SetElementName("Time");
    this->Value = 0.0;
    this->Array = NULL;
    this->Epsilon = 1e-7;
    this->DataItem = new XdmfDataItem;
    this->TimeType = XDMF_TIME_UNSET;
    this->Function = NULL;
}

XdmfTime::~XdmfTime() {
    if(this->DataItem) delete this->DataItem;
}

XdmfInt32
XdmfTime::Insert( XdmfElement *Child){
    if(Child && XDMF_WORD_CMP(Child->GetElementName(), "Time")){
        return(XdmfElement::Insert(Child));
    }else{
        XdmfErrorMessage("Time can only Insert Time elements");
    }
    return(XDMF_FAIL);
}

XdmfInt32 XdmfTime::UpdateInformation(){
    XdmfConstString attribute;
    XdmfFloat64 dValue;

    if(XdmfElement::UpdateInformation() != XDMF_SUCCESS) return(XDMF_FAIL);
    attribute = this->Get("TimeType");
    if(!attribute) attribute = this->Get("Type");
    if( XDMF_WORD_CMP(attribute, "Single") ){
        this->TimeType = XDMF_TIME_SINGLE;
    }else if( XDMF_WORD_CMP(attribute, "List") ){
        this->TimeType = XDMF_TIME_LIST;
    }else if( XDMF_WORD_CMP(attribute, "Range") ){
        this->TimeType = XDMF_TIME_RANGE;
    }else if( XDMF_WORD_CMP(attribute, "HyperSlab") ){
        this->TimeType = XDMF_TIME_HYPERSLAB;
    }else if( XDMF_WORD_CMP(attribute, "Function") ){
        this->TimeType = XDMF_TIME_FUNCTION;
    }else{
        if(attribute){
            XdmfErrorMessage("Unknown Time Type : " << attribute);
            return(XDMF_FAIL);
        }
        // Default
        this->TimeType = XDMF_TIME_SINGLE;
    }
    // Type == Function ?
    attribute = this->Get("Function");
    if(attribute){
        this->TimeType = XDMF_TIME_FUNCTION;
        this->SetFunction(attribute);
        return(XDMF_SUCCESS);
    }
    attribute = this->Get("Value");
    if(attribute){
        istrstream Value_ist(const_cast<char*>(attribute), strlen(attribute) );
        Value_ist >> dValue;
        this->SetValue(dValue);
    }else{
        XdmfXmlNode     node;

        if(this->TimeType == XDMF_TIME_SINGLE){
            XdmfErrorMessage("TimeType is Single but there is no Value Attribute");
            return(XDMF_FAIL);
        }
        node = this->DOM->FindDataElement(0, this->GetElement());
        if(!node){
            XdmfErrorMessage("No Time Value is set and there is no DataItem");
            return(XDMF_FAIL);
        }
        if(this->DataItem->SetDOM(this->DOM) == XDMF_FAIL) return(XDMF_FAIL);
        if(this->DataItem->SetElement(node) == XDMF_FAIL) return(XDMF_FAIL);
        if(this->DataItem->UpdateInformation() == XDMF_FAIL) return(XDMF_FAIL);
        if(this->DataItem->Update() == XDMF_FAIL) return(XDMF_FAIL);
        this->Array = this->DataItem->GetArray();
    }
    return(XDMF_SUCCESS);
}

XdmfInt32 XdmfTime::Build(){
    if(this->TimeType == XDMF_TIME_UNSET) return(XDMF_SUCCESS);
    if(XdmfElement::Build() != XDMF_SUCCESS) return(XDMF_FAIL);
    this->Set("TimeType", this->GetTimeTypeAsString());
    if(this->TimeType == XDMF_TIME_FUNCTION){
        this->Set("Function", this->Function);
        return(XDMF_SUCCESS);
    }
    if(this->Array){
        XdmfDataItem    *di = NULL;
        XdmfXmlNode     node;

        XdmfDebug("Build for XdmfTime = " << this);
        //! Is there already a DataItem
        node = this->DOM->FindDataElement(0, this->GetElement());
        if(node) {
            di = (XdmfDataItem *)this->GetCurrentXdmfElement(node);
            XdmfDebug("DataItem  = " << di);
        }
        if(!di){
            di = new XdmfDataItem;
            node = this->DOM->InsertNew(this->GetElement(), "DataItem");
            di->SetDOM(this->DOM);
            di->SetElement(node);
            if(this->Array->GetNumberOfElements() > 100) di->SetFormat(XDMF_FORMAT_HDF);
        }
        if(this->Array != di->GetArray()){
            XdmfDebug("Setting Array since " << this->Array << " != " << di->GetArray());
            di->SetArray(this->Array);
        }
        XdmfDebug("Building DataItem");
        di->Build();
        this->SetCurrentXdmfElement(di->GetElement(), NULL);
        delete di;
    }else{
        ostrstream   StringOutput;
        StringOutput << this->Value << ends;
        this->Set("Value", StringOutput.str());
        StringOutput.rdbuf()->freeze(0);
    }
    return(XDMF_SUCCESS);
}

XdmfInt32
XdmfTime::Evaluate(XdmfGrid *Grid, XdmfArray *ArrayToFill, XdmfInt32 Descend, XdmfInt32 Append){
    XdmfInt64   i, n, nelements;
    XdmfTime    *gt;

    if(!ArrayToFill){
        XdmfErrorMessage("Array to fill is NULL");
        return(XDMF_FAIL);
    }
    if(Append){
        nelements = ArrayToFill->GetNumberOfElements();
    }else{
        ArrayToFill->SetNumberType(XDMF_FLOAT64_TYPE);
        nelements = 0;
    }
    gt = Grid->GetTime();
    if(gt){
        switch(gt->GetTimeType()){
            case XDMF_TIME_SINGLE :
                nelements += 1;
                ArrayToFill->SetNumberOfElements(nelements);
                ArrayToFill->SetValueFromFloat64(nelements - 1, gt->GetValue());
                break;
            case XDMF_TIME_RANGE :
            case XDMF_TIME_LIST :
                // cout << "Adding " << gt->GetArray()->GetValues() << endl;
                n = gt->GetArray()->GetNumberOfElements();
                nelements += n;
                ArrayToFill->SetNumberOfElements(nelements);
                for(i=0 ; i < n ; i++){
                    ArrayToFill->SetValueFromFloat64(nelements - n + i, gt->GetArray()->GetValueAsFloat64(i));
                }
                // cout << "Array = " << ArrayToFill->GetValues() << endl;
                break;
            case XDMF_TIME_HYPERSLAB :
                n = gt->GetArray()->GetValueAsInt64(2);
                nelements += n;
                ArrayToFill->SetNumberOfElements(nelements);
                for(i=0 ; i < n ; i++){
                    ArrayToFill->SetValueFromFloat64(nelements - n + i,
                        gt->GetArray()->GetValueAsFloat64(0) + (gt->GetArray()->GetValueAsFloat64(1) * i));
                }
                // cout << "Array = " << ArrayToFill->GetValues() << endl;
                break;
            default :
                if(!Descend) return(XDMF_FAIL);
                break;
        }
    }else{
        XdmfErrorMessage("Grid has no XdmfTime");
    }
    if(Descend){
        for(i=0 ; i < Grid->GetNumberOfChildren() ; i++){
            // Append children's times
            if(this->Evaluate(Grid->GetChild(i), ArrayToFill, Descend, 1) != XDMF_SUCCESS) return(XDMF_FAIL);
        }
    }
    if(this->TimeType == XDMF_TIME_RANGE) {
        XdmfFloat64 minval, maxval;

        minval = ArrayToFill->GetMinAsFloat64();
        maxval = ArrayToFill->GetMaxAsFloat64();
        ArrayToFill->SetNumberOfElements(2);
        ArrayToFill->SetValueFromFloat64(0, minval);
        ArrayToFill->SetValueFromFloat64(1, maxval);
    }
return(XDMF_SUCCESS);
}

XdmfInt32
XdmfTime::SetTimeFromParent(XdmfTime *ParentTime, XdmfInt64 Index){
    XdmfArray *TimeArray;

    if(!ParentTime || (Index < 0)) return(XDMF_FAIL);
    // this->DebugOn();
    XdmfDebug("Setting Time from Type " << ParentTime->GetTimeTypeAsString() << " Index = " << Index);
    switch(ParentTime->GetTimeType()){
        case XDMF_TIME_SINGLE:
            this->TimeType = XDMF_TIME_SINGLE;
            this->Value = ParentTime->GetValue();
            XdmfDebug("Setting Time Value to " << this->Value);
            break;
        case XDMF_TIME_HYPERSLAB :
            TimeArray = ParentTime->GetArray();
            if(!TimeArray){
                XdmfErrorMessage("TimeType is HyperSlab but there is no array");
                return(XDMF_FAIL);
            }
            this->TimeType = XDMF_TIME_SINGLE;
            this->Value = TimeArray->GetValueAsFloat64(0) + (TimeArray->GetValueAsFloat64(1) * Index);
            XdmfDebug("Setting Time Value to " << this->Value);
            break;
        case XDMF_TIME_LIST :
            TimeArray = ParentTime->GetArray();
            if(!TimeArray){
                XdmfErrorMessage("TimeType is List but there is no array");
                return(XDMF_FAIL);
            }
            this->TimeType = XDMF_TIME_SINGLE;
            this->SetValue(TimeArray->GetValueAsFloat64(Index));
            XdmfDebug("Setting Time Value to " << this->GetValue());
            break;
        case XDMF_TIME_RANGE :
            this->TimeType = XDMF_TIME_RANGE;
            this->Array = ParentTime->GetArray();
            break;
        default :
            XdmfErrorMessage("Unknown or Invalid TimeType");
            return(XDMF_FAIL);
    }
    return(XDMF_SUCCESS);
}

XdmfConstString
XdmfTime::GetTimeTypeAsString(void){
    switch(this->TimeType){
        case XDMF_TIME_UNSET:
            return("Unset");
        case XDMF_TIME_LIST:
            return("List");
        case XDMF_TIME_RANGE:
            return("Range");
        case XDMF_TIME_HYPERSLAB:
            return("HyperSlab");
        case XDMF_TIME_FUNCTION:
            return("Function");
        default :
            return("Single");
    }
}

XdmfInt32
XdmfTime::IsValid(XdmfTime *TimeSpec){
    XdmfFloat64 minval, maxval;

    // cout << "this->TimeType = " << this->GetTimeTypeAsString() << endl;
    // cout << "TimeSpec->TimeType = " << TimeSpec->GetTimeTypeAsString() << endl;
    switch(TimeSpec->TimeType){
        case XDMF_TIME_SINGLE :
            minval = TimeSpec->GetValue();
            maxval = TimeSpec->GetValue();
            break;
        case XDMF_TIME_LIST :
            if(!TimeSpec->Array){
                XdmfErrorMessage("XdmfTime has no Array");
                return(XDMF_FALSE);
            }
            minval = TimeSpec->Array->GetMinAsFloat64();
            maxval = TimeSpec->Array->GetMaxAsFloat64();
            break;
        case XDMF_TIME_RANGE :
            if(!TimeSpec->Array){
                XdmfErrorMessage("XdmfTime has no Array");
                return(XDMF_FALSE);
            }
            minval = TimeSpec->Array->GetValueAsFloat64(0);
            maxval = TimeSpec->Array->GetValueAsFloat64(1);
            break;
        case XDMF_TIME_HYPERSLAB :
            if(!TimeSpec->Array){
                XdmfErrorMessage("XdmfTime has no Array");
                return(XDMF_FALSE);
            }
            minval = TimeSpec->Array->GetValueAsFloat64(0);
            maxval = (TimeSpec->Array->GetValueAsFloat64(1) * (TimeSpec->Array->GetValueAsFloat64(2) - 1));
            break;
        default :
            return(XDMF_FALSE);
    }
 return(this->IsValid(minval, maxval));
}

XdmfInt32
XdmfTime::IsValid(XdmfFloat64 TimeMin, XdmfFloat64 TimeMax){
    TimeMin -= this->Epsilon;
    TimeMax += this->Epsilon;
    switch(this->TimeType){
        case XDMF_TIME_SINGLE :
    // cout << "TimeMin, TimeMax, this->GetValue() = " << TimeMin << "," << TimeMax << "," << this->GetValue() << endl;
            if((this->GetValue() >= TimeMin) && (this->GetValue() <= TimeMax)){
                    // cout << "Time Test Passed" << endl;
                    return(XDMF_TRUE);
            }else{
                    // cout << "Time Test Failed" << endl;
            }
            break;
        case XDMF_TIME_LIST :
            if(!this->Array){
                XdmfErrorMessage("XdmfTime has no Array");
                return(XDMF_FALSE);
            }
            if((this->Array->GetMinAsFloat64() >= TimeMin) && (this->Array->GetMaxAsFloat64() <= TimeMax)) return(XDMF_TRUE);
            break;
        case XDMF_TIME_RANGE :
        // cout << "TimeMin, TimeMax, minmaxrange  = " << TimeMin << "," << TimeMax <<
        // "," << this->Array->GetValueAsFloat64(0) <<
        // "," << this->Array->GetValueAsFloat64(1) <<
        // endl;
            if(!this->Array){
                XdmfErrorMessage("XdmfTime has no Array");
                return(XDMF_FALSE);
            }
            if((this->Array->GetValueAsFloat64(0) >= TimeMin) && (this->Array->GetValueAsFloat64(1) <= TimeMax)) return(XDMF_TRUE);
            break;
        case XDMF_TIME_HYPERSLAB :
            if(!this->Array){
                XdmfErrorMessage("XdmfTime has no Array");
                return(XDMF_FALSE);
            }
            if((this->Array->GetValueAsFloat64(0) >= TimeMin) &&
                    ((this->Array->GetValueAsFloat64(1) * (this->Array->GetValueAsFloat64(2) - 1)) <= TimeMax)) return(XDMF_TRUE);
            break;
        default :
            return(XDMF_FALSE);
    }
return(XDMF_FALSE);
}
