// uses packaged tasks to execute multiple functions in one thread
// instead of having to create a new thread per function with async()
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

int main()
{
	// packaged_task is similar to std::function but returns a future
	std::vector<boost::packaged_task<std::string()>> tasks;
	tasks.emplace_back(std::bind(get_robotstxt, "theboostcpplibraries.com"));
	tasks.emplace_back(std::bind(get_robotstxt, "dieboostcppbibliotheken.de"));

	std::vector<boost::future<std::string>> futures;
	for (auto &task : tasks)
		futures.emplace_back(task.get_future());

	// thread is used instead of async() to create a thread manually
	boost::thread t([&tasks]() {
		for (auto &task : tasks)
			task();
	});
	// thread object t is detached from the operating system thread;
	// without detaching t's destructor would terminate the program
	t.detach();

	boost::wait_for_all(futures.begin(), futures.end());
	for (auto &future : futures)
		std::cout << future.get();
}
