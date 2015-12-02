// uses boost::promise instead of boost::packaged_task for a function
// which returns a value through a by-ref parameter
#define BOOST_THREAD_VERSION 4
#include <boost/thread.hpp>
#include <boost/thread/scoped_thread.hpp> // include required
#include <boost/asio.hpp>
#include <string>
#include <sstream>
#include <iostream>
#include <functional>
#include <utility>

// get_robotstxt() has been changed to return the result through a by-ref parameter;
// now get_robotstxt() can't be used with a boost::packaged_task anymore
void get_robotstxt(const std::string &host, std::string &payload)
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
		payload = s;
	}
	else
	{
		payload.clear();
	}
}

int main()
{
	// boost::promise and boost::future represent two ends
	// of a pipe which can be used to send a value from one thread
	// to another
	boost::promise<std::string> promise;
	boost::future<std::string> future = promise.get_future();
	// boost::promise has no copy-constructor
	boost::scoped_thread<> t([&promise]() {
		std::string payload;
		get_robotstxt("theboostcpplibraries.com", payload);
		// the value set in boost::promise is sent to the linked boost::future
		promise.set_value(payload);
	});
	std::cout << future.get();
}
