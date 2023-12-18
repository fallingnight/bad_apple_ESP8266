import struct
from mido import MidiFile
# 从midi文件中提取每个音符的频率-持续时间-开始时间

def ticks_to_ms(ticks, bpm, ticks_per_beat):
    tick_ms = 60000.0 / (bpm * ticks_per_beat)
    ms = ticks * tick_ms
    return ms

def ticks_to_us(ticks, bpm, ticks_per_beat):
    tick_us = 60000000.0 / (bpm * ticks_per_beat)
    us = ticks * tick_us
    return us


def note_to_freq(note):
    return 440*pow(2,(note-69)/12)

# midi序号转换成音名，调试用
def note_to_name(note):
    note_names = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
    note_index = note % 12
    octave = note // 12 - 1
    note_name = note_names[note_index] + str(octave)
    
    return note_name

# 打包3个int，写二进制文件用
def pack_audio_info(freq, duration, start_time):
    packed_data = struct.pack('III', freq, duration, start_time)
    return packed_data

def write_to_file(output_file, audio_info_list):
    with open(output_file, 'wb') as bat_file:
        for audio_info in audio_info_list:
            packed_data = pack_audio_info(*audio_info)
            bat_file.write(packed_data)



def extract_note_info(midi_file_path,audio_info_list):
    midi_file = MidiFile(midi_file_path)
    midi_file.play()

    # BPM和分辨率
    bpm = 138
    ticks_per_beat = midi_file.ticks_per_beat


    for i, track in enumerate(midi_file.tracks):
        print(f'Track {i}: {track.name}')
        total_time=0 #由于mido中note_on的time表示当前音符和上一个音符之间的距离，需要手动设置第一个音开始的时间
        active_notes = {}
        for msg in track:
            if msg.type == 'note_on':
                note = msg.note
                velocity = msg.velocity
                time = msg.time
                if(time!=0):
                    total_time+=ticks_to_us(msg.time,bpm, ticks_per_beat)

                active_notes[note] = {'velocity': velocity, 'start_time': total_time}

            elif msg.type == 'note_off':
                note = msg.note
                velocity = active_notes[note]['velocity']
                start_time = active_notes[note]['start_time']
                end_time = start_time + ticks_to_us(msg.time, bpm, ticks_per_beat)
                freq = note_to_freq(note)
                note_name=note_to_name(note)
                if(msg.time==0): # 跳过可能存在的持续时间为0的音符
                    del active_notes[note]
                    print("!") 
                    continue
                duration = ticks_to_us(msg.time, bpm, ticks_per_beat)
                total_time+=duration

                print(f'Note: {note_name}, Freq: {freq:.2f}Hz, Duration: {duration:.2f} us, Start_time:{start_time:.2f} us')
                audio_info_list.append((int(freq),int(duration),int(start_time)))

                del active_notes[note]

# 替换为你自己的MIDI文件路径
midi_file_path = 'G:/bad apple/tr_2_2.mid'
output_file = "output_audio_1.dat"
audio_info_list=[]
extract_note_info(midi_file_path,audio_info_list)
write_to_file(output_file, audio_info_list)