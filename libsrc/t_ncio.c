/*
 *   Copyright 2018, University Corporation for Atmospheric Research
 *   See top level COPYRIGHT file for copying and redistribution conditions.
 */
/* $Id: t_ncio.c,v 1.10 2010/05/26 11:11:26 ed Exp $ */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "ncio.h"
#ifndef NC_NOERR
#define NC_NOERR 0
#endif


static void
usage(const char *av0)
{
	(void)fprintf(stderr,
		"Usage: %s [options] fname\t\nOptions:\n", av0);
	(void)fprintf(stderr,
		"\t-v		Verbose\n") ;
	(void)fprintf(stderr,
		"\t-w		Open Read/Write, default is read only\n") ;
	(void)fprintf(stderr,
		"\t-c		Create, clobber existing\n") ;
	(void)fprintf(stderr,
		"\t-n		Create, error if it already exists\n") ;
	(void)fprintf(stderr,
		"\t-L		Use locking if available\n") ;
	(void)fprintf(stderr,
		"\t-S		Share updates (turn off caching)\n") ;
	(void)fprintf(stderr,
		"\t-U		Delete (unlink) on close\n") ;
	(void)fprintf(stderr,
		"\t-o igeto	Initial get offset\n") ;
	(void)fprintf(stderr,
		"\t-i igetsz	Initial get size\n") ;
	(void)fprintf(stderr,
		"\t-I initialsz	Initial file size for create\n") ;
	(void)fprintf(stderr,
		"\t-s sizehint	Buffer size\n") ;
	exit(EXIT_FAILURE);
}


static long
argscale(const char *arg, const char *tag)
{
	long value = 0;
		/* last character */
	const char *cp = arg + strlen(arg) -1;
	value = atol(arg);
	if(isalpha(*cp))
	{
		switch(*cp) {
		case 'k':
		case 'K':
			value *= 1024;
			break;
		case 'm':
		case 'M':
			value *= (1024*1024);
			break;
		default:
			value = 0; /* trigger error below */
			break;
		}
	}
	if(value == 0)
	{
		fprintf(stderr,
			 "Illegal %s \"%s\", ignored\n", tag, arg);
	}
	return value;
}


static void
modify_ex(off_t offset, size_t extent, void *vp)
{
	unsigned char *obuf = vp;
	const unsigned char *const end = &obuf[extent];
	unsigned char *cp = obuf;

	if(cp >= end) return;
	*cp++ = (unsigned char)( offset               >> 24);
	if(cp >= end) return;
	*cp++ = (unsigned char)((offset & 0x00ff0000) >> 16);
	if(cp >= end) return;
	*cp++ = (unsigned char)((offset & 0x0000ff00) >>  8);
	if(cp >= end) return;
	*cp++ = (unsigned char)( offset & 0x000000ff);
	if(cp >= end) return;
	*cp++ = (unsigned char)( extent               >> 24);
	if(cp >= end) return;
	*cp++ = (unsigned char)((extent & 0x00ff0000) >> 16);
	if(cp >= end) return;
	*cp++ = (unsigned char)((extent & 0x0000ff00) >>  8);
	if(cp >= end) return;
	*cp++   = (unsigned char)( extent & 0x000000ff);

	while(cp < end)
	{
		*cp++ = (unsigned char) (cp - obuf);
	}
}


typedef struct riu {
	struct riu *next;
	struct riu *prev;
	off_t offset;
	size_t extent;
	void *vp;
} riu;

static void
free_riu(riu *riup)
{
	if(riup == NULL)
		return;
	free(riup);
}

static riu *
new_riu(off_t offset, size_t extent, void *vp)
{
	riu *riup = (riu *)malloc(sizeof(riu));
	if(riup == NULL)
	{
		fprintf(stderr,
			"new_riu: malloc failed\n");
		exit(EXIT_FAILURE);
	}
	riup->next = NULL;
	riup->prev = NULL;
	riup->offset = offset;
	riup->extent = extent;
	riup->vp = vp;
	return riup;
}

static riu *stack = NULL;

static void
riu_push(off_t offset, size_t extent, void *vp)
{
	riu *riup = new_riu(offset, extent, vp);
	/* assert(riup != NULL); */
	riup->next = stack;
	if(stack != NULL)
		stack->prev = riup;
	stack = riup;
}

static int
riu_pop(off_t offset, int modify)
{
	riu *riup = stack;
	while(riup != NULL)
	{
		if(riup->offset == offset)
		{
			if(modify)
			{
				modify_ex(riup->offset, riup->extent, riup->vp);
			}
			if(riup->next != NULL)
			{
				riup->next->prev = riup->prev;
			}
			if(riup == stack)
			{
				stack = riup->next;
			}
			else
			{
				assert(riup->prev != NULL);
				riup->prev->next = riup->next;
			}
			free_riu(riup);
			return 1;
		}
		riup = riup->next;
	}
	/* else, not found */
	return 0;
}

main(int ac, char *av[])
{
	char *path = "";
	ncio *nciop;
	char linebuf[128];
	off_t offset;
	size_t extent;
	void *vp;
	int status = NC_NOERR;
	int verbose = 0;
	int flags = 0;
	int create = 0;
	off_t igeto = 0;
	size_t igetsz = 0;
	size_t initialsz = 0;
	int doUnlink = 0;
	size_t sizehint = NC_SIZEHINT_DEFAULT;

	{
	extern int optind;
	extern int opterr;
	extern char *optarg;
	int ch;

	opterr = 1;

	while ((ch = getopt(ac, av, "vwcnLSUo:i:I:s:")) != EOF)
		switch (ch) {
		case 'v':
			verbose = 1;
			break;
		case 'w':
			flags |= NC_WRITE;
			break;
		case 'c':
			create = 1;
			break;
		case 'n':
			create = 1;
			flags |= NC_NOCLOBBER;
			break;
		case 'L':
			flags |= NC_LOCK;
			break;
		case 'S':
			flags |= NC_SHARE;
			break;
		case 'U':
			doUnlink = 1;
			break;
		case 'o':
			igeto = argscale(optarg, "igeto");
			break;
		case 'i':
			igetsz = argscale(optarg, "igetsz");
			break;
		case 'I':
			initialsz = argscale(optarg, "initialsz");
			break;
		case 's':
			sizehint = argscale(optarg, "sizehint");
			break;
		case '?':
			usage(av[0]);
			break;
		}

	/* last arg, the file name, is required */
	if(ac - optind <= 0)
		usage(av[0]) ;
	path = av[optind];


	}

	if(!create)
	{
		status = ncio_open(path, flags,
				igeto, igetsz, &sizehint,
				&nciop, &vp);
		if(status != NC_NOERR)
		{
			fprintf(stderr, "ncio_open: %s: %s\n",
				path, strerror(status));
			return(EXIT_FAILURE);
		}
	} else {
		status = ncio_create(path, flags, initialsz,
			igeto, igetsz, &sizehint,
			&nciop, &vp);
		if(status != NC_NOERR)
		{
			fprintf(stderr, "ncio_create: %s: %s\n",
				path, strerror(status));
			return(EXIT_FAILURE);
		}
	}

	while(fgets(linebuf, sizeof(linebuf), stdin) != NULL)
	{
		offset = 0;
		extent = 0;

		if(*linebuf == '#')
			continue; /* comment */
		if(sscanf(linebuf, "rel 0x%lx", &offset) == 1
			|| sscanf(linebuf, "rel %ld", &offset) == 1)
		{
			if(verbose)
				printf("- rel  %8ld\n", offset);
			if(!riu_pop(offset, 0))
				continue;
			status = nciop->rel(nciop, offset, 0);
			if(status)
			{
				fprintf(stderr, "- rel  error: %s\n",
					strerror(status));
				continue;
			}
		}
		else if(sscanf(linebuf, "relm 0x%lx", &offset) == 1
			|| sscanf(linebuf, "relm %ld", &offset) == 1)
		{
			if(verbose)
				printf("- relm %8ld\n", offset);
			if(!riu_pop(offset, 1))
				continue;
			status = nciop->rel(nciop, offset, RGN_MODIFIED);
			if(status)
			{
				fprintf(stderr, "- relm %8ld error: %s\n",
					offset, strerror(status));
				continue;
			}
		}
		else if(sscanf(linebuf, "get 0x%lx %ld", &offset, &extent) == 2
			|| sscanf(linebuf, "get %ld %ld", &offset, &extent) == 2)
		{
			if(verbose)
				printf("- get  %10ld %8ld\n", offset, extent);
			status = nciop->get(nciop, offset, extent, 0, &vp);
			if(status)
			{
				fprintf(stderr, "- get  error: %s\n",
					strerror(status));
				continue;
			}
			riu_push(offset, extent, vp);
		}
		else if(sscanf(linebuf, "getw 0x%lx %ld", &offset, &extent) == 2
			|| sscanf(linebuf, "getw %ld %ld", &offset, &extent) == 2)
		{
			if(verbose)
				printf("- getw %10ld %8ld\n", offset, extent);
			status = nciop->get(nciop, offset, extent, RGN_WRITE, &vp);
			if(status)
			{
				fprintf(stderr, "- getw  error: %s\n",
					strerror(status));
				continue;
			}
			riu_push(offset, extent, vp);
		}
		else if(strchr(linebuf, 'q') != NULL)
			break;
		else
			printf("???\n");
	}

	status = ncio_close(nciop, doUnlink);
	if(status != NC_NOERR)
	{
		fprintf(stderr, "ncio_close(%s): %s: %s\n",
			doUnlink ? "doUnlink" : "",
			path, strerror(status));
		return(EXIT_FAILURE);
	}

	return(EXIT_SUCCESS);
}
