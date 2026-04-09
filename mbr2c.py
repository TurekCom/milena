#!/usr/bin/env python
#coding: utf-8

import re

lines=open('data/pl_mbrola.dat').readlines()
rows=[]
for row in lines:
    row=row.strip()
    if not row or row.startswith(';'):
        continue
    rows.append(row.split())

# wszystkie fonemy i equale
phonemes={}
cadrs=[]

irows=[]
while rows:
    row=rows.pop(0)
    if row[0] != 'code':
        irows.append(row)
        continue
    t=row[2]
    vowel=False
    if t.endswith('/v'):
        t=t[:-2]
        vowel=True
    phoneme={
            'phoneme':row[1],
            'outphone':t,
            'xlen':int(row[3]),
            'vowel': vowel,
            'cdrl':0,
            'cadr':len(cadrs)
            }
    phonemes[row[1]]=phoneme
    while True:
        row=rows.pop(0)
        if row[0] == '\\':
            break
        row[1]=' '.join(row[1:])
        ra=row[0].split(':')
        input_text=row[1]
        after=ra[-1]
        before=ra[0] if len(ra) > 1 else None
        flags=set()
        leng=0
        if input_text.find('_')>0:
            flags.add('MBROFG_SEPARATOR')
        if input_text.find('+')>=0:
            flags.add('MBROFG_ADDLEN')
        if input_text.find('-')>=0:
            flags.add('MBROFG_SUBLEN')
        if input_text.find('=')>=0:
            flags.add('MBROFG_IAFTER')
        if input_text.find('j')>=0:
            flags.add('MBROFG_JBEFORE')
        if input_text.find('s')>=0:
            flags.add('MBROFG_SBEFORE')
        if input_text.find('<')>=0:
            flags.add('MBROFG_YBEFORE')
        if input_text.find('S')>=0:
            flags.add('MBROFG_SIBEFORE')
        if input_text.find('H')>=0:
            flags.add('MBROFG_SHBEFORE')
        if input_text.find('h')>=0:
            flags.add('MBROFG_HBEFORE')
        if input_text.find('f')>=0:
            flags.add('MBROFG_FBEFORE')
        if input_text.find('z')>=0:
            flags.add('MBROFG_ZBEFORE')
        if input_text.find('Z')>=0:
            flags.add('MBROFG_ZHBEFORE')
        if input_text.find('!')>=0:
            flags.add('MBROFG_DUPNEXT')
        if input_text.find('n')>=0:
            flags.add('MBROFG_AFTERN')
        if input_text.find('N')>=0:
            flags.add('MBROFG_AFTERNG')
        if input_text.find('J')>=0:
            flags.add('MBROFG_PALATIZE')
        input_text=re.sub(r'[^0-9]*','',input_text)
        if input_text:
            leng=int(input_text,10)
        if flags:
            flags='|'.join(list(flags))
        else:
            flags='0'
        cadrs.append({
            'bef':before,
            'aft':after,
            'flags':flags,
            'length':leng})
        phoneme['cdrl'] += 1
    

def strnone(a):
    if a is None:
        return 'NULL'
    else:
        return '"%s"' % a
        
s=',\n    '.join(
    '{%s,%s,%s,%d}' % (strnone(x['bef']),strnone(x['aft']),x['flags'],x['length']) for x in cadrs)

t='static struct mbrorule static_mbrorules[]={' + s + '};\n'


for row in irows:
    if row[0] == 'equal':
        phoneme=phonemes[row[2]].copy()
        phoneme['phoneme']=row[1]
        phonemes[row[1]]=phoneme


phones1=[]
phones2=[]
def phs(a,b):
    return cmp(a['phoneme'],b['phoneme'])

for p in phonemes.iterkeys():
    if len(p) == 2:
        phones2.append(phonemes[p])
    else:
        phones1.append(phonemes[p])

phones1.sort(phs)
phones2.sort(phs)

s=',\n    '.join(
    '{"%s","%s",%d,%d,%d,%d}' % (x['phoneme'],x['outphone'],x['xlen'],1 if x['vowel'] else 0,x['cdrl'],x['cadr'])
        for x in phones1)

t += 'static struct mbrophon static_mbrophon_1[]={\n    '+s+('};\n\n#define PHONEMES_1_COUNT %d\n\n' % len(phones1))

s=',\n    '.join(
    '{"%s","%s",%d,%d,%d,%d}' % (x['phoneme'],x['outphone'],x['xlen'],1 if x['vowel'] else 0,x['cdrl'],x['cadr'])
        for x in phones2)

t += 'static struct mbrophon static_mbrophon_2[]={\n    '+s+('};\n\n#define PHONEMES_2_COUNT %d\n\n' % len(phones2))


print t
open('src/static_mbrola.c','w').write(t)




            
        


        
        
        




# smbrophon:
# phoneme (2)
# outphone (2)
# xlen/flags(1)
# cdrl(1)
# cadr(2)




    

