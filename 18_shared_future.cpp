// uses shared_future to receive return value in multiple threads
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
	// future defines share() which returns a shared future
	boost::shared_future<std::string> f1 = boost::async(std::bind(get_robotstxt, "theboostcpplibraries.com")).share();
	// get() may be called multiple times on a shared future;
	// threads may call get() on the same shared future instance
	boost::future<void> f2 = boost::async([&f1]() { std::cout << "Thread 1: " << f1.get(); });
	boost::future<void> f3 = boost::async([&f1]() { std::cout << "Thread 2: " << f1.get(); });
	boost::wait_for_all(f2, f3);
}
