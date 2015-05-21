#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

using namespace std;
const size_t BUFFER_SIZE = 1024;

void die(const char *message);

int main(int argc, char* argv[]) {
	if (argc < 3) {
		cerr << "Error: hostname port required" << endl;
		cout << "Usage: ./client <hostname> <port>" << endl;
		exit(EXIT_FAILURE);
	}

	int client_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (client_fd < 0) {
		die("Error opening socket");
	}

	struct hostent* server = gethostbyname(argv[1]);
	if (server == NULL) {
		cerr << "Can't resolve server: " << argv[1] << endl;
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
	server_addr.sin_port = htons(atoi(argv[2]));

	int result = connect(client_fd, (struct sockaddr *) &server_addr, sizeof(server_addr));
	if (result < 0) {
		die("Error connecting server");
	}

	string message;
	char* buffer = new char[BUFFER_SIZE];

	while (true) {
		cout << "Please enter the message: ";
		cin >> message;

		result = send(client_fd, message.c_str(), message.length(), 0);
		if (result < 0) {
			die("Error on send()");
		}

		memset(buffer, 0, BUFFER_SIZE);
		result = recv(client_fd, buffer, BUFFER_SIZE, 0);
		if (result < 0) {
			die("Error on recv()");
		}

		cout << "Client: " << buffer << endl;
	}

	close(client_fd);
	return EXIT_SUCCESS;
}

void die(const char *message) {
	perror(message);
	exit(EXIT_FAILURE);
}
