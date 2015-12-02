// uses boost::synchronized_value to link variable and mutex
#define BOOST_THREAD_VERSION 4
#include <boost/thread.hpp>
#include <boost/thread/synchronized_value.hpp> // include required
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

// lines is a boost::synchronized_value with a vector of strings and a mutex
boost::synchronized_value<std::vector<std::string>, boost::mutex> lines;

void store_lines_from_robotstxt(const std::string &host)
{
	std::string robotstxt = get_robotstxt(host);
	std::istringstream is(robotstxt);
	std::string s;
	while (std::getline(is, s))
	{
		// synchronize() returns a locked pointer of type boost::strict_lock_ptr
		// which provides synchronized access to the vector of strings; the destructor
		// of boost::strict_lock_ptr unlocks the mutex automatically
		boost::strict_lock_ptr<std::vector<std::string>, boost::mutex> p = lines.synchronize();
		p->emplace_back(s);
	}
}

int main()
{
	boost::thread_group group;
	group.create_thread(std::bind(store_lines_from_robotstxt, "theboostcpplibraries.com"));
	group.create_thread(std::bind(store_lines_from_robotstxt, "dieboostcppbibliotheken.de"));
	group.join_all();
	// boost::synchronized_value provides a member function value() to access
	// the vector of strings without having to use the mutex (which is not required
	// here as all threads have been terminated)
	for (auto &line : lines.value())
		std::cout << line << '\n';
}
