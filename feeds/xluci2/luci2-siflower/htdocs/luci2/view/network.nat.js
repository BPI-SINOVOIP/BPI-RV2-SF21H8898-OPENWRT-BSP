L.ui.view.extend({
	execute: function () {
		L.uci.load('firewall').then(function () {
			var rule = $('.rule').clone();
			var wanset;
			function format() {
				$('#ruletable .row').children('div:not(div:last-child)').css({
					width: '17%',
					'border-right': '1px solid #c9c9c9',
				});
				$('#ruletable .row').children('div:last-child').css('width', '15%');
				$('#ruletable .row').children('div').css({
					margin: '0',
					'text-align': 'center',
				});
				$('.rule div').css('line-height', '70px');
				$('#ruletable .rule:not(.rule:last-child)').css('border-bottom', '1px solid #c9c9c9');
				$('#type input').val(L.tr('Port Map'));
				$('#name input').attr('placeholder', L.tr('Must be at most %d characters').format(29));
				$('#EP input').attr('placeholder', '1~65535');
				$('#IP input').attr('placeholder', '1~65535');
				$('input,select').addClass('form-control').css('width', '55%');
				$('#settings div.row').css('margin', '10px 0');
				$('.edit_big').hover(
					function () {
						$(this).css('background-position', '46.9% 95.8085%');
					},
					function () {
						$(this).css('background-position', '46.3% 94.15%');
					},
				);
				$('.ic-del').hover(
					function () {
						$(this).css('background-position', '56.4% 99.226%');
					},
					function () {
						$(this).css('background-position', '56.4% 97.57%');
					},
				);
				$('#name input').keyup(function () {
					checkinput(1);
				});
				$('#Proto select').blur(function () {
					checkinput(2);
				});
				$('#EP input').keyup(function () {
					checkinput(3);
				});
				$('#IIP input').keyup(function () {
					checkinput(4);
				});
				$('#IP input').keyup(function () {
					checkinput(5);
				});
			}
			function showrule() {
				$('.head').nextAll().remove();
				var all = L.uci.sections('firewall');
				var countforrule = 0;
				for (var i = 0; i < all.length; i++) {
					if (!all[i]['name']) continue;
					if (all[i]['.type'] == 'zone' && all[i]['name'] == 'wan')
						wanset = all[i]['.name'];
					if (all[i]['.type'] == 'redirect' && all[i]['name'].indexOf('_pm') != -1) {
						onerule(all[i]['.name'], all[i]['name'].split('_pm')[0], all[i].proto, all[i].src_dport, all[i].dest_ip, all[i].dest_port);
						countforrule++;
					}
				}
				if (!countforrule) {
					var norule = $(rule).clone();
					$(norule)
						.removeClass('rule')
						.addClass('norule')
						.empty()
						.text(L.tr('No rules'))
						.css({
							'font-size': '16px',
							'text-align': 'center',
							'line-height': '70px',
						})
						.appendTo($('#ruletable'));
				}
				format();
			}
			function onerule(sid, name, proto, sdp, di, dp) {
				var a = $(rule).clone();
				a.prop('sid', sid);
				var sets = $(a).children('div');
				proto = String(proto).toUpperCase().replace(/ /g, '+');
				$(sets[0]).text(name);
				$(sets[1]).text(proto);
				$(sets[2]).text(sdp);
				$(sets[3]).text(di);
				$(sets[4]).text(dp);
				$('#ruletable').append(a);
			}
			function setError(elem, msg) {
				if (!msg) {
					$(elem).parent().removeClass('has-error');
					$(elem).find('.label').hide();
				} else {
					var error = $('<div />').addClass('label label-danger');
					$(elem).parent().addClass('has-error');
					if (!$(elem).find('.label').length) $(elem).append(error);
					$(elem).find('.label').text(msg).show();
				}
			}
			function checkinput(pos) {
				var end = pos;
				if (pos == 0) end = 5;
				for (; pos <= end; pos++) {
					switch (pos) {
						case 0:
							break;
						case 1:
							if (!$('#name input').val()) setError($('#name div:last-child'), L.tr('Field must not be empty'));
							else if (
								$('#name input')
									.val()
									.match(/^[a-zA-Z0-9_]+$/) == null
							)
								setError($('#name div:last-child'), L.tr('Must be a valid UCI identifier'));
							else if ($('#name input').val().length > 29) setError($('#name div:last-child'), L.tr('Must be at most %d characters').format(29));
							else setError($('#name div:last-child'));
							break;
						case 2:
							if (!$('#Proto select').val()) setError($('#Proto div:last-child'), L.tr('Field must not be empty'));
							else setError($('#Proto div:last-child'));
							break;
						case 3:
							if (!$('#EP input').val()) setError($('#EP div:last-child'), L.tr('Field must not be empty'));
							else if (
								$('#EP input')
									.val()
									.match(/^-?[0-9]+$/) == null ||
								Number($('#EP input').val()) < 0 ||
								Number($('#EP input').val()) > 65535
							)
								setError($('#EP div:last-child'), L.tr('Must be a valid port number'));
							else setError($('#EP div:last-child'));
							break;
						case 4:
							if (!$('#IIP input').val()) setError($('#IIP div:last-child'), L.tr('Field must not be empty'));
							else if (!L.parseIPv4($('#IIP input').val()) && !L.parseIPv6($('#IIP input').val())) setError($('#IIP div:last-child'), L.tr('Must be a valid IP address'));
							else setError($('#IIP div:last-child'));
							break;
						case 5:
							if (!$('#IP input').val()) setError($('#IP div:last-child'), L.tr('Field must not be empty'));
							else if (
								$('#IP input')
									.val()
									.match(/^-?[0-9]+$/) == null ||
								Number($('#IP input').val()) < 0 ||
								Number($('#IP input').val()) > 65535
							)
								setError($('#IP div:last-child'), L.tr('Must be a valid port number'));
							else setError($('#IP div:last-child'));
							break;
					}
				}
			}
			function saverule(sid) {
				L.uci.set('firewall', wanset, 'input', 'ACCEPT');
				L.uci.set('firewall', wanset, 'forward', 'ACCEPT');
				L.uci.set('firewall', sid, 'enabled', '1');
				L.uci.set('firewall', sid, 'target', 'DNAT');
				L.uci.set('firewall', sid, 'src', 'wan wan6');
				L.uci.set('firewall', sid, 'dest', 'lan');
				L.uci.set('firewall', sid, 'src_dport', $('#EP input').val());
				L.uci.set('firewall', sid, 'dest_ip', $('#IIP input').val());
				L.uci.set('firewall', sid, 'dest_port', $('#IP input').val());
				L.uci.set('firewall', sid, 'proto', $('#Proto select').val().toLowerCase().replace(/\+/g, ' '));
				L.uci.set('firewall', sid, 'name', $('#name input').val() + '_pm');
				L.uci.save();
				L.uci.apply();
			}
			function addrule(r) {
				if (!r) {
					$('#newrule .form-control:not(input:nth(1))').val('');
					checkinput(0);
					$('#newrule div:nth(0)').text(L.tr('Add New Server'));
					$('#save').click(function () {
						if ($('.has-error').length) return;
						var sid = L.uci.add('firewall', 'redirect');
						saverule(sid);
						$('#cancel').click();
						L.ui.setting(1, L.tr('Fresh rules...'));
						var sr = setTimeout(function () {
							clearTimeout(sr);
							$('.loading').remove();
							L.uci.load('firewall').then(function () {
								showrule();
							});
						}, 500);
					});
				} else {
					var sets = $(r).children('div');
					$('#name input').val($(sets[0]).text());
					$('#Proto select').val($(sets[1]).text());
					$('#EP input').val($(sets[2]).text());
					$('#IIP input').val($(sets[3]).text());
					$('#IP input').val($(sets[4]).text());
					checkinput(0);
					$('#newrule div:nth(0)').text(L.tr('Edit Server'));
					$('#save').click(function () {
						if ($('.has-error').length) return;
						var sid = r.prop('sid');
						saverule(sid);
						$('#cancel').click();
						L.ui.setting(1, L.tr('Fresh rules...'));
						var sr = setTimeout(function () {
							clearTimeout(sr);
							$('.loading').remove();
							L.uci.load('firewall').then(function () {
								showrule();
							});
						}, 500);
					});
				}
				$('#cancel').click(function () {
					$('#ruletable').show();
					$('#newrule').hide();
				});
				$('#ruletable').hide();
				$('#newrule').show();
			}
			$('.ic-add_normal').click(function () {
				addrule(0);
			});
			$('body').on('click', '.edit_big', function () {
				addrule($(this).parents('.rule'));
			});
			$('body').on('click', '.ic-del', function () {
				var now = $(this).parents('.rule');
				L.uci.remove('firewall', $(now).prop('sid'));
				L.uci.save();
				L.uci.apply();
				L.ui.setting(1, L.tr('Fresh rules...'));
				var sr = setTimeout(function () {
					clearTimeout(sr);
					$('.loading').remove();
					L.uci.load('firewall').then(function () {
						showrule();
					});
				}, 500);
			});
			L.uci.load('firewall').then(function () {
				showrule();
			});
		});
	}
})
