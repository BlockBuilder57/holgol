#include <basewss.h>
#include <set>

using namespace std::placeholders;

BaseWebsocketServer::BaseWebsocketServer(boost::asio::io_context &context)
	: context(context)
{
}
BaseWebsocketServer::~BaseWebsocketServer()
{
	//TODO erase all users eventually
}

boost::asio::ip::address BaseWebsocketServer::GetIPAddress(websocketpp::connection_hdl handle) 
{
	websocketpp::lib::error_code ec;
	BaseWebsocketServer::server::connection_ptr con = _server.get_con_from_hdl(handle, ec);

	if(ec)
		std::cout << "somehow failed at getting an IP address: too bad!";

	return con->get_raw_socket().remote_endpoint().address();
}

void BaseWebsocketServer::Start(uint16_t port)
{
	// get rid of most of the connection stuffs
	_server.clear_access_channels(websocketpp::log::alevel::all);
	_server.clear_error_channels(websocketpp::log::elevel::all);

	_server.init_asio(&context);

	_server.set_open_handler(std::bind(&BaseWebsocketServer::OnOpen, this, _1));
	_server.set_message_handler(std::bind(&BaseWebsocketServer::OnMessage, this, _1, _2));
	_server.set_close_handler(std::bind(&BaseWebsocketServer::OnClose, this, _1));
	
	// this avoids "Underlying Transport Error" if someone is trying to connect to the server
	// when the server is starting.
	_server.set_reuse_addr(true);

	_server.listen(port);
	_server.start_accept();

	// eventually multithread this
	OnStart();

	// asio event loop
	_server.run();
}

void BaseWebsocketServer::OnOpen(websocketpp::connection_hdl handle)
{
	users.insert(handle);
}

void BaseWebsocketServer::OnClose(websocketpp::connection_hdl handle)
{
	users.erase(handle);
}

void BaseWebsocketServer::OnMessage(websocketpp::connection_hdl handle, message_ptr message)
{
}

void BaseWebsocketServer::Send(websocketpp::connection_hdl handle, std::string message)
{
	websocketpp::lib::error_code ec;
	server::connection_ptr con = _server.get_con_from_hdl(handle, ec);

	if(ec)
		return;

	con->send(message);
}

void BaseWebsocketServer::SendJSON(websocketpp::connection_hdl handle, boost::json::object obj)
{
	Send(handle, boost::json::serialize(obj));
}

void BaseWebsocketServer::Broadcast(std::string message)
{
	if(message.empty())
		return;

	// range-for to look at all users
	// TODO: this should be locked
	for(auto &hdl : users)
	{
		Send(hdl, message);
	}
}

void BaseWebsocketServer::BroadcastJSON(boost::json::object obj)
{
	Broadcast(boost::json::serialize(obj));
}