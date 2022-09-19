/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string>
#include <iostream>
#include <arpa/inet.h>
#include <bits/stdc++.h>

using namespace std;

#define PORT "3490" // the port client will be connecting to 

#define MAXDATASIZE 100 // max number of bytes we can get at once 

typedef struct URL {
	string hostname;
	string port = "8080";
	string path;
	string fragment; 
} URL;

vector <string> tokenize(string str){
	vector <string> tokens;
	stringstream s(str);
	string intermeidate;
	while(getline(s, intermeidate, '/')) {
		tokens.push_back(intermeidate);
	}
	return tokens;
}

void print_vector(vector <string> v) {
	for (vector<string>::iterator iter = v.begin(); iter != v.end(); iter ++) {
		cout << *iter + string("hello") << endl;
	}
}

string get_path(vector<string> v) {
	string s;
	for (vector<string>::iterator iter = v.begin() + 1; iter != v.end(); iter ++) {
		s += *iter;
		if (iter != v.end() - 1) s += "/";
	}
	return s;
}
URL* parse_url(const string url_raw) {
	string url = url_raw; // max length of url could be 512
	URL* ret = new URL();
	if (!url.compare(0, 7, "http://")) {
		url = url.substr(7);
	}
	vector <string> tokens = tokenize(url);
	ret->hostname = tokens[0];
	if (tokens.size() > 1) ret->path = get_path(tokens);
	return ret;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
	// string url = "http://fuck you ass hole/ fdasjfl/fdsaf";
	URL* info = parse_url(argv[1]);
	// cout << int(inet_addr("127.0.0.1")) << endl;
	int sockfd, numbytes;  
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];
	cout << int(p->ai_protocol) << endl;

	if (argc != 2) {
	    fprintf(stderr,"usage: client hostname\n");
	    exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(info->hostname.c_str(), PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		// cout << "fuck" << endl;
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		cout << sockfd << endl;

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}

		cout << "fuck2" << endl;
		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure

	if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
	    perror("recv");
	    exit(1);
	}

	buf[numbytes] = '\0';

	printf("client: received '%s'\n",buf);

	close(sockfd);
	
	delete(info);

	return 0;
}

