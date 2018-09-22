/*
The stellarium telescope library helps building
telescope server programs, that can communicate with stellarium
by means of the stellarium TCP telescope protocol.
It also contains smaple server classes (dummy, Meade LX200).

Author and Copyright of this file and of the stellarium telescope library:
Johannes Gajdosik, 2006

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.

*/

#ifndef AltAzGoToCOMMAND_HPP
#define AltAzGoToCOMMAND_HPP

#include <QTextStream>
using namespace std;

class Server;
class TelescopeClientDirectAltAzGoTo;

//! Abstract base class for Meade AltAzGoTo (and compatible) commands.
class AltAzGoToCommand
{
public:
	virtual ~AltAzGoToCommand(void) {}
	virtual bool writeCommandToBuffer(char *&buff, char *end) = 0;
	bool hasBeenWrittenToBuffer(void) const {return has_been_written_to_buffer;}
	virtual int readAnswerFromBuffer(const char *&buff, const char *end) = 0;
	virtual bool needsNoAnswer(void) const {return false;}
	virtual void print(QTextStream &o) const = 0;
	virtual bool isCommandGotoSelected(void) const {return false;}
	virtual bool shortAnswerReceived(void) const {return false;}
	//returns true when reading is finished
	
protected:
	AltAzGoToCommand(Server &server);
	TelescopeClientDirectAltAzGoTo &server;
	bool has_been_written_to_buffer;
};

inline QTextStream &operator<<(QTextStream &o, const AltAzGoToCommand &c)
{
	c.print(o);
	return o;
}

//! Meade AltAzGoTo command: Toggle long or short format.
//! Does not require an answer from the telescope.
class AltAzGoToCommandToggleFormat : public AltAzGoToCommand
{
public:
	AltAzGoToCommandToggleFormat(Server &server) : AltAzGoToCommand(server) {}
	
private:
	bool writeCommandToBuffer(char *&buff, char *end);
	int readAnswerFromBuffer(const char*&, const char*) {return 1;}
	bool needsNoAnswer(void) const {return true;}
	void print(QTextStream &o) const;
};

//! Meade AltAzGoTo command: Stop the current slew.
//! Does not require an answer from the telescope.
class AltAzGoToCommandStopSlew : public AltAzGoToCommand
{
public:
	AltAzGoToCommandStopSlew(Server &server) : AltAzGoToCommand(server) {}
	
private:
	bool writeCommandToBuffer(char *&buff, char *end);
	int readAnswerFromBuffer(const char*&, const char*) {return 1;}
	bool needsNoAnswer(void) const {return true;}
	void print(QTextStream &o) const;
};

//! Meade AltAzGoTo command: Set right ascension.
class AltAzGoToCommandSetSelectedRa : public AltAzGoToCommand
{
public:
	AltAzGoToCommandSetSelectedRa(Server &server, int ra)
	                         : AltAzGoToCommand(server), ra(ra) {}
	bool writeCommandToBuffer(char *&buff, char *end);
	int readAnswerFromBuffer(const char *&buff, const char *end);
	void print(QTextStream &o) const;
	
private:
	const int ra;
};

//! Meade AltAzGoTo command: Set declination.
class AltAzGoToCommandSetSelectedDec : public AltAzGoToCommand
{
public:
	AltAzGoToCommandSetSelectedDec(Server &server,int dec)
	                          : AltAzGoToCommand(server), dec(dec) {}
	bool writeCommandToBuffer(char *&buff, char *end);
	int readAnswerFromBuffer(const char *&buff, const char *end);
	void print(QTextStream &o) const;
	
private:
	const int dec;
};

//! Meade AltAzGoTo command: Slew to the coordinates set before.
class AltAzGoToCommandGotoSelected : public AltAzGoToCommand
{
public:
	AltAzGoToCommandGotoSelected(Server &server)
	                        : AltAzGoToCommand(server), first_byte(256) {}
	bool writeCommandToBuffer(char *&buff, char *end);
	int readAnswerFromBuffer(const char *&buff, const char *end);
	void print(QTextStream &o) const;
	bool isCommandGotoSelected(void) const {return true;}
	bool shortAnswerReceived(void) const {return (first_byte != 256);}
	
private:
	int first_byte;
};

//! Meade AltAzGoTo command: Get the current right ascension.
class AltAzGoToCommandGetRa : public AltAzGoToCommand
{
public:
	AltAzGoToCommandGetRa(Server &server) : AltAzGoToCommand(server) {}
	bool writeCommandToBuffer(char *&buff, char *end);
	int readAnswerFromBuffer(const char *&buff, const char *end);
	void print(QTextStream &o) const;
};

//! Meade AltAzGoTo command: Get the current declination.
class AltAzGoToCommandGetDec : public AltAzGoToCommand
{
public:
	AltAzGoToCommandGetDec(Server &server) : AltAzGoToCommand(server) {}
	bool writeCommandToBuffer(char *&buff, char *end);
	int readAnswerFromBuffer(const char *&buff, const char *end);
	void print(QTextStream &o) const;
};

#endif // AltAzGoToCOMMAND_HPP
