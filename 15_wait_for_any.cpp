// calls wait_for_any() to detect when at least one of several futures is ready
#define BOOST_THREAD_VERSION 4
#include <boost/thread.hpp>
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

int main()
{
	boost::future<std::string> f1 = boost::async(std::bind(get_robotstxt, "theboostcpplibraries.com"));
	boost::future<std::string> f2 = boost::async(std::bind(get_robotstxt, "dieboostcppbibliotheken.de"));
	// blocks until at least one future is ready and returns the zero-based
	// index of the ready future
	unsigned index = boost::wait_for_any(f1, f2);
	std::cout << (index == 0 ? f1.get() : f2.get());
}
