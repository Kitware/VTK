# Java instructions

## Building

In order to build the JOGL rendering classes, you will need to have JOGL
libraries installed on your system. For that you can use Maven to download
for you the proper JARs.

List of possible classifiers:

 - natives-android-aarch64
 - natives-android-armv6
 - natives-linux-amd64
 - natives-linux-armv6
 - natives-linux-armv6hf
 - natives-linux-i586
 - natives-macosx-universal
 - natives-solaris-amd64
 - natives-solaris-i586
 - natives-windows-amd64
 - natives-windows-i586


For that you can run the following command lines:

```
mvn org.apache.maven.plugins:maven-dependency-plugin:2.8:get \
  -DrepoUrl=http://download.java.net/maven/2/  \
  -Dartifact=org.jogamp.gluegen:gluegen-rt:2.3.2

mvn org.apache.maven.plugins:maven-dependency-plugin:2.8:get \
  -DrepoUrl=http://download.java.net/maven/2/  \
  -Dartifact=org.jogamp.gluegen:gluegen-rt:2.3.2:jar:CLASSIFIER

mvn org.apache.maven.plugins:maven-dependency-plugin:2.8:get \
  -DrepoUrl=http://download.java.net/maven/2/  \
  -Dartifact=org.jogamp.jogl:jogl-all:2.3.2

mvn org.apache.maven.plugins:maven-dependency-plugin:2.8:get \
  -DrepoUrl=http://download.java.net/maven/2/  \
  -Dartifact=org.jogamp.jogl:jogl-all:2.3.2:jar:CLASSIFIER

cmake --build build --source vtk-source \
  -DVTK_WRAP_JAVA=ON \
  -DVTK_JAVA_JOGL_COMPONENT=ON \
  -DJOGL_VERSION="2.3.2" \
  -DJOGL_GLUE=$HOME/.m2/repository/org/jogamp/gluegen/gluegen-rt/2.3.2/gluegen-rt-2.3.2.jar \
  -DJOGL_LIB=$HOME/.m2/repository/org/jogamp/jogl/jogl-all/2.3.2/jogl-all-2.3.2.jar

# Substitute $INSTALLDIR
cmake --install build --prefix $INSTALLDIR
```

## Demonstration
```
java -cp $INSTALLDIR/vtk-XY.jar:/home/kitware/.m2/repository/org/jogamp/gluegen/gluegen-rt/2.3.2/gluegen-rt-2.3.2.jar:/home/kitware/.m2/repository/org/jogamp/jogl/jogl-all/2.3.2/jogl-all-2.3.2.jar -Djava.library.path=$INSTALLDIR/natives-Linux-64bit vtk.sample.Demo
```
