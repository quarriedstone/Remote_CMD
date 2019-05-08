#include "pch.h"
#include <iostream>
#include "Client.h"
#include "SVC.h"
#include "ConsoleHandler.h"


int main(int argc, char** argv) {

	if (argc < 1) {
		std::cout << "Not enough arguments";
		return 0;
	}
	if (0 == strcmp(argv[1], "--service"))
	{
		_ttmain(0, NULL);

	}

	if (strcmp(argv[1], "--client") == 0) {
		std::cout << "Client is starting\n";
		Client client = Client();
		ConsoleHandler console_handler = ConsoleHandler();
		Package read_pack, write_pack;

		client.client_initialize(argc - 1, argv + 1);
		client.client_connect();


		while (1) {
			// Reading from console

			read_pack = console_handler.readConsole();
			client.client_data_send(read_pack);

			write_pack = client.client_recv();
			console_handler.writeConsole(write_pack);
		}
		
		client.client_close_connection();
	}
	else if (strcmp(argv[1], "--server") == 0) {
		std::cout << "Server is starting\n";
		Server server = Server();

		server.set_up_socket();
		server.listen_socket();
		server.accept_client_socket();

		server.recv_data();

		server.close_connection();

	}

	return 0;
}