#include <optional>
#include <iostream>
#include <cstring>
#include <vector>

#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <csignal>
#include <unistd.h>

#include "base.h"
#include "fileio.h"
#include "utils.h"
#include "method.h"
#include "request.h"
#include "response.h"

std::vector<pid_t> workers;
pthread_mutex_t workers_mutex;
int g_sock_fd;

bool read_body(int sock_fd, RequestHeader &header)
{
	if (!header.has_header("Content-Length"))
  {
    printf("no content length header. Skipping read_body\n");
    return true;
  }

  auto content_length = std::stol(header["Content-Length"]);
  printf("got body with length: %ld\n", content_length);

  if (!should_read_body(header.method))
  {
    printf("but body is forbidden for method %s\n", header.uri.method.c_str());
    return true;
  }
  if (content_length == 0)
  {
    printf("content length is 0.\n");
    return true;
  }
  printf("reading body...\n");

  auto body = std::make_unique<char[]>(content_length + 1);
  body[content_length] = 0;

  auto len = read(sock_fd, body.get(), content_length);
  if (len < 0)
  {
    printf("failed to read body. stream is over\n");
    return false;
  }
  if (len != content_length)
  {
    printf("read body length %ld != %ld\n", len, content_length);
    return false;
  }

  header.body = std::string(body.get());

  printf("read body with length %ld\n", header.body.size());
  return true;
}

std::string content_type_by_ext(const std::string &ext)
{
  printf("resolving content type for extension %s\n", ext.c_str());
	if (ext == ".html") return "text/html";
	if (ext == ".jpg") return "image/jpeg";
	if (ext == ".png") return "image/png";
	if (ext == ".json") return "application/json";
	return "text/plain";
}

HandlerResponse handle_GET(const RequestHeader &header, bool send_body = true)
{
	auto path = get_path(url_decode(header.uri.uri.substr(1)));
	auto status = file_status(path);

	if (!status.is_exists)
	{
    printf("file %s does not exists\n", path.c_str());
		return {"404", "Not Found"};
	}

	switch (status.file_type)
	{
		case FileType::REGULAR:
      printf("file is regular file, reading...\n");
			if (auto content_o = read_file(path); content_o)
			{
        auto content = content_o.value();
        printf("content length: %lu\n", content.length());
				HandlerResponse resp { "200", "OK", send_body ? content : "" };
				resp.add_header("Content-Type", content_type_by_ext(file_extension(path)) + std::string("; charset=UTF-8"));
        resp.add_header("Content-Length", std::to_string(content.length()));
				return resp;
			}
			else
			{
        printf("failed to read file\n");
				return {"500", "Internal Server Error"};
			}
		case FileType::DIRECTORY:
			return {"200", "OK", std::string("Directory ") + path};
		default:
			return {"501", "Not Implemented"};
	}
}

HandlerResponse handle_request(const RequestHeader &header)
{
	switch(header.method) 
	{
    case Method::GET: case Method::HEAD:
        printf("handling GET request...\n");
				return handle_GET(header, header.method == Method::GET);
		default:
				fprintf(stderr, "unsupported method %d : %s\n", header.method, header.uri.method.c_str());
				return {"405", "Method Not Allowed"};
	}
}

auto parse_headers(const std::string &headers)
{
	printf("headers len %ld\n", headers.size());
	auto lines = tokenize(headers, '\n');

	if (lines.empty()) 
	{
		fprintf(stderr, "malformed request: empty\n");
		return std::optional<RequestHeader>();
	}
  	
	auto uri_header = tokenize(lines[0], ' ');
	if (uri_header.size() != 3) 
	{
		fprintf(stderr, "malformed request header: uri header size %ld != 3\n", uri_header.size());
		fprintf(stderr, "'%s'\n", lines[0].c_str());
		return std::optional<RequestHeader>();
	}

	UriHeader uri { uri_header[0], uri_header[1], uri_header[2] };
	printf("request %s by %s with protocol %s\n", uri.uri.c_str(), uri.method.c_str(), uri.protocol.c_str());

	lines.erase(lines.begin());
	lines.erase(lines.end() - 1);

	RequestHeader request(uri);

	for (const auto & line : lines) 
	{
		auto [k, v] = consume(line, ':');
		if (!v.empty())
		{
			request.add_header(k, v.substr(1));
		}
		else 
		{
			request.add_header(k, "");
		}
	}

	return std::optional<RequestHeader>(request);
}

std::string read_headers(int sock_fd) 
{
	std::string buffer;
	std::string ending;
	char c = 0;
	while (read(sock_fd, &c, 1) == 1)
 	{
		ending += c;
		if (ending.size() > 4) ending.erase(ending.begin());
		if (ending == "\r\n\r\n") break;
		if (c != '\r')
			buffer += c;
	}
	return buffer;
}

std::string build_response(const Response &resp) 
{
  std::ostringstream response_stream;
	response_stream << resp;

	response_stream << "Date: " << get_date() << "\n";
	response_stream << "Server: " << get_server_id() << "\n";
	response_stream << "Accept-Ranges: bytes\n";
  if (!resp.response.has_header("Content-Length"))
    response_stream << "Content-Length: " << resp.response.body.size() << "\n";
	if (!resp.response.has_header("Content-Type"))
    response_stream << "Content-Type: text/plain; charset=UTF-8\n";
	response_stream <<  "\n";
	response_stream << resp.response.body;

  auto response_data = response_stream.str();
	return response_data;
}

// why has response_data lifetime fail if pass it as const &?
// Fixed?
void write_response_data(int sock_fd, std::string response_data)
{
	printf("writing response...\n");
	auto bytes = write(sock_fd, response_data.c_str(), response_data.size());
	printf("wrote %ld bytes of %ld\n", bytes, response_data.size());
}

void send_response(int sock_fd, const Response &resp) 
{
  write_response_data(sock_fd, build_response(resp));
}

void error_cool_down()
{
	usleep(10000);
}

int client(int conn) 
{
  struct sigaction sa{};
  sa.sa_handler = SIG_DFL;
  sigaction(SIGKILL, &sa, nullptr);

  sa.sa_handler = SIG_DFL;
  sigaction(SIGTERM, &sa, nullptr);

  printf("connection worker with pid %d started\n", getpid());
	auto headers = read_headers(conn);
	printf("header read\n");

	if (auto o_request = parse_headers(headers); o_request) 
	{
		auto request = o_request.value();
		if (request.method != Method::INVALID) 
		{
			read_body(conn, request); 		
			Response response { request, handle_request(request) };
			send_response(conn, response);
		}
		else 
		{
			fprintf(stderr, "unknown method %s. Rejecting\n", request.uri.method.c_str());
		}
	}
	close(conn);
	return 0;
}

bool handle_connection(int sock_fd) {
  struct sockaddr_in conn_address{};
  std::memset(&conn_address, '\0', sizeof(conn_address));
  socklen_t conn_len = sizeof(conn_address);
  if (auto conn_fd =
        accept(sock_fd, reinterpret_cast<struct sockaddr*>(&conn_address), &conn_len);
      conn_fd >= 0)
  {
    printf("waiting workers availability...\n");
    pthread_mutex_lock(&workers_mutex);
    pid_t w_pid = fork();
    if (w_pid < 0)
    {
      perror("fork");
      return false;
    }
    if (w_pid == 0)
    {
      close(sock_fd);
      exit(client(conn_fd));
    }
    else
    {
      close(conn_fd);
      workers.push_back(w_pid);
      pthread_mutex_unlock(&workers_mutex);
    }
  }
  else
  {
    perror("accept");
    error_cool_down();
  }
  return true;
}

void await_workers() {
  // no more workers
  pthread_mutex_lock(&workers_mutex);
  // disable child death catching
  struct sigaction sa{};
  sa.sa_handler = SIG_DFL;
  sigaction(SIGCHLD, &sa, nullptr);

  int status = 0;
  for (const auto pid : workers) {
    kill(pid, SIGKILL);
    pid_t p = waitpid(pid, &status, 0);
    printf("%d process died while awaiting with status %d...\n", p, status);
  }

  pthread_mutex_unlock(&workers_mutex);
}

void setup_server(uint32_t port)
{
	if (auto sock_fd = socket(AF_INET, SOCK_STREAM, 0); sock_fd >= 0)
 	{
    g_sock_fd = sock_fd;
		struct sockaddr_in server_address{};
		std::memset(&server_address, '\0', sizeof(server_address));
		server_address.sin_family = AF_INET;
		server_address.sin_addr.s_addr = INADDR_ANY;
		server_address.sin_port = htons(port);
		
		if (bind(sock_fd, reinterpret_cast<struct sockaddr *>(&server_address), sizeof(server_address)) >= 0) 
		{
			printf("bind successful on port %d\n", port);
			printf("starting listen...\n");
			listen(sock_fd, 32);
			while (handle_connection(sock_fd)){}
      close(sock_fd);
      await_workers();
      return;
		}
    perror("bind");
	}
	else 
	{
		perror("socket");
	}
}

void child_died(int)
{
  int status = 0;
  pid_t p = waitpid(-1, &status, WNOHANG);
  printf("child death status %d pid %d\n", status, p);
  workers.erase(std::find(workers.begin(), workers.end(), p));
}

void server_kill(int) {
  printf("[!] requested server kill!\n");
  await_workers();
  pthread_mutex_destroy(&workers_mutex);
  close(g_sock_fd);
  exit(0);
}

int main(int argc, char *argv[])
{
  pthread_mutex_init(&workers_mutex, nullptr);

  struct sigaction sa{};
	sa.sa_handler = child_died;
	sa.sa_flags = SA_RESTART;
	sigaction(SIGCHLD, &sa, nullptr);

  sa.sa_handler = server_kill;
  sigaction(SIGKILL, &sa, nullptr);

  sa.sa_handler = server_kill;
  sigaction(SIGTERM, &sa, nullptr);

	setup_server(8180u);
  raise(SIGKILL);
}
