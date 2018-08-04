#include "screenshoter.h"
#include <QApplication>
#include <QScreen>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QClipboard>
#include <QMouseEvent>
#include <QStandardPaths>
#include <QShortcut>
#include <QDebug>

ScreenShoter::ScreenShoter(QWidget *parent)
    : Selector(parent)
{
    // menu
    menu_ = new MainMenu(this);
    gmenu_ = new GraphMenu(this);
    fmenu_ = new FontMenu(this);
    magnifier_ = new Magnifier(this);

    connect(menu_, &MainMenu::save, this, &ScreenShoter::save_image);
    connect(menu_, &MainMenu::copy, this, &ScreenShoter::copy2clipboard);
    connect(menu_, &MainMenu::fix, this, &ScreenShoter::pin_image);
    connect(menu_, &MainMenu::exit, this, &ScreenShoter::exit);

    connect(menu_, &MainMenu::undo, this, &ScreenShoter::undo);
    connect(menu_, &MainMenu::redo, this, &ScreenShoter::redo);

    connect(&undo_stack_, SIGNAL(changed(size_t)), this, SLOT(update()));
    connect(&undo_stack_, static_cast<void (CommandStack::*)(bool)>(&CommandStack::empty), [&](bool e) { status_ = e ? CAPTURED : LOCKED; });

    auto end_edit_functor = [=]() {
        if(undo_stack_.empty()){
            status_ = CAPTURED;
        }
        edit_status_ = NONE;

        gmenu_->hide();
        fmenu_->hide();
    };
    connect(menu_, &MainMenu::START_PAINT_RECTANGLE, [=]() { status_ = LOCKED;  edit_status_ = START_PAINTING_RECTANGLE; fmenu_->hide(); gmenu_->show(); });
    connect(menu_, &MainMenu::END_PAINT_RECTANGLE, end_edit_functor);

    connect(menu_, &MainMenu::START_PAINT_CIRCLE, [=]() { status_ = LOCKED;  edit_status_ = START_PAINTING_CIRCLE; fmenu_->hide(); gmenu_->show(); });
    connect(menu_, &MainMenu::END_PAINT_CIRCLE, end_edit_functor);

    connect(menu_, &MainMenu::START_PAINT_ARROW, [=]() { status_ = LOCKED;  edit_status_ = START_PAINTING_ARROW; fmenu_->hide(); gmenu_->show(); });
    connect(menu_, &MainMenu::END_PAINT_ARROW, end_edit_functor);

    connect(menu_, &MainMenu::START_PAINT_LINE, [=]() { status_ = LOCKED;  edit_status_ = START_PAINTING_LINE; fmenu_->hide(); gmenu_->show(); });
    connect(menu_, &MainMenu::END_PAINT_LINE, end_edit_functor);

    connect(menu_, &MainMenu::START_PAINT_CURVES, [=]() { status_ = LOCKED;  edit_status_ = START_PAINTING_CURVES; fmenu_->hide(); gmenu_->show(); });
    connect(menu_, &MainMenu::END_PAINT_CURVES, end_edit_functor);

    connect(menu_, &MainMenu::START_PAINT_TEXT, [=]() { status_ = LOCKED;  edit_status_ = START_PAINTING_TEXT; gmenu_->hide(); fmenu_->show(); });
    connect(menu_, &MainMenu::END_PAINT_TEXT, end_edit_functor);

    // graph menu
    connect(gmenu_, &GraphMenu::setWidth, [=](int width){ pen_width_ = width; fill_ = false; });
    connect(gmenu_, &GraphMenu::setFill, [=](bool fill){ fill_ = fill; });
    connect(gmenu_, &GraphMenu::setColor, [=](const QColor& color) { color_ = color; });

    // font menu
    connect(fmenu_, &FontMenu::familyChanged, [=](const QString &family){ font_family_ = family; });
    connect(fmenu_, &FontMenu::styleChanged, [=](const QString &style){ font_style_ = style; });
    connect(fmenu_, &FontMenu::sizeChanged, [=](int s){ font_size_ = s; });
    connect(fmenu_, &FontMenu::colorChanged, [=](const QColor& color){ font_color_ = color; });

    // move menu
    connect(this, &ScreenShoter::moved, this, &ScreenShoter::updateMenuPosition);
    connect(this, &ScreenShoter::resized, this, &ScreenShoter::updateMenuPosition);

    //
    registerShortcuts();
}

void ScreenShoter::start()
{
    if(status_ == INITIAL)
        captured_screen_ = QGuiApplication::primaryScreen()->grabWindow(QApplication::desktop()->winId());

    Selector::start();
}

void ScreenShoter::mousePressEvent(QMouseEvent *event)
{
    auto mouse_pos = event->pos();
    if(status_ == LOCKED) {
        // BORDER
        for(auto& command: undo_stack_.commands()) {
            switch (command->type()) {
            case Command::DRAW_RECTANGLE:
            {
                Resizer resizer(command->points()[0], command->points()[1]);
                if(resizer.isX1Y1Anchor(mouse_pos)){
                    setCursor(Qt::SizeFDiagCursor);
                    resize_begin_ = mouse_pos;

                    command_ = command;
                    focus_ = command_;

                    last_edit_status_ = edit_status_;
                    edit_status_ = GRAPH_RESIZING;
                }
                else if(resizer.isY1Anchor(mouse_pos)) {
                    setCursor(Qt::SizeVerCursor);

                    resize_begin_ = mouse_pos;

                    command_ = command;
                    focus_ = command_;

                    last_edit_status_ = edit_status_;
                    edit_status_ = GRAPH_RESIZING;
                }
                else if(resizer.isX1Anchor(mouse_pos)) {
                    setCursor(Qt::SizeHorCursor);

                    resize_begin_ = mouse_pos;

                    command_ = command;
                    focus_ = command_;

                    last_edit_status_ = edit_status_;
                    edit_status_ = GRAPH_RESIZING;
                }
                else if(resizer.isY2Anchor(mouse_pos)) {
                    setCursor(Qt::SizeVerCursor);

                    resize_begin_ = mouse_pos;

                    command_ = command;
                    focus_ = command_;

                    last_edit_status_ = edit_status_;
                    edit_status_ = GRAPH_RESIZING;
                }
                else if(resizer.isX2Anchor(mouse_pos)) {
                    setCursor(Qt::SizeHorCursor);

                    resize_begin_ = mouse_pos;

                    command_ = command;
                    focus_ = command_;

                    last_edit_status_ = edit_status_;
                    edit_status_ = GRAPH_RESIZING;
                }
                else if(resizer.isX2Y1Anchor(mouse_pos)) {
                    setCursor(Qt::SizeBDiagCursor);

                    resize_begin_ = mouse_pos;

                    command_ = command;
                    focus_ = command_;

                    last_edit_status_ = edit_status_;
                    edit_status_ = GRAPH_RESIZING;
                }
                else if(resizer.isX1Y2Anchor(mouse_pos)) {
                    setCursor(Qt::SizeBDiagCursor);

                    resize_begin_ = mouse_pos;

                    command_ = command;
                    focus_ = command_;

                    last_edit_status_ = edit_status_;
                    edit_status_ = GRAPH_RESIZING;
                }
                else if(resizer.isX2Y2Anchor(mouse_pos)) {
                    setCursor(Qt::SizeFDiagCursor);

                    resize_begin_ = mouse_pos;

                    command_ = command;
                    focus_ = command_;

                    last_edit_status_ = edit_status_;
                    edit_status_ = GRAPH_RESIZING;
                }
                else if(resizer.isBorder(mouse_pos)) {
                    setCursor(Qt::SizeAllCursor);
                    move_begin_ = mouse_pos;

                    command_ = command;
                    focus_ = command_;

                    last_edit_status_ = edit_status_;
                    edit_status_ = GRAPH_MOVING;
                }
                this->update();
                break;
            }

            case Command::DRAW_CIRCLE:
            {
                QRegion r1(QRect(command->points()[0] - QPoint(2, 2), command->points()[1] + QPoint(2, 2)), QRegion::Ellipse);
                QRegion r2(QRect(command->points()[0] + QPoint(2, 2), command->points()[1] - QPoint(2, 2)), QRegion::Ellipse);


                Resizer resizer(command->points()[0], command->points()[1]);
                if(resizer.isX1Y1Anchor(mouse_pos)){
                    setCursor(Qt::SizeFDiagCursor);
                    resize_begin_ = mouse_pos;

                    command_ = command;
                    focus_ = command_;

                    last_edit_status_ = edit_status_;
                    edit_status_ = GRAPH_RESIZING;
                }
                else if(resizer.isY1Anchor(mouse_pos)) {
                    setCursor(Qt::SizeVerCursor);

                    resize_begin_ = mouse_pos;

                    command_ = command;
                    focus_ = command_;

                    last_edit_status_ = edit_status_;
                    edit_status_ = GRAPH_RESIZING;
                }
                else if(resizer.isX1Anchor(mouse_pos)) {
                    setCursor(Qt::SizeHorCursor);

                    resize_begin_ = mouse_pos;

                    command_ = command;
                    focus_ = command_;

                    last_edit_status_ = edit_status_;
                    edit_status_ = GRAPH_RESIZING;
                }
                else if(resizer.isY2Anchor(mouse_pos)) {
                    setCursor(Qt::SizeVerCursor);

                    resize_begin_ = mouse_pos;

                    command_ = command;
                    focus_ = command_;

                    last_edit_status_ = edit_status_;
                    edit_status_ = GRAPH_RESIZING;
                }
                else if(resizer.isX2Anchor(mouse_pos)) {
                    setCursor(Qt::SizeHorCursor);

                    resize_begin_ = mouse_pos;

                    command_ = command;
                    focus_ = command_;

                    last_edit_status_ = edit_status_;
                    edit_status_ = GRAPH_RESIZING;
                }
                else if(resizer.isX2Y1Anchor(mouse_pos)) {
                    setCursor(Qt::SizeBDiagCursor);

                    resize_begin_ = mouse_pos;

                    command_ = command;
                    focus_ = command_;

                    last_edit_status_ = edit_status_;
                    edit_status_ = GRAPH_RESIZING;
                }
                else if(resizer.isX1Y2Anchor(mouse_pos)) {
                    setCursor(Qt::SizeBDiagCursor);

                    resize_begin_ = mouse_pos;

                    command_ = command;
                    focus_ = command_;

                    last_edit_status_ = edit_status_;
                    edit_status_ = GRAPH_RESIZING;
                }
                else if(resizer.isX2Y2Anchor(mouse_pos)) {
                    setCursor(Qt::SizeFDiagCursor);

                    resize_begin_ = mouse_pos;

                    command_ = command;
                    focus_ = command_;

                    last_edit_status_ = edit_status_;
                    edit_status_ = GRAPH_RESIZING;
                }
                else if(r1.contains(mouse_pos) && !r2.contains(mouse_pos)) {
                    setCursor(Qt::SizeAllCursor);

                    move_begin_ = mouse_pos;

                    command_ = command;
                    focus_ = command_;

                    last_edit_status_ = edit_status_;
                    edit_status_ = GRAPH_MOVING;
                }
                this->update();
                break;
            }

            case Command::DRAW_BROKEN_LINE:
            {
                QLine line(command->points()[0], command->points()[1]);
                QRect area(command->points()[0], command->points()[1]);
                float k = (float)line.dy()/line.dx();
                float b = command->points()[0].y() - k * command->points()[0].x();
                auto diff = mouse_pos.x() * k + b - mouse_pos.y();
                if(diff >= -2 && diff <= 2 && area.contains(mouse_pos)) {
                    setCursor(Qt::SizeAllCursor);

                    move_begin_ = mouse_pos;

                    command_ = command;
                    focus_ = command_;

                    last_edit_status_ = edit_status_;
                    edit_status_ = GRAPH_MOVING;
                }
                this->update();
                break;
            }
            default: break;
            }
        }

        // NOT BORDER & ACHOR
        switch (edit_status_) {
        case START_PAINTING_RECTANGLE:
        case END_PAINTING_RECTANGLE: rectangle_end_ = rectangle_begin_ = event->pos(); edit_status_ = PAINTING_RECTANGLE; break;
        case START_PAINTING_CIRCLE:
        case END_PAINTING_CIRCLE: circle_end_ = circle_begin_ = event->pos(); edit_status_ = PAINTING_CIRCLE; break;
        case START_PAINTING_ARROW:
        case END_PAINTING_ARROW: arrow_end_ = arrow_begin_ = event->pos(); edit_status_ = PAINTING_ARROW; break;
        case START_PAINTING_LINE:
        case END_PAINTING_LINE: line_end_ = line_begin_ = event->pos(); edit_status_ = PAINTING_LINE; break;
        case START_PAINTING_CURVES:
        case END_PAINTING_CURVES: curves_.push_back(event->pos()); edit_status_ = PAINTING_CURVES; break;
        case START_PAINTING_TEXT:
        case PAINTING_TEXT:
        case END_PAINTING_TEXT:
            edit_status_ = PAINTING_TEXT;
            text_pos_ = event->pos();

            if(!text_edit_ || !text_edit_->toPlainText().isEmpty()) {
                text_edit_ = new TextEdit(this);
            }
            text_edit_->setFocus();
            text_edit_->show();
            text_edit_->move(text_pos_);

            connect(text_edit_, &TextEdit::focus, [=](TextEdit* that) {
                return [=](bool f) {
                    if(!f && !that->toPlainText().isEmpty()) {
                        auto command = new Command(Command::DRAW_TEXT, QPen(font_color_));
                        command->widget(that);
                        DO(command);
                    }
                };
            }(text_edit_));
            break;
        default:
            break;
        }
    }

    Selector::mousePressEvent(event);
}

void ScreenShoter::mouseMoveEvent(QMouseEvent* event)
{
    auto mouse_pos = event->pos();

    if(status_ == LOCKED) {
        switch (edit_status_) {
        case PAINTING_RECTANGLE: rectangle_end_ = event->pos(); setCursor(Qt::CrossCursor); break;
        case PAINTING_CIRCLE: circle_end_ = event->pos(); setCursor(Qt::CrossCursor); break;
        case PAINTING_ARROW: arrow_end_ = event->pos(); setCursor(Qt::CrossCursor); break;
        case PAINTING_LINE: line_end_ = event->pos(); setCursor(Qt::CrossCursor); break;
        case PAINTING_CURVES: curves_.push_back(event->pos()); setCursor(Qt::CrossCursor); break;
        case PAINTING_TEXT: setCursor(Qt::IBeamCursor); break;
        case GRAPH_MOVING:
        {
            move_end_ = mouse_pos;

            auto move_x = move_end_.x() - move_begin_.x();
            auto move_y = move_end_.y() - move_begin_.y();
            command_->points()[0] += QPoint{move_x, move_y};
            command_->points()[1] += QPoint{move_x, move_y};

            move_begin_ = move_end_;
            break;
        }

        case GRAPH_RESIZING:
        {
            resize_end_ = mouse_pos;

            auto rx = resize_end_.x() - resize_begin_.x();
            auto ry = resize_end_.y() - resize_begin_.y();

            switch (cursor_graph_pos_) {
            case Resizer::X1Y1_ANCHOR: command_->points()[0].rx() += rx; command_->points()[0].ry() += ry; break;
            case Resizer::X2Y1_ANCHOR: command_->points()[1].rx() += rx; command_->points()[0].ry() += ry; break;
            case Resizer::X1Y2_ANCHOR: command_->points()[0].rx() += rx; command_->points()[1].ry() += ry; break;
            case Resizer::X2Y2_ANCHOR: command_->points()[1].rx() += rx; command_->points()[1].ry() += ry; break;
            case Resizer::X1_ANCHOR: command_->points()[0].rx() += rx; break;
            case Resizer::X2_ANCHOR: command_->points()[1].rx() += rx; break;
            case Resizer::Y1_ANCHOR: command_->points()[0].ry() += ry; break;
            case Resizer::Y2_ANCHOR: command_->points()[1].ry() += ry; break;
            default: break;
            }

            resize_begin_ = mouse_pos;

            break;
        }
        default:
            for(auto& command: undo_stack_.commands()) {
                switch (command->type()) {
                case Command::DRAW_RECTANGLE:
                {
                    cursor_graph_pos_ = Resizer(command->points()[0], command->points()[1]).position(mouse_pos);

                    Resizer resizer(command->points()[0], command->points()[1]);
                    if(resizer.isX1Y1Anchor(mouse_pos)){
                        setCursor(Qt::SizeFDiagCursor);
                        goto end;
                    }
                    else if(resizer.isY1Anchor(mouse_pos)) {
                        setCursor(Qt::SizeVerCursor);
                        goto end;
                    }
                    else if(resizer.isX1Anchor(mouse_pos)) {
                        setCursor(Qt::SizeHorCursor);
                        goto end;
                    }
                    else if(resizer.isY2Anchor(mouse_pos)) {
                        setCursor(Qt::SizeVerCursor);
                        goto end;
                    }
                    else if(resizer.isX2Anchor(mouse_pos)) {
                        setCursor(Qt::SizeHorCursor);
                        goto end;
                    }
                    else if(resizer.isX2Y1Anchor(mouse_pos)) {
                        setCursor(Qt::SizeBDiagCursor);
                        goto end;
                    }
                    else if(resizer.isX1Y2Anchor(mouse_pos)) {
                        setCursor(Qt::SizeBDiagCursor);
                        goto end;
                    }
                    else if(resizer.isX2Y2Anchor(mouse_pos)) {
                        setCursor(Qt::SizeFDiagCursor);
                        goto end;
                    }
                    else if(resizer.isBorder(mouse_pos)) {
                        setCursor(Qt::SizeAllCursor);
                        goto end;
                    }
                    else {
                        setCursor(Qt::CrossCursor);
                    }

                    break;
                }

                case Command::DRAW_CIRCLE:
                {
                    cursor_graph_pos_ = Resizer(command->points()[0], command->points()[1]).position(mouse_pos);

                    QRegion r1(QRect(command->points()[0] - QPoint(2, 2), command->points()[1] + QPoint(2, 2)), QRegion::Ellipse);
                    QRegion r2(QRect(command->points()[0] + QPoint(2, 2), command->points()[1] - QPoint(2, 2)), QRegion::Ellipse);

                    Resizer resizer(command->points()[0], command->points()[1]);
                    if(resizer.isX1Y1Anchor(mouse_pos)){
                        setCursor(Qt::SizeFDiagCursor);
                        goto end;
                    }
                    else if(resizer.isY1Anchor(mouse_pos)) {
                        setCursor(Qt::SizeVerCursor);
                        goto end;
                    }
                    else if(resizer.isX1Anchor(mouse_pos)) {
                        setCursor(Qt::SizeHorCursor);
                        goto end;
                    }
                    else if(resizer.isY2Anchor(mouse_pos)) {
                        setCursor(Qt::SizeVerCursor);
                        goto end;
                    }
                    else if(resizer.isX2Anchor(mouse_pos)) {
                        setCursor(Qt::SizeHorCursor);
                        goto end;
                    }
                    else if(resizer.isX2Y1Anchor(mouse_pos)) {
                        setCursor(Qt::SizeBDiagCursor);
                        goto end;
                    }
                    else if(resizer.isX1Y2Anchor(mouse_pos)) {
                        setCursor(Qt::SizeBDiagCursor);
                        goto end;
                    }
                    else if(resizer.isX2Y2Anchor(mouse_pos)) {
                        setCursor(Qt::SizeFDiagCursor);
                        goto end;
                    }
                    else if(r1.contains(mouse_pos) && !r2.contains(mouse_pos)) {
                        setCursor(Qt::SizeAllCursor);
                        goto end;
                    }
                    else {
                        setCursor(Qt::CrossCursor);
                    }
                    break;
                }

                case Command::DRAW_BROKEN_LINE:
                {
                    QLine line(command->points()[0], command->points()[1]);
                    QRect area(command->points()[0], command->points()[1]);
                    float k = (float)line.dy()/line.dx();
                    float b = command->points()[0].y() - k * command->points()[0].x();
                    auto diff = mouse_pos.x() * k + b - mouse_pos.y();
                    if(diff >= -2 && diff <= 2 && area.contains(mouse_pos)) {
                        setCursor(Qt::SizeAllCursor);
                        goto end;
                    }
                    else {
                        setCursor(Qt::CrossCursor);
                    }
                    break;
                }

                default:
                    break;
                }
            }
            end:
            break;
        }
    }

    Selector::mouseMoveEvent(event);
}

void ScreenShoter::mouseReleaseEvent(QMouseEvent *event)
{
    if(status_ == LOCKED) {
        switch (edit_status_) {
        case PAINTING_RECTANGLE:
        {
            if(rectangle_begin_ != rectangle_end_)
                DO(new Command(Command::DRAW_RECTANGLE, QPen(color_, pen_width_, Qt::SolidLine), QPoint(), fill_, { rectangle_begin_, rectangle_end_ }));
            edit_status_ = END_PAINTING_RECTANGLE;

            break;
        }

        case PAINTING_CIRCLE:
        {
            if(circle_begin_ != circle_end_)
                DO(new Command(Command::DRAW_CIRCLE, QPen(color_, pen_width_, Qt::SolidLine), QPoint(), fill_, { circle_begin_, circle_end_ }));
            edit_status_ = END_PAINTING_CIRCLE;

            break;
        }

        case PAINTING_ARROW:
        {
            if(arrow_begin_ != arrow_end_)
                DO(new Command(Command::DRAW_ARROW, QPen(color_, 1, Qt::SolidLine), QPoint(), fill_, { arrow_begin_, arrow_end_ }));
            edit_status_ = END_PAINTING_ARROW;

            break;
        }

        case PAINTING_LINE:
        {
            if(line_begin_ != line_end_)
                DO(new Command(Command::DRAW_BROKEN_LINE, QPen(color_, pen_width_, Qt::SolidLine), QPoint(), false, { line_begin_, line_end_ }));

            line_end_ = event->pos();
            edit_status_ = END_PAINTING_LINE;

            break;
        }

        case PAINTING_CURVES:
        {
            auto command = new Command(Command::DRAW_CURVE, QPen(color_, pen_width_, Qt::SolidLine, Qt::FlatCap));
            command->points(curves_);
            curves_.clear();
            DO(command);

            line_end_ = event->pos();
            edit_status_ = END_PAINTING_CURVES;
            break;
        }

        case GRAPH_MOVING:
        {
            move_end_ = event->pos();

            auto move_x = move_end_.x() - move_begin_.x();
            auto move_y = move_end_.y() - move_begin_.y();
            command_->points()[0] += QPoint{move_x, move_y};
            command_->points()[1] += QPoint{move_x, move_y};

            edit_status_ = last_edit_status_;
            break;
        }

        case GRAPH_RESIZING:
        {
            resize_end_ = event->pos();

            auto rx = resize_end_.x() - resize_begin_.x();
            auto ry = resize_end_.y() - resize_begin_.y();

            switch (cursor_graph_pos_) {
            case Resizer::X1Y1_ANCHOR: command_->points()[0].rx() += rx; command_->points()[0].ry() += ry; break;
            case Resizer::X2Y1_ANCHOR: command_->points()[1].rx() += rx; command_->points()[0].ry() += ry; break;
            case Resizer::X1Y2_ANCHOR: command_->points()[0].rx() += rx; command_->points()[1].ry() += ry; break;
            case Resizer::X2Y2_ANCHOR: command_->points()[1].rx() += rx; command_->points()[1].ry() += ry; break;
            case Resizer::X1_ANCHOR: command_->points()[0].rx() += rx; break;
            case Resizer::X2_ANCHOR: command_->points()[1].rx() += rx; break;
            case Resizer::Y1_ANCHOR: command_->points()[0].ry() += ry; break;
            case Resizer::Y2_ANCHOR: command_->points()[1].ry() += ry; break;
            default: break;
            }

            resize_begin_ = event->pos();

            edit_status_ = last_edit_status_;
            break;
        }

        default: break;
        }
    }

    Selector::mouseReleaseEvent(event);
}

void ScreenShoter::keyPressEvent(QKeyEvent *event)
{
    if((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) && status_ >= CAPTURED) {
        copy2clipboard();
        exit();
    }

    if(event->key() == Qt::Key_Escape) {
        exit();
    }

    Selector::keyPressEvent(event);
}

void ScreenShoter::updateMenuPosition()
{
    if(status_ <  CAPTURED) return;

    auto area = selected();

    menu_->show();

    menu_->height() * 2 + area.bottomRight().y() > rect().height()
        ? menu_->move(area.topRight().x() - menu_->width() + 1, area.topRight().y() + 1) // topright
        : menu_->move(area.bottomRight().x() - menu_->width() + 1, area.bottomRight().y() + 3); // bottomright

    gmenu_->move(menu_->pos().x(), menu_->pos().y() + menu_->height() + 1);
    fmenu_->move(menu_->pos().x(), menu_->pos().y() + menu_->height() + 1);
}

void ScreenShoter::upadateMagnifierPosition()
{
    if(status_ != LOCKED) {
        magnifier_->pixmap(captured_screen_.copy(magnifier_->mrect()));
        magnifier_->show();
        magnifier_->move(QCursor::pos().x() + 10, QCursor::pos().y() + 10);
    }
    else {
        magnifier_->hide();;
    }
}

void ScreenShoter::exit()
{
    undo_stack_.clear();

    menu_->hide();
    gmenu_->hide();
    fmenu_->hide();

    Selector::exit();
}

void ScreenShoter::paintEvent(QPaintEvent *event)
{
    updateMenuPosition();
    upadateMagnifierPosition();

    QRect area = selected();
    auto background = captured_screen_.copy();

    painter_.begin(this);
    painter_.setRenderHint(QPainter::Antialiasing, false);

    if(status_ == LOCKED) {
        QPainter edit_painter;
        edit_painter.begin(&background);
        edit_painter.setPen(QPen(color_, 1, Qt::DashDotLine));

        //////////////////////////////////////////////////////////////////////////////////////////////////////
        // track
        switch(edit_status_) {
        case PAINTING_RECTANGLE:
            edit_painter.drawRect(QRect(rectangle_begin_, rectangle_end_));
            break;

        case PAINTING_CIRCLE:
            edit_painter.setRenderHint(QPainter::Antialiasing, true);
            edit_painter.drawEllipse(QRect(circle_begin_, circle_end_));
            break;

        case PAINTING_ARROW:
        {
            QPoint points[6];
            getArrowPoints(arrow_begin_, arrow_end_, points);
            edit_painter.drawPolygon(points, 6);

            break;
        }

        case PAINTING_LINE:
            edit_painter.drawLine(line_begin_, line_end_);
            break;

        case PAINTING_CURVES:
            edit_painter.setRenderHint(QPainter::Antialiasing, true);
            for(size_t i = 0; i < curves_.size() - 1; ++i) {
                edit_painter.drawLine(curves_[i], curves_[i + 1]);
            }
            break;

        case PAINTING_TEXT:
        {
            QFont font;
            font.setFamily(font_family_);
            font.setStyleName(font_style_);
            font.setPointSize(font_size_);
            text_edit_->setTextColor(font_color_);
            text_edit_->setFont(font);

            QRect text_rect(text_pos_, text_edit_->size());
            edit_painter.drawRect(text_rect);

            break;
        }

        default: break;
        }

        edit_painter.end();
        //////////////////////////////////////////////////////////////////////////////////////////////////////
        // final
        for(auto& command: undo_stack_.commands()) {
            edit_painter.begin(&background);
            edit_painter.setPen(command->pen());
            edit_painter.setBrush(Qt::NoBrush);

            switch (command->type()) {
            case Command::DRAW_RECTANGLE:
            {

                edit_painter.setRenderHint(QPainter::Antialiasing, false);
                if(command->isFill()) edit_painter.setBrush(command->pen().color());

                edit_painter.drawRect(QRect(command->points()[0], command->points()[1]));
                edit_painter.end();

                if(focus_ == command) {
                    QPainter tip_painter;
                    tip_painter.begin(&background);
                    tip_painter.setPen(QPen(Qt::black, 1, Qt::DashLine));
                    tip_painter.setRenderHint(QPainter::Antialiasing, false);
                    tip_painter.drawRect(QRect(command->points()[0], command->points()[1]));

                    Resizer rectangle_resizer(command->points()[0], command->points()[1]);

                    tip_painter.setPen(QPen(Qt::black, 1, Qt::SolidLine));
                    tip_painter.drawRect(rectangle_resizer.X1Anchor());
                    tip_painter.drawRect(rectangle_resizer.Y1Anchor());
                    tip_painter.drawRect(rectangle_resizer.X2Anchor());
                    tip_painter.drawRect(rectangle_resizer.Y2Anchor());
                    tip_painter.drawRect(rectangle_resizer.X1Y1Anchor());
                    tip_painter.drawRect(rectangle_resizer.X2Y1Anchor());
                    tip_painter.drawRect(rectangle_resizer.X1Y2Anchor());
                    tip_painter.drawRect(rectangle_resizer.X2Y2Anchor());

                    tip_painter.end();
                }
                break;
            }

            case Command::DRAW_CIRCLE:
                edit_painter.setRenderHint(QPainter::Antialiasing, true);
                if(command->isFill()) edit_painter.setBrush(command->pen().color());

                edit_painter.drawEllipse(QRect(command->points()[0], command->points()[1]));

                edit_painter.end();

                if(focus_ == command) {
                    QPainter tip_painter;
                    tip_painter.begin(&background);
                    tip_painter.setPen(QPen(Qt::black, 1, Qt::DashLine));
                    tip_painter.setRenderHint(QPainter::Antialiasing, true);
                    tip_painter.drawRect(QRect(command->points()[0], command->points()[1]));

                    Resizer circle_resizer(command->points()[0], command->points()[1]);

                    tip_painter.setPen(QPen(Qt::black, 1, Qt::SolidLine));
                    tip_painter.drawRect(circle_resizer.X1Anchor());
                    tip_painter.drawRect(circle_resizer.Y1Anchor());
                    tip_painter.drawRect(circle_resizer.X2Anchor());
                    tip_painter.drawRect(circle_resizer.Y2Anchor());
                    tip_painter.drawRect(circle_resizer.X1Y1Anchor());
                    tip_painter.drawRect(circle_resizer.X2Y1Anchor());
                    tip_painter.drawRect(circle_resizer.X1Y2Anchor());
                    tip_painter.drawRect(circle_resizer.X2Y2Anchor());

                    tip_painter.end();
                }
                break;

            case Command::DRAW_BROKEN_LINE:
            {
                edit_painter.setRenderHint(QPainter::Antialiasing, true);
                edit_painter.drawLine(command->points()[0], command->points()[1]);
                edit_painter.end();

                if(focus_ == command) {
                    QPainter tip_painter;
                    tip_painter.begin(&background);
                    tip_painter.setPen(QPen(Qt::black, 1, Qt::DashLine));
                    tip_painter.setRenderHint(QPainter::Antialiasing, true);
                    tip_painter.drawLine(command->points()[0], command->points()[1]);

                    tip_painter.setPen(QPen(Qt::black, 1, Qt::SolidLine));
                    tip_painter.drawRect(QRect(command->points()[0] - QPoint(2, 2), command->points()[0] + QPoint(2, 2)));
                    tip_painter.drawRect(QRect(command->points()[1] - QPoint(2, 2), command->points()[1] + QPoint(2, 2)));
                    tip_painter.end();
                }
                break;
            }

            case Command::DRAW_ARROW:
            {
                edit_painter.setRenderHint(QPainter::Antialiasing, true);
                QPoint points[6];
                getArrowPoints(command->points()[0], command->points()[1], points);
                edit_painter.setBrush(command->pen().color());
                edit_painter.drawPolygon(points, 6);

                edit_painter.end();

                if(focus_ == command) {
                    QPainter tip_painter;
                    tip_painter.begin(&background);
                    tip_painter.setPen(QPen(Qt::black, 1, Qt::DashLine));
                    tip_painter.setRenderHint(QPainter::Antialiasing, true);
                    tip_painter.drawLine(command->points()[0], command->points()[1]);

                    tip_painter.setPen(QPen(Qt::black, 1, Qt::SolidLine));
                    tip_painter.drawRect(QRect(command->points()[0] - QPoint(2, 2), command->points()[0] + QPoint(2, 2)));
                    tip_painter.drawRect(QRect(command->points()[1] - QPoint(2, 2), command->points()[1] + QPoint(2, 2)));

                    tip_painter.end();
                }
                break;
            }

            case Command::DRAW_CURVE:
            {
                edit_painter.setRenderHint(QPainter::Antialiasing, true);
                auto points = command->points();
                for(size_t i = 0; i < points.size() - 1; ++i) {
                    edit_painter.drawLine(points[i], points[i + 1]);
                }

                edit_painter.end();
                break;
            }

            case Command::DRAW_TEXT:
            {
                edit_painter.setRenderHint(QPainter::Antialiasing, false);
                auto edit = reinterpret_cast<TextEdit*>(command->widget());
                edit_painter.setFont(edit->font());
                edit_painter.drawText(edit->geometry() + QMargins(-4, -4, 0, 0), Qt::TextWordWrap, edit->toPlainText());
                break;
            }

            default: break;
            }
        }
    }

    captured_image_ = background.copy(area);
    painter_.drawPixmap(0, 0, background);
    painter_.fillRect(rect(), mask_color_);
    painter_.drawPixmap(area.topLeft(), captured_image_);

    painter_.end();
    Selector::paintEvent(event);
}

void ScreenShoter::getArrowPoints(QPoint begin, QPoint end, QPoint* points)
{
    double par = 10.0;
    double par2 = 27.0;
    double slopy = atan2((end.y() - begin.y()), (end.x() - begin.x()));
    double alpha = 30 * 3.14 / 180;

    points[1] = QPoint(end.x() - int(par * cos(alpha + slopy)) - int(9 * cos(slopy)), end.y() - int(par*sin(alpha + slopy)) - int(9 * sin(slopy)));
    points[5] = QPoint(end.x() - int(par * cos(alpha - slopy)) - int(9 * cos(slopy)), end.y() + int(par*sin(alpha - slopy)) - int(9 * sin(slopy)));

    points[2] = QPoint(end.x() - int(par2 * cos(alpha + slopy)), end.y() - int(par2*sin(alpha + slopy)));
    points[4] = QPoint(end.x() - int(par2 * cos(alpha - slopy)), end.y() + int(par2*sin(alpha - slopy)));

    points[0] = begin;
    points[3] = end;
}


void ScreenShoter::save_image()
{
    auto filename = QFileDialog::getSaveFileName(this,
                                                 tr("Save Image"),
                                                 QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
                                                 tr("Image Files (*.png *.jpeg *.jpg *.bmp)"));

    if(!filename.isEmpty()) {
        captured_image_.save(filename);
        CAPTURE_SCREEN_DONE(captured_image_);
        exit();
    }
}

void ScreenShoter::copy2clipboard()
{
    QApplication::clipboard()->setPixmap(captured_image_);
    CAPTURE_SCREEN_DONE(captured_image_);

    exit();
}

void ScreenShoter::pin_image()
{
    FIX_IMAGE(captured_image_);
    CAPTURE_SCREEN_DONE(captured_image_);

    exit();
}

void ScreenShoter::registerShortcuts()
{
    auto save = new QShortcut(Qt::CTRL + Qt::Key_S, this);
    connect(save, &QShortcut::activated, [this](){
        if(status_ == CAPTURED) {
            save_image();
        }
    });

    auto pin = new QShortcut(Qt::Key_P, this);
    connect(pin, &QShortcut::activated, [this](){
        if(status_ == CAPTURED) {
            pin_image();
        }
    });
}

///////////////////////////////////////////////////// UNDO / REDO ////////////////////////////////////////////////////////////
void ScreenShoter::undo()
{
    if(undo_stack_.empty()) return;

    redo_stack_.push(undo_stack_.back());
    undo_stack_.pop();
}

void ScreenShoter::redo()
{
    if(redo_stack_.empty()) return;

    undo_stack_.push(redo_stack_.back());
    redo_stack_.pop();
}