// this uses my RapidRipoff library ($rr()) so expect errors for just copying this
// RapidRipoff just adds some extensions to the Element prototype so it's really simple

holgol = {
	websocket: null,

	setup: function () {
		holgol.websocket = new WebSocket("ws://localhost:5701");
		holgol.websocket.binaryType = "arraybuffer";
		
		holgol.websocket.onopen = () => {holgol.logger("websocket connected");};
		holgol.websocket.onclose = () => {holgol.logger("websocket disconnected");};
		holgol.websocket.onmessage = holgol.onwsmessage;
		
		// query
		
		holgol.query.queryEntry = $("#queryEntry");
		if (holgol.query.queryEntry)
			holgol.query.queryEntry.addEventListener("change", holgol.query.updateUX);
		
		holgol.query.queryOptions = $("#queryOptions");
		
		holgol.query.queryRemoveOption = $("#queryRemoveOption");
		if (holgol.query.queryRemoveOption)
			holgol.query.queryRemoveOption.addEventListener("click", holgol.query.removeOption);
		
		holgol.query.queryAddOption = $("#queryAddOption");
		if (holgol.query.queryAddOption)
			holgol.query.queryAddOption.addEventListener("click", holgol.query.addOption);
		
		holgol.query.addOption();
		holgol.query.addOption();
		
		holgol.query.queryMaxAnswers = $("#queryMaxAnswers");
		if (holgol.query.queryMaxAnswers)
			holgol.query.queryMaxAnswers.addEventListener("change", holgol.query.updateUX);
		
		holgol.query.querySubmit = $("#querySubmit");
		if (holgol.query.querySubmit)
			holgol.query.querySubmit.addEventListener("click", holgol.query.submit);
		
		holgol.query.updateUX();
		
		// options
		
		//holgol.options
	},
	
	onwsmessage: function (message) {
		holgol.logger("recieved " + message.data);
	},
	
	logger: function (message) {
		console.log(message)
		if ($("#log"))
			$("#log").innerText = message.toString();
	},
	
	query: {
		queryEntry: null,
		queryOptions: null,
		queryRemoveOption: null,
		queryAddOption: null,
		queryMaxAnswers: null,
		querySubmit: null,
		optionElements: [],
		
		removeOption: function (count) {
			if (holgol.query.optionElements.length > 2 && holgol.query.queryOptions) {
				holgol.query.queryOptions.removeChild(holgol.query.optionElements.pop());
			}
			
			holgol.query.updateUX();
		},
		
		addOption: function (count) {
			if (holgol.query.optionElements.length < 8 &&holgol.query.queryOptions) {
				var option = $rr("input").withAttrib("type", "text").withClass("queryOption");
				option.withAttrib("placeholder", `Option ${holgol.query.optionElements.length+1}`);
				option.addEventListener("change", holgol.query.updateUX);
				holgol.query.queryOptions.appendChild(option);
				holgol.query.optionElements.push(option);
			}
			
			holgol.query.updateUX();
		},
		
		updateUX: function () {
			if (holgol.query.queryRemoveOption)
				holgol.query.queryRemoveOption.disabled = holgol.query.optionElements.length <= 2;
			
			if (holgol.query.queryAddOption)
				holgol.query.queryAddOption.disabled = holgol.query.optionElements.length >= 8;
			
			if (holgol.query.queryMaxAnswers && holgol.query.optionElements && holgol.query.optionElements.length > 0)
				holgol.query.queryMaxAnswers.max = holgol.query.optionElements.length;
			
			if (holgol.query.querySubmit) {
				var passEntry = false;
				var passOptions = false;
				
				if (holgol.query.queryEntry && !isNullOrWhitespace(holgol.query.queryEntry.value))
					passEntry = true;
				
				if (holgol.query.optionElements && holgol.query.optionElements.length >= 2 ) {
					passOptions = true;
					
					for (var i = 0; i < 2; i++) {
						if (isNullOrWhitespace(holgol.query.optionElements[i].value))
							passOptions = false;
					}
				}
				
				holgol.query.querySubmit.disabled = !(passEntry && passOptions);
			}
		},
		
		submit: function () {
			var message = { type: 0, timestamp: parseInt(Date.now() / 1000) };
			
			message.query = holgol.query.queryEntry.value;
			
			message.options = [];
			holgol.query.optionElements.forEach((x) => {
				if (!isNullOrWhitespace(x.value))
					message.options.push(x.value);
			});
			message.maxAnswers = parseInt(holgol.query.queryMaxAnswers.value);

			if (message.options.length >= 2 && holgol.websocket) {
				holgol.websocket.send(JSON.stringify(message));
				holgol.logger("sent " + JSON.stringify(message));
			}
		}
		
	},
	
	options: {
		
	}
}

// https://stackoverflow.com/a/32800728
function isNullOrWhitespace (input) {
	return !input || !input.trim();
}

document.addEventListener("DOMContentLoaded", holgol.setup);
