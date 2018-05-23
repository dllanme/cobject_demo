#pragma once

#include "dNETBase.h"

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>
#include <event2/thread.h>

//#pragma comment(lib, "E:/develop2/libevent/lib/libevent.lib")

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "E:/develop2/sigslot/sigslot.h"
using namespace sigslot;

typedef dSIZE dLIBEVENT_BEV;

class TdLibEventBase : public has_slots<>
{
public:
	static void LibEventStartup(){
		evthread_use_windows_threads();
	}
	static void LibEventCleanup() {}

protected:
	static int  CheckInsideCMD(const dBYTE* buff, int size);
	static bool BuildInsideCMD(dBYTE* buff, int size, int cmd = dnetinsidecmdONLINE);

	static int  SocketSendNative(dLIBEVENT_BEV bev, const dBYTE* buff, dSIZE size);
	static bool SocketSendPack(dLIBEVENT_BEV bev, const dNETPACK& pack);

	static dSIZE SocketWaitForRecData(dLIBEVENT_BEV bev, dSIZE size, int wait);
	static dSIZE SocketGetRecSize(dLIBEVENT_BEV bev, bool* is_fail = NULL);
	static int  SocketReceiveNative(dLIBEVENT_BEV bev, dBYTE* buff, dSIZE size, dSIZE* rec_buff_size = NULL);
	//static bool SocketReceivePack(dLIBEVENT_BEV bev, dNETPACK& pack, int wait);

public:
	TdLibEventBase(){
		m_base = NULL;

		static bool isInit = false;
		if(isInit)
			return;
		isInit = true;
		LibEventStartup();
	}
	virtual ~TdLibEventBase(){}


protected:
	struct event_base*		m_base;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct tagdLibEventServerCBParameterRec{
	event_base*										base;
	signal3<dLIBEVENT_BEV, const dNETADDR*, bool*>	on_accept;
	signal1<dLIBEVENT_BEV>							on_receive;
	signal1<dLIBEVENT_BEV>							on_close;

	signal2<bool, bool*>							on_connect;
	signal0<>										on_disconnect;


} dLESERVERCBPARA;


class TdLibEventServerSocketBase : public TdNETBaseServerFrame, public TdLibEventBase
{
public:
	static void listener_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *user_data);
	static void conn_event_cb(struct bufferevent *bev, short events, void *user_data);
	static void conn_read_cb(struct bufferevent *bev, void *user_data);

public:
	TdLibEventServerSocketBase();
	virtual ~TdLibEventServerSocketBase();

	virtual bool DisConnect(dSIZE Index);

protected:
	virtual bool openPort();
	virtual bool closePort();
	virtual void onNotifyStopPortThread();
	virtual void onPortThread();
	virtual bool doResponseInsideCommand(dNETCLIENTINFO* client_info, dBYTE* buff, dSIZE size, bool* stop, bool& reply)	{return false;}
	
	virtual bool sendPack(dNETCLIENTINFO* client_info, const dNETPACK& pack);
	virtual int  sendBuff(dNETCLIENTINFO* client_info, const dBYTE* buff, dSIZE size)									{return 0;}
	virtual int  sendNative(dNETCLIENTINFO* client_info, const dBYTE* buff, dSIZE size);

protected:
	void doReleaseClientInfo(dNETCLIENTINFO* client_info);
	void doPortAccept(dNETCONNECTINFO* connect_info, bool* stop);
	//void doPortClose(dNETCLIENTINFO* client_info);
	//void doPortReceive(dNETCLIENTINFO* client_info, dBYTE* buff, dSIZE size, bool* stop);

	struct evconnlistener*	m_listener;
	dLESERVERCBPARA			m_cbParameter;

	void slotsOnAccept(dLIBEVENT_BEV bev, const dNETADDR* peerAddr, bool* pStop);
	void slotsOnReceive(dLIBEVENT_BEV bev);
	void slotsOnClose(dLIBEVENT_BEV bev);

};



////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

class TdLibEventClientSocketBase : public TdNETBaseClientFrame, public TdLibEventBase
{
public:
	static void conn_event_cb(struct bufferevent *bev, short events, void *user_data);
	static void conn_read_cb(struct bufferevent *bev, void *user_data);

public:
	TdLibEventClientSocketBase();
	virtual ~TdLibEventClientSocketBase();

	virtual bool SendPack(const dNETPACK& pack, bool autoThread = true);
	virtual int  SendNative(const dBYTE* buff, dSIZE size, bool autoThread = true);
	virtual bool ReceivePack(dNETPACK& pack, int wait = dTIMEOUT);
	virtual int  ReceiveNative(dBYTE* buff, dSIZE size, int wait = dTIMEOUT, dSIZE* data_size = NULL);

	virtual int SendBuff(const dBYTE* buff, dSIZE size, bool autoThread = true)							{return 0;}
	virtual int ReceiveBuff(dBYTE* buff, dSIZE size, int wait = dTIMEOUT, dSIZE* data_size = NULL)		{return 0;}
	
	virtual int  GetRecvDataSize();

	void SetThreadModeConnect(bool value)		{}
	void SetThreadModeReceive(bool value)		{m_ThreadModeReceive = value;}

protected:
	virtual bool openPort();
	virtual bool closePort();
	virtual void onNotifyStopPortThread();
	virtual void onPortThread();

	virtual bool doCheckOnline()																		{return true;}
	virtual int doSendNative(const dBYTE* buff, dSIZE size)												{return 0;}
	virtual int doReceiveNative(dBYTE* buff, dSIZE size, int wait = dTIMEOUT, dSIZE* data_size = NULL)	{return 0;}
	virtual dBUFFITEM* doReceiveBI(int wait = dTIMEOUT)													{return NULL;}
	virtual bool checkSendStatus(bool& autoThread, int packCount = 1)									{return false;}

protected:
	HANDLE		m_BevThdHandle;
	unsigned	m_BevThdID;
	HANDLE		m_BevThdStop;
	HANDLE		m_BevThdFinish;

	typedef unsigned (TdLibEventClientSocketBase::*CLASSTHREADPROC)(LPVOID);
	TdCallback<dTHREADPROC, TdLibEventClientSocketBase, CLASSTHREADPROC> m_fnOnBevThreadProc;
	unsigned OnBevThreadProc(LPVOID lpParam);


protected:
	bufferevent*			m_bev;
	dLESERVERCBPARA			m_cbParameter;
	bool					m_stopBev;
	dNETCMD					m_lastCmd;

	virtual void sendInsideCmdConnect(){}

	void slotsOnReceive(dLIBEVENT_BEV bev);
	void slotsOnConnect(bool succeed, bool* reconnect);
	void slotsOnDisConnect();


};



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


class TdLibEventServerStandard : public TdLibEventServerSocketBase
{
public:
	TdLibEventServerStandard(){}
	virtual ~TdLibEventServerStandard(){}

protected:
    virtual void OnEventAccept(dNETCONNECTINFO* connect_info, bool* stop){
		//dSHOWMESS("OnEventAccept");
	}
    virtual void OnEventClose(dNETCONNECTINFO* connect_info){
		//dSHOWMESS("OnEventClose");
	}

    virtual void OnEventReceiveNative(dNETCONNECTINFO* connect_info, dBYTE* buff, dSIZE size, dBYTE* reply_buff, dSIZE* reply_size, bool* stop){
		//dSHOWMESS(dFormat("OnEventReceiveNative size:%d", size));
	}
    virtual void OnEventReceivePack(dNETCONNECTINFO* connect_info, dNETPACK* pack, dNETPACK* reply_pack, bool* reply, bool* stop){
		//dSHOWMESS(dFormat("OnEventReceivePack size:%d", pack->cmd.buffsize));
		*reply = true;
		reply_pack->copyfrom(pack);
		//*stop = true;


	}

    virtual void OnEventError(dNETCONNECTINFO* connect_info){}

};



class TdLibEventClientStandard : public TdLibEventClientSocketBase
{
public:
	TdLibEventClientStandard(){}
	virtual ~TdLibEventClientStandard(){}

protected:
    virtual void OnEventReceiveNative(dNETCONNECTINFO* connect_info, dBYTE* buff, dSIZE size, dBYTE* reply_buff, dSIZE* reply_size, bool* stop){
	}
    virtual void OnEventReceivePack(dNETCONNECTINFO* connect_info, dNETPACK* pack, dNETPACK* reply_pack, bool* reply, bool* stop){
		//dSHOWMESS(dFormat("client OnEventReceivePack size:%d", pack->cmd.buffsize));
	}

    virtual void OnEventError(dNETCONNECTINFO* connect_info){
	}

	virtual void OnEventConnect(bool succeed, bool* reconnect){
		//dSHOWMESS(dFormat("OnEventConnect %s", succeed ? "ok" : "fail"));
	}
	virtual void OnEventDisConnect(bool* reconnect){
		//dSHOWMESS("OnEventDisConnect");
	}
    virtual void OnEventSendData(dSIZE size){
		//dSHOWMESS(dFormat("OnEventSendData %d", size));
	}
};