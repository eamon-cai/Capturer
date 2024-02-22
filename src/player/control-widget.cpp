#include "control-widget.h"

#include "combobox.h"
#include "titlebar.h"

#include <fmt/chrono.h>
#include <libcap/media.h>
#include <qabstractitemview.h>
#include <QVBoxLayout>

ControlWidget::ControlWidget(FramelessWindow *parent)
    : QWidget(parent)
{
    setMouseTracking(true);

    const auto layout = new QVBoxLayout();
    layout->setSpacing(0);
    layout->setContentsMargins({});
    setLayout(layout);

    // title bar
    {
        const auto title_bar = new TitleBar(parent);
        title_bar->setObjectName("title-bar");
        title_bar->setHideOnFullScreen(false);
        layout->addWidget(title_bar);
    }

    layout->addSpacerItem(new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding));

    // control
    {
        control_bar_ = new QWidget();
        control_bar_->setObjectName("control-bar");
        layout->addWidget(control_bar_);

        auto vl = new QVBoxLayout();
        vl->setSpacing(0);
        vl->setContentsMargins({ 10, 0, 10, 10 });
        connect(this, &ControlWidget::validDruation, [vl](auto valid) {
            vl->setContentsMargins({ 10, valid ? 0 : 10, 10, 10 });
        });
        control_bar_->setLayout(vl);

        time_slider_ = new Slider(Qt::Horizontal);
        time_slider_->setObjectName("time-slider");
        time_slider_->setMouseTracking(true);
        time_slider_->setRange(1, 100);
        connect(time_slider_, &Slider::seek, this, &ControlWidget::seek);
        connect(this, &ControlWidget::validDruation, time_slider_, &Slider::setVisible);
        vl->addWidget(time_slider_);

        const auto hl = new QHBoxLayout();
        hl->setSpacing(7);
        hl->setContentsMargins({});
        vl->addLayout(hl);

        pause_btn_ = new QCheckBox();
        pause_btn_->setObjectName("pause-btn");
        connect(pause_btn_, &QCheckBox::stateChanged, [this](int state) { state ? pause() : resume(); });
        connect(this, &ControlWidget::pause, [this] {
            if (!pause_btn_->isChecked()) pause_btn_->setChecked(true);
        });
        connect(this, &ControlWidget::resume, [this] {
            if (pause_btn_->isChecked()) pause_btn_->setChecked(false);
        });
        hl->addWidget(pause_btn_);

        // time
        time_label_ = new QLabel("--:--:--");
        time_label_->setObjectName("time-label");
        time_label_->setAlignment(Qt::AlignCenter);
        hl->addWidget(time_label_);

        const auto separator = new QLabel("/");
        connect(this, &ControlWidget::validDruation, separator, &QLabel::setVisible);
        hl->addWidget(separator);

        duration_label_ = new QLabel("--:--:--");
        duration_label_->setObjectName("duration-label");
        duration_label_->setAlignment(Qt::AlignCenter);
        connect(this, &ControlWidget::validDruation, duration_label_, &QLabel::setVisible);
        hl->addWidget(duration_label_);

        hl->addSpacerItem(new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Maximum));

        // subtitles
        // auto subtitles_btn = new QCheckBox();
        // subtitles_btn->setObjectName("subtitles-btn");
        // subtitles_btn->setCheckable(false);
        // hl->addWidget(subtitles_btn);

        // speed
        speed_box_ = new ComboBox();
        speed_box_->setObjectName("speed-box");
        speed_box_
            ->add({
                { 0.5, "0.5x" },
                { 0.75, "0.75x" },
                { 1.0, "1.0x" },
                { 1.25, "1.25x" },
                { 1.5, "1.5x" },
                { 2.0, "2.0x" },
                { 3.0, "3.0x" },
            })
            .onselected([this](const QVariant& v) { emit speed(v.toFloat()); })
            .select(1.0);
        connect(this, &ControlWidget::validDruation, speed_box_, &ComboBox::setVisible);

        hl->addWidget(speed_box_);

        // volume
        volume_btn_ = new QCheckBox();
        volume_btn_->setObjectName("volume-btn");
        connect(volume_btn_, &QCheckBox::toggled, this, &ControlWidget::mute);
        hl->addWidget(volume_btn_);

        volume_slider_ = new Slider(Qt::Horizontal);
        volume_slider_->setObjectName("volume-bar");
        volume_slider_->setFixedWidth(100);
        volume_slider_->setRange(0, 100);
        connect(volume_slider_, &Slider::valueChanged, [this](int v) {
            volume_btn_->setChecked(v == 0);
            emit volume(v);
        });
        hl->addWidget(volume_slider_);

        // settings
        // const auto setting_btn = new QCheckBox();
        // setting_btn->setObjectName("setting-btn");
        // setting_btn->setCheckable(false);
        // hl->addWidget(setting_btn);
    }
}

bool ControlWidget::hideable() const
{
    return !time_slider_->isSliderDown() && !volume_slider_->isSliderDown() &&
           !speed_box_->view()->isVisible();
}

void ControlWidget::setDuration(const int64_t duration)
{
    emit validDruation(duration >= 0);

    time_slider_->setMaximum(static_cast<int>(duration / 1000)); // ms
    duration_label_->setText(fmt::format("{:%T}", std::chrono::seconds{ duration / 1000000 }).c_str());
}

void ControlWidget::setTime(const int64_t ts)
{
    if (time_slider_ && !time_slider_->isSliderDown() && time_slider_->isVisible()) {
        time_slider_->setValue(static_cast<int>(ts / 1000)); // ms
    }
    time_label_->setText(fmt::format("{:%T}", std::chrono::seconds(ts / 1000000)).c_str());
}

void ControlWidget::setVolume(int v)
{
    v = std::clamp<int>(v, 0, volume_slider_->maximum());

    if (!volume_slider_->isSliderDown()) {
        volume_slider_->setValue(v);
    }
    volume_btn_->setChecked(v == 0);
}

void ControlWidget::setLiveMode(bool m)
{
    if (speed_box_) speed_box_->setVisible(!m);
}

void ControlWidget::setMute(bool muted) { volume_btn_->setChecked(muted); }

bool ControlWidget::paused() const { return pause_btn_->isChecked(); }