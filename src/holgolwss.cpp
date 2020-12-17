#include <holgolwss.h>
#include <nlohmann/json.hpp>

#include <ctime>
#include <vector>
#include <set>
#include <cstdint>

using json = nlohmann::json;

struct HolgolQuery
{
	uint64_t timestamp;
	std::string query;
	std::vector<std::string> options;
	uint16_t maxAnswers;

	static HolgolQuery create(json &queryObject)
	{
		HolgolQuery query;

		if(!queryObject["timestamp"].is_null() && !queryObject["query"].is_null() && !queryObject["options"].is_null() && !queryObject["maxAnswers"].is_null())
		{
			query.timestamp = queryObject["timestamp"].get<uint64_t>();
			query.query = queryObject["query"].get<std::string>();
			query.options.clear();
			query.maxAnswers = queryObject["maxAnswers"].get<uint64_t>();

			for(int i = 0; i < queryObject["options"].size(); i++)
			{
				query.options.push_back(queryObject["options"][i].get<std::string>());
			}
		}

		return query;
	}
};

struct HolgolAnswer
{
	uint16_t answer;
};

void HolgolWebsocketServer::OnStart()
{
	//main thread
}

void HolgolWebsocketServer::OnOpen(websocketpp::connection_hdl handle)
{
	BaseWebsocketServer::OnOpen(handle);
	std::cout << "user connected\n";
}

void HolgolWebsocketServer::OnClose(websocketpp::connection_hdl handle)
{
	BaseWebsocketServer::OnClose(handle);
	std::cout << "user left\n";
}

void HolgolWebsocketServer::OnMessage(websocketpp::connection_hdl handle, message_ptr message)
{
	std::cout << "user sent a message\n";

	// if it's not binary throw it out
	if(message->get_opcode() != websocketpp::frame::opcode::text)
		return;

	json payload = json::parse(message->get_payload());
	HolgolQuery query;
	HolgolAnswer answer;

	if(!payload["type"].is_null())
	{
		switch(payload["type"].get<uint16_t>())
		{
		case 0:
			query = HolgolQuery::create(payload);

			if(query.timestamp != -1)
			{
				printf("\twhat's timestamp %d\n", query.timestamp);
				printf("\twhat's query %s\n", query.query.c_str());
				for (int i = 0; i < query.options.size(); i++)
				{
					printf("\twhat's options[%d] %s\n", i, query.options[i].c_str());
				}
				printf("\twhat's maxAnswers %d\n", query.maxAnswers);
			}
			break;
		case 1:
			break;
		default:
			std::cout << "unhandled type\n";
		}
	}
	else
		std::cout << "someone is being naughty\n";
}