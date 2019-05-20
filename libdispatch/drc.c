/*
Copyright (c) 1998-2017 University Corporation for Atmospheric Research/Unidata
See LICENSE.txt for license information.
*/

#include "config.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nc.h"
#include "nclog.h"
#include "ncbytes.h"
#include "ncrc.h"

#define RTAG ']'
#define LTAG '['

#define TRIMCHARS " \t\r\n"

static char* ENVRCLIST[] = {"DAPRCFILE","NETCDFRCFILE",NULL};
static char* RCFILELIST[] = {".netcdfrc",".daprc", ".dodsrc",NULL};

/*Forward*/
static int rcreadline(FILE* f, NCbytes*);
static void rctrim(NCbytes* text);
static void storedump(char* msg, NCTripleStore*);
static int rc_compile(const char* path);
static const NCTriple* rc_locate(NCTripleStore* rc, char* key, char* tag);
static int rc_search(const char* prefix, const char* rcfile, char** pathp);
static int copycat(char* dst, size_t size, size_t n, ...);

/**************************************************/

static int ncrc_ignore = 0;
static int ncrc_loaded = 0;
static char* ncrc_home = NULL;
static NCTripleStore ncrc_store;

/**************************************************/

/* read and compile the rc file, if any */
int
ncrc_load(const char* filename)
{
    int stat = NC_NOERR;
    char* path = NULL;

    if(ncrc_ignore) {
        nclog(NCLOGDBG,"No runtime configuration file specified; continuing");
	goto done;
    }
    if(ncrc_loaded) return NC_NOERR;

    /* locate the configuration files in the following order:
       1. specified by argument
       2. set by any ENVRCLIST env variable
       3. '.'
       4. $HOME
    */
    if(filename != NULL) /* always use this */
	path = strdup(filename);
    if(path == NULL) {
	char** p;
	for(p=ENVRCLIST;*p;p++) {
	    const char* value = getenv(*p);
            if(value != NULL && strlen(value) > 0) {
		path = strdup(value);
		break;
	    }
	}
    }
    if(path == NULL) {
	char** rcname;
	for(rcname=RCFILELIST;*rcname;rcname++) {
	    stat = rc_search(".",*rcname,&path);
    	    if(stat != NC_NOERR || path != NULL) break;
	    stat = rc_search(ncrc_home,*rcname,&path);
    	    if(stat != NC_NOERR || path != NULL) break;
	}
	if(stat != NC_NOERR) goto done;
    }
    if(path == NULL) {
        nclog(NCLOGDBG,"Cannot find runtime configuration file; continuing");
	goto done;
    }
    nclog(NCLOGDBG,"RC files: %s\n", path);
    if(rc_compile(path) == 0) {
	nclog(NCLOGERR, "Error parsing %s\n",path);
	stat = NC_NOERR;
    }
done:
    ncrc_loaded = 1; /* even if not exists */
    if(path != NULL)
	free(path);
    return stat;
}

void
ncrc_reset(NCTripleStore* store)
{
    int i;
    if(store->triples == NULL) return;
    for(i=0;i<nclistlength(store->triples);i++) {
	NCTriple* triple = (NCTriple*)nclistget(store->triples,i);
	if(triple == NULL) continue;
	if(triple->tag != NULL) free(triple->tag);
	if(triple->key != NULL) free(triple->key);
	if(triple->value != NULL) free(triple->value);
	free(triple);
    }
    nclistfree(store->triples);
    store->triples = NULL;
}

char*
ncrc_lookup(NCTripleStore* store, char* key, char* tag)
{
    const NCTriple* triple = rc_locate(store, key, tag);
    if(triple != NULL && ncdebug > 2) {
	fprintf(stderr,"lookup %s: [%s]%s = %s\n",tag,triple->tag,triple->key,triple->value);
    }
    return (triple == NULL ? NULL : triple->value);
}

static const NCTriple*
rc_locate(NCTripleStore* rc, char* key, char* tag)
{
    int i,found;

    if(ncrc_ignore || !ncrc_loaded || rc->triples == NULL)
	return NULL;
    if(key == NULL || rc == NULL) return NULL;

    if(tag == NULL) tag = "";
    /* Assume that the triple store has been properly sorted */
    for(found=0,i=0;i<nclistlength(rc->triples);i++) {
	NCTriple* triple = (NCTriple*)nclistget(rc->triples,i);
        size_t taglen = strlen(triple->tag);
        int t;
        if(strcmp(key,triple->key) != 0) continue; /* keys do not match */
        /* If the triple entry has no tag, then use it
           (because we have checked all other cases)*/
        if(taglen == 0) {found=1;break;}
        /* do tag match */
        t = strcmp(tag,triple->tag);
        if(t ==  0) return triple;
    }
    return NULL;
}

static int
rcreadline(FILE* f, NCbytes* buf)
{
    int c;
    ncbytesclear(buf);
    for(;;) {
        c = getc(f);
        if(c < 0) break; /* eof */
        if(c == '\n') break; /* eol */
	ncbytesappend(buf,c);
    }
    ncbytesnull(buf);
    return 1;
}

/* Trim TRIMCHARS from both ends of text; */
static void
rctrim(NCbytes* buf)
{
    if(nclistlength(buf) == 0) return;
    for(;;) {
	int c = ncbytesget(buf,0);
        if(strchr(TRIMCHARS,c) == NULL) break; /* hit non-trim char */
	ncbytesremove(buf,0);
    }
    int pos = ncbyteslength(buf) - 1;
    while(pos >= 0) {
	int c = ncbytesget(buf,pos);
        if(strchr(TRIMCHARS,c) == NULL) break; /* hit non-trim char */
	ncbytesremove(buf,pos);
	pos--;
    }
}

/* insertion sort the triplestore based on tag */
static void
sorttriplestore(NCTripleStore* store)
{
    int i, nsorted, len;
    NCTriple** content = NULL;

    if(store == NULL) return; /* nothing to sort */
    len = nclistlength(store->triples);
    if(len <= 1) return; /* nothing to sort */
    if(ncdebug > 2)
        storedump("initial:",store);
    content = (NCTriple**)nclistdup(store->triples);
    nclistclear(store->triples);
    nsorted = 0;
    while(nsorted < len) {
        int largest;
        /* locate first non killed entry */
        for(largest=0;largest<len;largest++) {
            if(content[largest]->key[0] != '\0') break;
        }
        for(i=0;i<len;i++) {
            if(content[i]->key[0] != '\0') { /* avoid empty slots */
                int lexorder = strcmp(content[i]->tag,content[largest]->tag);
                int leni = strlen(content[i]->tag);
                int lenlarge = strlen(content[largest]->tag);
                /* this defines the ordering */
                if(leni == 0 && lenlarge == 0)
		    continue; /* if no tags, then leave in order */
                if(leni != 0 && lenlarge == 0)
		    largest = i;
                else if(lexorder > 0)
		    largest = i;
            }
        }
        /* Move the largest entry */
	nclistpush(store->triples,content[largest]);
        content[largest]->key[0] = '\0'; /* kill entry */
        nsorted++;
        if(ncdebug > 2)
	    storedump("pass:",store);
    }
    free(content);
    if(ncdebug > 1)
        storedump("final .rc order:",store);
}

/* Create a triple store from a file */
static int
rc_compile(const char* path)
{
    FILE *in_file = NULL;
    int linecount = 0;
    NCbytes* buf;
    NCTripleStore* rc;

    rc = &ncrc_store;
    memset(rc,0,sizeof(NCTripleStore));
    rc->triples = nclistnew();

    in_file = fopen(path, "r"); /* Open the file to read it */
    if (in_file == NULL) {
        nclog(NCLOGERR, "Could not open configuration file: %s",path);
        return NC_EPERM;
    }

    buf =  ncbytesnew();
    for(;;) {
        int c;
	int pos;
	char* line;
	size_t len,count;
	char* value;

        if(!rcreadline(in_file,buf)) break;
        linecount++;
        rctrim(buf);  /* trim leading and trailing blanks */
        len = ncbyteslength(buf);
	line = ncbytescontents(buf);

	if(len == 0) continue;
        if(line[0] == '#') continue; /* check for comment */

        /* setup */
	NCTriple* triple = (NCTriple*)calloc(1,sizeof(NCTriple));
	if(triple == NULL) {
            nclog(NCLOGERR, "Out of memory reading rc file: %s",path);
	    goto done;
	}	
        nclistpush(rc->triples,triple);
	c = line[0];
        if(c == LTAG) {
	    int i;
	    for(i=0;i<len;i++) {
		if(line[i] != RTAG) break;
	    }
	    if(i == len) {/* RTAG is missing */
		nclog(NCLOGERR, "Line has missing %c: %s",RTAG,line);
	        goto done;
	    }
	    count = (i - 1);
	    if(count > 0) {
		triple->tag = (char*)malloc(count+1);
		if(triple->tag == NULL) {goto done;}
		memcpy(triple->tag,&line[1],count);
	    }
	    memmove(line,&line[count]+1,count+2); /* remove [...] */
	}
        /* split off key and value */
        value = strchr(line, '=');
        if(value == NULL)
            value = line + strlen(line);
        else {
            *value = '\0';
            value++;
        }
	triple->key = strdup(line);
	if(*value == '\0')
	    triple->value = strdup("1");	    
	else
	    triple->value = strdup(value);
    }
done:
    fclose(in_file);
    sorttriplestore(rc);
    return 1;
}



static void
storedump(char* msg, NCTripleStore* rc)
{
    int i;

    if(msg != NULL) fprintf(stderr,"%s\n",msg);
    if(rc == NULL || nclistlength(rc->triples) == 0) {
        fprintf(stderr,"<EMPTY>\n");
        return;
    }
    for(i=0;i<nclistlength(rc->triples);i++) {
	NCTriple* triple = (NCTriple*)nclistget(rc->triples,i);
	if(triple->tag == NULL)
	    fprintf(stderr,"[%s]",triple->tag);
        fprintf(stderr,"%s=%s\n",triple->key,triple->value);
    }
    fflush(stderr);
}

static int
rc_search(const char* prefix, const char* rcname, char** pathp)
{
    char* path = NULL;
    FILE* f = NULL;
    int plen = strlen(prefix);
    int rclen = strlen(rcname);
    int stat = NC_NOERR;

    size_t pathlen = plen+rclen+1+1; /*+1 for '/' +1 for nul*/
    path = (char*)malloc(pathlen);
    if(path == NULL) {
	stat = NC_ENOMEM;
	goto done;
    }
    if(!copycat(path,pathlen,3,prefix,"/",rcname)) {
        stat = NC_ENOMEM;
	goto done;
    }
    /* see if file is readable */
    f = fopen(path,"r");
    if(f != NULL)
        nclog(NCLOGDBG, "Found rc file=%s",path);
done:
    if(f == NULL || stat != NC_NOERR) {
	if(path != NULL)
	    free(path);
	path = NULL;
    }
    if(f != NULL)
	fclose(f);
    if(pathp != NULL)
	*pathp = path;
    return stat;
}

/*
Instead of using snprintf to concatenate
multiple strings into a given target,
provide a direct concatenator.
So, this function concats the n argument strings
and overwrites the contents of dst.
Care is taken to never overrun the available
space (the size parameter).
Note that size is assumed to include the null
terminator and that in the event of overrun,
the string will have a null at dst[size-1].
Return 0 if overrun, 1 otherwise.
*/
static int
copycat(char* dst, size_t size, size_t n, ...)
{
    va_list args;
    size_t avail = size - 1;
    int i; 
    int status = 1; /* assume ok */
    char* p = dst;

    if(n == 0) {
	if(size > 0)
	    dst[0] = '\0';
	return (size > 0 ? 1: 0);
    }
	
    va_start(args,n);
    for(i=0;i<n;i++) {
	char* q = va_arg(args, char*);
	for(;;) {
	    int c = *q++;
	    if(c == '\0') break;
	    if(avail == 0) {status = 0; goto done;}
	    *p++ = c;
	    avail--;
	}
    }
    /* make sure we null terminate;
       note that since avail was size-1, there
       will always be room
    */
    *p = '\0';    

done:
    va_end(args);
    return status;    
}
