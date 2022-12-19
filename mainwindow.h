#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <SDL2/SDL.h>
#include <PdBase.hpp>
#include "slide.h"

class Receiver : public pd::PdReceiver
{
	void print(const std::string& message)
	{
		std::cout << message << std::endl;
	}
};

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent = nullptr);
	~MainWindow();

private slots:
	void tempo_show();
	void tempo_push(real mpb);
	void on_btnPreset1_pressed();
	void on_btnPreset2_pressed();
	void on_btnPreset3_pressed();
	void on_btnReset_pressed();
	void on_chkPause_stateChanged(int paused);

	void on_edtVolume_returnPressed();
	void on_edtTempo_returnPressed();
	void on_edtBPM_returnPressed();

	void sldVolume_valueChanged(int value);
	void sldTempo_valueChanged(int value);

	void spnAccent1_valueChanged(int i);
	void spnAccent2_valueChanged(int i);

private:
	Ui::MainWindow *ui;
	SDL_AudioDeviceID dev;
	SDL_AudioSpec have;
	pd::Patch patch;
	pd::PdBase pd;
	Receiver rec;

	// pd messages
	std::string dest_vol;
	std::string dest_play;
	std::string dest_beat;
	std::string dest_tempo;
	std::string dest_accent1;
	std::string dest_accent2;

	// tempo presets
	real preset1;
	real preset2;
	real preset3;

	Slide volume; // volume slider parameters
	Slide tempo;  // tempo slider parameters
};
#endif // MAINWINDOW_H
