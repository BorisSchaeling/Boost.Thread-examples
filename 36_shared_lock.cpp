// uses boost::shared_lock instead of boost::shared_lock_guard
#define BOOST_THREAD_VERSION 4
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <string>
#include <sstream>
#include <iostream>
#include <functional>
#include <cstddef>

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
boost::shared_mutex lines_mutex;

void store_lines_from_robotstxt(const std::string &host)
{
	std::string robotstxt = get_robotstxt(host);
	std::istringstream is(robotstxt);
	std::string s;
	while (std::getline(is, s))
	{
		boost::unique_lock<boost::shared_mutex> lg(lines_mutex);
		lines.emplace_back(s);
	}
}

void output_lines()
{
	std::size_t read_lines = 0;
	// boost::shared_lock is more versatile than boost::shared_lock_guard;
	// boost::defer_lock makes the constructor skip locking
	boost::shared_lock<boost::shared_mutex> lg(lines_mutex, boost::defer_lock);
	for (;;)
	{
		// mutex is locked through the lock explicitly
		lg.lock();
		if (read_lines < lines.size())
		{
			std::cout << lines[read_lines] << '\n';
			++read_lines;
		}
		// mutex is unlocked through the lock explicitly;
		// boost::shared_lock will unlock the mutex in the destructor
		// if code above throws an exception
		lg.unlock();
	}
}

void get_size_of_longest_line()
{
	std::size_t read_lines = 0;
	std::size_t max_size = 0;
	// boost::shared_lock is more versatile than boost::shared_lock_guard;
	// boost::defer_lock makes the constructor skip locking
	boost::shared_lock<boost::shared_mutex> lg(lines_mutex, boost::defer_lock);
	for (;;)
	{
		// mutex is locked through the lock explicitly
		lg.lock();
		if (read_lines < lines.size())
		{
			if (lines[read_lines].size() > max_size)
				max_size = lines[read_lines].size();
			++read_lines;
		}
		// mutex is unlocked through the lock explicitly;
		// boost::shared_lock will unlock the mutex in the destructor
		// if code above throws an exception
		lg.unlock();
	}
}

int main()
{
	boost::thread_group group;
	group.create_thread(std::bind(store_lines_from_robotstxt, "theboostcpplibraries.com"));
	group.create_thread(std::bind(store_lines_from_robotstxt, "dieboostcppbibliotheken.de"));
	group.create_thread(output_lines);
	group.create_thread(get_size_of_longest_line);
	group.join_all();
}
