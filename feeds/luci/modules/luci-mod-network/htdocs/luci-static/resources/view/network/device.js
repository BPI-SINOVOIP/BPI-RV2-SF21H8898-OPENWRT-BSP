'use strict';
'require ui';
'require view';
'require view_iface';
'require dom';
'require poll';
'require fs';
'require uci';
'require form';
'require network';
'require firewall';
'require rpc';

var getLocaltime = rpc.declare({
	object: 'luci',
	method: 'getLocaltime'
});


function getDeviceList() {
	return uci.callLoad('devlist').then(function (uci_devicelist) {
		var rv = [];

		for (let key in uci_devicelist) {
			if (uci_devicelist.hasOwnProperty(key)) {
				var uci_device = uci_devicelist[key],
					device = {};

				device.is_wifi = uci_device['is_wifi']
				if (device.is_wifi == 1) {
					device.ifname = uci_device['ifname']
				}
				else {
					device.port = uci_device['port']
				}

				device.ip = uci_device['ip'];
				if (device.ip == undefined) {
					device.ip = 'unknown';
				}
				device.DownSpeedLimit = uci_device['dslval'];
				device.UpSpeedLimit = uci_device['uslval'];
				if (device.DownSpeedLimit == undefined || device.DownSpeedLimit == '0') {
					device.DownSpeedLimit = 'none';
				}
				if (device.UpSpeedLimit == undefined || device.UpSpeedLimit == '0') {
					device.UpSpeedLimit = 'none';
				}
				device.mac = uci_device['mac'];
				device.hostname = uci_device['hostname'];
				device.online = uci_device['online'];
				device.latest_time = uci_device['latest_time'];
				device.vlan_id = uci_device['vlan_id'];
				device.internet = uci_device['internet'];
				if (device.internet == undefined) {
					device.internet = '1';
				}
				rv.push(device);
			}
		}
		return rv;
	})
}

return view.extend({
	load: function () {
		Promise.all([
			getDeviceList(),
			getLocaltime(),
			fs.lines("/proc/mac_mib")
		]).then(function (data) {
			this.refreshData(data);
		}.bind(this));

		setInterval(function () {
			this.refreshData();
		}.bind(this), 2000);
	},

	refreshData: function () {
		Promise.all([
			getDeviceList(),
			getLocaltime(),
			fs.lines("/proc/mac_mib")
		]).then(function (data) {
			var devinfo = data[0];
			var localtime = data[1]['result'];
			var mac_mib = data[2];

			devinfo = this.addTrafficInfo(devinfo, mac_mib);
			var v = this.render(devinfo, localtime);

			var container = document.getElementById('TableContainer');
			if (container.firstChild) {
				container.firstChild.remove();
			}
			container.appendChild(v);
		}.bind(this));
	},

	editLimit: function (device, ev) {
		var m = new form.Map('devlist'),
			s = m.section(form.NamedSection, 'deivce'),
			dsl, usl;

		var self = this;

		s.render = function () {
			return Promise.all([
				{},
				this.renderUCISection('deivce')
			]).then(this.renderContents.bind(this));
		};

		dsl = s.option(form.Value, 'dslval', _('DownSpeedLimit') + ' KB/s', _('Set 0 to cancel speedlimit'));
		dsl.datatype = 'and(uinteger,range(0,8589934))';
		dsl.placeholder = _('Set DownSpeedLimit...');

		usl = s.option(form.Value, 'uslval', _('UpSpeedLimit') + ' KB/s', _('Set 0 to cancel speedlimit'));
		usl.datatype = 'and(uinteger,range(0,8589934))';
		usl.placeholder = _('Set UpSpeedLimit...');

		m.render().then(L.bind(function (nodes) {
			ui.showModal(_('Set SpeedLimit...'), [
				nodes,
				E('div', { 'class': 'right' }, [
					E('button', {
						'class': 'btn',
						'click': ui.hideModal
					}, _('Cancel')), ' ',
					E('button', {
						'class': 'cbi-button cbi-button-positive important',
						'click': function (ev) {
							var dslval = dsl.formvalue('deivce'),
								uslval = usl.formvalue('deivce'),
								mac = device["mac"].replace(/:/g, "_"),
								vlan_id = device["vlan_id"],
								spl_index = String(device["index"]);
							fs.exec('/sbin/set_spl', [mac, String(dslval), String(uslval), vlan_id, spl_index]);
							self.refreshData();
							ui.hideModal();
						}
					}, _('Set SpeedLimit'))
				])
			], 'cbi-modal');
		}, this));
	},

	forbidDevice: function (device, ev) {
		var mac = mac = device["mac"].replace(/:/g, "_");
		var checked = document.getElementById(device["mac"] + '-switch').checked;
		var tasks = [];
		if (checked) {
			tasks.push(uci.callSet('devlist', mac, { 'internet': '0' }));
		}
		else {
			tasks.push(uci.callSet('devlist', mac, { 'internet': '1' }))
		}
		return Promise.all(tasks)
			.then(L.bind(ui.changes.init, ui.changes))
			.then(L.bind(ui.changes.apply, ui.changes));
	},

	updateTable: function (table, data, localtime) {
		var rows = [];

		for (var i = 0; i < data.length; i++) {
			var device = data[i];
			if (device['online'] == '0') {
				continue;
			}
			var dev_time = localtime - device['latest_time'];

			if (device['latest_time'] == 0 || dev_time < 0)
				dev_time = 0;
			var days = Math.floor(dev_time / 86400).toString();
			var hours = Math.floor((dev_time - days * 86400) / 3600).toString();
			var mins = Math.floor((dev_time - days * 86400 - hours * 3600) / 60).toString();
			var time = days + _('days') + hours + _('hours') + mins + _('mins');
			var deviceinfo = device['mac'] + '<br>' + device['ip'] + '<br>' + time;
			var id = device['mac'] + '-switch'
			rows.push([
				deviceinfo,
				device['DownSpeed'],
				device['UpSpeed'],
				device['DownSpeedLimit'],
				device['UpSpeedLimit'],
				E('div', {}, [
					E('button', {
						'class': 'btn cbi-button-action',
						'click': ui.createHandlerFn(this, 'editLimit', device)
					}, _('Edit')), ' ',
					E('label', { style: 'display: flex;' }, _('Forbid')),
					E('input', {
						'type': 'checkbox',
						'id': id,
						'checked': device['internet'] == "1" ? null : '',
						'change': ui.createHandlerFn(this, 'forbidDevice', device)
					}, _('Forbid'))
				])
			]);
		}

		cbi_update_table(table, rows, E('em', _('No information available')));
	},

	addTrafficInfo: function (devinfo, mac_mib) {
		for (var i = 1; i < mac_mib.length; i++) {
			var parts = mac_mib[i].trim().split(/\s+/),
				mac = parts[0].toUpperCase(),
				upload_bps = parts[2],
				download_bps = parts[3];

			for (var j = 0; j < devinfo.length; j++) {
				if (devinfo[j]['mac'] == mac) {
					devinfo[j]['UpSpeed'] = (parseFloat(upload_bps) / 8192).toFixed(2);
					devinfo[j]['DownSpeed'] = (parseFloat(download_bps) / 8192).toFixed(2);
					devinfo[j]['index'] = i;
					break;
				}
			}
		}

		return devinfo;
	},

	render: function (devinfo, localtime) {
		var v = E([], [
			E('h2', _('Device Management')),
			E('div', { 'id': 'TableContainer' }, 'Loading...')
		]);

		if (!devinfo || !localtime) {
			return v;
		}

		var table = E('table', { 'class': 'table' }, [
			E('tr', { 'class': 'tr table-titles' }, [
				E('th', { 'class': 'th' }, _('DeviceInfo')),
				E('th', { 'class': 'th' }, _('DownSpeed') + ' KB/s'),
				E('th', { 'class': 'th' }, _('UpSpeed') + ' KB/s'),
				E('th', { 'class': 'th' }, _('DownSpeedLimit') + ' KB/s'),
				E('th', { 'class': 'th' }, _('UpSpeedLimit') + ' KB/s'),
				E('th', { 'class': 'th center nowrap cbi-section-actions' })
			])
		]);

		this.updateTable(table, devinfo, localtime);

		return table;
	},
});