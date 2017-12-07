#include <fstream>
#include <iostream>
#include <ostream>
#include "common.hpp"

using std::cout;
using std::endl;
using std::ofstream;
using std::ostream;

int main(int argc, char* argv[]) {
	if (argc != 4) {
		cout << "Usage: " << argv[0] << " topofile messagefile changesfile" << endl;
		exit(1);
	}

	GraphUtil::Type type = string(argv[0]) == "distvec" ? GraphUtil::DISTANCE_VECTOR
                                                      : GraphUtil::LINK_STATE;	
	GraphUtil gu(type, argv[1], argv[3], argv[2]);
	cout << "Graph built" << endl;

	ofstream os("output.txt", ofstream::out);
	if (!os.is_open()) {
		cout << "output.txt not open, exiting" << endl;
		exit(1);
	}

	gu.Route();
	gu.PrintRoutingTables(os);
	gu.SendMessages(os);
	while (gu.HasChange()) {
		cout << "Applying change" << endl;
		gu.ApplyChange();
		gu.Route();
		gu.PrintRoutingTables(os);
		gu.SendMessages(os);
	}

	os.close();

	return 0;
}
