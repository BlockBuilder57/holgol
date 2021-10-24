#include <holgolwss.h>

#include <ctime>
#include <vector>
#include <set>
#include <cstdint>

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

	std::cout << "user connected with ip" << GetIPAddress(handle).to_string() << " \n";

	if(inQuery)
	{
		SendJSON(handle, curQuery.toJSONObject());
		SendTallies(handle);

		std::cout << "in a query, sent them the current query and votes\n";
	}
}

void HolgolWebsocketServer::OnClose(websocketpp::connection_hdl handle)
{
	BaseWebsocketServer::OnClose(handle);

	std::cout << "user left with ip" << GetIPAddress(handle).to_string() << " \n";

	if(inQuery)
	{
		boost::asio::ip::address handleaddr = GetIPAddress(handle);

		if(votes.find(handleaddr) != votes.end())
		{
			for(auto choice : votes[handleaddr].choices)
				if(choice < curQuery.options.size() && choice >= 0)
					tallies[choice]--;

			votes.erase(handleaddr);
			BroadcastTallies();
		}

		std::cout << "user had votes, subtracting\n";
	}
}

void HolgolWebsocketServer::OnMessage(websocketpp::connection_hdl handle, message_ptr message)
{
	std::cout << "user sent a message\n";

	// if it's not binary throw it out
	if(message->get_opcode() != websocketpp::frame::opcode::text)
		return;

	boost::system::error_code ec;

	boost::json::value payload_value = boost::json::parse(message->get_payload(), ec);

	if(ec)
	{
		std::cout << "someone is being naughty: " << ec.message() << "\n";
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
					std::cout << "rejecting query for too many or too few options\n";
					break;
				}

				// check timestamp was created in last second
				if(auto n = abs(difftime(query.timestamp, std::time(nullptr))); n > 2)
				{
					std::cout << "rejecting query for out of date timestamp\n";
					printf("abs(difftime(%ld, %ld): %d", query.timestamp, std::time(nullptr), n);
					break;
				}

				if(query.maxAnswers <= 0 || query.maxAnswers > query.options.size())
				{
					std::cout << "rejecting query for oob maxAnswers size\n";
					break;
				}

				printf("new query:\n");
				printf("\twhat's timestamp %ld\n", query.timestamp);
				printf("\twhat's query %s\n", query.query.c_str());
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
			std::cout << "unhandled/invalid type, someone is being very naughty\n";
		}
	}
	else
		std::cout << "someone is being naughty\n";
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
	std::cout << "vote naturally finished\n";
	curQuery.timestamp = -1;
	inQuery = false;

	int32_t winner = std::distance(tallies.begin(), std::max_element(tallies.begin(), tallies.end()));

	if(tallies[winner] == 0)
		winner = -1;

	boost::json::object obj;
	obj["type"] = (uint32_t)HolgolMessageType::EndQuery;
	obj["winner"] = winner;

	// this does make winner a double check,
	// but that's probably a good thing for redundancy:tm:
	BroadcastTallies();
	tallies.clear();

	BroadcastJSON(obj);
}
void HolgolWebsocketServer::CancelVote()
{
	std::cout << "force cancelling vote\n";
	boost::system::error_code ec;
	VoteTimerFinished(ec);

	if(ec)
		std::cout << "somehow errored when cancelling the vote\n";
}