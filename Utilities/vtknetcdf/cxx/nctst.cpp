/*********************************************************************
 * Copyright 2005, UCAR/Unidata See COPYRIGHT file for copying and
 * redistribution conditions.
 * 
 * This runs the C++ tests for netCDF.
 * 
 * $Id: nctst.cpp,v 1.28 2007/04/02 21:01:12 ed Exp $
 *********************************************************************/

#include <config.h>
#include <iostream>
using namespace std;

#include <string.h>
#include "netcdfcpp.h"

const char LAT[] = "lat";
const char LON[] = "lon";
const char FRTIME[] = "frtime";
const char TIMELEN1[] = "timelen";
const char P_NAME[] = "P";
const char PRES_MAX_WIND[] = "pressure at maximum wind";
const char LONG_NAME[] = "long_name";
const char UNITS[] = "units";
const char VALID_RANGE[] = "valid_range";
const char FILL_VALUE[] = "_FillValue";
const char DEGREES_NORTH[] = "degrees_north";
const char LONGITUDE[] = "longitude";
const char LATITUDE[] = "latitude";
const char HECTOPASCALS[] = "hectopascals";
const char DEGREES_EAST[] = "degrees_east";
const char HOURS[] = "hours";
const char FORECAST_TIME[] = "forecast time";
const char REFERENCE_TIME[] = "reference time";
const char REFTIME[] = "reftime";
const char TEXT_TIME[] = "text_time";
const char SCALARV[] = "scalarv";
const char SCALAR_ATT[] = "scalar_att";
const int SCALAR_VALUE = 1;
const char HISTORY[] = "history";
const char TITLE[] = "title";
const char HISTORY_STR[] = "created by Unidata LDM from NPS broadcast";
const char TITLE_STR[] = "NMC Global Product Set: Pressure at Maximum Wind";

const int NC_ERR = 2;
const int NLATS = 4;
const int NLONS = 3;
const int NFRTIMES = 2;
const int TIMESTRINGLEN = 20;
const int NRANGES = 2;

// These are data values written out by the gen() function, and read
// in again and checked by the read() function.
static float range[] = {0., 1500.};
static float lats[NLATS] = {-90, -87.5, -85, -82.5};
static float lons[NLONS] = {-180, -175, -170};
static int frtimes[NFRTIMES] = {12, 18};
static const char* s = "1992-3-21 12:00";
static float fill_value = -9999.0;
static float P_data[NFRTIMES][NLATS][NLONS] = {
   {{950, 951, 952}, {953, 954, 955}, {956, 957, 958}, {959, 960, 961}},
   {{962, 963, 964}, {965, 966, 967}, {968, 969, 970}, {971, 972, 973}}
};


// Check a string attribute to make sure it has the correct value.
int 
check_string_att(NcAtt *att, const char *theName, const char *value)
{
   if (!att->is_valid() || strncmp(att->name(), theName, strlen(theName)) || 
       att->type() != ncChar || att->num_vals() != (long)strlen(value)) 
      return NC_ERR;

   char *value_in = att->as_string(0);
   if (strncmp(value_in, value, strlen(value)))
      return NC_ERR;
   delete value_in;

   return 0;
}

// Check the units and long_name attributes of a var to make sure they
// are what is expected.
int
check_u_ln_atts(NcVar *var, const char *units, const char *long_name)
{
   NcAtt *att;

   if (!(att = var->get_att(UNITS)))
      return NC_ERR;
   if (check_string_att(att, UNITS, units))
      return NC_ERR;
   delete att;

   if (!(att = var->get_att(LONG_NAME)))
      return NC_ERR;
   if (check_string_att(att, LONG_NAME, long_name))
      return NC_ERR;
   delete att;

   return 0;
}

// This reads the netCDF file created by gen() and ensures that
// everything is there as expected.
int read(const char* path, NcFile::FileFormat format)
{
   NcAtt *att;

   // open the file
   NcFile nc(path); 

   if (!nc.is_valid())
      return NC_ERR;

   // Check the format.
   if (nc.get_format() != format)
      return NC_ERR;

   // Check the numbers of things.
   if (nc.num_dims() != 4 || nc.num_vars() != 6 || 
       nc.num_atts() != 2)  
      return NC_ERR;

   // Check the global attributes.
   if (!(att = nc.get_att(HISTORY)))
      return NC_ERR;
   if (check_string_att(att, HISTORY, HISTORY_STR))
      return NC_ERR;
   delete att;

   if (!(att = nc.get_att(TITLE)))
      return NC_ERR;
   if (check_string_att(att, TITLE, TITLE_STR))
      return NC_ERR;
   delete att;
   
   // Check the dimensions.
   NcDim *latDim, *lonDim, *frtimeDim, *timeLenDim;

   if (!(latDim = nc.get_dim(LAT)))
      return NC_ERR;
   if (!latDim->is_valid() || strncmp(latDim->name(), LAT, strlen(LAT)) || 
       latDim->size() != NLATS || latDim->is_unlimited()) 
      return NC_ERR;

   if (!(lonDim = nc.get_dim(LON)))
      return NC_ERR;
   if (!lonDim->is_valid() || strncmp(lonDim->name(), LON, strlen(LON)) || 
       lonDim->size() != NLONS || lonDim->is_unlimited()) 
      return NC_ERR;

   if (!(frtimeDim = nc.get_dim(FRTIME)))
      return NC_ERR;
   if (!frtimeDim->is_valid() || strncmp(frtimeDim->name(), FRTIME, strlen(FRTIME)) || 
       frtimeDim->size() != 2 || !frtimeDim->is_unlimited()) 
      return NC_ERR;

   if (!(timeLenDim = nc.get_dim(TIMELEN1)))
      return NC_ERR;
   if (!timeLenDim->is_valid() || strncmp(timeLenDim->name(), TIMELEN1, strlen(TIMELEN1)) || 
       timeLenDim->size() != TIMESTRINGLEN || timeLenDim->is_unlimited()) 
      return NC_ERR;

   // Check the coordinate variables.
   NcVar *latVar, *lonVar, *frtimeVar, *refTimeVar;

   // Get the latitude.
   if (!(latVar = nc.get_var(LAT)))
      return NC_ERR;

   // Check units and long name.
   if (check_u_ln_atts(latVar, DEGREES_NORTH, LATITUDE))
      return NC_ERR;

   // Get the longitude.
   if (!(lonVar = nc.get_var(LON)))
      return NC_ERR;

   // Check units and long name.
   if (check_u_ln_atts(lonVar, DEGREES_EAST, LONGITUDE))
      return NC_ERR;

   // Get the forecast time coordinate variable.
   if (!(frtimeVar = nc.get_var(FRTIME)))
      return NC_ERR;

   // Check units and long name.
   if (check_u_ln_atts(frtimeVar, HOURS, FORECAST_TIME))
      return NC_ERR;

   // Get the refTime coordinate variable.
   if (!(refTimeVar = nc.get_var(REFTIME)))
      return NC_ERR;

   // Check units and long name.
   if (check_u_ln_atts(refTimeVar, TEXT_TIME, REFERENCE_TIME))
      return NC_ERR;

   // Check the data variables.
   NcVar *pVar, *scalarVar;
     
   if (!(pVar = nc.get_var(P_NAME)))
      return NC_ERR;

   // Check units and long name.
   if (check_u_ln_atts(pVar, HECTOPASCALS, PRES_MAX_WIND))
      return NC_ERR;

   // Check the valid range, and check the values.
   if (!(att = pVar->get_att(VALID_RANGE)))
      return NC_ERR;
   if (!att->is_valid() || strncmp(att->name(), VALID_RANGE, strlen(VALID_RANGE)) || 
       att->type() != ncFloat || att->num_vals() != NRANGES) 
      return NC_ERR;
   float range_in[NRANGES] = {att->as_float(0), att->as_float(1)};
   if (range_in[0] != range[0] || range_in[1] != range[1])
      return NC_ERR;
   delete att;

   // Check the fill value, and check the value.
   if (!(att = pVar->get_att(FILL_VALUE)))
      return NC_ERR;
   if (!att->is_valid() || strncmp(att->name(), FILL_VALUE, strlen(FILL_VALUE)) || 
       att->type() != ncFloat || att->num_vals() != 1) 
      return NC_ERR;
   float fill_value_in = att->as_float(0);
   if (fill_value_in != fill_value)
      return NC_ERR;
   delete att;

   // Check the data in the pressure variable.
   float P_data_in[NFRTIMES][NLATS][NLONS];
   pVar->get(&P_data_in[0][0][0], NFRTIMES, NLATS, NLONS);
   for (int f = 0; f < NFRTIMES; f++)
      for (int la = 0; la < NLATS; la++)
	 for (int lo = 0; lo < NLONS; lo++)
	    if (P_data_in[f][la][lo] != P_data[f][la][lo])
	       return NC_ERR;

   // Get the scalar variable.
   if (!(scalarVar = nc.get_var(SCALARV)))
      return NC_ERR;

   // Check for the scalar attribute of the scalar variable and check its value.
   if (!(att = scalarVar->get_att(SCALAR_ATT)))
      return NC_ERR;
   if (!att->is_valid() || strncmp(att->name(), SCALAR_ATT, strlen(SCALAR_ATT)) || 
       att->type() != ncInt || att->num_vals() != 1) 
      return NC_ERR;
   int value_in = att->as_int(0);
   if (value_in != SCALAR_VALUE)
      return NC_ERR;
   delete att;

   // Check the value of the scalar variable.


   return 0;
}

int gen(const char* path, NcFile::FileFormat format)		// Generate a netCDF file
{

    NcFile nc(path, NcFile::Replace, NULL, 0, format); // Create, leave in define mode

    // Check if the file was opened successfully
    if (! nc.is_valid()) {
	cerr << "can't create netCDF file " << path << "\n";
	return NC_ERR;
    }

    // Create dimensions
    NcDim* latd = nc.add_dim(LAT, NLATS);
    NcDim* lond = nc.add_dim(LON, NLONS);
    NcDim* frtimed = nc.add_dim(FRTIME); // unlimited dimension
    NcDim* timelend = nc.add_dim(TIMELEN1, TIMESTRINGLEN); 

    // Create variables and their attributes
    NcVar* P = nc.add_var(P_NAME, ncFloat, frtimed, latd, lond);
    P->add_att(LONG_NAME, PRES_MAX_WIND);
    P->add_att(UNITS, HECTOPASCALS);
    P->add_att(VALID_RANGE, NRANGES, range);
    P->add_att(FILL_VALUE, fill_value);

    NcVar* lat = nc.add_var(LAT, ncFloat, latd);
    lat->add_att(LONG_NAME, LATITUDE);
    lat->add_att(UNITS, DEGREES_NORTH);

    NcVar* lon = nc.add_var(LON, ncFloat, lond);
    lon->add_att(LONG_NAME, LONGITUDE);
    lon->add_att(UNITS, DEGREES_EAST);

    NcVar* frtime = nc.add_var(FRTIME, ncLong, frtimed);
    frtime->add_att(LONG_NAME, FORECAST_TIME);
    frtime->add_att(UNITS, HOURS);

    NcVar* reftime = nc.add_var(REFTIME, ncChar, timelend);
    reftime->add_att(LONG_NAME, REFERENCE_TIME);
    reftime->add_att(UNITS, TEXT_TIME);

    NcVar* scalar = nc.add_var(SCALARV, ncInt);
    scalar->add_att(SCALAR_ATT, SCALAR_VALUE);

    // Global attributes
    nc.add_att(HISTORY, HISTORY_STR);
    nc.add_att(TITLE, TITLE_STR);

    // Start writing data, implictly leaves define mode

    lat->put(lats, NLATS);

    lon->put(lons, NLONS);

    frtime->put(frtimes, NFRTIMES);

    reftime->put(s, strlen(s));

    // We could write all P data at once with P->put(&P_data[0][0][0], P->edges()),
    // but instead we write one record at a time, to show use of setcur().
    long rec = 0;                                      // start at zero-th
    const long nrecs = 1;		               // # records to write
    P->put(&P_data[0][0][0], nrecs, NLATS, NLONS);           // write zero-th record
    P->set_cur(++rec);		                       // set to next record
    P->put(&P_data[1][0][0], nrecs, NLATS, NLONS); // write next record

    // close of nc takes place in destructor
    return 0;
}

/*
 * Convert pathname of netcdf file into name for CDL, by taking last component
 * of path and stripping off any extension.  The returned string is in static
 * storage, so copy it if you need to keep it.
 */
static char* 
cdl_name(const char* path)
{
    const char* cp = path + strlen(path);
    while (*(cp-1) != '/' && cp != path) // assumes UNIX path separator
	cp--;

    static char np[NC_MAX_NAME];
    strncpy(&np[0], cp, NC_MAX_NAME);

    char* ep = np + strlen(np);
    while (*ep != '.' && ep != np)
	ep--;
    if (*ep == '.')
      *ep = '\0';
    return np;
}

// A derived class, just like NcFile except knows how to "dump" its
// dimensions, variables, global attributes, and data in ASCII form.
class DumpableNcFile : public NcFile
{
  public:
    DumpableNcFile(const char* path, NcFile::FileMode mode = ReadOnly)
	: NcFile(path, mode) {} ;
    void dumpdims( void );
    void dumpvars( void );
    void dumpgatts( void );
    void dumpdata( void );
};

void DumpableNcFile::dumpdims( void )
{

    for (int n=0; n < num_dims(); n++) {
	NcDim* dim = get_dim(n);
	cout << "\t" << dim->name() << " = " ;
	if (dim->is_unlimited())
	  cout << "UNLIMITED" << " ;\t " << "// " << dim->size() <<
	    " currently\n";
	else
	  cout << dim->size() << " ;\n";
    }
}

void dumpatts(NcVar& var)
{
    NcToken vname = var.name();
    NcAtt* ap;
    for(int n = 0; (ap = var.get_att(n)); n++) {
	cout << "\t\t" << vname << ":" << ap->name() << " = " ;
	NcValues* vals = ap->values();
	cout << *vals << " ;" << endl ;
	delete ap;
	delete vals;
    }
}

void DumpableNcFile::dumpvars( void )
{
    int n;
    static const char* types[] =
      {"","byte","char","short","long","float","double"};
    NcVar* vp;

    for(n = 0; (vp = get_var(n)); n++) {
	cout << "\t" << types[vp->type()] << " " << vp->name() ;

	if (vp->num_dims() > 0) {
	    cout << "(";
	    for (int d = 0; d < vp->num_dims(); d++) {
		NcDim* dim = vp->get_dim(d);
		cout << dim->name();
		if (d < vp->num_dims()-1)
		  cout << ", ";		  
	    }
	    cout << ")";
	}
	cout << " ;\n";
	// now dump each of this variable's attributes
	dumpatts(*vp);
    }
}

void DumpableNcFile::dumpgatts( void )
{
    NcAtt* ap;
    for(int n = 0; (ap = get_att(n)); n++) {
	cout << "\t\t" << ":" << ap->name() << " = " ;
	NcValues* vals = ap->values();
	cout << *vals << " ;" << endl ;
	delete vals;
	delete ap;
    }
}

void DumpableNcFile::dumpdata( )
{
    NcVar* vp;
    for (int n = 0; (vp = get_var(n)); n++) {
	cout << " " << vp->name() << " = ";
	NcValues* vals = vp->values();
	cout << *vals << " ;" << endl ;
	delete vals;
    }
}

void dump(const char* path)
{
    DumpableNcFile nc(path);	// default is open in read-only mode

    cout << "netcdf " << cdl_name(path) << " {" << endl <<
	    "dimensions:" << endl ;

    nc.dumpdims();

    cout << "variables:" << endl;

    nc.dumpvars();

    if (nc.num_atts() > 0)
      cout << "// global attributes" << endl ;

    nc.dumpgatts();

    cout << "data:" << endl;

    nc.dumpdata();

    cout << "}" << endl;
}

/* Test everything for classic and 64-bit offsetfiles. If netcdf-4 is
 * included, that means another whole round of testing. */
#ifdef USE_NETCDF4
#define NUM_FORMATS (4)
#else
#define NUM_FORMATS (2)
#endif

int
main( void )	// test new netCDF interface
{

   cout << "*** Testing C++ API with " << NUM_FORMATS 
	<< " different netCDF formats.\n";

   // Set up the format constants.
   NcFile::FileFormat format[NUM_FORMATS] = {NcFile::Classic, NcFile::Offset64Bits
#ifdef USE_NETCDF4
					     , NcFile::Netcdf4, NcFile::Netcdf4Classic
#endif
   };

   // Set up the file names.
   char file_name[NUM_FORMATS][NC_MAX_NAME] = 
      {"nctst_classic.nc", "nctst_64bit_offset.nc"
#ifdef USE_NETCDF4
       , "nctst_netcdf4.nc", "nctst_netcdf4_classic.nc"
#endif
   };

   int errs = 0;
   for (int i = 0; i < NUM_FORMATS; i++)
   {
      if (gen(file_name[i], format[i]) || 
	  read(file_name[i], format[i]))
      {
	 cout << "*** FAILURE with file " << file_name[i] << "\n";
	 errs++;
      }
      else
	 cout << "*** SUCCESS with file " << file_name[i] << "\n";
   }

   cout << "\n*** Total number of failures: " << errs << "\n";
   if (errs)
      cout << "*** nctst FAILURE!\n";
   else
      cout << "*** nctst SUCCESS!\n";
      
   return errs;
}
