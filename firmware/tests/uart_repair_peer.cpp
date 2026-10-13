#include "../peripheral_spi/src/engine.h"
static Peripheral::Engine peer;
extern "C" void repaired_peer_init(uint32_t saved){peer.init(saved);}
extern "C" void repaired_peer_exchange(const uint8_t* input,uint8_t* output,uint32_t now){Proto::Frame q,r;memcpy(q.bytes,input,32);peer.handle(q,r,now);memcpy(output,r.bytes,32);}
