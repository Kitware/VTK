# Dashboard is opened for submissions for a 24 hour period starting at
# the specified NIGHLY_START_TIME. Time is specified in 24 hour format.
SET (NIGHTLY_START_TIME "23:00:00 EST")

# Dart server to submit results (used by client)
SET (DROP_SITE "public.kitware.com")
SET (DROP_LOCATION "/incoming")
SET (DROP_SITE_USER "ftpuser")
SET (DROP_SITE_PASSWORD "public")
SET (TRIGGER_SITE 
       "http://${DROP_SITE}/cgi-bin/Submit-vtk-TestingResults.pl")

# Dart server configuration 
SET (CVS_WEB_URL "http://${DROP_SITE}/cgi-bin/cvsweb.cgi/VTK/")
SET (CVS_WEB_CVSROOT "VTK")
SET (USE_DOXYGEN "On")
SET (DOXYGEN_URL "http://${DROP_SITE}/VTK/doc/nightly/html/" )
SET (GNATS_WEB_URL "http://${DROP_SITE}/")

# copy over the testing logo
CONFIGURE_FILE(${VTK_SOURCE_DIR}/TestingLogo.gif ${VTK_BINARY_DIR}/Testing/HTML/TestingResults/Icons/Logo.gif COPYONLY)
