#!/usr/bin/env python3
"""Render editable docs/project-plan.md as a paginated Korean project plan."""
from pathlib import Path
import re, html
from reportlab.pdfgen import canvas
from reportlab.platypus import SimpleDocTemplate,Paragraph,Spacer,Table,TableStyle,PageBreak,Flowable,Preformatted
from reportlab.lib.styles import ParagraphStyle
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.lib.colors import HexColor,white
from reportlab.lib.pagesizes import A4
from pypdf import PdfReader
ROOT=Path(__file__).resolve().parents[1]
TOTAL_PAGES=0
FONT=Path.home()/'Library/Fonts'
for name,file in [('KR','NotoSansKR-Regular.ttf'),('KR-B','NotoSansKR-Bold.ttf')]:pdfmetrics.registerFont(TTFont(name,str(FONT/file)))
BLUE=HexColor('#16364a');TEAL=HexColor('#147b82');GRAY=HexColor('#526775');PALE=HexColor('#eef5f6')
styles={
 'code':ParagraphStyle('code',fontName='Courier',fontSize=7.8,leading=11.4,textColor=BLUE,backColor=PALE,borderPadding=9,spaceBefore=5,spaceAfter=11),
 'title':ParagraphStyle('title',fontName='KR-B',fontSize=21,leading=29,textColor=BLUE,spaceAfter=17,wordWrap='CJK'),
 'h2':ParagraphStyle('h2',fontName='KR-B',fontSize=11.5,leading=17,textColor=TEAL,spaceBefore=13,spaceAfter=7,wordWrap='CJK'),
 'body':ParagraphStyle('body',fontName='KR',fontSize=9.1,leading=14.6,textColor=BLUE,spaceAfter=9,wordWrap='CJK'),
 'cell':ParagraphStyle('cell',fontName='KR',fontSize=8.3,leading=12.6,textColor=BLUE,wordWrap='CJK'),
 'head':ParagraphStyle('head',fontName='KR-B',fontSize=8.5,leading=13,textColor=white,wordWrap='CJK'),
 'ref':ParagraphStyle('ref',fontName='KR',fontSize=7.5,leading=11.5,textColor=GRAY,spaceAfter=6,wordWrap='CJK')}
def text(s,style='body'):
 s=html.escape(s)
 s=re.sub(r'(https://[^\s&lt;]+)',lambda m:f'<link href="{m[0]}" color="#147b82">{m[0]}</link>',s)
 return Paragraph(s,styles[style])
class Diagram(Flowable):
 def __init__(self,kind):
  super().__init__();self.kind=kind;self.width=511;self.height=112 if kind=='timeline' else 193
 def draw(self):
  c=self.canv
  def box(x,y,w,h,title,sub):
   c.setFillColor(PALE);c.setStrokeColor(TEAL);c.roundRect(x,y,w,h,5,fill=1,stroke=1)
   c.setFillColor(BLUE);c.setFont('KR-B',9);c.drawCentredString(x+w/2,y+h-17,title)
   c.setFont('KR',7.8);c.drawCentredString(x+w/2,y+10,sub)
  def line(x1,y1,x2,y2,dashed=False):
   c.setStrokeColor(TEAL);c.setDash(3,2) if dashed else c.setDash();c.line(x1,y1,x2,y2);c.setDash()
  if self.kind=='car':
   box(0,87,118,58,'메인 Uno','합산·판단·적용 ACK')
   box(259,142,246,44,'계산 Uno 1 / I2C','H32 → 부분 로짓10개')
   box(259,87,246,44,'계산 Uno 2 / SPI','H32 → 부분 로짓10개')
   box(259,16,246,48,'제어 Uno / UART','모터·조명·와이퍼·경적')
   line(118,129,259,164);line(118,113,259,109)
   line(118,97,259,53,True);line(118,87,259,27)
   c.setFont('KR',7.3);c.setFillColor(GRAY)
   c.drawString(122,69,'STATUS: 분류 중 조회')
   c.drawString(122,35,'ACTUATE: 판단 완료 후')
   c.drawString(0,51,'PC 입력: USB UART')
   c.drawString(0,5,'실선: 계산·최종 제어  |  점선: 동시 상태 조회  |  부품·실물 검증 예정')
  else:
   c.setFillColor(GRAY);c.setFont('KR',8);c.drawString(0,105,'계획 타임라인 · 실측 아님 · 같은 분류 job의 실제 상태/모델 전송')
   for y,label,x,w in [(78,'UART STATUS',111,280),(53,'I2C INPUT/RESULT',137,92),(28,'SPI INPUT/RESULT',145,67)]:
    c.setFont('KR',7.5);c.setFillColor(BLUE);c.drawString(0,y+3,label)
    c.setStrokeColor(HexColor('#ccd9df'));c.line(105,y+6,505,y+6)
    c.setFillColor(TEAL);c.roundRect(x,y,w,12,2,fill=1,stroke=0)
   c.setStrokeColor(BLUE);c.setDash(2,2);c.line(145,23,145,98);c.line(212,23,212,98);c.setDash()
   c.setFont('KR',7.5);c.setFillColor(BLUE);c.drawString(247,32,'공통 중첩 구간 → 계산 완료 → ACTUATE')
   c.setFont('KR',7.5);c.setFillColor(GRAY);c.drawString(110,6,'STATUS는 구동하지 않음 · 새 ACTUATE는 두 점수 수신 이후')
def footer(c,doc):
 w,h=A4;c.setStrokeColor(TEAL);c.setLineWidth(2);c.line(42,h-35,w-42,h-35)
 c.setFillColor(GRAY);c.setFont('KR',7.5);c.drawString(42,h-25,'EMBEDDED SYSTEMS · PROJECT PLAN');c.drawRightString(w-42,h-25,'UNO × 4 / UART · I2C · SPI')
 c.setStrokeColor(HexColor('#dce5e9'));c.setLineWidth(.5);c.line(42,35,w-42,35)
 c.setFont('KR',7.5);c.drawString(42,23,'2026.10.07  |  호스트 학습 완료 · 실물 및 4보드 검증 예정');c.drawRightString(w-42,23,f'{doc.page} / {TOTAL_PAGES}')
def main():
 global TOTAL_PAGES
 lines=(ROOT/'docs/project-plan.md').read_text().splitlines();story=[];i=0;page=0
 while i<len(lines):
  s=lines[i].strip();i+=1
  if not s:continue
  if s.startswith('```'):
   code=[]
   while i<len(lines) and not lines[i].startswith('```'):
    code.append(lines[i]);i+=1
   i+=1;story.append(Preformatted('\n'.join(code),styles['code']));continue
  if s.startswith('# '):
   if page:story.append(PageBreak())
   page+=1;story.append(text(s[2:],'title'));continue
  if s.startswith('## '):story.append(text(s[3:],'h2'));continue
  if s.startswith('[[diagram:'):story.extend([Diagram(s[10:-2]),Spacer(1,8)]);continue
  if s.startswith('|'):
   rows=[s.strip('|').split('|')]
   while i<len(lines) and lines[i].strip().startswith('|'):
    rows.append(lines[i].strip().strip('|').split('|'));i+=1
   n=len(rows[0]);widths=([113,194,204] if n==3 else [101,142,142,126])
   if n==2:widths=[220,291] if rows[0][0]=='예정 경로' or rows[0][0].startswith(('호스트 합성','V3 합성','V3 평가')) else [120,391]
   data=[[text(cell,'head' if r==0 else 'cell') for cell in row] for r,row in enumerate(rows)]
   t=Table(data,colWidths=widths,hAlign='LEFT');t.setStyle(TableStyle([('BACKGROUND',(0,0),(-1,0),BLUE),('ROWBACKGROUNDS',(0,1),(-1,-1),[PALE,white]),('VALIGN',(0,0),(-1,-1),'TOP'),('LEFTPADDING',(0,0),(-1,-1),9),('RIGHTPADDING',(0,0),(-1,-1),9),('TOPPADDING',(0,0),(-1,-1),7),('BOTTOMPADDING',(0,0),(-1,-1),7),('LINEBELOW',(0,0),(-1,0),1,TEAL)]));story.extend([t,Spacer(1,8)]);continue
  style='ref' if s.startswith('[') or s.startswith('로컬 근거:') else 'body'
  story.append(text(s,style))
 out=ROOT/'output/pdf/arduino_language_model_project_plan.pdf';out.parent.mkdir(parents=True,exist_ok=True)
 doc=SimpleDocTemplate(str(out),pagesize=A4,leftMargin=42,rightMargin=42,topMargin=53,bottomMargin=46,title='LM 기반 영어 명령 해석과 자동차 제어 - 프로젝트 계획서',author='')
 TOTAL_PAGES=page
 doc.build(story,onFirstPage=footer,onLaterPages=footer)
 actual=len(PdfReader(str(out)).pages)
 assert actual==TOTAL_PAGES, f'Layout overflow: expected {TOTAL_PAGES}, got {actual}; adjust content before publishing'
 print(out)
if __name__=='__main__':main()
