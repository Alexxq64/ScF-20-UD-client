#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <string>
#include <thread>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 12345

SOCKET clientSocket;
SOCKADDR_IN senderAddress;
SOCKADDR_IN receiverAddress;
SOCKADDR_IN serverAddress;

char buffer[1024];

std::string clientName;
std::string receiver;
//std::string sender;
std::string text;
char command;


void menuPrompt() {
	std::cout << "\n";
	std::cout << "[0] Exit  ";
	std::cout << "[1] Send message  ";
	std::cout << "[2] Get clients list\n";
	std::cout << "Enter your choice: ";
}


bool createClient() {
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << "Failed to initialize winsock" << std::endl;
		return false;
	}
	clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (clientSocket == INVALID_SOCKET) {
		std::cerr << "Failed to create socket" << std::endl;
		WSACleanup();
		return false;
	}
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
	serverAddress.sin_port = htons(PORT);
	std::cout << "Enter your name: ";
	std::getline(std::cin, clientName);
	std::cout << std::endl;
	return true;
}

SOCKADDR_IN getAddress(std::string IP, int port) {
	SOCKADDR_IN address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(IP.c_str());
	address.sin_port = htons(port);
	return address;
}


bool messageTo(std::string text, std::string name = "server") {
	SOCKADDR_IN address;
	address = (name == "server") ? serverAddress : receiverAddress;
	//address = (name == "server") ? serverAddress : getAddressByName(name);
	if (sendto(clientSocket, text.c_str(), text.size(), 0, (SOCKADDR*)&address, sizeof(address)) == SOCKET_ERROR) {
		return false;
	}
	return true;
}

void registerClient() {
	text = "R:" + clientName;
	messageTo(text);
}


void getMessage() {
	int senderAddressSize;
	memset(buffer, 0, sizeof(buffer));
	senderAddressSize = sizeof(senderAddress);
	int bytesReceived = recvfrom(clientSocket, buffer, sizeof(buffer), 0, (SOCKADDR*)&senderAddress, &senderAddressSize);
	if (bytesReceived == SOCKET_ERROR) {
		std::cerr << "Failed to receive message" << std::endl;
	}
}

std::string extractIPAddress(const std::string& input) {
	std::size_t delimiterPos = input.find(':');
	if (delimiterPos != std::string::npos) {
		return input.substr(0, delimiterPos);
	}
}

int extractPort(const std::string& input) {
	std::size_t delimiterPos = input.find(':');
	if (delimiterPos != std::string::npos) {
		std::string portStr = input.substr(delimiterPos + 1);
		return std::stoi(portStr);
	}
}

void handleMessage() {
	//std::cout << buffer << std::endl;
	command = buffer[0];
	text = "";
	text.append(&buffer[2]);
	switch (command){
	case 'E':
		receiverAddress = getAddress(extractIPAddress(text), extractPort(text));
		break;
	case 'N':
		std::cout << "No such client\n\n";
		break;
	case 'M':
		std::cout << std::endl;
		std::cout << std::endl;
		//std::cout << "\x1b[32m" << "----  " << text << std::endl;
		std::cout << "----  " << text << std::endl;
		std::cout << std::endl;
		menuPrompt();
		break;
	case 'L':
		std::cout << "\nNow there are in the chat: " << std::endl;
		for (auto c:text){
			if (c == ':') std::cout << std::endl;
			else std::cout << c;
		}
		std::cout << std::endl;
		menuPrompt();
		break;
	default:
		std::cout << std::endl;
		menuPrompt();
		break;
	}
}

void closeConsole() {
	std::cout << "Exiting client..." << std::endl;
}

bool checkName(std::string name) {
	text = "C:" + name;
	messageTo(text);
	return true;
}

void sendMessage() {
	std::cout << "Enter receiver name: ";
	std::cin >> receiver;
	checkName(receiver);
	Sleep(200);
	if (command == 'E') {
		std::cout << "Enter message: ";
		std::cin.ignore();
		text = "";
		std::getline(std::cin, text);
		//text = "M:" + clientName + "  --->  " + text + "\x1b[0m";
		text = "M:" + clientName + "  --->  " + text;
		messageTo(text, "receiver");
	}
	std::cout << std::endl;
	menuPrompt();
}

void getClients() {
	messageTo("L:");
}

void receiveMessages() {
	while (true) {
		getMessage();
		handleMessage();
	}
}

void console() {
	char choice;
	while (true) {
		std::cin >> choice;
		switch (choice) {
		case '0':
			closeConsole();
			return;
		case '1':
			sendMessage();
			break;
		case '2':
			getClients();
			break;
		default:
			std::cout << "Invalid menu choice. Please try again." << std::endl;
			break;
		}
	}
}


int main() {
	if (!createClient())
		return -1;
	registerClient();
	getClients();


	std::thread receiveThread(receiveMessages);
	std::thread consoleThread(console);

	receiveThread.join();
	consoleThread.join();

	return 0;
}
