"""
用于把视频转换为板子可以播放的数据
不止bad apple,其他视频也可以使用(但如果不是本来就是这种影绘风格的话，效果不会太好)
"""
import cv2 as cv
import numpy as np

filepath = 'G:/BA/ba.mp4'  # 请改成你自己存放视频的路径

video = cv.VideoCapture(filepath)
c = 0
time_interval = 1  # 最终帧率＝原帧率/time_interval，请输入整数


# 二值化
def binary_image(image):
    gray = cv.cvtColor(image, cv.COLOR_RGB2GRAY)
    h, w = gray.shape[:2]
    m = np.reshape(gray, [1, w*h])
    mean = m.sum()/(w*h)
    ret, binary = cv.threshold(gray, mean, 255, cv.THRESH_OTSU)
    return binary


#位操作，用于生成闪存方法需要的二进制文件
def pixel_to_bir(frame):
    btarr = bytearray()

    for itemh in frame:
        count = 0
        ba = 0x00
        bl = 0x80
        count_all = 0
        for itemw in itemh:
            count_all += 1
            count += 1
            if (itemw < 128):
                ba = ba | bl
            bl = bl >> 1
            if (count == 8):
                count = 0
                btarr.append(ba)
                ba = 0x00
                bl = 0x80

    return btarr


if video.isOpened():  
    flag, frame = video.read()  
else:
    flag = False

while flag:  # 循环读取视频帧
    if flag is True:
        if (c % time_interval == 0):  
            frame = binary_image(frame)  
            frame = cv.bitwise_not(frame)  # 是否反相
            frame = cv.resize(frame, (88, 64))  
            cv.imwrite("picfile/"+str(c) + ".pbm", frame)  # 将每帧保存为单色位图（有9b文件头，所以一会文件传输时要去掉）

            #将所有帧保存为一个二进制文件
            """
            bf=pixel_to_bir(frame)
            f1=open("picfile1/ba.bin","ab")
            f1.write(bf)
            f1.close()
            """
        print(c)
        c = c + 1

    flag, frame = video.read()