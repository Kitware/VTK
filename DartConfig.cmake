# Dashboard is opened for submissions for a 24 hour period starting at
# the specified NIGHLY_START_TIME. Time is specified in 24 hour format.
SET (NIGHTLY_START_TIME "23:00:00 EDT")

# Dart server to submit results (used by client)
SET (DROP_SITE "public.kitware.com")
SET (DROP_LOCATION "/incoming")
SET (DROP_SITE_USER "ftpuser")
SET (DROP_SITE_PASSWORD "public")
SET (TRIGGER_SITE 
       "http://${DROP_SITE}/cgi-bin/Submit-vtk-TestingResults.pl")

# Project Home Page
SET (PROJECT_URL "http://www.vtk.org/")

# Dart server configuration 
SET (ROLLUP_URL "http://${DROP_SITE}/cgi-bin/vtk-rollup-dashboard.sh")
SET (CVS_WEB_URL "http://${DROP_SITE}/cgi-bin/cvsweb.cgi/VTK/")
SET (CVS_WEB_CVSROOT "VTK")
SET (USE_DOXYGEN "On")
SET (DOXYGEN_URL "http://www.vtk.org/doc/nightly/html/" )
SET (GNATS_WEB_URL "http://${DROP_SITE}/")

# copy over the testing logo
CONFIGURE_FILE(${VTK_SOURCE_DIR}/TestingLogo.gif ${VTK_BINARY_DIR}/Testing/HTML/TestingResults/Icons/Logo.gif COPYONLY)

# Continuous email delivery variables
SET (CONTINUOUS_FROM "lorensen@crd.ge.com")
SET (SMTP_MAILHOST "public.kitware.com")
SET (CONTINUOUS_MONITOR_LIST "lorensen@crd.ge.com millerjv@crd.ge.com")
SET (CONTINUOUS_BASE_URL "http://www.vtk.org/Testing")

