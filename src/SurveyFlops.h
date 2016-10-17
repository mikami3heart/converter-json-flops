// SurveyFlops.h : SurveyFlops class file

#include <set>
#include "picojson.h"

class SurveyFlops
{
public:
	void survey(const std::string& path_json, const std::string& path_define);
	void write(const std::string& path);

private:
	struct Compare
	{
		bool operator() (picojson::object obj0, picojson::object obj1);
	};

private:
	picojson::object getObjectFromFile(const std::string& path);
	void surveyFile(picojson::object file);
	void surveySubroutine(picojson::object subroutine);
	void surveyChildren(picojson::array children, std::string indent = "");
	void surveyLoop(picojson::object loop, std::string indent = "");
	int getFlocChildren(picojson::array children);
	int getFlocBlock(picojson::object block);
	int getFlocBranch(picojson::object branch);
	int getFloc(picojson::object object);
	void cacheFloc(int floc, std::string indent = "");
	void cache(std::string text = "", int line_num = -4);
	void cacheWarning(std::string text = "", bool undefine = true);
	std::string castString(int number);

private:
	picojson::object define_;
	std::vector<std::string> buffer_;
	std::vector<std::string> buffer_warning_;
	std::set<std::string> functions_undefine_;
};

