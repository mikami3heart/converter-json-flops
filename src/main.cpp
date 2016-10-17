// main.cpp : main program file

#include "SurveyFlops.h"

int main(int argc, char *argv[])
{
	std::string path_json;
	std::string path_output;
	std::string path_define;

	if (argc > 1) {
		path_json = argv[1];
	} else {
		path_json = "input.json";
	}

	if (argc > 2) {
		path_output = argv[2];
	} else {
		path_output = "output.txt";
	}

	if (argc > 3) {
		path_define = argv[3];
	} else {
		path_define = "define.json";
	}

	SurveyFlops sf;
	printf("input  file : %s\n", path_json.c_str());
	printf("output file : %s\n", path_output.c_str());
	printf("define file : %s\n\n", path_define.c_str());
	sf.survey(path_json, path_define);
	sf.write(path_output);

	return 0;
}

