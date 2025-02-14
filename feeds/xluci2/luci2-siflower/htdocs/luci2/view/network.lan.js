L.ui.view.extend({
	cmd: L.rpc.declare({
	object: 'web.advance',
	method: 'cmd',
	params: ['cmd'],
	}),

	execute: function() {
		var self = this;
		var m = new L.cbi.Map('network', {
			caption:     L.tr('lan/dhcp Networksetting'),
			collabsible: true
		});

		var s = m.section(L.cbi.NamedSection, 'lan', {
//			caption:      L.tr('lan/dhcp network')
		});

		var e = s.option(L.cbi.InputValue, 'ipaddr', {
			caption:      L.tr('lan/dhcp ipaddress'),
			datatype:     'ip4addr'
		});

		var n = new L.cbi.Map('dhcp', {
			caption:     L.tr('dhcpsetting'),
			collabsible: true,
			topline:     true
		});

		var s2 = n.section(L.cbi.NamedSection, 'lan', {
//			caption:      L.tr('dhcp')
		});

		var e2 = s2.option(L.cbi.CheckboxValue, 'ignore', {
			caption:      L.tr('disable dhcp'),
			optional:     true
		});

		var a2 = s2.option(L.cbi.InputValue, 'start', {
			caption:      L.tr('dhcp starting address'),
			datatype:     'range(1,255)'
		});

		var dhcpend = s2.option(L.cbi.InputValue, '__dhcpend', {
                        caption:      L.tr('dhcp end address'),
                        datatype:     function(v, sid ) {

				var start_now = a2.formvalue('lan');
				var start_now1 = Number(start_now);
				if (v > 255 || v < 1 )
				{
					return L.tr('Please enter a value in the range of (1,255)');
				}
				else if (start_now1 > v && v <= 255 && v >= 0)
				{
					return L.tr('Please enter a value greater than the dhcp start address');
				}
				else
				{
					return true;
				}
			}
                });

                dhcpend.ucivalue = function(sid)
                {
                        var limit_now = this.ownerMap.get('dhcp', sid, 'limit');
                        var start_now = this.ownerMap.get('dhcp', sid, 'start');
                        var dhcpend_now = Number(limit_now)+Number(start_now)-1;
                        return (dhcpend_now);
                };


                dhcpend.save = function(sid){
                        var dhcpend_now = this.ownerSection.fields.__dhcpend.formvalue(sid);
                        var start_value = a2.formvalue(sid);
                        var limit_value = Number(dhcpend_now)-Number(start_value)+1;
                        var map = this.ownerMap;
                        map.set('dhcp', sid, 'limit', limit_value);
                        L.uci.save();
                }


		e2 = s2.option(L.cbi.InputValue, 'leasetime', {
			caption:      L.tr('dhcp lease time')
		});

	m.on('apply',function(){self.cmd("/etc/init.d/network restart")});
		n.insertInto('#amap');
		m.insertInto('#map');
	}
});
