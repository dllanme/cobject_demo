#include "StdAfx.h"
#include "dENETObject.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

#define _ENET_HOST_DESTROY(h)		{if(*h){enet_host_destroy(*h); (*h) = NULL;}}
#define _ENET_HOST_CHECK(h)			(h != NULL)
#define _ENET_HOST_CONNECT(h,a,c)	(enet_host_connect(h, &a, c, 0))
#define _ENET_PEER_RESET(p)			{if(*p){enet_peer_reset(*p); (*p) = NULL;}}
#define _ENET_PEER_CHECK(p)			(p != NULL)
#define _ENET_PEER_DISCONNECT(p)	{if(p) enet_peer_disconnect(p, 0);}
#define _ENET_PEER_TIMEOUT(p,n)		{if(p) enet_peer_timeout(p, ENET_PEER_TIMEOUT_LIMIT, (int)(n/2), n);}


#define _ENET_PEER(p)				((ENetPeer*)p)
#define _ENET_PEERNUM(p)			((dSIZE)p)




#define dENET_WAIT_ID		1
#define dENET_NOWAIT_ID		0
#define dNET_SENDCHANNEL	0
#define dNET_REPLYCHANNEL	1

static dBYTE dENETINSIDECMD[dENETDEF_INSIDECMDSIZE] = {0};
const int dENETINSIDECMD_CHECK_SIZE = dENETDEF_INSIDECMDSIZE - 1;

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////


int TdENETBase::SocketSendNative(ENetHost* host, ENetPeer* peer, int channel, const dBYTE* buff, dSIZE size, bool reliable)
{
	ENetPacket *packet = enet_packet_create(NULL, size, reliable ? ENET_PACKET_FLAG_RELIABLE : ENET_PACKET_FLAG_UNSEQUENCED); //创建包 //ENET_PACKET_FLAG_RELIABLE 
	memcpy(packet->data, buff, size);
	//dWRITEMESS(dFormat("%.2f", (float)size/1024));
	if(0 > enet_peer_send(peer, 0, packet))
		return 0;
	enet_host_flush(host);
	return size;
}

int TdENETBase::SocketSendBuff(ENetHost* host, ENetPeer* peer, int channel, const dBYTE* buff, dSIZE size, bool reliable)
{
	dDATABUFFITEMS dbis;
	TdDataMatc::BuffConvertDBIS(buff, size, dbis);
	int result = 0;
	for(dSIZE i = 0; i < dbis.size(); i++)
		result += SocketSendNative(host, peer, channel, dbis[i].bytes, dbis[i].dbisize(), reliable);
	return result;
}

bool TdENETBase::SocketSendPack(ENetHost* host, ENetPeer* peer, int channel, const dNETPACK& pack, bool reliable)
{
	dDATABUFFITEMS dbis;
	TdDataMatc::PackConvertDBIS(pack, dbis);
	int result = 0;
	for(dSIZE i = 0; i < dbis.size(); i++)
		result += SocketSendNative(host, peer, channel, dbis[i].bytes, dbis[i].dbisize(), reliable);
	//return result == (dDBISIZE * dbis.size());
	return result > 0;
}

int TdENETBase::SocketReceiveNative(ENetHost* host, dBYTE* buff, dSIZE size, int wait, dSIZE* rec_buff_size)
{
	int result = 0;
	ENetEvent event;
	while(enet_host_service(host, &event, wait) > 0){
		if(ENET_EVENT_TYPE_RECEIVE == event.type){
			if(rec_buff_size)
				*rec_buff_size = event.packet->dataLength;
			result = event.packet->dataLength;
			if(result > (int)size)
				result = (int)size;
			memcpy(buff, event.packet->data, result);
			enet_packet_destroy(event.packet);
			break;
		}
		else if(ENET_EVENT_TYPE_DISCONNECT == event.type){
			result = dNULL;
			break;
		}
	}
	return result;
}

int TdENETBase::SocketReceiveBuff(ENetHost* host, TdDataMatc* matc, dBYTE* buff, dSIZE size, int wait, dSIZE* rec_buff_size)
{
	int result = 0;
	dBUFFITEM* bi = SocketReceiveBI(host, matc, wait);
	if(bi){
		if(rec_buff_size)
			*rec_buff_size = bi->size;
		if(size >= bi->size){
			memcpy(buff, bi->buff, bi->size);
			result = bi->size;
		}
	}
	return result;
}

bool TdENETBase::SocketReceivePack(ENetHost* host, TdDataMatc* matc, dNETPACK& pack, int wait)
{
	dBUFFITEM* bi = SocketReceiveBI(host, matc, wait);
	if(bi){
		if(TdDataMatc::BIConvertPack(bi, pack))
			return true;
	}
	return false;
}

dBUFFITEM* TdENETBase::SocketReceiveBI(ENetHost* host, TdDataMatc* matc, int wait)
{
	dBUFFITEM* bi = NULL;
	ENetEvent event;
	while(enet_host_service(host, &event, wait) > 0){
		if(ENET_EVENT_TYPE_RECEIVE == event.type){
			matc->PutBuff(event.packet->data, event.packet->dataLength);
			enet_packet_destroy(event.packet);
			bi = matc->FetchBuffItem();
			if(bi)
				break;
		}
		else if(ENET_EVENT_TYPE_DISCONNECT == event.type)
			break;
	}
	return bi;	
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

TdENETBase::TdENETBase()
{
	m_ENETHost = NULL;
	m_OptChannelMax = dENETDEF_CHANNELMAX;

	static bool isInit = false;
	if(isInit)
		return;
	isInit = true;
	ENETStartup();
	for(int i = 0; i < dENETDEF_INSIDECMDSIZE; i++)
		dENETINSIDECMD[i] = 200 + i;
}

int TdENETBase::CheckInsideCMD(const dBYTE* buff, int size)
{
	if(dENETDEF_INSIDECMDSIZE != size)
		return dnetinsidecmdNONE;
	if(buff[0] != buff[dENETINSIDECMD_CHECK_SIZE])
		return dnetinsidecmdNONE;
	for(int i = 1; i < dENETINSIDECMD_CHECK_SIZE; i++){
		if(buff[i] != dENETINSIDECMD[i])
			return dnetinsidecmdNONE;
	}
	return buff[0];
}

bool TdENETBase::BuildInsideCMD(dBYTE* buff, int size, int cmd)
{
	if(dENETDEF_INSIDECMDSIZE != size)
		return false;
	memcpy(buff, dENETINSIDECMD, dENETDEF_INSIDECMDSIZE);
	buff[0] = cmd;
	buff[dENETINSIDECMD_CHECK_SIZE] = cmd;
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

//TdENETServerBase::TdENETServerBase()
//{
//	m_ResponseInsideCommand = dENETDEF_RESPONSEINSIDE;
//}
//
//TdENETServerBase::~TdENETServerBase()
//{
//}

//bool TdENETServerBase::onResponseInsideCommand(dNETCLIENTINFO* client_info, dBYTE* buff, dSIZE size, bool* stop)
//{
//	if(dENETDEF_INSIDECMDSIZE != size || !m_ResponseInsideCommand)
//		return false;
//
//	switch(CheckInsideCMD(buff, dENETDEF_INSIDECMDSIZE)){
//	case denetinsidecmdCONNECT:{
//		client_info->check_connect = true;
//	}break;
//	case denetinsidecmdONLINE:{
//		//UDT::send(udt_socket, (char*)buff, dUDTDEF_INSIDECMDSIZE, 0);
//	}break;
//	case denetinsidecmdCLOSE:{
//		*stop = true;
//	}break;
//	default: return false;
//	}
//	return true;	
//}


////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

TdENETServerSocketBase::TdENETServerSocketBase()
{
}

TdENETServerSocketBase::~TdENETServerSocketBase()
{
	ClosePort();
}

//bool TdENETServerSocketBase::SendPack(const dNETPACK& pack, int connectIndex)
//{
//	dNETCLIENTINFO* client_info = (dNETCLIENTINFO*)m_ClientList.At(connectIndex);
//	if(!client_info)
//		return false;
//
//	bool result = false;
//	client_info->mutex.Lock();
//	result = SocketSendPack(m_ENETHost, _ENET_PEER(client_info->peer), dNET_REPLYCHANNEL, pack);
//	client_info->mutex.UnLock();
//	return result;
//	//return SocketSendPack(m_ENETHost, _ENET_PEER(client_info->peer), dNET_REPLYCHANNEL, pack);
//}
//
//int TdENETServerSocketBase::SendBuff(const dBYTE* buff, dWORD size, int connectIndex)
//{
//	dNETCLIENTINFO* client_info = (dNETCLIENTINFO*)m_ClientList.At(connectIndex);
//	if(!client_info)
//		return dNULL;
//
//	int result = 0;
//	client_info->mutex.Lock();
//	result = SocketSendBuff(m_ENETHost, _ENET_PEER(client_info->peer), dNET_REPLYCHANNEL, buff, size);
//	client_info->mutex.UnLock();
//	return result;
//	//return SocketSendBuff(m_ENETHost, _ENET_PEER(client_info->peer), dNET_REPLYCHANNEL, buff, size);
//}
//
//int TdENETServerSocketBase::SendNative(const dBYTE* buff, dWORD size, int connectIndex)
//{
//	dNETCLIENTINFO* client_info = (dNETCLIENTINFO*)m_ClientList.At(connectIndex);
//	if(!client_info)
//		return dNULL;
//	
//	int result = 0;
//	client_info->mutex.Lock();
//	result = SocketSendNative(m_ENETHost, _ENET_PEER(client_info->peer), dNET_REPLYCHANNEL, buff, (dWORD)size, true);
//	client_info->mutex.UnLock();
//	return result;
//
//	//return SocketSendNative(m_ENETHost, _ENET_PEER(client_info->peer), dNET_REPLYCHANNEL, buff, (dWORD)size, true);
//}

bool TdENETServerSocketBase::DisConnect(dSIZE Index)
{
	dNETCLIENTINFO* clientInfo = (dNETCLIENTINFO*)m_ClientList.At(Index);
	if(!clientInfo)
		return false;
	
	bool result = false;

	if(!IsExistPortThread()){
		doReleaseClientInfo(clientInfo);
		result = true;
	}
	else if(_ENET_PEER_CHECK(clientInfo->peer))
		_ENET_PEER_DISCONNECT(_ENET_PEER(clientInfo->peer));

	return result;
}

bool TdENETServerSocketBase::openPort()
{
	tsClear();

	ENetAddress address;
	address.host = ENET_HOST_ANY;
	address.port = m_BandAddr.getdefaultport(m_DefaultPort);
	m_ENETHost = enet_host_create(&address /* the address to bind the server host to */, 
                              m_MaxConnect      /* allow up to 32 clients and/or outgoing connections */,
                              m_OptChannelMax      /* allow up to 2 channels to be used, 0 and 1 */,
                              0      /* assume any amount of incoming bandwidth */,
                              0      /* assume any amount of outgoing bandwidth */);
	if(m_ENETHost == NULL)
		return false;
	
	if(0 == m_BandAddr.port)
		m_BandAddr.port = m_DefaultPort;

	return true;
}

bool TdENETServerSocketBase::closePort()
{
	_ENET_HOST_DESTROY(&m_ENETHost);

	//在线程结束的时候，清除资源占用
	dSIZE count = GetConnectCount();
	for(dSIZE i  = 0; i < count; i++)
		doReleaseClientInfo((dNETCLIENTINFO*)m_ClientList.First());


	return true;
}

//void TdENETServerSocketBase::onNotifyStopPortThread()
//{
//	TdNETBaseServerFrame::onNotifyStopPortThread();
//	//_ENET_HOST_DESTROY(&m_ENETHost);
//}

void TdENETServerSocketBase::onPortThread()
{
	m_ClientList.ReCreateMutex(dFormat("denetclient_mutex%d", m_PortThdID));

	TdMutexObject	recMutex(dFormat("denetclient_mutex%d", m_PortThdID));
	dBYTE*			recBuff = (dBYTE*)malloc(dNETBUFFMAXSIZE); 
	dSIZE			recSize;

	dNETCLIENTINFO* clientInfo;
	bool stop = false;

	for(;;){
		if(doWaitStopPortThread(0))
			break;
		
		tsCheckOnSec();

		ENetEvent event;
		if(enet_host_service(m_ENETHost, &event, dNETDEF_READTIME) > 0){
			//while(enet_host_service(m_ENETHost, &event, 1000) > 0)
			//{
			if(event.type == ENET_EVENT_TYPE_CONNECT) //有客户机连接成功
			{
				ENetAddress remote = event.peer->address; //远程地址

				dNETCONNECTINFO connect_info;
				connect_info.data = NULL;
				connect_info.online = dCURRENTTIME;
				enet_address_get_host_ip(&remote, connect_info.netaddr.addr, 256);
				connect_info.netaddr.port = remote.port;
				
				stop = false;
				doPortAccept(&connect_info, &stop);
				if(!stop){
					clientInfo = new dNETCLIENTINFO;
					memcpy(&clientInfo->connect_info, &connect_info, sizeof(dNETCONNECTINFO));
					clientInfo->peer = _ENET_PEERNUM(event.peer);
					clientInfo->buff = (dBYTE*)malloc(dUDPMAXBUFFSIZE);
					clientInfo->mutex.ReCreateMutex(dFormat("peermutex_%d", clientInfo->peer));
					event.peer->data = clientInfo;
					m_ClientList.Add(clientInfo);
					
					_ENET_PEER_TIMEOUT(event.peer, m_Timeout);
					//enet_peer_timeout(event.peer, 64, 15000, ENET_PEER_TIMEOUT_MAXIMUM);
				}
				else
					enet_peer_reset(event.peer);
			}
			else if(event.type == ENET_EVENT_TYPE_RECEIVE) //收到数据
			{
				//recMutex.Lock();
				recSize = event.packet->dataLength;
				if(recSize >= dNETBUFFMAXSIZE)
					recSize = 0;
				memcpy(recBuff, event.packet->data, recSize);
				//recMutex.UnLock();
				
				enet_packet_destroy(event.packet);
			
				tsWriteRecvTraffic(recSize);
				
				stop = false;
				clientInfo = (dNETCLIENTINFO*)event.peer->data;
				if(!clientInfo)
					continue;

				clientInfo->connect_info.online = dCURRENTTIME;
				doPortReceive(clientInfo, recBuff, recSize, &stop);
				if(stop){
					enet_peer_reset(event.peer);
					//手工释放
					doReleaseClientInfo(clientInfo);
					event.peer->data = NULL;
				}

				//if(0 == event.packet->dataLength){
				//	enet_packet_destroy(event.packet);    //注意释放空间
				//	continue;
				//}

				//tsWriteRecvTraffic(event.packet->dataLength);
				//stop = false;
				//clientInfo = (dNETCLIENTINFO*)event.peer->data;
				//if(!clientInfo){
				//	enet_packet_destroy(event.packet);    //注意释放空间
				//	continue;
				//}
				//clientInfo->mutex.Lock();
				//clientInfo->connect_info.online = dCURRENTTIME;
				//doPortReceive(clientInfo, event.packet->data, event.packet->dataLength, &stop);
				//clientInfo->mutex.UnLock();
				//enet_packet_destroy(event.packet);    //注意释放空间
				//if(stop){
				//	enet_peer_reset(event.peer);
				//	//手工释放
				//	doReleaseClientInfo(clientInfo);
				//	event.peer->data = NULL;
				//}
			}
			else if(event.type == ENET_EVENT_TYPE_DISCONNECT) //失去连接
			{
				dDEBUG::WriteDebugFile("ENET_EVENT_TYPE_DISCONNECT_begin", "C:\\_debug.log");
				if(event.peer){
					clientInfo = (dNETCLIENTINFO*)event.peer->data;
					doReleaseClientInfo(clientInfo);
					event.peer->data = NULL;
				}
				dDEBUG::WriteDebugFile("ENET_EVENT_TYPE_DISCONNECT_end", "C:\\_debug.log");
			}
			else{
				//dSHOWMESS(event.type);
				//进行一些检测超时，组包失败之类的操作
			}
		}
		
		if(m_ReverseSendMode){
			//执行反向模式任务
			for(int i = 0; i < 9; i++){
				if(!doRSLFetchBuff())
					break;
			}
		}
	}

	//dWRITEMESS(dFormat("end %s", this->GetBandAddr().getstring()));

	free(recBuff);

	//Sleep(dNETDEF_READTIME);
	////在线程结束的时候，清除资源占用
	//_ENET_HOST_DESTROY(&m_ENETHost);

	//dSIZE count = GetConnectCount();
	//for(dSIZE i  = 0; i < count; i++)
	//	doReleaseClientInfo((dNETCLIENTINFO*)m_ClientList.First());

}

void TdENETServerSocketBase::doReleaseClientInfo(dNETCLIENTINFO* client_info)
{
	m_ClientList.Remove(client_info);
	doPortClose(client_info);
	free(client_info->buff);
	client_info->buff = NULL;
	delete client_info;
}

void TdENETServerSocketBase::doPortAccept(dNETCONNECTINFO* connect_info, bool* stop)
{
	if(!doCheckAddr(&connect_info->netaddr)){
		*stop = true;
		return;
	}
	onAccept(connect_info, stop);
}

void TdENETServerSocketBase::doPortClose(dNETCLIENTINFO* client_info)
{
	onClose(&client_info->connect_info);
}

void TdENETServerSocketBase::doPortReceive(dNETCLIENTINFO* client_info, dBYTE* buff, dSIZE size, bool* stop)
{
	bool reply = false;
	if(doResponseInsideCommand(client_info, buff, size, stop, reply)){
		if(reply){
			//SocketSendNative(m_ENETHost, _ENET_PEER(client_info->peer), dNET_REPLYCHANNEL, buff, (dWORD)size, true);
			sendNative(client_info, buff, size);
		}
		return;
	}

	dSIZE	reply_size;
	int		odd_size = size;
	int		add_size;
	int		offset = 0;
	while(odd_size > 0){
		add_size = min(odd_size, dNETBUFFMAXSIZE);
		if(!client_info->data_matc.PutBuff(&buff[offset], add_size)){
			reply_size = 0;
			onReceiveNative(&client_info->connect_info, &buff[offset], add_size, client_info->buff, &reply_size, stop);
			if(reply_size > 0){
				sendNative(client_info, client_info->buff, reply_size);
				//SocketSendNative(m_ENETHost, _ENET_PEER(client_info->peer), dNET_REPLYCHANNEL, client_info->buff, (dWORD)reply_size);
			}
		}
		offset += add_size;
		odd_size -= add_size;
	}

	
	dNETPACK pack;
	int		 buff_type;
	dNETPACK reply_pack;

	dBUFFITEM* bi = client_info->data_matc.FetchBuffItem();
	while(bi){

		reply = false;
		reply_size = 0;
		reply_pack.clear();
		
		if(client_info->data_matc.BIConvertPack(bi, pack)){
			buff_type = dnetPACK;
			onReceivePack(&client_info->connect_info, &pack, &reply_pack, &reply, stop);
		}
		else{
			buff_type = dnetBUFF;
			onReceiveBuff(&client_info->connect_info, bi->buff, bi->size, client_info->buff, &reply_size, stop);
		}

		if(reply){
			if(dnetPACK == buff_type){
				sendPack(client_info, reply_pack);
				//SocketSendPack(m_ENETHost, _ENET_PEER(client_info->peer), dNET_REPLYCHANNEL, reply_pack);
			}
			else{
				sendBuff(client_info, client_info->buff, reply_size);
				//SocketSendBuff(m_ENETHost, _ENET_PEER(client_info->peer), dNET_REPLYCHANNEL, client_info->buff, (dWORD)reply_size);
			}
		}

		bi = client_info->data_matc.FetchBuffItem();
	}

}

bool TdENETServerSocketBase::doResponseInsideCommand(dNETCLIENTINFO* client_info, dBYTE* buff, dSIZE size, bool* stop, bool& reply)
{
	if(dENETDEF_INSIDECMDSIZE != size || !m_ResponseInsideCommand)
		return false;
	switch(CheckInsideCMD(buff, dENETDEF_INSIDECMDSIZE)){
	case dnetinsidecmdCONNECT:{
		client_info->check_connect = true;
	}break;
	case dnetinsidecmdONLINE:{
		reply = true;
	}break;
	case dnetinsidecmdCLOSE:{
		*stop = true;
	}break;
	default: return false;
	}
	return true;	
}

bool TdENETServerSocketBase::sendPack(dNETCLIENTINFO* client_info, const dNETPACK& pack)
{
	bool result = false;
	client_info->mutex.Lock();
	result = SocketSendPack(m_ENETHost, _ENET_PEER(client_info->peer), dNET_REPLYCHANNEL, pack);
	client_info->mutex.UnLock();
	return result;
}

int TdENETServerSocketBase::sendBuff(dNETCLIENTINFO* client_info, const dBYTE* buff, dSIZE size)
{
	int result = 0;
	client_info->mutex.Lock();
	result = SocketSendBuff(m_ENETHost, _ENET_PEER(client_info->peer), dNET_REPLYCHANNEL, buff, size);
	client_info->mutex.UnLock();
	return result;
}

int TdENETServerSocketBase::sendNative(dNETCLIENTINFO* client_info, const dBYTE* buff, dSIZE size)
{
	int result = 0;
	client_info->mutex.Lock();
	result = SocketSendNative(m_ENETHost, _ENET_PEER(client_info->peer), dNET_REPLYCHANNEL, buff, size, true);
	client_info->mutex.UnLock();
	return result;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

//TdENETClientBase::TdENETClientBase()
//{
//	m_InsideConnected = false;
//	m_Peer = NULL;
//	m_SendOnlineInterval = dNETDEF_ONLINETIME;
//}
//
//TdENETClientBase::~TdENETClientBase()
//{
//	m_DataMatc.ClearAllList();
//}
//
//bool TdENETClientBase::SendPack(const dNETPACK& pack, bool autoThread)
//{
//	if(!checkPortStatus(false))
//		return false;
//
//	int addCount = TdDataMatc::CountOfPackConvertDBIS(pack);
//	if(!updateSendStatus(autoThread, addCount))
//		return false;
//
//	if(!autoThread)
//		if(!checkOnline())
//			return false;
//
//	dDATABUFFITEMS dbis;
//	TdDataMatc::PackConvertDBIS(pack, dbis);
//
//	int result = 0;
//	for(dSIZE i = 0; i < dbis.size(); i++){
//		if(autoThread)
//			addBuffItem(dbis[i].bytes, dDBISIZE, (i == dbis.size() - 1) ? dENET_WAIT_ID : dENET_NOWAIT_ID);
//		else
//			result += SocketSendNative(m_ENETHost, m_Peer, dNET_SENDCHANNEL, dbis[i].bytes, dDBISIZE); 
//	}
//
//	return result == (dDBISIZE * dbis.size());
//	
//}
//
//int TdENETClientBase::SendBuff(const dBYTE* buff, dWORD size, bool autoThread)
//{
//	if(!checkPortStatus(false))
//		return FALSE;
//
//	int addCount = TdDataMatc::CountOfBuffConvertDBIS(size);
//	if(!updateSendStatus(autoThread, addCount))
//		return false;
//
//	if(!autoThread)
//		if(!checkOnline())
//			return false;
//
//	dDATABUFFITEMS dbis;
//	TdDataMatc::BuffConvertDBIS(buff, (dWORD)size, dbis);
//
//	int result = 0;
//	for(dSIZE i = 0; i < dbis.size(); i++){
//		if(autoThread)
//			addBuffItem(dbis[i].bytes, dDBISIZE, (i == dbis.size() - 1) ? dENET_WAIT_ID : dENET_NOWAIT_ID);
//		else
//			result += SocketSendNative(m_ENETHost, m_Peer, dNET_SENDCHANNEL, dbis[i].bytes, dDBISIZE); 
//	}
//
//	return result;
//}
//
//int TdENETClientBase::SendNative(const dBYTE* buff, dWORD size, bool autoThread)
//{
//	if(!checkPortStatus(false))
//		return FALSE;
//
//	if(!updateSendStatus(autoThread, 1))
//		return false;
//
//	if(!autoThread)
//		if(!checkOnline())
//			return false;
//
//	if(!autoThread)
//		return SocketSendNative(m_ENETHost, m_Peer, dNET_SENDCHANNEL, buff, size);
//
//	addBuffItem(buff, size, dENET_WAIT_ID);
//	return 0;
//}
//
//bool TdENETClientBase::ReceivePack(dNETPACK& pack, int wait)
//{
//	if(!checkPortStatus(true))
//		return false;
//	if(IsExistPortThread() && m_ThreadModeReceive)
//		return false;
//	return SocketReceivePack(m_ENETHost, &m_DataMatc, pack, wait);
//}
//
//int TdENETClientBase::ReceiveBuff(dBYTE* buff, dWORD size, int wait, dWORD* data_size)
//{
//	if(!checkPortStatus(true))
//		return FALSE;
//	if(IsExistPortThread() && m_ThreadModeReceive)
//		return FALSE;
//	return SocketReceiveBuff(m_ENETHost, &m_DataMatc, buff, size, wait, data_size);
//}
//
//int TdENETClientBase::ReceiveNative(dBYTE* buff, dWORD size, int wait, dWORD* data_size)
//{
//	if(!checkPortStatus(true))
//		return FALSE;
//	if(IsExistPortThread() && m_ThreadModeReceive)
//		return FALSE;
//	return SocketReceiveNative(m_ENETHost, buff, size, wait, data_size);
//}
//
//void TdENETClientBase::sendInsideCmdOnline(bool reliable)
//{
//	dBYTE cmd[dENETDEF_INSIDECMDSIZE];
//	BuildInsideCMD(cmd, dENETDEF_INSIDECMDSIZE, denetinsidecmdONLINE);
//	SocketSendNative(m_ENETHost, m_Peer, dNET_SENDCHANNEL, cmd, dENETDEF_INSIDECMDSIZE, reliable);
//}
//
//void TdENETClientBase::sendInsideCmdConnect(bool reliable)
//{
//	dBYTE cmd[dENETDEF_INSIDECMDSIZE];
//	BuildInsideCMD(cmd, dENETDEF_INSIDECMDSIZE, denetinsidecmdCONNECT);
//	SocketSendNative(m_ENETHost, m_Peer, dNET_SENDCHANNEL, cmd, dENETDEF_INSIDECMDSIZE, reliable);
//}
//
//void TdENETClientBase::sendInsideCmdClose(bool reliable)
//{
//	dBYTE cmd[dENETDEF_INSIDECMDSIZE];
//	BuildInsideCMD(cmd, dENETDEF_INSIDECMDSIZE, denetinsidecmdCLOSE);
//	SocketSendNative(m_ENETHost, m_Peer, dNET_SENDCHANNEL, cmd, dENETDEF_INSIDECMDSIZE, reliable);
//}
//
//bool TdENETClientBase::checkOnline()
//{
//	if(!m_InsideConnected)
//		return false;
//
//	if(0 >= m_SendOnlineInterval || dTICKCOUNT - m_LastOnlineTime < m_SendOnlineInterval)
//		return true;
//
//	sendInsideCmdOnline(true);
//	m_LastOnlineTime = dTICKCOUNT;
//	dBYTE cmd[dENETDEF_INSIDECMDSIZE];
//	return dENETDEF_INSIDECMDSIZE == SocketReceiveNative(m_ENETHost, cmd, dENETDEF_INSIDECMDSIZE, dSECMILL, NULL);
//	//return dENETDEF_INSIDECMDSIZE == SocketReceiveNative(m_ENETHost, cmd, dENETDEF_INSIDECMDSIZE, GetTimeout(), NULL);
//}
//
//bool TdENETClientBase::updateSendStatus(bool& autoThread, int packCount)
//{
//	//不存在发送线程时，强制使用立即发送
//	if(!IsExistPortThread())
//		autoThread = false;
//	//当接收模式下，或当发送列队中内容时 强制使用线程发送
//	else if(IsExistPortThread() && (m_ThreadModeReceive || getBuffListCount() > 0))
//		autoThread = true;
//
//	if(autoThread && !CheckAddBuffList(packCount))
//		return false;
//	return true;
//}
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////


TdENETClientSocketBase::TdENETClientSocketBase()
{
	m_Peer = NULL;
}

TdENETClientSocketBase::~TdENETClientSocketBase()
{
	ClosePort();
}

bool TdENETClientSocketBase::openPort()
{
	ClearBuffList();
	m_DataMatc.ClearAllList();

	m_ENETHost = enet_host_create(NULL /* create a client host */,
            1 /* only allow 1 outgoing connection */,
            m_OptChannelMax /* allow up 2 channels to be used, 0 and 1 */,
            0 /* 56K modem with 56 Kbps downstream bandwidth */,
            0 /* 56K modem with 14 Kbps upstream bandwidth */);
	if(m_ENETHost == NULL)
		return false;

	ENetAddress address;
	enet_address_set_host(&address, m_ConnectAddr.addr);
	address.port = m_ConnectAddr.getdefaultport(m_DefaultPort);
	
	bool result = false;

	if(!m_PortThdEnable || !m_ThreadModeConnect){
		m_Peer = _ENET_HOST_CONNECT(m_ENETHost, address, m_OptChannelMax);
		if(!m_Peer){
			_ENET_HOST_DESTROY(&m_ENETHost);
			return false;
		}
		ENetEvent event;
		if(enet_host_service(m_ENETHost, &event, m_ConnectTimeout) > 0 && event.type == ENET_EVENT_TYPE_CONNECT){
			_ENET_PEER_TIMEOUT(m_Peer, m_Timeout);
			sendInsideCmdConnect();
			result = true;
			m_InsideConnected = true;
			onConnect(true, NULL);
		}
	}
	else
		result = true;

	if(!result){
		_ENET_PEER_RESET(&m_Peer);
		_ENET_HOST_DESTROY(&m_ENETHost);
	}
	else{
		if(0 == m_ConnectAddr.port)
			m_ConnectAddr.port = m_DefaultPort;
	}

	return result;
}

bool TdENETClientSocketBase::closePort()
{
	m_InsideConnected = false;
	if(_ENET_PEER_CHECK(m_Peer)){
		_ENET_PEER_DISCONNECT(m_Peer);

		ENetEvent event;
		while(enet_host_service(m_ENETHost, &event, dNETDEF_DISCONNECTTIME) > 0){
			switch(event.type){
			case ENET_EVENT_TYPE_RECEIVE:{
				enet_packet_destroy(event.packet);
			}break;
			case ENET_EVENT_TYPE_DISCONNECT:{
				break;
			}
			break;
			}
		}
	}


	_ENET_PEER_RESET(&m_Peer);
	_ENET_HOST_DESTROY(&m_ENETHost);

	ClearBuffList();
	return true;
}

void TdENETClientSocketBase::onPortThread()
{

	m_BuffList.ReCreateMutex(dFormat("denetthread_mutex%d", m_PortThdID));

	bool	reconnect		= true;
	bool	is_connectd		= !m_ThreadModeConnect;
	bool	is_disconnect	= false;

	dSIZE	lastTick		= 0;
	dSIZE	curTick			= dTICKCOUNT;
	int		waitTime		= 1;
	int		sendSize;
	dSIZE	waitTotal		= 0;

	dBYTE*  recbuff			= (dBYTE*)malloc(dNETBUFFMAXSIZE);
	int     recsize;

	dSIZE	checkerrTotal	= 0;

	ENetAddress address;
	enet_address_set_host(&address, m_ConnectAddr.addr);
	address.port = m_ConnectAddr.getdefaultport(m_DefaultPort);

	for(;;){
		if(doWaitStopPortThread(waitTime))
			break;

		if(!m_ENETHost)
			break;
		
		if(!is_connectd){
			checkerrTotal = 0;
			waitTime = 0;
			if(!m_Peer){
				m_Peer = _ENET_HOST_CONNECT(m_ENETHost, address, m_OptChannelMax);
				_ENET_PEER_TIMEOUT(m_Peer, m_Timeout);
				waitTotal = 0;
			}
			else if(waitTotal < m_ConnectTimeout){
				waitTotal += dNETDEF_READTIME;
				ENetEvent event;
				if(enet_host_service(m_ENETHost, &event, dNETDEF_READTIME) > 0 && event.type == ENET_EVENT_TYPE_CONNECT){
					sendInsideCmdConnect();
					m_InsideConnected = true;
					is_connectd = true;
					onConnect(is_connectd, &reconnect);
					waitTime = 1;
					waitTotal = m_ConnectTimeout;
					is_connectd = true;
				}
			}
			else{
				_ENET_PEER_RESET(&m_Peer);
				reconnect = true;
				onConnect(is_connectd, &reconnect);
				if(!reconnect)
					break;
				waitTime = dNETDEF_RECONNECTTIME;
			}
			continue;
		}
	
		is_disconnect = false;

		//判断是否断开了
		curTick = dTICKCOUNT;
		//间隔5秒测试一次
		if(curTick - lastTick > dSECMILL * 5){
			lastTick = curTick;
			recsize = doReceiveNative(recbuff, dNETBUFFMAXSIZE, 0);
			if(dNULL >= recsize){
				//dWRITEMESS(dFormat("%d", m_Timeout));
				//dWRITEMESS("FIND DISCONNECT!");
				is_disconnect = true;
			}
		}
		else
			recsize = 0;


		if(!is_disconnect){
			//优先进行发送
			dBUFFITEM* item = m_BuffList.fetchBuff(0);
			if(item){
				waitTime = item->id;
				//sendSize = SocketSendNative(m_ENETHost, m_Peer, dNET_SENDCHANNEL, item->buff, item->size);
				sendSize = doSendNative(item->buff, item->size);
				if(0 >= sendSize){
					//dWRITEMESS(dFMTCURRENT);
					//dWRITEMESS(dFormat("send fail! size:%d send:%d", item->size, sendSize));
					//is_disconnect = true;
				}
				else
					onSend(sendSize);
				item->release_buff();
				delete item;

			}
			else if(m_ThreadModeReceive){
				if(0 >= recsize){
					//recsize = SocketReceiveNative(m_ENETHost, recbuff, dNETBUFFMAXSIZE, dNETDEF_READTIME, NULL);
					recsize = doReceiveNative(recbuff, dNETBUFFMAXSIZE, dNETDEF_READTIME);
				}
				if(0 < recsize){
					//m_LastOnlineTime = dTICKCOUNT;

					if(dnetinsidecmdNONE != CheckInsideCMD(recbuff, recsize)){
						//dWRITEMESS("inside cmd");
					}
					else if(m_DataMatc.PutBuff(recbuff, recsize)){
						dNETPACK pack;
						dBUFFITEM* bi = m_DataMatc.FetchBuffItem();
						//if(!bi)
						//	dWRITEMESS(dFormat("UNKNOW size:%d", recsize));
						while(bi){
							if(m_DataMatc.BIConvertPack(bi, pack))
								onReceivePack(&pack);
							else
								onReceiveBuff(bi->buff, bi->size);
							bi = m_DataMatc.FetchBuffItem();
						}

					}
					else{
						//dWRITEMESS(dFormat("UNKNOW size:%d", recsize));
						onReceiveNative(recbuff, recsize);
					}
				}
				else if(dNULL >= recsize){
					//dWRITEMESS("rec error! find disconnect!");
					is_disconnect = true;
				}
			}
			else
				waitTime = 5;
		}

		if(!m_ThreadModeReceive){
			if(!is_disconnect){
				if(!checkPortOnline())
				{
					//dWRITEMESS("ONLINE CHECK FAIL!");
					checkerrTotal++;
					if(checkerrTotal >= 3)
						is_disconnect = true;
				}
				//if(!doCheckOnline())
				//	is_disconnect = true;
			}
		}

		if(is_disconnect){
			dWRITEMESS("is_disconnect CHECK FAIL!");
			m_InsideConnected = false;
			is_connectd = false;
			//Sleep(1);
			//此处不释放PEER，因为有可能出现关题，转到连接部分时，判断超时后进行释放原来的PEER
			//_ENET_PEER_RESET(&m_Peer);
			reconnect = true;
			onDisConnect(&reconnect);
			if(!reconnect)
				break;
			//waitTime = dNETDEF_RECONNECTTIME;
			//waitTime = 1 * dSECMILL;
		}
	}

	free(recbuff);
}

int TdENETClientSocketBase::doSendNative(const dBYTE* buff, dSIZE size)
{
	int result = 0;
	m_PortMutex.Lock();
	result = SocketSendNative(m_ENETHost, m_Peer, dNET_SENDCHANNEL, buff, size, false);
	m_PortMutex.UnLock();
	return result;
}

int TdENETClientSocketBase::doReceiveNative(dBYTE* buff, dSIZE size, int wait, dSIZE* data_size)
{
	int result = 0;
	m_PortMutex.Lock();
	result = SocketReceiveNative(m_ENETHost, buff, size, wait, data_size);
	m_PortMutex.UnLock();
	return result;
}

dBUFFITEM* TdENETClientSocketBase::doReceiveBI(int wait)
{
	dBUFFITEM* result = NULL;
	m_PortMutex.Lock();
	result = SocketReceiveBI(m_ENETHost, &m_DataMatc, wait);
	m_PortMutex.UnLock();
	return result;
}

bool TdENETClientSocketBase::doCheckOnline()
{
	bool result = false;
	m_PortMutex.Lock();
	dBYTE cmd[dENETDEF_INSIDECMDSIZE];
	BuildInsideCMD(cmd, dENETDEF_INSIDECMDSIZE, dnetinsidecmdONLINE);
	SocketSendNative(m_ENETHost, m_Peer, dNET_SENDCHANNEL, cmd, dENETDEF_INSIDECMDSIZE, true);
	m_LastOnlineTime = dTICKCOUNT;
	result = dENETDEF_INSIDECMDSIZE == SocketReceiveNative(m_ENETHost, cmd, dENETDEF_INSIDECMDSIZE, dSECMILL, NULL);
	m_PortMutex.UnLock();
	return result;
}

void TdENETClientSocketBase::sendInsideCmdConnect()
{
	m_PortMutex.Lock();
	dBYTE cmd[dENETDEF_INSIDECMDSIZE];
	BuildInsideCMD(cmd, dENETDEF_INSIDECMDSIZE, dnetinsidecmdCONNECT);
	SocketSendNative(m_ENETHost, m_Peer, dNET_SENDCHANNEL, cmd, dENETDEF_INSIDECMDSIZE, true);
	m_PortMutex.UnLock();
}
