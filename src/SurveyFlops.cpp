// SurveyFlops.cpp : SurveyFlops class file

#include <fstream>
#include <sstream>
#include <queue>

#include "SurveyFlops.h"

#define KEY_FILE_ID      "fid"
#define KEY_CHILDREN     "children"
#define KEY_LOCATION     "loc"
#define KEY_PROGRAM_UNIT "pu"
#define KEY_CATEGORY     "cat"
#define KEY_END_LINE     "end_line"
#define KEY_START_LINE   "start_line"
#define KEY_TYPE         "type"
#define KEY_NITER        "niter"
#define KEY_METRICS      "metrics"
#define KEY_IOPS         "iops"
#define KEY_FREFS        "nfref"
#define KEY_REAL         "real"
#define KEY_DOUBLE       "double"
#define KEY_SINGLE       "single"
#define KEY_ABS          "abs"


#define TYPE_SUBROUTINE "subroutine"
#define TYPE_MAIN       "main"
#define TYPE_BLOCK      "block"
#define TYPE_LOOP       "loop"
#define TYPE_BRANCH     "branch"
#define TYPE_PP         "pp"

void SurveyFlops::survey(const std::string& path_json, const std::string& path_define)
{
	picojson::object file = getObjectFromFile(path_json);
	define_ = picojson::object(getObjectFromFile(path_define));
	if (file.empty() || define_.empty()) {
		return;
	}
	surveyFile(file);
}

picojson::object SurveyFlops::getObjectFromFile(const std::string& path)
{
	std::ifstream fstm;
	fstm.open(path.c_str(), std::ios::binary);
	if (!fstm.is_open()) {
		printf("Cannot open file: %s\n", path.c_str());
		return picojson::object();
	}

	std::stringstream sstm;
	picojson::value val;

	sstm << fstm.rdbuf();
	sstm >> val;
	fstm.close();

	std::string err = picojson::get_last_error();
	if (!err.empty()) {
		printf("Cannot read json file: %s\n", path.c_str());
		return picojson::object();
	}

	return val.get<picojson::object>();
}

void SurveyFlops::write(const std::string& path)
{
	bool plot_warning = true;
	std::ofstream ofs;
	ofs.open(path.c_str(), std::ios::trunc | std::ios::binary);
	for (size_t i = 0; i < buffer_.size(); i++) {
		ofs << buffer_[i].c_str() << std::endl;
		printf("%s\n", buffer_[i].c_str());
	}
	if (plot_warning) {
		std::set<std::string>::iterator it;
		for (it = functions_undefine_.begin(); it != functions_undefine_.end(); it++) {
			ofs << "undifine function : " << (*it).c_str() << std::endl;
			printf("undifine function : %s\n", (*it).c_str());
		}
		for (size_t i = 0; i < buffer_warning_.size(); i++) {
			ofs << buffer_warning_[i].c_str() << std::endl;
			printf("%s\n", buffer_warning_[i].c_str());
		}
	}
	ofs.close();
}

bool SurveyFlops::Compare::operator() (picojson::object obj0, picojson::object obj1)
{
	if (!obj0[KEY_START_LINE].is<double>() || 
		!obj1[KEY_START_LINE].is<double>()) {
		return false;
	}
	return (obj0[KEY_START_LINE].get<double>() > obj1[KEY_START_LINE].get<double>());
}

void SurveyFlops::surveyFile(picojson::object file)
{
	if (!file[KEY_FILE_ID].is<std::string>() || 
		!file[KEY_LOCATION].is<std::string>() || 
		!file[KEY_CHILDREN].is<picojson::array>()) {
		return;
	}
	std::string fileId       = file[KEY_FILE_ID].get<std::string>();
	std::string location     = file[KEY_LOCATION].get<std::string>();
	picojson::array children = file[KEY_CHILDREN].get<picojson::array>();

	cache("File ID : " + fileId, -1);
	cache("Source  : " + location, -1);
	cache();
	picojson::object child;
	std::priority_queue<picojson::object, std::vector<picojson::object>, Compare> subroutines;
	for (size_t i = 0; i < children.size(); i++) {
		child = children[i].get<picojson::object>();
		std::string type = "";
		if (child[KEY_TYPE].is<std::string>()) {
			type = child[KEY_TYPE].get<std::string>();
		}
		if (child[KEY_TYPE].get<std::string>() == TYPE_SUBROUTINE) {
			subroutines.push(child);
		} else if (child[KEY_TYPE].get<std::string>() == TYPE_MAIN) {
			subroutines.push(child);
		}
	}
	while (!subroutines.empty()) {
		surveySubroutine(subroutines.top());
		subroutines.pop();
	}
}

void SurveyFlops::surveySubroutine(picojson::object subroutine)
{
	if (!subroutine[KEY_PROGRAM_UNIT].is<std::string>() || 
		!subroutine[KEY_END_LINE].is<double>() || 
		!subroutine[KEY_START_LINE].is<double>() || 
		!subroutine[KEY_CHILDREN].is<picojson::array>()) {
		return;
	}
	std::string program_unit = subroutine[KEY_PROGRAM_UNIT].get<std::string>();
	int end_line   = static_cast<int>(subroutine[KEY_END_LINE].get<double>());
	int start_line = static_cast<int>(subroutine[KEY_START_LINE].get<double>());
	picojson::array children = subroutine[KEY_CHILDREN].get<picojson::array>();

	cache(" " + program_unit, start_line);

	std::string indent = "    ";
	int floc = getFloc(subroutine);
	if (subroutine[KEY_CHILDREN].is<picojson::array>()) {
		picojson::array children = subroutine[KEY_CHILDREN].get<picojson::array>();
		floc += getFlocChildren(children);
		cacheFloc(floc, indent);
		surveyChildren(children, indent);
	} else {
		cacheFloc(floc, indent);
	}
	cache(indent + "~", end_line);
	cache();
	return;
}

int SurveyFlops::getFlocChildren(picojson::array children)
{
	int floc = 0;
	picojson::object child;
	for (size_t i = 0; i < children.size(); i++) {
		child = children[i].get<picojson::object>();
		std::string type = "";
		if (child[KEY_TYPE].is<std::string>()) {
			type = child[KEY_TYPE].get<std::string>();
		}
		if (type == TYPE_BRANCH || type == TYPE_BLOCK || type == TYPE_PP) {
			floc += getFlocBlock(child);
		}
	}
	return floc;
}

void SurveyFlops::surveyLoop(picojson::object loop, std::string indent)
{
	if (!loop[KEY_END_LINE].is<double>() || 
		!loop[KEY_START_LINE].is<double>()) {
		return;
	}
	int end_line   = static_cast<int>(loop[KEY_END_LINE].get<double>());
	int start_line = static_cast<int>(loop[KEY_START_LINE].get<double>());

	if (loop[KEY_NITER].is<std::string>()) {
		std::string niter = loop[KEY_NITER].get<std::string>();
		cache(indent + "+--for     : " + niter, start_line);
	} else {
		char chars[8];
		static int number = 1;
		sprintf(chars, "%d", number++);
		cache(indent + "+--for     : " "r" + chars, start_line);
	}

	indent += "|   ";
	int floc = getFloc(loop);
	if (loop[KEY_CHILDREN].is<picojson::array>()) {
		picojson::array children = loop[KEY_CHILDREN].get<picojson::array>();
		floc += getFlocChildren(children);
		cacheFloc(floc, indent);
		surveyChildren(children, indent);
	} else {
		cacheFloc(floc, indent);
	}
	cache(indent + "~", end_line);
}

int SurveyFlops::getFlocBlock(picojson::object block)
{
	int floc = getFloc(block);
	if (block[KEY_CHILDREN].is<picojson::array>()) {
		picojson::array children = block[KEY_CHILDREN].get<picojson::array>();
		floc += getFlocChildren(children);
	}

	return floc;
}

int SurveyFlops::getFlocBranch(picojson::object branch)
{
	int floc = getFloc(branch);
	if (branch[KEY_CHILDREN].is<picojson::array>()) {
		picojson::array children = branch[KEY_CHILDREN].get<picojson::array>();
		floc += getFlocChildren(children);
	}
	return floc;
}

void SurveyFlops::surveyChildren(picojson::array children, std::string indent)
{
	picojson::object child;
	for (size_t i = 0; i < children.size(); i++) {
		child = children[i].get<picojson::object>();
		std::string type = "";
		if (child[KEY_TYPE].is<std::string>()) {
			type = child[KEY_TYPE].get<std::string>();
		}
		if (type == TYPE_BRANCH || type == TYPE_BLOCK || type == TYPE_PP) {
			if (child[KEY_CHILDREN].is<picojson::array>()) {
				surveyChildren(child[KEY_CHILDREN].get<picojson::array>(), indent);
			}
		}
		if (child[KEY_TYPE].get<std::string>() == TYPE_LOOP) {
			surveyLoop(child, indent);
		}
	}
}

int SurveyFlops::getFloc(picojson::object object)
{
	if (!object[KEY_METRICS].is<picojson::object>()) {
		return 0;
	}
	picojson::object metrics = object[KEY_METRICS].get<picojson::object>();
	int floc = 0;
	picojson::object::iterator it;
	for (it = metrics.begin(); it != metrics.end(); it++) {
		std::string name = (*it).first;
		if (name == KEY_FREFS) continue;
		if (metrics[name].is<double>()) {
			if (metrics[name].get<double>() >= 0) {
				if (define_[name].is<double>()) {
					floc += static_cast<int>(metrics[name].get<double>() * define_[name].get<double>());
				} else {
					floc += static_cast<int>(metrics[name].get<double>());
				}
			} else {
				int start_line = static_cast<int>(object[KEY_START_LINE].get<double>());
				std::string text = name + " is nagative value at " + castString(start_line);
				cacheWarning(text, false);
			}
		}



		if ((*it).second.is<picojson::object>() && define_[name].is<picojson::array>()) {
			picojson::object function = (*it).second.get<picojson::object>();
			picojson::array define = define_[name].get<picojson::array>();
		}
	}


	if (metrics[KEY_FREFS].is<picojson::object>()) {
		picojson::object frefs = metrics[KEY_FREFS].get<picojson::object>();
		picojson::object::iterator it;
		for (it = frefs.begin(); it != frefs.end(); it++) {
			std::string name = (*it).first;
			if ((*it).second.is<picojson::object>() && define_[name].is<picojson::array>()) {
				picojson::object function = (*it).second.get<picojson::object>();
				picojson::array define = define_[name].get<picojson::array>();
				if (function[KEY_SINGLE].is<double>() && define[0].is<double>()) {
					if (function[KEY_SINGLE].get<double>() >= 0) {
						floc += static_cast<int>(function[KEY_SINGLE].get<double>() * define[0].get<double>());
					} else {
						int start_line = static_cast<int>(object[KEY_START_LINE].get<double>());
						std::string text = name + "is nagative value at " + castString(start_line);
						cacheWarning(text, false);
					}
				}
				if (function[KEY_DOUBLE].is<double>() && define[1].is<double>()) {
					if (function[KEY_DOUBLE].get<double>() >= 0) {
						floc += static_cast<int>(function[KEY_DOUBLE].get<double>() * define[1].get<double>());
					} else {
						int start_line = static_cast<int>(object[KEY_START_LINE].get<double>());
						std::string text = name + "is nagative value at " + castString(start_line);
						cacheWarning(text, false);
					}
				}
			} else {
				cacheWarning(name);
			}
		}
	}
	return floc;
}

void SurveyFlops::cacheFloc(int floc, std::string indent)
{
	cache(indent + "| <floc:" + castString(floc) + ">");
}

void SurveyFlops::cache(std::string text, int line_num)
{
	if (line_num == -1) {
		buffer_.push_back(text);
	} else if (line_num == -4) { // default
		buffer_.push_back("    " + text);
	} else {
		buffer_.push_back(castString(line_num) + text);
	}
}

void SurveyFlops::cacheWarning(std::string text, bool undefine)
{
	if (undefine) {
		functions_undefine_.insert(text);
	} else {
		buffer_warning_.push_back(text);
	}
}

std::string SurveyFlops::castString(int number)
{
	char chars[8];
	sprintf(chars, "%4d", number);
	return std::string(chars);
}
