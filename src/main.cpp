#include <common.h>
#include <holgolwss.h>

boost::asio::io_context global_context;

int main()
{
	std::cout << "Hello C++17 World\n";

	HolgolWebsocketServer holgol(global_context);
	holgol.Start(5701);

	global_context.run();
}
