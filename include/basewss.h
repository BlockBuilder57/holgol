#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <boost/json.hpp>
#include <common.h>

#include <vector>
#include <set>

struct BaseWebsocketServer
{
	typedef websocketpp::server<websocketpp::config::asio> server;
	typedef server::message_ptr message_ptr;
	
	BaseWebsocketServer(boost::asio::io_context& context);
	~BaseWebsocketServer();

	boost::asio::ip::address GetIPAddress(websocketpp::connection_hdl handle);
	
	void Start(uint16_t port);
	
	virtual void OnStart() = 0;
	virtual void OnOpen(websocketpp::connection_hdl handle);
	virtual void OnClose(websocketpp::connection_hdl handle);
	virtual void OnMessage(websocketpp::connection_hdl handle, message_ptr message) = 0;
	
	// send to specific user
	void Send(websocketpp::connection_hdl handle, const std::string& message);
	void SendJSON(websocketpp::connection_hdl handle, const boost::json::object& obj);

	// broadcast to all users
	void Broadcast(const std::string& message);
	void BroadcastJSON(const boost::json::object& obj);
	
protected: // inheriters would want this

	boost::asio::io_context& context;
	std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>> users;
	
//private:
	server _server;
};
