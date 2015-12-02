// uses interruption points to break out of infinite loop and exit the program automatically
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

std::string line;
boost::mutex line_mutex;
boost::condition_variable line_cond;
boost::thread_group group;

void store_lines_from_robotstxt(const std::string &host)
{
	std::string robotstxt = get_robotstxt(host);
	std::istringstream is(robotstxt);
	std::string s;
	boost::unique_lock<boost::mutex> lock(line_mutex);
	while (std::getline(is, s))
	{
		line = s;
		line_cond.notify_all();
		line_cond.wait(lock);
	}
	// thread_group is thread-safe; this thread may call interrupt_all()
	// while main() in the primary thread is stuck in join_all()
	group.interrupt_all();
}

void output_lines()
{
	try
	{
		boost::unique_lock<boost::mutex> lock(line_mutex);
		for (;;)
		{
			while (line.empty())
				line_cond.wait(lock); // wait is an interruption point
			std::cout << line << '\n';
			line.clear();
			line_cond.notify_all();
		}
	}
	catch (boost::thread_interrupted&)
	{
		// boost::thread_interrupted is thrown when group.interrupt_all() is called
	}
}

int main()
{
	group.create_thread(std::bind(store_lines_from_robotstxt, "theboostcpplibraries.com"));
	group.create_thread(output_lines);
	group.join_all();
}
