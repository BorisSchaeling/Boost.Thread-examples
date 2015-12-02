// uses boost::condition_variable to coordinate two threads - one of them
// writing to a global variable and one of them reading from it;
// program must be terminated with CTRL+C as it uses infinite loops in reader threads
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
// condition variable for global variable line above
boost::condition_variable line_cond;

void store_lines_from_robotstxt(const std::string &host)
{
	std::string robotstxt = get_robotstxt(host);
	std::istringstream is(robotstxt);
	std::string s;
	boost::unique_lock<boost::mutex> lock(line_mutex);
	while (std::getline(is, s))
	{
		line = s;
		// notify_all() sends a notification to all threads currently
		// waiting on the condition variable to wake up and resume
		line_cond.notify_all();
		// wait() pauses the thread until a notification is sent;
		// wait() may only be called with a locked mutex; wait()
		// automatically unlocks the mutex; the mutex is automatically
		// locked again when wait() returns; the while-loop is used to
		// protect us from spurious wakeups (see https://en.wikipedia.org/wiki/Spurious_wakeup)
		while (!line.empty())
			line_cond.wait(lock);
	}
}

void output_lines()
{
	boost::unique_lock<boost::mutex> lock(line_mutex);
	for (;;)
	{
		// wait() pauses the thread until a notification is sent;
		// wait() may only be called with a locked mutex; wait()
		// automatically unlocks the mutex; the mutex is automatically
		// locked again when wait() returns; the while-loop is used to
		// protect us from spurious wakeups (see https://en.wikipedia.org/wiki/Spurious_wakeup)
		while (line.empty())
			line_cond.wait(lock);
		std::cout << line << '\n';
		line.clear();
		// notify_all() sends a notification to all threads currently
		// waiting on the condition variable to wake up and resume
		line_cond.notify_all();
	}
}

int main()
{
	boost::thread_group group;
	group.create_thread(std::bind(store_lines_from_robotstxt, "theboostcpplibraries.com"));
	group.create_thread(output_lines);
	group.join_all();
}
