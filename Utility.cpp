//! MOONWAVE SYSTEM GMBH
//! copyright 2014

#include "Utility.h"

//! \param ms time intervall in milli seconds
//! \return formatted string HH:MM.ss,mmm
QString Utility::formatMilliSeconds(int ms)
{
	int sec = ms / 1000;  ms = ms - (sec * 1000);
	int min = sec / 60;   sec = sec - min * 60;
	int hrs = min / 60;   min = min - hrs * 60;

	bool b = false;
	QString s;
	if (hrs > 0)      { s += QString("%1h ").arg(hrs); b = true; }
	if (min > 0 || b) { s += QString("%1m ").arg(min); b = true; }
	if (sec > 0 || b) { s += QString("%1s ").arg(sec); b = true; }
	if (ms  > 0 || b) { s += QString("%1ms").arg(ms); }

	return s;
}

Utility::Utility()
{
}
