
import re

componentRegex = re.compile(r"^[a-z][a-z0-9_]*$")

def checkURI(uri):
  """
  uri: lowercase, dot separated string.
  throws exception if invalid.
  returns: uri
  """

  components = uri.split('.')
  for component in components:
    match = componentRegex.match(component)
    if not match:
      raise Exception("invalid URI")
  return uri
