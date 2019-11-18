import sys
import os
import socket
import time
import numpy as np
import configparser

from PyQt5.QtCore import QThread
from PyQt5.QtGui import QFont
from PyQt5.QtGui import QImage
from PyQt5.QtGui import QPainter
from PyQt5.QtGui import QPen
from PyQt5.QtGui import QPixmap
from PyQt5.QtWidgets import *
from PyQt5.QtCore import QSize,Qt,pyqtSignal
from sort import Sort

global image_data
global age_info_str
global sk
global persons
global ageWindow
image_data = []
sk = socket.socket()
persons = {}
ageWindow = []


updateAgeTime = 4.0

# # categorized previous faces
# class Person():
#     def __init__(self, x, y, age):
#         self.centerX = x
#         self.centerY = y
#         self.age_list = [age]
#         self.age2show = age
#
#     def distance(self, x, y):
#         return np.square(self.centerX-x)+np.square(self.centerY-y)
#
#     def update(self, x, y, age):
#         self.centerX = x
#         self.centerY = y
#         self.age_list.append(age)
#         if self.age2show == -1:
#             self.age2show = age
#
#     def clear_buffer(self):
#         if len(self.age_list) == 0:
#             self.age2show = -1
#         else:
#             self.age2show = int(sum(self.age_list)/len(self.age_list))
#         self.age_list.clear()
#
# # uncategorized current faces
# class Face():
#     def __init__(self, x1, y1, x2, y2, age):
#         self.x1 = x1
#         self.y1 = y1
#         self.x2 = x2
#         self.y2 = y2
#         self.age = age
#
# def findMin(list, matched):
#     MIN = max(list)+1
#     index = -1
#     for i,val in enumerate(list):
#         if matched[i] == 0 and val < MIN:
#             index = i
#             MIN = val
#     return index
#
#
#
#
# def match_faces_persons(faces):
#     global persons
#     result = []
#     if len(faces) == 0:
#         return []
#     elif len(persons) == 0:
#         for face in faces:
#             persons.append(Person((face.x1+face.x2)/2, (face.y1+face.y2)/2, face.age))
#             result.append((face.x1,face.y1,face.x2,face.y2,face.age))
#     elif len(faces) <= len(persons):
#         matched = [0 for i in range(len(persons))]
#         for face in faces:
#             list = [persons[i].distance((face.x1+face.x2)/2, (face.y1+face.y2)/2) for i in range(len(persons))]
#             index = findMin(list, matched)
#             matched[index] = 1
#             persons[index].update((face.x1+face.x2)/2, (face.y1+face.y2)/2, face.age)
#             result.append((face.x1,face.y1,face.x2,face.y2,persons[index].age2show))
#     else:
#         matched = [0 for i in range(len(faces))]
#         for person in persons:
#             list = [person.distance((faces[i].x1+faces[i].x2)/2, (faces[i].y1+faces[i].y2)/2) for i in range(len(faces))]
#             index = findMin(list, matched)
#             matched[index] = 1
#             person.update((faces[index].x1+faces[index].x2)/2, (faces[index].y1+faces[index].y2)/2, faces[index].age)
#             result.append((faces[index].x1, faces[index].y1, faces[index].x2, faces[index].y2, person.age2show))
#         for i, face in enumerate(faces):
#             if matched[i] == 0:
#                 persons.append(Person((face.x1 + face.x2) / 2, (face.y1 + face.y2) / 2, face.age))
#                 result.append((face.x1, face.y1, face.x2, face.y2, face.age))
#     return result

def addMargin(x1,x2,y1,y2):
    h = y2-y1
    w = x2-x1
    x1_ = int(x1-w*0.2)
    if x1_<0:
        x1_=0
    x2_ = int(x2+w*0.2)
    if x2_>1280:
        x2_=1280
    y1_ = int(y1-h*0.2)
    if y1_<0:
        y1_=0
    y2_ = int(y2 + h * 0.4)
    if y2_ > 720:
        y2_ = 720
    return x1_,x2_,y1_,y2_

class Person():
    def __init__(self, face):
        self.x1 = face[0]
        self.y1 = face[1]
        self.x2 = face[2]
        self.y2 = face[3]
        self.age2show = int(face[4]+0.5)
        self.age_list = [face[4]]

    def update(self, face):
        self.x1 = face[0]
        self.y1 = face[1]
        self.x2 = face[2]
        self.y2 = face[3]
        self.age_list.append(face[4])

    def clear_buffer(self):
        self.age2show = int(sum(self.age_list)/len(self.age_list))
        self.age_list.clear()


class MainWindow(QMainWindow):

    def __init__(self):
        super().__init__()
        self.initUI()
        self.thread = Thread()
        self.thread.sinout.connect(self.slotDisplayImage)
        self.thread.start()
        self.time = 0
        self.timeCount = 0
        self.frame = 0
        self.mot_tracker = Sort(max_age=5)


    def initUI(self):
        self.setWindowTitle('Age Estimation')
        self.setGeometry(0, 0, 1280, 720)
        self.setFixedSize(QSize(1280, 720))

        self.image_label = QLabel(self)
        self.show()

    def slotDisplayImage(self):
        # time_st = time.time()
        self.frame += 1
        global socketEnable, image_data, persons, ageWindow, cnt
        # update time count
        costTime = time.time() - self.time
        self.timeCount += costTime
        #update fps
        self.time = time.time()
        fps = 1.0 / costTime
        # decode the frame
        image = QImage.fromData(image_data)
        pixmap = QPixmap.fromImage(image)
        #get current faces
        dets = []
        faces = []
        if ';' in age_info_str:
            for string in age_info_str.split(';')[0:-1]:
                x1 = int(string.split(',')[0])
                y1 = int(string.split(',')[1])
                x2 = int(string.split(',')[2])
                y2 = int(string.split(',')[3])
                age = float(string.split(',')[4])

                score = float(string.split(',')[5])
                x1, x2, y1, y2 = addMargin(x1, x2, y1, y2)
                dets.append([x1,y1,x2,y2,score])
                faces.append([x1, y1, x2, y2, age])

                # faces.append(Face(x1,y1,x2,y2,age))
        dets = np.asarray(dets)
        #get current frame results
        dets_ids, remove_ids = self.mot_tracker.update(dets)
        for d, id in dets_ids.items():
            if id in persons:
                persons[id].update(faces[d])
            else:
                persons[id] = Person(faces[d])
        for id in remove_ids:
            persons.pop(id)
        ids = dets_ids.values()

        # clear buffer after a period of time
        if self.timeCount > updateAgeTime:
            for id, person in persons.items():
                person.clear_buffer()
            self.timeCount = 0


        # set painter and draw fps
        painter = QPainter(pixmap)
        fpsFont = QFont()
        fpsFont.setPixelSize(20);
        fpsFont.setFamily("SimSun")
        painter.setFont(fpsFont)
        painter.setPen(QPen(Qt.black, 2, Qt.SolidLine))
        painter.drawText(pixmap.width() - 100, 40, "fps: {}".format(int(fps)))

        # set painter to draw age
        painter.setPen(QPen(Qt.yellow, 2, Qt.SolidLine))
        ageFont = QFont()
        ageFont.setPixelSize(60);
        ageFont.setFamily("Microsoft YaHei");
        painter.setFont(ageFont)

        if ids != []:
            # draw results
            # avoid results out of border
            for id in ids:
                x1 = persons[id].x1
                y1 = persons[id].y1
                x2 = persons[id].x2
                y2 = persons[id].y2

                age = persons[id].age2show
                painter.drawRect(x1, y1, x2 - x1, y2 - y1)
                if y1 > 100:
                    painter.drawText(x1, y1 - 10, str(age))
                else:
                    painter.drawText(x1, y2 + 60, str(age))
        painter.end()

        # draw image onto the window
        self.image_label.setGeometry(0, 0, pixmap.width(), pixmap.height())
        self.image_label.setPixmap(pixmap)
        # print('draw time: {}'.format(time.time()-time_st))



class Thread(QThread):
    sinout = pyqtSignal()
    def __init__(self):
        super().__init__()

    def run(self):
        # read config.conf
        # use absolute path only
        parentDirPath = os.path.split(os.path.realpath(__file__))[0]
        cf = configparser.ConfigParser()
        cf.read(os.path.join(parentDirPath, 'config.conf'))
        ip = cf.get("baseconf", "presenter_server_ip")
        port = int(cf.get("baseconf", "presenter_server_port"))
        print('Socket sever listen on {}:{}'.format(ip, port))
        address = (ip, port)
        sk.bind(address)
        while 1:
            # wait for client to establish connection
            sk.listen(1)
            print('waiting for client connection........ ')
            conn, addr = sk.accept()
            print("client connected.")

            while 1:
                received_length = 0
                age_info = b''
                disConnected = False
                while received_length < 1024:
                    r_data = conn.recv(1024)
                    # check for disconnection, the same below
                    if r_data==b'':
                        print('client disconnected.')
                        disConnected = True
                        break
                    received_length += len(r_data)
                    age_info += r_data
                if disConnected:
                    break

                tmp_age_info_str = str(age_info)[2:-1]
                print(tmp_age_info_str)
                image_length = int(tmp_age_info_str.split(':')[0])
                received_length = 0
                received_data = b''
                # print('actual:', image_length)
                conn.send(b'A')


                while received_length < image_length:
                    r_data = conn.recv(1024 * 1024)
                    if r_data==b'':
                        print('client disconnected.')
                        disConnected = True
                        break
                    received_length += len(r_data)
                    received_data += r_data
                if disConnected:
                    break
                # print('received:', received_length)
                conn.send(b'B')
                global image_data, age_info_str, pixmap
                image_data = received_data
                age_info_str = tmp_age_info_str.split(':')[1]
                self.sinout.emit()





if __name__ == '__main__':
    app = QApplication(sys.argv)
    ex = MainWindow()
    sys.exit(app.exec_())


