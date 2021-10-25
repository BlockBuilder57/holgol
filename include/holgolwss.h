#pragma once
#include <basewss.h>
#include <consoleinteraction.h>
#include <boost/asio/steady_timer.hpp>

inline std::string strtostd(boost::json::string &str)
{
	return std::string(str.data(), str.size());
}

enum class HolgolMessageType
{
	Query,
	MakeVote,
	BroadcastTallies,
	EndQuery
};

struct HolgolQuery
{
	uint64_t timestamp;
	std::string query;
	std::vector<std::string> options;
	int64_t maxAnswers;

	boost::asio::ip::address ip;

	static HolgolQuery fromJSONObject(boost::json::object &queryObject)
	{
		HolgolQuery query;

		if (!queryObject["timestamp"].is_null() && !queryObject["query"].is_null() && !queryObject["options"].is_null() && !queryObject["maxAnswers"].is_null())
		{
			if (queryObject["timestamp"].is_int64())
				query.timestamp = queryObject["timestamp"].as_int64();
			if (queryObject["query"].is_string())
				query.query = strtostd(queryObject["query"].as_string());
			if (queryObject["maxAnswers"].is_int64())
				query.maxAnswers = queryObject["maxAnswers"].as_int64();

			if (queryObject["options"].is_array())
			{
				query.options.clear();
				boost::json::array jsonoptions = queryObject["options"].as_array();

				for (int i = 0; i < jsonoptions.size(); i++)
					if (jsonoptions[i].is_string())
						query.options.push_back(strtostd(jsonoptions[i].as_string()));
			}
		}
		else
			query.timestamp = -1;

		return query;
	}

	boost::json::object toJSONObject()
	{
		boost::json::object queryObject;
		queryObject["type"] = (uint32_t)HolgolMessageType::Query;

		queryObject["timestamp"] = timestamp;
		queryObject["query"] = query;
		
		boost::json::array jsonoptions;
		for(int i = 0; i < options.size(); i++)
		{
			jsonoptions.emplace_back(options[i]);
		}
		queryObject["options"] = jsonoptions;

		queryObject["maxAnswers"] = maxAnswers;

		return queryObject;
	}
};

struct HolgolVote
{
	std::vector<uint8_t> choices;

	static HolgolVote fromJSONObject(boost::json::object &voteObject)
	{
		HolgolVote vote;

		if (!voteObject["choices"].is_null() && voteObject["choices"].is_array())
		{
			boost::json::array jsonchoices = voteObject["choices"].as_array();

			for (int i = 0; i < jsonchoices.size(); i++)
				if (jsonchoices[i].is_int64())
					vote.choices.push_back(jsonchoices[i].as_int64());
		}

		return vote;
	}
};

struct HolgolWebsocketServer : public BaseWebsocketServer
{
public:

	HolgolWebsocketServer(boost::asio::io_context& context);

	void CancelVote();

	inline bool InQuery() const { return inQuery; }

	// only valid if InQuery() == true

	inline HolgolQuery& GetQuery() { return curQuery; }

	inline std::vector<uint32_t>& GetVoteTallies() { return tallies; }

private:

	void InvalidDataClose(websocketpp::connection_hdl);

	const static uint64_t VoteTime;

	// active when a vote is in progress,
	// when expired the vote is ended
	boost::asio::steady_timer voteTimer;

	// true if a vote is in progress
	bool inQuery{false};

	bool forceEnd{false};

	HolgolQuery curQuery{};
	std::map<boost::asio::ip::address, HolgolVote> votes{};
	std::vector<uint32_t> tallies{};

	void VoteTimerFinished(const boost::system::error_code& ec);

	boost::json::object MakeTallies();

	// send the current state of answers
	// when in a vote, used both when a client first connects
	// in the middle of a vote and to synchronize
	void SendTallies(websocketpp::connection_hdl handle);
	void BroadcastTallies();

	void OnStart();

	void OnOpen(websocketpp::connection_hdl handle);
	void OnClose(websocketpp::connection_hdl handle);
	void OnMessage(websocketpp::connection_hdl handle, message_ptr message);
};
