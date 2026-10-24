CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2

scheduler: scheduler.cpp
	$(CXX) $(CXXFLAGS) -o scheduler scheduler.cpp

clean:
	rm -f scheduler results.csv gantt.csv
