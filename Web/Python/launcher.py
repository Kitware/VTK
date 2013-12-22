#!/usr/bin/env python

import json
import logging
import os
import string
import subprocess
import sys
import time
import uuid

from random import choice

from twisted.internet import reactor, defer
from twisted.internet.task import deferLater
from twisted.python import log
from twisted.web import server, resource, http
from twisted.web.resource import Resource
from twisted.web.server import NOT_DONE_YET
from twisted.web.static import File

from vtk.web import upload

try:
    import argparse
except ImportError:
    import _argparse as argparse


sample_config_file = """
Here is a sample of what a configuration file could looks like:

    {
      ## ===============================
      ## General launcher configuration
      ## ===============================

      "configuration": {
        "host" : "localhost",
        "port" : 8080,
        "endpoint": "paraview",                   # SessionManager Endpoint
        "content": "/.../www",                    # Optional: Directory shared over HTTP
        "proxy_file" : "/.../proxy-mapping.txt",  # Proxy-Mapping file for Apache
        "sessionURL" : "ws://${host}:${port}/ws", # ws url used by the client to connect to the started process
        "timeout" : 5,                            # Wait time in second after process start
        "log_dir" : "/.../viz-logs",              # Directory for log files
        "upload_dir" : "/.../data",               # If launcher should act as upload server, where to put files
        "fields" : ["file", "host", "port", "updir"]       # List of fields that should be send back to client
      },

      ## ===============================
      ## Useful session vars for client
      ## ===============================

      "sessionData" : { updir": "/Home" }         # Tells client which path to updateFileBrowser after uploads

      ## ===============================
      ## Resources list for applications
      ## ===============================

      "resources" : [ { "host" : "localhost", "port_range" : [9001, 9003] } ],

      ## ===============================
      ## Set of properties for cmd line
      ## ===============================

      "properties" : {
        "build_dir" : "/.../build",
        "python_exec" : "/.../build/bin/vtkpython",
        "WWW" : "/.../build/www",
        "source_dir": "/.../src"
      },

      ## ===============================
      ## Application list with cmd line
      ## ===============================

      "apps" : {
        "cone" : {
          "cmd" : [
            "${python_exec}", "${build_dir}/Wrapping/Python/vtk/web/vtk_web_cone.py", "--content", "${WWW}", "--port", "$port", "-f", "--authKey", "$secret"
              ],
          "ready_line" : "Starting factory"
        },
        "test" : {
          "cmd" : [
            "${python_exec}", "${build_dir}/PhylogeneticTree/server/vtk_web_phylogenetic_tree.py", "--content", "${WWW}" ],
          "ready_line" : "Starting factory"
        },
        "launcher" : {
          "cmd" : [
            "/home/kitware/launcher.sh", "${host}", "${port}", "${node}", "${app}", "${user}", "${password}", "${secret}" ],
          "ready_line" : "Good to go"
        }
      }
    }
"""

# =============================================================================
# Helper module methods
# =============================================================================

def generatePassword():
    return ''.join(choice(string.letters + string.digits) for _ in xrange(16))

# -----------------------------------------------------------------------------

def validateKeySet(obj, expected_keys, object_name):
    all_key_found = True
    for key in expected_keys:
        if not obj.has_key(key):
            print "ERROR: %s is missing %s key." % (object_name, key)
            all_key_found = False
    return all_key_found

# -----------------------------------------------------------------------------

def replaceVariables(template_str, variable_list):
    for key_pair in variable_list:
        item_template = string.Template(template_str)
        template_str = item_template.safe_substitute(key_pair)

    if "$" in template_str:
        logging.error("Some properties could not be resolved: " + template_str)

    return template_str

# -----------------------------------------------------------------------------

def replaceList(template_list, variable_list):
    result_list = []
    for str in template_list:
        result_list.append(replaceVariables(str, variable_list))
    return result_list

# -----------------------------------------------------------------------------

def filterResponse(obj, public_keys):
    public_keys.extend(['id', 'sessionURL', 'sessionManagerURL'])
    filtered_output = {}
    for field in obj:
        if field in public_keys:
            filtered_output[field] = obj[field]
    return filtered_output

# -----------------------------------------------------------------------------

def extractSessionId(request):
    path = request.path.split('/')
    if len(path) < 3:
       return None
    return str(path[2])

# =============================================================================
# Session manager
# =============================================================================

class SessionManager(object):

    def __init__(self, config, mapping):
        self.sessions = {}
        self.config = config
        self.resources = ResourceManager(config["resources"])
        self.mapping = mapping

    def createSession(self, options):
        # Assign id and store options
        id = str(uuid.uuid1())

        # Assign resource to session
        host, port = self.resources.getNextResource()

        # Do we have resources
        if host:
            options['id'] = id
            options['host'] = host
            options['port'] = port
            if not options.has_key('secret'):
                options['secret'] = generatePassword()
            options['sessionURL'] = replaceVariables(self.config['configuration']['sessionURL'], [options, self.config['properties']])
            options['cmd'] = replaceList(self.config['apps'][options['application']]['cmd'], [options, self.config['properties']])

            if self.config.has_key('sessionData') :
                for key in self.config['sessionData'] :
                    options[key] = replaceVariables(self.config['sessionData'][key], [options, self.config['properties']])

            self.sessions[id] = options
            self.mapping.update(self.sessions)
            return options

        return None

    def deleteSession(self, id):
        host = self.sessions[id]['host']
        port = self.sessions[id]['port']
        self.resources.freeResource(host, port)
        del self.sessions[id]
        self.mapping.update(self.sessions)

    def getSession(self, id):
        if self.sessions.has_key(id):
            return self.sessions[id]
        return None

# =============================================================================
# Proxy manager
# =============================================================================

class ProxyMappingManager(object):

    def update(sessions):
        pass

class ProxyMappingManagerTXT(ProxyMappingManager):

    def __init__(self, file_path, pattern="%s %s:%d"):
        self.file_path = file_path
        self.pattern = pattern

    def update(self, sessions):
        with open(self.file_path, "w") as map_file:
            for id in sessions:
                map_file.write(self.pattern % (id, sessions[id]['host'], sessions[id]['port']))

# =============================================================================
# Resource manager
# =============================================================================

class ResourceManager(object):
    """
    Class that provides methods to keep track on available resources (host/port)
    """
    def __init__(self, resourceList):
        self.resources = {}
        for resource in resourceList:
            host = resource['host']
            portList = range(resource['port_range'][0],resource['port_range'][1]+1)
            if self.resources.has_key(host):
                self.resources[host]['available'].extend(portList)
            else:
                self.resources[host] = { 'available': portList, 'used': []}

    def getNextResource(self):
        """
        Return a (host, port) pair if any available otherwise will return None
        """
        # find host with max availibility
        winner = None
        availibilityCount = 0
        for host in self.resources:
            if availibilityCount < len(self.resources[host]['available']):
                availibilityCount = len(self.resources[host]['available'])
                winner = host

        if winner:
            port = self.resources[winner]['available'].pop()
            self.resources[winner]['used'].append(port)
            return (winner, port)

        return (None, None)

    def freeResource(self, host, port):
        """
        Free a previously reserved resource
        """
        if self.resources.has_key(host) and port in self.resources[host]['used']:
            self.resources[host]['used'].remove(port)
            self.resources[host]['available'].append(port)


# =============================================================================
# Process manager
# =============================================================================

class ProcessManager(object):
    def __init__(self, configuration):
        self.config = configuration
        self.log_dir = configuration['configuration']['log_dir']
        self.processes = {}

    def __del__(self):
        for id in self.processes:
            self.processes[id].terminate()

    def startProcess(self, session, ready_callback=None):
        proc = None

        # Create output log file
        logFilePath = self.log_dir + os.sep + str(session['id']) + ".txt"
        with open(logFilePath, "a+") as log_file:
            try:
                proc = subprocess.Popen(session['cmd'], stdout=log_file, stderr=log_file)
                self.processes[session['id']] = proc
            except:
                logging.error("The command line failed")
                logging.error(''.join(map(str, session['cmd'])))
                return None

        return proc

    def stopProcess(self, id):
        proc = self.processes[id]
        del self.processes[id]
        try:
            proc.terminate()
        except:
            pass # we tried

    def listEndedProcess(self):
        session_to_release = []
        for id in self.processes:
            if self.processes[id].poll() is not None:
                session_to_release.append(id)
        return session_to_release

    def isRunning(self, id):
        return self.processes[id].poll() is None


# ===========================================================================
# Class to implement requests to POST, GET and DELETE methods
# ===========================================================================

class LauncherResource(resource.Resource, object):
    def __init__(self, options, config):
        super(LauncherResource, self).__init__()
        self._options = options
        self._config = config
        self.time_to_wait = int(config['configuration']['timeout'])
        self.field_filter = config['configuration']['fields']
        self.session_manager = SessionManager(config,ProxyMappingManagerTXT(config['configuration']['proxy_file']))
        self.process_manager = ProcessManager(config)

    def getChild(self, path, request):
        return self

    def __del__(self):
        logging.warning("Server factory shutting down. Stopping all processes")

    # ========================================================================
    # Handle POST request
    # ========================================================================

    def render_POST(self, request):
        payload = json.loads(request.content.getvalue())

        # Make sure the request has all the expected keys
        if not validateKeySet(payload, ["application"], "Launch request"):
            return json.dumps({"error": "The request is not complete"})

        # Try to free any available resource
        id_to_free = self.process_manager.listEndedProcess()
        for id in id_to_free:
            self.session_manager.deleteSession(id)
            self.process_manager.stopProcess(id)

        # Create new session
        session = self.session_manager.createSession(payload)

        # No resource available
        if not session:
            return json.dumps({"error": "All the resources are currently taken"})

        # Create response callback
        d = deferLater(reactor, self.time_to_wait, lambda: request)
        d.addCallback(self._delayedRender, session_id=session['id'])

        # Start process
        proc = self.process_manager.startProcess(session, d)

        if not proc:
            request.setResponseCode(http.OK)
            return json.dumps({"error": "The process did not properly start. %s" % str(session['cmd'])})

        return NOT_DONE_YET

    def _delayedRender(self, request, session_id=None):
        session = self.session_manager.getSession(session_id)
        running = self.process_manager.isRunning(session_id)

        if session and running:
            request.write(json.dumps(filterResponse(session, self.field_filter)))
        else:
            request.write(json.dumps({"error":"Session did not properly started"}))

        request.setResponseCode(http.OK)
        request.finish()

    # =========================================================================
    # Handle GET request
    # =========================================================================

    def render_GET(self, request):
        id = extractSessionId(request)

        if not id:
           message = "id not provided in GET request"
           logging.error(message)
           return json.dumps({"error":message})

        logging.info("GET request received for id: %s" % id)

        session = self.session_manager.getSession(id)
        if not session:
           message = "No session with id: %s" % id
           logging.error(message)
           return json.dumps({"error":message})

        # Return session meta-data
        request.setResponseCode(http.OK)
        return json.dumps(filterResponse(session, self.field_filter))

    # =========================================================================
    # Handle DELETE request
    # =========================================================================

    def render_DELETE(self, request):
        id = extractSessionId(request)

        if not id:
           message = "id not provided in DELETE request"
           logging.error(message)
           return json.dumps({"error":message})

        logging.info("DELETE request received for id: %s" % id)

        session = self.session_manager.getSession(id)
        if not session:
           message = "No session with id: %s" % id
           logging.error(message)
           return json.dumps({"error":message})

        # Remove session
        self.session_manager.deleteSession(id)
        self.process_manager.stopProcess(id)

        message = "Deleted session with id: %s" % id
        logging.info(message)

        request.setResponseCode(http.OK)
        return session

# =============================================================================
# Start the web server
# =============================================================================

def startWebServer(options, config):
    # Extract properties from config
    log_dir  = str(config["configuration"]["log_dir"])
    content  = str(config["configuration"]["content"])
    endpoint = str(config["configuration"]["endpoint"])
    host     = str(config["configuration"]["host"])
    port     = int(config["configuration"]["port"])

    # Setup logging
    logFileName = log_dir + os.sep + "launcherLog.log"
    formatting = '%(asctime)s:%(levelname)s:%(name)s:%(message)s'
    logging.basicConfig(level=logging.DEBUG, filename=logFileName, filemode='w', format=formatting)
    observer = log.PythonLoggingObserver()
    observer.start()
    if options.debug:
        console = logging.StreamHandler(sys.stdout)
        console.setLevel(logging.INFO)
        formatter = logging.Formatter(formatting)
        console.setFormatter(formatter)
        logging.getLogger('').addHandler(console)

    # Initialize web resource
    web_resource = File(content) if (len(content) > 0) else resource.Resource()

    # Attach launcher
    web_resource.putChild(endpoint, LauncherResource(options, config))

    # Check if launcher should act as a file upload server as well
    if config["configuration"].has_key("upload_dir"):
        from upload import UploadPage
        updir = replaceVariables(config['configuration']['upload_dir'], [config['properties']])
        uploadResource = UploadPage(updir)
        web_resource.putChild("upload", uploadResource)

    site = server.Site(web_resource)
    reactor.listenTCP(port, site, interface=host)
    reactor.run()

# =============================================================================
# Parse config file
# =============================================================================

def parseConfig(options):
    # Read values from the configuration file
    try:
        config = json.loads(open(options.config[0]).read())
    except:
        message = "ERROR: Unable to read config file.\n"
        message += str(sys.exc_info()[1]) + "\n" + str(sys.exc_info()[2])
        print message
        print sample_config_file
        sys.exit(2)

    expected_keys = ["configuration", "apps", "properties", "resources"]
    if not validateKeySet(config, expected_keys, "Config file"):
        print sample_config_file
        sys.exit(2)

    expected_keys = ["endpoint", "host", "port", "proxy_file", "sessionURL", "timeout", "log_dir", "fields"]
    if not validateKeySet(config["configuration"], expected_keys, "file.configuration"):
        print sample_config_file
        sys.exit(2)

    if not config["configuration"].has_key("content"):
        config["configuration"]["content"] = ""

    return config

# =============================================================================
# Setup default arguments to be parsed
#   -d, --debug
#   -t, --proxyFileType  Type of proxy file (txt, dbm)
# =============================================================================

def add_arguments(parser):
    parser.add_argument("config", type=str,  nargs=1,
        help="configuration file for the launcher")
    parser.add_argument("-d", "--debug",
        help="log debugging messages to stdout",
        action="store_true")

    return parser

# =============================================================================
# Parse arguments
# =============================================================================

def start(argv=None,
         description="VTKWeb Launcher"):
    parser = argparse.ArgumentParser(description=description)
    add_arguments(parser)
    args = parser.parse_args(argv)
    config = parseConfig(args)
    startWebServer(args, config)

# =============================================================================
# Main
# =============================================================================

if __name__ == "__main__":
    start()
