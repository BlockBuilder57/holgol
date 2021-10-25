#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <boost/json.hpp>
#include <common.h>

#include <vector>
#include <set>

struct BaseWebsocketServer
{

	struct WebsocketConfig : public websocketpp::config::asio {
		using core = websocketpp::config::asio;

		typedef core::concurrency_type concurrency_type;
		typedef core::request_type request_type;
		typedef core::response_type response_type;
		typedef core::message_type message_type;
		typedef core::con_msg_manager_type con_msg_manager_type;
		typedef core::endpoint_msg_manager_type endpoint_msg_manager_type;
		typedef core::alog_type alog_type;
		typedef core::elog_type elog_type;
		typedef core::rng_type rng_type;
		typedef core::transport_type transport_type;
		typedef core::endpoint_base endpoint_base;

		struct connection_base {

			/**
			 * Set to true if this connection is proxied.
			 */
			bool under_proxy{false};

			/**
			 * Only valid if under_proxy is true.
			 */
			boost::asio::ip::address proxy_realip;
		};
	};

	typedef websocketpp::server<WebsocketConfig> server;
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
