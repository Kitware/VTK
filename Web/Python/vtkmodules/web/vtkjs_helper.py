import base64
import os
import shutil
import sys
import zipfile

try:
  import zlib
  compression = zipfile.ZIP_DEFLATED
except:
  compression = zipfile.ZIP_STORED

# -----------------------------------------------------------------------------

def convertDirectoryToZipFile(directoryPath):
    if os.path.isfile(directoryPath):
        return

    zipFilePath = '%s.zip' % directoryPath
    zf = zipfile.ZipFile(zipFilePath, mode='w')

    try:
      for dirName, subdirList, fileList in os.walk(directoryPath):
        for fname in fileList:
          fullPath = os.path.join(dirName, fname)
          relPath = '%s' % (os.path.relpath(fullPath, directoryPath))
          zf.write(fullPath, arcname=relPath, compress_type=compression)
    finally:
        zf.close()

    shutil.rmtree(directoryPath)
    shutil.move(zipFilePath, directoryPath)

# -----------------------------------------------------------------------------

def addDataToViewer(dataPath, srcHtmlPath):
    if os.path.isfile(dataPath) and os.path.exists(srcHtmlPath):
        dstDir = os.path.dirname(dataPath)
        dstHtmlPath = os.path.join(dstDir, '%s.html' % os.path.basename(dataPath)[:-6])

        # Extract data as base64
        with open(dataPath, 'rb') as data:
            dataContent = data.read()
            base64Content = base64.b64encode(dataContent)
            base64Content = base64Content.decode().replace('\n', '')

        # Create new output file
        with open(srcHtmlPath, mode='r') as srcHtml:
            with open(dstHtmlPath, mode='w') as dstHtml:
                for line in srcHtml:
                    if '</body>' in line:
                        dstHtml.write('<script>\n')
                        dstHtml.write('var contentToLoad = "%s";\n\n' % base64Content);
                        dstHtml.write('Glance.importBase64Dataset("%s" , contentToLoad, glanceInstance.proxyManager);\n' % os.path.basename(dataPath));
                        dstHtml.write('glanceInstance.showApp();\n');
                        dstHtml.write('</script>\n')

                    dstHtml.write(line)

# -----------------------------------------------------------------------------
# Main
# -----------------------------------------------------------------------------

if __name__ == "__main__":
  if len(sys.argv) < 2:
      print("Usage: directoryToFile /path/to/directory.vtkjs [/path/to/ParaViewGlance.html]")
  else:
      fileName = sys.argv[1]
      convertDirectoryToZipFile(fileName)

      if len(sys.argv) == 3:
        addDataToViewer(fileName, sys.argv[2])
