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
#include "XdmfDataItem.h"
#include "XdmfDataDesc.h"
#include "XdmfExpression.h"
#include "XdmfArray.h"
#include "XdmfDOM.h"
#include "XdmfHDF.h"

// Supported Xdmf Formats
#include "XdmfValuesXML.h"
#include "XdmfValuesHDF.h"
#ifdef XDMF_USE_MYSQL
#include "XdmfValuesMySQL.h"
#endif
#include "XdmfValuesBinary.h"

#include <libxml/tree.h>

XdmfDataItem::XdmfDataItem() {
    this->SetElementName("DataItem");
    this->Values = NULL;
    this->DataDesc = new XdmfDataDesc();
    this->DataDescIsMine = 1;
    this->Array = new XdmfArray();
    this->ArrayIsMine = 1;
    this->Array->SetNumberType(XDMF_FLOAT32_TYPE);
    this->Array->SetNumberOfElements(3);
    this->Format = XDMF_FORMAT_XML;
    this->HeavyDataSetName = NULL;
    this->ItemType = XDMF_ITEM_UNIFORM;
    this->Function = NULL;
    this->ColumnMajor = 0;
    this->TransposeInMemory = 1;
}

XdmfDataItem::~XdmfDataItem() {
    XdmfDebug(".... Deleteing DataItem " << this);
    if(this->Array && this->ArrayIsMine ){
        XdmfDebug("Deleting array " << this->Array);
        delete this->Array;
        XdmfDebug("Done Deleteing array");
    }
    if(this->DataDesc && this->DataDescIsMine) delete this->DataDesc;
    if(this->Values) delete this->Values;
    delete [] this->HeavyDataSetName;
}

XdmfInt32
XdmfDataItem::Release(){
    // Don't Delete it.
    if(this->Array) this->Array->SetNumberOfElements(3);
    return(XDMF_SUCCESS);
}

XdmfInt32
XdmfDataItem::Insert( XdmfElement *Child){
    if(Child && (
        XDMF_WORD_CMP(Child->GetElementName(), "DataItem") ||
        XDMF_WORD_CMP(Child->GetElementName(), "Information")
        )){
        return(XdmfElement::Insert(Child));
    }else{
        XdmfErrorMessage("DataItem can only Insert DataItem or Information elements");
    }
    return(XDMF_FAIL);
}

XdmfArray *
XdmfDataItem::GetArray(XdmfInt32 Create){
    if(!this->Array && Create){
        this->Array = new XdmfArray;
        this->ArrayIsMine = 1;
    }
    return(this->Array);
}

XdmfInt32
XdmfDataItem::SetArray(XdmfArray *anArray){
    if(this->Array && this->ArrayIsMine) delete this->Array;
    this->ArrayIsMine = 0;
    this->Array = anArray;
    return(XDMF_SUCCESS);
}

XdmfInt32
XdmfDataItem::SetDataDesc(XdmfDataDesc *aDataDesc){
    if(this->DataDesc && this->DataDescIsMine) delete this->DataDesc;
    this->DataDescIsMine = 0;
    this->DataDesc = aDataDesc;
    return(XDMF_SUCCESS);
}

// Derived version
XdmfInt32
XdmfDataItem::Copy(XdmfElement *Source){
    XdmfDataItem *ds;

    XdmfDebug("XdmfDataItem::Copy(XdmfElement *Source)");
    ds = (XdmfDataItem *)Source;
    // this->SetDOM(ds->GetDOM());
    this->SetFormat(ds->GetFormat());
    this->SetHeavyDataSetName(ds->GetHeavyDataSetName());
    this->SetDsmBuffer(ds->GetDsmBuffer());
    this->DataDesc->CopyType(ds->GetDataDesc());
    this->DataDesc->CopyShape(ds->GetDataDesc());
    this->DataDesc->CopySelection(ds->GetDataDesc());
    if(CopyReferenceData){
        XdmfDebug("Copying Data From Array");
        this->SetArray(ds->GetArray()->Clone());
        if(!this->Array){
            XdmfErrorMessage("Error Cloning Array");
            return(XDMF_FAIL);
        }
        // Allow for future deletion
        this->ArrayIsMine = 1;
    }else{
        XdmfDebug("Referenceing Data From Array");
        // Problem if owner deletes
        this->SetArray(ds->GetArray());
    }
    return(XDMF_SUCCESS);
}

XdmfInt32 XdmfDataItem::UpdateInformationFunction(){
    switch(this->ItemType){
        case XDMF_ITEM_FUNCTION :
           XdmfConstString Value;
           Value = this->Get("Function");
           this->SetFunction(Value);
           delete Value;
           break;
        case XDMF_ITEM_HYPERSLAB :
            break;
        case XDMF_ITEM_COORDINATES :
            break;
    }
    return(XDMF_SUCCESS);
}

XdmfInt32 XdmfDataItem::UpdateInformationCollection(){
    return(XDMF_SUCCESS);
}

XdmfInt32 XdmfDataItem::UpdateInformationTree(){
    return(XDMF_SUCCESS);
}

XdmfInt32 XdmfDataItem::UpdateInformationUniform(){
    XdmfConstString Value = 0;
    XdmfInt32   Precision = 4;

    Value = this->Get("Precision");
    if(Value) Precision = atoi(Value);
    free((void*)Value);
    Value = this->Get("NumberType");
    // Try DataType
    if(!Value) Value = this->Get("DataType");
    //! Try Old Style
    if(!Value) Value = this->Get("Type");
    // Only allow Simple for now.
    if(XDMF_WORD_CMP(Value, "Char")){
        this->DataDesc->SetNumberType(XDMF_INT8_TYPE);
    } else if(XDMF_WORD_CMP(Value, "UChar")){
        this->DataDesc->SetNumberType(XDMF_UINT8_TYPE);
    } else if(XDMF_WORD_CMP(Value, "Short")){
        this->DataDesc->SetNumberType(XDMF_INT16_TYPE);
    } else if(XDMF_WORD_CMP(Value, "UShort")){
        this->DataDesc->SetNumberType(XDMF_UINT16_TYPE);
    } else if(XDMF_WORD_CMP(Value, "Int")){
        if(Precision == 8){
            this->DataDesc->SetNumberType(XDMF_INT64_TYPE);
        }else{
            this->DataDesc->SetNumberType(XDMF_INT32_TYPE);
        }
    } else if(XDMF_WORD_CMP(Value, "UInt")){
        this->DataDesc->SetNumberType(XDMF_UINT32_TYPE);
    } else {
        if(Precision == 8){
            this->DataDesc->SetNumberType(XDMF_FLOAT64_TYPE);
        }else{
            this->DataDesc->SetNumberType(XDMF_FLOAT32_TYPE);
        }
    }
    free((void*)Value);;
    Value = this->Get("Format");
    // Currently XML or HDF5
    if(XDMF_WORD_CMP(Value, "HDF")){
        this->SetFormat(XDMF_FORMAT_HDF);
    } else if(XDMF_WORD_CMP(Value, "HDF5")){
        this->SetFormat(XDMF_FORMAT_HDF);
    } else if(XDMF_WORD_CMP(Value, "H5")){
        this->SetFormat(XDMF_FORMAT_HDF);
    } else if(XDMF_WORD_CMP(Value, "XML")){
        this->SetFormat(XDMF_FORMAT_XML);
    } else if(XDMF_WORD_CMP(Value, "MYSQL")){
        this->SetFormat(XDMF_FORMAT_MYSQL);
    } else if(XDMF_WORD_CMP(Value, "BINARY")){
        this->SetFormat(XDMF_FORMAT_BINARY);
    }else if(Value){
        XdmfErrorMessage("Unsupported DataItem Format :" << Value);
        free((void*)Value);
        return(XDMF_FAIL);
    }
    free((void*)Value);
    return(XDMF_SUCCESS);
}

XdmfInt32 XdmfDataItem::UpdateInformation(){
    XdmfConstString Value;

  Value = this->Get("Major");
  if(Value)
  {
    if(XDMF_WORD_CMP(Value,"Column"))
      this->ColumnMajor = 1;
    else if(XDMF_WORD_CMP(Value,"Row"))
      this->ColumnMajor = 0;
    else
    {
        XdmfErrorMessage("invalid major");
        free((void*)Value);
        return(XDMF_FAIL);
    }
  }
  free((void*)Value);
  XdmfDebug("Major = " << this->ColumnMajor);

    XdmfDebug("XdmfDataItem::UpdateInformation()");
    if(XdmfElement::UpdateInformation() != XDMF_SUCCESS) return(XDMF_FAIL);
    // If this is a Reference, this->Element now points to the end of the chain. Continue?
    XdmfDebug("Back from XdmfElement::UpdateInformation() IsReference = " << this->GetIsReference());
    // XdmfDebug("o = " << this->GetReferenceObject(this->Element) << " this = " << this);
    // XdmfDebug("r = " << this->ReferenceElement << " e = " << this->Element);
    // XdmfDebug(this->DOM->Serialize(this->ReferenceElement));
    // Dtetermine type : Uniform, Collection, or Tree
    Value = this->Get("ItemType");
    if(!Value){
        // Try Old "Type=XX" Style from Xdmf Version 1.0
        Value = this->Get("Type");
    }
    if(!Value){
        this->SetItemType(XDMF_ITEM_UNIFORM);
    }else{
        // Allow for "break";
        do {
            if(XDMF_WORD_CMP(Value, "Uniform")){
                this->SetItemType(XDMF_ITEM_UNIFORM);
                break;
            }
            if(XDMF_WORD_CMP(Value, "Collection")){
                this->SetItemType(XDMF_ITEM_COLLECTION);
                break;
            }
            if(XDMF_WORD_CMP(Value, "Tree")){
                this->SetItemType(XDMF_ITEM_TREE);
                break;
            }
            if(XDMF_WORD_CMP(Value, "HyperSlab")){
                this->SetItemType(XDMF_ITEM_HYPERSLAB);
                break;
            }
            if(XDMF_WORD_CMP(Value, "Coordinates")){
                this->SetItemType(XDMF_ITEM_COORDINATES);
                break;
            }
            if(XDMF_WORD_CMP(Value, "Function")){
                this->SetItemType(XDMF_ITEM_FUNCTION);
                break;
            }
            XdmfErrorMessage("Unknown DataItem Type = " << Value);
            free((void*)Value);
            return(XDMF_FAIL);
        } while(0);
    }
    free((void*)Value);
    if(this->GetIsReference() &&
        (this->ReferenceElement != this->Element) &&
        (this->GetReferenceObject(this->Element) != this)){
        XdmfDebug("Reference DataItem Copied Info from another ReferenceObject");
        return(XDMF_SUCCESS);
    }
    Value = this->Get("Dimensions");
    if(!Value) {
        XdmfErrorMessage("Dimensions are not set in XML Element");
        XdmfErrorMessage(this->DOM->Serialize(this->Element));
        return(XDMF_FAIL);
    }
    if(!this->DataDesc) this->DataDesc = new XdmfDataDesc();
    this->DataDesc->SetShapeFromString(Value);
    free((void*)Value);;
    switch(this->ItemType){
        case XDMF_ITEM_UNIFORM :
            return(this->UpdateInformationUniform());
            break;
        case XDMF_ITEM_COLLECTION:
            return(this->UpdateInformationCollection());
            break;
        case XDMF_ITEM_TREE:
            return(this->UpdateInformationTree());
            break;
        case XDMF_ITEM_HYPERSLAB :
        case XDMF_ITEM_COORDINATES :
        case XDMF_ITEM_FUNCTION :
            return(this->UpdateInformationFunction());
            break;
        default :
            // Should Never get Here
            XdmfErrorMessage("Unknown ItemType");
            return(XDMF_FAIL);
            break;
    }
    // Should Never get Here
    return(XDMF_SUCCESS);
}

XdmfInt32 XdmfDataItem::UpdateFunction(){
        XdmfConstString  Value;
        XdmfArray  *ReturnArray;
        XdmfDataItem *ItemToDelete[100];
        XdmfInt32  Id, NTmp = 0;

    if(this->ItemType == XDMF_ITEM_COORDINATES){
        // $0 is Selection $1 is Data Source
            XdmfXmlNode  Argument;
            XdmfArray  *TmpArray;
            XdmfDataItem *TmpItem, *SrcItem;
            XdmfInt64   NCoordinates, *Coordinates;

            XdmfDebug("Updating Coordinate Selection");
            Argument = this->DOM->FindDataElement(0, this->Element);
            TmpItem = new XdmfDataItem();
            TmpItem->SetDOM(this->DOM);
            TmpItem->SetElement(Argument);
            TmpItem->UpdateInformation();
            TmpItem->Update();
            TmpArray = TmpItem->GetArray();
            NCoordinates = TmpArray->GetNumberOfElements();
            Coordinates = new XdmfInt64[NCoordinates];
            TmpArray->GetValues( 0, Coordinates, NCoordinates );
            // Now Access the Source Data
            SrcItem = new XdmfDataItem();
            Argument = this->DOM->FindDataElement(1, this->Element);
            SrcItem->SetDOM(this->DOM);
            SrcItem->SetElement(Argument);
            SrcItem->UpdateInformation();
            NCoordinates /= SrcItem->GetRank();
            SrcItem->GetDataDesc()->SelectCoordinates(NCoordinates, Coordinates);
            SrcItem->Update();
            // Steal The Array
            ReturnArray = SrcItem->GetArray();
            SrcItem->SetArrayIsMine(0);
            ItemToDelete[ NTmp++ ] = TmpItem;
            ItemToDelete[ NTmp++ ] = SrcItem;
            delete Coordinates;
    }
    if(this->ItemType == XDMF_ITEM_HYPERSLAB){
        // $0 is Selection $1 is Data Source
            XdmfXmlNode  Argument;
            XdmfArray  *TmpArray;
            XdmfDataItem *TmpItem, *SrcItem;
            XdmfInt64   Rank;
            XdmfInt64   Start[ XDMF_MAX_DIMENSION ];
            XdmfInt64   Stride[ XDMF_MAX_DIMENSION ];
            XdmfInt64   Count[ XDMF_MAX_DIMENSION ];

            XdmfDebug("Updating HyperSlab");
            Argument = this->DOM->FindDataElement(0, this->Element);
            TmpItem = new XdmfDataItem();
            TmpItem->SetDOM(this->DOM);
            TmpItem->SetElement(Argument);
            TmpItem->UpdateInformation();
            TmpItem->Update();
            TmpArray = TmpItem->GetArray();
            Rank = TmpArray->GetNumberOfElements() / 3; // Start, Stride, Count for each Source Dim
            TmpArray->GetValues(0, Start, Rank);
            TmpArray->GetValues(Rank, Stride, Rank);
            TmpArray->GetValues(2 * Rank, Count, Rank);
            XdmfDebug("Selection is " << TmpArray->GetValues());
            // Now Access the Source Data
            SrcItem = new XdmfDataItem();
            Argument = this->DOM->FindDataElement(1, this->Element);
            SrcItem->SetDOM(this->DOM);
            SrcItem->SetElement(Argument);
            SrcItem->UpdateInformation();
            SrcItem->GetDataDesc()->SelectHyperSlab( Start, Stride, Count );
            SrcItem->Update();
            // Steal The Array
            ReturnArray = SrcItem->GetArray();
            SrcItem->SetArrayIsMine(0);
            ItemToDelete[ NTmp++ ] = TmpItem;
            ItemToDelete[ NTmp++ ] = SrcItem;

    }
    if(this->ItemType == XDMF_ITEM_FUNCTION){
        ostrstream  FunctionToEval;
        char    c, *StreamString, *scdata;


        if(!this->Function){
                XdmfErrorMessage("Function is NULL");
                return(XDMF_FAIL);
        }
        XDMF_STRING_DUPLICATE(scdata, this->Function);
        XdmfDebug("Transform Function = " << scdata);
        XdmfConstString ch = scdata;
        while( (c = *ch++) ) {
            if( c == '$' ) {
                XdmfXmlNode  Argument;
                XdmfArray  *TmpArray;
                XdmfDataItem *TmpItem;
                Id = atoi(ch);
                while( (c = *ch++) ) {
                    if( c > ' ') break;
                }
                Argument = this->DOM->FindDataElement(Id, this->Element );
                TmpItem = new XdmfDataItem();
                TmpItem->SetDOM(this->DOM);
                TmpItem->SetElement(Argument);
                TmpItem->UpdateInformation();
                TmpItem->Update();
                TmpArray = TmpItem->GetArray();
                if( TmpArray->GetNumberOfElements() == 1 ){
                    XdmfDebug("Using Scalar = " << TmpArray->GetValueAsFloat64( 0 ) );
                    FunctionToEval << " " << TmpArray->GetValueAsFloat64( 0 ) << " ";
                    delete TmpArray;
                } else {
                    ItemToDelete[ NTmp++ ] = TmpItem;
                    FunctionToEval << " " << TmpArray->GetTagName() << " ";
                }
            } else {
                FunctionToEval << c;
            }
        }
        delete [] scdata;
        FunctionToEval << ends;
        StreamString = FunctionToEval.str();
        XdmfDebug("Function Translation = " << StreamString );
        ReturnArray = XdmfExpr(StreamString);
        delete [] StreamString;
    }
    // Reform and Cleanup
    Value = this->DOM->Get( Element, "Dimensions" );
    if(Value && ReturnArray){
        ReturnArray->ReformFromString(Value);
    }
    free((void*)Value);;
    // If only a portion of the DataItem was requested
    // the XdmfValues did not reflect this selection since
    // DataDesc was used to select HyperSlad | Coordinates | Function
    if(this->DataDesc->GetSelectionType() != XDMF_SELECTALL){
            XdmfArray   *Portion;
            XdmfInt64  SelectionSize = this->DataDesc->GetSelectionSize();

            Portion = ReturnArray->Clone();
            ReturnArray->SetShape(1, &SelectionSize);
            ReturnArray->SelectAll();
            Portion->CopySelection(this->DataDesc);
            CopyArray(Portion, ReturnArray);
            delete Portion;
    }
    while( NTmp ){
        NTmp--;
        XdmfDebug("Deleteing DataItem #" << NTmp );
        delete ItemToDelete[ NTmp ];
    }
    this->SetArray(ReturnArray);
    // We'll need to delete this
    this->ArrayIsMine = 1;
    return( XDMF_SUCCESS);
}

XdmfInt32 XdmfDataItem::Update(){
    if(XdmfElement::Update() != XDMF_SUCCESS) return(XDMF_FAIL);
    if(this->IsReference){
        XdmfDebug("This is a Reference");
    }else{
        XdmfDebug("This is not a Reference");
    }
    if(this->GetIsReference() && (this->GetReferenceObject(this->Element) != this)){
        XdmfDebug("Reference DataItem Copied Info from another ReferenceObject");
        return(XDMF_SUCCESS);
    }
    if(this->GetIsMultiple()){
        XdmfDebug("Item Type does not evaluate to a single array. Done");
        return(XDMF_SUCCESS);
    }
    if(this->ItemType != XDMF_ITEM_UNIFORM){
        return(this->UpdateFunction());
    }
    if(this->Array->CopyType(this->DataDesc) != XDMF_SUCCESS) return(XDMF_FAIL);
    // if(this->Array->CopyShape(this->DataDesc) != XDMF_SUCCESS) return(XDMF_FAIL);
    // if(this->Array->CopySelection(this->DataDesc) != XDMF_SUCCESS) return(XDMF_FAIL);
    if(this->CheckValues(this->Format) != XDMF_SUCCESS){
        XdmfErrorMessage("Error Accessing Internal XdmfValues");
        return(XDMF_FAIL);
    }
    if(this->Values->GetDataDesc()->CopySelection(this->DataDesc) != XDMF_SUCCESS) return(XDMF_FAIL);
    XdmfDebug("Resize Array");
    if(this->Array->SetShapeFromSelection(this->DataDesc) != XDMF_SUCCESS) return(XDMF_FAIL);
    switch (this->Format) {
        case XDMF_FORMAT_HDF :
            this->Values->SetDebug(this->GetDebug());
            // this->SetDsmBuffer(this->Values->GetDsmBuffer());
            // cout << "Setting Values Dsm to " << this->DsmBuffer << endl;
            this->Values->SetDsmBuffer(this->DsmBuffer);
            XdmfDebug("Reading Data");
            if(!((XdmfValuesHDF *)this->Values)->Read(this->Array)){
                XdmfErrorMessage("Reading Values Failed");
                return(XDMF_FAIL);
            }
            this->SetHeavyDataSetName(this->Values->GetHeavyDataSetName());
            this->Array->SetHeavyDataSetName(this->Values->GetHeavyDataSetName());
            break;
        case XDMF_FORMAT_XML :
            this->Values->SetDebug(this->GetDebug());
            if(!((XdmfValuesXML *)this->Values)->Read(this->Array)){
                XdmfErrorMessage("Reading Values Failed");
                return(XDMF_FAIL);
            }
            break;
        case XDMF_FORMAT_MYSQL :
            this->Values->SetDebug(this->GetDebug());
#ifdef XDMF_USE_MYSQL
            if(!((XdmfValuesMySQL *)this->Values)->Read(this->Array)){
                XdmfErrorMessage("Reading Values Failed");
                return(XDMF_FAIL);
            }
#else
            XdmfErrorMessage("XdmfValuesMySQL not enabled in this Xdmf");
            return(XDMF_FAIL);
#endif

            break;
        case XDMF_FORMAT_BINARY :
            this->Values->SetDebug(this->GetDebug());
            if(!((XdmfValuesBinary *)this->Values)->Read(this->Array)){
                XdmfErrorMessage("Reading Values Failed");
                return(XDMF_FAIL);
            }
            break;
        default :
            XdmfErrorMessage("Unsupported Data Format");
            return(XDMF_FAIL);
    }

  // Support for Fortran matrices (2D arrays) added by Dominik Szczerba, June 2009.
  // Can be further optimized by e.g. in situ transpose methods
  // discussed on http://en.wikipedia.org/wiki/In-place_matrix_transposition
  if(ColumnMajor)
  {
    if(TransposeInMemory)
    {
      XdmfDebug("Transposing Array " << this->GetHeavyDataSetName());
      XdmfInt32 rank = 0;
      XdmfInt64 dims[XDMF_MAX_DIMENSION];
      rank = GetShape(dims);
      XdmfDebug("rank = " << rank);
      if(rank!=2)
      {
        XdmfErrorMessage("transpose is only implemented for rank 2 arrays");
        return(XDMF_FAIL);
      }
      // for(int i=0; i<rank; i++)
        // cerr << "DEBUG: dims[" << i << "] = " << dims[i] << endl;

      const int NI = dims[0], NJ = dims[1];

      void* ArrayT;
      switch(this->Array->GetNumberType())
      {
        case XDMF_FLOAT64_TYPE:
          ArrayT = (XdmfFloat64*)new XdmfFloat64[NI*NJ];
          break;
        case XDMF_INT32_TYPE:
          ArrayT = (XdmfInt32*)new XdmfInt32[NI*NJ];
          break;
        // TODO implement the other types
        default:
          XdmfErrorMessage("unknown data type");
          return(XDMF_FAIL);
      }

      // XdmfPointer ArrayPointer, ArrayPointerT;
      XdmfPointer ArrayPointer;

      for(int j=0; j<NJ; j++)
      {
        for(int i=0; i<NI; i++)
        {
          const int idx  = NI*j+i;
          const int idxT = NJ*i+j;
          // cerr << "DEBUG: idx/idxT = " << idx << " " << idxT << endl;
          switch(this->Array->GetNumberType())
          {
            case XDMF_FLOAT64_TYPE:
              ArrayPointer = (XdmfFloat64*)this->Array->GetDataPointer(idxT);
              ((XdmfFloat64*)ArrayT)[idx] = *(XdmfFloat64*)ArrayPointer;
              break;
            case XDMF_INT32_TYPE:
              ArrayPointer = (XdmfInt32*)this->Array->GetDataPointer(idxT);
              ((XdmfInt32*)ArrayT)[idx] = *(XdmfInt32*)ArrayPointer;
              break;
            // TODO implement the other types
            default:
              XdmfErrorMessage("unknown data type");
              return(XDMF_FAIL);
          }
        }
      }

      std::swap(dims[0], dims[1]);
      Array->Reform(rank,dims);

      for(int l=0; l<NI*NJ; l++)
      {
        switch(this->Array->GetNumberType())
        {
          case XDMF_FLOAT64_TYPE:
            // cerr << "DEBUG: l = " << l  << " " << ((XdmfFloat64*)ArrayT)[l] << endl;
            this->Array->SetValue(l,((XdmfFloat64*)ArrayT)[l]);
            break;
          case XDMF_INT32_TYPE:
            // cerr << "DEBUG: l = " << l << " " << ((XdmfInt32*)ArrayT)[l] << endl;
            this->Array->SetValue(l,((XdmfInt32*)ArrayT)[l]);
            break;
          // TODO implement the other types
          default:
            XdmfErrorMessage("unknown data type");
            return(XDMF_FAIL);
        }
      }

      // delete [] ArrayT;
      switch(this->Array->GetNumberType())
      {
        case XDMF_FLOAT64_TYPE:
          delete [] (XdmfFloat64*)ArrayT;
          break;
        case XDMF_INT32_TYPE:
          delete [] (XdmfInt32*)ArrayT;
          break;
        // TODO implement the other types
        default:
          XdmfErrorMessage("unknown data type");
          return(XDMF_FAIL);
      }

      XdmfDebug("done transpose");

    }
    else
    {
      XdmfErrorMessage("out-of-core transpose not implemented yet");
      return(XDMF_FAIL);
    }


  }

    return(XDMF_SUCCESS);
}

XdmfString XdmfDataItem::GetDataValues(XdmfInt64 Index, XdmfInt64 NumberOfValues, XdmfInt64 ArrayStride){
    if(!this->Array) return(NULL);
    return(this->Array->GetValues(Index, NumberOfValues, ArrayStride));
}

XdmfInt32 XdmfDataItem::SetDataValues(XdmfInt64 Index, XdmfConstString someValues, XdmfInt64 ArrayStride, XdmfInt64 ValuesStride){
    if(!this->Array){
        XdmfErrorMessage("DataItem has no XdmfArray");
        return(XDMF_FAIL);
    }
    return(this->Array->SetValues(Index, someValues, ArrayStride, ValuesStride));
}

XdmfInt32   XdmfDataItem::GetRank() {
    if(!this->DataDesc){
        XdmfErrorMessage("There is no XdmfDataDesc");
        return(XDMF_FAIL);
    }
    return(this->DataDesc->GetRank());
}

XdmfInt32 XdmfDataItem::SetShape(XdmfInt32 Rank, XdmfInt64 *Dimensions){
    if(!this->DataDesc){
        XdmfErrorMessage("There is no XdmfDataDesc");
        return(XDMF_FAIL);
    }
    return(this->DataDesc->SetShape(Rank, Dimensions));
}

XdmfInt32 XdmfDataItem::GetShape(XdmfInt64 *Dimensions){
    if(!this->DataDesc){
        XdmfErrorMessage("There is no XdmfDataDesc");
        return(XDMF_FAIL);
    }
    return(this->DataDesc->GetShape(Dimensions));
}

XdmfInt32 XdmfDataItem::SetDimensionsFromString(XdmfConstString Dimensions){
    if(!this->DataDesc){
        XdmfErrorMessage("There is no XdmfDataDesc");
        return(XDMF_FAIL);
    }
    return(this->DataDesc->SetShapeFromString(Dimensions));
}

XdmfConstString XdmfDataItem::GetShapeAsString(){
    if(!this->DataDesc){
        XdmfErrorMessage("There is no XdmfDataDesc");
        return(NULL);
    }
    return(this->DataDesc->GetShapeAsString());
}

XdmfInt32 XdmfDataItem::Build(){
    XdmfDataDesc *aDataDesc = this->DataDesc;
    if(XdmfElement::Build() != XDMF_SUCCESS) return(XDMF_FAIL);
    if(this->Array) aDataDesc = this->Array;
    this->Set("Dimensions", aDataDesc->GetShapeAsString());
    this->Set("NumberType", XdmfTypeToClassString(aDataDesc->GetNumberType()));
    switch (aDataDesc->GetElementSize()) {
        case 8 :
            this->Set("Precision", "8");
            break;
        case 4 :
            this->Set("Precision", "4");
            break;
  case 2 :
      this->Set("Precision", "2");
      break;
        case 1 :
            this->Set("Precision", "1");
            break;
        default :
            break;
    }
    switch (this->Format) {
        case XDMF_FORMAT_HDF :
            this->Set("Format", "HDF");
            break;
        case XDMF_FORMAT_XML :
            this->Set("Format", "XML");
            break;
        case XDMF_FORMAT_MYSQL :
            this->Set("Format", "MYSQL");
            break;
        case XDMF_FORMAT_BINARY :
            this->Set("Format", "BINARY");
            break;
        default :
            XdmfErrorMessage("Unsupported Data Format");
            return(XDMF_FAIL);
    }
    if(this->BuildFromDataXml(1) == XDMF_SUCCESS) return(XDMF_SUCCESS);
    if(this->CheckValues(this->Format) != XDMF_SUCCESS){
        XdmfErrorMessage("Error Accessing Internal XdmfValues");
        return(XDMF_FAIL);
    }
    this->Values->SetDataDesc(aDataDesc);
    switch (this->Format) {
        case XDMF_FORMAT_HDF :
            XdmfDebug("Writing Values in HDF Format");
            Values->SetHeavyDataSetName(this->GetHeavyDataSetName());
            Values->SetDsmBuffer(this->GetDsmBuffer());
            if(((XdmfValuesHDF *)Values)->Write(this->Array) != XDMF_SUCCESS){
                XdmfErrorMessage("Writing Values Failed");
                return(XDMF_FAIL);
            }
            this->Set("Format", "HDF");
            break;
        case XDMF_FORMAT_XML :
            XdmfDebug("Writing Values in XML Format");
            if(((XdmfValuesXML *)Values)->Write(this->Array) != XDMF_SUCCESS){
                XdmfErrorMessage("Writing Values Failed");
                return(XDMF_FAIL);
            }
            this->Set("Format", "XML");
            break;
        default :
            XdmfErrorMessage("Unsupported Data Format");
            return(XDMF_FAIL);
    }
    return(XDMF_SUCCESS);
}

XdmfInt32
XdmfDataItem::CheckValues(XdmfInt32 aFormat){
    if(this->Values){
        // Exists
        if(this->Values->Format != aFormat){
            // Wrong Format
            XdmfDebug("CheckValues Changing Format");
            delete this->Values;
            this->Values = NULL;
        }
    }
    if(!this->Values){
        // Create One of the Proper Format
        switch (this->Format) {
            case XDMF_FORMAT_HDF :
                this->Values = (XdmfValues *)new XdmfValuesHDF();
                break;
            case XDMF_FORMAT_XML :
                this->Values = (XdmfValues *)new XdmfValuesXML();
                break;
            case XDMF_FORMAT_MYSQL :
#ifdef XDMF_USE_MYSQL
                this->Values = (XdmfValues *)new XdmfValuesMySQL();
#else
                XdmfErrorMessage("MySQL not supported in this Xdmf");
                return(XDMF_FAIL);
#endif
                break;
            case XDMF_FORMAT_BINARY :
                this->Values = (XdmfValues *)new XdmfValuesBinary();
                break;
            default :
                XdmfErrorMessage("Unsupported Data Format");
                return(XDMF_FAIL);
        }
    }
    if(!this->Values){
        XdmfErrorMessage("Error Creating new XdmfValues");
        return(XDMF_FAIL);
    }
    if(this->Values->Inherit(this) != XDMF_SUCCESS){
        XdmfErrorMessage("Error Inheriting DOM, Element, and DataDesc");
        return(XDMF_FAIL);
    }
    return(XDMF_SUCCESS);
}


