/**
 * VTK-Web Widget Library.
 *
 * This module extend jQuery object to add support for graphical components
 * related to File Browsing.
 *
 * @class jQuery.vtk.ui.FileBrowser
 */
(function ($) {

    // =======================================================================
    // ==== Defaults constant values =========================================
    // =======================================================================
    var pathSeparator = '/',
    directives = {
         '.vtk-directory': {
            'directory <-': {
                '@path': function(arg) {
                    return pathToStr(arg.item.path);
                },
                '@class+': function(arg) {
                    return (arg.item.path.length === 1) ? ' active' : '';
                },
                '.vtk-label': 'directory.label',
                'li.vtk-files': {
                    'file <- directory.files': {
                        'div': 'file.label'
                    }
                },
                'li.vtk-groups': {
                    'gfile <- directory.groups': {
                        'div': 'gfile.label',
                        '@files': function(arg) {
                            return arg.item.files.join(":");
                        }
                    }
                },
                'li.vtk-dirs': {
                    'dir <- directory.dirs': {
                        'div': 'dir'
                    }
                },
                'li.vtk-path': {
                    'i <- directory.path': {
                        'div': 'i'
                    }//,
                    // filter : function(a) {
                    //     return a.pos < (a.items.length - 1);
                    // }
                }
            }
        }
    },
    fileBrowserGenerator = null;

    $.fn.fileBrowser = function(options) {
        // Handle data with default values
        var opts = $.extend({},$.fn.fileBrowser.defaults, options);

        // Compile template only once
        if(fileBrowserGenerator === null) {
            template = $(opts.template);
            fileBrowserGenerator = template.compile(directives);
        }

        return this.each(function() {
            var me = $(this).empty().addClass('vtk-filebrowser'),
            container = $('<div/>');
            me.append(container);
            me.data('file-list', opts.data);
            me.data('session', opts.session);
            me.data('cacheFiles', opts.cacheFiles);

            if(opts.data === null) {
                opts.session.call('file.server.directory.list',['.']).then(function(files) {
                    opts.data = [ files ];
                    me.data('file-list', opts.data);

                    // Generate HTML
                    container.render(opts.data, fileBrowserGenerator);

                    // Initialize pipelineBrowser (Visibility + listeners)
                    initializeListener(me);
                });
            } else {
                // Generate HTML
                container.render(opts.data, fileBrowserGenerator);

                // Initialize pipelineBrowser (Visibility + listeners)
                initializeListener(me);
            }
        });
    };

    $.fn.updateFileBrowser = function(activeDirectory) {

        return this.each(function() {
            var me = $(this).empty(),
            data = me.data('file-list'),
            newData = [],
            container = $('<div/>');

            me.append(container);

            // Delete the cached active directory and fetch again
            if(activeDirectory && me.data('session')){
                var dirArray = activeDirectory.split("/").splice(1);
                for(var i in data) {
                    var item = data[i];
                    var itemArray = item.path;
                    if ( !equals(itemArray, dirArray) ) {
                        newData.push(data[i]);
                    }
                }

                var requestPath =  activeDirectory.substring(1);
                if(requestPath.indexOf('/') == -1) {
                    requestPath = '.';
                }
                me.data('session').call('file.server.directory.list', [requestPath])
                    .then(function(newFiles){
                        newData.push(newFiles);
                        me.data('file-list', newData);
                        // Generate HTML
                        container.render(newData, fileBrowserGenerator);

                        // Initialize pipelineBrowser (Visibility + listeners)
                        initializeListener(me, activeDirectory);
                    });

            } else {
                // Generate HTML
                container.render(data, fileBrowserGenerator);

                // Initialize pipelineBrowser (Visibility + listeners)
                initializeListener(me, activeDirectory);
            }
        });
    };

    $.fn.fileBrowser.defaults = {
        template: "#vtk-templates > .vtkweb-widget-filebrowser > div",
        session: null,
        data: null,
        cacheFiles: true
    };

    // =======================================================================

    function strToPath(pathId) {
        var path = pathId.split(pathSeparator);
        return path.slice(1, path.length);
    }

    // =======================================================================

    function getParent(path) {
        return path.slice(0, path.length - 2);
    }

    // =======================================================================

    function getPath(parentPath, child) {
        return [].concat(parentPath).concat(child);
    }

    // =======================================================================

    function pathToStr(path) {
        //console.log(path);
        var str = pathSeparator + path.join(pathSeparator);
        return str;
    }

    // =======================================================================

    equals = function(array1, array2) {
        if (array1.length != array2.length) {
            return false;
        }

        for (var i in array1) {
            if (array1[i] !== array2[i]) {
                return false;
            }
        }

        return true;
    }

    // =======================================================================

    function getRelativePath(parentPath, fileName) {
        return '.' + pathToStr(getPath(parentPath, fileName).slice(1));
    }

    // =======================================================================

    function initializeListener(container, activePath) {
        $('.action', container).click(function(){
            var me = $(this), item = $('div', me), pathStr = me.closest('.vtk-directory').attr('path'), type = me.closest('ul').attr('data');

            if(type === 'path') {
                // Find out the panel to show
                var newPath = pathToStr(strToPath(pathStr).slice(0, me.index() + 1)),
                selector = '.vtk-directory[path="' + newPath + '"]';
                var newActive = $(selector , container).addClass('active');
                if(newActive.length === 1) {
                     $('.vtk-directory', container).removeClass('active');
                     newActive.addClass('active');
                }
                if (container.data('cacheFiles') === false) {
                    container.updateFileBrowser(newPath);
                }
            } else if(type === 'dir') {
                // Swicth active panel
                var str = '.vtk-directory[path="' + pathStr + pathSeparator + item.html() + '"]';
                var newActive = $(str, container);
                if(newActive.length === 1) {
                    $('.vtk-directory', container).removeClass('active');
                    newActive.addClass('active');
                    container.trigger({
                        type: 'directory-click',
                        path: pathStr,
                        name: me.text(),
                        relativePath: getRelativePath(strToPath(pathStr), me.text())
                    });
                } else {
                    if(container.data('session')) {
                        var relativePath = (pathStr + '/' + me.text());
                        container.data('session').call('file.server.directory.list', [relativePath.substring(1)]).then(function(newFiles){
                            container.data('file-list').push(newFiles);
                            container.updateFileBrowser(relativePath);
                        });

                    }
                    container.trigger({
                        type: 'directory-not-found',
                        path: pathStr,
                        name: me.text(),
                        relativePath: getRelativePath(strToPath(pathStr), me.text())
                    });
                }
            } else if(type === 'files') {
                container.trigger({
                    type: 'file-click',
                    path: pathStr,
                    name: me.text(),
                    relativePathList: [ getRelativePath(strToPath(pathStr), me.text()) ],
                    list: [ me.text() ],
                    relativePath: getRelativePath(strToPath(pathStr), me.text())
                });
            } else if(type === 'groups') {
                var relativePathList = [], fileList = me.attr('files').split(':');
                for(var i in fileList) {
                    relativePathList.push(getRelativePath(strToPath(pathStr), fileList[i]));
                }
                container.trigger({
                    type: 'file-group-click',
                    path: pathStr,
                    name: me.text(),
                    list: fileList,
                    relativePathList: relativePathList,
                    relativePath: getRelativePath(strToPath(pathStr), me.text())
                });
            }
        });
        if(activePath) {
            $('.vtk-directory',container).removeClass('active');
            $('.vtk-directory[path="' + activePath + '"]',container).addClass('active');
        }

    }

}(jQuery));