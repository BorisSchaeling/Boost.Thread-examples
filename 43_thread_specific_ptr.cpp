// uses thread-specific storage to count lines processed per thread
#define BOOST_THREAD_VERSION 4
#include <boost/thread.hpp>
#include <boost/thread/synchronized_value.hpp> // include required
#include <boost/thread/strict_lock.hpp> // include required
#include <boost/lockfree/queue.hpp>
#include <boost/asio.hpp>
#include <string>
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
// thread specific variable to count how many lines each thread processed
boost::thread_specific_ptr<int> lines_processed;

void store_lines_from_robotstxt(const std::string &host)
{
	std::string robotstxt = get_robotstxt(host);
	std::istringstream is(robotstxt);
	std::string s;
	while (std::getline(is, s))
	{
		auto p = lines.synchronize();
		p->emplace_back(s);
		++(*lines_processed);
	}
}

// no std::string as Boost.Lockfree containers support only elements with trivial destructors
boost::lockfree::queue<const char*> hosts(2);
boost::mutex cout_mutex;

void process_tasks()
{
	// thread_specific_ptr calls delete() in destructor (by default)
	lines_processed.reset(new int(0));
	const char *host;
	while (hosts.pop(host))
		store_lines_from_robotstxt(host);
	boost::strict_lock<boost::mutex> cout_lock(cout_mutex);
	// get_id() returns a thread identifier which can be written to an output stream
	std::cout << "Lines processed by thread " << boost::this_thread::get_id() << ": " << *lines_processed << '\n';
}

int main()
{
	hosts.push("theboostcpplibraries.com");
	hosts.push("dieboostcppbibliotheken.de");

	boost::thread_group group;
	// physical_concurrency() returns the number of processor cores (without hyperthreading units);
	// call hardware_concurrency() for the number of logical cores (including hyptherthreading units)
	for (unsigned int i = 0; i < boost::thread::physical_concurrency(); ++i)
		group.create_thread(process_tasks);
	group.join_all();
}
