#include "netmessages.h"
#include <tier1/strtools.h>

static char s_text[1024];

bool SVC_VoiceData::WriteToBuffer( bf_write &buffer ) {
	buffer.WriteUBitLong( GetType(), NETMSG_TYPE_BITS );
	buffer.WriteByte( m_nFromClient );
	//buffer.WriteByte( m_bProximity );
	buffer.WriteWord( m_nLength );
	
	return buffer.WriteBits( m_DataOut, m_nLength );
}

bool SVC_VoiceData::ReadFromBuffer( bf_read &buffer ) {
	m_nFromClient = buffer.ReadByte();
	//m_bProximity = !!buffer.ReadByte();
	m_nLength = buffer.ReadWord();

	m_DataIn = buffer;
	return buffer.SeekRelative( m_nLength );
}

const char *SVC_VoiceData::ToString(void) const {
	Q_snprintf(s_text, sizeof(s_text), "%s: client %i, bytes %i", GetName(), m_nFromClient, Bits2Bytes(m_nLength) );
	return s_text;
}