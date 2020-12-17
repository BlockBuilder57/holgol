#include <basewss.h>
#include <set>

using namespace std::placeholders;

BaseWebsocketServer::BaseWebsocketServer()
{
}
BaseWebsocketServer::~BaseWebsocketServer()
{
	//TODO erase all users eventually
}

void BaseWebsocketServer::Start(uint16_t port)
{
	_server.init_asio();

	_server.set_open_handler(std::bind(&BaseWebsocketServer::OnOpen, this, _1));
	_server.set_message_handler(std::bind(&BaseWebsocketServer::OnMessage, this, _1, _2));
	_server.set_close_handler(std::bind(&BaseWebsocketServer::OnClose, this, _1));

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

void BaseWebsocketServer::BroadcastBinary(bytemessage_t &bytes)
{
	if(bytes.empty())
		return;

	websocketpp::lib::error_code ec;

	// range-for to look at all users
	// TODO: this should be locked
	for(auto &hdl : users)
	{
		server::connection_ptr con = _server.get_con_from_hdl(hdl, ec);

		if(ec)
			return;

		// this will invoke the (void* const, size_t, opcode)
		// overload that defaults the opcode to binary
		con->send((void *)bytes.data(), bytes.size() * sizeof(uint8_t));
	}
}
