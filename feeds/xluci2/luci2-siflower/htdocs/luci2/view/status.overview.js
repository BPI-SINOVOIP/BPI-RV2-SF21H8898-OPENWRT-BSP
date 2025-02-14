L.ui.view.extend({

    execute: function() {

		function make_memory_info(v, total){
			return ('%d kB / %d kB (%d%%)').format(v / 1024, total / 1024, Math.round(v / total * 100));
		}

		L.system.getInfo().then(function(info) {
			$('#boardtype').text(info.model);
			$('#software_version').text(info.release.description.split('openwrt1806_')[1]);
			$('#kernel_version').text(info.kernel);
			var total = info.memory.total;
			$('#memory_1').attr("style", "background-color:#fb4444; width: " + Math.round((info.memory.free + info.memory.buffered) / total * 100)+"%");
			$('#memory_1_text').text(make_memory_info(info.memory.free + info.memory.buffered, total));
			$('#memory_2').attr("style", "background-color:#fb4444; width: " + Math.round(info.memory.free / total * 100)+"%");
			$('#memory_2_text').text(make_memory_info(info.memory.free, total));
			$('#memory_3').attr("style", "background-color:#fb4444; width: " + Math.round(info.memory.shared / total * 100)+"%");
			$('#memory_3_text').text(make_memory_info(info.memory.shared, total));
			$('#memory_4').attr("style", "background-color:#fb4444; width: " + Math.round(info.memory.buffered / total * 100)+"%");
			$('#memory_4_text').text(make_memory_info(info.memory.buffered, total));
		});
    }
});
