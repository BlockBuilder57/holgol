#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <vector>
#include <set> 
#include <cstdint>

struct BaseWebsocketServer
{
	typedef websocketpp::server<websocketpp::config::asio> server;
	typedef server::message_ptr message_ptr;
	typedef std::vector<uint8_t> bytemessage_t;
	
	BaseWebsocketServer();
	~BaseWebsocketServer();
	
	void Start(uint16_t port);
	
	virtual void OnStart() = 0;
	virtual void OnOpen(websocketpp::connection_hdl handle);
	virtual void OnClose(websocketpp::connection_hdl handle);
	virtual void OnMessage(websocketpp::connection_hdl handle, message_ptr message) = 0;
	
	// broadcast to all users
	void BroadcastBinary(bytemessage_t& bytes);
	
protected: // inheriters would want this

	std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>> users;
	
private:
	server _server;
};
