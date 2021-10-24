#include <common.h>
#include <holgolwss.h>
#include <iomanip>

boost::asio::io_context global_context;

int main()
{
	std::cout << "Hello C++17 World\n";

	try {
		HolgolWebsocketServer holgol(global_context);
		holgol.Start(5701);

		global_context.run();
	} catch(websocketpp::exception& ex) {
		std::cerr << "Got Websocket++ exception " << std::quoted(ex.what()) << ", exiting\n";
		return 1;
	}
	
	return 0;
}
