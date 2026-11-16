#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Generate and independently inspect authorized mixed audio; no private media."""
import json,pathlib,subprocess,sys
root=pathlib.Path(sys.argv[1]);root.mkdir(parents=True,exist_ok=True)
specs=[('m4a','aac',44100,2,.55),('mp3','libmp3lame',48000,2,.65),('flac','flac',96000,2,.75),('ogg','libvorbis',48000,1,.45),('flac','flac',48000,6,.8),('wav','pcm_s16le',48000,2,.5),('wav','pcm_s24le',96000,2,.7),('wav','pcm_f32le',44100,2,.6)]
files=[]
for n,(ext,codec,rate,channels,duration) in enumerate(specs):
 path=root/f'mixed-{n}.{ext}'
 tones='|'.join(f'0.12*sin(2*PI*{397+channel*103}*t)' for channel in range(channels))
 args=['ffmpeg','-hide_banner','-loglevel','error','-y','-f','lavfi','-i',f'aevalsrc={tones}:s={rate}:c={"5.1" if channels==6 else "stereo" if channels==2 else "mono"}','-t',str(duration),'-c:a',codec]
 if codec=='flac':args+=['-sample_fmt','s32']
 subprocess.run(args+[str(path)],check=True)
 data=json.loads(subprocess.check_output(['ffprobe','-v','error','-select_streams','a:0','-show_entries','stream=codec_name,sample_rate,channels,channel_layout,sample_fmt,bits_per_raw_sample:format=duration','-of','json',str(path)]));stream=data['streams'][0]
 assert int(stream['sample_rate'])==rate and stream['channels']==channels
 files.append({'file':path.name,'container':ext,'codec':stream['codec_name'],'rate':rate,'channels':channels,'layout':stream.get('channel_layout','unspecified'),'input_sample':stream['sample_fmt'],'duration_ms':round(float(data['format']['duration'])*1000)})
sequence=[0,1,2,1,0,3,4,3,0,5,6,5,7,5,0,4]
(root/'mixed-audio.json').write_text(json.dumps([files[n] for n in sequence],indent=2)+'\n')
subprocess.run(['ffmpeg','-hide_banner','-loglevel','error','-y','-f','lavfi','-i','sine=frequency=997:sample_rate=48000','-t','8','-c:a','aac','-ac','2',str(root/'continuity-control.m4a')],check=True)
print('Verified eight generated formats with ffprobe; sixteen directed transition slots')

subprocess.run(['ffmpeg','-hide_banner','-loglevel','error','-y','-f','lavfi','-i','testsrc2=size=320x180:rate=24','-f','lavfi','-i','sine=frequency=997:sample_rate=48000','-t','1','-c:v','libx264','-preset','ultrafast','-c:a','aac','-ac','2',str(root/'mixed-video.mp4')],check=True)
