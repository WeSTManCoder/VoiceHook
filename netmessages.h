#ifndef _NETMESSAGES_H
#define _NETMESSAGES_H

#define NETMSG_TYPE_BITS 5
#define svc_VoiceData 15

#include <inetchannelinfo.h>
#include <inetmsghandler.h>
#include <inetmessage.h>
#include <tier1/bitbuf.h>

#define DECLARE_BASE_MESSAGE( msgtype )						\
	public:													\
		bool			ReadFromBuffer( bf_read &buffer );	\
		bool			WriteToBuffer( bf_write &buffer );	\
		const char		*ToString() const;					\
		int				GetType() const { return msgtype; } \
		const char		*GetName() const { return #msgtype;}\

#define DECLARE_SVC_MESSAGE( name )		\
	DECLARE_BASE_MESSAGE( svc_##name );	\
	IServerMessageHandler *m_pMessageHandler;\
	bool Process() { return m_pMessageHandler->Process##name( this ); }\
	
#define Bits2Bytes(b) ((b+7)>>3)

class CNetMessage : public INetMessage {
	public:
		CNetMessage() {	m_bReliable = true;
						m_NetChannel = NULL; }

		virtual ~CNetMessage() {};

		virtual int		GetGroup() const { return INetChannelInfo::GENERIC; }
		INetChannel		*GetNetChannel() const { return m_NetChannel; }
			
		virtual void	SetReliable( bool state) {m_bReliable = state;};
		virtual bool	IsReliable() const { return m_bReliable; };
		virtual void    SetNetChannel(INetChannel * netchan) { m_NetChannel = netchan; }	
		virtual bool	Process() { Assert( 0 ); return false; };	// no handler set

	protected:
		bool				m_bReliable;	// true if message should be send reliable
		INetChannel			*m_NetChannel;	// netchannel this message is from/for
};

class SVC_VoiceData : public CNetMessage {
	DECLARE_SVC_MESSAGE ( VoiceData );

	int	GetGroup() const { return INetChannelInfo::VOICE; }

	SVC_VoiceData() { m_bReliable = false; }

public:	
	int				m_nFromClient;	// client who has spoken
	//bool			m_bProximity;
	int				m_nLength;		// data length in bits
	//uint64			m_xuid;			// X360 player ID ?????????

	bf_read			m_DataIn;
	void			*m_DataOut;
};

#endif