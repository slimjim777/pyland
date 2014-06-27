# Makefile for graphics tests.

CXX = g++-4.7
CXXFLAGS = -g -Wall -Wextra -O2 -std=c++11
CPPFLAGS = -DDEBUG

SDL_CFLAGS    = $(shell sdl2-config --cflags)
SDL_LDLIBS    = $(shell sdl2-config --libs)

# Taken from hello_triangle example
GLES_CFLAGS   = -I/opt/vc/include -I/opt/vc/include/interface/vmcs_host/linux -I/opt/vc/include/interface/vcos/pthreads
GLES_LDFLAGS  = -L/opt/vc/lib/
GLES_LDLIBS   = -lGLESv2 -lEGL -lbcm_host -lpthread

X11_CFLAGS = $(shell pkg-config x11 --cflags)
X11_LDLIBS = $(shell pkg-config x11 --libs)



all: game_window_test



game_window_test: game_window_test.o game_window.o
	$(CXX) -o game_window_test game_window.o game_window_test.o $(CXXFLAGS) $(SDL_LDLIBS) $(GLES_LDFLAGS) $(GLES_LDLIBS) $(X11_LDLIBS)
game_window_test.o: game_window_test.cpp
	$(CXX) -c -o game_window_test.o game_window_test.cpp $(CXXFLAGS) $(CPPFLAGS) $(SDL_CFLAGS)


game_window.o: game_window.cpp
	$(CXX) -c -o game_window.o game_window.cpp $(CXXFLAGS) $(CPPFLAGS) $(SDL_CFLAGS) $(GLES_CFLAGS) $(X11_CFLAGS)



clean:
	- rm -f game_window_test
	- rm -f game_window_test.o
	- rm -f game_window.o