#include <common.h>
#include <holgolwss.h>

int main()
{
	std::cout << "Hello C++17 World\n";

	HolgolWebsocketServer holgol;
	holgol.Start(5701);
}
