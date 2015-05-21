#include <iostream>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <netinet/in.h>

#undef NDEBUG
#include <cassert>

using namespace std;
const size_t BUFFER_SIZE = 1024;

int main(int argc, char *argv[]) {
	if (argc < 2) {
		cerr << "ERROR: no port provided" << endl;
		return EXIT_FAILURE;
	}

	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	assert(server_fd > 0);

	struct sockaddr_in server_addr, client_addr;

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(atoi(argv[1]));

	const int timeout = 60 * 1000;
	char* buffer = new char[BUFFER_SIZE];
	int res;

	res = bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr));
	assert(res >= 0);
	
	res = listen(server_fd, SOMAXCONN);
	assert(res >= 0);

	vector<struct pollfd> watch_fd(1);
	watch_fd[0].fd = server_fd;
	watch_fd[0].events = POLLIN;

	while (true) {
		cout << "polling..." << endl;

		res = poll(watch_fd.data(), watch_fd.size(), timeout);
		assert(res >= 0);

		if (res == 0) {
			cout << "poll() timed out, nothing happend" << endl;
			continue;
		}

		for (size_t i = 1; i < watch_fd.size(); i++) {
			if (watch_fd[i].revents == 0) {
				continue;
			} else {
				if (watch_fd[i].revents == POLLIN) {
					memset(buffer, 0, BUFFER_SIZE);

					res = read(watch_fd[i].fd, buffer, BUFFER_SIZE);
					assert(res >= 0);

					cout << "Message: " << buffer << endl;

					string response = "Recv: ";
					response += buffer;

					res = write(watch_fd[i].fd, response.c_str(), response.length());
					assert(res >= 0);
				} else {
					cerr << "other events happend" << endl;
					cout << watch_fd[i].revents << endl;
					//close(watch_fd[i].fd);
					watch_fd.erase(watch_fd.begin() + i);
				}

				watch_fd[i].revents = 0;
			}

		}

		if (watch_fd[0].revents == POLLIN) {
			socklen_t client_addr_length =  sizeof(client_addr);
			int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_length);
			assert(client_fd >= 0);

			struct pollfd new_fd;
			new_fd.fd = client_fd;
			new_fd.events = POLLIN;

			watch_fd.push_back(new_fd);

			watch_fd[0].revents = 0;
		}
	}

	close(server_fd);
	return EXIT_SUCCESS; 
}
