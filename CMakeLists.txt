set(CMAKE_C_STANDARD 99)
#include_directories(`xcrun --show-sdk-path`/usr/include)

project(Block5)



add_executable(peer peer.c include/sockUtils.h src/sockUtils.c include/message.h src/message.c include/lookup.h src/lookup.c include/hash.h src/hash.c include/packet.h src/packet.c include/uthash.h src/clientStore.c include/clientStore.h src/peerClientStore.c include/peerClientStore.h   include/Fingertable.h src/FingerTable.c)
