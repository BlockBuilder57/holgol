#include <common.h>
#include <consoleinteraction.h>

#include <array>
#include <algorithm>
#include <iomanip>

// The max amount of commands.
constexpr static auto MAX_COMMANDS = 12u;

static std::array<ConsoleCommand*, MAX_COMMANDS> commands;

static int registered_index = 0;

// Internally implemented command to get help on all of the commands
struct HelpCommand : public ConsoleCommand 
{

	void Invoke() override
	{
		std::cout << "Commands:\n";
		
		// kind of annoying to read but this should stop at the end of registered commands
		// without having to incur an additional branch
		// 
		// it's a worthless optimization, but it's one nonethenless
		for(int i = 0; i < registered_index; ++i) 
		{
#ifdef _DEBUG
			if(commands[i] == nullptr)
				break;
#endif
			auto& cmd = *commands[i];
			std::cout << "\t\b\b\b\bCommand " << std::quoted(cmd.GetName()) << " - " << cmd.GetDescription() << '\n';
		}
	}
	
	std::string GetName() override
	{
		return "help";
	}
	
	std::string GetDescription() override
	{
		return "Outputs a list of all commands, along with a description of what they do";
	}
};

static ConsoleCommandRegisterHelper<HelpCommand> _helpreg;

void RegisterConsoleCommand(ConsoleCommand* cmd) 
{
		if(cmd == nullptr)
			return;
		
		if(registered_index > MAX_COMMANDS) 
		{
#ifdef _DEBUG
			std::cout << "DEBUG ERROR: You are trying to register too many commands!!!\n";
			std::cout << "Raise MAX_COMMANDS up a bit if you really need more space.\n";
#endif			
			return;
		}
		
		//std::cout << "attempting register for " << std::quoted(cmd->GetName()) << '\n';
		
		if(auto it = std::find_if(commands.begin(), commands.end(), [&cmd](ConsoleCommand* c) { return c == cmd; }); it != commands.end())
		{
#ifdef _DEBUG
			std::cout << "DEBUG: Attempted to register a duplicate command already registered\n";
#endif			
			return;
		}
		
		commands[registered_index++] = cmd;
}

ConsoleCommand** GetConsoleCommands(int* Size) 
{
		// Give the callee the amount of registered commands if we can
		if(Size != nullptr)
			*Size = registered_index;
			
		return commands.data();
}

void ExecuteConsoleCommand(const std::string& command) 
{
		// TODO: we don't really need to use the public api in this case,
		// it is nice to but eh
#ifdef PUBLICAPI
		int size = 0;
		ConsoleCommand** cmds = GetConsoleCommands(&size);
#endif
		
		//std::cout << "DEBUG: size = " << size << '\n';

		// return early if there's somehow no commands registered
		if(registered_index == 0)
			return;
		
//#if 0
		for(std::size_t i = 0; i < registered_index; ++i) 
		{
			// if the command is valid and matches stored command data,
			// then invoke it. TODO would be to parse arguments and give it to the invokee
			// with a argv-like system
			
			//std::cout << "DEBUG: " << command << ' ' << cmds[i]->GetName() << '\n';
			if(commands[i]->GetName() == command) {
				commands[i]->Invoke();
				return;
			}
		}
//#endif

//		if (auto it = std::find_if(commands.begin(), commands.begin() + registered_index, [command](ConsoleCommand* cmd) { return cmd->GetName() == command; }); it != commands.end())
//			(*it)->Invoke();
//		else
			std::cout << "Command " << std::quoted(command) << " does not exist\n";
}