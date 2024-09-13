#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <json/json.h>
#include <fstream>
#include <QDir>
#include <QStandardPaths>
#include <QAction>

enum {
	  freq = 48000
	, samples = 1024
	, channels = 2
};

const static int exponent =
	log( channels * sizeof(real) * pd::PdBase::blockSize() ) / M_LN2;

static inline void fail(const char *err) {
	std::cerr << err << std::endl;
	QApplication::quit();
}

static inline QString fmt(real num) {
	int precision = 5 - QString::number((int)num).length();
	return QString::number(num, 'f', precision);
}

static void callback(void *lpd, Uint8 *stream, int size) {
	if (size >= 512) {
		((pd::PdBase *)lpd)->processFloat(size >> exponent, nullptr, (float *)stream);
	}
}


MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
{
	QStringList appdata =
		QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);
	std::string path = appdata[0].toStdString();

	// default settings
	int accent1 = 4, accent2 = 2;
	preset1 = 1000, preset2 = 875, preset3 = 750;
	real t_min = 2000, t_max = 20, t_val = 1000;
	bool t_log = true;
	real vol = 0.1;
	std::string file = path + "/hihat.pd";

	// user settings
	Json::Value root;
	JSONCPP_STRING err;
	Json::CharReaderBuilder builder;
	std::ifstream ifs(path + "/settings.json");
	parseFromStream(builder, ifs, &root, &err);
	if (root["accent1"]) accent1 = root["accent1"].asInt();
	if (root["accent2"]) accent2 = root["accent2"].asInt();
	if (root["preset1"]) preset1 = root["preset1"].asFloat();
	if (root["preset2"]) preset2 = root["preset2"].asFloat();
	if (root["preset3"]) preset3 = root["preset3"].asFloat();
	if (root["volume"]) vol = root["volume"].asFloat();
	if (root["patch"]) {
		file = root["patch"].asString();
		if (file.at(0) == '~') { // home path
			file = QDir::homePath().toStdString() + file.substr(1);
		} else if (file.at(0) != '/') { // relative path
			file = path + "/" + file;
		}
	}
	if (root["tempo"]) {
		if (root["tempo"]["min"]) {
			t_min = root["tempo"]["min"].asFloat();
			if (t_min < 1) t_min = 1;
		}
		if (root["tempo"]["max"]) {
			t_max = root["tempo"]["max"].asFloat();
			if (t_max < 1) t_max = 1;
		}
		if (root["tempo"]["val"]) t_val = root["tempo"]["val"].asFloat();
		if (root["tempo"]["log"]) t_log = root["tempo"]["log"].asFloat();
	}
	volume = Slide(0.001, 1, vol);
	tempo  = Slide(t_min, t_max, t_val, t_log);

	// initialize audio

	if (SDL_Init(SDL_INIT_AUDIO) < 0) {
		fail(SDL_GetError());
	}
	if (!lpd.init(0, channels, freq)) {
		SDL_CloseAudioDevice(dev);
		SDL_CloseAudio();
		fail("Error initializing pd.");
	}
	lpd.setReceiver(this);

	std::size_t end = file.find_last_of("/\\");
	patch = lpd.openPatch(file.substr(end + 1), file.substr(0, end));
	const std::string dlr = patch.dollarZeroStr();
	dest_vol     = dlr + "vol";
	dest_play    = dlr + "play";
	dest_beat    = dlr + "beat";
	dest_tempo   = dlr + "tempo";
	dest_accent1 = dlr + "accent1";
	dest_accent2 = dlr + "accent2";

	lpd.sendFloat(dest_accent1, accent1);
	lpd.sendFloat(dest_accent2, accent2);
	lpd.sendFloat(dest_vol, volume.val);
	lpd.sendFloat(dest_play, tempo.val);
	startAudio();

	ui->setupUi(this);

	ui->btnPreset1->setText(QString::number(preset1));
	ui->btnPreset2->setText(QString::number(preset2));
	ui->btnPreset3->setText(QString::number(preset3));

	ui->sldVolume->setMaximum(run);
	ui->sldVolume->setValue(volume.tostep());
	connect(ui->sldVolume, SIGNAL(valueChanged(int))
		, this, SLOT(sldVolume_valueChanged(int)));

	ui->sldTempo->setMaximum(run);
	ui->sldTempo->setValue(tempo.tostep());
	connect(ui->sldTempo, SIGNAL(valueChanged(int))
		, this, SLOT(sldTempo_valueChanged(int)));

	ui->edtVolume->setText(fmt(volume.val));
	ui->edtVolume->setValidator(new QDoubleValidator(0, 1, -1, this));

	ui->edtTempo->setText(fmt(tempo.val));
	ui->edtTempo->setValidator(
		new QDoubleValidator(tempo.min(), tempo.max(), -1, this));

	ui->edtBPM->setText(fmt(60000/tempo.val));
	ui->edtBPM->setValidator(
		new QDoubleValidator(60000/tempo.max(), 60000/tempo.min(), -1, this));

	ui->spnAccent1->setValue(accent1);
	connect(ui->spnAccent1, SIGNAL(valueChanged(int))
		, this, SLOT(spnAccent1_valueChanged(int)), Qt::QueuedConnection);

	ui->spnAccent2->setValue(accent2);
	connect(ui->spnAccent2, SIGNAL(valueChanged(int))
		, this, SLOT(spnAccent2_valueChanged(int)), Qt::QueuedConnection);

	QAction *ctrl_q = new QAction(this);
	ctrl_q->setShortcut(Qt::Key_Q | Qt::CTRL);
	connect(ctrl_q, SIGNAL(triggered()), this, SLOT(close()));
	this->addAction(ctrl_q);

	QAction *esc = new QAction(this);
	esc->setShortcut(Qt::Key_Escape);
	connect(esc, SIGNAL(triggered()), this, SLOT(close()));
	this->addAction(esc);
}

MainWindow::~MainWindow() {
	delete ui;
	lpd.closePatch(patch);
	lpd.computeAudio(false);
	SDL_CloseAudioDevice(dev);
	SDL_CloseAudio();
}

void MainWindow::print(const std::string &message) {
	std::cout << message << std::endl;
}

void MainWindow::startAudio() {
	SDL_AudioSpec spec = {};
	spec.freq = freq;
	spec.format = AUDIO_F32;
	spec.channels = channels;
	spec.samples = samples;
	spec.callback = callback;
	spec.userdata = &lpd;
	dev = SDL_OpenAudioDevice(NULL, 0, &spec, &spec, 0);
	SDL_PauseAudioDevice(dev, 0);
	lpd.computeAudio(true);
}

void MainWindow::stopAudio() {
	lpd.computeAudio(false);
	SDL_CloseAudioDevice(dev);
	dev = 0;
}

void MainWindow::tempo_show() {
	ui->edtTempo->setText(fmt(tempo.val));
	ui->edtBPM->setText(fmt(60000 / tempo.val));
}

void MainWindow::tempo_push(real mpb) {
	tempo.val = mpb;
	lpd.sendFloat(dest_tempo, tempo.val);
	ui->sldTempo->blockSignals(true);
	ui->sldTempo->setValue(tempo.tostep());
	ui->sldTempo->blockSignals(false);
}

void MainWindow::on_btnPreset1_pressed() {
	tempo_push(preset1);
	tempo_show();
}

void MainWindow::on_btnPreset2_pressed() {
	tempo_push(preset2);
	tempo_show();
}

void MainWindow::on_btnPreset3_pressed() {
	tempo_push(preset3);
	tempo_show();
}

void MainWindow::on_btnReset_pressed() {
	lpd.sendFloat(dest_beat, 0);
}

void MainWindow::on_chkPause_stateChanged(int paused) {
	paused ? stopAudio() : startAudio();
}

void MainWindow::on_edtVolume_returnPressed() {
	volume.val = ui->edtVolume->text().toFloat();
	lpd.sendFloat(dest_vol, volume.val);
	ui->sldVolume->blockSignals(true);
	ui->sldVolume->setValue(volume.tostep());
	ui->sldVolume->blockSignals(false);
	ui->edtVolume->setText(fmt(volume.val));
}

void MainWindow::on_edtTempo_returnPressed() {
	real mpb = ui->edtTempo->text().toFloat();
	tempo_push(mpb);
	tempo_show();
}

void MainWindow::on_edtBPM_returnPressed() {
	real bpm = ui->edtBPM->text().toFloat();
	real mpb = 60000 / bpm;
	tempo_push(mpb);
	ui->edtTempo->setText(fmt(mpb));
	ui->edtBPM->setText(fmt(bpm));
}

void MainWindow::sldVolume_valueChanged(int step) {
	volume.val = (step > 0) ? volume.fromstep(step) : 0;
	lpd.sendFloat(dest_vol, volume.val);
	ui->edtVolume->setText(fmt(volume.val));
}

void MainWindow::sldTempo_valueChanged(int step) {
	tempo.val = tempo.fromstep(step);
	lpd.sendFloat(dest_tempo, tempo.val);
	tempo_show();
}

void MainWindow::spnAccent1_valueChanged(int i) {
	lpd.sendFloat(dest_accent1, i);
	ui->spnAccent1->findChild<QLineEdit*>()->deselect();
}

void MainWindow::spnAccent2_valueChanged(int i) {
	lpd.sendFloat(dest_accent2, i);
	ui->spnAccent2->findChild<QLineEdit*>()->deselect();
}
