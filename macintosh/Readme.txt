vtk on osx Installation Instructions

1) Get the most recent Python 2.1 distribution, build and install it.

### Get the Python source tarball (change the URL as needed)
wget http://www.python.org/ftp/python/2.1/Python-2.1b2a.tgz
### Decompress and extract it.
gnutar xvcf Python-2.1b2a.tgz
### Change to the Python directory.
cd Python-2.1b2a
### Configure Python.
./configure --with-suffix=.exe --with-dyld
### Edit Module/setup to uncomment the line that says #*shared*
### Make Python.
make OPT="-traditional-cpp"
### Install Python (enter root password).
sudo make install
### Make a symlink to get rid of the ugly Python.exe kludge.
sudo ln -s /usr/local/bin/python2.1.exe /usr/local/bin/python

2) copy the enclosed makefiles into their proper directories, replacing
	the ones already there.

3) ./configure --with-quartz --with-shared --with-python --with-patented --with-contrib

4) make
5) sudo make install

6) Fix the DYLD_LIBRARY_PATH environment variable.
Create a Library/init/tcsh directory in your home directory
Create an environment.mine file in that directory
Edit it to include the setenv DYLD_LIBRARY_PATH line to find the modules.

7) Copy the nib directory into /usr/local/lib/
(that's where its hard coded to look for it right now)

Let me know how this works for you and if you have any improvements.
Yves Starreveld
ystarrev@uwo.ca