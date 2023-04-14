#include "settingdialog.h"
#include <QSpinBox>
#include <QVBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QCheckBox>
#include <QSettings>
#include <QCoreApplication>
#include <QDir>
#include <QListWidget>
#include <QStackedWidget>
#include <QLabel>
#include "combobox.h"
#include "shortcutinput.h"
#include "colorpanel.h"
#include "titlebar.h"
#include "devices.h"
#include "logging.h"
#include "version.h"
#include "hwaccel.h"

SettingWindow::SettingWindow(QWidget* parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);

    setMinimumSize(850, 600);
    setContentsMargins(20, 20, 20, 20);

    auto shadow_layout = new QVBoxLayout();
    shadow_layout->setSpacing(0);
    shadow_layout->setContentsMargins({});
    auto window = new QWidget();
    window->setObjectName("settings-mainwindow");
    shadow_layout->addWidget(window);
    setLayout(shadow_layout);

    // shadow
    auto effect = new QGraphicsDropShadowEffect(this);
    effect->setBlurRadius(20);
    effect->setOffset(0);
    effect->setColor(QColor(0, 0, 0, 50));
    window->setGraphicsEffect(effect);

    //
    auto wrapper_layer = new QVBoxLayout();
    wrapper_layer->setSpacing(0);
    wrapper_layer->setContentsMargins({});
    window->setLayout(wrapper_layer);

    // title bar
    auto titlebar = new TitleBar();
    titlebar->setTitle(tr("Settings"));
    connect(titlebar, &TitleBar::close, this, &QWidget::close);
    connect(titlebar, &TitleBar::moved, [this](const QPoint& m) {
        move(pos() + m);
    });
    wrapper_layer->addWidget(titlebar);

    auto layout = new QHBoxLayout();
    layout->setSpacing(0);
    layout->setContentsMargins({});
    wrapper_layer->addLayout(layout);

    auto menu = new QListWidget();
    menu->setFocusPolicy(Qt::NoFocus);
    menu->addItem(new QListWidgetItem(tr("General")));
    menu->addItem(new QListWidgetItem(tr("Shortcuts")));
    menu->addItem(new QListWidgetItem(tr("Screenshot")));
    menu->addItem(new QListWidgetItem(tr("Screen Recording")));
    menu->addItem(new QListWidgetItem(tr("GIF Recording")));
    menu->addItem(new QListWidgetItem(tr("Devices")));
    menu->addItem(new QListWidgetItem(tr("About")));
    layout->addWidget(menu);

    pages_ = new QStackedWidget();
    layout->addWidget(pages_);

    pages_->addWidget(setupGeneralWidget());
    pages_->addWidget(setupHotkeyWidget());
    pages_->addWidget(setupSnipWidget());
    pages_->addWidget(setupRecordWidget());
    pages_->addWidget(setupGIFWidget());
    pages_->addWidget(setupDevicesWidget());
    pages_->addWidget(setupAboutWidget());

    connect(menu, &QListWidget::currentItemChanged, [=](auto current, auto) {
        pages_->setCurrentIndex(menu->row(current));
    });
    menu->setCurrentRow(0);
}

QWidget* SettingWindow::setupGeneralWidget()
{
    auto general_widget = new QWidget(pages_);

    auto layout = new QGridLayout();
    layout->setContentsMargins(35, 10, 35, 15);

    autorun_ = new QCheckBox();
    autorun_->setObjectName("autorun");
    autorun_->setChecked(config["autorun"].get<bool>());
    setAutoRun(autorun_->checkState());
    connect(autorun_, &QCheckBox::stateChanged, this, &SettingWindow::setAutoRun);
    layout->addWidget(new QLabel(tr("Run on Startup")), 0, 0, 1, 1);
    layout->addWidget(autorun_, 0, 1, 1, 2);

    //
    auto _1_2 = new ComboBox();
    _1_2->add({
                      {"en_US", "English"},
                      {"zh_CN", "简体中文"}
            })
            .select(config["language"].get<QString>())
            .onselected([this](auto value){
                config.set(config["language"], value.toString());
            });
    layout->addWidget(new QLabel(tr("Language")), 1, 0, 1, 1);
    layout->addWidget(_1_2, 1, 1, 1, 2);

    //
    auto _2_2 = new QLineEdit(config.getFilePath());
    _2_2->setContextMenuPolicy(Qt::NoContextMenu);
    _2_2->setFocusPolicy(Qt::NoFocus);
    layout->addWidget(new QLabel(tr("Settings File")), 2, 0, 1, 1);
    layout->addWidget(_2_2, 2, 1, 1, 2);

    //
    auto _3_2 = new ComboBox();
    _3_2->add({
                      {"auto", tr("Auto")},
                      {"dark", tr("Dark")},
                      {"light", tr("Light")}
            })
            .select(config["theme"].get<QString>())
            .onselected([this](auto value) {
                config.set_theme(value.toString().toStdString());
            });
    layout->addWidget(new QLabel(tr("Theme")), 3, 0, 1, 1);
    layout->addWidget(_3_2, 3, 1, 1, 2);

    layout->setRowStretch(4, 1);
    general_widget->setLayout(layout);

    return general_widget;
}

QWidget* SettingWindow::setupSnipWidget()
{
    auto snip_widget = new QWidget(pages_);

    auto layout = new QGridLayout();
    layout->setContentsMargins(35, 10, 35, 15);

    auto _0 = new QLabel(tr("Appearance:"));
    _0->setObjectName("sub-title");
    layout->addWidget(_0, 0, 1, 1, 1);

    auto _1_2 = new QSpinBox();
    _1_2->setMinimum(1);
    _1_2->setMaximum(20);
    _1_2->setContextMenuPolicy(Qt::NoContextMenu);
    _1_2->setValue(config["snip"]["selector"]["border"]["width"].get<int>());
    connect(_1_2, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](int w){
        config.set(config["snip"]["selector"]["border"]["width"], w);
    });
    layout->addWidget(new QLabel(tr("Border Width")), 1, 1, 1, 1);
    layout->addWidget(_1_2, 1, 2, 1, 2);

    auto _2_2 = new ColorDialogButton(config["snip"]["selector"]["border"]["color"].get<QColor>());
    connect(_2_2, &ColorDialogButton::changed, [this](auto&& c) { config.set(config["snip"]["selector"]["border"]["color"], c); });
    layout->addWidget(new QLabel(tr("Border Color")), 2, 1, 1, 1);
    layout->addWidget(_2_2, 2, 2, 1, 2);

    auto _3_2 = new ComboBox();
    _3_2->add({
                      "NoPen", "SolidLine", "DashLine", "DotLine", "DashDotLine", "DashDotDotLine", "CustomDashLine"
            })
            .select(config["snip"]["selector"]["border"]["style"].get<int>())
            .onselected([this](auto value){
                config.set(config["snip"]["selector"]["border"]["style"], value.toInt());
            });
    layout->addWidget(new QLabel(tr("Line Type")), 3, 1, 1, 1);
    layout->addWidget(_3_2, 3, 2, 1, 2);

    auto _4_2 = new ColorDialogButton(config["snip"]["selector"]["mask"]["color"].get<QColor>());
    connect(_4_2, &ColorDialogButton::changed, [this](auto&& c){ config.set(config["snip"]["selector"]["mask"]["color"], c); });
    layout->addWidget(new QLabel(tr("Mask Color")), 4, 1, 1, 1);
    layout->addWidget(_4_2, 4, 2, 1, 2);

    layout->setRowStretch(5, 1);
    snip_widget->setLayout(layout);
    
    return snip_widget;
}

QWidget* SettingWindow::setupRecordWidget()
{
    auto record_widget = new QWidget(pages_);

    auto layout = new QGridLayout();
    layout->setContentsMargins(35, 10, 35, 15);

    auto _0 = new QLabel(tr("Appearance:"));
    _0->setObjectName("sub-title");
    layout->addWidget(_0, 0, 1, 1, 1);

    auto _1_2 = new QSpinBox();
    _1_2->setMinimum(1);
    _1_2->setMaximum(20);
    _1_2->setContextMenuPolicy(Qt::NoContextMenu);
    _1_2->setValue(config["record"]["selector"]["border"]["width"].get<int>());
    connect(_1_2, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](int w){
        config.set(config["record"]["selector"]["border"]["width"], w);
    });
    layout->addWidget(new QLabel(tr("Border Width")), 1, 1, 1, 1);
    layout->addWidget(_1_2, 1, 2, 1, 2);

    auto _2_2 = new ColorDialogButton(config["record"]["selector"]["border"]["color"].get<QColor>());
    connect(_2_2, &ColorDialogButton::changed, [this](auto&& c){ config.set(config["record"]["selector"]["border"]["color"], c); });
    layout->addWidget(new QLabel(tr("Border Color")), 2, 1, 1, 1);
    layout->addWidget(_2_2, 2, 2, 1, 2);

    auto _3_2 = new ComboBox();
    _3_2->add({
                      "NoPen", "SolidLine", "DashLine", "DotLine", "DashDotLine", "DashDotDotLine", "CustomDashLine"
            })
            .select(config["record"]["selector"]["border"]["style"].get<int>())
            .onselected( [this](auto value){
                config.set(config["record"]["selector"]["border"]["style"], value.toInt());
            });
    layout->addWidget(new QLabel(tr("Line Type")), 3, 1, 1, 1);
    layout->addWidget(_3_2, 3, 2, 1, 2);

    auto _4_2 = new ColorDialogButton(config["record"]["selector"]["mask"]["color"].get<QColor>());
    connect(_4_2, &ColorDialogButton::changed, [this](auto&& c){ config.set(config["record"]["selector"]["mask"]["color"], c); });
    layout->addWidget(new QLabel(tr("Mask Color")), 4, 1, 1, 1);
    layout->addWidget(_4_2, 4, 2, 1, 2);

    auto _5_2 = new QCheckBox();
    _5_2->setChecked(config["record"]["box"].get<bool>());
    connect(_5_2, &QCheckBox::stateChanged, [this](int state) {
        config.set(config["record"]["box"], state == Qt::Checked);
    });
    layout->addWidget(new QLabel(tr("Show Region")), 5, 1, 1, 1);
    layout->addWidget(_5_2, 5, 2, 1, 2);

    layout->addWidget(new QLabel(), 6, 1, 1, 1);

    auto _5 = new QLabel(tr("Params:"));
    _5->setObjectName("sub-title");
    layout->addWidget(_5, 7, 1, 1, 1);

    auto _6_2 = new QSpinBox();
    _6_2->setContextMenuPolicy(Qt::NoContextMenu);
    _6_2->setValue(config["record"]["framerate"].get<int>());
    connect(_6_2, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](int w){
        config.set(config["record"]["framerate"], w);
    });
    layout->addWidget(new QLabel(tr("Framerate")), 8, 1, 1, 1);
    layout->addWidget(_6_2, 8, 2, 1, 2);

    auto _7_2 = new ComboBox();
    _7_2->add({
                      {"libx264", tr("Software x264 [H.264 / AVC]")},
                      {"libx265", tr("Software x265 [H.265 / HEVC]")}
              });
    if (hwaccel::is_support(AV_HWDEVICE_TYPE_CUDA)) {
        _7_2->add("h264_nvenc", tr("Hardware NVENC [H.264 / AVC]"));
        _7_2->add("hevc_nvenc", tr("Hardware NVENC [H.265 / HEVC]"));
    }
    _7_2->select(
                    config["record"]["encoder"].get<QString>()
            )
            .onselected([this](auto value) {
                config.set(config["record"]["encoder"], value.toString());
            });
    layout->addWidget(new QLabel(tr("Encoder")), 9, 1, 1, 1);
    layout->addWidget(_7_2, 9, 2, 1, 2);

    auto _8_2 = new ComboBox();
    _8_2->add({
                      {"high",      tr("High")},
                      {"medium",    tr("Medium")},
                      {"low",       tr("Low")}
            })
            .select(config["record"]["quality"].get<QString>())
            .onselected([this](auto value) {
                config.set(config["record"]["quality"], value.toString());
            });
    layout->addWidget(new QLabel(tr("Quality")), 10, 1, 1, 1);
    layout->addWidget(_8_2, 10, 2, 1, 2);

    layout->setRowStretch(11, 1);

    record_widget->setLayout(layout);
    
    return record_widget;
}

QWidget* SettingWindow::setupGIFWidget()
{
   auto gif_widget = new QWidget();

    auto layout = new QGridLayout();
    layout->setContentsMargins(35, 10, 35, 15);

    auto _0 = new QLabel(tr("Appearance:"));
    _0->setObjectName("sub-title");
    layout->addWidget(_0, 0, 1, 1, 1);

    auto _1_2 = new QSpinBox();
    _1_2->setMinimum(1);
    _1_2->setMaximum(20);
    _1_2->setValue(config["gif"]["selector"]["border"]["width"].get<int>());
    connect(_1_2, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](int w){
        config.set(config["gif"]["selector"]["border"]["width"], w);
    });
    layout->addWidget(new QLabel(tr("Border Width")), 1, 1, 1, 1);
    layout->addWidget(_1_2, 1, 2, 1, 2);

    auto _2_2 = new ColorDialogButton(config["gif"]["selector"]["border"]["color"].get<QColor>());
    connect(_2_2, &ColorDialogButton::changed, [this](auto&& c){ config.set(config["gif"]["selector"]["border"]["color"], c); });
    layout->addWidget(_2_2, 2, 2, 1, 2);
    layout->addWidget(new QLabel(tr("Border Color")), 2, 1, 1, 1);

    auto _3_2 = new ComboBox();
    _3_2->add({
                      "NoPen", "SolidLine", "DashLine", "DotLine", "DashDotLine", "DashDotDotLine", "CustomDashLine"
              })
            .select(config["gif"]["selector"]["border"]["style"].get<int>())
            .onselected([this](auto value){
                config.set(config["gif"]["selector"]["border"]["style"], value.toInt());
            });
    layout->addWidget(_3_2, 3, 2, 1, 2);
    layout->addWidget(new QLabel(tr("Line Type")), 3, 1, 1, 1);

    auto _4_2 = new ColorDialogButton(config["gif"]["selector"]["mask"]["color"].get<QColor>());
    connect(_4_2, &ColorDialogButton::changed, [this](auto&& c){ config.set(config["gif"]["selector"]["mask"]["color"], c); });
    layout->addWidget(_4_2, 4, 2, 1, 2);
    layout->addWidget(new QLabel(tr("Mask Color")), 4, 1, 1, 1);

    auto _5_2 = new QCheckBox();
    _5_2->setChecked(config["gif"]["box"].get<bool>());
    connect(_5_2, &QCheckBox::stateChanged, [this](int state) {
        config.set(config["gif"]["box"], state == Qt::Checked);
    });
    layout->addWidget(new QLabel(tr("Show Region")), 5, 1, 1, 1);
    layout->addWidget(_5_2, 5, 2, 1, 2);

    layout->addWidget(new QLabel(), 6, 1, 1, 1);

    auto _5 = new QLabel(tr("Params:"));
    _5->setObjectName("sub-title");
    layout->addWidget(_5, 7, 1, 1, 1);

    auto _7_2 = new QSpinBox();
    _7_2->setContextMenuPolicy(Qt::NoContextMenu);
    _7_2->setValue(config["gif"]["framerate"].get<int>());
    connect(_7_2, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](int w){
        config.set(config["gif"]["framerate"], w);
    });
    layout->addWidget(new QLabel(tr("Framerate")), 8, 1, 1, 1);
    layout->addWidget(_7_2, 8, 2, 1, 2);

    auto _8_2 = new ComboBox();
    _8_2->add({
                      {"high", tr("High")},
                      {"medium", tr("Medium")},
                      {"low", tr("Low")}
              })
            .select(config["gif"]["quality"].get<QString>())
            .onselected([this](auto value) {
                config.set(config["gif"]["quality"], value.toString());
            });
    layout->addWidget(new QLabel(tr("Quality")), 9, 1, 1, 1);
    layout->addWidget(_8_2, 9, 2, 1, 2);

    layout->setRowStretch(10, 1);
    gif_widget->setLayout(layout);
    
    return gif_widget;
}

QWidget* SettingWindow::setupDevicesWidget()
{
    auto devices_widget = new QWidget(pages_);

    auto layout = new QGridLayout();
    layout->setContentsMargins(35, 10, 35, 15);

    auto _1_2 = new ComboBox();
    _1_2->addItems(Devices::microphones());
    layout->addWidget(new QLabel(tr("Microphones")), 1, 1, 1, 1);
    connect(_1_2, &QComboBox::currentTextChanged, [this](QString s) {
        config.set(config["devices"]["microphones"], s);
    });
    layout->addWidget(_1_2, 1, 2, 1, 2);

    auto _2_2 = new ComboBox();
    _2_2->addItems(Devices::speakers());
    layout->addWidget(new QLabel(tr("Speakers")), 2, 1, 1, 1);
    connect(_2_2, &QComboBox::currentTextChanged, [this](QString s) {
        config.set(config["devices"]["speakers"], s);
    });
    layout->addWidget(_2_2, 2, 2, 1, 2);

    auto _3_2 = new ComboBox();
    _3_2->addItems(Devices::cameras());
    layout->addWidget(new QLabel(tr("Cameras")), 3, 1, 1, 1);
    connect(_3_2, &QComboBox::currentTextChanged, [this](QString s) {
        config.set(config["devices"]["cameras"], s);
    });
    layout->addWidget(_3_2, 3, 2, 1, 2);

    layout->setRowStretch(5, 1);

    devices_widget->setLayout(layout);
    
    return devices_widget;
}

QWidget* SettingWindow::setupHotkeyWidget()
{
    auto hotkey_widget = new QWidget(pages_);
    auto idx_row = 1;

    auto layout = new QGridLayout();
    layout->setContentsMargins(35, 10, 35, 15);
    layout->setVerticalSpacing(10);

    auto _1_2 = new ShortcutInput(config["snip"]["hotkey"].get<QKeySequence>());
    connect(_1_2, &ShortcutInput::changed, [this](auto&& ks){
        config.set(config["snip"]["hotkey"], ks);
    });
    layout->addWidget(new QLabel(tr("Screenshot")), idx_row, 1, 1, 1);
    layout->addWidget(_1_2, idx_row++, 2, 1, 3);

    auto _2_2 = new ShortcutInput(config["pin"]["hotkey"].get<QKeySequence>());
    connect(_2_2, &ShortcutInput::changed, [this](auto&& ks){
        config.set(config["pin"]["hotkey"], ks);
    });
    layout->addWidget(new QLabel(tr("Paste")), idx_row, 1, 1, 1);
    layout->addWidget(_2_2, idx_row++, 2, 1, 3);

    auto hide_show_images = new ShortcutInput(config["pin"]["visible"]["hotkey"].get<QKeySequence>());
    connect(hide_show_images, &ShortcutInput::changed, [this](auto&& ks){
        config.set(config["pin"]["visible"]["hotkey"], ks);
    });
    layout->addWidget(new QLabel(tr("Hide/Show All Images")), idx_row, 1, 1, 1);
    layout->addWidget(hide_show_images, idx_row++, 2, 1, 3);

    auto _3_2 = new ShortcutInput(config["record"]["hotkey"].get<QKeySequence>());
    connect(_3_2, &ShortcutInput::changed, [this](auto&& ks){
        config.set(config["record"]["hotkey"], ks);
    });
    layout->addWidget(new QLabel(tr("Video Recording")), idx_row, 1, 1, 1);
    layout->addWidget(_3_2, idx_row++, 2, 1, 3);

    auto _4_2 = new ShortcutInput(config["gif"]["hotkey"].get<QKeySequence>());
    connect(_4_2, &ShortcutInput::changed, [this](auto&& ks){
        config.set(config["gif"]["hotkey"], ks);
    });
    layout->addWidget(new QLabel(tr("Gif Recording")), idx_row, 1, 1, 1);
    layout->addWidget(_4_2, idx_row++, 2, 1, 3);

    layout->setRowStretch(idx_row, 1);
    hotkey_widget->setLayout(layout);

    return hotkey_widget;
}

QWidget* SettingWindow::setupAboutWidget()
{
    auto about_widget =  new QWidget(pages_);
    pages_->addWidget(about_widget);

    auto parent_layout = new QVBoxLayout();
    about_widget->setLayout(parent_layout);
    parent_layout->setContentsMargins(35, 10, 35, 15);

    /////
    auto layout = new QGridLayout();
    parent_layout->addLayout(layout);

    layout->addWidget(new QLabel(tr("Version")), 0, 0, 1, 1);
    auto version = new QLineEdit(CAPTURER_VERSION);
    version->setFocusPolicy(Qt::NoFocus);
    version->setAlignment(Qt::AlignCenter);
    layout->addWidget(version, 0, 1, 1, 2);

    /////
    parent_layout->addStretch();

    auto copyright_ = new QLabel(tr("Copyright © 2018 - 2023 ffiirree. All rights reserved"));
    copyright_->setObjectName("copyright-label");
    copyright_->setAlignment(Qt::AlignCenter);
    parent_layout->addWidget(copyright_);

    return about_widget;
}

void SettingWindow::setAutoRun(int statue)
{
    QString exec_path = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());

#ifdef _WIN32
    QSettings settings(R"(HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\Run)", QSettings::NativeFormat);
    settings.setValue("capturer_run", statue == Qt::Checked ? exec_path : "");
#elif __linux__
    std::string desktop_file = "/usr/share/applications/capturer.desktop";
    std::string autorun_dir = std::string{ ::getenv("HOME") } + "/.config/autostart";
    std::string autorun_file = autorun_dir + "/capturer.desktop";

    if (!std::filesystem::exists(desktop_file)) {
        LOG(WARNING) << "failed to set `autorun` since the '" << desktop_file << "' does not exists.";
        statue = Qt::Unchecked;
        autorun_->setCheckState(Qt::Unchecked);
    }

    if (statue == Qt::Checked) {
        if (std::filesystem::exists(desktop_file) && !std::filesystem::exists(autorun_file)) {
            if (!std::filesystem::exists(autorun_dir)) {
                std::filesystem::create_directories(autorun_dir);
            }
            std::filesystem::create_symlink(desktop_file, autorun_file);
        }
    }
    else {
        std::filesystem::remove(autorun_file);
    }
#endif

    config.set(config["autorun"], statue == Qt::Checked);
}
