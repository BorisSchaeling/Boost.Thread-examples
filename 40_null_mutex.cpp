// uses boost::null_mutex to disable synchronization
#define BOOST_THREAD_VERSION 4
#include <boost/thread.hpp>
#include <boost/thread/synchronized_value.hpp> // include required
#include <boost/thread/null_mutex.hpp> // include required
#include <boost/asio.hpp>
#include <string>
#include <sstream>
#include <iostream>
#include <functional>

std::string get_robotstxt(const std::string &host)
{
	using namespace boost::asio;
	io_service ioservice;
	ip::tcp::resolver resolver(ioservice);
	ip::tcp::resolver::query query(host, "http");
	auto it = resolver.resolve(query);
	ip::tcp::socket socket(ioservice);
	socket.connect(*it);
	std::string request = "GET /robots.txt HTTP/1.1\r\nHost: " + host + "\r\n\r\n";
	write(socket, buffer(request));
	streambuf response;
	boost::system::error_code ec;
	read(socket, response, transfer_all(), ec);
	if (ec == error::eof)
	{
		std::ostringstream os;
		os << &response;
		std::string s = os.str();
		std::size_t idx = s.find("\r\n\r\n");
		if (idx != std::string::npos)
			s.erase(0, idx + 4);
		return s;
	}
	return "";
}

// null_mutex used to disable synchronization
boost::synchronized_value<std::vector<std::string>, boost::null_mutex> lines;

void store_lines_from_robotstxt(const std::string &host)
{
	std::string robotstxt = get_robotstxt(host);
	std::istringstream is(robotstxt);
	std::string s;
	while (std::getline(is, s))
	{
		auto p = lines.synchronize();
		p->emplace_back(s);
	}
}

int main()
{
	boost::thread_group group;
	// no synchronization necessary as only one thread is used to store lines
	group.create_thread(std::bind(store_lines_from_robotstxt, "theboostcpplibraries.com"));
	group.join_all();
	for (auto &line : lines.value())
		std::cout << line << '\n';
}
