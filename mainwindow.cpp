#include "mainwindow.h"

#include <QElapsedTimer>
#include <QGraphicsItem>
#include <QGraphicsProxyWidget>
#include <QPainter>
#include <QVector2D>
#include <algorithm>

#include "ui_mainwindow.h"

QPoint MainWindow::random_point(QRect rect) {
    int x0, y0, x1, y1;
    rect.getCoords(&x0, &y0, &x1, &y1);
    return QPoint(rng.bounded(x0, x1), rng.bounded(y0, y1));
}

QPolygon MainWindow::random_polygon(QRect rect, int n) {
    QPolygon polygon;
    for (int i = 0; i < n; ++i) {
        polygon << random_point(rect);
    }
    return polygon;
}

int det(QPoint col0, QPoint col1) {
    return col0.x() * col1.y() - col0.y() * col1.x();
}

std::optional<QPointF> segm_line_intersection(QLine s, QLine l) {
    QPoint p0 = s.p1();
    QPoint p1 = s.p2();
    QPoint q0 = l.p1();
    QPoint q1 = l.p2();
    QPoint p1_p0 = p1 - p0;
    QPoint q1_q0 = q1 - q0;
    int d = det(q1_q0, -p1_p0);
    if (d == 0) {
        return std::nullopt;
    }

    QPoint p0_q0 = p0 - q0;
    int d1 = det(q1_q0, p0_q0);
    if (d * d1 < 0 || std::abs(d1) > std::abs(d)) {
        return std::nullopt;
    }

    double t = static_cast<double>(d1) / d;
    return QPointF(p0) + t * QPointF(p1_p0);
}

QLine horiz_line(int y) {
    return {QPoint(0, y), QPoint(1, y)};
}

std::optional<QPointF> segm_horizline_intersection(QLine s, int y) {
    int p0y = s.p1().y();
    int p1y = s.p2().y();
    if (p0y == p1y) {
        return std::nullopt;
    }
    double t = static_cast<double>(y - p0y) / (p1y - p0y);
    if (t < 0.0 || t > 1) {
        return std::nullopt;
    } else {
        return QPointF(s.p1()) + t * QPointF(s.p2() - s.p1());
    }
}

QList<double> polygon_horiz_intersections(const QList<QLine> &edges, int y) {
//    auto horizlile = horiz_line(y);
    QList<double> res;
    res.reserve(2);
    auto prev_edge = edges.last();
    for (int i = 0; i < edges.size(); ++i) {
        auto cur_edge = edges[i];
//        auto ointer = segm_line_intersection(cur_edge, horizlile);
        auto ointer = segm_horizline_intersection(cur_edge, y);
        if (ointer.has_value()) {
            if (cur_edge.p1().y() == y) {
                auto p0 = prev_edge.p1();
                auto p1 = cur_edge.p1();
                auto p2 = cur_edge.p2();
                if ((p1.y() - p0.y()) * (p2.y() - p1.y()) <= 0) {
                    res.push_back(ointer.value().x());
                }
            } else {
                res.push_back(ointer.value().x());
            }
        }
        prev_edge = cur_edge;
    }
    std::sort(res.begin(), res.end());
    return res;
}

void draw_polygon(QImage &image, const QPolygon &poly,
                  std::function<QRgb()> rgb) {
    QList<QLine> edges;
    edges.reserve(poly.size());
    edges.push_back(QLine(poly.last(), poly.first()));
    for (int i = 1; i < poly.size(); ++i) {
        edges.push_back(QLine(poly[i - 1], poly[i]));
    }

    QRgb *bits = reinterpret_cast<QRgb *>(image.bits());
    QRect brect = poly.boundingRect();
    for (int y = brect.top(); y < brect.bottom(); ++y) {
        auto xinters = polygon_horiz_intersections(edges, y);
        for (int i = 0; i < xinters.size(); i += 2) {
            int xbeg = std::max(static_cast<int>(xinters[i] + 0.5), 0);
            int xend =
                std::min(static_cast<int>(xinters[i + 1] + 1.5), image.width());
            int start = y * image.width();
            for (int x = xbeg; x < xend; ++x) {
                bits[start + x] = rgb();
            }
        }
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    scene = new QGraphicsScene();

    ui->graphicsView->setScene(scene);
    ui->graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::on_gen_btn_clicked() {
    QElapsedTimer t;
    t.start();

    QImage image(ui->graphicsView->size(), QImage::Format_RGB32);
    image.fill(Qt::white);

    auto poly = random_polygon(QRect({0, 0}, ui->graphicsView->size()),
                               ui->n_sbox->value());
    draw_polygon(image, poly, [this]() { return rng.generate(); });
    scene->clear();
    scene->setSceneRect(ui->graphicsView->rect());
    scene->addPixmap(QPixmap::fromImage(image))->setPos(0, 0);
    auto label = ui->time_label;
    label->setText(label->text().split(':')[0] + ": " +
                   QString::number(t.elapsed()) + " ms");
}

void MainWindow::on_gen_builtin_btn_clicked() {
    QElapsedTimer t;
    t.start();
    auto poly = random_polygon(QRect({0, 0}, ui->graphicsView->size()),
                               ui->n_sbox->value());
    QImage image(ui->graphicsView->size(), QImage::Format_RGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    QColor color = QColor::fromRgb(rng.generate());
    QBrush brush(color);
    QPen pen(Qt::black);
    painter.setBrush(brush);
    painter.setPen(pen);
    painter.drawPolygon(poly);
    painter.end();
    scene->clear();
    scene->setSceneRect(ui->graphicsView->rect());
    scene->addPixmap(QPixmap::fromImage(image))->setPos(0, 0);
    auto label = ui->builtin_time_label;
    label->setText(label->text().split(':')[0] + ": " +
                   QString::number(t.elapsed()) + " ms");
}

void MainWindow::on_thous_gen_btn_clicked() {
    QElapsedTimer t;
    t.start();

    QImage image(ui->graphicsView->size(), QImage::Format_RGB32);
    image.fill(Qt::white);

    for (int poly_i = 0; poly_i < 1000; ++poly_i) {
        auto poly = random_polygon(QRect({0, 0}, ui->graphicsView->size()),
                                   ui->n_sbox->value());
        draw_polygon(image, poly, [this]() { return rng.generate(); });
    }

    scene->clear();
    scene->setSceneRect(ui->graphicsView->rect());
    scene->addPixmap(QPixmap::fromImage(image))->setPos(0, 0);
    auto label = ui->time_label;
    label->setText(label->text().split(':')[0] + ": " +
                   QString::number(t.elapsed()) + " ms");
}

void MainWindow::on_thous_gen_builtin_btn_clicked() {
    QElapsedTimer t;
    t.start();
    QImage image(ui->graphicsView->size(), QImage::Format_RGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    QPen pen(Qt::black);
    painter.setPen(pen);
    scene->setSceneRect(ui->graphicsView->rect());
    scene->clear();
    for (int i = 0; i < 1000; ++i) {
        auto poly = random_polygon(QRect({0, 0}, ui->graphicsView->size()),
                                   ui->n_sbox->value());
        QColor color = QColor::fromRgb(rng.generate());
        QBrush brush(color);
        painter.setBrush(brush);
        painter.drawPolygon(poly);
    }
    painter.end();
    scene->addPixmap(QPixmap::fromImage(image))->setPos(0, 0);
    auto label = ui->builtin_time_label;
    label->setText(label->text().split(':')[0] + ": " +
                   QString::number(t.elapsed()) + " ms");
}

void MainWindow::on_gen_gl_btn_clicked() {
    QElapsedTimer t;
    t.start();
    auto poly = random_polygon(QRect({0, 0}, ui->graphicsView->size()),
                               ui->n_sbox->value());
    QColor color = QColor::fromRgb(rng.generate());
    QBrush brush(color);
    QPen pen(Qt::black);
    QPainter painter(ui->openGLWidget);
    painter.fillRect(ui->openGLWidget->rect(), QBrush(Qt::white));
    painter.setBrush(brush);
    painter.setPen(pen);
    painter.drawPolygon(poly);
    painter.end();
    auto label = ui->gl_time_label;
    label->setText(label->text().split(':')[0] + ": " +
                   QString::number(t.elapsed()) + " ms");
}

void MainWindow::on_thous_gen_gl_btn_clicked() {
    QElapsedTimer t;
    t.start();
    QPainter painter(ui->openGLWidget);
    painter.fillRect(ui->openGLWidget->rect(), QBrush(Qt::white));
    QPen pen(Qt::black);
    for (int i = 0; i < 1000; ++i) {
        auto poly = random_polygon(QRect({0, 0}, ui->graphicsView->size()),
                                   ui->n_sbox->value());
        QColor color = QColor::fromRgb(rng.generate());
        QBrush brush(color);
        painter.setPen(pen);
        painter.setBrush(brush);
        painter.drawPolygon(poly);
    }
    painter.end();
    auto label = ui->gl_time_label;
    label->setText(label->text().split(':')[0] + ": " +
                   QString::number(t.elapsed()) + " ms");
}
