#pragma once 
#include <basewss.h>

struct HolgolWebsocketServer : public BaseWebsocketServer
{
	void OnStart();

	void OnOpen(websocketpp::connection_hdl handle);
	void OnClose(websocketpp::connection_hdl handle);
	void OnMessage(websocketpp::connection_hdl handle, message_ptr message);
};
