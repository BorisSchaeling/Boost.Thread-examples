// uses boost::once_flag and boost::call_once() to call a function exactly once
#define BOOST_THREAD_VERSION 4
#include <boost/thread.hpp>
#include <boost/thread/synchronized_value.hpp> // include required
#include <boost/asio.hpp>
#include <string>
#include <functional>
#include <sstream>
#include <iostream>

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

boost::synchronized_value<std::vector<std::string>, boost::mutex> lines;
// flag required for boost::call_once(); flag is used to detect
// whether boost::call_once() has already called the function
boost::once_flag once;

void store_lines_from_robotstxt(const std::string &host)
{
	std::string robotstxt = get_robotstxt(host);
	std::istringstream is(robotstxt);
	std::string s;
	while (std::getline(is, s))
	{
		auto p = lines.synchronize();
		p->emplace_back(s);
	}
	// boost::call_once() calls the lambda function exactly once
	// even though boost::call_once() is called in each thread
	boost::call_once(once, []() {
		std::cout << "Thread " << boost::this_thread::get_id() << " is done first\n";
	});
}

int main()
{
	boost::thread_group group;
	group.create_thread(std::bind(store_lines_from_robotstxt, "theboostcpplibraries.com"));
	group.create_thread(std::bind(store_lines_from_robotstxt, "dieboostcppbibliotheken.de"));
	group.join_all();
}
