/*
 * Stellarium Telescope Control Plug-in
 * 
 * Copyright (C) 2009 Bogdan Marinov (this file,
 * reusing code written by Johannes Gajdosik in 2006)
 * 
 * Johannes Gajdosik wrote in 2006 the original telescope control feature
 * as a core module of Stellarium. In 2009 it was significantly extended with
 * GUI features and later split as an external plug-in module by Bogdan Marinov.
 * 
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */
 
#include "TelescopeClientDirectAltAzGoTo.hpp"

#include "AltAzGoToConnection.hpp"
#include "AltAzGoToCommand.hpp"
#include "common/LogFile.hpp"
#include "StelCore.hpp"

#include <QRegExp>
#include <QStringList>

TelescopeClientDirectAltAzGoTo::TelescopeClientDirectAltAzGoTo (const QString &name, const QString &parameters, Equinox eq)
	: TelescopeClient(name)
	, time_delay(0)
	, equinox(eq)
	, AltAzGoTo(Q_NULLPTR)
	, long_format_used(false)
	, answers_received(false)
	, last_ra(0)
	, queue_get_position(true)
	, next_pos_time(0)
{
	interpolatedPosition.reset();
	
	//Extract parameters
	//Format: "serial_port_name:time_delay"
	QRegExp paramRx("^([^:]*):(\\d+)$");
	QString serialDeviceName;
	if (paramRx.exactMatch(parameters))
	{
		// This QRegExp only matches valid integers
		serialDeviceName = paramRx.capturedTexts().at(1).trimmed();
		time_delay       = paramRx.capturedTexts().at(2).toInt();
	}
	else
	{
		qWarning() << "ERROR creating TelescopeClientDirectAltAzGoTo: invalid parameters.";
		return;
	}
	
	qDebug() << "TelescopeClientDirectAltAzGoTo parameters: port, time_delay:" << serialDeviceName << time_delay;
	
	//Validation: Time delay
	if (time_delay <= 0 || time_delay > 10000000)
	{
		qWarning() << "ERROR creating TelescopeClientDirectAltAzGoTo: time_delay not valid (should be less than 10000000)";
		return;
	}
	
	//end_of_timeout = -0x8000000000000000LL;
	
	#ifdef Q_OS_WIN
	if(serialDeviceName.right(serialDeviceName.size() - 3).toInt() > 9)
		serialDeviceName = "\\\\.\\" + serialDeviceName;//"\\.\COMxx", not sure if it will work
	#endif //Q_OS_WIN
	
	//Try to establish a connection to the telescope
	AltAzGoTo = new AltAzGoToConnection(*this, qPrintable(serialDeviceName));
	if (AltAzGoTo->isClosed())
	{
		qWarning() << "ERROR creating TelescopeClientDirectAltAzGoTo: cannot open serial device" << serialDeviceName;
		return;
	}
	
	// AltAzGoTo will be deleted in the destructor of Server
	addConnection(AltAzGoTo);
	
	long_format_used = false; // unknown
	last_ra = 0;
	queue_get_position = true;
	next_pos_time = -0x8000000000000000LL;
	answers_received = false;
}

//! queues a GOTO command
/*

THIS IS PROBABLY OF INTEREST


*/
void TelescopeClientDirectAltAzGoTo::telescopeGoto(const Vec3d &j2000Pos, StelObjectP selectObject)
{
	qInfo() << j2000Pos;
	Q_UNUSED(selectObject);

	if (!isConnected())
		return;

	Vec3d position = j2000Pos;
	const StelCore* core = StelApp::getInstance().getCore();
	/*
		probably should disable refraction, not sure how it changes things just yet
	*/
	position = core->j2000ToAltAz(j2000Pos, StelCore::RefractionOff);
	//if (writeBufferEnd - writeBuffer + 20 < (int)sizeof(writeBuffer))
	//TODO: See the else clause, think how to do the same thing
	{
		const double az_signed = atan2(position[1], position[0]);
		//Workaround for the discrepancy in precision between Windows/Linux/PPC Macs and Intel Macs:
		const double az = (az_signed >= 0) ? az_signed : (az_signed + 2.0 * M_PI);
		const double alt = atan2(position[2], std::sqrt(position[0]*position[0]+position[1]*position[1]));
		unsigned int az_int = (unsigned int)floor(0.5 + az*(((unsigned int)0x80000000)/M_PI));
		int alt_int = (int)floor(0.5 + alt*(((unsigned int)0x80000000)/M_PI));

		gotoReceived(az_int, alt_int);
	}
	/*
		else
		{
			qDebug() << "TelescopeTCP(" << name << ")::telescopeGoto: "<< "communication is too slow, I will ignore this command";
		}
	*/
}

void TelescopeClientDirectAltAzGoTo::gotoReceived(unsigned int ra_int, int dec_int)
{
	AltAzGoTo->sendGoto(ra_int, dec_int);
}

//! estimates where the telescope is by interpolation in the stored
//! telescope positions:
Vec3d TelescopeClientDirectAltAzGoTo::getJ2000EquatorialPos(const StelCore*) const
{
	const qint64 now = getNow() - time_delay;
	return interpolatedPosition.get(now);
}

bool TelescopeClientDirectAltAzGoTo::prepareCommunication()
{
	//TODO: Nothing to prepare?
	return true;
}

void TelescopeClientDirectAltAzGoTo::performCommunication()
{
	step(10000);
}

void TelescopeClientDirectAltAzGoTo::communicationResetReceived(void)
{
	long_format_used = false;
	queue_get_position = true;
	next_pos_time = -0x8000000000000000LL;
	
#ifndef QT_NO_DEBUG
	*log_file << Now() << "TelescopeClientDirectAltAzGoTo::communicationResetReceived" << endl;
#endif

	if (answers_received)
	{
		closeAcceptedConnections();
		answers_received = false;
	}
}

//! Called in AltAzGoToCommandGetRa and AltAzGoToCommandGetDec.
void TelescopeClientDirectAltAzGoTo::longFormatUsedReceived(bool long_format)
{
	answers_received = true;
	if (!long_format_used && !long_format)
	{
		AltAzGoTo->sendCommand(new AltAzGoToCommandToggleFormat(*this));
	}
	long_format_used = true;
}

//! Called by AltAzGoToCommandGetRa::readAnswerFromBuffer().
void TelescopeClientDirectAltAzGoTo::raReceived(unsigned int ra_int)
{
	answers_received = true;
	last_ra = ra_int;
#ifndef QT_NO_DEBUG
	*log_file << Now() << "TelescopeClientDirectAltAzGoTo::raReceived: " << ra_int << endl;
#endif
}

//! Called by AltAzGoToCommandGetDec::readAnswerFromBuffer().
//! Should be called after raReceived(), as it contains a call to sendPosition().
void TelescopeClientDirectAltAzGoTo::decReceived(unsigned int dec_int)
{
	answers_received = true;
#ifndef QT_NO_DEBUG
	*log_file << Now() << "TelescopeClientDirectAltAzGoTo::decReceived: " << dec_int << endl;
#endif
	const int AltAzGoTo_status = 0;
	sendPosition(last_ra, dec_int, AltAzGoTo_status);
	queue_get_position = true;
}

void TelescopeClientDirectAltAzGoTo::step(long long int timeout_micros)
{
	long long int now = GetNow();
	if (queue_get_position && now >= next_pos_time)
	{
		AltAzGoTo->sendCommand(new AltAzGoToCommandGetRa(*this));
		AltAzGoTo->sendCommand(new AltAzGoToCommandGetDec(*this));
		queue_get_position = false;
		next_pos_time = now + 500000;// 500000;
	}
	Server::step(timeout_micros);
}

bool TelescopeClientDirectAltAzGoTo::isConnected(void) const
{
	return (!AltAzGoTo->isClosed());//TODO
}

bool TelescopeClientDirectAltAzGoTo::isInitialized(void) const
{
	return (!AltAzGoTo->isClosed());
}

//Merged from Connection::sendPosition() and TelescopeTCP::performReading()
void TelescopeClientDirectAltAzGoTo::sendPosition(unsigned int az_int, int alt_int, int status)
{
	//Server time is "now", because this class is the server
	const qint64 server_micros = (qint64) getNow();
	const double az  =  az_int * (M_PI/(unsigned int)0x80000000);
	const double alt = alt_int * (M_PI/(unsigned int)0x80000000);
	const double calt = cos(alt);
	Vec3d position(cos(az)*calt, sin(az)*calt, sin(alt));
	Vec3d altAzPosition = position;
	const StelCore* core = StelApp::getInstance().getCore();
	//not tested, mess with refraction on/off later
	Vec3d j2000Position = core->altAzToJ2000(altAzPosition, StelCore::RefractionOff);
	interpolatedPosition.add(j2000Position, getNow(), server_micros, status);
}
