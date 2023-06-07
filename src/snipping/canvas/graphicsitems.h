#ifndef CAPTURER_GRAPHICS_ITEMS_H
#define CAPTURER_GRAPHICS_ITEMS_H

#include "command.h"
#include "resizer.h"
#include "types.h"

#include <QBrush>
#include <QGraphicsItem>
#include <QPen>

class GraphicsItemWrapper
{
public:
    virtual canvas::graphics_t graph() const = 0;

    // getter
    virtual QPen pen() const { return Qt::NoPen; }

    virtual QBrush brush() const { return Qt::NoBrush; }

    virtual bool filled() const { return false; }

    virtual QFont font() const { return {}; }

    // setter
    virtual void setPen(const QPen&) {}

    virtual void setBrush(const QBrush&) {}

    virtual void setFont(const QFont&) {}

    virtual void fill(bool) {}

    // push or update points
    virtual void push(const QPointF&) = 0;

    //
    virtual bool invalid() const = 0;

    //
    virtual ResizerLocation location(const QPointF&) const;

    // rotate
    virtual qreal angle() const { return angle_; }

    virtual void rotate(qreal) {}

    // resize
    virtual ResizerF geometry() const { return geometry_; }

    virtual void resize(const ResizerF&, ResizerLocation) {}

    // callbacks
    virtual void onhovered(const std::function<void(ResizerLocation)>& fn) { onhover = fn; }

    virtual void onfocused(const std::function<void()>& fn) { onfocus = fn; }

    virtual void onblurred(const std::function<void()>& fn) { onblur = fn; }

    virtual void onresized(const std::function<void(const ResizerF&, ResizerLocation)>& fn)
    {
        onresize = fn;
    }

    virtual void onmoved(const std::function<void(const QPointF&)>& fn) { onmove = fn; }

    virtual void onrotated(const std::function<void(qreal)>& fn) { onrotate = fn; }

protected:
    // events
    void focus(QFocusEvent *);
    void blur(QFocusEvent *);
    void hover(QGraphicsSceneHoverEvent *);
    void hoverLeave(QGraphicsSceneHoverEvent *);

    void mousePress(QGraphicsSceneMouseEvent *, const QPointF& pos);
    void mouseMove(QGraphicsSceneMouseEvent *, const QPointF& center);
    void mouseRelease(QGraphicsSceneMouseEvent *, const QPointF& pos);

    // callbacks
    std::function<void(ResizerLocation)> onhover                   = [](auto) {};
    std::function<void()> onfocus                                  = []() {};
    std::function<void()> onblur                                   = []() {};
    std::function<void(const ResizerF&, ResizerLocation)> onresize = [](auto, auto) {};
    std::function<void(const QPointF&)> onmove                     = [](auto) {};
    std::function<void(qreal)> onrotate                            = [](auto) {};

    // hovering
    ResizerLocation hover_location_ = ResizerLocation::DEFAULT;

    //
    canvas::adjusting_t adjusting_ = canvas::adjusting_t::none;

    // rotation
    qreal angle_{};

    // used by moving / rotating
    QPointF mpos_{};

    //
    ResizerF geometry_{ 0.0, 0.0, 0.0, 0.0, 12.0 };

    // for recording adjusted command
    ResizerF before_resizing_{}; // geometry
    qreal before_rotating_{};    // angle
    QPointF before_moving_{};    // position
};

class GraphicsItem : public GraphicsItemWrapper, public QGraphicsItem
{
public:
    explicit GraphicsItem(QGraphicsItem * = nullptr);

    QRectF boundingRect() const override;

    //
    void resize(const ResizerF&, ResizerLocation) override;

    // getter
    QPen pen() const override;

    // setter
    void setPen(const QPen&) override;

protected:
    void focusInEvent(QFocusEvent *) override;
    void focusOutEvent(QFocusEvent *) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *) override;

    void mousePressEvent(QGraphicsSceneMouseEvent *) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *) override;

    QPen pen_{ Qt::red, 3, Qt::SolidLine };
};

/// Line Item

class GraphicsLineItem : public GraphicsItem
{
public:
    explicit GraphicsLineItem(QGraphicsItem * = nullptr);
    explicit GraphicsLineItem(const QPointF&, const QPointF&, QGraphicsItem * = nullptr);

    // QGraphicsItem
    QPainterPath shape() const override;

    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget * = nullptr) override;

    //
    void setVertexes(const QPointF&, const QPointF&);

    // GraphicsItemWrapper
    canvas::graphics_t graph() const override { return canvas::graphics_t::line; }

    bool invalid() const override { return QLineF(geometry_.point1(), geometry_.point2()).length() < 4; }

    void push(const QPointF&) override;

    ResizerLocation location(const QPointF&) const override;
};

/// Arrow Item

class GraphicsArrowItem : public GraphicsItem
{
public:
    explicit GraphicsArrowItem(QGraphicsItem * = nullptr);
    explicit GraphicsArrowItem(const QPointF&, const QPointF&, QGraphicsItem * = nullptr);

    // QGraphicsItem
    QPainterPath shape() const override;

    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget * = nullptr) override;

    //
    void setVertexes(const QPointF&, const QPointF&);

    canvas::graphics_t graph() const override { return canvas::graphics_t::arrow; }

    void push(const QPointF&) override;

    bool invalid() const override { return QLineF(polygon_[0], polygon_[3]).length() < 25; }

    ResizerLocation location(const QPointF&) const override;

    void resize(const ResizerF&, ResizerLocation) override;

private:
    QPolygonF polygon_{ 6 };
};

/// Rectangle Item

class GraphicsRectItem : public GraphicsItem
{
public:
    explicit GraphicsRectItem(QGraphicsItem * = nullptr);
    explicit GraphicsRectItem(const QPointF&, const QPointF&, QGraphicsItem * = nullptr);

    // QGraphicsItem
    QPainterPath shape() const override;

    bool filled() const override;
    void fill(bool) override;

    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget * = nullptr) override;

    //
    void setVertexes(const QPointF&, const QPointF&);

    // GraphicsItemWrapper
    canvas::graphics_t graph() const override { return canvas::graphics_t::rectangle; }

    QBrush brush() const override;
    void setBrush(const QBrush&) override;

    void push(const QPointF&) override;

    bool invalid() const override { return geometry_.width() * geometry_.height() < 16; }

private:
    QBrush brush_{ Qt::NoBrush };
    bool filled_{};
};

/// Pixmap Item

class GraphicsPixmapItem : public GraphicsItem
{
public:
    explicit GraphicsPixmapItem(const QPixmap&, const QPointF&, QGraphicsItem * = nullptr);

    // QGraphicsItem
    QPainterPath shape() const override;

    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget * = nullptr) override;

    //
    void setPixmap(const QPixmap&);

    // GraphicsItemWrapper
    canvas::graphics_t graph() const override { return canvas::graphics_t::pixmap; }

    void push(const QPointF&) override {}

    bool filled() const override { return true; }

    bool invalid() const override { return pixmap_.size() == QSize{ 0, 0 }; }

    void resize(const ResizerF& g, ResizerLocation) override;

    void rotate(qreal) override;

private:
    QPixmap pixmap_{};
};

/// Ellipse Item

class GraphicsEllipseleItem : public GraphicsItem
{
public:
    explicit GraphicsEllipseleItem(QGraphicsItem * = nullptr);
    explicit GraphicsEllipseleItem(const QPointF&, const QPointF&, QGraphicsItem * = nullptr);

    ~GraphicsEllipseleItem() {}

    // QGraphicsItem
    QPainterPath shape() const override;

    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget * = nullptr) override;

    //
    void setVertexes(const QPointF&, const QPointF&);

    // GraphicsItemWrapper
    canvas::graphics_t graph() const override { return canvas::graphics_t::ellipse; }

    QBrush brush() const override;
    void setBrush(const QBrush&) override;

    bool filled() const override;
    void fill(bool) override;

    void push(const QPointF&) override;

    bool invalid() const override { return geometry_.width() * geometry_.height() < 16; }

    ResizerLocation location(const QPointF&) const override;

private:
    QBrush brush_{ Qt::NoBrush };
    bool filled_{};
};

/// Counter Item

class GraphicsCounterleItem : public GraphicsItem
{
public:
    explicit GraphicsCounterleItem(const QPointF&, int, QGraphicsItem * = nullptr);

    ~GraphicsCounterleItem() {}

    // QGraphicsItem
    QPainterPath shape() const override;

    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget * = nullptr) override;

    //
    void setCounter(int);

    // GraphicsItemWrapper
    canvas::graphics_t graph() const override { return canvas::graphics_t::counter; }

    bool filled() const override { return true; }

    void push(const QPointF&) override {}

    bool invalid() const override { return false; }

    ResizerLocation location(const QPointF&) const override;

private:
    int counter_{};

    static int counter;
    QFont font_{};
};

/// Path Item

class GraphicsPathItem : public GraphicsItem
{
public:
    explicit GraphicsPathItem(const QPointF&, QGraphicsItem * = nullptr);

    ~GraphicsPathItem() {}

    // QGraphicsItem
    QPainterPath shape() const override;

    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget * = nullptr) override;

    //
    void pushVertexes(const QVector<QPointF>&);

    // GraphicsItemWrapper
    canvas::graphics_t graph() const override { return canvas::graphics_t::curve; }

    void push(const QPointF&) override;

    bool invalid() const override { return false; }

    ResizerLocation location(const QPointF&) const override { return ResizerLocation::DEFAULT; }

protected:
    void focusOutEvent(QFocusEvent *) override;

private:
    QPainterPath path_{};
};

class GraphicsCurveItem : public GraphicsPathItem
{
public:
    explicit GraphicsCurveItem(const QPointF&, QGraphicsItem * = nullptr);

    canvas::graphics_t graph() const override { return canvas::graphics_t::curve; }
};

class GraphicsMosaicItem : public GraphicsPathItem
{
public:
    explicit GraphicsMosaicItem(const QPointF&, QGraphicsItem * = nullptr);

    canvas::graphics_t graph() const override { return canvas::graphics_t::mosaic; }
};

class GraphicsEraserItem : public GraphicsPathItem
{
public:
    explicit GraphicsEraserItem(const QPointF&, QGraphicsItem * = nullptr);

    canvas::graphics_t graph() const override { return canvas::graphics_t::eraser; }
};

/// Text Item

class GraphicsTextItem : public GraphicsItemWrapper, public QGraphicsTextItem
{
public:
    explicit GraphicsTextItem(const QPointF&, QGraphicsItem * = nullptr);
    explicit GraphicsTextItem(const QString&, const QPointF&, QGraphicsItem * = nullptr);

    // QGraphicsTextItem
    QRectF boundingRect() const override;
    QPainterPath shape() const override;

    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget * = nullptr) override;

    // GraphicsItemWrapper
    canvas::graphics_t graph() const override { return canvas::graphics_t::text; }

    QPen pen() const override { return QPen(defaultTextColor()); }

    void setPen(const QPen& pen) override { setDefaultTextColor(pen.color()); }

    QFont font() const override { return QGraphicsTextItem::font(); }

    void setFont(const QFont&) override;

    void push(const QPointF&) override {}

    bool invalid() const override { return toPlainText().isEmpty(); }

    ResizerLocation location(const QPointF&) const override;

    //
    QRectF paddingRect() const;

    //
    void resize(const ResizerF&, ResizerLocation) override;

    void rotate(qreal) override;

    ResizerF geometry() const override { return ResizerF{ QGraphicsTextItem::boundingRect() }; }

protected:
    void focusInEvent(QFocusEvent *) override;
    void focusOutEvent(QFocusEvent *) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *) override;

private:
    const float padding = 5;
};

#endif //! CAPTURER_GRAPHICS_ITEMS_H