# Project name. Used in some of the resulting file names and xrefs to
# uniquify two or more projects linked together through their Doxygen tag files.
# Example: PROJECT_NAME=@PROJECT_NAME@
#
export PROJECT_NAME=VTK

# Path to the directory holding the Perl scripts used to produce the VTK doc
# in Doxygen format.
# Example: PATH_TO_VTK_DOX_SCRIPTS=@VTK_SOURCE_DIR@/Utilities/Doxygen
#
export PATH_TO_VTK_DOX_SCRIPTS=.

# Source directory.
# Example: SOURCE_DIR=@SBVTK_SOURCE_DIR@
#
export SOURCE_DIR=../..

# Relative path from the source directory to the top most directory holding
# the files to document.
# Example: REL_PATH_TO_TOP=framework/src
#       or REL_PATH_TO_TOP=.
#
export REL_PATH_TO_TOP=.

# Directory where the intermediate Doxygen files should be stored (mainly
# the headers files converted from the VTK format to the Doxygen format).
# DOXTEMP might be used to simplify syntax.
# Example: DOXTEMP=DOXTEMP=@SBVTK_BINARY_DIR@/Utilities/Doxygen
#          INTERMEDIATE_DOX_DIR=$DOXTEMP/dox
#
export INTERMEDIATE_DOX_DIR=../../../$PROJECT_NAME-doxygen

# URL to the CVSWeb of the project, in checkout mode (i.e. appending a file 
# name to this URL will retrieve the contents of the file).
# Example: CVSWEB_CHECKOUT=http://public.kitware.com/cgi-bin/cvsweb.cgi/~checkout~/VTK
#
export CVSWEB_CHECKOUT=http://public.kitware.com/cgi-bin/cvsweb.cgi/~checkout~/VTK

# Path to the Doxygen configuration file (i.e. doxyfile).
# Example: DOXYFILE=$DOXTEMP/doxyfile
#
export DOXYFILE=$PATH_TO_VTK_DOX_SCRIPTS/doxyfile

# Path to the Doxygen output directory (where the resulting doc is stored).
# Example: RESULTING_DOC_DIR=$DOXTEMP/doc
#
export RESULTING_DOC_DIR=../../../doc

# Name of the resulting CHM (Compressed HTML) file. If set and non-empty this
# will actually trigger the HTML-Help compiler to create the CHM. The resulting
# file will be renamed to this name.
# Example: RESULTING_CHM_FILE=$DOXTEMP/SBVTKFramework.chm
#
# export RESULTING_CHM_FILE=vtk4nightly.chm

# Name, remote location and destination dir of the VTK 4.0 tag file. If set, the
# tag file is retrieved from its remote location using wget and stored in the
# destination dir. It will be automatically deleted at the end of this script.
# Example: VTK_TAGFILE=vtk4.tag
#          VTK_TAGFILE_REMOTE_DIR=http://public.kitware.com/VTK/doc/nightly/html
#          VTK_TAGFILE_DEST_DIR=$DOXTEMP
# The Doxygen configuration file must be tailored to make use of this tag file.
# Example: TAGFILES = vtk4.tag=http://public.kitware.com/VTK/doc/nightly/html
#
# export VTK_TAGFILE=vtk4.tag
# export VTK_TAGFILE_REMOTE_DIR=http://public.kitware.com/VTK/doc/nightly/html
# export VTK_TAGFILE_DEST_DIR=../../../$PROJECT_NAME-doxygen

# ----------------------------------------------------------------------------
# Convert the VTK headers to the Doxygen format.

perl $PATH_TO_VTK_DOX_SCRIPTS/doc_header2doxygen.pl \
        --to $INTERMEDIATE_DOX_DIR \
        $SOURCE_DIR/$REL_PATH_TO_TOP/Common \
        $SOURCE_DIR/$REL_PATH_TO_TOP/Filtering \
        $SOURCE_DIR/$REL_PATH_TO_TOP/Graphics \
        $SOURCE_DIR/$REL_PATH_TO_TOP/Hybrid \
        $SOURCE_DIR/$REL_PATH_TO_TOP/Imaging \
        $SOURCE_DIR/$REL_PATH_TO_TOP/IO \
        $SOURCE_DIR/$REL_PATH_TO_TOP/Parallel \
        $SOURCE_DIR/$REL_PATH_TO_TOP/Patented \
        $SOURCE_DIR/$REL_PATH_TO_TOP/Rendering

# ----------------------------------------------------------------------------
# Extract the VTK version and create the main page.

perl $PATH_TO_VTK_DOX_SCRIPTS/doc_version.pl \
        --header $SOURCE_DIR/$REL_PATH_TO_TOP/Common/vtkVersion.h \
        --logo "vtk-logo.gif" \
        --store "doc_""$PROJECT_NAME""_version.dox" \
        --to $INTERMEDIATE_DOX_DIR

# ----------------------------------------------------------------------------
# Generate the 'Class to Examples' page cross-linking each class to these
# examples that use that class.

perl $PATH_TO_VTK_DOX_SCRIPTS/doc_class2example.pl \
        --dirmatch "^Examples\$" \
        --label "Examples" \
        --link $CVSWEB_CHECKOUT \
        --project $PROJECT_NAME \
        --store "doc_""$PROJECT_NAME""_class2examples.dox" \
        --title "Class To Examples" \
        --to $INTERMEDIATE_DOX_DIR \
        --unique "e" \
        $SOURCE_DIR/$REL_PATH_TO_TOP

# ----------------------------------------------------------------------------
# Generate the 'Class to Tests' page cross-linking each class to these
# tests that use that class.

perl $PATH_TO_VTK_DOX_SCRIPTS/doc_class2example.pl \
        --dirmatch "^Testing$" \
        --label "Tests" \
        --link $CVSWEB_CHECKOUT \
        --project $PROJECT_NAME \
        --store "doc_""$PROJECT_NAME""_class2tests.dox" \
        --title "Class To Tests" \
        --to $INTERMEDIATE_DOX_DIR \
        --unique "t"
        $SOURCE_DIR/$REL_PATH_TO_TOP

# ----------------------------------------------------------------------------
# Build the full-text index.

perl $PATH_TO_VTK_DOX_SCRIPTS/doc_index.pl \
        --project $PROJECT_NAME \
        --stop  $PATH_TO_VTK_DOX_SCRIPTS/doc_index.stop \
        --store "doc_""$PROJECT_NAME""_index.dox" \
        --to $INTERMEDIATE_DOX_DIR \
        $SOURCE_DIR/$REL_PATH_TO_TOP/Common \
        $SOURCE_DIR/$REL_PATH_TO_TOP/Filtering \
        $SOURCE_DIR/$REL_PATH_TO_TOP/Graphics \
        $SOURCE_DIR/$REL_PATH_TO_TOP/Hybrid \
        $SOURCE_DIR/$REL_PATH_TO_TOP/Imaging \
        $SOURCE_DIR/$REL_PATH_TO_TOP/IO \
        $SOURCE_DIR/$REL_PATH_TO_TOP/Parallel \
        $SOURCE_DIR/$REL_PATH_TO_TOP/Patented \
        $SOURCE_DIR/$REL_PATH_TO_TOP/Rendering

# ----------------------------------------------------------------------------
# Retrieve the VTK 4 tag file.

if test "x$VTK_TAGFILE" != "x" ; then
    wget -nd -nH \
        $VTK_TAGFILE_REMOTE_DIR/$VTK_TAGFILE \
        -O $VTK_TAGFILE_DEST_DIR/$VTK_TAGFILE
fi

# ----------------------------------------------------------------------------
# Create the Doxygen doc.

doxygen $DOXYFILE

# ----------------------------------------------------------------------------
# Clean the HTML pages to remove the path to the intermediate Doxygen dir.

perl $PATH_TO_VTK_DOX_SCRIPTS/doc_rmpath.pl \
        --to $INTERMEDIATE_DOX_DIR \
        --html $RESULTING_DOC_DIR/html

# ----------------------------------------------------------------------------
# Create the CHM doc.

if test "x$RESULTING_CHM_FILE" != "x" ; then
    cd $RESULTING_DOC_DIR/html
    hhc index.hhp
    mv index.chm $RESULTING_CHM_FILE
fi

# ----------------------------------------------------------------------------
# Clean-up.

rm -fr $INTERMEDIATE_DOX_DIR

if test "x$VTK_TAGFILE" != "x" ; then
    rm -fr $VTK_TAGFILE_DEST_DIR/$VTK_TAGFILE
fi

if test "x$RESULTING_CHM_FILE" != "x" ; then
#    rm -fr $RESULTING_DOC_DIR
    true
fi


