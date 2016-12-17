
#include "ClientSocket.h"
#include "SocketException.h"
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <thread>

using namespace std;

void startNewClient(int interval, vector<string>& vecCommand, std::ofstream& out) {
	std::thread([interval, &vecCommand, &out]()
	{
		while (true) {
			try {
				// Create the Socket
				ClientSocket c_socket("localhost", 30000);
				std::string reply;
				for (string command : vecCommand) {
					cout << "Client command: " << command << endl;
					if(command != "") {
						try {
							// send command
							out << "\n[Sending]\t" + command + "\n";
							std::cout << "[Sending]\t" + command + "\n";
							c_socket << command;
							// receive reply from server
							c_socket >> reply;
						} catch (SocketException&) {}
						out << "[Response]\n" << reply << "\n";
						out.flush();
						std::cout << "[Response]\t" << endl << reply << "\n";
					}
				}
			} catch(SocketException& e) {
				std::cout << "Exception caught: " << e.description() << std::endl;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(interval));
		}
	}).detach();
}

int main(int argc, char* argv[]) {
	std::ofstream out("client.log");
	vector<string> vecCommand;
	// insert test
	/*vecCommand.push_back("SCAN|9999|o_orderkey|=");
	vecCommand.push_back("INSERT|9999|D|56789|Le Van Duc");
	vecCommand.push_back("SCAN|9999|o_orderkey|=");
	startNewClient(30000, vecCommand, out);*/
	// update test
	vecCommand.clear();
	vecCommand.push_back("SCAN|1383879|o_orderkey|=");
	vecCommand.push_back("UPDATE|1|1383879|D|56789|Le Van Duc");
	vecCommand.push_back("SCAN|1383879|o_orderkey|=");
	startNewClient(30000, vecCommand, out);
	// garbage collection test
	/*vecCommand.clear();
	vecCommand.push_back("UPDATE|2|8888");
	vecCommand.push_back("SCAN|8888|o_orderkey|=");
	vecCommand.push_back("SCAN|8888|o_orderkey|=");
	startNewClient(5000, vecCommand, out);*/
	// write conflict test
	/*vecCommand.clear();
	vecCommand.push_back("UPDATE|3|1111");
	startNewClient(60000, vecCommand, out);
	vector<string> vecCommand1;
	vecCommand1.push_back("UPDATE|3|2222");
	startNewClient(60000, vecCommand1, out);*/
	// scan
	/*vecCommand.clear();
	vecCommand.push_back("UPDATE|4|3333");
	vecCommand.push_back("UPDATE|5|3334");
	vecCommand.push_back("UPDATE|42|3335");
	vecCommand.push_back("UPDATE|76|3336");
	vecCommand.push_back("SCAN|56789|o_totalprice|>");
	vecCommand.push_back("SCAN|5678|o_totalprice|>|56789|o_totalprice|<");
	startNewClient(60000, vecCommand, out);*/

	// loop forever
	while(true);

    return 0;
}
