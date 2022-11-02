#!/usr/bin/env python3

from sys import path
from math import log ,exp
from threading import Thread

path.append(f'{path[0]}/../pylibpd')
import pdmain as pd

from PyQt5.QtGui import QFont
from PyQt5.QtCore import Qt
from PyQt5.QtWidgets import (
	 QApplication
	,QGridLayout
	,QWidget
	,QLabel
	,QSlider
	,QSpinBox
	,QLineEdit
	,QCheckBox
	,QPushButton
)

def fmt(num):
	decimals = 5 - len(str(int(num)))
	return '{:.{prec}f}'.format(num ,prec=decimals)

steps = 2048
class Slide:
	def __init__(self ,scale ,value ,islog=True):
		self.value = value
		self.islog = islog
		(mn ,mx) = scale
		if islog:
			if mn == 0.0 and mx == 0.0:
				mx = 1.0
			if mx > 0.0:
				if mn <= 0.0:
					mn = 0.01 * mx
			else:
				if mn >  0.0:
					mx = 0.01 * mn
			self.slope = log(mx / mn) / steps
		else:
			self.slope =    (mx - mn) / steps
		self.mn = mn

	def fromstep(self ,step):
		if self.islog:
			return exp(self.slope * step) * self.mn
		else:
			return     self.slope * step  + self.mn

	def tostep(self):
		if self.islog:
			return int(log(self.value / self.mn) / self.slope)
		else:
			return int(   (self.value - self.mn) / self.slope)

# Preferences

volume = Slide(scale=(0.001 ,1) ,value=0.35)
'volume slider parameters'

tempo  = Slide(scale=(2000 ,30) ,value=1000)
'tempo slider parameters'

accent = 12
'number of beats between accented beats'

sub_accent = 4
'number of beats between sub-accented beats'


patch = pd.open(volume=volume.value)
dest_vol    = f'{patch}vol'
dest_play   = f'{patch}play'
dest_met    = f'{patch}met'
dest_bang   = f'{patch}bang'
dest_accent = f'{patch}accent'
dest_sub    = f'{patch}sub'

class Window(QWidget):
	def __init__(self ,parent = None):
		QWidget.__init__(self ,parent)
		grid = QGridLayout(self)
		top    = Qt.AlignmentFlag.AlignTop
		bottom = Qt.AlignmentFlag.AlignBottom
		center = Qt.AlignmentFlag.AlignHCenter
		mono   = QFont('Monospace')

		self.sld_tempo = QSlider(Qt.Vertical)
		self.sld_tempo.setMaximum(steps)
		self.sld_tempo.setValue(tempo.tostep())
		self.sld_tempo.valueChanged.connect(self.changed_tempo)
		self.lbl_tempo = QLabel(fmt(tempo.value))
		self.lbl_tempo.setFont(mono)
		grid.addWidget(QLabel('Tempo') ,0 ,0 ,1 ,1 ,center)
		grid.addWidget(self.sld_tempo  ,1 ,0 ,7 ,1 ,center)
		grid.addWidget(self.lbl_tempo  ,8 ,0 ,1 ,1 ,center)

		self.lbl_bpm = QLabel(fmt(60000 / tempo.value))
		self.lbl_bpm.setFont(mono)
		grid.addWidget(QLabel('BPM') ,0 ,1 ,1 ,1 ,center)
		grid.addWidget(self.lbl_bpm  ,1 ,1 ,1 ,1 ,center|top)

		sld_volume = QSlider(Qt.Vertical)
		sld_volume.setMaximum(steps)
		sld_volume.setValue(volume.tostep())
		sld_volume.valueChanged.connect(self.changed_volume)
		self.lbl_volume = QLabel(fmt(volume.value))
		self.lbl_volume.setFont(mono)
		grid.addWidget(QLabel('Volume') ,0 ,2 ,1 ,1 ,center)
		grid.addWidget(sld_volume       ,1 ,2 ,7 ,1 ,center)
		grid.addWidget(self.lbl_volume  ,8 ,2 ,1 ,1 ,center)

		chk_pause = QCheckBox()
		chk_pause.stateChanged.connect(self.changed_pause)
		grid.addWidget(QLabel('Pause') ,2 ,1 ,1 ,1 ,center)
		grid.addWidget(chk_pause       ,3 ,1 ,1 ,1 ,center|top)

		spn_accent = QSpinBox()
		spn_accent.setRange(1 ,128)
		spn_accent.setValue(accent)
		self.edit:QLineEdit = spn_accent.findChild(QLineEdit)
		spn_accent.valueChanged.connect(self.changed_accent ,Qt.QueuedConnection)
		grid.addWidget(QLabel('Accent') ,4 ,1 ,1 ,1 ,center|bottom)
		grid.addWidget(spn_accent       ,5 ,1 ,1 ,1 ,center)
		pd.libpd_float(dest_accent ,accent)

		spn_sub = QSpinBox()
		spn_sub.setRange(1 ,128)
		spn_sub.setValue(sub_accent)
		self.subedit:QLineEdit = spn_sub.findChild(QLineEdit)
		spn_sub.valueChanged.connect(self.changed_sub ,Qt.QueuedConnection)
		grid.addWidget(QLabel('Sub-Accent') ,6 ,1 ,1 ,1 ,center|bottom)
		grid.addWidget(spn_sub              ,7 ,1 ,1 ,1 ,center)
		pd.libpd_float(dest_sub ,sub_accent)

		btn_slow = QPushButton('1000')
		btn_slow.clicked.connect(self.slow_clicked)
		grid.addWidget(btn_slow ,9 ,0 ,1 ,1)

		btn_medm = QPushButton('875')
		btn_medm.clicked.connect(self.medm_clicked)
		grid.addWidget(btn_medm ,9 ,1 ,1 ,1)

		btn_fast = QPushButton('750')
		btn_fast.clicked.connect(self.fast_clicked)
		grid.addWidget(btn_fast ,9 ,2 ,1 ,1)

		grid.setRowStretch(1 ,6)
		grid.setRowStretch(3 ,6)
		self.resize(0 ,500)

		# start audio thread by invoking pause function
		self.changed_pause(False)

	def show_tempo(self):
		self.lbl_tempo.setText(fmt(tempo.value))
		self.lbl_bpm.setText(fmt(60000 / tempo.value))

	def btn_tempo(self ,value):
		tempo.value = value
		pd.libpd_float(dest_bang ,tempo.value)
		self.sld_tempo.blockSignals(True)
		self.sld_tempo.setValue(tempo.tostep())
		self.sld_tempo.blockSignals(False)
		self.show_tempo()

	def slow_clicked(self): self.btn_tempo(1000)
	def medm_clicked(self): self.btn_tempo(875)
	def fast_clicked(self): self.btn_tempo(750)

	def changed_tempo(self ,step):
		tempo.value = tempo.fromstep(step)
		pd.libpd_float(dest_met ,tempo.value)
		self.show_tempo()

	def changed_volume(self ,step):
		volume.value = volume.fromstep(step) if step > 0 else 0
		pd.libpd_float(dest_vol ,volume.value)
		self.lbl_volume.setText(fmt(volume.value))

	def changed_accent(self ,i):
		pd.libpd_float(dest_accent ,i)
		self.edit.deselect()

	def changed_sub(self ,i):
		pd.libpd_float(dest_sub ,i)
		self.subedit.deselect()

	def changed_pause(self ,paused):
		pd.playing = not paused
		if paused:
			self.audio.join()
		else:
			self.audio = Thread(target=pd.loop)
			self.audio.start()

	def closeEvent(self, event):
		pd.playing = False
		self.audio.join()
		pd.libpd_release()
		event.accept()


app = QApplication([])
app.setApplicationName('Metronome')
window = Window()
window.show()
app.exec()
