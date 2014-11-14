/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page.  It can also be found at     *
 * http://hdfgroup.org/HDF5/doc/Copyright.html.  If you do not have          *
 * access to either file, you may request a copy from help@hdfgroup.org.     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*keep this declaration near the top of this file -RPM*/
static const char *FileHeader = "\n\
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\n\
 * Copyright by The HDF Group.                                               *\n\
 * Copyright by the Board of Trustees of the University of Illinois.         *\n\
 * All rights reserved.                                                      *\n\
 *                                                                           *\n\
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *\n\
 * terms governing use, modification, and redistribution, is contained in    *\n\
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *\n\
 * of the source code distribution tree; Copyright.html can be found at the  *\n\
 * root level of an installed copy of the electronic HDF5 document set and   *\n\
 * is linked from the top-level documents page.  It can also be found at     *\n\
 * http://hdfgroup.org/HDF5/doc/Copyright.html.  If you do not have          *\n\
 * access to either file, you may request a copy from help@hdfgroup.org.     *\n\
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *";
/*
 *
 * Created:	H5make_libsettings.c
 *		17 Mar 2010
 *		Quincey Koziol
 *
 * Purpose:	Generate the H5libsettings.c file from the
 *		libhdf5.settings file.
 *
 *-------------------------------------------------------------------------
 */

#include <stdio.h>
#include <time.h>
#include "H5private.h"

#define LIBSETTINGSFNAME "libhdf5.settings"


/*-------------------------------------------------------------------------
 * Function:	insert_libhdf5_settings
 *
 * Purpose:	insert the contents of libhdf5.settings into a file
 *		represented by flibinfo.
 *		Make it an empty string if H5_HAVE_EMBEDDED_LIBINFO is not
 *		defined, i.e., not enabled.
 *
 * Return:	void
 *
 * Programmer:	Albert Cheng
 *		Apr 20, 2009
 *
 *-------------------------------------------------------------------------
 */
static void
insert_libhdf5_settings(FILE *flibinfo)
{
#ifdef H5_HAVE_EMBEDDED_LIBINFO
    FILE *fsettings;	/* for files libhdf5.settings */
    int inchar;
    int	bol = 0;	/* indicates the beginning of a new line */

    if(NULL == (fsettings = HDfopen(LIBSETTINGSFNAME, "r"))) {
        HDperror(LIBSETTINGSFNAME);
        HDexit(1);
    } /* end if */

    /* print variable definition and the string */
    /* Do not use const else AIX strings does not show it. */
    fprintf(flibinfo, "char H5libhdf5_settings[]=\n");
    bol++;
    while(EOF != (inchar = HDgetc(fsettings))) {
	if(bol) {
	    /* Start a new line */
	    fprintf(flibinfo, "\t\"");
	    bol = 0;
	} /* end if */
	if(inchar == '\n') {
	    /* end of a line */
	    fprintf(flibinfo, "\\n\"\n");
	    bol++;
        } /* end if */
	else
	    HDputc(inchar, flibinfo);
    } /* end while */
    if(HDfeof(fsettings)) {
	/* wrap up */
	if(!bol)
	    /* EOF found without a new line */
	    fprintf(flibinfo, "\\n\"\n");
	fprintf(flibinfo, ";\n\n");
    } /* end if */
    else {
	fprintf(stderr, "Read errors encountered with %s\n", LIBSETTINGSFNAME);
	HDexit(1);
    } /* end else */
    if(0 != HDfclose(fsettings)) {
	HDperror(LIBSETTINGSFNAME);
	HDexit(1);
    } /* end if */
#else
    /* print variable definition and an empty string */
    /* Do not use const else AIX strings does not show it. */
    fprintf(flibinfo, "char H5libhdf5_settings[]=\"\";\n");
#endif
} /* insert_libhdf5_settings() */


/*-------------------------------------------------------------------------
 * Function:	make_libinfo
 *
 * Purpose:	Create the embedded library information definition.
 * 		This sets up for a potential extension that the declaration
 *		is printed to a file different from stdout.
 *
 * Return:	void
 *
 * Programmer:	Albert Cheng
 *		Sep 15, 2009
 *
 *-------------------------------------------------------------------------
 */
static void
make_libinfo(void)
{
    /* print variable definition and then the string as a macro. */
    insert_libhdf5_settings(stdout);
}


/*-------------------------------------------------------------------------
 * Function:	print_header
 *
 * Purpose:	Prints the header for the generated file.
 *
 * Return:	void
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Mar 12 1997
 *
 *-------------------------------------------------------------------------
 */
static void
print_header(void)
{
    time_t		now = HDtime(NULL);
    struct tm		*tm = HDlocaltime(&now);
    char		real_name[30];
    char		host_name[256];
    int			i;
    const char		*s;
#ifdef H5_HAVE_GETPWUID
    struct passwd	*pwd = NULL;
#else
    int			pwd = 1;
#endif
    static const char	*month_name[] =
    {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    static const char	*purpose = "\
This machine-generated source code contains\n\
information about the library build configuration\n";

    /*
     * The real name is the first item from the passwd gecos field.
     */
#ifdef H5_HAVE_GETPWUID
    {
	size_t n;
	char *comma;

	if((pwd = HDgetpwuid(HDgetuid()))) {
	    if((comma = HDstrchr(pwd->pw_gecos, ','))) {
		n = MIN(sizeof(real_name) - 1, (unsigned)(comma - pwd->pw_gecos));
		HDstrncpy(real_name, pwd->pw_gecos, n);
		real_name[n] = '\0';
	    } /* end if */
            else {
		HDstrncpy(real_name, pwd->pw_gecos, sizeof(real_name));
		real_name[sizeof(real_name) - 1] = '\0';
	    } /* end else */
	} /* end if */
        else
	    real_name[0] = '\0';
    }
#else
    real_name[0] = '\0';
#endif

    /*
     * The FQDM of this host or the empty string.
     */
#ifdef H5_HAVE_GETHOSTNAME
    if(HDgethostname(host_name, sizeof(host_name)) < 0)
	host_name[0] = '\0';
#else
    host_name[0] = '\0';
#endif

    /*
     * The file header: warning, copyright notice, build information.
     */
    printf("/* Generated automatically by H5make_libsettings -- do not edit */\n\n\n");
    HDputs(FileHeader);		/*the copyright notice--see top of this file */

    printf(" *\n * Created:\t\t%s %2d, %4d\n",
	   month_name[tm->tm_mon], tm->tm_mday, 1900 + tm->tm_year);
    if(pwd || real_name[0] || host_name[0]) {
	printf(" *\t\t\t");
	if(real_name[0])
            printf("%s <", real_name);
#ifdef H5_HAVE_GETPWUID
	if(pwd)
            HDfputs(pwd->pw_name, stdout);
#endif
	if(host_name[0])
            printf("@%s", host_name);
	if(real_name[0])
            printf(">");
	HDputchar('\n');
    } /* end if */
    printf(" *\n * Purpose:\t\t");
    for(s = purpose; *s; s++) {
	HDputchar(*s);
	if('\n' == *s && s[1])
            printf(" *\t\t\t");
    } /* end for */

    printf(" *\n * Modifications:\n *\n");
    printf(" *\tDO NOT MAKE MODIFICATIONS TO THIS FILE!\n");
    printf(" *\tIt was generated by code in `H5make_libsettings.c'.\n");

    printf(" *\n *");
    for(i = 0; i < 73; i++)
        HDputchar('-');
    printf("\n */\n\n");
}


/*-------------------------------------------------------------------------
 * Function:	print_footer
 *
 * Purpose:	Prints the file footer for the generated file.
 *
 * Return:	void
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Mar 31 2010
 *
 *-------------------------------------------------------------------------
 */
static void
print_footer(void)
{
    /* nothing */
}


/*-------------------------------------------------------------------------
 * Function:	main
 *
 * Purpose:	Main entry point.
 *
 * Return:	Success:	exit(0)
 *
 *		Failure:	exit(1)
 *
 * Programmer:	Albert Cheng
 *		2010/4/1
 *
 *-------------------------------------------------------------------------
 */
int
main(void)
{
    print_header();

    /* Generate embedded library information variable definition */
    make_libinfo();

    print_footer();

    HDexit(0);
}
