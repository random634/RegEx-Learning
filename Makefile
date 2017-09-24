CXX = g++
CFLAG = -Wall
ReNFA: ReNFA.cpp
	$(CXX) $(CFLAG) -o $@ $<
