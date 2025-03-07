/*
	bif is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	bif is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with bif.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <utils/common.h>
#include "argument.h"
#include <signal.h>
#include <utils/strings.h>
#include "private_key.h"
#include "data_secret_key.h"


namespace agent {
	bool g_enable_ = true;
	Argument::Argument(){}
	Argument::~Argument() {}

	bool Argument::Parse(int argc, char *argv[]) {
		if (argc > 1) {
			std::string s(argv[1]);
            if (s == "--version") {
				std::string sk_hash = utils::String::BinToHexString(agent::HashWrapper::Crypto(agent::GetDataSecuretKey())).substr(0, 2);
#ifdef SVNVERSION
				printf("secure:%s;git:" SVNVERSION "\n", sk_hash.c_str());
#else
				printf("secure:%s\n",sk_hash.c_str());
#endif 
				return true;
			}
			else if (s == "--help") {
				Usage();
				return true;
			}
            else {
				Usage();
				return true;
			}
		}

		return false;
	}

	void Argument::Usage() {
		printf(
			"Usage: agent [OPTIONS]\n"
			"OPTIONS:\n"
			"  --version                                                     display version information\n"
			"  --help                                                        display this help\n"
			);
	}

	void SignalFunc(int32_t code) {
		fprintf(stderr, "Get quit signal(%d)\n", code);
		g_enable_ = false;
	}

	void InstallSignal() {
		signal(SIGHUP, SignalFunc);
		signal(SIGQUIT, SignalFunc);
		signal(SIGINT, SignalFunc);
		signal(SIGTERM, SignalFunc);
        signal(SIGPIPE, SIG_IGN);
        signal(SIGFPE, SIG_IGN);
	}

}
