#pragma once
#include <common.h>

/**
 * Base class for console commands to follow.
 */
struct ConsoleCommand 
{
	
	/**
	 * Executes this command.
	 */
	virtual void Invoke() = 0;
	
	/**
	 * Get the name of this command.
	 */
	virtual std::string GetName() = 0;
	
	/**
	 * Get the description of this command, used for help.
	 */
	virtual std::string GetDescription() = 0;
	
};

/**
 * Execute a console command, as if it was typed in.
 *
 * \param[in] command The command string to run. 
 */
void RegisterConsoleCommand(ConsoleCommand* cmd);

/**
 * Gets a list of all console commands.
 *
 * \param[out] Size The amount of registered console commands.
 */
ConsoleCommand** GetConsoleCommands(int* Size);

/**
 * Execute a console command, as if it was typed in.
 *
 * \param[in] command The command string to run. 
 */
void ExecuteConsoleCommand(const std::string& command);

/**
 * Helper class to automatically manage a static instance of
 * a console command type and register it with the console interaction system.
 */
template<class TConsoleCommand>
struct ConsoleCommandRegisterHelper 
{
	
	ConsoleCommandRegisterHelper() 
	{
		static TConsoleCommand cmd;
		
		// Register the command.
		RegisterConsoleCommand((ConsoleCommand*)&cmd);
	}
	
};