// uses scoped_thread to detach in the destructor (instead of joining)
#define BOOST_THREAD_VERSION 4
#include <boost/thread.hpp>
#include <boost/thread/scoped_thread.hpp> // include required
#include <boost/asio.hpp>
#include <string>
#include <functional>
#include <sstream>
#include <iostream>
#include <utility>

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
	boost::packaged_task<std::string()> task(std::bind(get_robotstxt, "theboostcpplibraries.com"));
	boost::future<std::string> future = task.get_future();
	// template parameter of boost::scoped_thread must be a callable;
	// template parameter is called in destructor; boost::detach
	// will detach from the thread instead of joining (which is
	// the default behavior if no template parameter is passed)
	boost::scoped_thread<boost::detach> st(std::move(task));
	std::cout << future.get();
}
