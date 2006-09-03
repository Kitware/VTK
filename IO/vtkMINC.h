/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMINC.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

Copyright (c) 2006 Atamai, Inc.

Use, modification and redistribution of the software, in source or
binary forms, are permitted provided that the following terms and
conditions are met:

1) Redistribution of the source code, in verbatim or modified
   form, must retain the above copyright notice, this license,
   the following disclaimer, and any notices that refer to this
   license and/or the following disclaimer.

2) Redistribution in binary form must include the above copyright
   notice, a copy of this license and the following disclaimer
   in the documentation or with other materials provided with the
   distribution.

3) Modified copies of the source code must be clearly marked as such,
   and must not be misrepresented as verbatim copies of the source code.

THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE SOFTWARE "AS IS"
WITHOUT EXPRESSED OR IMPLIED WARRANTY INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE.  IN NO EVENT SHALL ANY COPYRIGHT HOLDER OR OTHER PARTY WHO MAY
MODIFY AND/OR REDISTRIBUTE THE SOFTWARE UNDER THE TERMS OF THIS LICENSE
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, LOSS OF DATA OR DATA BECOMING INACCURATE
OR LOSS OF PROFIT OR BUSINESS INTERRUPTION) ARISING IN ANY WAY OUT OF
THE USE OR INABILITY TO USE THE SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGES.

=========================================================================*/

/* 
   This file has been modified from the original "minc.h" that was
   distributed with minc version 1.4.  All of the prototype
   declarations have been removed.  This modification was made in
   support of the use of this file with the vtkMINCReader
   and vtkMINCWriter.

   David Gobbi, Atamai Inc..
*/

#ifndef vtkMINC_h
#define vtkMINC_h

/* Added since the file moved to VTK */
#define MIident       "ident"

/* Internally, attributes are stored as named arrays where
 * the format of the name is "/minc/variable/attribute" rather
 * than the typical NetCDF format "variable:attribute".
 */
#define MI_GRPNAME    "/minc"
#define MI_GRP_SEP    "/"
#define MI_ATT_SEP    "/"

/* ----------------------------- MNI Header -----------------------------------
@NAME       : minc.h
@DESCRIPTION: Header file for minc (Medical Image NetCDF) file format 
              standard.
@METHOD     : Includes name definitions for NetCDF dimensions, attributes
              and variables for the following :
                 NetCDF standard attributes
                 MI general variable attributes
                 MI dimensions and associated variables
                 MI dimension variable attributes
                 MI root variable
                 MI image variable
                 MI patient variable
                 MI study variable
                 MI acquisition variable
@CREATED    : July 24, 1992. (Peter Neelin, Montreal Neurological Institute)
@MODIFIED   : 
 * Log: vtkMINC.h
 * Revision 1.1  2006/09/03 01:01:21  dgobbi
 * ENH: move the MINC attributes into a separate file to make it easy to copy from the reader to the writer
 *
 * Revision 1.1  2006/08/30 16:03:22  dgobbi
 * ENH: Add a vtkMINC.h that contains all the MINC constants.  Make the MINCReader and MINCWriter use it.
 *
 * Revision 6.11.2.2  2004/09/28 20:23:40  bert
 * Minor portability fixes for Windows
 *
 * Revision 6.11.2.1  2004/08/11 21:03:06  bert
 * Fix MI_MAX_IMGDIMS at 100 to avoid netCDF 3.5.1 problem
 *
 * Revision 6.11  2004/04/15 21:13:21  bert
 * Add C++ linkage specifier
 *
 * Revision 6.10  2004/02/02 18:22:34  bert
 * Added miget_version() and miappend_history()
 *
 * Revision 6.9  2003/03/17 16:03:28  bert
 * Added declaration of micreate_tempfile()
 *
 * Revision 6.8  2001/11/13 14:15:17  neelin
 * Added functions miget_image_range and mivar_exists
 *
 * Revision 6.7  2001/08/20 13:19:14  neelin
 * Added function miattget_with_sign to allow the caller to specify the sign
 * of the input attribute since this information is ambiguous. This is
 * necessary for the valid_range attribute which should have the same sign
 * as the image data. Modified miget_valid_range to make use of this function.
 *
 * Revision 6.6  2001/08/16 16:41:31  neelin
 * Added library functions to handle reading of datatype, sign and valid range,
 * plus writing of valid range and setting of default ranges. These functions
 * properly handle differences between valid_range type and image type. Such
 * difference can cause valid data to appear as invalid when double to float
 * conversion causes rounding in the wrong direction (out of range).
 * Modified voxel_loop, volume_io and programs to use these functions.
 *
 * Revision 6.5  2001/08/16 13:32:18  neelin
 * Partial fix for valid_range of different type from image (problems
 * arising from double to float conversion/rounding). NOT COMPLETE.
 *
 * Revision 6.4  2001/04/24 13:38:40  neelin
 * Replaced NC_NAT with MI_ORIGINAL_TYPE.
 *
 * Revision 6.3  2001/04/17 18:40:13  neelin
 * Modifications to work with NetCDF 3.x
 * In particular, changed NC_LONG to NC_INT (and corresponding longs to ints).
 * Changed NC_UNSPECIFIED to NC_NAT.
 * A few fixes to the configure script.
 *
 * Revision 6.2  2001/04/10 22:05:30  neelin
 * Start of modifications to get minc working with netcdf 3.5.
 *
 * Revision 6.1  1999/10/19 14:45:08  neelin
 * Fixed Log subsitutions for CVS
 *
 * Revision 6.0  1997/09/12 13:24:54  neelin
 * Release of minc version 0.6
 *
 * Revision 5.0  1997/08/21  13:25:53  neelin
 * Release of minc version 0.5
 *
 * Revision 4.0  1997/05/07  20:07:52  neelin
 * Release of minc version 0.4
 *
 * Revision 3.1  1995/06/12  17:37:05  neelin
 * Added MI_LABEL modality.
 *
 * Revision 3.0  1995/05/15  19:33:12  neelin
 * Release of minc version 0.3
 *
 * Revision 2.4  1995/01/24  08:34:30  neelin
 * Added optional tempfile argument to miexpand_file.
 *
 * Revision 2.3  95/01/23  08:28:31  neelin
 * Changed name of midecompress_file to miexpand_file.
 * 
 * Revision 2.2  95/01/20  15:21:16  neelin
 * Added midecompress_file with ability to decompress only the header of a file.
 * 
 * Revision 2.1  94/11/25  15:32:32  neelin
 * Added #undef for public if it wasn't previously defined so that C++
 * code won't get upset.
 * 
 * Revision 2.0  94/09/28  10:38:00  neelin
 * Release of minc version 0.2
 * 
 * Revision 1.29  94/09/28  10:37:24  neelin
 * Pre-release
 * 
 * Revision 1.28  93/11/03  13:08:37  neelin
 * Added prototypes for miopen, miclose, micreate.
 * 
 * Revision 1.27  93/11/03  12:29:11  neelin
 * Added error code for failure to uncompress a file.
 * 
 * Revision 1.26  93/08/11  12:06:34  neelin
 * Added RCS logging in source.
 * 
              July 15, 1993 (P.N.)
                 - added MI_ICV_DO_FILLVALUE and MI_FILLVALUE
@COPYRIGHT  :
              Copyright 1993 Peter Neelin, McConnell Brain Imaging Centre, 
              Montreal Neurological Institute, McGill University.
              Permission to use, copy, modify, and distribute this
              software and its documentation for any purpose and without
              fee is hereby granted, provided that the above copyright
              notice appear in all copies.  The author and McGill University
              make no representations about the suitability of this
              software for any purpose.  It is provided "as is" without
              express or implied warranty.
@RCSID      : Header MINC (MNI)
---------------------------------------------------------------------------- */

/* Some useful constants */
#define MI_EMPTY_STRING ""
/* Error flags */
#define MI_ERROR (-1)
#define MI_NOERROR 0
/* Maximum length of standard attributes */
#define MI_MAX_ATTSTR_LEN  64
/* Number of spatial dimensions */
#define MI_NUM_SPACE_DIMS 3
/* Maximum number of image dimensions for image conversion */

/* Bert 10-Aug-2004 - MI_MAX_IMGDIMS used to be defined to be MAX_VAR_DIMS,
 * a constant defined in netcdf.h. For many years MAX_VAR_DIMS was 100,
 * but in netCDF 3.5.1 the value was changed to 512.
 * Unfortunately, the definitions of MI_ICV_DIM_SIZE, MI_ICV_DIM_STEP,
 * and MI_ICV_DIM_START assume that MI_MAX_IMGDIMS is less than or
 * equal to 100.  To avoid changing the MINC API, we have to define
 * MI_MAX_IMGDIMS to 100 here.  Otherwise the miicv_inqdbl() function
 * will return bogus values for these ICV properties.
 */
#define MI_MAX_IMGDIMS 100

/* NetCDF standard attributes */
#define MIunits       "units"
#define MIlong_name   "long_name"
#define MIvalid_range "valid_range"
#define MIvalid_max   "valid_max"
#define MIvalid_min   "valid_min"
#define MI_FillValue  "_FillValue"
#define MItitle       "title"
#define MIhistory     "history"

/* General variable attributes */
#define MIvartype  "vartype"
#define MIvarid    "varid"
#define MIsigntype "signtype"
#define MIparent   "parent"
#define MIchildren "children"
#define MIcomments "comments"
#define MIversion  "version"

/* General attribute constants */
/*    Prefix for identifying a variable attribute pointer */
#define MI_VARATT_POINTER_PREFIX "--->"
/*    Separator for elements of MIchildren */
#define MI_CHILD_SEPARATOR "\n"
/*    MIvartype values */
#define MI_GROUP     "group________"
#define MI_DIMENSION "dimension____"
#define MI_DIM_WIDTH "dim-width____"
#define MI_VARATT    "var_attribute"
/*    MIvarid value */
#define MI_STDVAR "MINC standard variable"
/*    MIsigntype values */
#define MI_SIGNED   "signed__"
#define MI_UNSIGNED "unsigned"
/*    MIversion value */
#define MI_VERSION_1_0 "MINC Version    1.0"
#define MI_CURRENT_VERSION MI_VERSION_1_0
/* Generally useful values for boolean attributes */
#define MI_TRUE  "true_"
#define MI_FALSE "false"

/* Dimension names and names of associated variables */
#define MIxspace           "xspace"
#define MIyspace           "yspace"
#define MIzspace           "zspace"
#define MItime             "time"
#define MItfrequency       "tfrequency"
#define MIxfrequency       "xfrequency"
#define MIyfrequency       "yfrequency"
#define MIzfrequency       "zfrequency"
#define MIvector_dimension "vector_dimension"
#define MIxspace_width     "xspace-width"
#define MIyspace_width     "yspace-width"
#define MIzspace_width     "zspace-width"
#define MItime_width       "time-width"
#define MItfrequency_width "tfrequency-width"
#define MIxfrequency_width "xfrequency-width"
#define MIyfrequency_width "yfrequency-width"
#define MIzfrequency_width "zfrequency-width"

/* Dimension variable attribute names */
/* For dimension variables (MIspacing is also for dimension width vars) */
#define MIspacing           "spacing"
#define MIstep              "step"
#define MIstart             "start"
#define MIspacetype         "spacetype"
#define MIalignment         "alignment"
#define MIdirection_cosines "direction_cosines"
/* For dimension width variables */
#define MIwidth             "width"
#define MIfiltertype        "filtertype"

/* Dimension attribute constants */
/*    MIgridtype values */
#define MI_REGULAR   "regular__"
#define MI_IRREGULAR "irregular"
/*    MIspacetype values */
#define MI_NATIVE    "native____"
#define MI_TALAIRACH "talairach_"
#define MI_CALLOSAL  "callosal__"
/*    MIalignment values */
#define MI_START  "start_"
#define MI_CENTRE "centre"
#define MI_END    "end___"
#define MI_CENTER MI_CENTRE
/*    MIfiltertype values */
#define MI_SQUARE     "square____"
#define MI_GAUSSIAN   "gaussian__"
#define MI_TRIANGULAR "triangular"

/* The root variable */
#define MIrootvariable "rootvariable"

/* The image variable and its attributes */
#define MIimage    "image"
#define MIimagemax "image-max"
#define MIimagemin "image-min"
#define MIcomplete "complete"

/* The patient variable and its attributes */
#define MIpatient        "patient"
#define MIfull_name      "full_name"
#define MIother_names    "other_names"
#define MIidentification "identification"
#define MIother_ids      "other_ids"
#define MIbirthdate      "birthdate"
#define MIsex            "sex"
#define MIage            "age"
#define MIweight         "weight"
#define MIsize           "size"
#define MIaddress        "address"
#define MIinsurance_id   "insurance_id"

/* Patient attribute constants */
#define MI_MALE   "male__"
#define MI_FEMALE "female"
#define MI_OTHER  "other_"

/* The study variable and its attributes */
#define MIstudy               "study"
#define MIstart_time          "start_time"
#define MIstart_year          "start_year"
#define MIstart_month         "start_month"
#define MIstart_day           "start_day"
#define MIstart_hour          "start_hour"
#define MIstart_minute        "start_minute"
#define MIstart_seconds       "start_seconds"
#define MImodality            "modality"
#define MImanufacturer        "manufacturer"
#define MIdevice_model        "device_model"
#define MIinstitution         "institution"
#define MIdepartment          "department"
#define MIstation_id          "station_id"
#define MIreferring_physician "referring_physician"
#define MIattending_physician "attending_physician"
#define MIradiologist         "radiologist"
#define MIoperator            "operator"
#define MIadmitting_diagnosis "admitting_diagnosis"
#define MIprocedure           "procedure"
#define MIstudy_id            "study_id"

/* Study attribute constants */
#define MI_PET   "PET__"
#define MI_SPECT "SPECT"
#define MI_GAMMA "GAMMA"
#define MI_MRI   "MRI__"
#define MI_MRS   "MRS__"
#define MI_MRA   "MRA__"
#define MI_CT    "CT___"
#define MI_DSA   "DSA__"
#define MI_DR    "DR___"
#define MI_LABEL "label"

/* The acquisition variable and its attributes */
#define MIacquisition           "acquisition"
#define MIprotocol              "protocol"
#define MIscanning_sequence     "scanning_sequence"
#define MIrepetition_time       "repetition_time"
#define MIecho_time             "echo_time"
#define MIinversion_time        "inversion_time"
#define MInum_averages          "num_averages"
#define MIimaging_frequency     "imaging_frequency"
#define MIimaged_nucleus        "imaged_nucleus"
#define MIradionuclide          "radionuclide"
#define MIcontrast_agent        "contrast_agent"
#define MIradionuclide_halflife "radionuclide_halflife"
#define MItracer                "tracer"
#define MIinjection_time        "injection_time"
#define MIinjection_year        "injection_year"
#define MIinjection_month       "injection_month"
#define MIinjection_day         "injection_day"
#define MIinjection_hour        "injection_hour"
#define MIinjection_minute      "injection_minute"
#define MIinjection_seconds     "injection_seconds"
#define MIinjection_length      "injection_length"
#define MIinjection_dose        "injection_dose"
#define MIdose_units            "dose_units"
#define MIinjection_volume      "injection_volume"
#define MIinjection_route       "injection_route"

/* End ifndef vtkMINC_h */
#endif
