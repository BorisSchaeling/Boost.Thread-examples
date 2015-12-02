// HTTP client to download and output the file theboostcpplibraries.com/robots.txt;
// intentionally calls blocking functions to "slow down" the program; motivation
// to execute the HTTP client in its own thread in the other sample programs
#include <boost/asio.hpp>
#include <string>
#include <sstream>
#include <iostream>

int main()
{
	std::string host = "theboostcpplibraries.com";
	using namespace boost::asio;
	io_service ioservice;
	ip::tcp::resolver resolver(ioservice);
	ip::tcp::resolver::query query(host, "http");
	auto it = resolver.resolve(query); // blocking
	ip::tcp::socket socket(ioservice);
	socket.connect(*it); // blocking
	std::string request = "GET /robots.txt HTTP/1.1\r\nHost: " + host + "\r\n\r\n";
	write(socket, buffer(request)); // blocking
	streambuf response;
	boost::system::error_code ec;
	read(socket, response, transfer_all(), ec); // blocking
	if (ec == error::eof)
	{
		std::ostringstream os;
		os << &response;
		std::string s = os.str();
		std::string::size_type idx = s.find("\r\n\r\n");
		if (idx != std::string::npos)
			s.erase(0, idx + 4);
		std::cout << s;
	}
}
