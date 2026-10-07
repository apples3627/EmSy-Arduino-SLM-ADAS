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
 def __init__(self,kind):super().__init__();self.kind=kind;self.width=511;self.height=94 if kind=='single' else 99 if kind=='timeline' else 142
 def draw(self):
  c=self.canv
  def box(x,y,w,h,title,sub):
   c.setFillColor(PALE);c.setStrokeColor(TEAL);c.roundRect(x,y,w,h,6,fill=1,stroke=1)
   c.setFillColor(BLUE);c.setFont('KR-B',9);c.drawCentredString(x+w/2,y+h-18,title)
   c.setFont('KR',8);c.drawCentredString(x+w/2,y+12,sub)
  def arrow(x1,y1,x2,y2,label):
   c.setStrokeColor(TEAL);c.line(x1,y1,x2,y2);c.line(x2,y2,x2-5,y2+3);c.line(x2,y2,x2-5,y2-3)
   c.setFillColor(GRAY);c.setFont('KR',7.5);c.drawCentredString((x1+x2)/2,(y1+y2)/2+6,label)
  if self.kind=='timeline':
   c.setFillColor(GRAY);c.setFont('KR',8);c.drawString(0,94,'계획 파형 개념도 · 실측 아님 · 같은 job ID의 실제 전송')
   for y,label,x,w in [(69,'UART frame',110,390),(44,'I2C transaction',137,94),(19,'SPI transaction',145,67)]:
    c.setFont('KR',8);c.setFillColor(BLUE);c.drawString(0,y+3,label)
    c.setStrokeColor(HexColor('#ccd9df'));c.line(104,y+6,505,y+6)
    c.setFillColor(TEAL);c.roundRect(x,y,w,12,2,fill=1,stroke=0)
   c.setStrokeColor(BLUE);c.setDash(2,2);c.line(145,14,145,88);c.line(212,14,212,88);c.setDash()
   c.setFont('KR',7.5);c.setFillColor(BLUE);c.drawString(244,25,'점선 사이: 세 전송의 공통 구간')
   c.setFont('KR',7.5);c.setFillColor(GRAY);c.drawString(110,1,'UART 시작 → TWI 시작 → SPI 바이트 진행 → 각 링크 독립 poll')
  elif self.kind=='single':
   box(0,19,140,57,'PC / 시리얼 모니터','학습·업로드·입출력');box(230,19,275,57,'한 대의 Uno R3','임베딩 → 은닉층 → 로짓 → 다음 문자');arrow(140,46,230,46,'프롬프트 / 결과')
   c.setFillColor(GRAY);c.setFont('KR',8);c.drawString(0,2,'추론 산술은 Uno 내부에서 수행 · PC는 표시만 담당')
  else:
   box(0,48,122,58,'메인 Uno','합산 + bias + argmax')
   for y,title,sub,bus in [(94,'워커 1','은닉 뉴런 일부 계산','UART'),(47,'워커 2','은닉 뉴런 일부 계산','I2C'),(0,'워커 3','은닉 뉴런 일부 계산','SPI')]:
    box(305,y,200,42,title,sub);arrow(122,77,305,y+21,bus)
   c.setFont('KR',7.4);c.setFillColor(GRAY);c.drawString(0,22,'입력: 공통 16문자 문맥');c.drawString(0,8,'반환: 워커당 부분 로짓 29개')
def footer(c,doc):
 w,h=A4;c.setStrokeColor(TEAL);c.setLineWidth(2);c.line(42,h-35,w-42,h-35)
 c.setFillColor(GRAY);c.setFont('KR',7.5);c.drawString(42,h-25,'EMBEDDED SYSTEMS · PROJECT PLAN');c.drawRightString(w-42,h-25,'UNO × 4 / UART · I2C · SPI')
 c.setStrokeColor(HexColor('#dce5e9'));c.setLineWidth(.5);c.line(42,35,w-42,35)
 c.setFont('KR',7.5);c.drawString(42,23,'2026.10.07  |  계획 단계 · 실물 및 4보드 검증 예정');c.drawRightString(w-42,23,f'{doc.page} / {TOTAL_PAGES}')
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
   if n==2:widths=[220,291] if rows[0][0]=='예정 경로' else [120,391]
   data=[[text(cell,'head' if r==0 else 'cell') for cell in row] for r,row in enumerate(rows)]
   t=Table(data,colWidths=widths,hAlign='LEFT');t.setStyle(TableStyle([('BACKGROUND',(0,0),(-1,0),BLUE),('ROWBACKGROUNDS',(0,1),(-1,-1),[PALE,white]),('VALIGN',(0,0),(-1,-1),'TOP'),('LEFTPADDING',(0,0),(-1,-1),9),('RIGHTPADDING',(0,0),(-1,-1),9),('TOPPADDING',(0,0),(-1,-1),7),('BOTTOMPADDING',(0,0),(-1,-1),7),('LINEBELOW',(0,0),(-1,0),1,TEAL)]));story.extend([t,Spacer(1,8)]);continue
  style='ref' if s.startswith('[') or s.startswith('로컬 근거:') else 'body'
  story.append(text(s,style))
 out=ROOT/'output/pdf/arduino_language_model_project_plan.pdf';out.parent.mkdir(parents=True,exist_ok=True)
 doc=SimpleDocTemplate(str(out),pagesize=A4,leftMargin=42,rightMargin=42,topMargin=53,bottomMargin=46,title='네 대의 Arduino로 함께 생성하는 작은 문장 - 프로젝트 계획서',author='')
 TOTAL_PAGES=page
 doc.build(story,onFirstPage=footer,onLaterPages=footer)
 actual=len(PdfReader(str(out)).pages)
 assert actual==TOTAL_PAGES, f'Layout overflow: expected {TOTAL_PAGES}, got {actual}; adjust content before publishing'
 print(out)
if __name__=='__main__':main()
