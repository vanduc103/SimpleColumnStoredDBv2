
#include "ClientSocket.h"
#include "SocketException.h"
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>

using namespace std;

int main(int argc, char* argv[]) {
    try {
        // Create the Socket
        ClientSocket c_socket("localhost", 30000);
        std::string command;
        std::string reply;

        //std::ifstream in("orders_sample.tbl");
        std::ofstream out("client.log");

        while(true) {
        	cout << "Client command: ";
            getline(cin, command);
            if(command != "") {
                try {
                    //std::string message = "INSERT|" + line;
                    out << "[Sending]\t" + command + "\n";
                    std::cout << "[Sending]\t" + command + "\n";
                    c_socket << command;
                    // receive reply from server
                    c_socket >> reply;
                } catch (SocketException&) {}
                out << "[Response]\t" << reply << "\n";
                std::cout << "[Response]\t" << endl << reply << "\n";
            }
        }
    } catch(SocketException& e) {
        std::cout << "Exception caught: " << e.description() << std::endl;
    }

    return 0;
}
