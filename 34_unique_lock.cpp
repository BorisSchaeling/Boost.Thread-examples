// uses boost::unique_lock instead of boost::strict_lock and boost::lock_guard
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

std::vector<std::string> lines;
boost::mutex lines_mutex;

void store_lines_from_robotstxt(const std::string &host)
{
	std::string robotstxt = get_robotstxt(host);
	std::istringstream is(robotstxt);
	std::string s;
	// boost::unique_lock is most versatile lock class;
	// boost::defer_lock makes the constructor skip locking
	boost::unique_lock<boost::mutex> l(lines_mutex, boost::defer_lock);
	while (std::getline(is, s))
	{
		// mutex is locked through the lock explicitly
		l.lock();
		lines.emplace_back(s);
		// mutex is unlocked through the lock explicitly;
		// boost::unique_lock will unlock the mutex in the destructor
		// if emplace_back() throws an exception
		l.unlock();
	}
}

int main()
{
	boost::thread_group group;
	group.create_thread(std::bind(store_lines_from_robotstxt, "theboostcpplibraries.com"));
	group.create_thread(std::bind(store_lines_from_robotstxt, "dieboostcppbibliotheken.de"));
	group.join_all();
	for (auto &l : lines)
		std::cout << l << '\n';
}
