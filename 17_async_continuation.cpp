// calls then() on the future to use continuations
#define BOOST_THREAD_VERSION 4
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <string>
#include <sstream>
#include <iostream>
#include <functional>
#include <vector>

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
	std::vector<std::string> v;
	// functions/functors passed to async() and then() are executed sequentially;
	// the future of the previous function/functor is passed as a parameter to the
	// next function/functor
	boost::future<std::string> f = boost::async(std::bind(get_robotstxt, "theboostcpplibraries.com")).then([&v](boost::future<std::string> f) {
		v.push_back(f.get());
		return get_robotstxt("dieboostcppbibliotheken.de");
	}).then([&v](boost::future<std::string> f) -> std::string {
		v.push_back(f.get());
		return "";
	});
	while (!f.is_ready())
	{
		boost::this_thread::sleep_for(boost::chrono::seconds(1));
		std::cout << '.';
	}
	std::cout << v[0] << v[1];
}
