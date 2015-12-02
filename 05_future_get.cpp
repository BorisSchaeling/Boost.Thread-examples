// get_robotstxt() returns robots.txt instead of writing it to std::cout;
// the return value is fetched through the future and written to std::cout
// in the primary thread in main()
#define BOOST_THREAD_VERSION 4
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <string>
#include <sstream>
#include <iostream>
#include <functional>

// return type has been changed from void to std::string
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
	// future's template parameter is std::string as this is the return type
	// of get_robotstxt()
	boost::future<std::string> f = boost::async(std::bind(get_robotstxt, "theboostcpplibraries.com"));
	while (!f.is_ready())
	{
		boost::this_thread::sleep_for(boost::chrono::seconds(1));
		std::cout << '.';
	}
	// get() returns the return value from get_robotstxt()
	std::cout << f.get();
}
