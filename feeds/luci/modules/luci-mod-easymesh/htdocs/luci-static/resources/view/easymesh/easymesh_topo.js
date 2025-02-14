'use strict';
'require ui';
'require rpc';
'require uci';
'require view';
'require form';
'require view_iface';
'require dom';
'require poll';
'require fs';
'require network';
'require firewall';

return view.extend({
    load: function () {
        return Promise.all([
            uci.callLoad('easymesh'),
            fs.exec('/bin/sf_easymesh', ["get_topo_for_web"])
        ]);
    },

    render: function (data) {
        return Promise.all([
            fs.read("/tmp/mesh_topo.txt", "utf8")
        ]).then(function (topo) {
            var topoLines = topo[0].split('\n');
            console.log(topoLines)
            var status = data[0]['status'];
            var meshtypetip = [];

            meshtypetip = E('h1', {}, [_('Easymesh topology')]);

            var topoElements = topoLines.map(function(line) {
                // Bold name and ssid
                var formattedLine = line.replace(/(name: |ssid: | mac: )([^:\n\r]*?)(\s*mac:|$)/g, function(match, p1, p2, p3) {
                    if (p1 === "name: " || p1 === "ssid: ") {
                        return p1 + '<strong>' + p2.trim() + '</strong>' + p3;
                    } else {
                        return p1 + p2.trim() + p3;
                    }
                });

                // Double the indentation per line
                formattedLine = formattedLine.replace(/^ +/g, function(match) {
                    return match.replace(/ /g, '  ');
                });

                var paragraph = E('p', {style: 'white-space: pre-wrap; font-size: 18px;'});
                paragraph.innerHTML = formattedLine;

                return paragraph;
            });
            var topo_show = E('div', {style: 'white-space: pre-wrap; font-size: 18px;'}, topoElements);
            var component = E([], [meshtypetip, topo_show]);
            return component;

        }).catch(function(error) {
            var status = data[0]['status'];
            var meshtypetip = [];
            if(status.enable == '0') {
                meshtypetip = E('h3', {}, [_('Please Start Service')]);
                var component = E([], [meshtypetip]);
            } else {
                console.log("enabled")
                if(status.management_mode == 'agent') {
                    meshtypetip = E('h3', {}, [_('In agent state, Only controller can get the topo')]);
                    var component = E([], [meshtypetip]);
                } else if(status.is_set_mode == 'false'){
                    meshtypetip = E('h3', {}, [_('Not load driver, please load config in basic setting')]);
                    var component = E([], [meshtypetip]);
                } else {
                    fs.exec('/bin/sf_easymesh', ["get_topo_for_web"])
                    console.log(1)
                    meshtypetip = E('h3', {}, [_('Unknow error, please refresh page')]);
                    var component = E([], [meshtypetip]);
                }
            }
            return component;
        });
    },
    handleSaveApply: null,
    handleSave: null,
    handleReset: null
});