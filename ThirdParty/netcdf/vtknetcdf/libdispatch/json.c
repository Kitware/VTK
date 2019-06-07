/* Copyright 2018, UCAR/Unidata.
   See the COPYRIGHT file for more information.
*/

#ifndef NCJSON_INC
#define NCJSON_INC 1

#define NCJ_DICT     1
#define NCJ_LIST     2
#define NCJ_WORD     3
#define NCJ_NUMBER   4
#define NCJ_BOOLEAN  5

/* Don't bother with unions */
typedef struct NCjson {
    long sort;
    char* word; /* string or (!boolean && !number) */
    long long num; /* number || boolean (0=>false; !0=>true)*/
    NCjson* list;
} NCjson;

#define NCJ_LBRACKET '['
#define NCJ_RBRACKET ']'
#define NCJ_LBRACE '{'
#define NCJ_RBRACE '}'
#define NCJ_COLON ':'
#define NCJ_COMMA ','
#define NCJ_QUOTE '"'
#define NCJ_TRUE "true"
#define NCJ_FALSE "false"

#define NCJ_WORD "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-$"

/*//////////////////////////////////////////////////*/

typedef struct NCJparser {
    char* text;
    char* pos;
    char* yytext;
    int errno;
    struct {
        char* yytext;
	int token;
    } pushback;
} NCJparser;

static int
NCjsonparse(char* text, NCjson** treep)
{
    int status = NCJ_OK;
    size_t len;
    NCJparser parser = NULL;
    NCjson* tree = NULL;
    if(text == NULL) {status = NCJ_EINVAL; goto done;}
    parser = calloc(1,sizeof(NCJparser));
    if(parser == NULL) {status = NCJ_ENOMEM; goto done;}
    len = strlen(text);
    parser->text = (char*)malloc(len+1+1);
    if(parser->text == NULL) {status = NCJ_ENOMEM; goto done;}
    strcpy(parser->text,text);
    parser->text[len] = '\0';
    parser->text[len+1] = '\0';
    tree = NCJparseR(parser);
done:
    if(parser != NULL) {
	nullfree(parser->text);
	nullfree(parser->yytext);
	free(parser);
    }
    if(status != NCJ_OK) {
	if(tree != NULL) NCjsonfree(tree);
    } else
	if(treep) *treep = tree;
    return status;
}

static int
NCJyytext(NCJparser* parser, char* start, ptrdiff_t pdlen)
{
    size_t len = (size_t)pdlen;
    if(parser->yytext == NULL)
	parser->yytext = (char*)malloc(len+1);
    else
	parser->yytext = (char*) realloc(parser->yytext,len+1);
    if(parser->yytext == NULL) return NCJ_ENOMEM;
    memcpy(parser->yytext,start,len);
    parser->yytext[len] = NCJ_NUL;
    return NCJ_OK;
}

static void
NCJpushback(NCJparser* parser, int token)
{
    parser->pushback.token = token;
    parser->pushback.yytext = strdup(parser->yytext);
}

static int
NCJlex(NCJparser* parser)
{
    int c;
    int token = NCJ_NUL;
    char* start;
    char* next;

    if(parser->pushback.token != NCJ_NOTOKEN) {
	token = parser->pushback.token;
	NCJyytext(parser,parser->pushback.yytext,strlen(parser->pushback.yytext));
	nullfree(parser->pushback.yytext);
	parser->pushback.yytext = NULL;
	parser->pushback.token = NCJ_NOTOKEN;
	return token;
    }

    c = *parser->pos;
    if(c == NCJ_NUL) {
	token = NCJ_NUL;
    } else if(strchr(NCJ_WORD, c) != NULL) {
	size_t len;
        start = parser->pos;
        next = start + 1;
	for(;;) {
	    c = *parser->pos++;
	    if(strchr(NCJ_WHITESPACE,c) != NULL || c == NCJ_NUL) break;
	    last++;
	}
	if(!NCJyytext(parser,start,(next - start))) goto done;
	token = NCJ_WORD;
    } else if(c == NCJ_QUOTE) {
	parser->pos++;
	start = parser->pos;
	next = start+1;
	for(;;) {
	    c = *parser->pos++;
	    if(c == NCJ_QUOTE || c == NCJ_NUL) break;
	    last++;
	}
	if(c == NCJ_NUL) {
	    parser->errno = NCJ_ESTRING;
	    token = NCJ_ERR;
	    goto done;
	}
	if(!NCJyytext(parser,start,(next - start))) goto done;
	token = NCJ_STRING;
    } else { /* single char token */
	token = *parser->pos++;
    }
done:
    if(parser->errno) token = NCJ_ERR;
    return token;
}

/* Simple recursive descent */

static int
NCJparseR(NCJparser* parser, NCjson** listp)
{
    int token = NCJ_ERR;
    NCjson* list = NULL;
    if((token = NCJlex(parser)) == NCJ_ERR) goto done;
    switch (token) {
    case NCJ_NUL;
	break;
    case NCJ_WORD:
        NCJappend(NCJparseAtomic(parser,token),listp);
	break;
    case NCJ_LBRACE:
	NCJappend(NCJparseMap(parser,locallist),listp);
	break;
    case NCJ_LBRACKET:
        NCJappend(NCJparseArray(parser,NULL),)
    case NCJ_STRING:
        return NCJparseAtomic(parser,token);
    default:
	parser->errno = NCJ_EBADTOKEN;
    }
    return NULL;
}

static NCjson*
NCJparseAtomic(NCJparser* parser, int kind)
{
    /* assert (kind == NCJ_WORD || kind = NCJ_QUOTE) */
    NCjson* node;
    if((node = NCJmakenode(parser)) == NULL)
	{parser->errno = NCJ_ENOMEM; goto done;}
    if(kind == NCJ_STRING)
        node->sort = NCJ_WORD;
        node->word = strdup(parser->yytext);
    } else {
	/* Try to convert to number or boolean; last resort is word */
        size_t count = (last - start) + 1;
	int nread = 0;
	int ncvt = sscan(parser->yytext,
			 "%L",&node->num,&nread);
	if(ncvt == 1 && nread == count) {
	    node->sort = NCJ_NUMBER;
	} else if(strcasecmp(parser->yytext,NCJ_TRUE)==0) {
	    node->sort = NCJ_BOOLEAN;
	    node->num = 1;
	} else if(strcasecmp(parser->yytext,NCJ_FALSE)==0) {
	    node->sort = NCJ_BOOLEAN;
	    node->num = 0;
	} else {
	    node->word = strdup(parser->yytext);
	    node->sort = NCJ_WORD;
        }
    }
done:
    return node;
}

static NCjson*
NCJparseArray(NCJparser* parser)
{
    NCjson* head = NULL;
    NCjson* last = NULL;
    int token = NCJ_ERR;
#if 0
    if((node = NCJmakenode(parser)) == NULL) goto done;
#endif
    loop:
    for(;;) {
        if((token = NCJlex(parser)) == NCJ_ERR) goto done;
        switch (token) {
        case NCJ_NUL;
            break;
        case NCJ_RBRACKET:
	    break loop;
        default:
	    NCJpushback(parser,token);
            NCjson* o = NCJparseR(parser);
            tokens.nextToken();
            if(tokens.ttype == NCJ_EOF) break;
            else if(tokens.ttype == RBRACKET) tokens.pushBack();
            else if(tokens.ttype != COMMA)
                throw new IOException("Missing comma in list");
            array.add(o);
        }
    }
    return array;
}

static NCjson parseMap(StreamTokenizer tokens)
{
    assert (tokens.ttype == LBRACE);
    Map<char*, Object> map = new LinkedHashMap<>();  /* Keep insertion order */
    loop:
    for(; ; ) {
        int token = tokens.nextToken();
        switch (token) {
        case NCJ_NCJ_EOL:
            break; /* ignore */
        case NCJ_NCJ_EOF:
            throw new IOException("Unexpected eof");
        case NCJ_RBRACE:
            break loop;
        default:
            tokens.pushBack();
            NCjson name = parseR(tokens);
            if(tokens.ttype == NCJ_EOF) break;
            if(name instanceof char*
                    || name instanceof Long
                    || name instanceof Boolean) {
                  /*ok*/
            } else
                throw new IOException("Unexpected map name type: " + name);
            if(tokens.nextToken() != COLON)
                throw new IOException("Expected ':'; found: " + tokens.ttype);
            NCjson o = parseR(tokens);
            tokens.nextToken();
            if(tokens.ttype == NCJ_EOF) break;
            else if(tokens.ttype == RBRACE) tokens.pushBack();
            else if(tokens.ttype != COMMA)
                throw new IOException("Missing comma in list");
            map.put(name.tochar*(), o);
        }
    }
    return map;
}
}

static char* tochar*(Object o) {return tochar*(o,"");}

static char* tochar*(Object o, char* demark)
{
char*Builder buf = new char*Builder();
tochar*R(o, buf, demark, 0);
return buf.tochar*();
}

static static void tochar*R(Object o, char*Builder buf, char* demark, int indent)
{
boolean first = true;
if(o instanceof List) {
    List<Object> list = (List<Object>) o;
    if(list.size()== 0) {
        buf.append(LBRACKET);
        buf.append(RBRACKET);
    } else {
        buf.append(LBRACKET);
        buf.append('\n');
        for(int i=0;i<list.size();i++) {
            NCjson e = list.get(i);
            buf.append(indent(indent));
            tochar*R(e, buf, demark, indent + 2);
            if(i < list.size()-1) buf.append(",");
            buf.append("\n");
        }
        buf.append(indent(indent));
        buf.append(RBRACKET);
    }
} else if(o instanceof Map) {
    Map<char*, Object> map = (Map<char*, Object>) o;
    if(map.size() == 0) {
        buf.append(LBRACE);
        buf.append(RBRACE);
    } else {
        buf.append(LBRACE);
        buf.append('\n');
        int i = 0;
        for(Map.Entry<char*, Object> e : map.entrySet()) {
            buf.append(indent(indent + 2));
            buf.append(QUOTE);
            buf.append(e.getKey().replace("\"","\\\""));
            buf.append(QUOTE);
            buf.append(' ');
            buf.append(COLON);
            buf.append(' ');
            tochar*R(e.getValue(), buf, demark, indent + 2);
            if(i < map.size() - 1) buf.append(",");
            buf.append("\n");
            i++;
        }
        buf.append(indent(indent));
        buf.append(RBRACE);
    }
} else if((o instanceof Long) || (o instanceof Boolean)) {
    buf.append(demark);
    buf.append(o.tochar*());
    buf.append(demark);
} else {
    buf.append(QUOTE);
    buf.append(o.tochar*().replace("\"","\\\""));
    buf.append(QUOTE);
}
}

static char* blanks = "                                                  ";

static static char* indent(int n)
{
while(n > blanks.length()) {
    blanks = blanks + blanks;
}
return blanks.substring(0, n);
}

}

static NCjson*
NCJmakenode(NCjsonparser* parser)
{
    NCjson* node = NULL;
    parser->errno = NCJ_OK;
    node = (NCjson*)calloc(1,sizeof(NCjson));
    if(node == null) parser->errno = NCJ_ENOMEM;
    return node;
}


#endif /*NCJSON_INC*/
