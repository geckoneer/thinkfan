/********************************************************************
 * parser.cpp: Recursive descent parser for the config.
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

#include "parser.h"
#include "config.h"
#include "error.h"
#include <memory>

namespace thinkfan {

using namespace std;

RegexParser::RegexParser(const string expr, const unsigned int data_idx, bool bol_only, bool match_nl)
: data_idx_(data_idx),
  re_str(expr),
  bol_only_(bol_only && expr[0] == '^')
{
	this->expr_ = (regex_t *) malloc(sizeof(regex_t));
	if (regcomp(this->expr_, expr.c_str(), REG_EXTENDED | (match_nl ? 0 : REG_NEWLINE))) {
		throw ParserMisdefinition();
	}
}


RegexParser::~RegexParser()
{
	regfree(expr_);
	free(expr_);
}


std::string *RegexParser::_parse(const char *&input) const
{
	regmatch_t matches[data_idx_ + 1];
	std::string *rv = nullptr;

	int err;
	if (!(err = regexec(this->expr_, input, this->data_idx_ + 1, matches, 0))) {
		int so = matches[data_idx_].rm_so;
		int eo = matches[data_idx_].rm_eo;
		if (so != -1 && (!bol_only_ || matches[0].rm_so == 0)) {
			rv = new std::string(input, so, eo - so);
			input += matches->rm_eo;
		}
	}
	else if (err == REG_ESPACE) {
		delete rv;
		throw ParserOOM();
	}
	return rv;
}


KeywordParser::KeywordParser(const string keyword)
: RegexParser("^[[:space:]]*" + keyword + "[[:space:]]+([^[:space:]]+|\"[^\"]+\")", 1)
{}


FanDriver *FanParser::_parse(const char *&input) const
{
	FanDriver *fan = nullptr;
	unique_ptr<string> path;

	if ((path = unique_ptr<string>(KeywordParser("fan").parse(input))))
		fail(TF_ERR) << ConfigError(MSG_CONF_FAN_DEPRECATED) << flush;
	else if ((path = unique_ptr<string>(KeywordParser("tp_fan").parse(input))))
		fan = new TpFanDriver(*path);
	else if ((path = unique_ptr<string>(KeywordParser("pwm_fan").parse(input))))
		fan = new HwmonFanDriver(*path);

	return fan;
}


SensorDriver *SensorParser::_parse(const char *&input) const
{
	SensorDriver *sensor = nullptr;
	unique_ptr<string> path;

	if ((path = unique_ptr<string>(KeywordParser("sensor").parse(input))))
		log(TF_ERR, TF_ERR) << MSG_CONF_SENSOR_DEPRECATED << flush;
	else if ((path = unique_ptr<string>(KeywordParser("tp_thermal").parse(input))))
		sensor = new TpSensorDriver(*path);
	else if ((path = unique_ptr<string>(KeywordParser("hwmon").parse(input))))
		sensor = new HwmonSensorDriver(*path);
	else if ((path = unique_ptr<string>(KeywordParser("atasmart").parse(input)))) {
#ifdef USE_ATASMART
		sensor = new AtasmartSensorDriver(*path);
#else
		log(TF_ERR, TF_ERR) << MSG_CONF_ATASMART_UNSUPP << flush;
#endif /* USE_ATASMART */
	}
	else if ((path = unique_ptr<string>(KeywordParser("nv_thermal").parse(input)))) {
#ifdef USE_NVML
		sensor = new NvmlSensorDriver(*path);
#else
		log(TF_ERR, TF_ERR) << MSG_CONF_NVML_UNSUPP << flush;
#endif /* USE_ATASMART */
	}

	if (sensor) {
		unique_ptr<string> list_inner;
		if ((list_inner = unique_ptr<string>(BracketParser("(", ")", false).parse(input)))
				|| (list_inner = unique_ptr<string>(BracketParser("{", "}", false).parse(input)))) {
			const char *list_inner_c = list_inner->c_str();
			unique_ptr<vector<int>> correction(TupleParser().parse(list_inner_c));
			if (correction) sensor->set_correction(*correction);
		}
	}

	return sensor;
}


IntListParser::IntListParser()
: int_parser_("^[[:space:]]*([[:digit:]]+)", 1),
  sep_parser_("^([[:space:]]*,)|([[:space:]]+)", 0)
{}

vector<int> *IntListParser::_parse(const char *&input) const
{
	vector<int> *rv = new vector<int>();
	string *s_int;

	while ((s_int = int_parser_.parse(input))) {
		const char *cs_int = s_int->c_str();
		rv->push_back(strtol(cs_int, nullptr, 0));
		delete s_int;

		string *s_sep;
		if (!(s_sep = sep_parser_.parse(input))) break;
		delete s_sep;
	}

	if (rv->size() > 0) return rv;
	delete rv;
	return nullptr;
}


BracketParser::BracketParser(const string opening, const string closing, bool nl)
: RegexParser((nl ? "^[[:space:]]*\\" : "^[[:blank:]]*\\")
		+ opening + "([^" + opening + closing + "]+)\\" + closing, 1, true, nl)
{}


vector<int> *TupleParser::_parse(const char *&input) const
{
	string *list_inner = nullptr;
	if (!((list_inner = (BracketParser("(", ")").parse(input)))
			|| (list_inner = (BracketParser("{", "}").parse(input))))) {
		return nullptr;
	}
	const char *list_inner_c = list_inner->c_str();

	vector<int> *rv = IntListParser().parse(list_inner_c);
	delete list_inner;
	return rv;
}


SimpleLevel *SimpleLevelParser::_parse(const char *&input) const
{
	SimpleLevel *rv = nullptr;
	string *list_inner = nullptr;
	if (!((list_inner = (BracketParser("(", ")").parse(input)))
			|| (list_inner = (BracketParser("{", "}").parse(input))))) {
		return nullptr;
	}

	vector<int> *ints = nullptr;
	string *lvl_str = nullptr;
	const char *list_inner_c = list_inner->c_str();
	if (!((ints = IntListParser().parse(list_inner_c))
			|| ((lvl_str = BracketParser("\"", "\"").parse(list_inner_c))
					&& (ints = IntListParser().parse(list_inner_c))))) {
		delete lvl_str;
		delete list_inner;
		return nullptr;
	}

	delete list_inner;

	if (lvl_str && ints->size() == 2) rv = new SimpleLevel(*lvl_str, (*ints)[0], (*ints)[1]);
	else if (ints->size() == 3) rv = new SimpleLevel((*ints)[0], (*ints)[1], (*ints)[2]);

	delete lvl_str;
	delete ints;

	return rv;
}


ComplexLevel *ComplexLevelParser::_parse(const char *&input) const
{
	ComplexLevel *rv = nullptr;
	unique_ptr<string> list_inner, lvl_str;
	unique_ptr<vector<int>> lower_lim, upper_lim, lvl_int;

	if (!((list_inner = unique_ptr<string>(BracketParser("(", ")", true).parse(input)))
			|| (list_inner = unique_ptr<string>(BracketParser("{", "}", true).parse(input)))))
		return nullptr;

	const char *list_inner_c = list_inner->c_str();
	if (!((lvl_str = unique_ptr<string>(BracketParser("\"", "\"").parse(list_inner_c)))
			|| ((lvl_int = unique_ptr<vector<int>>(IntListParser().parse(list_inner_c))) && lvl_int->size() == 1)))
		return nullptr;

	TupleParser tp;
	RegexParser sep_parser("^[[:space:]]+|^[[:space:]]*,?[[:space:]]*", 0, true, true);
	sep_parser.match(list_inner_c);
	lower_lim = unique_ptr<vector<int>>(tp.parse(list_inner_c));
	sep_parser.match(list_inner_c);
	upper_lim = unique_ptr<vector<int>>(tp.parse(list_inner_c));
	if (!(lower_lim && upper_lim)) return nullptr;

	if (lvl_str) rv = new ComplexLevel(*lvl_str, *lower_lim, *upper_lim);
	else if (lvl_int) rv = new ComplexLevel(lvl_int->at(0), *lower_lim, *upper_lim);

	return rv;
}


ConfigParser::ConfigParser()
: parser_comment("^[[:space:]]*#.*$", 0)
{}


Config *ConfigParser::_parse(const char *&input) const
{
	// Use smart pointers here since we may cause an exception (rv->add_*()...)
	unique_ptr<Config> rv(new Config());
	RegexParser space_parser_("^[[:space:]]+", 0, true, true);

	bool some_match;
	do {
		some_match = parser_comment.match(input)
				|| space_parser_.match(input)
				|| rv->add_fan(unique_ptr<FanDriver>(parser_fan.parse(input)))
				|| rv->add_sensor(unique_ptr<SensorDriver>(parser_sensor.parse(input)))
				|| rv->add_level(unique_ptr<SimpleLevel>(parser_simple_lvl.parse(input)))
				|| rv->add_level(unique_ptr<ComplexLevel>(parser_complex_lvl.parse(input)));
	} while(*input != 0 && some_match);

	if (*input != 0 && !some_match) return nullptr;

	return rv.release();
}

}





