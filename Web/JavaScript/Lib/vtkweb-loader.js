/**
 * vtkWebLoader JavaScript Library.
 *
 * vtkWebLoader use the vtkWeb namespace to manage JavaScript dependency and more specifically
 * vtkWeb dependencies.
 *
 * @class vtkWebLoader
 *
 * @singleton
 */
(function (GLOBAL) {

    var vtkWebLibs = {
        "core" : [
        "ext/core/jquery-1.8.3.min.js",
        "ext/core/autobahn.min.js",
        "ext/core/gl-matrix.js",
        "ext/core/jquery.hammer.js",
        "ext/core/vgl.min.js",
        "lib/core/vtkweb-all.js"
        ],
        "core-min": [
        "ext/core/jquery-1.8.3.min.js",
        "ext/core/autobahn.min.js",
        "ext/core/gl-matrix-min.js",
        "ext/core/jquery.hammer.min.js",
        "ext/core/vgl.min.js",
        "lib/core/vtkweb-all.min.js"
        ],
        "bootstrap": [
        "ext/bootstrap/js/bootstrap.min.js",
        "ext/bootstrap/css/bootstrap-responsive.min.css",
        "ext/bootstrap/css/bootstrap.min.css"
        ],
        "fontello": [
        "ext/fontello/css/animation.css",
        "ext/fontello/css/fontello.css"
        ],
        "color": [
        "ext/jscolor/jscolor.js"
        ],
        "filebrowser": [
        "ext/pure/pure.min.js",
        "lib/widgets/FileBrowser/vtkweb-widget-filebrowser.js",
        "lib/widgets/FileBrowser/vtkweb-widget-filebrowser.tpl",
        "lib/widgets/FileBrowser/vtkweb-widget-filebrowser.css"
        ],
        "pv-pipeline": [
        "ext/jquery-ui/jquery-ui-1.10.0.css",
        "ext/jquery-ui/jquery-ui-1.10.0.min.js",
        "lib/css/paraview.ui.pipeline.css",
        "lib/js/paraview.ui.pipeline.js",
        ],
        "pv-toolbar": [
        "lib/css/paraview.ui.toolbar.css",
        "lib/css/paraview.ui.toolbar.vcr.css",
        "lib/css/paraview.ui.toolbar.viewport.css",
        "lib/css/paraview.ui.toolbar.connect.css",
        "lib/js/paraview.ui.toolbar.js",
        "lib/js/paraview.ui.toolbar.vcr.js",
        "lib/js/paraview.ui.toolbar.viewport.js",
        "lib/js/paraview.ui.toolbar.connect.js"
        ],
        "jquery-ui": [
        "ext/jquery-ui/jquery-ui-1.10.0.css",
        "ext/jquery-ui/jquery-ui-1.10.0.min.js"
        ],
        "d3":[
        "ext/d3/d3.v2.js"
        ],
        "nvd3":[
        "ext/nvd3/nv.d3.css",
        "ext/nvd3/nv.d3.js"
        ],
        "rickshaw": [
        "ext/rickshaw/rickshaw.min.css",
        "ext/rickshaw/rickshaw.min.js"
        ],
        "widgets": [
        "ext/pure/pure.min.js",
        "ext/d3/d3.v2.js",
        "ext/rickshaw/rickshaw.min.css",
        "ext/rickshaw/rickshaw.min.js",
        "ext/fontello/css/animation.css",
        "ext/fontello/css/fontello.css",
        "lib/widgets/FileBrowser/vtkweb-widget-filebrowser.tpl",
        "lib/widgets/TreeWidget/vtkweb-widget-tree.tpl",
        "lib/widgets/vtkweb-widgets-min.css",
        "lib/widgets/vtkweb-widgets-min.js"
        ],
        "pv-visualizer": [
        "ext/fontello/css/animation.css",
        "ext/fontello/css/fontello.css",
        "ext/bootstrap3/css/bootstrap-theme.min.css",
        "ext/bootstrap3/css/bootstrap.min.css",
        "ext/bootstrap3/js/bootstrap.min.js",
        "lib/js/paraview.ui.action.list.js",
        "lib/js/paraview.ui.files.js",
        "lib/js/paraview.ui.data.js",
        "lib/js/paraview.ui.proxy.editor.js",
        "lib/css/paraview.ui.proxy.editor.css",
        "lib/js/paraview.ui.svg.pipeline.js",
        "lib/js/paraview.ui.opacity.editor.js",
        "lib/css/paraview.ui.opacity.editor.css"
        ]

    },
    modules = [],
    script = document.getElementsByTagName("script")[document.getElementsByTagName("script").length - 1],
    basePath = "",
    extraScripts = [];

    // ---------------------------------------------------------------------
    function loadCss(url) {
        var link = document.createElement("link");
        link.type = "text/css";
        link.rel = "stylesheet";
        link.href = url;
        head = document.getElementsByTagName("head")[0];
        head.insertBefore(link, head.childNodes[0]);
    }

    // ---------------------------------------------------------------------
    function loadJavaScript(url) {
        document.write('<script src="' + url + '"></script>');
    }

    // ---------------------------------------------------------------------
    function loadTemplate(url) {
        var templates = document.getElementById("vtk-templates");
        if(templates === null) {
            templates = document.createElement("div");
            templates.setAttribute("style", "display: none;");
            templates.setAttribute("id", "vtk-templates");
            document.getElementsByTagName("body")[0].appendChild(templates);
        }

        // Fetch template and append to vtk-templates
        var request = makeHttpObject();
        request.open("GET", url, true);
        request.send(null);
        request.onreadystatechange = function() {
            if (request.readyState == 4) {
              var content = templates.innerHTML;
              content += request.responseText;
              templates.innerHTML = content;
            }
        };
    }

    // ---------------------------------------------------------------------

    function makeHttpObject() {
        try {
            return new XMLHttpRequest();
        }
        catch (error) {}
        try {
            return new ActiveXObject("Msxml2.XMLHTTP");
        }
        catch (error) {}
        try {
            return new ActiveXObject("Microsoft.XMLHTTP");
        }
        catch (error) {}

        throw new Error("Could not create HTTP request object.");
    }

    // ---------------------------------------------------------------------
    function _endWith(string, end) {
        return string.lastIndexOf(end) === (string.length - end.length);
    }

    // ---------------------------------------------------------------------
    function loadFile(url) {
        if(_endWith(url, ".js")) {
            loadJavaScript(url);
        } else if(_endWith(url, ".css")) {
            loadCss(url);
        } else if(_endWith(url, ".tpl")) {
            loadTemplate(url);
        }
    }

    // ---------------------------------------------------------------------
    // Extract modules to load
    // ---------------------------------------------------------------------
    try {
        modules = script.getAttribute("load").split(",");
        for(var j in modules) {
            modules[j] = modules[j].replace(/^\s+|\s+$/g, ''); // Trim
        }
    } catch(e) {
    // We don't care we will use the default setup
    }

    // ---------------------------------------------------------------------
    // Extract extra script to load
    // ---------------------------------------------------------------------
    try {
        extraScripts = script.getAttribute("extra").split(",");
        for(var j in extraScripts) {
            extraScripts[j] = extraScripts[j].replace(/^\s+|\s+$/g, ''); // Trim
        }
    } catch(e) {
    // We don't care we will use the default setup
    }

    // ---------------------------------------------------------------------
    // If no modules have been defined, just pick the default
    // ---------------------------------------------------------------------
    if(modules.length == 0) {
        modules = [ "core-min" ];
    }

    // ---------------------------------------------------------------------
    // Extract basePath
    // ---------------------------------------------------------------------
    var lastSlashIndex = script.getAttribute("src").lastIndexOf('lib/core/vtkweb-loader');
    if(lastSlashIndex != -1) {
        basePath = script.getAttribute("src").substr(0, lastSlashIndex);
    }

    // ---------------------------------------------------------------------
    // Add missing libs
    // ---------------------------------------------------------------------
    for(var i in modules) {
        for(var j in vtkWebLibs[modules[i]]) {
            var path = basePath + vtkWebLibs[modules[i]][j];
            loadFile(path);
        }
    }

    // ---------------------------------------------------------------------
    // Add extra libs
    // ---------------------------------------------------------------------
    for(var i in extraScripts) {
        loadFile(extraScripts[i]);
    }

    // ---------------------------------------------------------------------
    // Remove loader
    // ---------------------------------------------------------------------
    script.parentNode.removeChild(script);
}(window));
