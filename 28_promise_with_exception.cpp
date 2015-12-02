// passes an exception from a boost::promise to a boost::future
#define BOOST_THREAD_VERSION 4
#include <boost/thread.hpp>
#include <boost/thread/scoped_thread.hpp> // include required
#include <boost/asio.hpp>
#include <string>
#include <sstream>
#include <iostream>
#include <functional>
#include <utility>

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
	boost::promise<std::string> promise;
	boost::future<std::string> future = promise.get_future();
	boost::scoped_thread<> t([&promise]() {
		try
		{
			std::string payload;
			// intentionally misspelled hostname to make get_robotstxt() throw an exception
			get_robotstxt("theboostcpplibraries.comm", payload);
			promise.set_value(payload);
		}
		catch (std::exception &ex)
		{
			// exception is set in the promise
			promise.set_exception(ex);
		}
	});
	try
	{
		// get() throws the exception
		std::cout << future.get();
	}
	catch (std::exception &ex)
	{
		std::cout << ex.what();
	}
}
