/********************************************************************
 * message.h: Logging and error management.
 * (C) 2015, Victor Mataré
 *
 * this file is part of thinkfan. See thinkfan.c for further information.
 *
 * thinkfan is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * thinkfan is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with thinkfan.  If not, see <http://www.gnu.org/licenses/>.
 *
 * ******************************************************************/

#ifndef MESSAGE_H
#define MESSAGE_H

#include <syslog.h>
#include <string>
#include <exception>
#include <memory>

namespace thinkfan {

enum LogLevel {
	TF_ERR = LOG_ERR,
	TF_WRN = LOG_WARNING,
	TF_INF = LOG_NOTICE,
	TF_DBG = LOG_DEBUG
};

class Error;

class Logger {
private:
	Logger();
	static std::unique_ptr<Logger> instance_;
public:
	~Logger();
	void enable_syslog();
	Logger &level(const LogLevel &lvl);
	Logger &flush();
	static Logger &instance();

	Logger &operator << (const std::string &msg);
	Logger &operator << (const unsigned int i);
	Logger &operator << (const int i);
	Logger &operator << (const float &d);
	Logger &operator << (const Error e);
	Logger &operator << (Logger & (*pf_flush)(Logger &));
private:
	bool syslog_;
	LogLevel log_lvl_;
	std::string log_str_;
	std::exception_ptr exception_;
};


Logger &flush(Logger &l);
Logger &fail(LogLevel lvl_insane);
Logger &log(LogLevel lvl_sane, LogLevel lvl_insane);

}

#ifdef USE_ATASMART
#define DND_DISK_HELP \
 "\n -d  Don't read S.M.A.R.T. temperature from sleeping disks"
#else
#define DND_DISK_HELP ""
#endif

#define MSG_USAGE \
 "\nthinkfan " VERSION ": A minimalist fan control program\n" \
 "\nUsage: thinkfan [-hnqzD [-b BIAS] [-c CONFIG] [-s SECONDS] [-p [SECONDS]]]" \
 "\n -h  This help message" \
 "\n -s  Maximum cycle time in seconds (Integer. Default: 5)" \
 "\n -b  Floating point number (-10 to 30) to control rising temperature" \
 "\n     exaggeration (see README). Default: 5.0" \
 "\n -c  Load different configuration file (default: /etc/thinkfan.conf)" \
 "\n -q  Be quiet (report only important events)" \
 "\n -z  Assume we don't have to worry about resuming when using the sysfs" \
 "\n     interface (see README!)" \
 "\n -p  Use the pulsing-fan workaround (for older Thinkpads). Takes an optional" \
 "\n     floating-point argument (0 ~ 10s) as depulsing duration. Default 0.5s." \
 DND_DISK_HELP \
 "\n -D  DANGEROUS mode: Disable all sanity checks. May result in undefined" \
 "\n     behaviour!\n"

#define MSG_FILE_HDR(file, line_count, line) file + ":" + std::to_string(line_count) + ":" + line
#define MSG_T_STAT(tmp_sleeptime, tmax, last_tmax, b_tmax, lvl) \
	"sleeptime=" << tmp_sleeptime << \
	", tmax=" << tmax << \
	", last_tmax=" << last_tmax << \
	", biased_tmax=" << b_tmax << \
	" -> fan=\"" << lvl << "\""
#define MSG_RELOAD_CONF "Received SIGHUP: reloading config..."
#define MSG_SANITY "Sanity checks are on. Exiting."
#define MSG_INSANITY "Sanity checks are off. Continuing."
#define MSG_CONFIG(path) \
 "Config as read from " + path + ":\nFan level\tLow\tHigh"
#define MSG_CONF_ITEM(level, low, high) " " + std::to_string(level) + "\t\t" std::to_string(low) + "\t" + std::to_string(high)
#define MSG_TERM "Cleaning up and resetting fan control."
#define MSG_DEPULSE(delay, time) "Disengaging the fan controller for " \
	<< time <<" seconds every " << delay << " seconds"
#define MSG_SYSFS_SAFE "Using safe but wasteful way of setting PWM value. Check README to know more."
#define MSG_SENSOR_DEFAULT "Using default temperature inputs in " << DEFAULT_SENSOR << "."
#define MSG_RUNNING PID_FILE " already exists. Either thinkfan is " \
	"already running, or it was killed by SIGKILL. If you're sure thinkfan" \
	" is not running, delete " << PID_FILE << " manually."
#define MSG_SENSOR_LOST "A sensor has vanished! Exiting since there's no " \
	"safe way of handling this."


#define MSG_T_GET(file) std::string("Failed to read temperature(s) from ") + file + ": "
#define MSG_T_INVALID(s, d) s << ": Invalid temperature: " << d


#define MSG_FAN_MODOPTS \
 "Module thinkpad_acpi doesn't seem to support fan_control"
#define MSG_FAN_CTRL(fan) __func__ << ": Writing to " + fan + ": "
#define MSG_FAN_INIT(fan) __func__ << ": Initializing fan control in " << fan << ": "
#define MSG_FAN_RESET(fan) __func__ << ": Resetting fan control in " << fan << ": "


#define MSG_OPT_S_15(t) std::to_string(t) + " seconds of not realizing "\
	"rising temperatures may be dangerous!"
#define MSG_OPT_S_1(t) "A sleeptime of " + std::to_string(t) + " seconds doesn't make much " \
 "sense."
#define MSG_OPT_S "option -s requires an int argument!"
#define MSG_OPT_S_INVAL(x) string("invalid argument to option -s: ") + x
#define MSG_OPT_B "bias must be between -10 and 30!"
#define MSG_OPT_B_NOARG "option -b requires an argument!"
#define MSG_OPT_B_INVAL(x) string("invalid argument to option -b: ") + x
#define MSG_OPT_P(x) string("invalid argument to option -p: ") + x


#define MSG_CONF_DEFAULT_FAN "Using default fan control in " DEFAULT_FAN "."
#define MSG_CONF_NOBIAS(t) "You're using simple temperature limits" \
	" without correction values, and your fan will only start at " << t << " °C. This can " \
	"be dangerous for your hard drive."
#define MSG_CONF_NUM_TEMPS(n, i) "You have " << std::to_string(n) << " sensors but your temper" \
	"ature limits only have " << std::to_string(i) << " entries. Excess sensors will be ignored."
#define MSG_CONF_SENSOR_DEPRECATED "The `sensor' keyword is deprecated. " \
	"Please use the `hwmon' or `tp_thermal' keywords instead!"
#define MSG_CONF_FAN_DEPRECATED "Guessing the fan type from the path" \
	" is deprecated. Please use `tp_fan' or `pwm_fan' to make things clear."
#define MSG_CONF_NOFILE "Refusing to run without usable config file!"
#define MSG_CONF_RELOAD_ERR "Error reloading config. Keeping old one."
#define MSG_CONF_NOFAN "Could not find any fan speed settings in" \
	" the config file. Please read AND UNDERSTAND the documentation!"
#define MSG_CONF_LOWHIGH "Your LOWER limit is not lesser than your " \
	"UPPER limit. That doesn't make sense."
#define MSG_CONF_OVERLAP "LOWER limit doesn't overlap with previous UPPER" \
	" limit."
#define MSG_CONF_FAN "Thinkfan can't use more than one fan."
#define MSG_CONF_LVLORDER "Fan levels are not ordered correctly."
#define MSG_CONF_PARSE "Syntax error"
#define MSG_CONF_LVL0 "The LOWER limit of the first fan level cannot con" \
	"tain any values greater than 0!"
#define MSG_CONF_LVLFORMAT(s) "Unrecognized fan level string: " + s
#define MSG_CONF_LCOUNT "The number of limits must either be 1 or equal to the" \
	" number of temperatures"
#define MSG_CONF_LONG_LIMIT "You have configured more temperature limits " \
	"than sensors. That doesn't make sense"
#define MSG_CONF_LIMITLEN "Inconsistent limit length"
#define MSG_CONF_CORRECTION_LEN(path, clen, ntemp) std::string("Sensor ") + path + " has " \
	+ std::to_string(ntemp) + " temperatures," \
	" but you have " + std::to_string(clen) + " correction values for it."
#define MSG_CONF_ATASMART_UNSUPP "S.M.A.R.T support is not compiled in. Recompile with -DUSE_ATASMART or " \
	"contact your distribution's package maintainer."
#define MSG_CONF_NVML_UNSUPP "NVML support is not compiled in. Recompile with -DUSE_NVML or " \
	"contact your distribution's package maintainer."
#define MSG_TEMP_COUNT(t_conf, t_found) "Your config requires at least " << t_conf << " temperatures, " \
	"but only " << t_found << " temperatures were found."



#endif
