#include <iostream>
#include <string>
#include <thread>

#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <Windows.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define Sleep(x) usleep((x)*1000)
#endif

#ifdef _WIN32
#define CROSS_PLATFORM_SOCKET_ERROR SOCKET_ERROR
#else
#define CROSS_PLATFORM_SOCKET_ERROR -1
#endif

#define PORT 12345

SOCKET clientSocket;
struct sockaddr_in senderAddress;
struct sockaddr_in receiverAddress;
struct sockaddr_in serverAddress;

char buffer[1024];

std::string clientName;
std::string receiver;
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
#ifdef _WIN32
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << "Failed to initialize winsock" << std::endl;
		return false;
	}
#endif

	clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (clientSocket == INVALID_SOCKET) {
		std::cerr << "Failed to create socket" << std::endl;
#ifdef _WIN32
		WSACleanup();
#endif
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

struct sockaddr_in getAddress(std::string IP, int port) {
	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(IP.c_str());
	address.sin_port = htons(port);
	return address;
}

bool messageTo(std::string text, std::string name = "server") {
	struct sockaddr_in address;
	address = (name == "server") ? serverAddress : receiverAddress;

	int result = sendto(clientSocket, text.c_str(), text.size(), 0, (struct sockaddr*)&address, sizeof(address));
	if (result == CROSS_PLATFORM_SOCKET_ERROR) {
#ifdef _WIN32
		int error = WSAGetLastError();
		std::cerr << "Failed to send message. Error code: " << error << std::endl;
#else
		perror("Failed to send message");
#endif
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

#ifdef _WIN32
	int bytesReceived = recvfrom(clientSocket, buffer, sizeof(buffer), 0, (struct sockaddr*)&senderAddress, &senderAddressSize);
#else
	ssize_t bytesReceived = recvfrom(clientSocket, buffer, sizeof(buffer), 0, (struct sockaddr*)&senderAddress, (socklen_t*)&senderAddressSize);
#endif

	if (bytesReceived == CROSS_PLATFORM_SOCKET_ERROR) {
#ifdef _WIN32
		int error = WSAGetLastError();
		std::cerr << "Failed to receive message. Error code: " << error << std::endl;
#else
		perror("Failed to receive message");
#endif
	}
}


std::string extractIPAddress(const std::string& input) {
	std::size_t delimiterPos = input.find(':');
	if (delimiterPos != std::string::npos) {
		return input.substr(0, delimiterPos);
	}
	return "";
}

int extractPort(const std::string& input) {
	std::size_t delimiterPos = input.find(':');
	if (delimiterPos != std::string::npos) {
		std::string portStr = input.substr(delimiterPos + 1);
		return std::stoi(portStr);
	}
	return 0;
}

void handleMessage() {
	command = buffer[0];
	text = "";
	text.append(&buffer[2]);
	switch (command) {
	case 'E':
		receiverAddress = getAddress(extractIPAddress(text), extractPort(text));
		break;
	case 'N':
		std::cout << "No such client\n\n";
		break;
	case 'M':
		std::cout << std::endl;
		std::cout << std::endl;
		std::cout << "----  " << text << std::endl;
		std::cout << std::endl;
		menuPrompt();
		break;
	case 'L':
		std::cout << "\nNow there are in the chat: " << std::endl;
		for (auto c : text) {
			if (c == ':')
				std::cout << std::endl;
			else
				std::cout << c;
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

#ifdef _WIN32
	WSACleanup();
#endif

	return 0;
}
