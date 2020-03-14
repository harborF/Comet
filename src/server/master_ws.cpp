#include "acl_cpp/http/websocket.hpp"
#include "master_ws.h"
#include "conn_mgr.h"

extern MasterConfigVar g_CfgVar;

MasterWSServlet::MasterWSServlet(MasterWSFiber*f, acl::socket_stream* stream)
	: fiber_(f), HttpServlet(stream)
{
}

MasterWSServlet::~MasterWSServlet(void)
{

}

bool MasterWSServlet::doUnknown(acl::HttpServletRequest&, acl::HttpServletResponse& res)
{
	res.setStatus(400);
	res.setContentType("text/html; charset=");

	if (res.sendHeader() == false)
		return false;

	acl::string buf("<root error='unkown request method' />\r\n");
	(void)res.getOutputStream().write(buf);
	return false;
}

bool MasterWSServlet::doGet(acl::HttpServletRequest& req, acl::HttpServletResponse& res)
{
	acl_msg_error("[%s]in doGet", __func__);
	return doPost(req, res);
}

bool MasterWSServlet::doPing(acl::websocket& in, acl::websocket& out)
{
	unsigned long long len = in.get_frame_payload_len();
	if (len == 0)
		return out.send_frame_pong((const void*)NULL, 0);

	out.reset().set_frame_fin(true)
		.set_frame_opcode(acl::FRAME_PONG)
		.set_frame_payload_len(len);

	char buf[8192];
	while (true)
	{
		int ret = in.read_frame_data(buf, sizeof(buf) - 1);
		if (ret == 0)
			break;
		if (ret < 0) {
			acl_msg_error("[%s]read_frame_data error", __func__);
			return false;
		}

		buf[ret] = 0;
		acl_msg_info("[%s]read: %s", __func__, buf);
		if (out.send_frame_data(buf, ret) == false) {
			acl_msg_error("[%s]send_frame_data error", __func__);
			return false;
		}
	}

	return true;
}

bool MasterWSServlet::doPong(acl::websocket& in, acl::websocket&)
{
	unsigned long long len = in.get_frame_payload_len();
	if (len == 0)
		return true;

	char buf[8192];
	while (true)
	{
		int ret = in.read_frame_data(buf, sizeof(buf) - 1);
		if (ret == 0)
			break;
		if (ret < 0) {
			acl_msg_error("[%s]read_frame_data error", __func__);
			return false;
		}

		buf[ret] = 0;
		acl_msg_info("[%s]read: [%s]", __func__, buf);
	}

	return true;
}

bool MasterWSServlet::doClose(acl::websocket&, acl::websocket&)
{
	return false;
}

bool MasterWSServlet::doWebsocket(acl::HttpServletRequest& req, acl::HttpServletResponse&)
{
	acl::socket_stream& ss = req.getSocketStream();
	acl::websocket in(ss), out(ss);

	while (true)
	{
		if (fiber_->isExit())
			return false;

		if (in.read_frame_head() == false) {
			return false;
		}

		unsigned char opCode = in.get_frame_opcode();
		int ret = false;
		switch (opCode)
		{
		case acl::FRAME_PING:
			ret = doPing(in, out);
			break;
		case acl::FRAME_PONG:
			ret = doPong(in, out);
			break;
		case acl::FRAME_CLOSE:
			ret = doClose(in, out);
			break;
		case acl::FRAME_TEXT:
		case acl::FRAME_BINARY:
			ret = in.read_frame_data(fiber_->buffer_, g_CfgVar.uiMaxReadBuffer - 1);
			if (ret <= 0) {
				acl_msg_error("[%s]read_frame_data error", __func__);
				return false;
			}
			ret = fiber_->handleMsg(fiber_->buffer_, ret);
			break;
		case acl::FRAME_CONTINUATION:
			ret = false;
			break;
		default:
			acl_msg_error("[%s]opcode: 0x%x", __func__, opCode);
			ret = false;
			break;
		}

		if (ret == false)
			return false;
	}

	// XXX: NOT REACHED
	return false;
}

bool MasterWSServlet::doPost(acl::HttpServletRequest&, acl::HttpServletResponse& res)
{
	res.setContentType("text/xml; charset=utf-8")	// 设置响应字符集
		.setContentEncoding(true)		// 设置是否压缩数据
		.setChunkedTransferEncoding(false);	// 采用 chunk 传输方式

	acl_msg_error("[%s]error", __func__);
	acl::string buf("error");

	// 发送 http 响应体，因为设置了 chunk 传输模式，所以需要多调用一次
	// res.write 且两个参数均为 0 以表示 chunk 传输数据结束
	return res.write(buf) && res.write(NULL, 0) && false;
}

//////////////////////////////////////////////////////////////////
MasterWSFiber::MasterWSFiber(MasterThread* t, acl::socket_stream* c)
	:MFiberBase(t, c)
{
	thread_->getConnMgr()->login(this);
	buffer_ = (char*)acl_mymalloc(g_CfgVar.uiMaxReadBuffer);
	acl_assert(buffer_ != NULL);
}

MasterWSFiber::~MasterWSFiber(void)
{
	thread_->getConnMgr()->logout(this);
	acl_myfree(buffer_);
}

void MasterWSFiber::run(void)
{
	acl_msg_info("[%s]fiber-%ld-%d fd %d peer %s",
		__func__, thread_->thread_id(), get_id(),
		client_->sock_handle(), client_->get_peer());

	MasterWSServlet servlet(this, client_);
	servlet.setLocalCharset("utf-8");

	while (true) {
		if (servlet.doRun() == false || isExit())
			break;
	}

	acl_msg_info("[%s]fiber-%ld-%d force-%d close websocket", __func__, thread_->thread_id(), get_id(), bForceExist_);

	client_->close();
	delete client_;
	delete this;
}

int MasterWSFiber::sendMsg(const unsigned int uiCmdId, const Json::Value& jsVal)
{
	Json::FastWriter writer;
	const std::string str = writer.write(jsVal);
	return sendMsg(uiCmdId, str.c_str(), str.length());
}

int MasterWSFiber::sendMsg(const unsigned int uiCmdId, const int code)
{
	acl::string str;
	str.format("{\"code\":%d}", code);
	return sendMsg(uiCmdId, str.c_str(), str.length());
}

int MasterWSFiber::sendMsg(const unsigned int uiCmdId, const char* msg)
{
	return sendMsg(uiCmdId, msg, strlen(msg));
}

int MasterWSFiber::sendMsg(const unsigned int uiCmdId, const void* msg, const unsigned int uiLen)
{
	acl_msg_info("[%s]fiber-%ld-%d member-%u command-%u len-%u",
		__func__, thread_->thread_id(), get_id(),
		uiMemberId_, uiCmdId, uiLen);

	if (isExit())return -1;

	NetPkgHeader stHeader;
	stHeader.usHdrLen = sizeof(NetPkgHeader);
	stHeader.uiCmdId = uiCmdId;
	stHeader.uiSeqId = ++uiCmdSeq_;
	stHeader.uiPkgLen = sizeof(NetPkgHeader) + uiLen;

	acl_fiber_mutex_lock(mutex_);

	CommUtils::writeNetHeader2(buffer_, stHeader);

	acl::websocket out(*client_);
	out.reset().set_frame_fin(true)
		.set_frame_opcode(acl::FRAME_BINARY)
		.set_frame_payload_len(stHeader.uiPkgLen);
	if (out.send_frame_data(buffer_, sizeof(NetPkgHeader)) == false) {
		acl_msg_error("[%s]fiber-%ld-%d member-%u command-%u send_frame_data error",
			__func__, thread_->thread_id(), get_id(),
			uiMemberId_, uiCmdId);
		this->checkTimeOut();

		acl_fiber_mutex_unlock(mutex_);
		return -1;
	}

	if (isExit()) {
		acl_fiber_mutex_unlock(mutex_);
		return -1;
	}

	if (uiLen > 0 && out.send_frame_data(msg, uiLen) == false) {
		acl_msg_error("[%s]fiber-%ld-%d member-%u command-%u send_frame_data error",
			__func__, thread_->thread_id(), get_id(),
			uiMemberId_, uiCmdId);
		this->checkTimeOut();

		acl_fiber_mutex_unlock(mutex_);
		return -1;
	}
	acl_fiber_mutex_unlock(mutex_);

	return 0;
}

bool MasterWSFiber::handleMsg(const char* msg, size_t uiLen)
{
	if (uiLen < sizeof(NetPkgHeader) || bForceExist_) {
		acl_msg_error("[%s]fiber-%ld-%d Failed to get header", __func__, thread_->thread_id(), get_id());
		return false;
	}
	NetPkgHeader stHeader;
	CommUtils::readNetHeader2(msg, stHeader);

	const unsigned int uiDataLen = stHeader.uiPkgLen - stHeader.usHdrLen;
	if (uiDataLen >= g_CfgVar.uiMaxReadBuffer) {
		acl_msg_error("[%s]fiber-%ld-%d Failed to get data len: %u",
			__func__, thread_->thread_id(), get_id(), uiDataLen);
		sendMsg(ConstUtils::kCmdKillConnReq, ConstUtils::kErrLength);
		return false;
	}

	WSHandler handler = getWSHandler(stHeader.uiCmdId);
	if (NULL == handler) {
		acl_msg_error("[%s]fiber-%ld-%d member-%u command-%u",
			__func__, thread_->thread_id(), get_id(),
			this->getMemberId(), stHeader.uiCmdId);
		sendMsg(ConstUtils::kCmdKillConnReq, ConstUtils::kErrCmdId);
		return false;
	}

	if (!this->isLogin() && stHeader.uiCmdId > ConstUtils::kCmdAuthReq) {
		acl_msg_error("[%s]fiber-%ld-%d command id:%u not login",
			__func__, thread_->thread_id(), get_id(), stHeader.uiCmdId);
		sendMsg(ConstUtils::kCmdKillConnReq, ConstUtils::kErrNoLogin);
		return false;
	}
	++uiCmdSeq_;
	uiLastHeart_ = time(NULL);

#if USE_SUB_FIBER
	MasterSubWSFiber* subF = new MasterSubWSFiber(this, buffer_ + sizeof(NetPkgHeader), uiDataLen);
	subF->handler_ = handler;
	subF->stHeader_ = stHeader;
	subF->start();
#else
	try {
		if (-1 == handler(stHeader, this, buffer_ + sizeof(NetPkgHeader), uiDataLen)) {
			acl_msg_error("[%s]fiber-%ld-%d member-%u command-%u ret error",
				__func__, thread_->thread_id(), get_id(), this->getMemberId(), stHeader.uiCmdId);
			return false;
		}
	}
	catch (...) {
		acl_msg_error("[%s]fiber-%ld-%d member-%u command-%u exception",
			__func__, thread_->thread_id(), get_id(), this->getMemberId(), stHeader.uiCmdId);
	}
#endif

	return true;
}

///////////////////////////////////////////////////////////////////////////////////
MasterSubWSFiber::MasterSubWSFiber(MasterWSFiber* f, void* buf, int len) :
	ws_fiber_(f), bufLen_(len)
{
	buffer_ = acl_mymalloc(len + 1);
	acl_assert(buffer_ != NULL);
	memcpy(buffer_, buf, len);
}

MasterSubWSFiber::~MasterSubWSFiber()
{
	acl_myfree(buffer_);
}

void MasterSubWSFiber::run(void)
{
	try {
		if (-1 == handler_(stHeader_, ws_fiber_, buffer_, bufLen_)) {
			acl_msg_error("[%s]fiber-%ld-%d member-%u command-%u ret error",
				__func__, ws_fiber_->getThread()->thread_id(), ws_fiber_->get_id(),
				ws_fiber_->getMemberId(), stHeader_.uiCmdId);
		}
	}
	catch (...) {
		acl_msg_error("[%s]fiber-%ld-%d member-%u command-%u exception",
			__func__, ws_fiber_->getThread()->thread_id(), ws_fiber_->get_id(),
			ws_fiber_->getMemberId(), stHeader_.uiCmdId);
	}

	delete this;
}
