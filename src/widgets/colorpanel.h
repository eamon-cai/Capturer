#ifndef CAPTURER_COLOR_PANEL_H
#define CAPTURER_COLOR_PANEL_H

#include <QColorDialog>
#include <QPainter>
#include <QPushButton>

class ColorButton : public QPushButton
{
    Q_OBJECT

public:
    explicit ColorButton(QWidget * = nullptr);
    explicit ColorButton(const QColor&, QWidget * = nullptr);

    void color(const QColor& c, bool silence = true)
    {
        if (color_ == c) return;

        color_ = c;
        update();

        if (!silence) emit changed(color_);
    }

    [[nodiscard]] QColor color() const { return color_; }

signals:
    void clicked(const QColor&);
    void changed(const QColor&);

protected:
    void paintEvent(QPaintEvent *) override;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEnterEvent *) override;
#else
    void enterEvent(QEvent *) override;
#endif

    void leaveEvent(QEvent *) override;

protected:
    QColor color_{ Qt::blue };
    QColor default_color_{ "#bfbfbf" };
    QPen   border_pen_{ default_color_, 2 };
    QColor hover_color_{ "#2080f0" };
};

///////////////////////////////////////////////////////////////////////////
class ColorDialogButton : public ColorButton
{
    Q_OBJECT

public:
    explicit ColorDialogButton(QWidget * = nullptr);
    explicit ColorDialogButton(const QColor&, QWidget * = nullptr);
    ~ColorDialogButton() override;

protected:
    void wheelEvent(QWheelEvent *) override;

private:
    QColorDialog *color_dialog_{};
};

///////////////////////////////////////////////////////////////////////////
class ColorPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ColorPanel(QWidget * = nullptr);

    [[nodiscard]] QColor color() const { return color_dialog_btn_->color(); }

signals:
    void changed(const QColor&);

public slots:

    void setColor(const QColor& color, bool silence = true) { color_dialog_btn_->color(color, silence); }

private:
    ColorDialogButton *color_dialog_btn_{};
};

#endif //! CAPTURER_COLOR_PANEL_H
