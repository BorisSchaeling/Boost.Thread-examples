// uses boost::externally_locked to link a variable to a mutex elsewhere
#define BOOST_THREAD_VERSION 4
#include <boost/thread.hpp>
#include <boost/thread/externally_locked.hpp> // include required
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

boost::mutex lines_mutex;
// boost::externally_locked is used to define a vector of strings
// which is linked to the mutex lines_mutex; the vector can only be accessed
// through member functions of boost::externally_locked which will guarantee
// that the mutex is locked properly
boost::externally_locked<std::vector<std::string>, boost::mutex> lines(lines_mutex);

void store_lines_from_robotstxt(const std::string &host)
{
	std::string robotstxt = get_robotstxt(host);
	std::istringstream is(robotstxt);
	std::string s;
	while (std::getline(is, s))
	{
		boost::strict_lock<boost::mutex> lock(lines_mutex);
		// get() returns a reference to the vector; get() requires that
		// a lock of type boost::strict_lock<boost::mutex> is passed;
		// get() checks at run-time whether the same mutex is locked
		// which has been passed to the constructor of boost::externally_locked
		lines.get(lock).emplace_back(s);
	}
}

int main()
{
	boost::thread_group group;
	group.create_thread(std::bind(store_lines_from_robotstxt, "theboostcpplibraries.com"));
	group.create_thread(std::bind(store_lines_from_robotstxt, "dieboostcppbibliotheken.de"));
	group.join_all();
	// boost::externally_locked does not provide unsynchronized access;
	// boost::strict_lock must be used although not necessary here
	// (all threads have been terminated at this point)
	boost::strict_lock<boost::mutex> lock(lines_mutex);
	for (auto &line : lines.get(lock))
		std::cout << line << '\n';
}
