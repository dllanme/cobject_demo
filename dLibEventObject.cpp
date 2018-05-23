#include "StdAfx.h"
#include "dLibEventObject.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "E:/develop2/libevent/lib/libevent.lib")
#pragma comment(lib, "E:/develop2/libevent/lib/libevent_core.lib")
#pragma comment(lib, "E:/develop2/libevent/lib/libevent_extras.lib")

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////

#define LIBEVENT_FAIL		-1
#define LIBEVENT_ISOK(x)	(0 == (x))
#define BUFFEVENT(x)		((bufferevent*)x)
#define PEER(x)				((dSIZE)x)
#define CHECKBEV(b)			(0 != (b))

int TdLibEventBase::SocketSendNative(dLIBEVENT_BEV bev, const dBYTE* buff, dSIZE size)
{
	//if(!CHECKBEV(bev))
	//	return 0;

	int result = 0;
	//bufferevent_lock(BUFFEVENT(bev));

	//dWRITEMESS("SocketSendNative 1");
	try{
		if(LIBEVENT_ISOK(bufferevent_write(BUFFEVENT(bev), buff, size)))
			result = size;
	}
	catch(...){
	}
	//dWRITEMESS("SocketSendNative 2");

	//bufferevent_unlock(BUFFEVENT(bev));
	return result;
}

bool TdLibEventBase::SocketSendPack(dLIBEVENT_BEV bev, const dNETPACK& pack)
{
	//if(!CHECKBEV(bev))
	//	return false;

	bool result = true;
	//bufferevent_lock(BUFFEVENT(bev));
	//dWRITEMESS("SocketSendPack 1");
	try{
		while(true){
			if(!LIBEVENT_ISOK(bufferevent_write(BUFFEVENT(bev), &pack.cmd, sizeof(dNETCMD))))
				break;
			if(pack.cmd.buffsize > 0){
				if(!LIBEVENT_ISOK(bufferevent_write(BUFFEVENT(bev), pack.buff, pack.cmd.buffsize)))
					break;
			}
			result = true;
			break;
		}
	}
	catch(...){
	}
	//dWRITEMESS("SocketSendPack 2");
	//bufferevent_unlock(BUFFEVENT(bev));
	return result;
}

dSIZE TdLibEventBase::SocketWaitForRecData(dLIBEVENT_BEV bev, dSIZE size, int wait)
{
	//struct evbuffer *input = bufferevent_get_input(BUFFEVENT(bev));
	//if(!input)
	//	return 0;

	//dSIZE rs =  evbuffer_get_length(input);

	//int waitTotal = 0;
	//while(rs < size && waitTotal < wait){
	//	::Sleep(10);
	//	waitTotal += 10;
	//	rs = evbuffer_get_length(input);
	//}
	//return rs;

	

	//if(!CHECKBEV(bev))
	//	return 0;

	//dWRITEMESS("SocketWaitForRecData 1");

	bool is_fail = false;
	dSIZE rs = SocketGetRecSize(bev, &is_fail);
	if(is_fail)
		return 0;
	int waitTotal = 0;

	while(rs < size && waitTotal < wait && !is_fail){
		::Sleep(10);
		waitTotal += 10;
		rs = SocketGetRecSize(bev, &is_fail);
	}
	//dWRITEMESS("SocketWaitForRecData 2");

	return rs;

}

dSIZE TdLibEventBase::SocketGetRecSize(dLIBEVENT_BEV bev, bool* is_fail)
{
	//if(!CHECKBEV(bev))
	//	return 0;
	//dWRITEMESS("SocketGetRecSize 1");

	dSIZE result = 0;

	//bufferevent_lock(BUFFEVENT(bev));
	try{
		struct evbuffer *input = bufferevent_get_input(BUFFEVENT(bev));
		if(input){
			result = evbuffer_get_length(input);
		}
		else{
			if(is_fail)
				*is_fail = true;
		}
	}
	catch(...){
		if(is_fail)
			*is_fail = true;
	}

	//bufferevent_unlock(BUFFEVENT(bev));
	// dWRITEMESS("SocketGetRecSize 2");

	return result;

}

int TdLibEventBase::SocketReceiveNative(dLIBEVENT_BEV bev, dBYTE* buff, dSIZE size, dSIZE* rec_buff_size)
{
	//dWRITEMESS("SocketReceiveNative 1");
	//if(!CHECKBEV(bev))
	//	return 0;
	int result = 0;

	//bufferevent_lock(BUFFEVENT(bev));
	try{
		result = (int)bufferevent_read(BUFFEVENT(bev), buff, size);
	}
	catch(...){
	}

	//bufferevent_unlock(BUFFEVENT(bev));
	//dWRITEMESS("SocketReceiveNative 2");

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////

void TdLibEventServerSocketBase::listener_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *user_data)
{
	dLESERVERCBPARA* para = (dLESERVERCBPARA*)user_data;

	dNETADDR peerAddr(sa);
	struct bufferevent* bev = bufferevent_socket_new(para->base, fd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);
	if(!bev)
		return;	

	evutil_make_socket_nonblocking(fd);  

	bool stop = false;
	para->on_accept(PEER(bev), &peerAddr, &stop);

	if(stop){
		bufferevent_free(bev);
		return;
	}

	bufferevent_setcb(bev, TdLibEventServerSocketBase::conn_read_cb, NULL, TdLibEventServerSocketBase::conn_event_cb, user_data);
	bufferevent_enable(bev, EV_READ);
}

void TdLibEventServerSocketBase::conn_event_cb(struct bufferevent *bev, short events, void *user_data)
{
	//此函数要考虑下将来的多线程操作
	dLESERVERCBPARA* para = (dLESERVERCBPARA*)user_data;

	if (events & BEV_EVENT_EOF) {
		//para->on_close(bev);
	} else if (events & BEV_EVENT_ERROR) {
		//para->on_error(bev, dnetbaseerrorERROR);
	}

	para->on_close(PEER(bev));
	bufferevent_free(bev);
}

void TdLibEventServerSocketBase::conn_read_cb(struct bufferevent *bev, void *user_data)
{
	dLESERVERCBPARA* para = (dLESERVERCBPARA*)user_data;
	para->on_receive(PEER(bev));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////


TdLibEventServerSocketBase::TdLibEventServerSocketBase()
{
	m_listener = NULL;
	m_base = event_base_new();
	evthread_make_base_notifiable(m_base);

	m_cbParameter.base = m_base;
	m_cbParameter.on_accept.connect(this, &TdLibEventServerSocketBase::slotsOnAccept);
	m_cbParameter.on_receive.connect(this, &TdLibEventServerSocketBase::slotsOnReceive);
	m_cbParameter.on_close.connect(this, &TdLibEventServerSocketBase::slotsOnClose);

}

TdLibEventServerSocketBase::~TdLibEventServerSocketBase()
{
	ClosePort();
	event_base_free(m_base);
}

bool TdLibEventServerSocketBase::DisConnect(dSIZE Index)
{
	dNETCLIENTINFO* client_info = (dNETCLIENTINFO*)m_ClientList.At(Index);
	if(!client_info)
		return false;
	
	dLIBEVENT_BEV bev = client_info->peer;
	slotsOnClose(bev);
	bufferevent_free(BUFFEVENT(bev));
	return true;
}

bool TdLibEventServerSocketBase::openPort()
{
	tsClear();

	SOCKADDR_IN addrSrv;  
    if(0 < _tcslen(m_BandAddr.addr))
		addrSrv.sin_addr.S_un.S_addr = inet_addr(m_BandAddr.addr);
	else
		addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);  
    addrSrv.sin_family = AF_INET;  
	addrSrv.sin_port = htons(m_BandAddr.getdefaultport(m_DefaultPort));  

	m_listener = evconnlistener_new_bind(m_base, TdLibEventServerSocketBase::listener_cb, &m_cbParameter,
		LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE, -1, (struct sockaddr*)&addrSrv, sizeof(addrSrv));

	if(!m_listener)
		return false;

	return true;
}

bool TdLibEventServerSocketBase::closePort()
{
	dSIZE count = GetConnectCount();
	for(dSIZE i  = 0; i < count; i++)
		doReleaseClientInfo((dNETCLIENTINFO*)m_ClientList.First());

	evconnlistener_free(m_listener);
	m_listener = NULL;

	return true;
}

void TdLibEventServerSocketBase::onNotifyStopPortThread()
{
	event_base_loopbreak(m_base);
	TdNETBaseServerFrame::onNotifyStopPortThread();
}

void TdLibEventServerSocketBase::onPortThread()
{
	event_base_dispatch(m_base);
}

bool TdLibEventServerSocketBase::sendPack(dNETCLIENTINFO* client_info, const dNETPACK& pack)
{
	tsWriteSendTraffic(pack.get_pack_size());
	return SocketSendPack(client_info->peer, pack);
}

int TdLibEventServerSocketBase::sendNative(dNETCLIENTINFO* client_info, const dBYTE* buff, dSIZE size)
{
	tsWriteSendTraffic(size);
	return SocketSendNative(client_info->peer, buff, size);
}

void TdLibEventServerSocketBase::doReleaseClientInfo(dNETCLIENTINFO* client_info)
{
	//dSHOWMESS(client_info->connect_info.netaddr.getstring());
	m_ClientList.Remove(client_info);
	free(client_info->buff);
	onClose(&client_info->connect_info);
	delete client_info;
}

void TdLibEventServerSocketBase::doPortAccept(dNETCONNECTINFO* connect_info, bool* stop)
{
	if(!doCheckAddr(&connect_info->netaddr)){
		*stop = true;
		return;
	}
	onAccept(connect_info, stop);
}

void TdLibEventServerSocketBase::slotsOnAccept(dLIBEVENT_BEV bev, const dNETADDR* peerAddr, bool* pStop)
{
	dNETCONNECTINFO connect_info;
	connect_info.data = NULL;
	connect_info.online = dCURRENTTIME;
	memcpy(&connect_info.netaddr, peerAddr, sizeof(dNETADDR));
	
	bool stop = false;
	doPortAccept(&connect_info, &stop);
	if(stop)
		return;

	dNETCLIENTINFO* clientInfo = new dNETCLIENTINFO;
	memcpy(&clientInfo->connect_info, &connect_info, sizeof(dNETCONNECTINFO));
	clientInfo->peer = (dSIZE)bev;
	clientInfo->buff = (dBYTE*)malloc(sizeof(dNETCMD));
	clientInfo->recreate_mutex();
	memset(clientInfo->buff, 0, sizeof(dNETCMD));
	m_ClientList.Add(clientInfo);
}

void TdLibEventServerSocketBase::slotsOnReceive(dLIBEVENT_BEV bev)
{
	dNETCLIENTINFO* client_info = (dNETCLIENTINFO*)findClient((dSIZE)bev);
	if(!client_info)
		return;

	dSIZE recSize = SocketGetRecSize(bev);

	tsWriteRecvTraffic(recSize);

	if(0 == recSize)
		return;

	bool recFinish = false;

	dNETPACK pack;
	if(IsNativePack()){
		pack.cmd.buffsize = recSize;
		pack.reallocbuff();
		SocketReceiveNative(bev, pack.buff, pack.cmd.buffsize);
		recFinish = true;
	}
	else{
		dNETCMD* lastCMD = (dNETCMD*)client_info->buff;
		if(lastCMD->buffsize > 0){
			//收完
			if(recSize >= lastCMD->buffsize){
				memcpy(&pack.cmd, lastCMD, sizeof(dNETCMD));
				pack.reallocbuff();
				SocketReceiveNative(bev, pack.buff, pack.cmd.buffsize);
				recFinish = true;
			}
			else{
				//应加入一个机制或变量以控制次数
				//或是不用，因为是TCP发包，有发必有收
			}
		}
		else{
			//首次接PACK包
			if(recSize >= sizeof(dNETCMD)){
				recFinish = true;
				SocketReceiveNative(bev, (dBYTE*)&pack.cmd, sizeof(dNETCMD));
				if(pack.cmd.buffsize > 0){
					//能一次收完
					if(recSize == pack.cmd.buffsize + sizeof(dNETCMD)){
						pack.reallocbuff();
						SocketReceiveNative(bev, pack.buff, pack.cmd.buffsize);
					}
					else{
						recFinish = false;
						memcpy(lastCMD, &pack.cmd, sizeof(dNETCMD));
					}
				}
			}
			
		}
	}

	if(!recFinish){
		return;
	}

	memset(client_info->buff, 0, sizeof(dNETCMD));

	bool stop = false;
	
	if(IsNativePack()){
		dBYTE* replyBuff = (dBYTE*)malloc(dNETDEF_MAXBUFFSIZE);
		dSIZE reply_size = 0;
		onReceiveNative(&client_info->connect_info, pack.buff, pack.cmd.buffsize, replyBuff, &reply_size, &stop);
		if(reply_size > 0){
			if(0 >= SocketSendNative(bev, replyBuff, reply_size))
				stop = true;
			else
				tsWriteSendTraffic(reply_size);

		}
		free(replyBuff);
	}
	else{
		dNETPACK reply_pack;
		bool reply = false;
		onReceivePack(&client_info->connect_info, &pack, &reply_pack, &reply, &stop);
		if(reply){
			if(!SocketSendPack(bev, reply_pack))
				stop = true;
			else
				tsWriteSendTraffic(reply_pack.get_pack_size());
		}
	}

	if(stop){
		slotsOnClose(bev);
		bufferevent_free(BUFFEVENT(bev));
	}

}

void TdLibEventServerSocketBase::slotsOnClose(dLIBEVENT_BEV bev)
{
	dNETCLIENTINFO* client_info = (dNETCLIENTINFO*)findClient((dSIZE)bev);
	if(!client_info)
		return;
	doReleaseClientInfo(client_info);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void TdLibEventClientSocketBase::conn_event_cb(struct bufferevent *bev, short events, void *user_data)
{
	dLESERVERCBPARA* para = (dLESERVERCBPARA*)user_data;
	if		(events & BEV_EVENT_CONNECTED) {
		para->on_connect(true, NULL);
	}
	else if	(events & BEV_EVENT_EOF) {
		para->on_disconnect();
	}
	else if	(events & BEV_EVENT_ERROR) {
		para->on_disconnect();
		//para->on_error(bev, dnetbaseerrorERROR);
	}

	//不需要在此次释放
	//para->on_close(PEER(bev));
	//bufferevent_free(bev);
}

void TdLibEventClientSocketBase::conn_read_cb(struct bufferevent *bev, void *user_data)
{
	dLESERVERCBPARA* para = (dLESERVERCBPARA*)user_data;
	para->on_receive(PEER(bev));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////

TdLibEventClientSocketBase::TdLibEventClientSocketBase()
{
	m_PortThdEnable = false;
	m_ThreadModeConnect = true;

	m_fnOnBevThreadProc.Assign(this, &TdLibEventClientSocketBase::OnBevThreadProc);
	m_BevThdHandle = NULL;
	m_BevThdID = NULL;
	m_BevThdStop = NULL;
	m_BevThdFinish = NULL;

	m_base = event_base_new();
	evthread_make_base_notifiable(m_base);
	
	m_bev = NULL;
	m_stopBev = false;

	m_cbParameter.base = m_base;
	m_cbParameter.on_receive.connect(this, &TdLibEventClientSocketBase::slotsOnReceive);
	m_cbParameter.on_connect.connect(this, &TdLibEventClientSocketBase::slotsOnConnect);
	m_cbParameter.on_disconnect.connect(this, &TdLibEventClientSocketBase::slotsOnDisConnect);
}

TdLibEventClientSocketBase::~TdLibEventClientSocketBase()
{
	ClosePort();
	event_base_free(m_base);
}

bool TdLibEventClientSocketBase::SendPack(const dNETPACK& pack, bool autoThread)
{
	if(!m_InsideConnected)
		return false;
	if(autoThread & IsExistPortThread()){
		dBYTE* buff;
		dSIZE size;
		pack.savetobuff(&buff, size);
		addBuffItem(buff, size, dNETDEF_WAIT_ID);
		free(buff);
		return true;
	}

	bool result = false;
	m_PortMutex.Lock();
	result = SocketSendPack(PEER(m_bev), pack);
	m_PortMutex.UnLock();
	return result;
}

int TdLibEventClientSocketBase::SendNative(const dBYTE* buff, dSIZE size, bool autoThread)
{
	if(!m_InsideConnected)
		return 0;
	if(autoThread & IsExistPortThread()){
		addBuffItem(buff, size, dNETDEF_WAIT_ID);
		return size;
	}
	int result = 0;
	m_PortMutex.Lock();
	result = SocketSendNative(PEER(m_bev), buff, size);
	m_PortMutex.UnLock();
	return result;
}

bool TdLibEventClientSocketBase::ReceivePack(dNETPACK& pack, int wait)
{
	if(!m_InsideConnected)
		return false;
	if(m_ThreadModeReceive)
		return false;

	bool result = false;

	m_PortMutex.Lock();
	while(true){
		dSIZE recSize = SocketWaitForRecData(PEER(m_bev), sizeof(dNETCMD), wait);
		if(sizeof(dNETCMD) != SocketReceiveNative(PEER(m_bev), (dBYTE*)&pack.cmd, sizeof(dNETCMD))){
			pack.cmd.buffsize = 0;	
			break;
		}
		if(pack.cmd.buffsize > 0){
			pack.reallocbuff();
			recSize = SocketWaitForRecData(PEER(m_bev), pack.cmd.buffsize, wait);
			if(recSize < pack.cmd.buffsize)
				break;
			SocketReceiveNative(PEER(m_bev), pack.buff, pack.cmd.buffsize);
		}
		result = true;
		break;
	}
	m_PortMutex.UnLock();

	return result;
}

int TdLibEventClientSocketBase::ReceiveNative(dBYTE* buff, dSIZE size, int wait, dSIZE* data_size)
{
	if(!m_InsideConnected)
		return 0;
	if(m_ThreadModeReceive)
		return 0;

	int result = 0;
	m_PortMutex.Lock();
	while(true){
		dSIZE recSize = SocketWaitForRecData(PEER(m_bev), 1, wait);
		if(data_size)
			*data_size = recSize;
		if(recSize == 0)
			break;
		result = SocketReceiveNative(PEER(m_bev), buff, size);
		break;
	}
	m_PortMutex.UnLock();
	return result;
}

int TdLibEventClientSocketBase::GetRecvDataSize()
{
	int result = 0;
	m_PortMutex.Lock();
	result = SocketGetRecSize(PEER(m_bev));
	m_PortMutex.UnLock();
	return result;
}

bool TdLibEventClientSocketBase::openPort()
{
	memset(&m_lastCmd, 0, sizeof(dNETCMD));
	ClearBuffList();
	m_stopBev = false;
    if(0 == _tcslen(m_ConnectAddr.addr))
		strcpy(m_ConnectAddr.addr, "127.0.0.1");

	m_BevThdStop = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_BevThdFinish = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_BevThdHandle = (HANDLE)_beginthreadex(NULL, 0, m_fnOnBevThreadProc, NULL, CREATE_SUSPENDED, &m_BevThdID);
	::ResumeThread(m_BevThdHandle);
	return true;
}

bool TdLibEventClientSocketBase::closePort()
{
	event_base_loopbreak(m_base);
	SetEvent(m_BevThdStop);
	if(WAIT_OBJECT_0 != WaitForSingleObject(m_BevThdFinish, dNETDEF_THREADWAITEXIT))
		return false;
	CloseHandle(m_BevThdHandle);
	CloseHandle(m_BevThdStop);
	CloseHandle(m_BevThdFinish);
	m_BevThdHandle = NULL;

	ClearBuffList();
	m_stopBev = true;

	return true;
}

void TdLibEventClientSocketBase::onNotifyStopPortThread()
{
	SetEvent(m_BevThdStop);
	m_stopBev = true;
	event_base_loopbreak(m_base);
	TdNETBaseClientFrame::onNotifyStopPortThread();
}

void TdLibEventClientSocketBase::onPortThread()
{
	int waitTime = 1;
	for(;;){
		if(doWaitStopPortThread(waitTime))
			break;

		dBUFFITEM* item = m_BuffList.fetchBuff(0);
		if(item){
			waitTime = item->id;
			dSIZE sendSize = SocketSendNative(PEER(m_bev), item->buff, item->size);
			onSend(sendSize);
			item->release_buff();
			delete item;
		}

	}
}

void TdLibEventClientSocketBase::slotsOnReceive(dLIBEVENT_BEV bev)
{
	if(!m_ThreadModeReceive)
		return;

//注意，此部分未做LOCK

	dSIZE recSize = SocketGetRecSize(bev);
	if(0 == recSize)
		return;


	dNETPACK pack;

	if(IsNativePack()){
		pack.cmd.buffsize = recSize;
		pack.reallocbuff();
		SocketReceiveNative(bev, pack.buff, pack.cmd.buffsize);
		onReceiveNative(pack.buff, pack.cmd.buffsize);
		return;
	}

	
	bool recFinish = false;

	//不是首包
	if(m_lastCmd.buffsize > 0){
		//收完
		if(recSize >= m_lastCmd.buffsize){
			memcpy(&pack.cmd, &m_lastCmd, sizeof(dNETCMD));
			pack.reallocbuff();
			SocketReceiveNative(bev, pack.buff, pack.cmd.buffsize);
			recFinish = true;
		}
	}
	else{
		//首次接PACK包
		if(recSize >= sizeof(dNETCMD)){
			recFinish = true;
			SocketReceiveNative(bev, (dBYTE*)&pack.cmd, sizeof(dNETCMD));
			if(pack.cmd.buffsize > 0){
				//能一次收完
				if(recSize == pack.cmd.buffsize + sizeof(dNETCMD)){
					pack.reallocbuff();
					SocketReceiveNative(bev, pack.buff, pack.cmd.buffsize);
				}
				else{
					recFinish = false;
					memcpy(&m_lastCmd, &pack.cmd, sizeof(dNETCMD));
				}
			}
		}
		
	}

	if(recFinish){
		memset(&m_lastCmd, 0, sizeof(dNETCMD));
		onReceivePack(&pack);
	}

}

void TdLibEventClientSocketBase::slotsOnConnect(bool succeed, bool* reconnect)
{
	m_InsideConnected = succeed;
	onConnect(succeed, reconnect);
}

void TdLibEventClientSocketBase::slotsOnDisConnect()
{
	if(!m_InsideConnected)
		return;
	m_InsideConnected = false;
	bool reconnect = true;
	onDisConnect(&reconnect);
	if(!reconnect)
		onNotifyStopPortThread();
}

unsigned TdLibEventClientSocketBase::OnBevThreadProc(LPVOID lpParam)
{
    SOCKADDR_IN addrSrv;  
	addrSrv.sin_addr.S_un.S_addr = inet_addr(m_ConnectAddr.addr);
    addrSrv.sin_family = AF_INET;  
	addrSrv.sin_port = htons(m_ConnectAddr.getdefaultport(m_DefaultPort));  

	for(;;){
		if(WAIT_OBJECT_0 == WaitForSingleObject(m_BevThdStop, 0))
			break;

		m_bev = bufferevent_socket_new(m_base, -1, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);

		bufferevent_setcb(m_bev, TdLibEventClientSocketBase::conn_read_cb, NULL, TdLibEventClientSocketBase::conn_event_cb, &m_cbParameter);
		bufferevent_enable(m_bev, EV_READ);
		
		int rc = bufferevent_socket_connect(m_bev, (struct sockaddr*)&addrSrv, sizeof(addrSrv));
		if(!LIBEVENT_ISOK(rc))
			break;

		event_base_dispatch(m_base);

		m_InsideConnected = false;
		
		m_PortMutex.Lock();
		bufferevent_free(m_bev);
		m_bev = NULL;
		m_PortMutex.UnLock();

		if(m_stopBev)
			break;

		bool reconnect = true;
		slotsOnConnect(false, &reconnect);

		if(!reconnect)
			break;
	}

	if(m_bev){
		m_PortMutex.Lock();
		bufferevent_free(m_bev);
		m_bev = NULL;
		m_PortMutex.UnLock();
	}

	SetEvent(m_BevThdFinish);
	return 0;
}
