// sets thread attributes and thread priority
#define BOOST_THREAD_VERSION 4
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <string>
#include <sstream>
#include <iostream>
#include <functional>
#include <utility>

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
	boost::packaged_task<std::string()> task(std::bind(get_robotstxt, "theboostcpplibraries.com"));
	boost::future<std::string> future = task.get_future();
	// sets the thread's stack size to 1024 bytes;
	// this is a hint only
	boost::thread::attributes attrs;
	attrs.set_stack_size(1024);
	boost::thread t(attrs, std::move(task));
#ifdef BOOST_WINDOWS
	// gets the thread's native handle to increase the thread priority on Windows
	SetThreadPriority(t.native_handle(), THREAD_PRIORITY_HIGHEST);
#endif
	t.detach();
	std::cout << future.get();
}
