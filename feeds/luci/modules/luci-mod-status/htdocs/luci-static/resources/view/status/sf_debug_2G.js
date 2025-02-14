'use strict';
'require view';
'require fs';
'require ui';
'require uci';
'require form';

var isReadonlyView = !L.hasViewPermission();
var mapdata = { ifconfig_sec: {}, iwinfo_sec: {}, iw_station_sec: {}, txq_sec: {}, hwq_sec: {}, stats_sec: {}, lmactx_sec: {}, lmacrx_sec: {}, clk_summary_sec: {} };

return view.extend({
	load: function() {
		return uci.load('uhttpd').then(function() {
			var debug_disabled = uci.get('uhttpd', 'main', 'debug_disabled');
			if ( '0' === debug_disabled ){
				return Promise.all([
					L.resolveDefault(fs.exec_direct("/sbin/ifconfig", [ ]), null),
					L.resolveDefault(fs.exec_direct("/usr/bin/iwinfo", [ ]), null),
					L.resolveDefault(fs.exec_direct("/usr/sbin/iw", [ 'wlan1', 'station', 'dump' ]), null),
					fs.trimmed('/sys/kernel/debug/ieee80211/phy1/siwifi/txq'),
					fs.trimmed('/sys/kernel/debug/ieee80211/phy1/siwifi/hwq'),
					fs.trimmed('/sys/kernel/debug/ieee80211/phy1/siwifi/stats'),
					fs.trimmed('/sys/kernel/debug/ieee80211/phy1/siwifi/lmactx'),
					fs.trimmed('/sys/kernel/debug/ieee80211/phy1/siwifi/lmacrx'),
					fs.trimmed('/sys/kernel/debug/clk/clk_summary')
				]);
			}
			else{
				return Promise.all([
					L.resolveDefault(fs.exec_direct("/sbin/ifconfig", [ ]), null),
					L.resolveDefault(fs.exec_direct("/usr/bin/iwinfo", [ ]), null),
					L.resolveDefault(fs.exec_direct("/usr/sbin/iw", [ 'wlan1', 'station', 'dump' ]), null)
				]);
			}
		}).catch(function(){
			return Promise.all([
				L.resolveDefault(fs.exec_direct("/sbin/ifconfig", [ ]), null),
				L.resolveDefault(fs.exec_direct("/usr/bin/iwinfo", [ ]), null),
				L.resolveDefault(fs.exec_direct("/usr/sbin/iw", [ 'wlan1', 'station', 'dump' ]), null)
			]);
		});
	},

	handleDownload: function(rpc_replies) {
		var cmds = ['ifconfig', 'iwinfo', 'iw wlan1 station dump', 'txq', 'hwq', 'stats', 'lmactx', 'lmacrx', 'clk_summary'];
		var saveBtn = document.createElement('button');
		saveBtn.textContent = _('Save Debug Info');
		saveBtn.className = 'btn cbi-button-action important';
		var data = "";
		rpc_replies.forEach(function(array, i) {
			if ( i < 3 )
				data = data + cmds[i] + ":\n\n" + array;
			else
				data = data + "\n\n" + cmds[i] + ":\n\n" + array;
		});
		saveBtn.addEventListener('click', function(ev) {
			var blob = new Blob([data], {type: 'text/plain'});
			var url = URL.createObjectURL(blob);
			var a = document.createElement('a');
			a.href = url;
			a.download = '2G_Debug_Info.txt';
			a.click();
			URL.revokeObjectURL(url);
		});
		var container = document.createElement('div');
		container.classList.add('cbi-section-actions');
		container.appendChild(saveBtn);
		var map = document.getElementById('view');
		map.parentNode.appendChild(container, map);
	},

	render: function(rpc_replies) {
		if ( 3 == rpc_replies.length ){
			var ifconfig = rpc_replies[0],
		    iwinfo = rpc_replies[1],
		    iw_station_dump = rpc_replies[2],
			m, s, o;;

			m = new form.JSONMap(mapdata, _('2.4G Debug Info'));
			m.tabbed = true;
			m.readonly = isReadonlyView;

			s = m.section(form.NamedSection, 'ifconfig_sec', 'ifconfig');

			o = s.option(form.TextValue, 'ifconfig_content');
			o.forcewrite = true;
			o.rows = ifconfig.trim().split(/\n/).length + 1;
			o.load = function(section_id) {
				return ifconfig;
			};

			s = m.section(form.NamedSection, 'iwinfo_sec', 'iwinfo');

			o = s.option(form.TextValue, 'iwinfo_content');
			o.forcewrite = true;
			o.rows = iwinfo.trim().split(/\n/).length + 1;
			o.load = function(section_id) {
				return iwinfo;
			};

			s = m.section(form.NamedSection, 'iw_station_sec', 'iw wlan1 station dump');

			o = s.option(form.TextValue, 'iw_station_content');
			o.forcewrite = true;
			o.rows = iw_station_dump.trim().split(/\n/).length + 1;
			o.load = function(section_id) {
				return iw_station_dump;
			};

			return m.render().then(this.handleDownload(rpc_replies));
		}
		else {
			var ifconfig = rpc_replies[0],
		    iwinfo = rpc_replies[1],
		    iw_station_dump = rpc_replies[2],
			txq = rpc_replies[3],
		    hwq = rpc_replies[4],
		    stats = rpc_replies[5],
		    lmactx = rpc_replies[6],
		    lmacrx = rpc_replies[7],
			clk_summary = rpc_replies[8],
			m, s, o;;

			m = new form.JSONMap(mapdata, _('2.4G Debug Info'));
			m.tabbed = true;
			m.readonly = isReadonlyView;

			s = m.section(form.NamedSection, 'ifconfig_sec', 'ifconfig');

			o = s.option(form.TextValue, 'ifconfig_content');
			o.forcewrite = true;
			o.rows = ifconfig.trim().split(/\n/).length + 1;
			o.load = function(section_id) {
				return ifconfig;
			};

			s = m.section(form.NamedSection, 'iwinfo_sec', 'iwinfo');

			o = s.option(form.TextValue, 'iwinfo_content');
			o.forcewrite = true;
			o.rows = iwinfo.trim().split(/\n/).length + 1;
			o.load = function(section_id) {
				return iwinfo;
			};

			s = m.section(form.NamedSection, 'iw_station_sec', 'iw wlan1 station dump');

			o = s.option(form.TextValue, 'iw_station_content');
			o.forcewrite = true;
			o.rows = iw_station_dump.trim().split(/\n/).length + 1;
			o.load = function(section_id) {
				return iw_station_dump;
			};

			s = m.section(form.NamedSection, 'txq_sec', 'txq');

			o = s.option(form.TextValue, 'txq_content');
			o.forcewrite = true;
			o.rows = txq.trim().split(/\n/).length + 1;
			o.load = function(section_id) {
				return txq;
			};

			s = m.section(form.NamedSection, 'hwq_sec', 'hwq');

			o = s.option(form.TextValue, 'hwq_content');
			o.forcewrite = true;
			o.rows = hwq.trim().split(/\n/).length + 1;
			o.load = function(section_id) {
				return hwq;
			};

			s = m.section(form.NamedSection, 'stats_sec', 'stats');

			o = s.option(form.TextValue, 'stats_content');
			o.forcewrite = true;
			o.rows = stats.trim().split(/\n/).length + 1;
			o.load = function(section_id) {
				return stats;
			};

			s = m.section(form.NamedSection, 'lmactx_sec', 'lmactx');

			o = s.option(form.TextValue, 'lmactx_content');
			o.forcewrite = true;
			o.rows = lmactx.trim().split(/\n/).length + 1;
			o.load = function(section_id) {
				return lmactx;
			};

			s = m.section(form.NamedSection, 'lmacrx_sec', 'lmacrx');

			o = s.option(form.TextValue, 'lmacrx_content');
			o.forcewrite = true;
			o.rows = lmacrx.trim().split(/\n/).length + 1;
			o.load = function(section_id) {
				return lmacrx;
			};

			s = m.section(form.NamedSection, 'clk_summary_sec', 'clk_summary');

			o = s.option(form.TextValue, 'clk_summary_content');
			o.forcewrite = true;
			o.rows = clk_summary.trim().split(/\n/).length + 1;
			o.load = function(section_id) {
				return clk_summary;
			};

			return m.render().then(this.handleDownload(rpc_replies));
		}
	},

	handleSaveApply: null,
	handleSave: null,
	handleReset: null
});
