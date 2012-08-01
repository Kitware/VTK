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
#include "XdmfDOM.h"
#include "XdmfElement.h"

#include <libxml/globals.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xinclude.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

XdmfDOM *HandleToXdmfDOM( XdmfConstString Source ){
  XdmfObject  *TempObj;
  XdmfDOM   *DOM;

  TempObj = HandleToXdmfObject( Source );
  DOM = (XdmfDOM *)TempObj;
  return( DOM );
  }

static XdmfXmlNode
XdmfGetNextElement(XdmfXmlNode Node){

XdmfXmlNode NextElement = Node->next;
while(NextElement && (NextElement->type != XML_ELEMENT_NODE)){
    NextElement = NextElement->next;
}
return(NextElement);
}


XdmfDOM::XdmfDOM(){
  this->NdgmHost = 0;
  this->Tree = NULL;
  this->Output = &cout;
  this->Input = &cin;
  this->Doc = NULL;
  this->DTD = 1;
  this->OutputFileName = 0;
  XDMF_STRING_DUPLICATE(this->OutputFileName, "stdout");
  this->SetFileName("stdin");
  this->SetNdgmHost( "" );
  this->SetWorkingDirectory( "" );
  // Allow Indenting on Serialization
  xmlIndentTreeOutput = 1;
  xmlKeepBlanksDefault(0);
  // Set Default Options
  this->ParserOptions = XML_PARSE_NOENT | XML_PARSE_XINCLUDE | XML_PARSE_NONET;
}

XdmfDOM::~XdmfDOM(){
   XdmfDebug("Destroying DOM");
  if( ( this->Output != &cout ) && ( this->Output != &cerr ) ) {
    ofstream *OldOutput = ( ofstream *)this->Output;
    OldOutput->close();
    delete OldOutput;
  }
  if( this->Input != &cin ) {
     XdmfDebug("Deleting Input");
    ifstream *OldInput = ( ifstream *)this->Input;
    OldInput->close();
    delete this->Input;
    this->Input = &cin;
  }
  this->SetNdgmHost(0);
  if ( this->OutputFileName ) {
    delete [] this->OutputFileName;
    }
    if(this->Doc) this->FreeDoc(this->Doc);
}

void XdmfDOM::FreePrivateData(XdmfXmlNode node)
{
	for(XdmfXmlNode currNode = node; currNode != NULL; currNode = currNode->next)
	{
		if(currNode->type == XML_ELEMENT_NODE)
		{
			delete (XdmfElementData*)currNode->_private;
		}
		FreePrivateData(currNode->children);
	}
}

void XdmfDOM::FreeDoc(XdmfXmlDoc doc)
{
	xmlNode* rootElement = xmlDocGetRootElement(doc);
	this->FreePrivateData(rootElement);
	xmlFreeDoc(doc);
  xmlCleanupParser();
}

XdmfInt32
XdmfDOM::GetNumberOfAttributes( XdmfXmlNode Node ){
XdmfInt32  NumberOfAttributes = 0;
xmlAttr *attr;

if(!Node) return(0);
attr = Node->properties;
while(attr){
    attr = attr->next;
    NumberOfAttributes++;
}
return( NumberOfAttributes );
}

XdmfConstString
XdmfDOM::GetAttributeName( XdmfXmlNode Node, XdmfInt32 Index ){
XdmfInt32  EIndex = 0;
xmlAttr *attr;

if(!Node) return(0);
attr = Node->properties;
while( attr && (EIndex < Index)){
    attr = attr->next;
    EIndex++;
}
if (attr) {
    return((XdmfConstString)attr->name);
}
return(NULL);
}

XdmfInt32
XdmfDOM::IsChild( XdmfXmlNode ChildToCheck, XdmfXmlNode Node) {
XdmfXmlNode child;

// Check All Children
for(child=Node->xmlChildrenNode; child ; child=child->next){
    if(child->type == XML_ELEMENT_NODE) {
        // Is this it?
        if(child == ChildToCheck) {
            return(XDMF_SUCCESS);
        }
        // Check Its children
        if(this->IsChild(ChildToCheck, child) == XDMF_SUCCESS){
            return(XDMF_SUCCESS);
        }
    }
}

return(XDMF_FAIL);
}

XdmfInt32
XdmfDOM::SetOutputFileName(XdmfConstString Filename){

  if( ( this->Output != &cout ) && ( this->Output != &cerr ) ) {
          ofstream *OldOutput = ( ofstream *)this->Output;
          OldOutput->close();
        }
  if( XDMF_WORD_CMP( Filename, "stdin" ) ) {
          this->Output = &cout;
  } else if( XDMF_WORD_CMP( Filename, "stderr" ) ) {
          this->Output = &cerr;
  } else {
          ofstream        *NewOutput = new ofstream( Filename );
          if( !NewOutput ) {
                  XdmfErrorMessage("Can't Open Output File " << Filename );
                  return( XDMF_FAIL );
                }
          this->Output = NewOutput;
        }
  if ( this->OutputFileName )
    {
    delete [] this->OutputFileName;
    }
  XDMF_STRING_DUPLICATE(this->OutputFileName, Filename);
  return( XDMF_SUCCESS );

}

XdmfInt32
XdmfDOM::SetInputFileName( XdmfConstString Filename ){

  if( this->Input != &cin ) {
    ifstream *OldInput = ( ifstream *)this->Input;
    OldInput->close();
    delete this->Input;
    this->Input = &cin;
  }
  if( XDMF_WORD_CMP( Filename, "stdin" ) ) {
    this->Input = &cin;
  } else {
    ifstream        *NewInput = new ifstream( Filename );
    if( !NewInput ) {
      XdmfErrorMessage("Can't Open Input File " << Filename );
      return( XDMF_FAIL );
    }
    this->Input = NewInput;
  }
  this->SetFileName(Filename);
  return( XDMF_SUCCESS );
}

XdmfInt32
XdmfDOM::GenerateHead() {
	if(this->DTD){
  	*this->Output << "<?xml version=\"1.0\" ?>" << endl 
        << "<!DOCTYPE Xdmf SYSTEM \"Xdmf.dtd\" []>" << endl ;
	} else {
  	*this->Output << "<?xml version=\"1.0\" ?>" << endl;
	} 
  this->Output->flush();
  return( XDMF_SUCCESS );
}

XdmfInt32
XdmfDOM::Puts( XdmfConstString String ){
  *this->Output << String;
  this->Output->flush();
  return( XDMF_SUCCESS );
}

XdmfInt32
XdmfDOM::GenerateTail() {
  this->Output->flush();
  return( XDMF_SUCCESS );
}

XdmfConstString
XdmfDOM::Serialize(XdmfXmlNode Node) {
int buflen;
xmlBufferPtr bufp;

if(!Node) Node = this->Tree;
if(!Node) return(NULL);
bufp = xmlBufferCreate();
buflen = xmlNodeDump(bufp, this->Doc, Node, 0, 1);
return(this->DupBuffer(bufp));
}

XdmfInt32 XdmfDOM::Write(XdmfConstString wOutput){
    XdmfConstString OldOutputFileName;

    if(wOutput){
        this->SetOutputFileName(wOutput);
        OldOutputFileName = this->GetOutputFileName();
    }
    if(!this->GenerateHead()) return(XDMF_FAIL);
    if(!this->Puts(this->Serialize())) return(XDMF_FAIL);
    if(wOutput){
            ofstream *OutputStr = ( ofstream *)this->Output;
            OutputStr->flush();
            OutputStr->close();
    }
    return(XDMF_SUCCESS);
}

XdmfXmlNode 
XdmfDOM::__Parse(XdmfConstString inxml, XdmfXmlDoc *DocPtr) {

XdmfXmlNode Root = NULL;
XdmfXmlDoc  pDoc;
int parserOptions;

parserOptions = this->ParserOptions;
if(inxml) {
    // Is  this XML or a File Name
    if(inxml[0] == '<'){
        // It's XML
        pDoc = xmlReadMemory(inxml, strlen(inxml), NULL, NULL, parserOptions);
    }else{
        // It's a File Name
        this->SetInputFileName(inxml);
        pDoc = xmlReadFile(this->GetInputFileName(), NULL, parserOptions);
    }
}else{
    pDoc = xmlReadFile(this->GetInputFileName(), NULL, parserOptions);
}
if(pDoc){
    if(parserOptions & XML_PARSE_XINCLUDE){
        if (xmlXIncludeProcess(pDoc) < 0) {
        	this->FreeDoc(pDoc);
            pDoc = NULL;
        }
    }
    Root = xmlDocGetRootElement(pDoc);
}
if(DocPtr) *DocPtr = pDoc;
return(Root);
}

XdmfInt32
XdmfDOM::Parse(XdmfConstString inxml) {

XdmfXmlNode Root;
XdmfXmlNode Node;
XdmfConstString  Attribute;

// Remove Previous Data
if(this->Doc) this->FreeDoc(this->Doc);
this->Tree = NULL;

Root = this->__Parse(inxml, &this->Doc);
if (Root) {
  this->Tree = Root;
} else {
  return(XDMF_FAIL);
  }

Node = this->FindElement("Xdmf", 0, NULL );
if( Node != NULL ){
  Attribute = this->Get( Node, "NdgmHost" );
  if( Attribute != NULL ){
    XdmfDebug("NdgmHost = " << Attribute );
    this->SetNdgmHost( Attribute );
    }
  Attribute = this->Get( Node, "WorkingDirectory" );
  if( Attribute != NULL ){
    XdmfDebug("WorkingDirectory = " << Attribute );
    this->SetWorkingDirectory( Attribute );
    }
  }
return( XDMF_SUCCESS );
}


XdmfInt32
XdmfDOM::DeleteNode(XdmfXmlNode Node) {
if(!Node) return(XDMF_FAIL);
xmlUnlinkNode(Node);
this->FreePrivateData(Node);
xmlFreeNode(Node);
return(XDMF_SUCCESS);
}

XdmfXmlNode
XdmfDOM::Create(XdmfConstString RootElementName, XdmfConstString Version){
    XdmfInt32   Status;
    ostrstream  XmlString;
    XdmfString  constructed;
    const xmlChar *XmlNs = XINCLUDE_NS;

    if(!Version) {
        Version = XDMF_VERSION_STRING;
    }
    XmlString << "<?xml version=\"1.0\" ?>";
    XmlString << "<" << RootElementName <<  " Version=\"" << Version << "\" xmlns:xi=\"" << XmlNs << "\" />" << ends;
    constructed = XmlString.str();
    Status = this->Parse(constructed);
    delete [] constructed;
    if(Status == XDMF_FAIL) return(NULL);
    return(this->GetRoot());
}

XdmfXmlNode
XdmfDOM::InsertFromString(XdmfXmlNode Parent, XdmfConstString inxml) {

XdmfXmlNode NewNode = NULL;
XdmfXmlDoc doc = NULL;
XdmfXmlNode root = NULL;
int parserOptions = this->ParserOptions;

doc = xmlReadMemory(inxml, strlen(inxml), NULL, NULL, parserOptions);
if(doc){
    root = xmlDocGetRootElement(doc);
    NewNode = root;
}
if(NewNode){
    XdmfXmlNode Child;
    Child = this->Insert(Parent, NewNode);
    this->FreeDoc(doc);
    return(Child);
}
return(NULL);
}

XdmfXmlNode
XdmfDOM::Insert(XdmfXmlNode Parent, XdmfXmlNode Child) {

XdmfXmlNode ChildCopy;

if(Parent && Child){
    if(Parent->doc == Child->doc){
        XdmfDebug("Docs are same : Don't Copy Child");
        ChildCopy = Child;
    }else{
        XdmfDebug("Docs are different : Copy Child");
        ChildCopy = xmlCopyNodeList(Child);
    }
    if(xmlAddChildList(Parent, ChildCopy)){
        return(ChildCopy);
    }
}
return(NULL);
}

XdmfXmlNode
XdmfDOM::InsertNew(XdmfXmlNode Parent, XdmfConstString Type) {

XdmfXmlNode Child;

if(Parent){
    Child = xmlNewNode(NULL, (const xmlChar *)Type);
    if(Child) {
        XdmfXmlNode RealChild;
        RealChild = xmlAddChildList(Parent, Child);
        if(RealChild){
            return(RealChild);
        }
        this->FreePrivateData(Child);
        xmlFreeNode(Child);
    }
}
return(NULL);
}

XdmfXmlNode
XdmfDOM::GetChild( XdmfInt64 Index, XdmfXmlNode Node ){
XdmfXmlNode child;

if(!Node){
    Node = this->Tree;
}
if(!Node) return(0);
child = Node->children;
if(Index == 0){
    if(child->type != XML_ELEMENT_NODE){
        child = XdmfGetNextElement(child);
    }
}
while(child && Index){
    child = XdmfGetNextElement(child);
    Index--;
}
return(child);
}


XdmfInt64
XdmfDOM::GetNumberOfChildren( XdmfXmlNode Node ){
XdmfInt64 Index = 0;
XdmfXmlNode child;

if(!Node){
    Node = this->Tree;
}
if(!Node) return(0);
child = Node->children;
while(child){
    if(child->type == XML_ELEMENT_NODE) Index++;
    child = XdmfGetNextElement(child);
}
return(Index);
}

XdmfXmlNode  
XdmfDOM::GetRoot( void ) {
return(this->Tree);
}

XdmfXmlNode  
XdmfDOM::FindDataElement(XdmfInt32 Index, XdmfXmlNode Node, XdmfInt32 IgnoreInfo) {
XdmfXmlNode child;

if(!Node) {
    if(!this->Tree) return( NULL );
    Node = this->Tree;
}
child = Node->children;
if(!child) return(NULL);
while(child){
    if(IgnoreInfo && XDMF_WORD_CMP("Information", (const char *)(child)->name)){
        child = XdmfGetNextElement(child);
    }else{
        if(XDMF_WORD_CMP("DataItem", (const char *)(child)->name) ||
            XDMF_WORD_CMP("DataStructure", (const char *)(child)->name) ||
            XDMF_WORD_CMP("DataTransform", (const char *)(child)->name)
            ){
            if(Index <= 0){
                return(child);
            }
            Index--;
        }
        child = XdmfGetNextElement(child);
    }
}
return(NULL);
}

XdmfXmlNode  
XdmfDOM::FindElement(XdmfConstString TagName, XdmfInt32 Index, XdmfXmlNode Node, XdmfInt32 IgnoreInfo) {

XdmfString type = (XdmfString )TagName;
XdmfXmlNode child;

// this->SetDebug(1);
if(TagName){
    XdmfDebug("FindElement " << TagName << " Index = " << Index);
}else{
    XdmfDebug("FindElement NULL Index = " << Index);
}
if(!Node) {
    if(!this->Tree) return( NULL );
    Node = this->Tree;
}
child = Node->children;
if(!child) return(NULL);
if ( type ) {
  if( STRNCASECMP( type, "NULL", 4 ) == 0 ) type = NULL;
}
if ( !type ) {
    if(IgnoreInfo){
        while(child){
            if(XDMF_WORD_CMP("Information", (const char *)(child)->name) == 0){
                if(Index <= 0){
                    return(child);
                }
                Index--;
            }
            child = XdmfGetNextElement(child);
        }
    }else{
        return(this->GetChild(Index, Node));
    }
} else {
    while(child){
        if(IgnoreInfo && XDMF_WORD_CMP("Information", (const char *)(child)->name)){
            child = XdmfGetNextElement(child);
        }else{
            if(XDMF_WORD_CMP((const char *)type, (const char *)(child)->name)){
                if(Index <= 0){
                    return(child);
                }
                Index--;
            }
            child = XdmfGetNextElement(child);
        }
    }
}
return(NULL);
}


XdmfXmlNode  
XdmfDOM::FindNextElement(XdmfConstString TagName, XdmfXmlNode Node, XdmfInt32 IgnoreInfo) {

XdmfString type = (XdmfString )TagName;
XdmfXmlNode child;

// this->SetDebug(1);
if(TagName){
    XdmfDebug("FindNextElement" << TagName);
}else{
    XdmfDebug("FindNextElement NULL");
}
if(!Node) {
    if(!this->Tree) return( NULL );
    Node = this->Tree->children;
}
if(!Node) return(NULL);
if ( type ) {
  if( STRNCASECMP( type, "NULL", 4 ) == 0 ) type = NULL;
}

child = XdmfGetNextElement(Node);
while (child) {
  if(IgnoreInfo && XDMF_WORD_CMP("Information", (const char *)(child)->name)){
    // skip Information elements.
  } else {
    if(!type ||
      (XDMF_WORD_CMP((const char *)type, (const char *)(child)->name))){
      return child;
    }
  }
  child = XdmfGetNextElement(child);
}
return (NULL);
}

XdmfXmlNode  
XdmfDOM::FindElementByAttribute(XdmfConstString Attribute,
    XdmfConstString Value, XdmfInt32 Index, XdmfXmlNode Node ) {
XdmfXmlNode child;

if( !Node) {
  Node = this->Tree;
}
if( !Node ) return( NULL );
child = Node->children;
while(child){
    xmlChar *txt = xmlGetProp(child, (xmlChar *)Attribute);
    if(XDMF_WORD_CMP((const char *)txt, (const char *)Value)){
        if(Index <= 0){
            xmlFree(txt);
            return(child);
        }
        xmlFree(txt);
        Index--;
    }
    child = XdmfGetNextElement(child);
}
return(NULL);
}

XdmfXmlNode 
XdmfDOM::FindElementByPath(XdmfConstString Path){
    // Use an XPath expression to return a Node
    xmlXPathContextPtr xpathCtx;
    xmlXPathObjectPtr xpathObj;
    xmlNodeSetPtr nodes;
    XdmfXmlNode child = NULL;
    int i;

    if(!this->Doc){
        XdmfErrorMessage("XML must be parsed before XPath is available");
        return(NULL);
    }
    // Create the context
    xpathCtx = xmlXPathNewContext(this->Doc);
    if(xpathCtx == NULL){
        XdmfErrorMessage("Can't Create XPath Context");
        return(NULL);
    }
    xpathObj = xmlXPathEvalExpression((const xmlChar *)Path, xpathCtx);
    if(xpathObj == NULL){
        XdmfErrorMessage("Can't evaluate XPath : " << Path);
        return(NULL);
    }
    // Return the first XML_ELEMENT_NODE
    nodes = xpathObj->nodesetval;
    if(!nodes){
        XdmfErrorMessage("No Elements Match XPath Expression : " << Path);
        return(NULL);
    }
    XdmfDebug("Found " << nodes->nodeNr << " Element that match XPath expression " << Path);
    for(i=0 ; i < nodes->nodeNr ; i++){
        child = nodes->nodeTab[i];
        if(child->type == XML_ELEMENT_NODE){
            // this is it
            xmlXPathFreeObject(xpathObj);
            xmlXPathFreeContext(xpathCtx);
            return(child);
        }
    }
xmlXPathFreeObject(xpathObj);
xmlXPathFreeContext(xpathCtx);
return(NULL);
}

XdmfConstString
XdmfDOM::GetPath(XdmfXmlNode Node){
    char *txt;

    if(!Node){
        XdmfErrorMessage("Node == NULL");
        return((XdmfConstString)NULL);
    }
    txt = (char *)xmlGetNodePath(Node);
    return(this->DupChars(txt));
}

XdmfInt32
XdmfDOM::FindNumberOfElements(XdmfConstString TagName, XdmfXmlNode Node ) {
XdmfXmlNode child;
XdmfInt32 Index = 0;

if( !Node ) {
    if(!this->Tree) return(XDMF_FAIL);
    Node = this->Tree;
}
child = Node->children;
if(!child) return(0);
while(child){
    if(XDMF_WORD_CMP(TagName, (const char *)child->name)){
        Index++;
    }
    child = XdmfGetNextElement(child);
}
return(Index);
}

XdmfInt32
XdmfDOM::FindNumberOfElementsByAttribute(XdmfConstString Attribute, 
    XdmfConstString Value, XdmfXmlNode Node ) {
XdmfInt32 NElements = 0;
XdmfXmlNode child;

if( !Node) {
  Node = this->Tree;
}
if(!Node) return(0);
child = Node->children;
while(child){
    xmlChar *txt;
    txt = xmlGetProp(child, (xmlChar *)Attribute);
    if(XDMF_WORD_CMP((const char *)txt, (const char *)Value)){
        NElements++;
    }
    xmlFree(txt);
    child = XdmfGetNextElement(child);
}
return(0);
}

XdmfConstString
XdmfDOM::GetAttribute(XdmfXmlNode Node, XdmfConstString Attribute) {

if(!Node) {
    Node = this->Tree;
}
if(!Node) return(0);
/*
cout << "For " << Attribute << " DOM Returning ";
if((XdmfConstString)xmlGetProp(Node, (xmlChar *)Attribute)) {
    cout << (XdmfConstString)xmlGetProp(Node, (xmlChar *)Attribute) << endl;
    } else{
        cout << " NULL " << endl;
    }
    */
return((XdmfConstString)xmlGetProp(Node, (xmlChar *)Attribute));
}

XdmfConstString
XdmfDOM::GetCData(XdmfXmlNode Node) {

char    *txt;

if( !Node ) {
    Node = this->Tree;
}
if(!Node) return(0);
txt = (char *)xmlNodeListGetString(this->Doc, Node->xmlChildrenNode, 1);
return(this->DupChars(txt));
}

XdmfConstString
XdmfDOM::Get(XdmfXmlNode Node, XdmfConstString Attribute) {

if(!Node) return(0);
if( STRNCASECMP( Attribute, "CDATA", 5 ) == 0 ){
    return(this->GetCData(Node));
}
return(this->GetAttribute(Node, Attribute));
}

void
XdmfDOM::Set( XdmfXmlNode Node, XdmfConstString Attribute, XdmfConstString Value ){

if(!Node) return;
if( STRNCASECMP( Attribute, "CDATA", 5 ) == 0 ){
    XdmfXmlNode  child, next, text;

    // cout << "Setting CDATA to " << Value << endl;
    // Delete Existing CData
    child = Node->children;
    while(child){
        // cout << "Checking Node of type " << Node->type << endl;
        next = child->next;
        if ((child->type == XML_TEXT_NODE) ||
            (child->type == XML_CDATA_SECTION_NODE)) {
            // cout << "Deleting Node" << endl;
            xmlUnlinkNode(child);
            this->FreePrivateData(child);
            xmlFreeNode(child);
        }
       child = next;
    }
    // cout << "Adding Node" << endl;
    text = xmlNewDocText(this->Doc, (const xmlChar *)Value);
    xmlAddChildList(Node, text);
}else{
    if(Value){
        xmlSetProp(Node, (xmlChar *)Attribute, (xmlChar *)Value);
    }else{
        xmlUnsetProp(Node, (xmlChar *)Attribute);
    }
}
}

