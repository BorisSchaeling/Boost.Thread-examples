// uses a future to detect when the thread created with async() has terminated;
// the program exits when it is done (no need to press CTRL+C)
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <string>
#include <sstream>
#include <iostream>
#include <functional>

void get_robotstxt(const std::string &host)
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
		std::cout << s;
	}
}

int main()
{
	// async() returns a future ...
	boost::unique_future<void> f = boost::async(std::bind(get_robotstxt, "theboostcpplibraries.com"));
	// ... which provides a member function is_ready(); is_ready() returns true
	// when the thread has terminated
	while (!f.is_ready())
	{
		boost::this_thread::sleep_for(boost::chrono::seconds(1));
		std::cout << '.';
	}
}
