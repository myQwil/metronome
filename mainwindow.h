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
	void on_btnSlow_pressed();
	void on_btnMedm_pressed();
	void on_btnFast_pressed();
	void on_btnReset_pressed();
	void on_chkPause_stateChanged(int paused);

	void on_numVolume_returnPressed();
	void on_numTempo_returnPressed();
	void on_numBPM_returnPressed();

	void sldVolume_valueChanged(int value);
	void sldTempo_valueChanged(int value);

	void spnAccent_valueChanged(int i);
	void spnSubacc_valueChanged(int i);

private:
	const char *initAudio();

	Ui::MainWindow *ui;
	SDL_AudioDeviceID dev;
	SDL_AudioSpec have;
	pd::Patch patch;
	pd::PdBase pd;
	Receiver rec;

	std::string dest_vol;
	std::string dest_play;
	std::string dest_met;
	std::string dest_set;
	std::string dest_accent;
	std::string dest_subacc;
};
#endif // MAINWINDOW_H
