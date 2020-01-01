/*********************************************************************
 *   Copyright 2018, UCAR/Unidata
 *   See netcdf/COPYRIGHT file for copying and redistribution conditions.
 *********************************************************************/

/**
   This provides a simple netcdf-4 metadata -> xml printer.
   Primarily for use in debugging, but could be adapted to
   create other tools.
*/

#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "netcdf.h"
#include "ncbytes.h"
#include "nclist.h"

#undef DEBUG

#define BUFSIZE 4096

#define NC_MAX_IDS 8192

typedef enum NCSORT {
    GROUP,
    VAR,
    FIELD,
    DIM,
    ATTR,
    ATOMTYPE,
    USERTYPE,
} NCSORT;

typedef struct NCID NCID;

typedef struct NC4printer {
    NCbytes* out;
    NClist* types;
    NClist* dims;
    NClist* allnodes;
    NCbytes* tmp1;
    NCbytes* tmp2;
} NC4printer;

struct NCID {
    NCSORT sort;
    struct NCID* parent;
    int id;
    char name[NC_MAX_NAME+1];
    NCID* base;
    size_t size;
    struct {nc_type kind;} usertype; /*sort == USERTYPE*/
    struct {int rank;} var; /*sort == VAR*/
    struct {int fid;} field; /*sort == FIELD*/
    struct {int isroot;} group; /*sort == GROUP*/
};

#define MAKEID(Node,Sort,Parent,Id)                             \
    NCID* Node = (NCID*)calloc(1,sizeof(NCID));                 \
    Node->sort = Sort; Node->parent = Parent; Node->id = Id;    \
    track(out,Node);

union NUMVALUE {
    unsigned char i8[8];
    unsigned short i16[4];
    unsigned short i32[2];
    unsigned long long i64[1];
};

#define SETNAME(x,y) strncpy((x)->name,(y),NC_MAX_NAME+1);
#define GRPIDFOR(gid) ((gid) & 0xFFFF)
#define GROUPOF(x) ((x)->parent->id)

#define FAIL {return ret;}

#define PRINTF(fmt,...) snprintf(out->buf,BUFSIZE,fmt,__VAR_ARGS__)
#define CAT(x) ncbytescat(out->out,x)
#define INDENT(x) indent(out,x)

#define hasMetadata(node) (nclistlength(node->attributes) > 0)
#define hasMaps(var) (nclistlength(node->maps) > 0)
#define hasDimensions(var) (nclistlength(node->dimrefs) > 0)

static void track(NC4printer* out, NCID* node);

/*Forward*/
static int buildAtomicTypes(NC4printer* out, NCID* root);
static void* computeOffset(NCID* base, void* values, size_t index);
static void entityEscape(NCbytes* buf, const char* s);
static NCID* findDim(NC4printer* out, int dimid);
static NCID* findType(NC4printer* out, nc_type t);
static void fqnWalk(NCID* grp, NCbytes* path);
static void freeNC4Printer(NC4printer* out);
static void getAtomicTypeName(nc_type base, char* name);
static int getPrintValue(NCbytes* out, NCID* basetype, void* value);
static void indent(NC4printer* out, int depth);
static unsigned long long getNumericValue(union NUMVALUE numvalue, nc_type base);
static void makeFQN(NCID* id, NCbytes* path);
static int printAttribute(NC4printer* out, NCID* attr, int depth);
static int printDimref(NC4printer* out, NCID* dim, int depth);;
static int printNode(NC4printer* out, NCID* node, int depth);
static void printOpaque(NCbytes* out, const unsigned char* s, size_t len, int leadx);
static void printString(NCbytes* out, const char* s, int quotes);
static int printValue(NC4printer* out, NCID* basetype, void* value, int depth);
static int printXMLAttributeInt(NC4printer* out, char* name, long long value);
static int printXMLAttributeName(NC4printer* out, char* name, char* value);
static int printXMLAttributeSize(NC4printer* out, char* name, size_t value);
static int printXMLAttributeString(NC4printer* out, char* name, char* s);
static int readAttributeValues(NCID* attr,void**);
static void record(NC4printer* out, NCID* node);

/**************************************************/

int
NC4print(NCbytes* buf, int ncid)
{
    int ret = NC_NOERR;
    NC4printer* out;

    if(buf == NULL) return NC_EINVAL;
    out = (NC4printer*)calloc(1,sizeof(NC4printer));
    if(out == NULL) return NC_ENOMEM;
    out->out = buf;
    out->tmp1 = ncbytesnew();
    out->tmp2 = ncbytesnew();
    out->allnodes = nclistnew();
    out->types = nclistnew();
    out->dims = nclistnew();

    MAKEID(root,GROUP,NULL,ncid);
    root->group.isroot = 1;

    buildAtomicTypes(out,root);

    ret = printNode(out,root,0);

    freeNC4Printer(out);

    return ret;
}

/*************************************************/

static void
freeNC4Printer(NC4printer* out)
{
    int i;

    if(out == NULL) return;

#ifdef DEBUG
    fprintf(stderr,"free: |allnodes=%ld\n",nclistlength(out->allnodes));
    fflush(stderr);
#endif

    for(i=0;i<nclistlength(out->allnodes);i++) {
        NCID* node = (NCID*)nclistget(out->allnodes,i);
#ifdef DEBUG
        fprintf(stderr,"free: node=%lx\n",(unsigned long)node);
        fflush(stderr);
#endif
        if(node != NULL) free(node);
    }

    ncbytesfree(out->tmp1);
    ncbytesfree(out->tmp2);
    nclistfree(out->types);
    nclistfree(out->dims);
    nclistfree(out->allnodes);

    free(out);
}


/*************************************************/

/**
 * Print an arbitrary file and its subnodes in xml
 * Handling newlines is a bit tricky because they may be
 * embedded for e.g. groups, enums,
 * etc.  So the rule is that the
 * last newline is elided and left
 * for the caller to print.
 * Exceptions: printMetadata
 * printDimrefs.
 *
 * @param out - the output buffer
 * @param ncid - the open'd file to print
 * @param depth - the depth of our code
 */

static int
printNode(NC4printer* out, NCID* node, int depth)
{
    int ret = NC_NOERR;
    int i = 0;
    char name[NC_MAX_NAME+1];
    int ndims, nvars, natts, nunlim, ntypes, ngroups;
    int n;
    int ids[NC_MAX_IDS];
    nc_type base;
    size_t len, count, size;
    union NUMVALUE numvalue;

    switch (node->sort) {
    case GROUP:
        /* Get group name */
        if((ret=nc_inq_grpname(node->id,name))) FAIL;
        SETNAME(node,name);
        /* get group counts */
        if((ret=nc_inq(node->id,&ndims,&nvars,&natts,&nunlim))) FAIL;
        if((ret=nc_inq_typeids(node->id,&ntypes,NULL))) FAIL;
        if((ret=nc_inq_grps(node->id,&ngroups,NULL))) FAIL;
        if(ndims >= NC_MAX_IDS) FAIL;
        if(nvars >= NC_MAX_IDS) FAIL;
        if(nunlim >= NC_MAX_IDS) FAIL;
        if(ntypes >= NC_MAX_IDS) FAIL;
        if(ngroups >= NC_MAX_IDS) FAIL;

        INDENT(depth);
        CAT("<Group");
        printXMLAttributeName(out,"name",name);
        CAT(">\n");
        depth++;

        {
            /* Print: dims, types, vars(+attr), group-attr, subgroups */
            if((ret=nc_inq_dimids(node->id,&n,ids, 0))) FAIL;
            for(i=0;i<ndims;i++) {
                MAKEID(eid,DIM,node,ids[i]);
                printNode(out,eid,depth);
                CAT("\n");
                record(out,eid);
            }
        }
        {
            if((ret=nc_inq_typeids(node->id,&n,ids))) FAIL;
            for(i=0;i<ntypes;i++) {
                nc_type kind;
                if((ret=nc_inq_user_type(node->id, ids[i], name, &size, &base, NULL, &kind))) FAIL;
                MAKEID(eid,USERTYPE,node,ids[i]);
                SETNAME(eid,name);
                eid->size = size;
                eid->usertype.kind = kind;
                if(base > 0) eid->base = findType(out,base);
                record(out,eid);
                printNode(out,eid,depth);
                CAT("\n");
            }
        }
        {
            if((ret=nc_inq_varids(node->id,&n,ids))) FAIL;
            for(i=0;i<nvars;i++) {
                nc_type base;
                if((ret=nc_inq_var(node->id, ids[i], name, &base, &ndims, NULL, NULL))) FAIL;
                MAKEID(vid,VAR,node,ids[i]);
                SETNAME(vid,name);
                vid->base = findType(out,base);
                vid->var.rank = ndims;
                printNode(out,vid,depth);
                CAT("\n");
            }
        }
        {
            for(i=0;i<natts;i++) {
                if((ret=nc_inq_attname(node->id,NC_GLOBAL,i,name))) FAIL;
                MAKEID(id,ATTR,node,NC_GLOBAL);
                SETNAME(id,name);
                printAttribute(out,id,depth);
                CAT("\n");
            }
        }

        {
            if((ret=nc_inq_grps(node->id,&n,ids))) FAIL;
            for(i=0;i<ngroups;i++) {
                MAKEID(id,GROUP,node,ids[i]);
                printNode(out,id,depth);
                CAT("\n");
                record(out,id);
            }
        }
        depth--;
        INDENT(depth); CAT("</Group>");
        break;

    case DIM:
        if((ret=nc_inq_dim(GROUPOF(node),node->id,name,&len))) FAIL;
        SETNAME(node,name);
        node->size = len;
        INDENT(depth);
        CAT("<Dimension");
        printXMLAttributeName(out, "name", name);
        printXMLAttributeSize(out, "size", len);
        CAT("/>");
        break;

    case USERTYPE:
        switch (node->usertype.kind) {
        case NC_OPAQUE:
            INDENT(depth); CAT("<Opaque");
            printXMLAttributeName(out, "name", node->name);
            printXMLAttributeSize(out, "size", node->size);
            CAT("/>");
            break;
        case NC_ENUM:
            if((ret=nc_inq_enum(GROUPOF(node),node->id,NULL,NULL,NULL,&count))) FAIL;
            INDENT(depth); CAT("<Enumeration");
            printXMLAttributeName(out, "name", node->name);
            CAT(">\n");
            depth++;
            for(i=0;i<count;i++) {
                long long value;
                if((ret=nc_inq_enum_member(GROUPOF(node),node->id,i,name,&numvalue))) FAIL;
                value = getNumericValue(numvalue,node->base->id);
                INDENT(depth);
                CAT("<EnumConst");
                printXMLAttributeName(out, "name", name);
                printXMLAttributeInt(out, "value", value);
                CAT("/>\n");
            }
            depth--;
            INDENT(depth); CAT("</Enumeration>");
            break;

        case NC_COMPOUND:
            if((ret=nc_inq_compound(GROUPOF(node),node->id,NULL,NULL,&count))) FAIL;
            INDENT(depth); CAT("<Compound");
            printXMLAttributeName(out, "name", node->name);
            CAT(">\n");
            depth++;
            for(i=0;i<count;i++) {
                if((ret=nc_inq_compound_field(GROUPOF(node),node->id,i,name,NULL,&base,NULL,NULL))) FAIL;
                MAKEID(id,FIELD,node->parent,node->id);
                SETNAME(id,name);
                id->base = findType(out,base);
                id->field.fid = i;
                printNode(out,id,depth);
                CAT("\n");
            }
            depth--;
            INDENT(depth); CAT("</Compound>");
            break;
        case NC_VLEN:
            abort();
            break;
        default:
            abort();
            break;
        }
        break;

    case VAR:
        if((ret=nc_inq_var(GROUPOF(node), node->id, name, &base, &ndims, ids, &natts))) FAIL;
        node->base = findType(out,base);
        SETNAME(node,name);
        node->var.rank = ndims;
        INDENT(depth); CAT("<Var");
        printXMLAttributeName(out, "name", node->name);
        makeFQN(node->base,out->tmp2);
        printXMLAttributeName(out, "type", ncbytescontents(out->tmp2));
        if(node->var.rank > 0)
            printXMLAttributeInt(out, "rank", node->var.rank);
        if(ndims > 0 || natts > 0) {
            CAT(">\n");
            depth++;
            for(i=0;i<ndims;i++) {
                NCID* dim = findDim(out,ids[i]);
                printDimref(out,dim,depth);
                CAT("\n");
            }
            for(i=0;i<natts;i++) {
                if((ret=nc_inq_attname(GROUPOF(node),node->id,i,name))) FAIL;
                if((ret=nc_inq_att(GROUPOF(node),node->id,name,&base,&count))) FAIL;
                MAKEID(id,ATTR,node,node->id);
                SETNAME(id,name);
                id->base = findType(out,base);
                id->size = count;
                printAttribute(out,id,depth);
                CAT("\n");
            }
            depth--;
            INDENT(depth); CAT("</Var>");
        } else
            CAT("/>");
        break;

    case ATOMTYPE:
    default:
        abort();
        ret = NC_EINVAL;
        break;
    }
    return ret;
}

static int
printXMLAttributeName(NC4printer* out, char* name, char* value)
{
    int ret = NC_NOERR;
    if(name == NULL) return ret;
    CAT(" "); CAT(name); CAT("=\"");
    if(value == NULL) value = "";
    /* add xml entity escaping */
    entityEscape(out->tmp1,value);
    CAT(ncbytescontents(out->tmp1));
    CAT("\"");
    return ret;
}

static int
printXMLAttributeSize(NC4printer* out, char* name, size_t value)
{
    return printXMLAttributeInt(out,name,(long long)value);
}

static int
printXMLAttributeInt(NC4printer* out, char* name, long long value)
{
    int ret = NC_NOERR;
    char svalue[128+1];

    CAT(" "); CAT(name); CAT("=\"");
    snprintf(svalue,sizeof(svalue),"%lld",value);
    CAT(svalue);
    CAT("\"");
    return ret;
}

static int
printXMLAttributeString(NC4printer* out, char* name, char* s)
{
    int ret = NC_NOERR;
    CAT(" "); CAT(name); CAT("=");
    printString(out->out,s,1);
    return ret;
}

static int
printAttribute(NC4printer* out, NCID* attr, int depth)
{
    int ret = NC_NOERR;
    int i = 0;
    void* values;

    INDENT(depth); CAT("<Attribute");
    printXMLAttributeName(out,"name",attr->name);
    CAT(">\n");
    if((ret=readAttributeValues(attr,&values))) FAIL;
    depth++;
    for(i=0;i<attr->size;i++) {
        void* value = computeOffset(attr->base,values,i);
        if((ret=printValue(out,attr->base,value,depth))) FAIL;
    }
    depth--;
    INDENT(depth);
    CAT("</Attribute>");
    return ret;
}

/**
 * Print the dimrefs for a variable's dimensions.
 * If the variable has a non-whole projection, then use size
 * else use the dimension name.
 *
 * @param var whole dimensions are to be printed
 * @throws DapException
 */
static int
printDimref(NC4printer* out, NCID* d, int depth)
{
    INDENT(depth);
    CAT("<Dim");
    makeFQN(d,out->tmp2);
    printXMLAttributeName(out, "name", ncbytescontents(out->tmp2));
    CAT("/>");
    return NC_NOERR;
}

static int
printValue(NC4printer* out, NCID* basetype, void* value, int depth)
{
    int ret;
    if(basetype->id > NC_MAX_ATOMIC_TYPE && basetype->usertype.kind == NC_ENUM) {
        basetype = basetype->base;
    }
    if((ret=getPrintValue(out->tmp2,basetype,value))) FAIL;
    INDENT(depth);
    CAT("<Value");
    printXMLAttributeString(out,"value",ncbytescontents(out->tmp2));
    CAT("/>\n");
    return ret;
}

/*************************************************/
/* Misc. Static Utilities */

/* Make public to allow use elsewhere */

static const char hexchars[16] = "0123456789abcdef";

static int
getPrintValue(NCbytes* out, NCID* basetype, void* value)
{
    int ret = NC_NOERR;
    char buf[256];
    ncbytesclear(out);
    switch (basetype->id) {
    case NC_CHAR:
        snprintf(buf,sizeof(buf),"'%c'",*(char*)value);
        ncbytescat(out,buf);
        break;
    case NC_BYTE:
        snprintf(buf,sizeof(buf),"%d",*(char*)value);
        ncbytescat(out,buf);
        break;
    case NC_UBYTE:
        snprintf(buf,sizeof(buf),"%u",*(unsigned char*)value);
        ncbytescat(out,buf);
        break;
    case NC_SHORT:
        snprintf(buf,sizeof(buf),"%d",*(short*)value);
        ncbytescat(out,buf);
        break;
    case NC_USHORT:
        snprintf(buf,sizeof(buf),"%u",*(unsigned short*)value);
        ncbytescat(out,buf);
        break;
    case NC_INT:
        snprintf(buf,sizeof(buf),"%d",*(int*)value);
        ncbytescat(out,buf);
        break;
    case NC_UINT:
        snprintf(buf,sizeof(buf),"%u",*(unsigned int*)value);
        ncbytescat(out,buf);
        break;
    case NC_INT64:
        snprintf(buf,sizeof(buf),"%lld",*(long long*)value);
        ncbytescat(out,buf);
        break;
    case NC_UINT64:
        snprintf(buf,sizeof(buf),"%llu",*(unsigned long long*)value);
        ncbytescat(out,buf);
        break;
    case NC_FLOAT:
        snprintf(buf,sizeof(buf),"%g",*(float*)value);
        ncbytescat(out,buf);
        break;
    case NC_DOUBLE:
        snprintf(buf,sizeof(buf),"%g",*(double*)value);
        ncbytescat(out,buf);
        break;
    case NC_STRING: {
        char* s = *(char**)value;
        printString(out,s,0);
    } break;
    case NC_OPAQUE: {
        unsigned char* s = *(unsigned char**)value;
        printOpaque(out,s,basetype->size,1);
    } break;
    case NC_ENUM:
        /* use true basetype */
        ret = getPrintValue(out,basetype->base,value);
        break;
    default:
        break;
    }
    return ret;
}

static void
getAtomicTypeName(nc_type base, char* name)
{
    const char* tname = NULL;
    switch (base) {
    case NC_BYTE: tname = "Byte"; break;
    case NC_UBYTE: tname = "UByte"; break;
    case NC_SHORT: tname = "Short"; break;
    case NC_USHORT: tname = "UShort"; break;
    case NC_INT: tname = "Int"; break;
    case NC_UINT: tname = "UInt"; break;
    case NC_FLOAT: tname = "Float"; break;
    case NC_DOUBLE: tname = "Double"; break;
    case NC_INT64: tname = "Int64"; break;
    case NC_UINT64: tname = "UInt64"; break;
    case NC_STRING: tname = "String"; break;
    default: tname = ""; break;
    }
    strncpy(name,tname,strlen(tname)+1);
}

static void
indent(NC4printer* out, int depth)
{
    while(depth-- >= 0) ncbytescat(out->out,"  ");
}

static unsigned long long
getNumericValue(union NUMVALUE numvalue, nc_type base)
{
    switch (base) {
    case NC_CHAR: case NC_BYTE: return numvalue.i8[0];
    case NC_SHORT: case NC_USHORT: return numvalue.i16[0];
    case NC_INT: case NC_UINT: return numvalue.i32[0];
    case NC_INT64: case NC_UINT64: return numvalue.i64[0];
    }
    return NC_MAX_UINT64;
}

static NCID*
findType(NC4printer* out, nc_type t)
{
    int len = nclistlength(out->types);
    if(t >= len)
        abort();
    return (NCID*)nclistget(out->types,t);
}

static NCID*
findDim(NC4printer* out, int dimid)
{
    if(nclistlength(out->dims) <= dimid) abort();
    return (NCID*)nclistget(out->dims,dimid);
}

static void
makeFQN(NCID* id, NCbytes* path)
{
    NCID* g = id;
    ncbytesclear(path);
    if(id->sort != GROUP)
        g = id->parent;
    if(!g->group.isroot)
        fqnWalk(g,path);
    ncbytesappend(path,'/');
    if(id->sort != GROUP)
        ncbytescat(path,id->name);
    ncbytesnull(path);
}

static void
fqnWalk(NCID* grp, NCbytes* path)
{
    if(grp->id != 0) {
        NCID* parent = grp->parent;
        fqnWalk(parent,path);
        ncbytesappend(path,'/');
        ncbytescat(path,parent->name);
    }
}

static void
record(NC4printer* out, NCID* node)
{
    switch (node->sort) {
    case DIM:
        if(nclistlength(out->dims) <= node->id) {
            nclistsetalloc(out->dims,node->id+1);
            nclistsetlength(out->dims,node->id+1);
        }
        nclistset(out->dims,node->id,node);
        break;
    case ATOMTYPE:
    case USERTYPE:
        if(nclistlength(out->types) <= node->id) {
            nclistsetalloc(out->types,node->id+1);
            nclistsetlength(out->types,node->id+1);
        }
        nclistset(out->types,node->id,node);
        break;
    default: break;
    }
}

static void
track(NC4printer* out, NCID* node)
{
    if(out == NULL || node == NULL || out->allnodes == NULL)
        abort();
#ifdef DEBUG
    fprintf(stderr,"track: node=%lx\n",(unsigned long)node);
#endif
    nclistpush(out->allnodes,node);
#ifdef DEBUG
    fprintf(stderr,"track: |allnodes|=%ld\n",nclistlength(out->allnodes));
    fflush(stderr);
#endif
}

static void
entityEscape(NCbytes* escaped, const char* s)
{
    const char* p;
    ncbytesclear(escaped);
    for(p=s;*p;p++) {
        int c = *p;
        switch (c) {
        case '&':  ncbytescat(escaped,"&amp;"); break;
        case '<':  ncbytescat(escaped,"&lt;"); break;
        case '>':  ncbytescat(escaped,"&gt;"); break;
        case '"':  ncbytescat(escaped,"&quot;"); break;
        case '\'': ncbytescat(escaped,"&apos;"); break;
        default:   ncbytesappend(escaped,(c)); break;
        }
        ncbytesnull(escaped);
    }
}

static int
buildAtomicTypes(NC4printer* out, NCID* root)
{
    int ret = NC_NOERR;
    nc_type tid;
    char name[NC_MAX_NAME+1];
    size_t size;
    for(tid=NC_NAT+1;tid<=NC_MAX_ATOMIC_TYPE;tid++) {
        if((ret=nc_inq_type(root->id,tid,NULL,&size))) FAIL;
        getAtomicTypeName(tid,name);
        MAKEID(type,ATOMTYPE,root,tid);
        SETNAME(type,name);
        type->size = size;
        type->usertype.kind = tid;
        record(out, type);
    }
    return ret;
}

static void
printString(NCbytes* out, const char* s, int quotes)
{
    const char* p;
    if(quotes) ncbytesappend(out,'"');
    if(s == NULL) s = "";
    for(p=s;*p;p++) {
        int c = *p;
        if(c == '\\') ncbytescat(out,"\\\\");
        else if(c == '"') ncbytescat(out,"\\\"");
        else ncbytesappend(out,c);
    }
    if(quotes) ncbytesappend(out,'"');
    ncbytesnull(out);
}

static void
printOpaque(NCbytes* out, const unsigned char* s, size_t len, int leadx)
{
    int i;
    char digit;
    if(s == NULL) {s = (unsigned char*)""; len = 1;}
    if(leadx) ncbytescat(out,"0x");
    for(i=0;i<len;i++) {
        unsigned int c = s[i];
        digit = hexchars[(c>>4) & 0xF];
        ncbytesappend(out,digit);
        digit = hexchars[c & 0xF];
        ncbytesappend(out,digit);
    }
    ncbytesnull(out);
}

static void*
computeOffset(NCID* base, void* values, size_t index)
{
    unsigned char* p = (unsigned char*)values; /*so we can do arithmetic */
    return (void*)(p + ((base->size)*index));
}

static int
readAttributeValues(NCID* attr, void** valuesp)
{
    int ret;
    void* values = NULL;
    NCID* var = attr->parent;
    NCID* base = attr->base;
    size_t len;

    len = base->size * attr->size;
    values = malloc(len);
    if(values == NULL) return NC_ENOMEM;
    if((ret=nc_get_att(GROUPOF(var),var->id,attr->name,values))) FAIL;
    if(valuesp) *valuesp = values;
    return ret;
}
