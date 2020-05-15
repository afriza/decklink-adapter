CC=g++
SDK_PATH=${HOME}/BMD_SDK/Linux/include
CFLAGS=-std=c++11 -Wno-multichar -I $(SDK_PATH) -fno-rtti -Wall
LDFLAGS=-lm -ldl -lpthread

decklink-adapter: main.cpp platform.cpp $(SDK_PATH)/DeckLinkAPIDispatch.cpp
	$(CC) -o decklink-adapter main.cpp platform.cpp $(SDK_PATH)/DeckLinkAPIDispatch.cpp $(CFLAGS) $(LDFLAGS)

clean:
	rm -f decklink-adapter
