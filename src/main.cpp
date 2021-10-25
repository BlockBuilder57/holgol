#include <common.h>
#include <holgolwss.h>

#include <iomanip>

static boost::asio::io_context g_context;
HolgolWebsocketServer g_holgol(g_context);

// should be configurable?
std::uint16_t g_port = 5701;

struct ExitCommand : public ConsoleCommand {

		void Invoke() override
		{
			// or have a cleanup function..
			if(g_holgol.InQuery())
				g_holgol.CancelVote(); // should really be a brute-force removal of in-query status, but this works
			
			// stop the global ASIO context, which will cause the server to stop
			g_context.stop();
		}
	
		std::string GetName() override
		{
			return "exit";
		}
		
		std::string GetDescription() override
		{
			return "Exits the server cleanly.";
		}
};

static ConsoleCommandRegisterHelper<ExitCommand> exit_;


// Background thread function to read a line off the console and dispatch
// it to the console interaction code
void ConsoleThread() {
	
	// Loop forever, attempting to execute commands from input
	while(true)
	{
		std::cout << "Holgol> ";
		
		std::string line;
		std::getline(std::cin, line);
		
		if(line == "")
			continue;
		
		ExecuteConsoleCommand(line);
	}
	
}

int main()
{
	std::cout << "Hello C++17 World\n";

	try 
	{
		// Start and detach the console thread.
		std::thread thread(ConsoleThread);
		thread.detach();
		
		g_holgol.Start(g_port);

		// run completion handlers on the main thread
		// (we don't need to implement locking in this case)
		g_context.run();
	} catch(std::exception& ex) 
	{
		std::cerr << "Exception thrown: " << std::quoted(ex.what()) << ", exiting\n";
		return 1;
	}
	
	return 0;
}
