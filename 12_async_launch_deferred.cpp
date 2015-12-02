// passes deferred launch policy to async()
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
	// passing deferred as a launch policy immediately terminates the program;
	// support for deferred missing in all Boost.Thread versions (at least up to 1.59)
	boost::future<std::string> f = boost::async(boost::launch::deferred, std::bind(get_robotstxt, "theboostcpplibraries.com"));
	// deferred is supposed to execute function/functor passed to async() when get() is
	// called on the future; the function/functor is also supposed to execute in the same
	// thread get() is called from
	std::cout << f.get();
}
