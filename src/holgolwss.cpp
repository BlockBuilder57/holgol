#include <holgolwss.h>

#include <consoleinteraction.h>

#include <ctime>
#include <vector>
#include <set>
#include <cstdint>

// This is only for commands to use to reference the one and true
// holgol server instance, without having to make a GetHolgol() function
// or something like that. Don't use it in class code.
extern HolgolWebsocketServer g_holgol;
extern std::uint16_t g_port;

struct StatusCommand : public ConsoleCommand
{

	void Invoke() override
	{
		std::cout << "Server status: Running (on port " << g_port << ")" << std::endl;
		std::cout << "Server in query: " << std::boolalpha << g_holgol.InQuery() << std::endl;

		if(g_holgol.InQuery()) {
			// provide a "fancy" output of the current query stats

			auto& query = g_holgol.GetQuery();
			auto& tallies = g_holgol.GetVoteTallies();
			
			std::cout << "\t\b\b\b\bQuery: " << std::quoted(query.query) << std::endl;
			std::cout << "\t\b\b\b\bQuery Creator IP: " << query.ip.to_string() << std::endl;
			std::cout << "\t\b\b\b\bTimestamp: " << query.timestamp << std::endl; // todo: strftime?
			std::cout << "\t\b\b\b\bMax answers: " << query.maxAnswers << std::endl;
			std::cout << "\t\b\b\b\bOptions: " << std::endl;

			for (std::size_t i = 0; i < query.options.size(); ++i)
				std::cout << "\t" << query.options[i] << ": " << tallies[i] << " votes " << std::endl;
		}
	}

	std::string GetName() override
	{
		return "status";
	}

	std::string GetDescription() override
	{
		return "Outputs the current server status.";
	}
};

struct EndQueryCommand : public ConsoleCommand
{

	void Invoke() override
	{
		if(!g_holgol.InQuery())
		{
			std::cout << "Can't end a query if there's no query active, silly!\n";
			return;
		}
		g_holgol.CancelVote();
		std::cout << "Ended query by force. Have mercy on me.\n";
	}

	std::string GetName() override
	{
		return "endquery";
	}

	std::string GetDescription() override
	{
		return "Ends the current query.";
	}
};

// static console register doodads go here.
static ConsoleCommandRegisterHelper<StatusCommand> _status_register;
static ConsoleCommandRegisterHelper<EndQueryCommand> _endquery_register;

HolgolWebsocketServer::HolgolWebsocketServer(boost::asio::io_context &context)
	: BaseWebsocketServer(context), voteTimer(context)
{
}

// in seconds
#ifdef _DEBUG
const uint64_t HolgolWebsocketServer::VoteTime = 15;
#else
const uint64_t HolgolWebsocketServer::VoteTime = 45;
#endif

void HolgolWebsocketServer::OnStart()
{
	//main thread
}

void HolgolWebsocketServer::OnOpen(websocketpp::connection_hdl handle)
{
	BaseWebsocketServer::OnOpen(handle);

	std::cout << "User connected with IP " << GetIPAddress(handle).to_string() << " " << std::endl;

	if(inQuery)
	{
		// Send them the current query and vote tallies
		// if they join mid-query.

		SendJSON(handle, curQuery.toJSONObject());
		SendTallies(handle);

#ifdef _DEBUG
		std::cout << "in a query, sent them the current query and votes" << std::endl;
#endif
	}
}

void HolgolWebsocketServer::OnClose(websocketpp::connection_hdl handle)
{
	BaseWebsocketServer::OnClose(handle);

	std::cout << "User left with IP address " << GetIPAddress(handle).to_string() << " \n";

	if(inQuery)
	{
		boost::asio::ip::address handleaddr = GetIPAddress(handle);

		if(votes.find(handleaddr) != votes.end())
		{
			std::cout << "User had votes, subtracting\n";

			for(auto choice : votes[handleaddr].choices)
				if(choice < curQuery.options.size() && choice >= 0)
					tallies[choice]--;

			votes.erase(handleaddr);
			BroadcastTallies();
		}

	}
}

void HolgolWebsocketServer::OnMessage(websocketpp::connection_hdl handle, message_ptr message)
{
	// if it's not binary throw it out
	if(message->get_opcode() != websocketpp::frame::opcode::text)
		return;

	boost::system::error_code ec;


	boost::json::value payload_value = boost::json::parse(message->get_payload(), ec);

	// All of our packets are JSON.
	// If someone sends invalid data, then they are probably up to no good.
	if(ec)
	{
		std::cout << "Someone is being naughty (JSON parsing failure): " << ec.message() << "\n";	
		InvalidDataClose(handle);
		return;
	}

	boost::json::object payload = payload_value.as_object();

	if(!payload["type"].is_null())
	{
		switch((HolgolMessageType)payload["type"].as_int64())
		{
		case HolgolMessageType::Query: {
			// ignore someone thinking they are funny
			if(inQuery)
				break;

			HolgolQuery query = HolgolQuery::fromJSONObject(payload);

			if(query.timestamp != -1)
			{
				if(query.options.size() < 2 || query.options.size() > 8)
				{
					std::cout << "Rejecting query for too many or too few options" << std::endl;
					break;
				}

				// check timestamp was created in last second
				if(auto n = abs(difftime(query.timestamp, std::time(nullptr))); n > 2)
				{
					std::cout << "Rejecting query for out of date timestamp" << std::endl;
					printf("abs(difftime(%ld, %ld): %d", query.timestamp, std::time(nullptr), n);
					break;
				}

				if(query.maxAnswers <= 0 || query.maxAnswers > query.options.size())
				{
					std::cout << "Rejecting query for oob maxAnswers size" << std::endl;
					break;
				}

				// tie query creator ip
				query.ip = GetIPAddress(handle);

				printf("new query:\n");
				printf("\twhat's timestamp %ld\n", query.timestamp);
				printf("\twhat's query %s\n", query.query.c_str());
				printf("\twhat's the creator %s\n", query.ip.to_string().c_str());

				for(int i = 0; i < query.options.size(); i++)
				{
					printf("\twhat's options[%d] %s\n", i, query.options[i].c_str());
				}
				printf("\twhat's maxAnswers %ld\n", query.maxAnswers);

				inQuery = true;
				curQuery = query;
				votes.clear();
				tallies.resize(query.options.size());
				BroadcastJSON(query.toJSONObject());

				// finally, start the vote!
				boost::system::error_code ec;
				voteTimer.expires_from_now(std::chrono::seconds(HolgolWebsocketServer::VoteTime), ec);
				voteTimer.async_wait(std::bind(&HolgolWebsocketServer::VoteTimerFinished, this, std::placeholders::_1));
			}

			break;
		}
		case HolgolMessageType::MakeVote: {
			// how are you gonna vote with no query
			if(!inQuery)
				break;

			HolgolVote vote = HolgolVote::fromJSONObject(payload);
			boost::asio::ip::address handleaddr = GetIPAddress(handle);

			if(vote.choices.size() <= curQuery.maxAnswers && votes.find(handleaddr) == votes.end())
			{
				votes[handleaddr].choices = vote.choices;

				for(auto choice : vote.choices)
					if(choice < curQuery.options.size() && choice >= 0)
						tallies[choice]++;

				BroadcastTallies();
			}

			break;
		}
		default:
#ifdef _DEBUG
			std::cout << "Unhandled/invalid type, someone is being very naughty" << std::endl;
#else
			return;
#endif
		}
	}
	else { 
		// If this is missing from our JSON 
		//std::cout << "someone is being naughty\n";
		
		InvalidDataClose(handle);
	}
}

void HolgolWebsocketServer::InvalidDataClose(websocketpp::connection_hdl handle) {
		websocketpp::lib::error_code ec;
		auto con = _server.get_con_from_hdl(handle, ec);

		if (ec)
			return;

		con->close(websocketpp::close::status::normal, "stop trying to hack my shit");
}

boost::json::object HolgolWebsocketServer::MakeTallies()
{
	boost::json::object obj;
	boost::json::array tallies_array;
	obj["type"] = (uint32_t)HolgolMessageType::BroadcastTallies;

	for(auto &tally : tallies)
		tallies_array.emplace_back(tally);

	obj["tallies"] = tallies_array;
	return obj;
}

void HolgolWebsocketServer::SendTallies(websocketpp::connection_hdl handle)
{
	boost::json::object obj = MakeTallies();
	SendJSON(handle, obj);
}
void HolgolWebsocketServer::BroadcastTallies()
{
	boost::json::object obj = MakeTallies();
	BroadcastJSON(obj);
}

void HolgolWebsocketServer::VoteTimerFinished(const boost::system::error_code &ec)
{
	if(ec)
		std::cout << "timer errored: " << ec.message() << '\n';

	if (ec.value() == boost::asio::error::operation_aborted)
		forceEnd = true;

	boost::json::object obj;
	obj["type"] = (uint32_t)HolgolMessageType::EndQuery;

	int32_t winner = -1;
	if(!forceEnd)
	{
		winner = std::distance(tallies.begin(), std::max_element(tallies.begin(), tallies.end()));

		if(tallies[winner] == 0)
			winner = -1;
	}

	obj["winner"] = winner;

	std::cout << "vote finished\n";

	// Reset all important state back to defaults

	curQuery.timestamp = -1;
	inQuery = false;

	if(forceEnd)
		forceEnd = false;

	// this does make winner a double check,
	// but that's probably a good thing for redundancy:tm:
	BroadcastTallies();
	tallies.clear();

	BroadcastJSON(obj);
}

void HolgolWebsocketServer::CancelVote()
{
	std::cout << "force cancelling vote\n";
	//forceEnd = true;
	voteTimer.cancel();
}