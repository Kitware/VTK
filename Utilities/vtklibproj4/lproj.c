/*
** lproj: projgram to exercise library of cartographic projections
**
** Copyright (c) 2003, 2005, 2006   Gerald I. Evenden
*/
static const char
RCS_ID[] = "Id";
/*
** Permission is hereby granted, free of charge, to any person obtaining
** a copy of this software and associated documentation files (the
** "Software"), to deal in the Software without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Software, and to
** permit persons to whom the Software is furnished to do so, subject to
** the following conditions:
**
** The above copyright notice and this permission notice shall be
** included in all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
** SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#define PROJ_UV_TYPE
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include "proj_config.h"
#ifdef PROJ_LIST_EXTERNAL
   /* we must define our own proj_list since libproj4 does not */
#  define PROJ_LIST_H "proj_list.h"
#endif /* PROJ_LIST_EXTERNAL */
#include <lib_proj.h>

#define MAX_LINE 200
#define MAX_PARGS 100
#define PROJ_INVERS(P) (P->inv ? 1 : 0)
static PROJ_UV
(*proj)(PROJ_UV, PROJ *);
static PROJ *Proj;
  static int
reversein = 0,  /* != 0 reverse input arguments */
reverseout = 0,  /* != 0 reverse output arguments */
bin_in = 0,  /* != 0 then binary input */
bin_out = 0,  /* != 0 then binary output */
echoin = 0,  /* echo input data to output line */
tag = '#',  /* beginning of line tag character */
inverse = 0,  /* != 0 then inverse projection */
prescale = 0,  /* != 0 apply cartesian scale factor */
dofactors = 0,  /* determine scale factors */
facs_bad = 0,  /* return condition from proj_factors */
very_verby = 0, /* very verbose mode */
postscale = 0;
  static char
*oform = (char *)0,  /* output format for x-y or decimal degrees */
*oterr = "*\t*",  /* output line for unprojectable input */
*usage =
"usage: %s [ -beEfiIlormsStvVwW [args] ] [ +opts[=arg] ] [ files ]\n";
  static struct PROJ_FACTORS
facs;
  static double
(*informat)(const char *, char **),  /* input data deformatter function */
fscale = 0.;  /* cartesian scale factor */
static struct {
  char    *File_name,     /* input file name */
      *Prog_name;     /* name of program */
  int File_line;      /* approximate line read where error occured */
} emess_dat = { (char *)0, (char *)0, 0 };
  static void
emess(int code, char *fmt, ...) {
  va_list args;

  va_start(args, fmt);
  /* prefix program name, if given */
  if (fmt != NULL)
    (void)fprintf(stderr,"libproj4 system\n<%s>: ",emess_dat.Prog_name);
  /* print file name and line, if given */
  if (emess_dat.File_name != NULL && *emess_dat.File_name) {
    (void)fprintf(stderr,"while processing file: %s", emess_dat.File_name);
    if (emess_dat.File_line > 0)
      (void)fprintf(stderr,", line %d\n", emess_dat.File_line);
    else
      (void)fputc('\n', stderr);
  } else
    putc('\n', stderr);
  /* if |code|==2, print errno code data */
  if (code == 2 || code == -2)
    (void)fprintf(stderr, "Sys errno: %d: %s\n", errno, strerror(errno));
  /* post remainder of call data */
  (void)vfprintf(stderr,fmt,args);
  va_end(args);
  /* die if code positive */
  if (code > 0) {
    (void)fputs("\nprogram abnormally terminated\n", stderr);
    exit(code);
  }
  else
    putc('\n', stderr);
}
  static void  /* file processing function */
process(FILE *fid) {
  char line[MAX_LINE+3], *s = 0, pline[40];
  PROJ_UV data;

  for (;;) {
    ++emess_dat.File_line;
    if (bin_in) {  /* binary input */
      if (fread(&data, sizeof(PROJ_UV), 1, fid) != 1)
        break;
    } else {  /* ascii input */
      if (!(s = fgets(line, MAX_LINE, fid)))
        break;
      if (!strchr(s, '\n')) { /* overlong line */
        int c;
        (void)strcat(s, "\n");
        /* gobble up to newline */
        while ((c = fgetc(fid)) != EOF && c != '\n') ;
      }
      if (*s == tag) {
        if (!bin_out)
          (void)fputs(line, stdout);
        continue;
      }
      if (reversein) {
        data.v = (*informat)(s, &s);
        data.u = (*informat)(s, &s);
      } else {
        data.u = (*informat)(s, &s);
        data.v = (*informat)(s, &s);
      }
      if (data.v == HUGE_VAL)
        data.u = HUGE_VAL;
      if (!*s && (s > line)) --s; /* assumed we gobbled \n */
      if (!bin_out && echoin) {
        int t;
        t = *s;
        *s = '\0';
        (void)fputs(line, stdout);
        *s = (char)t;
        putchar('\t');
      }
    }
    if (data.u != HUGE_VAL) {
      if (prescale) { data.u *= fscale; data.v *= fscale; }
      if (dofactors && !inverse)
        facs_bad = proj_factors(data, Proj, 0., &facs);
      data = (*proj)(data, Proj);
      if (dofactors && inverse)
        facs_bad = proj_factors(data, Proj, 0., &facs);
      if (postscale && data.u != HUGE_VAL)
        { data.u *= fscale; data.v *= fscale; }
    }
    if (bin_out) { /* binary output */
      (void)fwrite(&data, sizeof(PROJ_UV), 1, stdout);
      continue;
    } else if (data.u == HUGE_VAL) /* error output */
      (void)fputs(oterr, stdout);
    else if (inverse && !oform) {  /*ascii DMS output */
      if (reverseout) {
        (void)fputs(proj_rtodms(pline, data.v, "NS"), stdout);
        putchar('\t');
        (void)fputs(proj_rtodms(pline, data.u, "EW"), stdout);
      } else {
        (void)fputs(proj_rtodms(pline, data.u, "EW"), stdout);
        putchar('\t');
        (void)fputs(proj_rtodms(pline, data.v, "NS"), stdout);
      }
    } else {  /* x-y or decimal degree ascii output */
      if (inverse) {
        data.v *= RAD_TO_DEG;
        data.u *= RAD_TO_DEG;
      }
      if (reverseout) {
        (void)printf(oform,data.v); putchar('\t');
        (void)printf(oform,data.u);
      } else {
        (void)printf(oform,data.u); putchar('\t');
        (void)printf(oform,data.v);
      }
    }
    if (dofactors) { /* print scale factor data */
      if (!facs_bad)
        (void)printf("\t<%.7g %.7g %g %g %g %g>",
          facs.h, facs.k, facs.s,
          facs.omega * RAD_TO_DEG, facs.a, facs.b);
      else
        (void)fputs("\t<* * * * * *>", stdout);
    }
    (void)fputs(bin_in ? "\n" : s, stdout);
  }
}
  static void  /* file processing function --- verbosely */
vprocess(FILE *fid) {
  char line[MAX_LINE+3], *s, pline[40];
  PROJ_UV dat_ll, dat_xy;
  int linvers;

  if (!oform)
    oform = "%.3f";
  if (bin_in || bin_out)
    emess(1,"binary I/O not available in -V option");  
  for (;;) {
    ++emess_dat.File_line;
    if (!(s = fgets(line, MAX_LINE, fid)))
      break;
    if (!strchr(s, '\n')) { /* overlong line */
      int c;
      (void)strcat(s, "\n");
      /* gobble up to newline */
      while ((c = fgetc(fid)) != EOF && c != '\n') ;
    }
    if (*s == tag) { /* pass on data */
      (void)fputs(s, stdout);
      continue;
    }
    /* check to override default input mode */
    if (*s == 'I' || *s == 'i') {
      linvers = 1;
      ++s;
    } else if (*s == 'I' || *s == 'i') {
      linvers = 0;
      ++s;
    } else
      linvers = inverse;
    if (linvers) {
      if (!PROJ_INVERS(Proj)) {
        emess(-1,"inverse for this projection not avail.\n");
        continue;
      }
      dat_xy.u = strtod(s, &s);
      dat_xy.v = strtod(s, &s);
      if (dat_xy.u == HUGE_VAL || dat_xy.v == HUGE_VAL) {
        emess(-1,"lon-lat input conversion failure\n");
        continue;
      }
      if (prescale) { dat_xy.u *= fscale; dat_xy.v *= fscale; }
      dat_ll = proj_inv(dat_xy, Proj);
    } else {
      dat_ll.u = proj_dmstor(s, &s);
      dat_ll.v = proj_dmstor(s, &s);
      if (dat_ll.u == HUGE_VAL || dat_ll.v == HUGE_VAL) {
        emess(-1,"lon-lat input conversion failure\n");
        continue;
      }
      dat_xy = proj_fwd(dat_ll, Proj);
      if (postscale) { dat_xy.u *= fscale; dat_xy.v *= fscale; }
    }
    if (proj_errno) {
      emess(-1, proj_strerrno(proj_errno));
      continue;
    }
    if (!*s && (s > line)) --s; /* assumed we gobbled \n */
    if (proj_factors(dat_ll, Proj, 0., &facs)) {
      emess(-1,"failed to conpute factors\n\n");
      continue;
    }
    if (*s != '\n')
      (void)fputs(s, stdout);
    (void)fputs("Longitude: ", stdout);
    (void)fputs(proj_rtodms(pline, dat_ll.u, "EW"), stdout);
    (void)printf(" [ %.11g ]\n", dat_ll.u * RAD_TO_DEG);
    (void)fputs("Latitude:  ", stdout);
    (void)fputs(proj_rtodms(pline, dat_ll.v, "NS"), stdout);
    (void)printf(" [ %.11g ]\n", dat_ll.v * RAD_TO_DEG);
    (void)fputs("Easting (x):   ", stdout);
    (void)printf(oform, dat_xy.u); putchar('\n');
    (void)fputs("Northing (y):  ", stdout);
    (void)printf(oform, dat_xy.v); putchar('\n');
    (void)printf("Meridian scale (h)%c: %.8f  ( %.4g %% error )\n",
      facs.code & IS_ANAL_HK ? '*' : ' ', facs.h, (facs.h-1.)*100.);
    (void)printf("Parallel scale (k)%c: %.8f  ( %.4g %% error )\n",
      facs.code & IS_ANAL_HK ? '*' : ' ', facs.k, (facs.k-1.)*100.);
    (void)printf("Areal scale (s):     %.8f  ( %.4g %% error )\n",
      facs.s, (facs.s-1.)*100.);
    (void)printf("Angular distortion (w): %.3f\n", facs.omega *
      RAD_TO_DEG);
    (void)printf("Meridian/Parallel angle: %.5f\n",
      facs.thetap * RAD_TO_DEG);
    (void)printf("Convergence%c: ",facs.code & IS_ANAL_CONV ? '*' : ' ');
    (void)fputs(proj_rtodms(pline, facs.conv, 0), stdout);
    (void)printf(" [ %.8f ]\n", facs.conv * RAD_TO_DEG);
    (void)printf("Max-min (Tissot axis a-b) scale error: %.5f %.5f\n\n",
      facs.a, facs.b);
  }
}
  int
main(int argc, char **argv) {
  char *arg, **eargv = argv, *pargv[MAX_PARGS];
  FILE *fid;
  int pargc = 0, eargc = 0, c, mon = 0;

  emess_dat.Prog_name = *argv;
  inverse = ! strncmp(emess_dat.Prog_name, "inv", 3);
  if (argc <= 1 ) {
    (void)fprintf(stderr, usage, emess_dat.Prog_name);
    exit (0);
  }
    /* process run line arguments */
  while (--argc > 0) { /* collect run line arguments */
    if(**++argv == '-') for(arg = *argv;;) {
      switch(*++arg) {
      case '\0': /* position of "stdin" */
        if (arg[-1] == '-') eargv[eargc++] = "-";
        break;
      case 'b': /* binary I/O */
        bin_in = bin_out = 1;
        continue;
      case 'v': /* monitor dump of initialization */
        mon = 1;
        continue;
      case 'i': /* input binary */
        bin_in = 1;
        continue;
      case 'o': /* output binary */
        bin_out = 1;
        continue;
      case 'I': /* alt. method to spec inverse */
        inverse = 1;
        continue;
      case 'E': /* echo ascii input to ascii output */
        echoin = 1;
        continue;
      case 'V': /* very verbose processing mode */
        very_verby = 1;
        mon = 1;
      case 'S': /* compute scale factors */
        dofactors = 1;
        continue;
      case 't': /* set col. one char */
        if (arg[1]) tag = *++arg;
        else emess(1,"missing -t col. 1 tag");
        continue;
      case 'l': /* list projections, ellipses or units */
        if (!arg[1] || arg[1] == 'p' || arg[1] == 'P') {
          /* list projections */
          const struct PROJ_LIST *lp;
          int do_long = arg[1] == 'P';
          char *str;

          for (lp = proj_list ; lp->id ; ++lp) {
            (void)printf("%s : ", lp->id);
            if (do_long)  /* possibly multiline description */
              (void)puts(*lp->descr);
            else { /* first line, only */
              str = *lp->descr;
              while ((c = *str++) && c != '\n')
                putchar(c);
              putchar('\n');
            }
          }
        } else if (arg[1] == '=') { /* list projection 'descr' */
          const struct PROJ_LIST *lp;

          arg += 2;
          for (lp = proj_list ; lp->id ; ++lp)
            if (!strcmp(lp->id, arg)) {
              (void)printf("%9s : %s\n", lp->id, *lp->descr);
              break;
            }
        } else if (arg[1] == 'e') { /* list ellipses */
          const struct PROJ_ELLPS *le;

          for (le = proj_ellps; le->id ; ++le)
            (void)printf("%9s %-16s %-16s %s\n",
              le->id, le->major, le->ell, le->name);
        } else if (arg[1] == 'u') { /* list units */
          const struct PROJ_UNITS *lu;

          for (lu = proj_units; lu->id ; ++lu)
            (void)printf("%12s %-20s %s\n",
              lu->id, lu->to_meter, lu->name);
        } else
          emess(1,"invalid list option: l%c",arg[1]);
        exit(0);
        continue; /* artificial */
      case 'e': /* error line alternative */
        if (--argc <= 0)
noargument:         emess(1,"missing argument for -%c",*arg);
        oterr = *++argv;
        continue;
      case 'm': /* cartesian multiplier */
        if (--argc <= 0) goto noargument;
        postscale = 1;
        if (!strncmp("1/",*++argv,2) || 
            !strncmp("1:",*argv,2)) {
          if((fscale = atof((*argv)+2)) == 0.)
            goto badscale;
          fscale = 1. / fscale;
        } else
          if ((fscale = atof(*argv)) == 0.) {
badscale:
                emess(1,"invalid scale argument");
          }
        continue;
      case 'W': /* specify seconds precision */
      case 'w': /* -W for constant field width */
        if ((c = arg[1]) != 0 && isdigit(c)) {
          proj_set_rtodms(c - '0', *arg == 'W');
          ++arg;
        } else
            emess(1,"-W argument missing or non-digit");
        continue;
      case 'f': /* alternate output format degrees or xy */
        if (--argc <= 0) goto noargument;
        oform = *++argv;
        continue;
      case 'r': /* reverse input */
        reversein = 1;
        continue;
      case 's': /* reverse output */
        reverseout = 1;
        continue;
      default:
        emess(1, "invalid option: -%c",*arg);
        break;
      }
      break;
    } else if (**argv == '+') { /* + argument */
      if (pargc < MAX_PARGS)
        pargv[pargc++] = *argv + 1;
      else
        emess(1,"overflowed + argument table");
    } else /* assumed to be input file name(s) */
      eargv[eargc++] = *argv;
  }
  if (eargc == 0) /* if no specific files force sysin */
    eargv[eargc++] = "-";
    /* done with parameter and control input */
  if (inverse && postscale) {
    prescale = 1;
    postscale = 0;
    fscale = 1./fscale;
  }
  if (!(Proj = proj_init(pargc, pargv)))
    emess(3,"projection initialization failure\ncause: %s",
      proj_strerrno(proj_errno));
  if (inverse) {
    if (!Proj->inv)
      emess(3,"inverse projection not available");
    proj = proj_inv;
  } else
    proj = proj_fwd;
  /* set input formating control */
  if (mon) {
    proj_pr_list(Proj);
    if (very_verby) {
      (void)printf("#Final Earth figure: ");
      if (Proj->es) {
        (void)printf("ellipsoid\n#  Major axis (a): ");
        (void)printf(oform ? oform : "%.3f", Proj->a);
        (void)printf("\n#  1/flattening: %.6f\n",
          1./(1. - sqrt(1. - Proj->es)));
        (void)printf("#  squared eccentricity: %.12f\n", Proj->es);
      } else {
        (void)printf("sphere\n#  Radius: ");
        (void)printf(oform ? oform : "%.3f", Proj->a);
        (void)putchar('\n');
      }
    }
  }
  if (inverse)
    informat = strtod;
  else {
    informat = proj_dmstor;
    if (!oform)
      oform = "%.2f";
  }
  /* process input file list */
  for ( ; eargc-- ; ++eargv) {
    if (**eargv == '-') {
      fid = stdin;
      emess_dat.File_name = "<stdin>";
    } else {
      if ((fid = fopen(*eargv, "r")) == NULL) {
        emess(-2, *eargv, "input file");
        continue;
      }
      emess_dat.File_name = *eargv;
    }
    emess_dat.File_line = 0;
    if (very_verby)
      vprocess(fid);
    else
      process(fid);
    (void)fclose(fid);
    emess_dat.File_name = 0;
  }
  return 0; /* normal completion */
}
/*
** Log: lproj.c
** Revision 1.2  2008-11-07 21:40:43  jeff
** ENH: Fixing some proj.4 warnings.
**
** Revision 1.1  2008-11-07 16:41:13  jeff
** ENH: Adding a 2D geoview. Adding the geographic projection library libproj4
** to Utilities. Updating the architecture of the geospatial views. All
** multi-resolution sources are now subclasses of vtkGeoSource. Each source
** has its own worker thread for fetching refined images or geometry.
** On the 3D side, vtkGeoGlobeSource is an appropriate source for vtkGeoTerrain,
** and vtkGeoAlignedImageSource is an appropriate source for
** vtkGeoAlignedImageRepresentation. On the 2D side, vtkGeoProjectionSource is an
** appropriate source for vtkGeoTerrain2D, and the image source is the same.
**
** Revision 3.2  2006/01/24 02:00:12  gie
** additional corrections
**
** Revision 3.1  2006/01/11 02:41:14  gie
** Initial
**
*/
