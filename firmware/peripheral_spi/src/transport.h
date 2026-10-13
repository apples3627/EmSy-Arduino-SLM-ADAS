#pragma once
#include "protocol.h"
namespace PeripheralTransport {
struct ReceiveQueue {
 volatile uint8_t head,tail; volatile uint8_t overflow;
 uint8_t frames[3][32];
 ReceiveQueue():head(0),tail(0),overflow(0){}
 uint8_t* incoming(){return frames[head];}
 bool publish(uint8_t length,bool dummy=false){if(length!=32||dummy)return false;uint8_t next=(head+1)%3;if(next==tail){overflow=1;return false;}head=next;return true;}
 bool pop(Proto::Frame& frame){if(tail==head)return false;uint8_t current=tail;memcpy(frame.bytes,frames[current],32);tail=(current+1)%3;return true;}
 void drain(){tail=head;overflow=0;}
};
struct TransmitCursor {
 volatile uint8_t used;
 TransmitCursor():used(0){}
 uint8_t begin(const Proto::Frame& f){used=1;return f.bytes[0];}
 uint8_t next(const Proto::Frame& f){return used<32?f.bytes[used++]:0xff;}
};
struct Receiver {
 ReceiveQueue queue;volatile uint8_t used;
 Receiver():used(0){}
 void begin(){used=0;}
 void byte(uint8_t b){if(used<32)queue.incoming()[used]=b;if(used<33)used++;}
 bool finish(bool spi){return queue.publish(used,spi&&queue.incoming()[0]!=0xa5);}
};
}
