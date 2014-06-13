(function ($, GLOBAL) {
    var TOOLBAR_TEMPLATE = '<ul class=toolbar-main><li class="logo"/><li class="vtk-icon-menu-1 toggle-button run-button" data-animation="left" data-group=runs data-view="run-content">Runs</li><li class="vtk-icon-info-1 toggle-button need-project default-toggle" data-group="content" data-view="info-content" alt="Toggle Informations" title="Toggle Informations"/><li class="vtk-icon-th toggle-button need-project" data-group=content data-view="bench-content" alt="Toggle Exploration" title="Toggle Exploration"/><li class="vtk-icon-beaker toggle-button need-project" data-group=content data-view="search-content" alt="Toggle Search" title="Toggle Search"/><li class="vtk-icon-dollar toggle-button need-project" data-group=content data-view="cost-content" alt="Toggle Cost" title="Toggle Cost"/><li class="vtk-icon-gauge-1 toggle-button right" data-group=content data-view="estimate-content" alt="Data exploration cost estimate" title="Data exploration cost estimate"/><li class="vtk-icon-user-add-1 toggle-button need-project right" data-group=content data-view="share-content" alt="Share active project" title="Share active project"/></ul><ul class="toggle-content run-content" data-group=runs></ul><div class="info-content toggle-content" data-group=content></div><div class="bench-content toggle-content" data-group="content"></div><div class="search-content toggle-content" data-group=content></div><div class="cost-content toggle-content" data-group="content"></div><div class="share-content toggle-content" data-group="content">The current version does not support user management.</div><div class="estimate-content toggle-content" data-group="content">COST ESTIMATE</div>',
    RUN_LINE_TEMPLATE = '<li class=select-run data-path=PATH>TITLE<i class=help>DESCRIPTION</i></li>',
    TABLE_LINE_TEMPLATE = '<tr><td class="key">KEY</td><td class="value">VALUE</td></tr>';


    // ========================================================================
    // Helper
    // ========================================================================

    function projectInfoToHTML(info, path) {
        var projectDescription = "<table>",
        exclude = { "title": 1, "description": 1, "analysis": 1, "path": 1 };

        // Update project description
        projectDescription += TABLE_LINE_TEMPLATE.replace(/KEY/g, "Name").replace(/VALUE/g, info.title);
        projectDescription += TABLE_LINE_TEMPLATE.replace(/KEY/g, "Description").replace(/VALUE/g, info.description);
        for(var key in info) {
            if(!exclude.hasOwnProperty(key)) {
                projectDescription += TABLE_LINE_TEMPLATE.replace(/KEY/g, key).replace(/VALUE/g, info[key]);
            }
        }
        projectDescription += "</table>";

        return projectDescription;
    }

    // ------------------------------------------------------------------------

    function handlePath(fullPath, projectPath) {
        if(projectPath.indexOf("http://") === 0 || projectPath.indexOf("https://") === 0 || projectPath.indexOf("file://") === 0) {
            return projectPath;
        } else {
            // Relative path
            var basePath = fullPath.substr(0, 1 + fullPath.lastIndexOf("/"));
            return basePath + projectPath;
        }
    }

    // ------------------------------------------------------------------------

    function createControlToolbar(container, projectList, fullURL) {
        // Fill run list
        var count = projectList.length, buffer = [];
        while(count--) {
            buffer.push(RUN_LINE_TEMPLATE.replace(/PATH/g, handlePath(fullURL, projectList[count].path)).replace(/TITLE/g, projectList[count].title).replace(/DESCRIPTION/g, projectList[count].description));
        }
        container.html(TOOLBAR_TEMPLATE);
        $('.run-content', container).html(buffer.join(''));
    }

    // ------------------------------------------------------------------------

    function initializeListeners(container) {
        // Handle view/button toggle
        $('.toggle-button', container).addClass('action').click(function(){
            var me = $(this),
            group = me.attr('data-group'),
            view = me.attr('data-view'),
            animation = me.attr('data-animation'),
            isActive = me.hasClass('active'),
            buttons = $('.toggle-button[data-group="' + group + '"]', container),
            contents = $('.toggle-content[data-group="' + group + '"]', container);

            // Disable all
            buttons.removeClass('active');
            if(animation && isActive) {
                contents.animate({
                    left: "-1000"
                }, 500, function() {
                    // Animation complete.
                    contents.hide();
                });
            } else {
                contents.hide();
            }


            // Enable local one if not previously active
            if(!isActive) {
                me.addClass('active');
                if(animation) {
                    $('.toggle-content.' + view, container).show().animate({
                        left: "0"
                    }, 500, function() {
                        // Animation complete.
                    });
                } else {
                    $('.toggle-content.' + view, container).show();
                }
            } else {
                $('.default-toggle[data-group="' + group + '"]', container).trigger('click');
            }
        });

        // Load run
        $('.select-run', container).addClass('action').click(function(){
            var me = $(this), basePath = me.attr('data-path');

            // Load project
            $.ajax({
                url: basePath + '/info.json',
                dataType: 'json',
                success: function( data ) {
                    $('.toggle-button[data-group="runs"]', container).click();

                    // Store metadata
                    container.data('project', data);

                    // Add project description / viewers / search / cost
                    $('.info-content', container).empty().html(projectInfoToHTML(data, basePath));
                    $('.bench-content', container).vtkCatalystAnalysisBench(data, basePath);
                    $('.search-content', container).vtkCatalystAnalysisSearch(data, basePath);
                    $('.cost-content', container).vtkCatalystAnalysisCost(data, basePath);

                    // Update title
                    document.title = data.title;

                    // Show default
                    $('.default-toggle[data-group="content"]', container).trigger('click');
                },
                error: function(error) {
                    console.log("error when trying to download " + basePath + '/info.json');
                    console.log(error);
                }
            });

            // Enable toolbar
            $('li.need-project', container).css('display', "inline");
        });
    }

    /**
     * jQuery catalyst view constructor.
     *
     * @member jQuery.vtkCatalystViewer
     * @param basePath
     * Root directory for data to visualize
     */

    $.fn.vtkCatalystAnalysis = function(fullURL) {
        return this.each(function() {
            var me = $(this).unbind().empty().addClass('vtkweb-catalyst-analysis');

            // Get meta-data
            $.ajax({
                url: fullURL,
                dataType: 'json',
                success: function( data ) {
                    // Store metadata
                    me.data('projects', data);

                    // Create project list
                    createControlToolbar(me, data, fullURL);

                    // Attach interaction listeners
                    initializeListeners(me);

                    // Add general purpose cost estimate
                    $('.estimate-content',me).vtkCatalystAnalysisCostEstimate();
                },
                error: function(error) {
                    console.log("error when trying to download " + fullURL);
                    console.log(error);
                }
            });
        });
    }

    }(jQuery, window));