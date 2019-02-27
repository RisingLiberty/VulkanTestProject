#include <stdexcept>
#include <iostream>

#include "HelloTriangleApplication.h"

int Program()
{
	HelloTriangleApplication app;

	try
	{
		app.Run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int main()
{
	int errCode = Program();

	std::cout << "exited with error code: " << errCode;
	std::cin.get();

	return errCode;
}