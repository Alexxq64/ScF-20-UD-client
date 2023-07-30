#include <iostream>
#include <string>
#include <thread>

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <Windows.h>
#pragma comment(lib, "ws2_32.lib")

#define CROSS_PLATFORM_SOCKET_ERROR SOCKET_ERROR

#define PORT 12345

SOCKET clientSocket;
struct sockaddr_in senderAddress;
struct sockaddr_in receiverAddress;
struct sockaddr_in serverAddress;

char buffer[1024];

std::string clientName;
std::string receiver;
std::string text;
std::string password;
char command;
bool toExit = false;

void menuPrompt() {
	std::cout << "\n";
	std::cout << "[0] Exit  ";
	std::cout << "[1] Send message  ";
	std::cout << "[2] Get clients list  ";
	std::cout << "[3] Get message history  \n";
	std::cout << "Enter your choice: ";
}

bool createClientSocket() {
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

struct sockaddr_in getAddress(std::string IP, int port) {
	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(IP.c_str());
	address.sin_port = htons(port);
	return address;
}

bool sendMessage(std::string text, std::string name = "server") {
	struct sockaddr_in address;
	address = (name == "server") ? serverAddress : receiverAddress;

	int result = sendto(clientSocket, text.c_str(), text.size(), 0, (struct sockaddr*)&address, sizeof(address));
	if (result == CROSS_PLATFORM_SOCKET_ERROR) {
		int error = WSAGetLastError();
		std::cerr << "Failed to send message. Error code: " << error << std::endl;
		return false;
	}

	return true;
}

void getMessage() {
	int senderAddressSize;
	memset(buffer, 0, sizeof(buffer));
	senderAddressSize = sizeof(senderAddress);

	int bytesReceived = recvfrom(clientSocket, buffer, sizeof(buffer), 0, (struct sockaddr*)&senderAddress, &senderAddressSize);

	if (bytesReceived == CROSS_PLATFORM_SOCKET_ERROR) {
		int error = WSAGetLastError();
		std::cerr << "Failed to receive message. Error code: " << error << std::endl;
	}
}

void registerClient() {
	bool wrongPW = true;
	text = "R:" + clientName;
	sendMessage(text);
	Sleep(500);
	getMessage();
	switch (buffer[0]) {
	case 'C':
		std::cout << "To sign in enter your password: \n";
		while (wrongPW)	{
			std::getline(std::cin, password);
			sendMessage("V:" + password);
			Sleep(500);
			getMessage();
			wrongPW = buffer[0] == 'B';
			if (wrongPW)
				std::cout << "Wrong password. Try again: \n";
		}
		break;
	case 'P':
		std::cout << "Enter the password: \n";
		std::getline(std::cin, password);
		sendMessage("S:" + password);
		break;
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
	case 'A':
		std::cout << "The client is not in the chat now\n\n";
		break;
	case 'N':
		std::cout << "No such client\n\n";
		break;
	case 'M':
		std::cout << std::endl;
		std::cout << std::endl;
		std::cout << "  " << text << std::endl;
		std::cout << std::endl;
		menuPrompt();
		break;
	case 'L':
		std::cout << text;
		std::cout << std::endl;
		menuPrompt();
		break;
	case 'H':
		std::cout << text;
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
	// to do: signed_in = 0
}

// check if the client is in the chat now or not and if it is even in the database at all 
bool checkName(std::string name) {
	text = "C:" + name;
	sendMessage(text);
	return true;
}

void sendMessage() {
	std::cout << "Enter receiver name: ";
	std::cin >> receiver;
	checkName(receiver);
	Sleep(200);
	// if the client is in the chat now
	if (command == 'E') {
		std::cout << "Enter message: ";
		std::cin.ignore();
		text = "";
		std::getline(std::cin, text);
		text = "M:" + clientName + ">>  " + text;
		// send message to the receiver
		sendMessage(text, "receiver");
		// send message to the server to save it
		sendMessage(text);
	}
	std::cout << std::endl;
	menuPrompt();
}

// get list of the clients in the chat
void getPresentClientsList() {
	sendMessage("L:");
}

void getMessageHistory() {
	sendMessage("H:");
}

void receiveMessages() {
	while (!toExit) {
		getMessage();
		handleMessage();
	}
}

void console() {
	char choice;
	while (!toExit) {
		std::cin >> choice;
		switch (choice) {
		case '0':
			closeConsole();
			return;
		case '1':
			sendMessage();
			break;
		case '2':
			getPresentClientsList();
			break;
		case '3':
			getMessageHistory();
			break;
		default:
			std::cout << "Invalid menu choice. Please try again." << std::endl;
			break;
		}
	}
}

int main() {
	if (!createClientSocket())
		return -1;

	registerClient();
	getPresentClientsList();
	SetConsoleOutputCP(CP_UTF8);

	std::thread receiveThread(receiveMessages);
	std::thread consoleThread(console);

	receiveThread.join();
	consoleThread.join();

	WSACleanup();

	return 0;
}
