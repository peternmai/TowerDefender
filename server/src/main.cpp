#include <iostream>

#include "GameServer.hpp"

int main() {

    int portNumber = 0;
    std::cout << "Please select server port number [1024-65535]: ";
    std::cin >> portNumber;
    std::cin.ignore();
    
    // Check if port number given is valid
    if ((portNumber < 1024) || (portNumber > 65535)) {
        std::cerr << "Invalid port number provided. Valid range: 1024-65535" << std::endl;
        return -1;
    }

    // Start the game server
    GameServer gameServer(portNumber);

    // Wait for user to decide to stop the program
    std::cout << "\tPress [ENTER] anytime to stop the server" << std::endl;
    std::cin.ignore();
    gameServer.stop();

    return 0;
}