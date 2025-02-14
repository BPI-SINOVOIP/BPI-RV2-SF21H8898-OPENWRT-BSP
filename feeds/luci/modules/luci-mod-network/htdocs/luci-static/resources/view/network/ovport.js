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

var setOvport = rpc.declare({
    object: 'dpns.nat',
    method: 'ovport_set',
    params: ['ifname']
});

var getOvport = rpc.declare({
    object: 'dpns.nat',
    method: 'ovport_get',
});

return view.extend({
    load: function () {
        return Promise.all([
            getOvport(),
            uci.load('wireless')
        ]);
    },

    handleSave: function () {
        const enableOvportCheckbox = document.getElementById('enableOvportCheckbox');
        const ovportSelect = document.getElementById('ovportSelect');

        if (!enableOvportCheckbox.checked) {
            setOvport('none');
        }
        else {
            setOvport(ovportSelect.value);
        }
    },

    handleEnableOvportChange: function () {
        const enableOvportCheckbox = document.getElementById('enableOvportCheckbox');
        const ovportlabel = document.getElementById('ovportlabel');
        const ovportSelect = document.getElementById('ovportSelect');

        if (enableOvportCheckbox.checked) {
            ovportlabel.style.display = 'block';
            ovportSelect.style.display = 'block';
            ovportSelect.value = 'wlan0';
        }
        else {
            ovportlabel.style.display = 'none';
            ovportSelect.style.display = 'none';
        }
    },

    render: function (data) {
        var ovport = data[0]['ifname'];
        var wifiIfaces = uci.sections('wireless', 'wifi-iface');

        var enableOvportCheckbox = E('input', {
            'type': 'checkbox',
            'id': 'enableOvportCheckbox',
            change: this.handleEnableOvportChange.bind(this)
        });

        var enableOvportlabel = E('label', { 'id': 'enableOvportlabel' }, _('Enable Ovport'));

        var ovportSelect = E('select', { 'id': 'ovportSelect' }, []);

        for (var i = 0; i < wifiIfaces.length; i++) {
            if (wifiIfaces[i].hasOwnProperty('disabled') && wifiIfaces[i].disabled == "1")
                continue;
            var ifname = '' + wifiIfaces[i].ifname;
            var option = E('option', { 'value': ifname }, [ifname]);
            ovportSelect.appendChild(option);
        }

        var ovportlabel = E('label', { 'id': 'ovportlabel' }, _('Select Port'))

        var saveButton = E('button', {
            'class': 'btn cbi-button-action',
            'id': 'saveButton',
            'click': this.handleSave.bind(this)
        }, _('Save'));

        var component = E([], [
            E('h2', {}, [_('Ovport')]),
            E('div', { style: 'display: flex; align-items: center; margin-bottom: 10px; margin-top: 10px; width: 400px;' },
                [E('div', { style: 'width: 200px' }, [enableOvportlabel]),
                    enableOvportCheckbox]
            ),
            E('div', { style: 'display: flex; align-items: center; margin-bottom: 10px; margin-top: 10px; width: 400px;' },
                [E('div', { style: 'width: 200px' }, [ovportlabel]),
                    ovportSelect]
            ),
            E('div', { style: 'display: flex; align-items: center; margin-bottom: 10px; margin-top: 10px; width: 400px;' },
                [saveButton]
            )
        ]);

        if (ovport != 'none' && ovport != undefined) {
            enableOvportCheckbox.checked = 1;
            ovportlabel.style.display = 'block';
            ovportSelect.style.display = 'block';
            ovportSelect.value = ovport;
        }
        else {
            enableOvportCheckbox.checked = 0;
            ovportlabel.style.display = 'none';
            ovportSelect.style.display = 'none';
        }
        return component;
    }
});