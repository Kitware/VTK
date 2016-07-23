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

$ mvn org.apache.maven.plugins:maven-dependency-plugin:2.8:get \
    -DrepoUrl=http://download.java.net/maven/2/  \
    -Dartifact=org.jogamp.gluegen:gluegen-rt:2.3.2

$ mvn org.apache.maven.plugins:maven-dependency-plugin:2.8:get \
    -DrepoUrl=http://download.java.net/maven/2/  \
    -Dartifact=org.jogamp.gluegen:gluegen-rt:2.3.2:jar:CLASSIFIER


$ mvn org.apache.maven.plugins:maven-dependency-plugin:2.8:get \
    -DrepoUrl=http://download.java.net/maven/2/  \
    -Dartifact=org.jogamp.jogl:jogl-all:2.3.2

$ mvn org.apache.maven.plugins:maven-dependency-plugin:2.8:get \
    -DrepoUrl=http://download.java.net/maven/2/  \
    -Dartifact=org.jogamp.jogl:jogl-all:2.3.2:jar:CLASSIFIER
