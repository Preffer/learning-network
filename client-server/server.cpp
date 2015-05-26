/* socket-server
 *
 * Simple poll socket server implementation based on poll()
 * 2015 Yuzo(Huang Yuzhong)
 *
 * Server program arch:
 * 		Master thread: Use poll() to handle multiple client.
 */

#include <iostream>
#include <vector>
#include <map>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <boost/format.hpp>

using namespace std;
using namespace boost;
typedef pair <char*, uint16_t> ClientInfo;
typedef map <int, ClientInfo> ClientMap;

const size_t BUFFER_SIZE = 1024;
const int POLL_TIMEOUT = 60 * 1000;
ClientMap online;

void die(const char* message);
string onCommand(const string& command, int client_fd);

int main(int argc, char *argv[]) {
	if (argc < 2) {
		cerr << "Error: port required" << endl;
		cout << format("Usage: %1% <port>") % argv[0] << endl;
		exit(EXIT_FAILURE);
	}

	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0) {
		die("Error opening socket");
	}

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(atoi(argv[1]));

	if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
		die("Error binding address");
	}
	
	if (listen(server_fd, SOMAXCONN) < 0) {
		die("Error listening socket");
	}

	vector<struct pollfd> watch_fd(1);
	watch_fd[0].fd = server_fd;
	watch_fd[0].events = POLLIN;

	char* buffer = new char[BUFFER_SIZE];

	while (true) {
		cout << "Polling..." << endl;

		int active = poll(watch_fd.data(), watch_fd.size(), POLL_TIMEOUT);
		if (active <= 0) {
			if (active == 0) {
				cout << "poll() time out, no event happend" << endl;
				continue;
			} else {
				die("Error on poll()");
			}
		}

		// handle active client
		for (size_t i = 1; i < watch_fd.size(); i++) {
			if (watch_fd[i].revents == 0) {
				continue;
			} else {
				if (watch_fd[i].revents == POLLIN) {
					memset(buffer, 0, BUFFER_SIZE);
					int bytes = recv(watch_fd[i].fd, buffer, BUFFER_SIZE, 0);

					if (bytes > 0) {
						string response = onCommand(buffer, watch_fd[i].fd);

						if (send(watch_fd[i].fd, response.c_str(), response.length(), 0) > 0) {
							watch_fd[i].revents = 0;
							continue;
						} else {
							perror("Error on send()");
						}
					} else {
						if (bytes == 0) {
							cout << format("Client#%1% disconnected") % watch_fd[i].fd << endl;
						} else {
							perror("Error on recv()");
						}
					}
				} else {
					cerr << format("Unexpected event happend: %1%") % watch_fd[i].revents << endl;
				}

				close(watch_fd[i].fd);
				cout << format("Socket#%1% closed") % watch_fd[i].fd << endl;
				online.erase(watch_fd[i].fd);
				watch_fd.erase(watch_fd.begin() + i);
			}
		}

		// handle new client
		if (watch_fd[0].revents == POLLIN) {
			struct sockaddr_in client_addr;
			socklen_t client_addr_length =  sizeof(client_addr);

			int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_length);
			if (client_fd > 0) {
				char* ip = inet_ntoa(client_addr.sin_addr);
				uint16_t port = ntohs(client_addr.sin_port);

				cout << format("Client connected, ID: %1%, IP: %2%, Port: %3%") % client_fd % ip % port << endl;

				string hello = (format("Client ID: %1%\n") % client_fd).str();

				if (send(client_fd, hello.c_str(), hello.length(), 0) > 0) {
					struct pollfd poll_fd;
					poll_fd.fd = client_fd;
					poll_fd.events = POLLIN;

					watch_fd.push_back(poll_fd);
					online[client_fd] = ClientInfo(ip, port);
				} else {
					perror("Error on send()");
				}
			} else {
				perror("Error on accept()");
			}

			watch_fd[0].revents = 0;
		}
	}

	close(server_fd);
	return EXIT_SUCCESS; 
}

void die(const char* message) {
	perror(message);
	exit(EXIT_FAILURE);
}

string onCommand(const string& command, int client_fd) {
	if (command.find('\n') == string::npos) {
		return "Bad request\n";
	}

	if (command == "time\n") {
		time_t now = time(NULL);
		return ctime(&now);
	}

	if (command == "name\n") {
		char hostname[128];
		gethostname(hostname, 128);
		return string(hostname) + '\n';
	}

	if (command == "list\n") {
		string response;
		for (auto client : online) {
			response += (format("ID: %1%,\tIP: %2%,\tPort: %3%\n") % client.first % client.second.first % client.second.second).str();
		}
		return response;
	}

	if (command.find("send") == 0) {
		try {
			size_t begin = command.find(' ');
			size_t end = command.find(' ', begin + 1);

			if (end == string::npos) {
				return "Invalid command\n";
			}

			int dest_fd = stoi(command.substr(begin, end));

			if (online.find(dest_fd) == online.end()) {
				return "No such client\n";
			} else {
				string message = (format("Client#%1%: %2%") % client_fd % command.substr(end + 1)).str();
				if (send(dest_fd, message.c_str(), message.length(), 0) > 0) {
					return "Send success\n";
				} else {
					return "Send failed\n";
				}
			}
		} catch (const invalid_argument& e) {
			return "Invalid command\n";
		}
	}

	return "Invalid command\n";
}
