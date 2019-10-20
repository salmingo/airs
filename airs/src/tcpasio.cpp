/*
 * @file tcpasio.cpp 定义文件, 基于boost::asio实现TCP通信接口
 */

#include <boost/lexical_cast.hpp>
#include "tcpasio.h"

using std::string;
using namespace boost::asio;
//////////////////////////////////////////////////////////////////////////////
/*---------------- TCPClient: 客户端 ----------------*/
TcpCPtr maketcp_client() {// 工厂函数, 创建TcpCPtr
	return boost::make_shared<TCPClient>();
}

TCPClient::TCPClient()
	: sock_(keep_.get_service()) {
	bytercv_ = 0;
	bufrcv_.reset(new char[TCP_PACK_SIZE]);
	usebuf_ = false;
	pause_rcv_ = false;
}

TCPClient::~TCPClient() {
	Close();
}

tcp::socket& TCPClient::GetSocket() {
	return sock_;
}

/*
 * @note 同步方式连接服务器
 */
bool TCPClient::Connect(const string& host, const uint16_t port) {
	tcp::resolver resolver(keep_.get_service());
	tcp::resolver::query query(host, boost::lexical_cast<string>(port));
	tcp::resolver::iterator itertor = resolver.resolve(query);
	boost::system::error_code ec;
	sock_.connect(*itertor, ec);
	if (!ec) {
		sock_.set_option(socket_base::keep_alive(true));
		start_read();
	}
	return !ec;
}

/*
 * @note 异步方式连接服务器, 由回调函数监测连接结果
 */
void TCPClient::AsyncConnect(const string& host, const uint16_t port) {
	tcp::resolver resolver(keep_.get_service());
	tcp::resolver::query query(host, boost::lexical_cast<string>(port));
	tcp::resolver::iterator itertor = resolver.resolve(query);

	sock_.async_connect(*itertor,
			boost::bind(&TCPClient::handle_connect, this, placeholders::error));
}

int TCPClient::Close() {
	boost::system::error_code ec;
	if (sock_.is_open()) sock_.close(ec);
	return ec.value();
}

bool TCPClient::IsOpen() {
	return sock_.is_open();
}

/*
 * @note UseBuffer()应在建立连接前仅调用一次
 */
void TCPClient::UseBuffer(bool usebuf) {
	if (usebuf_ != usebuf) {
		usebuf_ = usebuf;
		if (usebuf_) {
			crcread_.set_capacity(TCP_PACK_SIZE * 100);
			crcwrite_.set_capacity(TCP_PACK_SIZE * 100);
		}
		else {
			crcread_.clear();
			crcwrite_.clear();
		}
	}
}

/*
 * @note 注册回调函数
 */
void TCPClient::RegisterConnect(const CBSlot& slot) {
	if (!cbconn_.empty()) cbconn_.disconnect_all_slots();
	cbconn_.connect(slot);
}

/*
 * @brief 注册回调函数
 */
void TCPClient::RegisterRead(const CBSlot& slot) {
	mutex_lock lck(mtxread_);
	if (!cbread_.empty()) cbread_.disconnect_all_slots();
	cbread_.connect(slot);
}

/*
 * @brief 注册回调函数
 */
void TCPClient::RegisterWrite(const CBSlot& slot) {
	mutex_lock lck(mtxwrite_);
	if (!cbwrite_.empty()) cbwrite_.disconnect_all_slots();
	cbwrite_.connect(slot);
}

int TCPClient::Lookup(char* first) {
	int n = usebuf_ ? crcread_.size() : bytercv_;
	if (!(first && n)) return -1;
	*first = usebuf_ ? crcread_[0] : bufrcv_[0];
	return n;
}

int TCPClient::Lookup(const char* flag, const int len, const int from) {
	if (!flag || len <= 0 || from < 0) return -1;

	int n = usebuf_ ? (crcread_.size() - len) : (bytercv_ - len);
	int i(-1), j, pos;

	if (usebuf_) {
		mutex_lock lck(mtxread_);
		for (pos = from; i != len && pos <= n; ++pos) {
			for (i = 0, j = pos; i < len && flag[i] == crcread_[j]; ++i, ++j);
		}
	}
	else {
		for (pos = from; i != len && pos <= n; ++pos) {
			for (i = 0, j = pos; i < len && flag[i] == bufrcv_[j]; ++i, ++j);
		}
	}
	return i != len ? -1 : (pos - 1);
}

int TCPClient::Read(char* buff, const int len, const int from) {
	if (!buff || len <= 0 || from < 0) return 0;

	mutex_lock lck(mtxread_);
	int n0(from + len), i(0), j;
	if (usebuf_) {
		int n(crcread_.size());
		if (n > n0) n = n0;
		for (j = from; j < n; ++i, ++j) buff[i] = crcread_[j];
		if (i) {
			crcread_.erase_begin(i);
			if (pause_rcv_) {
				pause_rcv_ = (crcread_.capacity() - crcread_.size()) < TCP_PACK_SIZE;
				if (!pause_rcv_) start_read();
			}
		}
	}
	else {
		int n = bytercv_ < n0 ? (bytercv_ - from) : len;
		if (n > 0) {
			memcpy(buff, bufrcv_.get() + from, n);
			bytercv_ -= n;
		}
		i = n > 0 ? n : 0;
	}
	return i;
}

int TCPClient::Write(const char* buff, const int len) {
	if (!buff || len <= 0) return 0;

	mutex_lock lck(mtxwrite_);
	int n;
	if (usebuf_) {
		int n0(crcwrite_.size()), i;
		if ((n = crcwrite_.capacity() - n0) > len) n = len;
		for (i = 0; i < n; ++i) crcwrite_.push_back(buff[i]);
		if (!n0 && n) start_write();
	}
	else {
		n = sock_.write_some(buffer(buff, len));
	}
	return n;
}

void TCPClient::handle_connect(const boost::system::error_code& ec) {
	if (!cbconn_.empty()) cbconn_((const long) this, ec.value());
	if (!ec) {
		sock_.set_option(socket_base::keep_alive(true));
		start_read();
	}
}

void TCPClient::handle_read(const boost::system::error_code& ec, int n) {
	if (!ec){
		mutex_lock lock(mtxread_);
		if (usebuf_) {
			for(int i = 0; i < n; ++i) crcread_.push_back(bufrcv_[i]);
			pause_rcv_ = (crcread_.capacity() - crcread_.size()) < TCP_PACK_SIZE;
		}
		else bytercv_ = n;
	}
	if (!cbread_.empty()) cbread_((const long) this, ec.value());
	if (!(ec || pause_rcv_)) start_read();
}

void TCPClient::handle_write(const boost::system::error_code& ec, int n) {
	if (!ec) {
		mutex_lock lock(mtxwrite_);
		crcwrite_.erase_begin(n);
		if (!cbwrite_.empty()) cbwrite_((const long) this, n);
		start_write();
	}
}

void TCPClient::start_read() {
	if (sock_.is_open()) {
		sock_.async_read_some(buffer(bufrcv_.get(), TCP_PACK_SIZE),
				boost::bind(&TCPClient::handle_read, this,
						placeholders::error, placeholders::bytes_transferred));
	}
}

void TCPClient::start_write() {
	int n(crcwrite_.size());
	if (n) {
		sock_.async_write_some(buffer(crcwrite_.linearize(), n),
				boost::bind(&TCPClient::handle_write, this,
						placeholders::error, placeholders::bytes_transferred));
	}
}

void TCPClient::start() {
	sock_.set_option(socket_base::keep_alive(true));
	start_read();
}

//////////////////////////////////////////////////////////////////////////////
/*---------------- TCPServer: 服务器 ----------------*/
TcpSPtr maketcp_server() {// 工厂函数, 创建TcpSPtr
	return boost::make_shared<TCPServer>();
}

TCPServer::TCPServer()
	: acceptor_(keep_.get_service()) {
}

TCPServer::~TCPServer() {
	boost::system::error_code ec;
	if (acceptor_.is_open()) acceptor_.close(ec);
}

void TCPServer::RegisterAccespt(const CBSlot& slot) {
	if (!cbaccept_.empty()) cbaccept_.disconnect_all_slots();
	cbaccept_.connect(slot);
}

int TCPServer::CreateServer(const uint16_t port) {
	int rslt(0);
	try {
		tcp::endpoint endpoint(tcp::v4(), port);
		acceptor_.open(endpoint.protocol());
		acceptor_.set_option(tcp::acceptor::reuse_address(true));
		acceptor_.bind(endpoint);
		acceptor_.listen(10);
		start_accept();
	}
	catch (boost::system::error_code& ec) {
		rslt = ec.value();
	}
	return rslt;
}

void TCPServer::start_accept() {
	if (acceptor_.is_open()) {
		TcpCPtr client = maketcp_client();
		acceptor_.async_accept(client->GetSocket(),
				boost::bind(&TCPServer::handle_accept, this, client, placeholders::error));
	}
}

void TCPServer::handle_accept(const TcpCPtr& client, const boost::system::error_code& ec) {
	if (!ec) {
		if (!cbaccept_.empty()) cbaccept_(client, (const long) this);
		client->start();
	}

	start_accept();
}
