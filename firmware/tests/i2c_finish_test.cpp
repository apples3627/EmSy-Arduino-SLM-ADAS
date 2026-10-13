#define main audit_base_main
#include "i2c_finish_base.h"
#undef main
void pausedInterrupt(){
 for(uint8_t phase: {uint8_t(1),uint8_t(2),uint8_t(6),uint8_t(7),uint8_t(8)}){
  reset();MainLinks::begin(0,req(),0);auto& b=MainLinks::bus[0];b.phase=phase;b.index=0;TWSR=phase==1||phase==6?8:phase==2?0x18:phase==7?0x40:0x50;TWCR.value|=_BV(TWINT);TWDR=0xa5;
  uint8_t flags=TWCR.value;poll(99);check(b.phase==phase&&b.index==0&&TWCR.value==flags,"paused ISR: foreground does not duplicate active hardware transition");
  poll(100);check(b.phase==10&&!b.done&&!b.ok,"paused ISR: foreground transaction timeout requests bounded STOP");
  poll(200);check(b.phase==0&&b.done&&!b.ok,"paused ISR: error STOP deadline eventually completes failure");
 }
}
void cancel(){reset();auto& b=MainLinks::bus[0];b.phase=11;b.deadline=100;TWCR.master=true;TWCR=_BV(TWINT)|_BV(TWEN)|_BV(TWSTO);poll(99);check(b.phase==11&&!b.done,"cancel stuck STOP retains bound before deadline");poll(100);check(b.phase==0&&!b.done&&TWCR.disables==1,"cancel timeout resets peripheral without publishing a completed request");}
void wraparound(){reset();uint32_t start=0xfffffff0u;MainIO::ticks=start;MainLinks::begin(0,req(),start);poll(start+99);check(MainLinks::bus[0].phase==1,"transaction deadline comparison survives millis wrap before bound");poll(start+100);check(MainLinks::bus[0].phase==10,"transaction deadline comparison survives millis wrap at bound");poll(start+200);check(MainLinks::bus[0].done&&!MainLinks::bus[0].ok,"STOP deadline comparison survives millis wrap at bound");}
void diagnostics(){reset();fullRead();auto& b=MainLinks::bus[0];check(b.lastStatus==0x58&&b.error==MainLinks::I2C_OK&&b.readComplete,"success retains real final TWI status and complete response");reset();fullRead(true);check(MainLinks::bus[0].error==MainLinks::BAD_REPLY_FRAME&&MainLinks::bus[0].readComplete,"bad CRC retains raw completed reply");reset();MainLinks::begin(0,req(),0);event(8,1);event(0x20,2);check(MainLinks::bus[0].lastStatus==0x20&&MainLinks::bus[0].error==MainLinks::WRITE_ADDRESS_NACK&&!MainLinks::bus[0].readComplete,"write-address NACK diagnosed exactly");reset();MainLinks::begin(0,req(),0);MainLinks::bus[0].phase=7;event(0x48,2);check(MainLinks::bus[0].error==MainLinks::READ_ADDRESS_NACK,"read-address NACK diagnosed exactly");reset();MainLinks::begin(0,req(),0);poll(100);check(MainLinks::bus[0].lastStatus==0xff&&MainLinks::bus[0].error==MainLinks::TRANSFER_TIMEOUT,"missing event diagnosed independently of previous transaction");check(MainLinks::bus[0].deadline==200,"timeout STOP uses supplied atomic service time");}
int main(){audit_base_main();pausedInterrupt();cancel();wraparound();diagnostics();printf("ALL_FAILURES=%u\n",failures);return failures?1:0;}
