The ThirdParty/netcdf/vtknetcdf directory contains a reduced
distribution of the NetCDF source tree with only the library
source code needed by VTK.  It is not a submodule; the actual
content is part of our source tree and changes can be made and
committed directly.

We update from upstream using Git's "subtree" merge strategy.  A
special branch contains commits of upstream netcdf snapshots and
nothing else.  No Git ref points explicitly to the head of this
branch, but it is merged into our history.

Update netcdf from upstream as follows.  Create a local branch to
explicitly reference the upstream snapshot branch head:

 git branch netcdf-upstream c3ace7de

Use a temporary directory to checkout the branch:

 mkdir netcdf-tmp
 cd netcdf-tmp
 git init
 git pull .. netcdf-upstream
 rm -rf *

Now place the (reduced) netcdf content in this directory.  See
instructions shown by

 git log c3ace7de

for help extracting the content from the upstream tarball.  Then run
the following commands to commit the new version.  Substitute the
appropriate date and version number:

 git add --all

 GIT_AUTHOR_NAME='NetCDF Developers \
 GIT_AUTHOR_EMAIL='netcdfgroup@unidata.ucar.edu' \
 GIT_AUTHOR_DATE='2011-03-29 18:00:00 -0400' \
 git commit -m 'netcdf 4.1.2 (reduced)' &&
 git commit --amend

Edit the commit message to describe the procedure used to obtain the
content.  Be sure to add a Gerrit Change-Id line to the bottom of the
message.  Then push the changes back up to the main local repository:

 git push .. HEAD:netcdf-upstream
 cd ..
 rm -rf netcdf-tmp

Create a topic in the main repository on which to perform the update:

 git checkout -b update-netcdf master

Merge the netcdf-upstream branch as a subtree:

 git merge -s recursive -X subtree=ThirdParty/netcdf/vtknetcdf \
           netcdf-upstream

If there are conflicts, resolve them and commit.  Build and test the
tree.  Commit any additional changes needed to succeed.

Finally, run

 git rev-parse --short=8 netcdf-upstream

to get the commit from which the netcdf-upstream branch must be started
on the next update.  Edit the "git branch netcdf-upstream" line above to
record it, and commit this file.
