L.ui.view.extend({

	ip_mac_cmd: L.rpc.declare({
		object: 'web.advance',
		method: 'cmd',
		params: [ 'cmd' ],
	}),

	getARPTable: L.rpc.declare({
		object: 'luci2.network',
		method: 'arp_table',
		expect: { entries: [ ] },
		filter: function(data, params) {
			var tmp = [ ];
			for (var i = 0; i < data.length; i++)
				if (data[i].macaddr != '00:00:00:00:00:00')
					tmp.push(data[i]);
			return tmp;
		}
	}),

	getHostName: L.rpc.declare({
		object: 'luci2.network',
		method: 'dhcp_leases',
		expect: { leases: [ ] },
		filter: function(data, params) {
		var tmp = [ ];
			for (var i = 0; i < data.length; i++)
				if (data[i].macaddr != '00:00:00:00:00:00')
					tmp.push(data[i]);
			return tmp;
		}
	}),

	callLoad: L.rpc.declare({
		object: 'uci',
		method: 'get',
		params: [ 'config' ],
		expect: { values: { } }
	}),

	callLoad: L.rpc.declare({
		object: 'uci',
		method: 'get',
		params: [ 'config' ],
		expect: { values: { } }
	}),

	callDelete: L.rpc.declare({
		object: 'uci',
		method: 'delete',
        params: [ 'config', 'section']
    }),

	callCommit: L.rpc.declare({
		object: 'uci',
		method: 'commit',
		params: [ 'config' ]
	}),

	execute: function() {
		var self = this;
		L.globals.flag = 0;
		function loadBoundTable(){
			var boundresult = [ ];
			$('#mapping').find('.listtable').remove();
			var load = self.callLoad("bindtable").then(function(values){
				for(var key in values){
					boundresult.push({
						hostname:values[key].name,
						macaddr:values[key].mac,
						ipaddr:values[key].ip
					});
				}
				for(var i = 0; i < boundresult.length; i ++){
					$('#mapping').append($('<div />')
						.append($('<div />').addClass('border_top listtable height_70 text_center')
							.append($('<div />').addClass('pull-left border_1px_right height_70').css('width','200px')
								.append($('<div />').text(boundresult[i].hostname).css('margin-top','25px')))
							.append($('<div />').addClass('pull-left border_1px_right height_70').css('width','200px')
								.append($('<div />').text(boundresult[i].macaddr).css('margin-top','25px')))
							.append($('<div />').addClass('pull-left border_1px_right height_70').css('width','180px')
								.append($('<div />').text(boundresult[i].ipaddr).css('margin-top','25px').attr('id',boundresult[i].ipaddr)))
							.append($('<div />').addClass('pull-left border_1px_right height_70').css({'width':'80px','border':'none'})
								.append($('<div />').addClass('ic-del').css({"margin-top":"20px","margin-left":"33px"}).click(function(){
									var mac = $(this).parent().prev().prev().children().text();
									mac = mac.split(':');
									mac = mac.join('_');
									self.callDelete('bindtable',mac).then(function(){
										self.callCommit('bindtable').then(function(){
											 self.ip_mac_cmd("/www/luci2/scripts/refresh_ip.sh").then(function(){
												 loadBoundTable();
											 })
										})
									})
								})))));
				}
			});
			return load;
		}

		function loadSelect(){
			var load;
			var result = [ ];
			var z = 1;
			var t = 1;
			var h = "anonymous_hostname";
			self.getARPTable().then(function(arp){
				self.getHostName().then(function(leases){
					self.callLoad("bindtable").then(function(values){
						$('#test').remove();
						for(var i = 0; i < arp.length; i++){
							if(arp[i].device == 'br-lan'){
								result.push({
									macaddr:arp[i].macaddr,
									ipaddr:arp[i].ipaddr,
									hostname:h + z
								});
								z ++;
							}
						}
						for(var i = 0; i < result.length; i ++){
							for(var j = 0; j < leases.length; j++){
								if(leases[j].macaddr == result[i].macaddr && leases[j].hostname != undefined){
									result[i].hostname = leases[j].hostname + t;
									t ++;
								}
							}
							for(var key in values){
								if(values[key].mac == result[i].macaddr && values[key].ip == result[i].ipaddr){
									result[i].hostname = values[key].name;
								}
							}
						}

						$('#select_device').append($('<div />').addClass('list_fixed border_1px').attr('id','test').css({'max-height':'222px','height':'222px','display':'none'})
							.append($('<ul />').attr('id','lan_HostName_ctrl_selectlist').addClass('select_list').css({'margin-right':'-12px','padding-right':'0px','height':'100%','overflow':'auto'})
								.append($('<li />').attr('id','new_selectlist_SmallSelectBoxScrollItemID').addClass('paddingleft_10 paddingright_15 active').text(L.tr("Manual input")))));
						for(var i = 0; i < result.length; i ++){
							$('#lan_HostName_ctrl_selectlist').append($('<li />').attr('id',result[i].hostname).addClass('paddingleft_10 paddingright_15').text(result[i].hostname));
						}

						$('#lan_HostName_ctrl_selectlist_parenselect').blur(function(){
							var text = $('#lan_HostName_ctrl_selectlist_parenselect').val();
							if(text != L.tr("Manual input"))
								$('#inpu').hide();
							$('#test').css('display','none');
						});

						$('#lan_HostName_ctrl_selectlist_parenselect').mousedown(function(){
							$('#test').css('display','');
							$('#inpu').show();
							L.globals.flag = 0;
							$("ul#lan_HostName_ctrl_selectlist").on("mousedown","li",function(){
								$('#test').css('display','none');
								if(this.id == 'new_selectlist_SmallSelectBoxScrollItemID'){
									$('#lan_HostName_ctrl_selectlist_parenselect').val(L.tr("Manual input"));
									L.globals.flag = 0;
									$('#inpu').show();
									$('#hostname').val('');
									$('#lan_IpAddressReserve_chaddr_ctrl').removeProp("readonly").val('');
									$('#lan_IpAddressReserve_ipaddr_ctrl').val('');
								}
								else{
									L.globals.flag = 1;
									$('#hostname').val('');
									$('#inpu').hide();
									$('#lan_HostName_ctrl_selectlist_parenselect').val(this.id);
									L.globals.hostname = this.id;
									for(var i = 0; i < result.length; i ++){
										if(this.id == result[i].hostname){
											$('#lan_IpAddressReserve_chaddr_ctrl').val(result[i].macaddr).prop("readonly",true);
											$('#lan_IpAddressReserve_ipaddr_ctrl').val(result[i].ipaddr);
										}
									}
								}
							});
						});

					});
				});
			});
			return load;
		}

		loadBoundTable();
		$('#add_route_btn').click(function(){
			$('#mapping').append($('<div />').addClass('PopWin').attr('id','popwin')
						.append($('<div />').addClass('PopWin_box pop_absolute_center').css({'width':'700px','margin':'-241px -350px'})
							.append($('<div />').addClass('popWin_title table_cell').css('width','700px').text(L.tr("Static IP-MAC Bind")))
							.append($('<div />').addClass('PopWin_content').attr('id','content')
								.append($('<div />').css('margin-top','30px')
									.append($('<div />').addClass('row InputContent')
										.append($('<div />').attr('id','lan_HostName_labelinfo').css('font-size','15px').text(L.tr("Select device")))
									.append($('<div />').css('width','263px').attr('id','main')
										.append($('<div />').addClass('select_box border_1px paddingleft_14').css({'height':'37px','line-height':'37px'}).attr('id','select_device')
												.append($('<input />').attr('id','lan_HostName_ctrl_selectlist_parenselect').addClass('inputArea paddingright_14').val(L.tr("Manual input")).css({'border':'none','width':'200px','height':'30px','outline':'none'}).prop('readonly',true))
										.append(loadSelect)))))
								.append($('<div />').css('margin-top','20px')
									.append($('<div />').addClass('InputComponent')
										.append($('<div />').addClass('InputContent row')
											.append($('<div />').attr('id','lan_IpAddressReserve_chaddr_labelinfo').text(L.tr("MAC address")))
											.append($('<div />').attr('id','lan_IpAddressReserve_chaddr').addClass('inputBox')
												.append($('<input />').addClass('inputArea paddingleft_14').attr('id','lan_IpAddressReserve_chaddr_ctrl').css({'width':'243px','height':'37px','text-align':'left','border':'1px solid rgb(201,201,201)','padding-right':'14px','outline':'none'}))))
										.append($('<div />').addClass('row InputDesc')
											.append($('<div />').addClass('marginleft_215').text(L.tr("MAC address format") + ': XX:XX:XX:XX:XX:XX')))))
								.append($('<div />').css('margin-top','20px')
									.append($('<div />').addClass('InputComponent')
										.append($('<div />').addClass('InputContent row')
											.append($('<div />').attr('id','lan_IpAddressReserve_ipaddr_labelinfo').text(L.tr("IP address")))
											.append($('<div />').attr('id','lan_IpAddressReserve_ipaddr').addClass('inputBox')
												.append($('<input />').addClass('inputArea paddingleft_14').attr('id','lan_IpAddressReserve_ipaddr_ctrl').css({'width':'243px','height':'37px','text-align':'left','border':'1px solid rgb(201,201,201)','padding-right':'14px','outline':'none'}))))
										.append($('<div />').addClass('row InputDesc')
											.append($('<div />').addClass('marginleft_215').text(L.tr("IP format") + ': XXX.XXX.XXX.XXX'))))))
								.append($('<div />').css('margin-top','20px').attr('id','inpu')
									.append($('<div />').addClass('InputComponent')
										.append($('<div />').addClass('InputContent row')
											.append($('<div />').attr('id','lan_IpAddressReserve_chaddr_labelinfo').text(L.tr("Device name")))
											.append($('<div />').attr('id','lan_IpAddressReserve_chaddr').addClass('inputBox')
												.append($('<input />').addClass('inputArea paddingleft_14').attr('id','hostname').css({'width':'243px','height':'37px','text-align':'left','border':'1px solid rgb(201,201,201)','padding-right':'14px','outline':'none'}))))))
					.append($('<div />').addClass('PopWin_buttons').css('width','386px')
						.append($('<div />').addClass('button fl button_common_short').attr('id','calcelipserver').text(L.tr("Cancel")).css('margin','7px'))
						.append($('<div />').addClass('button fl button_common_short').attr('id','postserver').text(L.tr("Ok")).css('margin','7px')))));

			$('#calcelipserver').click(function(){
				$('#popwin').remove();
			});
			$('#postserver').click(function(){
				if(L.globals.flag == 0)
					var name = $('#hostname').val();
				else if(L.globals.flag == 1){
					var name = L.globals.hostname;
					L.globals.flag = 0;
				}
				var mac = $('#lan_IpAddressReserve_chaddr_ctrl').val();
				var ip = $('#lan_IpAddressReserve_ipaddr_ctrl').val();
				var res = "/www/luci2/scripts/ip_mac.sh %s %s %s".format(mac,name,ip);
				$('#popwin').remove();
				self.ip_mac_cmd(res).then(function(){
					loadBoundTable();
				});
			});
		});
	}
});
