// this uses my RapidRipoff library ($rr()) so expect errors for just copying this
// RapidRipoff just adds some extensions to the Element prototype so it's really simple

holgol = {
	websocket: null,

	setup: function () {
		holgol.connectWebSocket();
		
		// query
		
		holgol.query.query = $("#query");
		holgol.query.queryEntry = setElemWithListener($("#queryEntry"), "change", holgol.query.updateUX);
		holgol.query.queryRemoveOption = setElemWithListener($("#queryRemoveOption"), "click", holgol.query.removeOption);
		holgol.query.queryAddOption = setElemWithListener($("#queryAddOption"), "click", holgol.query.addOption);
		holgol.query.queryOptions = $("#queryOptions");
		holgol.query.queryMaxAnswers = setElemWithListener($("#queryMaxAnswers"), "change", holgol.query.updateUX);
		holgol.query.querySubmit = setElemWithListener($("#querySubmit"), "click", holgol.query.submit);
		
		holgol.query.addOption();
		holgol.query.addOption();
		
		holgol.query.updateUX();
		
		// options
		
		holgol.options.options = $("#options");
		holgol.options.optionsQuery = $("#optionsQuery");
		holgol.options.optionsMaxAnswers = $("#optionsMaxAnswers");
		holgol.options.optionsChoices = $("#optionsChoices");
		holgol.options.optionsSubmit = setElemWithListener($("#optionsSubmit"), "click", holgol.options.submit);
		
		// this is default because the default state is no query
		if (holgol.options.options)
			holgol.options.options.style.display = "none";
	},
	
	connectWebSocket: function () {
		if (holgol.websocket)
			holgol.websocket.close();
		
		holgol.websocket = new WebSocket("ws://localhost:5701");
		holgol.websocket.binaryType = "arraybuffer";
		
		holgol.websocket.onopen = () => {holgol.logger("websocket connected");};
		holgol.websocket.onclose = () => {holgol.logger("websocket disconnected");};
		holgol.websocket.onmessage = holgol.onwsmessage;
	},
	
	onwsmessage: function (message) {
		holgol.logger("received " + message.data);
		
		var data = tryParseJSON(message.data);
		if (data && data.type != null) {
			switch (data.type) {
				case 0:
					holgol.options.handleQuery(data.timestamp, data.query, data.options, data.maxAnswers);
					break;
				case 2:
					holgol.options.handleTallies(data.tallies);
					break;
				case 3:
					holgol.options.handleWinner(data.winner);
					break;
			}
		}
	},
	
	logger: function (message) {
		console.log(message)
		if ($("#log"))
			$("#log").innerText = message.toString();
	},
	
	query: {
		query: null,
		queryEntry: null,
		queryRemoveOption: null,
		queryAddOption: null,
		queryOptions: null,
		queryMaxAnswers: null,
		querySubmit: null,
		optionElements: [],
		
		removeOption: function () {
			if (holgol.query.optionElements.length > 2 && holgol.query.queryOptions) {
				holgol.query.queryOptions.removeChild(holgol.query.optionElements.pop());
			}
			
			holgol.query.updateUX();
		},
		
		addOption: function () {
			if (holgol.query.optionElements.length < 8 && holgol.query.queryOptions) {
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
		options: null,
		optionsQuery: null,
		optionsMaxAnswers: null,
		optionsChoices: null,
		optionsSubmit: null,
		
		optionElements: [],
		choices: [],
		
		curQuery: null,
		curTallies: null,
		hasSubmitted: false,
		
		addOption: function (text) {
			var option = $rr("div").withClass("optionChoice")
			option.addEventListener("click", holgol.options.selectChoice);
			var textdisplay = $rr("p").withText(text);
			var tallycount = $rr("p").withText("0");
			
			option.apndChain(textdisplay).apndChain(tallycount);
			holgol.options.optionsChoices.appendChild(option);
			holgol.options.optionElements.push(option);
		},
		
		handleQuery: function (timestamp, query, options, maxAnswers) {
			if (parseInt(Date.now() / 1000) - timestamp > 45)
				return;
			if (isNullOrWhitespace(query))
				return;
			if (!Array.isArray(options))
				return;
			if (maxAnswers <= 0 || maxAnswers > options.length)
				return;
			
			if (optionsQuery)
				optionsQuery.innerText = query;
			if (optionsMaxAnswers)
				optionsMaxAnswers.innerText = maxAnswers == 1 ? "~ choose one ~ " : `~ choose up to ${maxAnswers} ~`;
			
			if (optionsChoices) {
				holgol.options.optionsChoices.textContent = '';
				
				for (var i = 0; i < options.length; i++) {
					holgol.options.addOption(options[i]);
				}
			}
			
			holgol.options.curQuery = {timestamp: timestamp, query: query, options: options, maxAnswers: maxAnswers}
			
			if (holgol.query.query)
				holgol.query.query.style.display = "none";
			if (holgol.options.options)
				holgol.options.options.style.display = "";
		},
		
		handleTallies: function (tallies) {
			if (tallies.length != holgol.options.optionElements.length)
				return;
			
			holgol.options.curTallies = tallies;
			
			for (var i = 0; i < holgol.options.optionElements.length; i++) {
				var option = holgol.options.optionElements[i];
				option.children[1].innerText = tallies[i];
			}
		},
		
		handleWinner: function (winner) {
			if (holgol.query.query)
				holgol.query.query.style.display = "";
			if (holgol.options.options)
				holgol.options.options.style.display = "none";
			
			if (winner != -1)
				holgol.logger(`[temporary] the winner of "${holgol.options.curQuery.query}" was "${holgol.options.curQuery.options[winner]}" with ${holgol.options.curTallies[winner]} vote(s)`);
			else
				holgol.logger(`[temporary] nobody voted on "${holgol.options.curQuery.query}"`);
			
			// why do I have to clear arrays like this
			holgol.options.optionElements.length = 0;
			holgol.options.choices.length = 0;
			
			holgol.options.hasSubmitted = false;
			if (holgol.options.optionsSubmit)
				holgol.options.optionsSubmit.style.display = "";
		},
		
		selectChoice: function (e) {
			if (holgol.options.hasSubmitted)
				return;
			
			var index = holgol.options.optionElements.indexOf(e.currentTarget);
			
			if (holgol.options.choices.indexOf(index) != -1) {
				holgol.options.choices.splice(holgol.options.choices.indexOf(index), 1);
				return;
			}
			
			if (holgol.options.choices.length >= holgol.options.curQuery.maxAnswers)
				holgol.options.choices.shift();
			holgol.options.choices.push(index);
			
			holgol.options.updateUX();
		},
		
		submit: function () {
			var message = { type: 1, choices: holgol.options.choices };

			if (message.choices.length > 0 && holgol.websocket) {
				holgol.websocket.send(JSON.stringify(message));
				holgol.logger("sent " + JSON.stringify(message));
				
				holgol.options.hasSubmitted = true;
				if (holgol.options.optionsSubmit)
					holgol.options.optionsSubmit.style.display = "none";
			}
		},
		
		updateUX: function () {
			for (var i = 0; i < holgol.options.optionElements.length; i++) {
				var option = holgol.options.optionElements[i];
				option.classList.remove("choiceSelected");
				
				if (holgol.options.choices.indexOf(i) != -1)
					option.classList.add("choiceSelected");
			}
		}
	}
}

// https://stackoverflow.com/a/32800728
function isNullOrWhitespace (input) {
	return !input || !input.trim();
}

function setElemWithListener (element, listener, func) {
	if (element)
		element.addEventListener(listener, func);
	return element;
}

// https://stackoverflow.com/a/20392392
function tryParseJSON (jsonString) {
	try {
		var o = JSON.parse(jsonString);
		
		if (o && typeof o === "object") {
			return o;
		}
	}
	catch (e) { }

	return false;
};

document.addEventListener("DOMContentLoaded", holgol.setup);
