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
        "core-min" : [
        "ext/core/jquery-1.8.3.min.js",
        "ext/core/autobahn.min.js",
        "lib/core/vtkweb-all.min.js"
        ],
        "core" : [
        "ext/core/jquery-1.8.3.min.js",
        "ext/core/autobahn.js",
        "lib/core/vtkweb-all.js"
        ],
        "webgl-min" : [
        "ext/core/gl-matrix-min.js"
        ],
        "webgl" : [
        "ext/core/gl-matrix.js"
        ],
        "mobile-min" : [
        "ext/core/jquery.hammer.min.js"
        ],
        "mobile" : [
        "ext/core/jquery.hammer.js"
        ],
        "all" : [
        "ext/core/jquery-1.8.3.min.js",
        "ext/core/autobahn.js",
        "lib/core/vtkweb-all.js",
        "ext/core/gl-matrix.js",
        "ext/core/jquery.hammer.js"
        ],
        "all-min": [
        "ext/core/jquery-1.8.3.min.js",
        "ext/core/autobahn.min.js",
        "lib/core/vtkweb-all.min.js",
        "ext/core/gl-matrix-min.js",
        "ext/core/jquery.hammer.min.js"
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
        "lib/widgets/vtkweb-widget-filebrowser.js",
        "lib/widgets/vtkweb-widget-filebrowser.tpl",
        "lib/widgets/vtkweb-widget-filebrowser.css"
        ],
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
        document.getElementsByTagName("head")[0].appendChild(link);
    }

    // ---------------------------------------------------------------------
    function loadJavaScript(url) {
        document.write('<script src="' + url + '"></script>');
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
            // template to load
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
        modules = [ "all-min" ];
    }

    // ---------------------------------------------------------------------
    // Extract basePath
    // ---------------------------------------------------------------------
    var lastSlashIndex = script.getAttribute("src").lastIndexOf('lib/js/vtkweb-loader');
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
