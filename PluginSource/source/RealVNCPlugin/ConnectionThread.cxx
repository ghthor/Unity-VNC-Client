#include "ConnectionThread.h"

#include "VNCClient.h"
#include <network/TcpSocket.h>
#include "CConn.h"
#include <rfb/Hostname.h>
#include <rfb/Exception.h>
#include <rfb/LogWriter.h>

using namespace rfb;
using namespace rfb::win32;

using namespace win32;

static LogWriter vlog("ConnectionThread");


ConnectionThread::ConnectionThread() : Thread("VNC Connection thread")
{

}

void ConnectionThread::QuitAndDelete()
{
	exit = true;
	setDeleteAfterRun();
}

void ConnectionThread::run()
{
	if (m_client == NULL)
		return;
	CConn conn;
	m_client->setConnectionState(Connecting);

	network::Socket* newSocket;
	try 
	{
		newSocket = new network::TcpSocket(m_host, m_port);
		m_client->setConnectionState(Connected);
	}
	catch (rdr::Exception& e) 
	{
		vlog.error("Connection Error : %s", e.str_);
		m_client->setConnectionState(Error);
		return;
	}

	// Run the RFB protocol over the connected socket
	conn.initialise(newSocket, false);
	while (!conn.isClosed()) {
		try {
			conn.getInStream()->check(1, 1);
			conn.processMsg();
		}
		catch (rdr::EndOfStream) {
			if (conn.state() == CConnection::RFBSTATE_NORMAL)
				conn.close();
			else
				conn.close("The connection closed unexpectedly");
		}
		catch (rfb::AuthCancelledException) {
			conn.close();
		}
		catch (rfb::AuthFailureException& e) {
			// Clear the password, in case we auto-reconnect
			conn.close(e.str());
		}
		catch (rdr::Exception& e) {
			conn.close(e.str());
		}
	}

}

void ConnectionThread::Connect(VNCClient * client,  const char* host, int port)
{
	m_host = new char[256];
	strcpy(m_host, host);

	m_port = port;
	m_client = client;
	start();
}


