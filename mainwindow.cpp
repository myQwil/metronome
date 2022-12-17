#include "mainwindow.h"
#include "ui_mainwindow.h"

using namespace pd;

enum {
	  samples = 1024
	, ticks = samples / 64
};

Slide volume = Slide(0.001, 1, 0.35); // volume slider parameters
Slide tempo  = Slide(2000, 20, 1000); // tempo slider parameters
int accent = 12; // number of beats between accented beats
int subacc = 4;  // number of beats between sub-accented beats

static inline QString fmt(real num)
{
	int precision = 5 - QString::number((int)num).length();
	return QString::number(num, 'f', precision);
}

static void callback(void *userdata, Uint8 *stream, int)
{
	PdBase *pd = (PdBase *)userdata;
	pd->processFloat(ticks, nullptr, (float *)stream);
}

const char *MainWindow::initAudio()
{
	if (SDL_Init(SDL_INIT_AUDIO) < 0) {
		return SDL_GetError();
	}
	SDL_AudioSpec want = {};
	SDL_GetDefaultAudioInfo(NULL, &have, 0);
	want.freq = have.freq;
	want.format = AUDIO_F32;
	want.channels = 2;
	want.samples = samples;
	want.callback = callback;
	want.userdata = &pd;

	dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
	if (!dev) {
		return SDL_GetError();
	}
	SDL_PauseAudioDevice(dev, 0);

	if (!pd.init(0, have.channels, have.freq)) {
		SDL_CloseAudioDevice(dev);
		return "Error initializing pd.";
	}
	pd.setReceiver(&rec);
	QString path = QCoreApplication::applicationDirPath() + "/../pd";
	patch = pd.openPatch("main.pd", path.toStdString());

	const std::string dlr = patch.dollarZeroStr();
	dest_vol    = dlr + "vol";
	dest_play   = dlr + "play";
	dest_met    = dlr + "met";
	dest_set    = dlr + "set";
	dest_accent = dlr + "accent";
	dest_subacc = dlr + "sub";

	pd.sendFloat(dest_accent, accent);
	pd.sendFloat(dest_subacc, subacc);
	pd.sendFloat(dest_vol, volume.val);
	pd.sendBang(dest_play);

	return 0;
}

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
{
	const char *err_msg = initAudio();
	if (err_msg) {
		std::cerr << err_msg << std::endl;
		QApplication::quit();
	}

	ui->setupUi(this);

	ui->sldVolume->setMaximum(run);
	ui->sldVolume->setValue(volume.tostep());
	connect(ui->sldVolume, SIGNAL(valueChanged(int))
		, this, SLOT(sldVolume_valueChanged(int)));

	ui->sldTempo->setMaximum(run);
	ui->sldTempo->setValue(tempo.tostep());
	connect(ui->sldTempo, SIGNAL(valueChanged(int))
		, this, SLOT(sldTempo_valueChanged(int)));

	ui->numVolume->setText(fmt(volume.val));
	ui->numVolume->setValidator(
		new QDoubleValidator(volume.min, volume.max, -1, this));

	ui->numTempo->setText(fmt(tempo.val));
	ui->numTempo->setValidator(
		new QDoubleValidator(tempo.min, tempo.max, -1, this));

	ui->numBPM->setText(fmt(60000 / tempo.val));
	ui->numBPM->setValidator(
		new QDoubleValidator(60000/tempo.max, 60000/tempo.min, -1, this));

	ui->spnAccent->setValue(accent);
	connect(ui->spnAccent, SIGNAL(valueChanged(int))
		, this, SLOT(spnAccent_valueChanged(int)), Qt::QueuedConnection);

	ui->spnSubacc->setValue(subacc);
	connect(ui->spnSubacc, SIGNAL(valueChanged(int))
		, this, SLOT(spnSubacc_valueChanged(int)), Qt::QueuedConnection);
}

MainWindow::~MainWindow()
{
	delete ui;
	pd.closePatch(patch);
	pd.computeAudio(false);
}


void MainWindow::tempo_show()
{
	ui->numTempo->setText(fmt(tempo.val));
	ui->numBPM->setText(fmt(60000 / tempo.val));
}

void MainWindow::tempo_push(real mpb)
{
	tempo.val = mpb;
	pd.sendFloat(dest_met, tempo.val);
	ui->sldTempo->blockSignals(true);
	ui->sldTempo->setValue(tempo.tostep());
	ui->sldTempo->blockSignals(false);
}

void MainWindow::on_btnSlow_pressed()
{
	tempo_push(1000);
	tempo_show();
}

void MainWindow::on_btnMedm_pressed()
{
	tempo_push(875);
	tempo_show();
}

void MainWindow::on_btnFast_pressed()
{
	tempo_push(750);
	tempo_show();
}

void MainWindow::on_btnReset_pressed()
{
	pd.sendFloat(dest_set, 0);
}

void MainWindow::on_chkPause_stateChanged(int paused)
{
	if (paused) {
		SDL_CloseAudioDevice(dev);
	} else {
		dev = SDL_OpenAudioDevice(NULL, 0, &have, nullptr, 0);
		SDL_PauseAudioDevice(dev, 0);
	}
}

void MainWindow::on_numVolume_returnPressed()
{
	volume.val = ui->numVolume->text().toFloat();
	pd.sendFloat(dest_vol, volume.val);
	ui->sldVolume->blockSignals(true);
	ui->sldVolume->setValue(volume.tostep());
	ui->sldVolume->blockSignals(false);
	ui->numVolume->setText(fmt(volume.val));
}

void MainWindow::on_numTempo_returnPressed()
{
	real mpb = ui->numTempo->text().toFloat();
	tempo_push(mpb);
	tempo_show();
}

void MainWindow::on_numBPM_returnPressed()
{
	real bpm = ui->numBPM->text().toFloat();
	real mpb = 60000 / bpm;
	tempo_push(mpb);
	ui->numTempo->setText(fmt(mpb));
	ui->numBPM->setText(fmt(bpm));
}

void MainWindow::sldVolume_valueChanged(int step)
{
	volume.val = (step > 0) ? volume.fromstep(step) : 0;
	pd.sendFloat(dest_vol, volume.val);
	ui->numVolume->setText(fmt(volume.val));
}

void MainWindow::sldTempo_valueChanged(int step)
{
	tempo.val = tempo.fromstep(step);
	pd.sendFloat(dest_met, tempo.val);
	tempo_show();
}

void MainWindow::spnAccent_valueChanged(int i)
{
	pd.sendFloat(dest_accent, i);
	ui->spnAccent->findChild<QLineEdit*>()->deselect();
}

void MainWindow::spnSubacc_valueChanged(int i)
{
	pd.sendFloat(dest_subacc, i);
	ui->spnSubacc->findChild<QLineEdit*>()->deselect();
}
