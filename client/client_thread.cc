
#include "ClientSocket.h"
#include "SocketException.h"
#include <string>
#include <iostream>
#include <fstream>
#include <thread>

using namespace std;

void startNewClient(int interval, std::string command) {
	std::thread([interval, &command]()
	{
		while (true) {
			try {
				// Create the Socket
				ClientSocket c_socket("localhost", 30000);
				std::string reply;

				std::ofstream out;
				//append
				out.open("client.log", std::ios_base::app);

				cout << "Client command: " << command << endl;
				if(command != "") {
					try {
						// send command
						out << "[Sending]\t" + command + "\n";
						std::cout << "[Sending]\t" + command + "\n";
						c_socket << command;
						// receive reply from server
						c_socket >> reply;
					} catch (SocketException&) {}
					out << "[Response]\n" << reply << "\n";
					out.flush();
					std::cout << "[Response]\t" << endl << reply << "\n";
				}
			} catch(SocketException& e) {
				std::cout << "Exception caught: " << e.description() << std::endl;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(interval));
		}
	}).detach();
}

int main(int argc, char* argv[]) {
	string command = "UPDATE|1|8888"; // update o_orderkey = 8888
	startNewClient(5000, command);
	string command2 = "SCAN|8888|o_orderkey|="; // scan with o_orderkey = 8888
	startNewClient(5000, command2);

	// loop forever
	while(true);

    return 0;
}
