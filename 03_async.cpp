// creates a new thread to execute get_robotstxt() and prints a dot every
// second in the primary thread; program must be canceled with CTRL+C;
// latest Boost.Thread version is 4 (by default version 2 is used)
#define BOOST_THREAD_VERSION 4
#include <boost/thread.hpp>
#include <boost/asio.hpp>
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
	// async() creates a new thread; the function/functor passed as parameter is
	// executed in the new thread; when the function/functor returns, the thread
	// terminates
	boost::async(std::bind(get_robotstxt, "theboostcpplibraries.com"));
	while (true)
	{
		// sleep_for() is a global function (this_thread is a namespace) to pause
		// a thread; the time duration parameter is based on Boost.Chrono
		boost::this_thread::sleep_for(boost::chrono::seconds(1));
		std::cout << '.';
	}
}
