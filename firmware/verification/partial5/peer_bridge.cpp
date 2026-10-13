#include <cstring>
#if PEER==0
#include PERIPHERAL_ENGINE_HEADER
static Peripheral::Engine peer;
extern "C" void peer_reset(){peer.init(0);}
extern "C" void peer_exchange(const uint8_t* q,uint8_t* r,uint32_t now){Proto::Frame in,out;memcpy(in.bytes,q,32);peer.handle(in,out,now);memcpy(r,out.bytes,32);}
extern "C" void peer_alert(bool active,bool event){peer.adas(active,event);}
extern "C" int peer_flags(){return(peer.outputs().lights?1:0)|(peer.outputs().horn?4:0);}
extern "C" void peer_snapshot(void* out){memcpy(out,&peer.snapshot(),sizeof(Proto::SharedState));}
#else
#include SLM_SESSION_HEADER
#include SLM_PLANNER_HEADER
static SlmUart::Session peer;
extern "C" void slm_reset(){peer=SlmUart::Session();}
extern "C" bool slm_exchange(const uint8_t* q,uint8_t* r,uint32_t now){Proto::StreamParser parser;Proto::Frame in,out;for(uint8_t i=0;i<32;i++)parser.push(q[i],in);bool reply=peer.receive(in,now,out);if(reply){memcpy(r,out.bytes,32);return true;}if(peer.stopNeeded&&!(peer.pending&&Proto::op(peer.pendingFrame)==Proto::STOP_ALL)&&peer.issueStop(now)){memcpy(r,peer.pendingFrame.bytes,32);peer.sent(now);return true;}return false;}
extern "C" bool slm_command(const char* text,uint8_t* out,uint32_t now){SlmUart::Planner plan;bool ok=plan.begin(text,strlen(text),peer.generation,peer.epoch());if(!ok)return false;while(plan.active){char clause[97];Intent::Input in;float scores[10];uint8_t effective=7;if(plan.prepare(text,clause,in)){Intent::logits(in,scores);effective=Intent::decide(in,scores).effective;}plan.accept(effective);}bool sent=plan.decisions.stop?peer.issueStop(now):(!plan.decisions.rejected&&peer.issuePlan(plan.decisions.intents,plan.decisions.count,now));if(sent){memcpy(out,peer.pendingFrame.bytes,32);peer.sent(now);}return sent;}
extern "C" void slm_snapshot(void* out){memcpy(out,&peer.shared,sizeof(Proto::SharedState));}
extern "C" bool slm_pending(){return peer.pending;}
#endif
