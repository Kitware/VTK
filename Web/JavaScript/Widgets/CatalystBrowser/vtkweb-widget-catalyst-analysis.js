(function ($, GLOBAL) {
    var TOOLBAR_ANALYSIS_ITEM_TEMPLATE = '<li class="action add-viewer" data-type="TYPE" data-path="PATH" data-title="TITLE"><span class="CLASS">TITLE<i>DESCRIPTION</i></span></li>',
    OPTION_ANALYSIS_ITEM_TEMPLATE = '<option value="TYPE:PATH">TITLE</option>',
    TOOLBAR_PROJECT_TEMPLATE = '<li class="action"><span class="vtk-icon-info-1" data-path="PATH">TITLE<i><table class="description">TABLE_CONTENT</table></i></span></li>',
    TABLE_LINE_TEMPLATE = '<tr><td class="key">KEY</td><td class="value">VALUE</td></tr>',
    TOOLBAR_LAYOUT = '<ul class="left">LIST</ul>',
    STATIC_TOOLS = '<ul class="right"><li class="action vtk-icon-th active toggle-view" title="Exploration view" data-view="content"/><li class="action vtk-icon-beaker search toggle-view" data-view="search" title="Search"/><li class="action vtk-icon-dollar toggle-view" data-view="cost" title="Search" /><li class="action vtk-icon-magic control"/><li class="action vtk-icon-trash clear-workspace"/></ul>',
    VIEWER_FACTORY = {
        "catalyst-viewer": {
            class: "vtk-icon-loop-alt",
            factory: function(domToFill, path) {
                domToFill.vtkCatalystViewer(path, false);
            }
        },
        "catalyst-resample-viewer" : {
            class: "vtk-icon-chart-line",
            factory: function(domToFill, path) {
                domToFill.vtkCatalystResamplerViewer(path);
            }
        },
        "composite-image-stack" : {
            class: "vtk-icon-list-add",
            factory: function(domToFill, path) {
                domToFill.vtkCatalystCompositeViewer(path);
            }
        },
        "catalyst-pvweb" : {
            class: "vtk-icon-laptop",
            factory: function(domToFill, path) {
                domToFill.vtkCatalystPVWeb(path);
            }
        }
    },
    SEARCH_FACTORY = {
        "catalyst-viewer": function(domToFill, path) {
            domToFill.vtkCatalystAnalysisSearch(path);
        },
        "catalyst-resample-viewer" : function(domToFill, path) {
            domToFill.vtkCatalystAnalysisSearch(path);
        },
        "composite-image-stack" : function(domToFill, path) {
            domToFill.vtkCatalystCompositeSearch(path);
        },
        "catalyst-pvweb" : function(domToFill, path) {
            domToFill.empty().html("<p style='padding: 20px;font-weight: bold;'>This type of data is not searchable.</p>");
        }
    };

    // ========================================================================

    function formula(cost) {
        var dollarsAmount = 0;
        if(cost) {
            if(cost["time"]) {
                dollarsAmount += 0.001 * cost["time"];
            }
            if(cost["space"]) {
                dollarsAmount += 0.000000002 * cost["space"];
            }
            if(cost["images"]) {
                dollarsAmount += 0.001 * cost["images"];
            }
        }
        return dollarsAmount;
    }

    // ========================================================================

    function formatTime(t) {
        var seconds = Number(t),
        minutes = Math.floor(seconds / 60),
        hours = Math.floor(minutes / 60),
        buffer = [];

        seconds %= 60;
        minutes %= 60;

        if(hours > 0) {
           buffer.push(hours);
        }
        if(minutes > 0 || hours > 0) {
           buffer.push(("00" + minutes).slice (-2));
        }
        if(seconds > 0 || minutes > 0 || hours > 0) {
            buffer.push(("00" + seconds).slice (-2));
        }

        return buffer.join(':');
    }

    // ========================================================================

    function formatSpace(t) {
        var space = Number(t), unit = [ ' B', ' K', ' M', ' G', ' T'], currentUnit = 0;
        while(space > 1000) {
            space /= 1000;
            currentUnit++;
        }
        return space.toFixed(2) + unit[currentUnit];
    }

    // ========================================================================

    function formatDollars(v) {
        x = v.toFixed(2).toString();
        var pattern = /(-?\d+)(\d{3})/;
        while (pattern.test(x)) {
            x = x.replace(pattern, "$1,$2");
        }
        return x;
    }

    // ========================================================================
    // Helper
    // ========================================================================

    function projectToHTML(info, path) {
        var items = [ '<div class="workbench-toolbar"><ul class="left">'],
        projectDescription = "",
        exclude = { "title": 1, "description": 1, "analysis": 1, "path": 1 },
        analysisCount = info['analysis'].length,
        optionsList = [];

        // Update project description
        projectDescription += TABLE_LINE_TEMPLATE.replace(/KEY/g, "Name").replace(/VALUE/g, info.title);
        projectDescription += TABLE_LINE_TEMPLATE.replace(/KEY/g, "Description").replace(/VALUE/g, info.description);
        for(var key in info) {
            if(!exclude.hasOwnProperty(key)) {
                projectDescription += TABLE_LINE_TEMPLATE.replace(/KEY/g, key).replace(/VALUE/g, info[key]);
            }
        }
        items.push(TOOLBAR_PROJECT_TEMPLATE.replace(/PATH/g, path).replace(/TITLE/g, info.title).replace(/TABLE_CONTENT/g, projectDescription));

        // Add each analysis
        while(analysisCount--) {
            optionsList.push(analysisItemToOPTION(path, info['analysis'][analysisCount]));
            items.push(analysisItemToHTML(path, info['analysis'][analysisCount]));
        }

        // Add static tools
        items.push("</ul>");
        items.push(STATIC_TOOLS);
        items.push("</div><div class='view content'></div>");
        items.push("<div class='view search'><select name='exploration'>OPTIONS</select><div class='search-results'></div></div>".replace(/OPTIONS/g, optionsList.join('')));
        items.push(buildBillingPage(info, path, formula));

        return items.join('');
    }

    // ------------------------------------------------------------------------

    function buildBillingPage(info, path, formula) {
        var content = [ "<table class='catalyst-bill'><tr class='head'></td><td class='empty title'></td><td><span class='vtk-icon-clock'/></td><td><span class='vtk-icon-database'/></td><td><span class='vtk-icon-picture-1'/></td><td><span class='vtk-icon-dollar'/></td></tr>" ],
        total = { "space": 0, "images": 0, "time": 0 , "dollars": 0},
        analysisCount = info['analysis'].length;

        // Add each analysis
        while(analysisCount--) {
            var item = info['analysis'][analysisCount],
            cost = item['cost'],
            dollars = formula(cost);

            total['space'] += cost['space'];
            total['images'] += cost['images'];
            total['time'] += cost['time'];
            total['dollars'] += dollars;

            content.push(buildBillEntry(item, cost, dollars));
        }

        // Add total
        content.push("<tr class='sum'><td>Total</td><td>"+ formatTime(total["time"]) +"</td><td>"+ formatSpace(total["space"]) +"</td><td>"+ total["images"] +"</td><td>"+ formatDollars(total["dollars"]) +"</td></tr></table>")

        return "<div class='view cost'>" + content.join('') + "</div>";
    }

    // ------------------------------------------------------------------------

    function buildBillEntry(item, cost, dollars) {
        var classType = VIEWER_FACTORY[item["type"]]["class"],
        title = item["title"],
        time = cost["time"],
        space = cost["space"],
        images = cost["images"];

        return "<tr><td class='title'><span class='" + classType + "'/>" + title + "</td><td class='time value'>" + formatTime(time) + "</td><td class='space value'>" + formatSpace(space) + "</td><td class='images value'>" + images + "</td><td class='dollars value'>" + formatDollars(dollars) + "</td></tr>";
    }

    // ------------------------------------------------------------------------

    function analysisItemToHTML(basePath, item) {
        return TOOLBAR_ANALYSIS_ITEM_TEMPLATE
            .replace(/TYPE/g, item.type)
            .replace(/PATH/g, basePath + '/' + item.id)
            .replace(/CLASS/g, VIEWER_FACTORY[item.type].class)
            .replace(/TITLE/g, item.title)
            .replace(/DESCRIPTION/g, item.description);
    }

    // ------------------------------------------------------------------------

    function analysisItemToOPTION(basePath, item) {
        return OPTION_ANALYSIS_ITEM_TEMPLATE
            .replace(/TYPE/g, item.type)
            .replace(/PATH/g, basePath + '/' + item.id)
            .replace(/TITLE/g, item.title);
    }

    // ------------------------------------------------------------------------

    function attachViewerContent(viewer, type, path) {
        var content = $('.content', viewer),
        path = (window.location.href.split('/').slice(0,-1).join('/') + '/' + path).split('/'),
        canonicalPath = [],
        count = path.length;

        // Handle ..
        while(count--) {
            if(path[count] === '..') {
                count--;
            } else {
                canonicalPath.push(path[count]);
            }
        }
        canonicalPath.reverse();
        VIEWER_FACTORY[type].factory(content, canonicalPath.join('/'));
    }

    // ------------------------------------------------------------------------

    function initializeListeners(container) {
        // Listen layout size
        container.bind('layout-size', function(e){
            container.data('layout-size', Number(e.size));
        });

        // Handle view add-on
        $('.add-viewer', container).bind('click', function(){
            var me = $(this),
            type = me.attr('data-type'),
            path = me.attr('data-path'),
            title = me.attr('data-title'),
            workspace = $('.view.content', container),
            layoutSize = container.data('layout-size'),
            size = $(window).width() / layoutSize - 15,
            viewer = $('<div/>', { class: 'viewer', 'data-type': type, html: "<div class='title-bar'><span class='title'>"+title+"</span><span class='right action close vtk-icon-cancel'/></div><div class='content'></div>"});

            // Attach close action to viewer
            $('.close', viewer).bind('click', function(){
                viewer.remove();
            });

            // Add viewer
            viewer.css('width', (size-2) + 'px').css('height', (20 + size - 2) + 'px').appendTo(workspace);

            // Provide content
            attachViewerContent(viewer, type, path);
        });

        // Tools controller
        $('.clear-workspace', container).bind('click', function(){
            $('.view.content', container).empty();
        });

        // Toggle view type
        $('.toggle-view', container).bind('click', function(){
            var me = $(this), toggles = $('.toggle-view', me.parent());
            toggles.removeClass('active');
            me.addClass('active');
            $('.view', container).hide();
            $('.view.' + me.attr('data-view'), container).show();
        });

        // Search panel data selection
        $('select[name="exploration"]').change(function(){
            var type_path = $(this).val().split(':');
            SEARCH_FACTORY[type_path[0]]($('.search-results', container), type_path[1]);
        }).trigger('change');
    }

    /**
     * jQuery catalyst view constructor.
     *
     * @member jQuery.vtkCatalystViewer
     * @param basePath
     * Root directory for data to visualize
     */

    $.fn.vtkCatalystAnalysis = function(dataBasePath) {
        return this.each(function() {
            var me = $(this).unbind().empty().addClass('vtkweb-catalyst-analysis');

            // Get meta-data
            $.ajax({
                url: dataBasePath + '/info.json',
                dataType: 'json',
                success: function( data ) {
                    // Store metadata
                    me.data('info', data);
                    me.data('layout-size', 3);

                    // Build UI
                    var toolbarContainer = $('<div/>', {
                        html: projectToHTML(data, dataBasePath)
                    });
                    toolbarContainer.appendTo(me);

                    // Attach interaction listeners
                    initializeListeners(me);
                },
                error: function(error) {
                    console.log("error when trying to download " + dataBasePath + '/info.json');
                    console.log(error);
                }
            });
        });
    }

    }(jQuery, window));