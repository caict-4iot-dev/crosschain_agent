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
#ifndef RANDOM_H
#define RANDOM_H

#include <chrono>
#include <string>

namespace utils {

	int64_t GetPerformanceCounter();
	void MemoryClean(void *ptr, size_t len);
	void RandAddSeed();
	bool GetRandBytes(unsigned char* buf, int num);
	bool GetOSRand(unsigned char *buf, int num);
	bool GetStrongRandBytes(std::string & out);

}
#endif
