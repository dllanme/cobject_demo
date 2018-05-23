#pragma once

#include "dNETBase.h"
#include <enet/enet.h>

//#pragma comment(lib, "E:/develop2/enet/lib/enet_mt.lib")
//#pragma comment(lib, "E:/develop2/enet/lib/enet_md.lib")

//#pragma comment(lib,"winmm.lib")


////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
#define dENETDEF_CHANNELMAX			2
#define dENETDEF_INSIDECMDSIZE		8

#define dENETDEF_RESPONSEINSIDE		true

#define dENETDEF_SENDTHREADENABLE	false


////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

class TdENETBase
{
public:
	static void ENETStartup() {enet_initialize();}
	static void ENETCleanup() {enet_deinitialize();}

protected:
	static int  CheckInsideCMD(const dBYTE* buff, int size);
	static bool BuildInsideCMD(dBYTE* buff, int size, int cmd = dnetinsidecmdONLINE);

	static int  SocketSendNative(ENetHost* host, ENetPeer* peer, int channel, const dBYTE* buff, dSIZE size, bool reliable = false);
	static int  SocketSendBuff(ENetHost* host, ENetPeer* peer, int channel, const dBYTE* buff, dSIZE size, bool reliable = false);
	static bool SocketSendPack(ENetHost* host, ENetPeer* peer, int channel, const dNETPACK& pack, bool reliable = false);

	static int  SocketReceiveNative(ENetHost* host, dBYTE* buff, dSIZE size, int wait, dSIZE* rec_buff_size);
	static int  SocketReceiveBuff(ENetHost* host, TdDataMatc* matc, dBYTE* buff, dSIZE size, int wait, dSIZE* rec_buff_size = NULL);
	static bool SocketReceivePack(ENetHost* host, TdDataMatc* matc, dNETPACK& pack, int wait);
	static dBUFFITEM* SocketReceiveBI(ENetHost* host, TdDataMatc* matc, int wait);

public:
	TdENETBase();
	virtual ~TdENETBase(){}

	void SetChannelMax(int value)	{m_OptChannelMax = value;}

protected:
	ENetHost*	m_ENETHost;
	int			m_OptChannelMax;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////


class TdENETServerSocketBase : public TdNETBaseServerFrame, public TdENETBase
{
public:
	TdENETServerSocketBase();
	virtual ~TdENETServerSocketBase();

	virtual bool DisConnect(dSIZE Index);

protected:
	virtual bool openPort();
	virtual bool closePort();
	//virtual void onNotifyStopPortThread();
	virtual void onPortThread();
	virtual bool doResponseInsideCommand(dNETCLIENTINFO* client_info, dBYTE* buff, dSIZE size, bool* stop, bool& reply);

	virtual bool sendPack(dNETCLIENTINFO* client_info, const dNETPACK& pack);
	virtual int  sendBuff(dNETCLIENTINFO* client_info, const dBYTE* buff, dSIZE size);
	virtual int  sendNative(dNETCLIENTINFO* client_info, const dBYTE* buff, dSIZE size);

protected:
	void doReleaseClientInfo(dNETCLIENTINFO* client_info);
	void doPortAccept(dNETCONNECTINFO* connect_info, bool* stop);
	void doPortClose(dNETCLIENTINFO* client_info);
	void doPortReceive(dNETCLIENTINFO* client_info, dBYTE* buff, dSIZE size, bool* stop);

	//void doPortReverseSend(dNETCLIENTINFO* client_info, bool* stop);


};

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

//class TdENETClientBase : public TdNETBaseClient, public TdBuffListFrame, public TdENETBase
//{
//public:
//	TdENETClientBase();
//	virtual ~TdENETClientBase();
//
//	virtual bool SendPack(const dNETPACK& pack, bool autoThread = true);
//	virtual int  SendBuff(const dBYTE* buff, dWORD size, bool autoThread = true);
//	virtual int  SendNative(const dBYTE* buff, dWORD size, bool autoThread = true);
//
//	virtual bool ReceivePack(dNETPACK& pack, int wait = dTIMEOUT);
//	virtual int  ReceiveBuff(dBYTE* buff, dWORD size, int wait = dTIMEOUT, dWORD* data_size = NULL);
//	virtual int  ReceiveNative(dBYTE* buff, dWORD size, int wait = dTIMEOUT, dWORD* data_size = NULL);
//
//	void SetSendOnlineInterval(dSIZE value)		{m_SendOnlineInterval = value;}
//	bool IsInsideConnected()					{return m_InsideConnected;}
//
//protected:
//	bool		m_InsideConnected;
//	ENetPeer*	m_Peer;
//	TdDataMatc	m_DataMatc;
//	dSIZE		m_SendOnlineInterval;
//	dSIZE		m_LastOnlineTime;
//
//	//virtual bool checkPortStatus(bool check_receive){return IsConnectedServer() && IsInsideConnected();}
//	virtual bool checkPortStatus(bool check_receive){return m_InsideConnected;}
//	virtual void sendInsideCmdOnline(bool reliable = true);
//	virtual void sendInsideCmdConnect(bool reliable = true);
//	virtual void sendInsideCmdClose(bool reliable = true);
//	virtual bool checkOnline();
//	virtual bool updateSendStatus(bool& autoThread, int packCount = 1);
//};


class TdENETClientSocketBase : public TdNETBaseClientFrame, public TdENETBase
{
public:
	TdENETClientSocketBase();
	virtual ~TdENETClientSocketBase();

protected:
	virtual bool openPort();
	virtual bool closePort();
	virtual void onPortThread();

	virtual bool doCheckOnline();
	virtual int doSendNative(const dBYTE* buff, dSIZE size);
	virtual int doReceiveNative(dBYTE* buff, dSIZE size, int wait = dTIMEOUT, dSIZE* data_size = NULL);
	virtual dBUFFITEM* doReceiveBI(int wait = dTIMEOUT);

	virtual bool checkSendStatus(bool& autoThread, int packCount = 1){
		if(!checkPortStatus(false) || !m_InsideConnected)
			return false;
			//不存在发送线程时，强制使用立即发送
		if(!IsExistPortThread())
			autoThread = false;
		//当接收模式下，或当发送列队中内容时 强制使用线程发送
		else if(IsExistPortThread() && (m_ThreadModeReceive || getBuffListCount() > 0))
			autoThread = true;
		if(autoThread && !CheckAddBuffList(packCount))
			return false;
		//if(!autoThread)
		//	if(!checkPortOnline())
		//		return false;
		return true;
	}

protected:
	ENetPeer* m_Peer;
	
	virtual void sendInsideCmdConnect();


//
//public:
//	void SetLongTimeout(){
//	enet_peer_timeout(m_Peer, 64, 15000, ENET_PEER_TIMEOUT_MAXIMUM);
//
//
//
//	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

class TdENETServerStandard : public TdENETServerSocketBase
{
public:
	TdENETServerStandard(){}
	virtual ~TdENETServerStandard(){}

protected:
    virtual void OnEventAccept(dNETCONNECTINFO* connect_info, bool* stop){
	}
    virtual void OnEventClose(dNETCONNECTINFO* connect_info){
	}

    virtual void OnEventReceiveNative(dNETCONNECTINFO* connect_info, dBYTE* buff, dSIZE size, dBYTE* reply_buff, dSIZE* reply_size, bool* stop){
	}
    virtual void OnEventReceiveBuffer(dNETCONNECTINFO* connect_info, dBYTE* buff, dSIZE size, dBYTE* reply_buff, dSIZE* reply_size, bool* stop){
	}
    virtual void OnEventReceivePack(dNETCONNECTINFO* connect_info, dNETPACK* pack, dNETPACK* reply_pack, bool* reply, bool* stop){
	}

    virtual void OnEventError(dNETCONNECTINFO* connect_info){}

    virtual void OnEventSendData(dSIZE size){
	}
};

class TdENETClientStandard : public TdENETClientSocketBase
{
public:
	TdENETClientStandard(){}
	virtual ~TdENETClientStandard(){}

protected:
    virtual void OnEventReceiveNative(dNETCONNECTINFO* connect_info, dBYTE* buff, dSIZE size, dBYTE* reply_buff, dSIZE* reply_size, bool* stop){
	}
    virtual void OnEventReceiveBuffer(dNETCONNECTINFO* connect_info, dBYTE* buff, dSIZE size, dBYTE* reply_buff, dSIZE* reply_size, bool* stop){
	}
    virtual void OnEventReceivePack(dNETCONNECTINFO* connect_info, dNETPACK* pack, dNETPACK* reply_pack, bool* reply, bool* stop){
	}

    virtual void OnEventError(dNETCONNECTINFO* connect_info){
	}

	virtual void OnEventConnect(bool succeed, bool* reconnect){
	}
	virtual void OnEventDisConnect(bool* reconnect){
	}
    virtual void OnEventSendData(dSIZE size){
	}
};