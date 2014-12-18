In order to build the JOGL rendering classes, you will need to have JOGL
libraries installed on your system. For that you can use Maven to download
for you the proper JARs.

For that you can run the following command lines:

$ mvn org.apache.maven.plugins:maven-dependency-plugin:2.1:get
   -DrepoUrl=http://download.java.net/maven/2/
   -Dartifact=org.jogamp.gluegen:gluegen-rt:2.0.2

$ mvn org.apache.maven.plugins:maven-dependency-plugin:2.1:get
   -DrepoUrl=http://download.java.net/maven/2/
   -Dartifact=org.jogamp.jogl:jogl-all-main:2.0.2

$ mvn org.apache.maven.plugins:maven-dependency-plugin:2.1:get
   -DrepoUrl=http://download.java.net/maven/2/
   -Dartifact=org.jogamp.jogl:jogl-all:2.0.2
