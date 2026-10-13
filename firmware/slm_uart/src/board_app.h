#pragma once
#include "protocol.h"
#include "planner.h"
#include "session.h"
#include "register_io.h"
namespace SlmBoard {
SlmUart::Session controller;SlmUart::Planner planner;Proto::StreamParser parser;
Proto::Frame reply;bool replyDue=false,wasReady=false,lineOverflow=false;
char line[193],clauseText[97];uint8_t lineSize=0;
Intent::Input input;float scores[10];
inline void error(const char* reason){SlmIO::sayP(PSTR("ERROR "));SlmIO::say(reason);SlmIO::sayP(PSTR(" ALL_STOP\n"));}
inline void stop(const char* reason){planner.active=false;lineSize=0;lineOverflow=false;SlmIO::actorFlush();controller.issueStop(SlmIO::now());error(reason);}
inline void status(){SlmIO::sayP(PSTR("STATUS session="));SlmIO::number(controller.session);SlmIO::sayP(PSTR(" epoch="));SlmIO::number(controller.epoch());SlmIO::sayP(PSTR(" ready="));SlmIO::number(controller.ready(SlmIO::now()));SlmIO::sayP(PSTR(" drive="));if(controller.shared.appliedDrive<0)SlmIO::pcWrite('-');SlmIO::number(controller.shared.appliedDrive<0?-controller.shared.appliedDrive:controller.shared.appliedDrive);SlmIO::sayP(PSTR(" lights="));SlmIO::number((controller.shared.appliedFlags&1)!=0);SlmIO::sayP(PSTR(" wiper=0 horn="));SlmIO::number((controller.shared.appliedFlags&4)!=0);SlmIO::sayP(PSTR(" source="));SlmIO::number(controller.shared.stopSources);SlmIO::sayP(PSTR("\n"));}
inline void serviceMain(){using namespace SlmIO;if(softOverflow){uint8_t saved=SREG;cli();softOverflow=0;rr=rw;SREG=saved;parser.reset();controller.fail();stop("main_rx_overflow");}
 while(rr!=rw){uint8_t b=rx[rr];rr=(rr+1)&63;Proto::Frame f;if(!parser.push(b,f))continue;uint32_t previous=controller.session,g=controller.generation;if(controller.receive(f,now(),reply))replyDue=true;if(previous!=controller.session){planner.active=false;lineSize=0;lineOverflow=false;actorFlush();}else if(planner.active&&g!=controller.generation){planner.active=false;controller.stopNeeded=true;error("stale_plan");}}
 if(controller.appliedEvent){controller.appliedEvent=false;sayP(PSTR("APPLIED cmd="));number(controller.shared.command);sayP(PSTR(" drive="));if(controller.shared.appliedDrive<0)pcWrite('-');number(controller.shared.appliedDrive<0?-controller.shared.appliedDrive:controller.shared.appliedDrive);sayP(PSTR(" lights="));number((controller.shared.appliedFlags&1)!=0);sayP(PSTR(" wiper=0 horn="));number((controller.shared.appliedFlags&4)!=0);sayP(PSTR(" epoch="));number(controller.epoch());sayP(PSTR("\n"));}
 if(controller.errorEvent){controller.errorEvent=false;error("main_transaction");}}
inline void pump(){controller.tick(SlmIO::now());if(controller.stopNeeded&&!(controller.pending&&Proto::op(controller.pendingFrame)==Proto::STOP_ALL)){SlmIO::actorFlush();controller.issueStop(SlmIO::now());}if(replyDue&&SlmIO::actorSend(reply))replyDue=false;if(controller.pending&&controller.sendDue&&SlmIO::actorSend(controller.pendingFrame))controller.sent(SlmIO::now());bool ready=controller.ready(SlmIO::now());if(ready!=wasReady){SlmIO::sayP(ready?PSTR("READY model=95616d56\n"):PSTR("WAIT MAIN\n"));wasReady=ready;}}
inline void finish(){SlmIO::sayP(PSTR("PLAN "));SlmIO::number(planner.decisions.count);for(uint8_t i=0;i<planner.decisions.count;i++){SlmIO::pcWrite(' ');SlmIO::number(planner.decisions.intents[i]);}SlmIO::sayP(PSTR("\n"));if(planner.decisions.stop){stop("stop");return;}if(planner.decisions.rejected||!planner.decisions.count){stop("plan_rejected");return;}serviceMain();if(!planner.valid(controller.generation,controller.epoch())||!controller.issuePlan(planner.decisions.intents,planner.decisions.count,SlmIO::now()))stop("not_ready");lineSize=0;}
inline void processLine(){line[lineSize]=0;if(SlmUart::exactStop(line,lineSize)){stop("priority_stop");return;}if(!strcmp(line,"!status")){status();lineSize=0;return;}if(lineOverflow){stop("line_too_long");return;}if(!controller.ready(SlmIO::now())){stop("not_ready_or_busy");return;}if(!planner.begin(line,lineSize,controller.generation,controller.epoch())){stop("grammar");return;}if(!planner.active)finish();}
inline void pc(){using namespace SlmIO;if(pcOverflow){pcOverflow=0;stop("pc_rx_overflow");uint8_t saved=SREG;cli();pr=pw;SREG=saved;return;}while(pr!=pw){char c=pcRx[pr];pr=(pr+1)&127;if(c=='\r')continue;if(planner.active){stop("busy");}if(c=='\n'){processLine();if(planner.active)break;continue;}if(lineSize<192)line[lineSize++]=c;else lineOverflow=true;}}
inline void advance(){if(!planner.active)return;uint32_t g=controller.generation,e=controller.epoch();if(!planner.valid(g,e)||!controller.ready(SlmIO::now())){stop("stale_plan");return;}bool compute=planner.prepare(line,clauseText,input);Intent::Result result={7,7,input.reason,0,0};if(compute){Intent::logits(input,scores);++planner.kernels;
 // Every blocking NN kernel must yield to authoritative MAIN frames before decision/forwarding.
 serviceMain();controller.tick(SlmIO::now());if(!planner.active||!planner.valid(controller.generation,controller.epoch())){planner.active=false;controller.stopNeeded=true;return;}result=Intent::decide(input,scores);}else if(input.guard)result.reason=Intent::GUARDED;
 SlmIO::sayP(PSTR("RESULT raw="));SlmIO::number(result.raw);SlmIO::sayP(PSTR(" effective="));SlmIO::number(result.effective);SlmIO::sayP(PSTR(" reason="));SlmIO::number(result.reason);SlmIO::sayP(PSTR("\n"));planner.accept(result.effective);if(!planner.active)finish();}
inline void setup(){SlmIO::setup();SlmIO::sayP(PSTR("SLM_UART model=95616d56 WAIT MAIN\n"));}
inline void loop(){serviceMain();pump();pc();advance();serviceMain();pump();}
}
